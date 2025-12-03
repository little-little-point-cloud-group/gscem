#pragma once
#include "common/FXPoint.h"
#include <math.h>
#include <common/AttributePredictor.h>
#include <queue>
//============================================================================
void fwdTransform2Node(int numBufs, FXPoint buf[][2]) {
  FXPoint inputx[2] = {};
  for (int attri = 0; attri < numBufs; attri++) {
    inputx[0] = buf[attri][0];
    inputx[0] += buf[attri][1];
    inputx[0] /= FXPoint(1.41421356);
    inputx[1] = buf[attri][0];
    inputx[1] -= buf[attri][1];
    inputx[1] /= FXPoint(1.41421356);
    buf[attri][0] = inputx[0];
    buf[attri][1] = inputx[1];
  }
}

//============================================================================

void invTransform2Node(int numBufs, FXPoint buf[][2]) {
  FXPoint inputx[2] = {};
  for (int attri = 0; attri < numBufs; attri++) {
    inputx[0] = buf[attri][0];
    inputx[0] += buf[attri][1];
    inputx[0] >>= 1;
    inputx[1] = buf[attri][0];
    inputx[1] -= buf[attri][1];
    inputx[1] >>= 1;
    buf[attri][0] = inputx[0];
    buf[attri][1] = inputx[1];
  }
}

//=============================================================================

int64_t recQuantValue(int64_t detail, const UInt8 transQuantParam, const int offsetAdd = 0) {
  int residualSign = detail < 0 ? -1 : 1;
  uint64_t absResidual = std::abs(detail);
  uint64_t residualQuant = QuantizaResidual(absResidual, transQuantParam, offsetAdd);
  int64_t output;
  output = residualSign * InverseQuantizeResidual(residualQuant, transQuantParam);
  return output;
};
int64_t fwdQuantValue(int64_t detail, const UInt8 transQuantParam, const int offsetAdd = 0) {
  int residualSign = detail < 0 ? -1 : 1;
  uint64_t absResidual = std::abs(detail);
  uint64_t residualQuant = QuantizaResidual(absResidual, transQuantParam, offsetAdd);
  int64_t output = residualSign * residualQuant;
  return output;
};
int64_t invQuantValue(int64_t detail, const UInt8 transQuantParam, const int offsetAdd = 0) {
  int residualSign = detail < 0 ? -1 : 1;
  uint64_t absResidual = std::abs(detail);
  int64_t output = residualSign * InverseQuantizeResidual(absResidual, transQuantParam);
  return output;
};

V3<FXPoint> getColorDCPredictor(int idx, FXPoint* attributes, int* positions,
                                bool* NodePredFlag = NULL, int bottom_idx = 0, int top_idx = 0,
                                const SequenceParameterSet& sps = {}) {
  UInt k = 3;
  V3<FXPoint> predictorColor;
  V3<FXPoint> lastPointColor1;
  V3<FXPoint> lastPointColor2;
  V3<FXPoint> lastPointColor3;
  int axisBias = 1;
  auto attrQP = sps.colorQuantParam;
  if (idx == bottom_idx) {
    predictorColor = {128, 128, 128};
  } else if (idx == bottom_idx - 1) {
    auto last1PointIndex = idx + 1;
    for (int ii = 0; ii < 3; ii++)
      predictorColor[ii] = attributes[last1PointIndex * 3 + ii];
  } else if (idx == bottom_idx - 2) {
    auto lastPointIndex1 = idx + 1;
    auto lastPointIndex2 = idx + 2;
    PC_POS curPos, neighborPos1, neighborPos2;
    for (int ii = 0; ii < 3; ii++) {
      lastPointColor1[ii] = attributes[lastPointIndex1 * 3 + ii];
      lastPointColor2[ii] = attributes[lastPointIndex2 * 3 + ii];
    }
    for (int kk = 0; kk < 3; kk++) {
      curPos[kk] = positions[idx * 3 + kk];
      neighborPos1[kk] = positions[lastPointIndex1 * 3 + kk];
      neighborPos2[kk] = positions[lastPointIndex2 * 3 + kk];
    }

    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);

    int64_t sumW = w1 + w2;
    assert(w1 + w2 > 0);  // assume no duplicate point
    if (w1 + w2 > 0)
      for (int ii = 0; ii < 3; ii++) {
        //auto tmp = (w2 * lastPointColor1[ii].round() + w1 * lastPointColor2[ii].round()) / sumW;
        //predictorColor[ii]= FXPoint(tmp);
        predictorColor[ii] = w2 * lastPointColor1[ii].round() + w1 * lastPointColor2[ii].round();
        predictorColor[ii] /= FXPoint(sumW);
      }
    else
      for (int ii = 0; ii < 3; ii++)
        predictorColor[ii] = attributes[lastPointIndex1 * 3 + ii];
  } else {
    PC_POS curPos, neiPos;
    for (int kk = 0; kk < 3; kk++) {
      curPos[kk] = positions[idx * 3 + kk];
    }
    int minBegin = std::max(int(idx - 128), top_idx);
    int maxEnd = std::min(int(idx + 128), bottom_idx);
    // Find three nearest neighbors
    int64_t dis;
    PredictedNearsetNeighbors<PredictedNeighborInfo> neis;
    neis.init(3);
    for (int i = maxEnd; i >= minBegin; i--) {
      if (NodePredFlag[i] == false || i > idx) {
        for (int kk = 0; kk < 3; kk++) {
          neiPos[kk] = positions[i * 3 + kk];
        }
        dis = getDisNorm1(curPos, neiPos, axisBias);
        neis.insertNeighor(i, dis);
      }
    }
    auto w3 = neis.getNeigbors(2).weight;
    auto lastPointIndex3 = neis.getNeigbors(2).predictorIndex;

    auto w2 = neis.getNeigbors(1).weight;
    auto lastPointIndex2 = neis.getNeigbors(1).predictorIndex;

    auto w1 = neis.getNeigbors(0).weight;
    auto lastPointIndex1 = neis.getNeigbors(0).predictorIndex;

    V3<FXPoint> color1, color2, color3, lastcolor;
    for (int ii = 0; ii < 3; ii++) {
      color1[ii] = attributes[lastPointIndex1 * 3 + ii];
      color2[ii] = attributes[lastPointIndex2 * 3 + ii];
      color3[ii] = attributes[lastPointIndex3 * 3 + ii];
    }
    if (w1 == 0)
      return color1;

    for (int k = 0; k < 3; k++) {
      lastcolor[k] = color3[k];
    }
    int count = std::min((neis.getIndexesCount() - 3), 13);

    for (int j = 3; j < (count + 3); j++) {
      for (int k = 0; k < 3; k++) {
        lastcolor[k] += attributes[neis.getNeigbors(j).predictorIndex * 3 + k];
      }
    }
    int64_t W = w1 * w2 + w2 * w3 + w1 * w3;
    int64_t dw;

    if (w3 > w2) {
      int64_t dw1 = w2 * w3;
      int64_t dw2 = w1 * w3;
      int64_t dw3 = w1 * w2;
      if (attrQP == 0) {
        dw = (dw1 + dw2 + dw3) * (count + 1);
        for (int k = 0; k < 3; ++k) {
          int64_t tmp =
            (int64_t)(dw1 * color1[k].round() * (count + 1) +
                      dw2 * color2[k].round() * (count + 1) + dw3 * lastcolor[k].round()) /
            dw;
          predictorColor[k] = FXPoint(tmp);
        }
      } else {
        dw = dw1 + dw2 + dw3 * (count + 1);
        for (int k = 0; k < 3; ++k) {
          int64_t tmp = (int64_t)(dw1 * color1[k].round() + dw2 * color2[k].round() +
                                  dw3 * lastcolor[k].round()) /
            dw;
          predictorColor[k] = FXPoint(tmp);
        }
      }
    } else if ((w3 == w2) && (w2 > w1)) {
      if (attrQP == 0) {
        dw = (w1 + w2) * (count + 2);
        for (int k = 0; k < 3; ++k) {
          int64_t tmp = (int64_t)(w2 * color1[k].round() * (count + 2) +
                                  w1 * (color2[k].round() + lastcolor[k].round())) /
            dw;
          predictorColor[k] = FXPoint(tmp);
        }
      } else {
        dw = (w1 * (count + 2) + w2);
        for (int k = 0; k < 3; ++k) {
          int64_t tmp =
            (int64_t)(w2 * color1[k].round() + w1 * (color2[k].round() + lastcolor[k].round())) /
            dw;
          predictorColor[k] = FXPoint(tmp);
        }
      }
    } else {
      for (int k = 0; k < 3; ++k) {
        int64_t tmp =
          (int64_t)(color1[k].round() + color2[k].round() + lastcolor[k].round()) / (count + 3);
        predictorColor[k] = FXPoint(tmp);
      }
    }
  }

  return predictorColor;
}

FXPoint getReflectanceDCPredictor(int idx, FXPoint* attributes, int* positions,
    bool* NodePredFlag = NULL, int bottom_idx = 0, int top_idx = 0,
    int axisBias = 1, int predFixedPointFracBit = 0) {
    FXPoint predRef;
    if (idx == bottom_idx) {
        predRef = 0;
    }
    else if (idx == bottom_idx - 1) {
        predRef = attributes[idx + 1];
    }
    else if (idx == bottom_idx - 2) {
        auto last1PointIndex = idx + 1;
        auto last2PointIndex = idx + 2;
        PC_POS curPoint, last1Point, last2Point;
        for (int kk = 0; kk < 3; kk++) {
            curPoint[kk] = positions[idx * 3 + kk];

            last1Point[kk] = positions[last1PointIndex * 3 + kk];
            last2Point[kk] = positions[last2PointIndex * 3 + kk];
        }
        const int64_t weight1 = (abs(curPoint[0] - last1Point[0]) + abs(curPoint[1] - last1Point[1]) +
            axisBias * abs(curPoint[2] - last1Point[2]));
        const int64_t weight2 = (abs(curPoint[0] - last2Point[0]) + abs(curPoint[1] - last2Point[1]) +
            axisBias * abs(curPoint[2] - last2Point[2]));

        auto last1Reflectance = attributes[last1PointIndex];
        auto last2Reflectance = attributes[last2PointIndex];
        int64_t sumW = weight1 + weight2;
        int64_t minW = min(weight1, weight2);
        //assert(sumW > 0);  // assume no duplicate point
        if (minW > 0) {
            if (predFixedPointFracBit == 0) {
                int64_t predRefup = weight2 * last1Reflectance.round() + weight1 * last2Reflectance.round();
                predRef = FXPoint(predRefup / sumW);
            }
            else {
                int64_t distanceScale = (1 << predFixedPointFracBit) - 1;
                int64_t up, bottom, sw;
                sw = (distanceScale + (weight1 >> 1)) / weight1;
                up = last1Reflectance.round() * sw;
                bottom = sw;
                sw = (distanceScale + (weight2 >> 1)) / weight2;
                up += last2Reflectance.round() * sw;
                bottom += sw;
                predRef = FXPoint(up / bottom);
            }
        }
        else
            predRef = last1Reflectance;
    }
    else {
        PC_POS curPos, neiPos;
        for (int kk = 0; kk < 3; kk++) {
            curPos[kk] = positions[idx * 3 + kk];
        }
        int minBegin = std::max(int(idx - 128), top_idx);
        int maxEnd = std::min(int(idx + 128), bottom_idx);
        // Find three nearest neighbors
        int64_t dis;
        std::priority_queue<pair<int64_t, int32_t>> neis;
        for (int i = maxEnd; i >= minBegin; i--) {
            if (NodePredFlag[i] == false || i > idx) {
                for (int kk = 0; kk < 3; kk++) {
                    neiPos[kk] = positions[i * 3 + kk];
                }
                dis = getDisNorm1(curPos, neiPos, axisBias);
                if (neis.size() < 3) {
                    neis.push({ dis, i });
                }
                else {
                    if (dis < neis.top().first) {
                        neis.pop();
                        neis.push({ dis, i });
                    }
                }
            }
        }

        //Compute prediction value
        auto w1 = neis.top().first;
        auto& ref1 = attributes[neis.top().second];
        neis.pop();

        auto w2 = neis.top().first;
        auto& ref2 = attributes[neis.top().second];
        neis.pop();

        auto w3 = neis.top().first;
        auto& ref3 = attributes[neis.top().second];

        auto sumW = w1 * w2 + w2 * w3 + w1 * w3;
        //assert(sumW > 0);  // assume no duplicate point
        if (w3 > 0) {
            if (predFixedPointFracBit == 0) {
                predRef =
                    FXPoint((w2 * w3 * ref1.round() + w1 * w3 * ref2.round() + w1 * w2 * ref3.round()) / sumW);

            }
            else {
                int64_t distanceScale = (1 << predFixedPointFracBit) - 1;
                int64_t up, bottom, sw;
                sw = (distanceScale + (w1 >> 1)) / w1;
                up = ref1.round() * sw;
                bottom = sw;
                sw = (distanceScale + (w2 >> 1)) / w2;
                up += ref2.round() * sw;
                bottom += sw;
                sw = (distanceScale + (w3 >> 1)) / w3;
                up += ref3.round() * sw;
                bottom += sw;
                predRef = FXPoint(up / bottom);
            }
        }
        else
            predRef = ref3;
    }

    return predRef;
}

#define Norm1(p, q, z) (abs(p[0] - q[0]) + abs(p[1] - q[1]) + z * abs(p[2] - q[2]));
V3<FXPoint> getColorDCPredictorFromRefl(int idx, FXPoint* attributes, int* positions, int* reflectances,
    bool* NodePredFlag = NULL, int bottom_idx = 0, int top_idx = 0,
    const SequenceParameterSet& sps = {}) {
    UInt k = 3;
    V3<FXPoint> predictorColor;
    V3<FXPoint> lastPointColor1;
    V3<FXPoint> lastPointColor2;
    V3<FXPoint> lastPointColor3;
    int axisBias = 1;
    auto attrQP = sps.colorQuantParam;
    if (idx == bottom_idx) {
        predictorColor = { 128, 128, 128 };
    }
    else if (idx == bottom_idx - 1) {
        auto last1PointIndex = idx + 1;
        for (int ii = 0; ii < 3; ii++)
            predictorColor[ii] = attributes[last1PointIndex * 3 + ii];
    }
    else if (idx == bottom_idx - 2) {
        auto lastPointIndex1 = idx + 1;
        auto lastPointIndex2 = idx + 2;
        PC_POS curPos, neighborPos1, neighborPos2;
        for (int ii = 0; ii < 3; ii++) {
            lastPointColor1[ii] = attributes[lastPointIndex1 * 3 + ii];
            lastPointColor2[ii] = attributes[lastPointIndex2 * 3 + ii];
        }
        for (int kk = 0; kk < 3; kk++) {
            curPos[kk] = positions[idx * 3 + kk];
            neighborPos1[kk] = positions[lastPointIndex1 * 3 + kk];
            neighborPos2[kk] = positions[lastPointIndex2 * 3 + kk];
        }

        auto w1 = Norm1(curPos, neighborPos1, 1);
        auto w2 = Norm1(curPos, neighborPos2, 1);

        int64_t sumW = w1 + w2;
        assert(w1 + w2 > 0);  // assume no duplicate point
        if (w1 + w2 > 0)
            for (int ii = 0; ii < 3; ii++) {
                //auto tmp = (w2 * lastPointColor1[ii].round() + w1 * lastPointColor2[ii].round()) / sumW;
                //predictorColor[ii]= FXPoint(tmp);
                predictorColor[ii] = w2 * lastPointColor1[ii].round() + w1 * lastPointColor2[ii].round();
                predictorColor[ii] /= FXPoint(sumW);
            }
        else
            for (int ii = 0; ii < 3; ii++)
                predictorColor[ii] = attributes[lastPointIndex1 * 3 + ii];
    }
    else {
        PC_POS curPos, neiPos;
        for (int kk = 0; kk < 3; kk++) {
            curPos[kk] = positions[idx * 3 + kk];
        }
        int minBegin = std::max(int(idx - 128), top_idx);
        int maxEnd = std::min(int(idx + 128), bottom_idx);
        // Find three nearest neighbors
        int64_t dis, distPos, distRef;
        PredictedNearsetNeighbors<PredictedNeighborInfo> neis;
        neis.init(3);
        for (int i = maxEnd; i >= minBegin; i--) {
            if (NodePredFlag[i] == false || i > idx) {
                for (int kk = 0; kk < 3; kk++) {
                    neiPos[kk] = positions[i * 3 + kk];
                }
                distPos = Norm1(curPos, neiPos, axisBias);
                distRef = abs((int64_t)(reflectances[idx] - reflectances[i])) >> 10;
                dis = distPos + distRef;
                neis.insertNeighor(i, dis);
            }
        }
        auto w3 = neis.getNeigbors(2).weight;
        auto lastPointIndex3 = neis.getNeigbors(2).predictorIndex;

        auto w2 = neis.getNeigbors(1).weight;
        auto lastPointIndex2 = neis.getNeigbors(1).predictorIndex;

        auto w1 = neis.getNeigbors(0).weight;
        auto lastPointIndex1 = neis.getNeigbors(0).predictorIndex;

        V3<FXPoint> color1, color2, color3, lastcolor;
        for (int ii = 0; ii < 3; ii++) {
            color1[ii] = attributes[lastPointIndex1 * 3 + ii];
            color2[ii] = attributes[lastPointIndex2 * 3 + ii];
            color3[ii] = attributes[lastPointIndex3 * 3 + ii];
        }
        if (w1 == 0)
            return color1;

        for (int k = 0; k < 3; k++) {
            lastcolor[k] = color3[k];
        }
        int count = std::min((neis.getIndexesCount() - 3), 13);

        for (int j = 3; j < (count + 3); j++) {
            for (int k = 0; k < 3; k++) {
                lastcolor[k] += attributes[neis.getNeigbors(j).predictorIndex * 3 + k];
            }
        }
        int64_t W = w1 * w2 + w2 * w3 + w1 * w3;
        int64_t dw;

        if (w3 > w2) {
            int64_t dw1 = w2 * w3;
            int64_t dw2 = w1 * w3;
            int64_t dw3 = w1 * w2;
            if (attrQP == 0) {
              dw = (dw1 + dw2 + dw3) * (count + 1);
              for (int k = 0; k < 3; ++k) {
                int64_t tmp = (int64_t)(dw1 * color1[k].round() * (count + 1) +
                                        dw2 * color2[k].round() * (count + 1) +
                                        dw3 * lastcolor[k].round()) /
                  dw;
                predictorColor[k] = FXPoint(tmp);
              }
            } else {
              dw = dw1 + dw2 + dw3 * (count + 1);
              for (int k = 0; k < 3; ++k) {
                int64_t tmp = (int64_t)(dw1 * color1[k].round() +
                                        dw2 * color2[k].round() +
                                        dw3 * lastcolor[k].round()) /
                  dw;
                predictorColor[k] = FXPoint(tmp);
              }
            }
            
        }
        else if ((w3 == w2) && (w2 > w1)) {
          if (attrQP == 0) {
            dw = (w1 + w2) * (count + 2);
            for (int k = 0; k < 3; ++k) {
              int64_t tmp = (int64_t)(w2 * color1[k].round() * (count + 2) +
                                      w1 * (color2[k].round() + lastcolor[k].round())) /
                dw;
              predictorColor[k] = FXPoint(tmp);
            }
          } else {
            dw = (w1 * (count + 2) + w2);
            for (int k = 0; k < 3; ++k) {
              int64_t tmp = (int64_t)(w2 * color1[k].round() +
                                      w1 * (color2[k].round() + lastcolor[k].round())) /
                dw;
              predictorColor[k] = FXPoint(tmp);
            }
          }
            
        }
        else {
            for (int k = 0; k < 3; ++k) {
                int64_t tmp =
                    (int64_t)(color1[k].round() + color2[k].round() + lastcolor[k].round()) / (count + 3);
                predictorColor[k] = FXPoint(tmp);
            }
        }
    }

    return predictorColor;
}

FXPoint getReflectanceDCPredictorFromColor(int idx, FXPoint* attributes, int* positions, int* colors,
    bool* NodePredFlag = NULL, int bottom_idx = 0, int top_idx = 0,
    int axisBias = 1, int predFixedPointFracBit = 0) {
    FXPoint predRef;
    if (idx == bottom_idx) {
        predRef = 0;
    }
    else if (idx == bottom_idx - 1) {
        predRef = attributes[idx + 1];
    }
    else if (idx == bottom_idx - 2) {
        auto last1PointIndex = idx + 1;
        auto last2PointIndex = idx + 2;
        PC_POS curPoint, last1Point, last2Point;
        for (int kk = 0; kk < 3; kk++) {
            curPoint[kk] = positions[idx * 3 + kk];
            last1Point[kk] = positions[last1PointIndex * 3 + kk];
            last2Point[kk] = positions[last2PointIndex * 3 + kk];
        }

        const int64_t weight1 = (abs(curPoint[0] - last1Point[0]) + abs(curPoint[1] - last1Point[1]) +
            axisBias * abs(curPoint[2] - last1Point[2]));
        const int64_t weight2 = (abs(curPoint[0] - last2Point[0]) + abs(curPoint[1] - last2Point[1]) +
            axisBias * abs(curPoint[2] - last2Point[2]));

        auto last1Reflectance = attributes[last1PointIndex];
        auto last2Reflectance = attributes[last2PointIndex];
        int64_t sumW = weight1 + weight2;
        //assert(sumW > 0);  // assume no duplicate point
        int64_t minW = min(weight1, weight2);
        if (minW > 0) {
            if (predFixedPointFracBit == 0) {
                int64_t predRefup = weight2 * last1Reflectance.round() + weight1 * last2Reflectance.round();
                predRef = FXPoint(predRefup / sumW);
            }
            else {
                int64_t distanceScale = (1 << predFixedPointFracBit) - 1;
                int64_t up, bottom, sw;
                sw = (distanceScale + (weight1 >> 1)) / weight1;
                up = last1Reflectance.round() * sw;
                bottom = sw;
                sw = (distanceScale + (weight2 >> 1)) / weight2;
                up += last2Reflectance.round() * sw;
                bottom += sw;
                predRef = FXPoint(up / bottom);
            }
        }
        else
            predRef = last1Reflectance;
    }
    else {
        PC_POS curPos, neiPos;
        for (int kk = 0; kk < 3; kk++) {
            curPos[kk] = positions[idx * 3 + kk];
        }
        int minBegin = std::max(int(idx - 128), top_idx);
        int maxEnd = std::min(int(idx + 128), bottom_idx);
        // Find three nearest neighbors
        int64_t dis, disPos, disColor;
        int curColorIdx = idx * 3;
        std::priority_queue<pair<int64_t, int32_t>> neis;
        for (int i = maxEnd; i >= minBegin; i--) {
            if (NodePredFlag[i] == false || i > idx) {
                for (int kk = 0; kk < 3; kk++) {
                    neiPos[kk] = positions[i * 3 + kk];
                }
                disPos = Norm1(curPos, neiPos, axisBias);
                disColor = abs(colors[curColorIdx] - colors[i * 3]) +
                    abs(colors[curColorIdx + 1] - colors[i * 3 + 1]) +
                    abs(colors[curColorIdx + 2] - colors[i * 3 + 2]);
                disColor = (disColor >> 10);
                dis = disColor + disPos;
                if (neis.size() < 3) {
                    neis.push({ dis, i });
                }
                else {
                    if (dis < neis.top().first) {
                        neis.pop();
                        neis.push({ dis, i });
                    }
                }
            }
        }

        //Compute prediction value
        auto w1 = neis.top().first;
        auto& ref1 = attributes[neis.top().second];
        neis.pop();

        auto w2 = neis.top().first;
        auto& ref2 = attributes[neis.top().second];
        neis.pop();

        auto w3 = neis.top().first;
        auto& ref3 = attributes[neis.top().second];

        auto sumW = w1 * w2 + w2 * w3 + w1 * w3;
        //assert(sumW > 0);  // assume no duplicate point
        if (w3 > 0) {
            if (predFixedPointFracBit == 0) {
                predRef =
                    FXPoint((w2 * w3 * ref1.round() + w1 * w3 * ref2.round() + w1 * w2 * ref3.round()) / sumW);

            }
            else {
                int64_t distanceScale = (1 << predFixedPointFracBit) - 1;
                int64_t up, bottom, sw;
                sw = (distanceScale + (w1 >> 1)) / w1;
                up = ref1.round() * sw;
                bottom = sw;
                sw = (distanceScale + (w2 >> 1)) / w2;
                up += ref2.round() * sw;
                bottom += sw;
                sw = (distanceScale + (w3 >> 1)) / w3;
                up += ref3.round() * sw;
                bottom += sw;
                predRef = FXPoint(up / bottom);
            }
        }
        else
            predRef = ref3;
    }

    return predRef;
}