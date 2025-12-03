/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2019-2033, Audio Video coding Standard Workgroup of China
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of Audio Video coding Standard Workgroup of China
*    nor the names of its contributors maybe used to endorse or promote products
*    derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "AttributePredictor.h"
#include <queue>
#include <vector>

struct Neighbor {
  int64_t dis;
  int32_t index;
};
bool cmp(Neighbor a, Neighbor b) {
  return a.dis < b.dis;
};

int64_t getDisNorm1(PC_POS& posA, PC_POS& posB, int axisBias) {
  return (int64_t)(abs(posA[0] - posB[0]) + abs(posA[1] - posB[1]) +
                   axisBias * abs(posA[2] - posB[2]));
}
int64_t getDisNorm1(PC_POS& posA, PC_POS& posB, const vector<int>& distWeight) {
  int64_t dist =
    ((uint64_t)((abs(posA[0] - posB[0]) * distWeight[0] + abs(posA[1] - posB[1]) * distWeight[1] +
                 distWeight[2] * abs(posA[2] - posB[2])))) >>
    3;
  return dist > 0 ? dist : 1;
}

PC_COL getColorPredictorFarthest(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                 const SequenceParameterSet& sps, const AttributeParameterSet& aps,
                                 vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
                                 vector<colorNeighborSet>& neighborSet) {
  PC_COL predictorColor;
  int farestpoint = 0;
  int64_t farestlength = 0;
  PC_POS farestPos;
  auto attrQP = sps.colorQuantParam;
  auto pointIndexc = pointCloudCode[curIndex].index;
  auto curPos = outputPointCloud[pointIndexc];
  int setlength = neighborSet.size();
  if (minIdx > setlength) {
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    int64_t dis;
    /*  int32_t indexes[125];*/
    std::vector<int32_t> indexes;
    indexes.resize(aps.maxNumOfNeighbours - 3);
    int count = 0;
    for (int i = 0; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      dis = getDisNorm1(curPos, neighborPos, 1);
      if (dis > farestlength && i >= subGroupCount) {
        farestlength = dis;
        farestpoint = i;
        farestPos = neighborPos;
      }
      if (neis[1].dis > dis) {
        if (neis[2].dis == neis[1].dis && neis[2].dis != INT_FAST64_MAX) {
          indexes[count] = neis[2].index;
          count++;
        } else
          count = 0;
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].dis = dis;
          neis[0].index = i;
          continue;
        }
        neis[2] = neis[1];
        neis[1].dis = dis;
        neis[1].index = i;
        continue;
      }
      if (neis[2].dis > dis) {
        neis[2].dis = dis;
        neis[2].index = i;
        count = 0;
        continue;
      }
      if (neis[2].dis == dis) {
        indexes[count] = i;
        count++;
      }
    }
    auto w3 = neis[2].dis;
    auto lastPointIndex3 = neis[2].index;
    auto color3 = neighborSet[lastPointIndex3].color;
    auto w2 = neis[1].dis;
    auto lastPointIndex2 = neis[1].index;
    auto color2 = neighborSet[lastPointIndex2].color;
    auto w1 = neis[0].dis;
    auto lastPointIndex1 = neis[0].index;
    auto color1 = neighborSet[lastPointIndex1].color;

    V3<int32_t> lastcolor;
    lastcolor[0] = color3[0];
    lastcolor[1] = color3[1];
    lastcolor[2] = color3[2];
    count = std::min(count, 13);
    for (int j = 0; j < count; j++) {
      auto temp = neighborSet[indexes[j]].color;
      lastcolor[0] = lastcolor[0] + temp[0];
      lastcolor[1] = lastcolor[1] + temp[1];
      lastcolor[2] = lastcolor[2] + temp[2];
    }
    int64_t dw;

    if (w3 > w2) {
      uint64_t dw1 = w2 * w3;
      uint64_t dw2 = w1 * w3;
      uint64_t dw3 = w1 * w2;
      int countp1 = count + 1;
      if (attrQP == 0) {
        dw = (dw1 + dw2 + dw3) * (countp1);
        predictorColor[0] = (dw1 * color1[0] * (countp1) + dw2 * color2[0] * (countp1) +
                             dw3 * lastcolor[0] + (dw >> 1)) / dw;
        predictorColor[1] = (dw1 * color1[1] * (countp1) + dw2 * color2[1] * (countp1) +
                             dw3 * lastcolor[1] + (dw >> 1)) / dw;
        predictorColor[2] = (dw1 * color1[2] * (countp1) + dw2 * color2[2] * (countp1) +
                             dw3 * lastcolor[2] + (dw >> 1)) / dw;
      } else {
        dw = dw1 + dw2 + dw3 * countp1;
        predictorColor[0] = (dw1 * color1[0] + dw2 * color2[0] + dw3 * lastcolor[0] + 
			                (dw >> 1)) / dw;
        predictorColor[1] = (dw1 * color1[1] + dw2 * color2[1] + dw3 * lastcolor[1] + 
			                (dw >> 1)) / dw;
        predictorColor[2] = (dw1 * color1[2] + dw2 * color2[2] + dw3 * lastcolor[2] +
			                (dw >> 1)) / dw;
      }
    } else if ((w3 == w2) && (w2 > w1)) {
      int countp2 = count + 2;
      if (attrQP == 0) {
        dw = (w1 + w2) * (countp2);
        predictorColor[0] =
          (w2 * color1[0] * (countp2) + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] =
          (w2 * color1[1] * (countp2) + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] =
          (w2 * color1[2] * (countp2) + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      } else {
        dw = (w1 * countp2 + w2);
        predictorColor[0] = (w2 * color1[0] + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] = (w2 * color1[1] + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] = (w2 * color1[2] + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      }
    } else {
      predictorColor[0] = (color1[0] + color2[0] + lastcolor[0] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[1] = (color1[1] + color2[1] + lastcolor[1] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[2] = (color1[2] + color2[2] + lastcolor[2] + ((count + 3) >> 1)) / (count + 3);
    }
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
  } else if (minIdx == 0) {
    predictorColor = {128, 128, 128};
  } else if (minIdx == 1) {
    predictorColor = neighborSet[0].color;
  } else if (minIdx == 2) {
    auto lastPointColor1 = neighborSet[1].color;
    auto lastPointColor2 = neighborSet[0].color;
    auto neighborPos1 = neighborSet[1].pos;
    auto neighborPos2 = neighborSet[0].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    int64_t sumW = w1 + w2;
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * lastPointColor1[j] + w1 * lastPointColor2[j] + (sumW >> 1)) / sumW;
    }
  } else {
    auto lastPointColor1 = neighborSet[minIdx - 1].color;
    auto lastPointColor2 = neighborSet[minIdx - 2].color;
    auto lastPointColor3 = neighborSet[minIdx - 3].color;
    auto neighborPos1 = neighborSet[minIdx - 1].pos;
    auto neighborPos2 = neighborSet[minIdx - 2].pos;
    auto neighborPos3 = neighborSet[minIdx - 3].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    auto w3 = getDisNorm1(curPos, neighborPos3, 1);
    int64_t sumW = w1 * w2 + w2 * w3 + w1 * w3;
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * w3 * lastPointColor1[j] + w1 * w3 * lastPointColor2[j] +
                           w1 * w2 * lastPointColor3[j] + (sumW >> 1)) /
        sumW;
    }
  }
  return predictorColor;
}

PC_COL getColorPredictorFarthest(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                 const SequenceParameterSet& sps, const AttributeParameterSet& aps,
                                 vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
                                 vector<colorNeighborSet>& neighborSet, int64_t& minNeighborDis) {
  PC_COL predictorColor;
  int farestpoint = 0;
  int64_t farestlength = 0;
  PC_POS farestPos;
  auto pointIndexc = pointCloudCode[curIndex].index;
  auto curPos = outputPointCloud[pointIndexc];
  int setlength = neighborSet.size();
  // adjust color QP per point tool
  bool minNeighborDisFlag = aps.colorQPAdjustFlag || aps.chromaDeadzoneFlag;
  auto attrQP = sps.colorQuantParam;
  if (minIdx > setlength) {
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    int64_t dis;
    /*  int32_t indexes[125];*/
    std::vector<int32_t> indexes;
    indexes.resize(aps.maxNumOfNeighbours - 3);
    int count = 0;
    for (int i = 0; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      dis = getDisNorm1(curPos, neighborPos, 1);
      if (dis > farestlength && i >= subGroupCount) {
        farestlength = dis;
        farestpoint = i;
        farestPos = neighborPos;
      }
      if (neis[1].dis > dis) {
        if (neis[2].dis == neis[1].dis && neis[2].dis != INT_FAST64_MAX) {
          indexes[count] = neis[2].index;
          count++;
        } else
          count = 0;
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].dis = dis;
          neis[0].index = i;
          continue;
        }
        neis[2] = neis[1];
        neis[1].dis = dis;
        neis[1].index = i;
        continue;
      }
      if (neis[2].dis > dis) {
        neis[2].dis = dis;
        neis[2].index = i;
        count = 0;
        continue;
      }
      if (neis[2].dis == dis) {
        indexes[count] = i;
        count++;
      }
    }
    auto w3 = neis[2].dis;
    auto lastPointIndex3 = neis[2].index;
    auto color3 = neighborSet[lastPointIndex3].color;
    auto w2 = neis[1].dis;
    auto lastPointIndex2 = neis[1].index;
    auto color2 = neighborSet[lastPointIndex2].color;
    auto w1 = neis[0].dis;
    auto lastPointIndex1 = neis[0].index;
    auto color1 = neighborSet[lastPointIndex1].color;

    if (minNeighborDisFlag) {
      minNeighborDis = w1;
    }

    V3<int32_t> lastcolor;
    lastcolor[0] = color3[0];
    lastcolor[1] = color3[1];
    lastcolor[2] = color3[2];
    count = std::min(count, 13);
    for (int j = 0; j < count; j++) {
      auto temp = neighborSet[indexes[j]].color;
      lastcolor[0] = lastcolor[0] + temp[0];
      lastcolor[1] = lastcolor[1] + temp[1];
      lastcolor[2] = lastcolor[2] + temp[2];
    }
    int64_t dw;

    if (w3 > w2) {
      uint64_t dw1 = w2 * w3;
      uint64_t dw2 = w1 * w3;
      uint64_t dw3 = w1 * w2;
      int countp1 = count + 1;
      if (attrQP == 0) {
        dw = (dw1 + dw2 + dw3) * (countp1);
        predictorColor[0] = (dw1 * color1[0] * (countp1) + dw2 * color2[0] * (countp1) +
                             dw3 * lastcolor[0] + (dw >> 1)) / dw;
        predictorColor[1] = (dw1 * color1[1] * (countp1) + dw2 * color2[1] * (countp1) +
                             dw3 * lastcolor[1] + (dw >> 1)) / dw;
        predictorColor[2] = (dw1 * color1[2] * (countp1) + dw2 * color2[2] * (countp1) +
                             dw3 * lastcolor[2] + (dw >> 1)) / dw;
      } else {
        dw = dw1 + dw2 + dw3 * countp1;
        predictorColor[0] =
          (dw1 * color1[0] + dw2 * color2[0] + dw3 * lastcolor[0] + (dw >> 1)) / dw;
        predictorColor[1] =
          (dw1 * color1[1] + dw2 * color2[1] + dw3 * lastcolor[1] + (dw >> 1)) / dw;
        predictorColor[2] =
          (dw1 * color1[2] + dw2 * color2[2] + dw3 * lastcolor[2] + (dw >> 1)) / dw;
      }
    } else if ((w3 == w2) && (w2 > w1)) {
      int countp2 = count + 2;
      if (attrQP == 0) {
        dw = (w1 + w2) * (countp2);
        predictorColor[0] =
          (w2 * color1[0] * (countp2) + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] =
          (w2 * color1[1] * (countp2) + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] =
          (w2 * color1[2] * (countp2) + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      } else {
        dw = (w1 * countp2 + w2);
        predictorColor[0] = (w2 * color1[0] + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] = (w2 * color1[1] + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] = (w2 * color1[2] + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      }
    } else {
      predictorColor[0] = (color1[0] + color2[0] + lastcolor[0] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[1] = (color1[1] + color2[1] + lastcolor[1] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[2] = (color1[2] + color2[2] + lastcolor[2] + ((count + 3) >> 1)) / (count + 3);
    }
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
  } else if (minIdx == 0) {
    predictorColor = {128, 128, 128};
  } else if (minIdx == 1) {
    predictorColor = neighborSet[0].color;
  } else if (minIdx == 2) {
    auto lastPointColor1 = neighborSet[1].color;
    auto lastPointColor2 = neighborSet[0].color;
    auto neighborPos1 = neighborSet[1].pos;
    auto neighborPos2 = neighborSet[0].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    int64_t sumW = w1 + w2;
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * lastPointColor1[j] + w1 * lastPointColor2[j] + (sumW >> 1)) / sumW;
    }
  } else {
    auto lastPointColor1 = neighborSet[minIdx - 1].color;
    auto lastPointColor2 = neighborSet[minIdx - 2].color;
    auto lastPointColor3 = neighborSet[minIdx - 3].color;
    auto neighborPos1 = neighborSet[minIdx - 1].pos;
    auto neighborPos2 = neighborSet[minIdx - 2].pos;
    auto neighborPos3 = neighborSet[minIdx - 3].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    auto w3 = getDisNorm1(curPos, neighborPos3, 1);
    int64_t sumW = w1 * w2 + w2 * w3 + w1 * w3;
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * w3 * lastPointColor1[j] + w1 * w3 * lastPointColor2[j] +
                           w1 * w2 * lastPointColor3[j] + (sumW >> 1)) /
        sumW;
    }
  }
  return predictorColor;
}

PC_REFL getReflectancePredictorFarthest(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                        const SequenceParameterSet& sps,
                                        const AttributeParameterSet& aps,
                                        vector<pointCodeWithIndex>& pointCloudCode,
                                        int& subGroupCount, vector<reflNeighborSet>& neighborSet) {
  PC_REFL predRef;
  auto& pointIndex = pointCloudCode[curIndex].index;
  auto& curPos = outputPointCloud[pointIndex];
  int farestpoint = 0;
  int64_t farestlength = 0;
  int setlength = aps.maxNumOfNeighbours;
  if (minIdx > setlength) {
    int64_t dis;
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    for (int i = 0; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      dis = getDisNorm1(curPos, neighborPos, aps.axisBias);
      if (dis > farestlength && i >= subGroupCount) {
        farestlength = dis;
        farestpoint = i;
      }
      if (neis[1].dis > dis) {
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].dis = dis;
          neis[0].index = i;
          continue;
        }
        neis[2] = neis[1];
        neis[1].dis = dis;
        neis[1].index = i;
        continue;
      }
      if (neis[2].dis > dis) {
        neis[2].dis = dis;
        neis[2].index = i;
      }
    }
    auto w1 = neis[0].dis;
    auto ref1 = neighborSet[neis[0].index].refl;
    auto w2 = neis[1].dis;
    auto ref2 = neighborSet[neis[1].index].refl;
    auto w3 = neis[2].dis;
    auto ref3 = neighborSet[neis[2].index].refl;
    assert(w1 > 0);  // assume no duplicate point
    if (aps.predFixedPointFracBit == 0) {
      auto k1 = w1 * w2;
      auto k2 = w2 * w3;
      auto k3 = w1 * w3;
      auto sumW = k1 + k2 + k3;
      predRef = (k2 * ref1 + k3 * ref2 + k1 * ref3) / sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (w1 >> 1)) / w1;
      up = ref1 * sw;
      bottom = sw;
      sw = (distanceScale + (w2 >> 1)) / w2;
      up += ref2 * sw;
      bottom += sw;
      sw = (distanceScale + (w3 >> 1)) / w3;
      up += ref3 * sw;
      bottom += sw;
      predRef = up / bottom;
    }
    if (sps.reflThreshold) {
      if (abs(ref1 - ref3) >= sps.reflThreshold) {
        predRef = ref1;
      }
    }
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
  } else if (minIdx == 0) {
    predRef = 0;
  } else if (minIdx == 1) {
    predRef = neighborSet[0].refl;
  } else if (minIdx == 2) {
    const PC_POS last1Point = neighborSet[1].pos;
    const PC_POS last2Point = neighborSet[0].pos;
    const int64_t weight1 = (abs(curPos[0] - last1Point[0]) + abs(curPos[1] - last1Point[1]) +
                             aps.axisBias * abs(curPos[2] - last1Point[2]));
    const int64_t weight2 = (abs(curPos[0] - last2Point[0]) + abs(curPos[1] - last2Point[1]) +
                             aps.axisBias * abs(curPos[2] - last2Point[2]));
    const auto last1Reflectance = neighborSet[1].refl;
    const auto last2Reflectance = neighborSet[0].refl;
    assert(weight1 + weight2 > 0);  // assume no duplicate point
    if (aps.predFixedPointFracBit == 0) {
      int64_t sumW = weight1 + weight2;
      predRef = (weight2 * last1Reflectance + weight1 * last2Reflectance) / sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (weight1 >> 1)) / weight1;
      up = last1Reflectance * sw;
      bottom = sw;
      sw = (distanceScale + (weight2 >> 1)) / weight2;
      up += last2Reflectance * sw;
      bottom += sw;
      predRef = up / bottom;
    }
  } else {
    const PC_POS last1Point = neighborSet[minIdx - 1].pos;
    const PC_POS last2Point = neighborSet[minIdx - 2].pos;
    const PC_POS last3Point = neighborSet[minIdx - 3].pos;
    const int64_t weight1 = (abs(curPos[0] - last1Point[0]) + abs(curPos[1] - last1Point[1]) +
                             aps.axisBias * abs(curPos[2] - last1Point[2]));
    const int64_t weight2 = (abs(curPos[0] - last2Point[0]) + abs(curPos[1] - last2Point[1]) +
                             aps.axisBias * abs(curPos[2] - last2Point[2]));
    const int64_t weight3 = (abs(curPos[0] - last3Point[0]) + abs(curPos[1] - last3Point[1]) +
                             aps.axisBias * abs(curPos[2] - last3Point[2]));
    const auto last1Reflectance = neighborSet[minIdx - 1].refl;
    const auto last2Reflectance = neighborSet[minIdx - 2].refl;
    const auto last3Reflectance = neighborSet[minIdx - 3].refl;
    if (aps.predFixedPointFracBit == 0) {
      auto sumW = weight2 * weight3 + weight1 * weight3 + weight1 * weight2;
      assert(sumW > 0);  // assume no duplicate point
      predRef = (weight2 * weight3 * last1Reflectance + weight1 * weight3 * last2Reflectance +
                 weight1 * weight2 * last3Reflectance) /
        sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (weight1 >> 1)) / weight1;
      up = last1Reflectance * sw;
      bottom = sw;
      sw = (distanceScale + (weight2 >> 1)) / weight2;
      up += last2Reflectance * sw;
      bottom += sw;
      sw = (distanceScale + (weight3 >> 1)) / weight3;
      up += last3Reflectance * sw;
      bottom += sw;
      predRef = up / bottom;
    }
    if (sps.reflThreshold) {
      if (abs(last1Reflectance - last3Reflectance) >= sps.reflThreshold) {
        predRef = last1Reflectance;
      }
    }
  }
  return predRef;
}

PC_REFL getReflectancePredictorFarthest(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                        const SequenceParameterSet& sps,
                                        const AttributeParameterSet& aps,
                                        vector<pointCodeWithIndex>& pointCloudCode,
                                        int& subGroupCount, vector<reflNeighborSet>& neighborSet,
                                        const vector<int>& distWeight) {
  PC_REFL predRef;
  auto& pointIndex = pointCloudCode[curIndex].index;
  auto& curPos = outputPointCloud[pointIndex];
  int farestpoint = 0;
  int64_t farestlength = 0;
  int setlength = aps.maxNumOfNeighbours;
  if (minIdx > setlength) {
    int64_t dis;
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    for (int i = 0; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      if (distWeight[0] != 0 && distWeight[1] != 0 && distWeight[2] != 0)
        dis = getDisNorm1(curPos, neighborPos, distWeight);
      else
        dis = getDisNorm1(curPos, neighborPos, aps.axisBias);
      if (dis > farestlength && i >= subGroupCount) {
        farestlength = dis;
        farestpoint = i;
      }
      if (neis[1].dis > dis) {
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].dis = dis;
          neis[0].index = i;
          continue;
        }
        neis[2] = neis[1];
        neis[1].dis = dis;
        neis[1].index = i;
        continue;
      }
      if (neis[2].dis > dis) {
        neis[2].dis = dis;
        neis[2].index = i;
      }
    }
    auto w1 = neis[0].dis;
    auto ref1 = neighborSet[neis[0].index].refl;
    auto w2 = neis[1].dis;
    auto ref2 = neighborSet[neis[1].index].refl;
    auto w3 = neis[2].dis;
    auto ref3 = neighborSet[neis[2].index].refl;
    assert(w1 > 0);  // assume no duplicate point
    if (aps.predFixedPointFracBit == 0) {
      auto k1 = w1 * w2;
      auto k2 = w2 * w3;
      auto k3 = w1 * w3;
      auto sumW = k1 + k2 + k3;
      predRef = (k2 * ref1 + k3 * ref2 + k1 * ref3) / sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (w1 >> 1)) / w1;
      up = ref1 * sw;
      bottom = sw;
      sw = (distanceScale + (w2 >> 1)) / w2;
      up += ref2 * sw;
      bottom += sw;
      sw = (distanceScale + (w3 >> 1)) / w3;
      up += ref3 * sw;
      bottom += sw;
      predRef = up / bottom;
    }
    if (sps.reflThreshold) {
      if (abs(ref1 - ref3) >= sps.reflThreshold) {
        predRef = ref1;
      }
    }
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
  } else if (minIdx == 0) {
    predRef = 0;
  } else if (minIdx == 1) {
    predRef = neighborSet[0].refl;
  } else if (minIdx == 2) {
    const PC_POS last1Point = neighborSet[1].pos;
    const PC_POS last2Point = neighborSet[0].pos;
    const int64_t weight1 = (abs(curPos[0] - last1Point[0]) + abs(curPos[1] - last1Point[1]) +
                             aps.axisBias * abs(curPos[2] - last1Point[2]));
    const int64_t weight2 = (abs(curPos[0] - last2Point[0]) + abs(curPos[1] - last2Point[1]) +
                             aps.axisBias * abs(curPos[2] - last2Point[2]));
    const auto last1Reflectance = neighborSet[1].refl;
    const auto last2Reflectance = neighborSet[0].refl;
    assert(weight1 + weight2 > 0);  // assume no duplicate point
    if (aps.predFixedPointFracBit == 0) {
      int64_t sumW = weight1 + weight2;
      predRef = (weight2 * last1Reflectance + weight1 * last2Reflectance) / sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (weight1 >> 1)) / weight1;
      up = last1Reflectance * sw;
      bottom = sw;
      sw = (distanceScale + (weight2 >> 1)) / weight2;
      up += last2Reflectance * sw;
      bottom += sw;
      predRef = up / bottom;
    }
  } else {
    const PC_POS last1Point = neighborSet[minIdx - 1].pos;
    const PC_POS last2Point = neighborSet[minIdx - 2].pos;
    const PC_POS last3Point = neighborSet[minIdx - 3].pos;
    const int64_t weight1 = (abs(curPos[0] - last1Point[0]) + abs(curPos[1] - last1Point[1]) +
                             aps.axisBias * abs(curPos[2] - last1Point[2]));
    const int64_t weight2 = (abs(curPos[0] - last2Point[0]) + abs(curPos[1] - last2Point[1]) +
                             aps.axisBias * abs(curPos[2] - last2Point[2]));
    const int64_t weight3 = (abs(curPos[0] - last3Point[0]) + abs(curPos[1] - last3Point[1]) +
                             aps.axisBias * abs(curPos[2] - last3Point[2]));
    const auto last1Reflectance = neighborSet[minIdx - 1].refl;
    const auto last2Reflectance = neighborSet[minIdx - 2].refl;
    const auto last3Reflectance = neighborSet[minIdx - 3].refl;
    if (aps.predFixedPointFracBit == 0) {
      auto sumW = weight2 * weight3 + weight1 * weight3 + weight1 * weight2;
      assert(sumW > 0);  // assume no duplicate point
      predRef = (weight2 * weight3 * last1Reflectance + weight1 * weight3 * last2Reflectance +
                 weight1 * weight2 * last3Reflectance) /
        sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (weight1 >> 1)) / weight1;
      up = last1Reflectance * sw;
      bottom = sw;
      sw = (distanceScale + (weight2 >> 1)) / weight2;
      up += last2Reflectance * sw;
      bottom += sw;
      sw = (distanceScale + (weight3 >> 1)) / weight3;
      up += last3Reflectance * sw;
      bottom += sw;
      predRef = up / bottom;
    }
    if (sps.reflThreshold) {
      if (abs(last1Reflectance - last3Reflectance) >= sps.reflThreshold) {
        predRef = last1Reflectance;
      }
    }
  }
  return predRef;
}

PC_REFL getReflectancePredictorNoUpdate(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                        const SequenceParameterSet& sps,
                                        const AttributeParameterSet& aps,
                                        const vector<pointCodeWithIndex>& pointCloudCode,
                                        int& subGroupCount, vector<colorNeighborSet> neighborSet,
                                        vector<reflWithCoefNeighborSet> reflWithCoefNeighborSet,
                                        vector<reflNeighborSet> reflbuffer,
                                        const vector<int>& distWeight) {
  PC_REFL predRef;
  auto& pointIndex = pointCloudCode[curIndex].index;
  auto& curPos = outputPointCloud[pointIndex];
  int setlength = aps.maxNumOfNeighbours;
  if (minIdx > setlength) {
    int64_t dis;
    pair<int64_t, PC_REFL> neis[3];
    neis[0].first = neis[1].first = neis[2].first = INT_FAST64_MAX;
    int bufferCount = reflbuffer.size();
    for (int i = subGroupCount; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      if (distWeight[0] != 0 && distWeight[1] != 0 && distWeight[2] != 0)
        dis = getDisNorm1(curPos, neighborPos, distWeight);
      else
        dis = getDisNorm1(curPos, neighborPos, aps.axisBias);
      if (neis[1].first > dis) {
        if (neis[0].first > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].first = dis;
          neis[0].second = reflWithCoefNeighborSet[i].refl;
          continue;
        }
        neis[2] = neis[1];
        neis[1].first = dis;
        neis[1].second = reflWithCoefNeighborSet[i].refl;
        continue;
      }
      if (neis[2].first > dis) {
        neis[2].first = dis;
        neis[2].second = reflWithCoefNeighborSet[i].refl;
      }
    }
    for (int i = 0; i < bufferCount; i++) {
      auto neighborPos = reflbuffer[i].pos;
      dis = getDisNorm1(curPos, neighborPos, aps.axisBias);
      if (neis[1].first > dis) {
        if (neis[0].first > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].first = dis;
          neis[0].second = reflbuffer[i].refl;
          continue;
        }
        neis[2] = neis[1];
        neis[1].first = dis;
        neis[1].second = reflbuffer[i].refl;
        continue;
      }
      if (neis[2].first > dis) {
        neis[2].first = dis;
        neis[2].second = reflbuffer[i].refl;
      }
    }
    auto w1 = neis[0].first;
    auto& ref1 = neis[0].second;
    auto w2 = neis[1].first;
    auto& ref2 = neis[1].second;
    auto w3 = neis[2].first;
    auto& ref3 = neis[2].second;
    assert(w1 > 0);  // assume no duplicate point
    if (aps.predFixedPointFracBit == 0) {
      auto k1 = w1 * w2;
      auto k2 = w2 * w3;
      auto k3 = w1 * w3;
      auto sumW = k1 + k2 + k3;
      predRef = (k2 * ref1 + k3 * ref2 + k1 * ref3) / sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (w1 >> 1)) / w1;
      up = ref1 * sw;
      bottom = sw;
      sw = (distanceScale + (w2 >> 1)) / w2;
      up += ref2 * sw;
      bottom += sw;
      sw = (distanceScale + (w3 >> 1)) / w3;
      up += ref3 * sw;
      bottom += sw;
      predRef = up / bottom;
    }
    if (sps.reflThreshold) {
      if (abs(ref1 - ref3) >= sps.reflThreshold) {
        predRef = ref1;
      }
    }
  } else if (minIdx == 0) {
    predRef = 0;
  } else if (minIdx == 1) {
    predRef = reflWithCoefNeighborSet[0].refl;
  } else if (minIdx == 2) {
    const PC_POS last1Point = neighborSet[1].pos;
    const PC_POS last2Point = neighborSet[0].pos;
    const int64_t weight1 = (abs(curPos[0] - last1Point[0]) + abs(curPos[1] - last1Point[1]) +
                             aps.axisBias * abs(curPos[2] - last1Point[2]));
    const int64_t weight2 = (abs(curPos[0] - last2Point[0]) + abs(curPos[1] - last2Point[1]) +
                             aps.axisBias * abs(curPos[2] - last2Point[2]));
    const auto last1Reflectance = reflWithCoefNeighborSet[1].refl;
    const auto last2Reflectance = reflWithCoefNeighborSet[0].refl;
    assert(weight1 + weight2 > 0);  // assume no duplicate point
    if (aps.predFixedPointFracBit == 0) {
      int64_t sumW = weight1 + weight2;
      predRef = (weight2 * last1Reflectance + weight1 * last2Reflectance) / sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (weight1 >> 1)) / weight1;
      up = last1Reflectance * sw;
      bottom = sw;
      sw = (distanceScale + (weight2 >> 1)) / weight2;
      up += last2Reflectance * sw;
      bottom += sw;
      predRef = up / bottom;
    }
  } else {
    const PC_POS last1Point = neighborSet[minIdx - 1].pos;
    const PC_POS last2Point = neighborSet[minIdx - 2].pos;
    const PC_POS last3Point = neighborSet[minIdx - 3].pos;
    const int64_t weight1 = (abs(curPos[0] - last1Point[0]) + abs(curPos[1] - last1Point[1]) +
                             aps.axisBias * abs(curPos[2] - last1Point[2]));
    const int64_t weight2 = (abs(curPos[0] - last2Point[0]) + abs(curPos[1] - last2Point[1]) +
                             aps.axisBias * abs(curPos[2] - last2Point[2]));
    const int64_t weight3 = (abs(curPos[0] - last3Point[0]) + abs(curPos[1] - last3Point[1]) +
                             aps.axisBias * abs(curPos[2] - last3Point[2]));
    const auto last1Reflectance = reflWithCoefNeighborSet[minIdx - 1].refl;
    const auto last2Reflectance = reflWithCoefNeighborSet[minIdx - 2].refl;
    const auto last3Reflectance = reflWithCoefNeighborSet[minIdx - 3].refl;
    if (aps.predFixedPointFracBit == 0) {
      auto sumW = weight2 * weight3 + weight1 * weight3 + weight1 * weight2;
      assert(sumW > 0);  // assume no duplicate point
      predRef = (weight2 * weight3 * last1Reflectance + weight1 * weight3 * last2Reflectance +
                 weight1 * weight2 * last3Reflectance) /
        sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (weight1 >> 1)) / weight1;
      up = last1Reflectance * sw;
      bottom = sw;
      sw = (distanceScale + (weight2 >> 1)) / weight2;
      up += last2Reflectance * sw;
      bottom += sw;
      sw = (distanceScale + (weight3 >> 1)) / weight3;
      up += last3Reflectance * sw;
      bottom += sw;
      predRef = up / bottom;
    }

    if (sps.reflThreshold) {
      if (abs(last1Reflectance - last3Reflectance) >= sps.reflThreshold) {
        predRef = last1Reflectance;
      }
    }
  }
  return predRef;
}

PC_COL getColorPredictorFromReflectance(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                        const SequenceParameterSet& sps,
                                        const AttributeParameterSet& aps,
                                        vector<pointCodeWithIndex>& pointCloudCode,
                                        int& subGroupCount, vector<colorNeighborSet>& neighborSet,
                                        vector<reflWithCoefNeighborSet>& reflWithCoefNeighborSet,
                                        const uint64_t curReflWithCoef) {
  PC_COL predictorColor;
  int farestpoint = 0;
  int64_t farestlength = 0;
  PC_POS farestPos;
  auto attrQP = sps.colorQuantParam;
  auto pointIndexc = pointCloudCode[curIndex].index;
  auto curPos = outputPointCloud[pointIndexc];
  int setlength = aps.maxNumOfNeighbours;
  if (minIdx > setlength) {
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    int64_t dis;
    int64_t disG;

    /* int32_t indexes[125];*/

    std::vector<int32_t> indexes;
    indexes.resize(aps.maxNumOfNeighbours - 3);
    int count = 0;
    for (int i = setlength - 1; i >= 0; i--) {
      auto neighborPos = neighborSet[i].pos;
      disG = getDisNorm1(curPos, neighborPos, 1);
      auto lastPointRefl = reflWithCoefNeighborSet[i].reflWithCoef;
      uint64_t distRef = (abs((int64_t)(curReflWithCoef - lastPointRefl))) >> 10;
      dis = disG + distRef;
      if (disG > farestlength && i >= subGroupCount) {
        farestlength = disG;
        farestpoint = i;
        farestPos = neighborPos;
      }
      if (neis[1].dis > dis) {
        if (neis[2].dis == neis[1].dis && neis[2].dis != INT_FAST64_MAX) {
          indexes[count] = neis[2].index;
          count++;
        } else
          count = 0;
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].dis = dis;
          neis[0].index = i;
          continue;
        }
        neis[2] = neis[1];
        neis[1].dis = dis;
        neis[1].index = i;
        continue;
      }
      if (neis[2].dis > dis) {
        neis[2].dis = dis;
        neis[2].index = i;
        count = 0;
        continue;
      }
      if (neis[2].dis == dis) {
        indexes[count] = i;
        count++;
      }
    }
    auto w3 = neis[2].dis;
    auto lastPointIndex3 = neis[2].index;
    auto color3 = neighborSet[lastPointIndex3].color;
    auto w2 = neis[1].dis;
    auto lastPointIndex2 = neis[1].index;
    auto color2 = neighborSet[lastPointIndex2].color;
    auto w1 = neis[0].dis;
    auto lastPointIndex1 = neis[0].index;
    auto color1 = neighborSet[lastPointIndex1].color;

    V3<int32_t> lastcolor;
    lastcolor[0] = color3[0];
    lastcolor[1] = color3[1];
    lastcolor[2] = color3[2];
    count = std::min(count, 13);
    for (int j = 0; j < count; j++) {
      auto temp = neighborSet[indexes[j]].color;
      lastcolor[0] = lastcolor[0] + temp[0];
      lastcolor[1] = lastcolor[1] + temp[1];
      lastcolor[2] = lastcolor[2] + temp[2];
    }
    int64_t dw;

    if (w3 > w2) {
      uint64_t dw1 = w2 * w3;
      uint64_t dw2 = w1 * w3;
      uint64_t dw3 = w1 * w2;
      int countp1 = count + 1;
      if (attrQP == 0) {
        dw = (dw1 + dw2 + dw3) * (countp1);
        predictorColor[0] = (dw1 * color1[0] * (countp1) + dw2 * color2[0] * (countp1) +
                             dw3 * lastcolor[0] + (dw >> 1)) /
          dw;
        predictorColor[1] = (dw1 * color1[1] * (countp1) + dw2 * color2[1] * (countp1) +
                             dw3 * lastcolor[1] + (dw >> 1)) /
          dw;
        predictorColor[2] = (dw1 * color1[2] * (countp1) + dw2 * color2[2] * (countp1) +
                             dw3 * lastcolor[2] + (dw >> 1)) /
          dw;
      } else {
        dw = dw1 + dw2 + dw3 * countp1;
        predictorColor[0] =
          (dw1 * color1[0] + dw2 * color2[0] + dw3 * lastcolor[0] + (dw >> 1)) / dw;
        predictorColor[1] =
          (dw1 * color1[1] + dw2 * color2[1] + dw3 * lastcolor[1] + (dw >> 1)) / dw;
        predictorColor[2] =
          (dw1 * color1[2] + dw2 * color2[2] + dw3 * lastcolor[2] + (dw >> 1)) / dw;
      }
    } else if ((w3 == w2) && (w2 > w1)) {
      int countp2 = count + 2;
      if (attrQP == 0) {
        dw = (w1 + w2) * (countp2);
        predictorColor[0] =
          (w2 * color1[0] * (countp2) + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] =
          (w2 * color1[1] * (countp2) + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] =
          (w2 * color1[2] * (countp2) + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      } else {
        dw = (w1 * countp2 + w2);
        predictorColor[0] = (w2 * color1[0] + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] = (w2 * color1[1] + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] = (w2 * color1[2] + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      }
    } else {
      predictorColor[0] = (color1[0] + color2[0] + lastcolor[0] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[1] = (color1[1] + color2[1] + lastcolor[1] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[2] = (color1[2] + color2[2] + lastcolor[2] + ((count + 3) >> 1)) / (count + 3);
    }
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
    std::swap(reflWithCoefNeighborSet[subGroupCount], reflWithCoefNeighborSet[farestpoint]);
  } else if (minIdx == 0) {
    predictorColor = {128, 128, 128};
  } else if (minIdx == 1) {
    predictorColor = neighborSet[0].color;
  } else if (minIdx == 2) {
    auto lastPointColor1 = neighborSet[1].color;
    auto lastPointColor2 = neighborSet[0].color;
    auto neighborPos1 = neighborSet[1].pos;
    auto neighborPos2 = neighborSet[0].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    int64_t sumW = w1 + w2;
    assert(sumW > 0);
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * lastPointColor1[j] + w1 * lastPointColor2[j] + (sumW >> 1)) / sumW;
    }
  } else {
    auto lastPointColor1 = neighborSet[minIdx - 1].color;
    auto lastPointColor2 = neighborSet[minIdx - 2].color;
    auto lastPointColor3 = neighborSet[minIdx - 3].color;
    auto neighborPos1 = neighborSet[minIdx - 1].pos;
    auto neighborPos2 = neighborSet[minIdx - 2].pos;
    auto neighborPos3 = neighborSet[minIdx - 3].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    auto w3 = getDisNorm1(curPos, neighborPos3, 1);
    int64_t sumW = w1 * w2 + w2 * w3 + w1 * w3;
    assert(sumW > 0);
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * w3 * lastPointColor1[j] + w1 * w3 * lastPointColor2[j] +
                           w1 * w2 * lastPointColor3[j] + (sumW >> 1)) /
        sumW;
    }
  }
  return predictorColor;
}

PC_COL getColorPredictorFromReflectance(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                        const SequenceParameterSet& sps,
                                        const AttributeParameterSet& aps,
                                        vector<pointCodeWithIndex>& pointCloudCode,
                                        int& subGroupCount, vector<colorNeighborSet>& neighborSet,
                                        vector<reflWithCoefNeighborSet>& reflWithCoefNeighborSet,
                                        const uint64_t curReflWithCoef, int64_t& minNeighborDis) {
  PC_COL predictorColor;
  int farestpoint = 0;
  int64_t farestlength = 0;
  PC_POS farestPos;
  auto pointIndexc = pointCloudCode[curIndex].index;
  auto curPos = outputPointCloud[pointIndexc];
  int setlength = aps.maxNumOfNeighbours;
  auto attrQP = sps.colorQuantParam;
  // adjust color QP per point tool
  bool minNeighborDisFlag = aps.colorQPAdjustFlag || aps.chromaDeadzoneFlag;
  if (minIdx > setlength) {
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    int64_t dis;
    int64_t disG;

    /* int32_t indexes[125];*/

    std::vector<int32_t> indexes;
    indexes.resize(aps.maxNumOfNeighbours - 3);
    int count = 0;
    for (int i = setlength - 1; i >= 0; i--) {
      auto neighborPos = neighborSet[i].pos;
      disG = getDisNorm1(curPos, neighborPos, 1);
      auto lastPointRefl = reflWithCoefNeighborSet[i].reflWithCoef;
      uint64_t distRef = (abs((int64_t)(curReflWithCoef - lastPointRefl))) >> 10;
      dis = disG + distRef;
      if (disG > farestlength && i >= subGroupCount) {
        farestlength = disG;
        farestpoint = i;
        farestPos = neighborPos;
      }
      if (neis[1].dis > dis) {
        if (neis[2].dis == neis[1].dis && neis[2].dis != INT_FAST64_MAX) {
          indexes[count] = neis[2].index;
          count++;
        } else
          count = 0;
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].dis = dis;
          neis[0].index = i;
          continue;
        }
        neis[2] = neis[1];
        neis[1].dis = dis;
        neis[1].index = i;
        continue;
      }
      if (neis[2].dis > dis) {
        neis[2].dis = dis;
        neis[2].index = i;
        count = 0;
        continue;
      }
      if (neis[2].dis == dis) {
        indexes[count] = i;
        count++;
      }
    }
    auto w3 = neis[2].dis;
    auto lastPointIndex3 = neis[2].index;
    auto color3 = neighborSet[lastPointIndex3].color;
    auto w2 = neis[1].dis;
    auto lastPointIndex2 = neis[1].index;
    auto color2 = neighborSet[lastPointIndex2].color;
    auto w1 = neis[0].dis;
    auto lastPointIndex1 = neis[0].index;
    auto color1 = neighborSet[lastPointIndex1].color;
    if (minNeighborDisFlag) {
      disG = getDisNorm1(curPos, neighborSet[lastPointIndex1].pos, 1);
      minNeighborDis = disG;
    }
    V3<int32_t> lastcolor;
    lastcolor[0] = color3[0];
    lastcolor[1] = color3[1];
    lastcolor[2] = color3[2];
    count = std::min(count, 13);
    for (int j = 0; j < count; j++) {
      auto temp = neighborSet[indexes[j]].color;
      lastcolor[0] = lastcolor[0] + temp[0];
      lastcolor[1] = lastcolor[1] + temp[1];
      lastcolor[2] = lastcolor[2] + temp[2];
    }
    int64_t dw;

    if (w3 > w2) {
      uint64_t dw1 = w2 * w3;
      uint64_t dw2 = w1 * w3;
      uint64_t dw3 = w1 * w2;
      int countp1 = count + 1;
      if (attrQP == 0) {
        dw = (dw1 + dw2 + dw3) * (countp1);
        predictorColor[0] = (dw1 * color1[0] * (countp1) + dw2 * color2[0] * (countp1) +
                             dw3 * lastcolor[0] + (dw >> 1)) /
          dw;
        predictorColor[1] = (dw1 * color1[1] * (countp1) + dw2 * color2[1] * (countp1) +
                             dw3 * lastcolor[1] + (dw >> 1)) /
          dw;
        predictorColor[2] = (dw1 * color1[2] * (countp1) + dw2 * color2[2] * (countp1) +
                             dw3 * lastcolor[2] + (dw >> 1)) /
          dw;
      } else {
        dw = dw1 + dw2 + dw3 * countp1;
        predictorColor[0] =
          (dw1 * color1[0] + dw2 * color2[0] + dw3 * lastcolor[0] + (dw >> 1)) / dw;
        predictorColor[1] =
          (dw1 * color1[1] + dw2 * color2[1] + dw3 * lastcolor[1] + (dw >> 1)) / dw;
        predictorColor[2] =
          (dw1 * color1[2] + dw2 * color2[2] + dw3 * lastcolor[2] + (dw >> 1)) / dw;
      }
    } else if ((w3 == w2) && (w2 > w1)) {
      int countp2 = count + 2;
      if (attrQP == 0) {
        dw = (w1 + w2) * (countp2);
        predictorColor[0] =
          (w2 * color1[0] * (countp2) + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] =
          (w2 * color1[1] * (countp2) + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] =
          (w2 * color1[2] * (countp2) + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      } else {
        dw = (w1 * countp2 + w2);
        predictorColor[0] = (w2 * color1[0] + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] = (w2 * color1[1] + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] = (w2 * color1[2] + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      }
    } else {
      predictorColor[0] = (color1[0] + color2[0] + lastcolor[0] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[1] = (color1[1] + color2[1] + lastcolor[1] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[2] = (color1[2] + color2[2] + lastcolor[2] + ((count + 3) >> 1)) / (count + 3);
    }
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
    std::swap(reflWithCoefNeighborSet[subGroupCount], reflWithCoefNeighborSet[farestpoint]);
  } else if (minIdx == 0) {
    predictorColor = {128, 128, 128};
  } else if (minIdx == 1) {
    predictorColor = neighborSet[0].color;
  } else if (minIdx == 2) {
    auto lastPointColor1 = neighborSet[1].color;
    auto lastPointColor2 = neighborSet[0].color;
    auto neighborPos1 = neighborSet[1].pos;
    auto neighborPos2 = neighborSet[0].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    int64_t sumW = w1 + w2;
    assert(sumW > 0);
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * lastPointColor1[j] + w1 * lastPointColor2[j] + (sumW >> 1)) / sumW;
    }
  } else {
    auto lastPointColor1 = neighborSet[minIdx - 1].color;
    auto lastPointColor2 = neighborSet[minIdx - 2].color;
    auto lastPointColor3 = neighborSet[minIdx - 3].color;
    auto neighborPos1 = neighborSet[minIdx - 1].pos;
    auto neighborPos2 = neighborSet[minIdx - 2].pos;
    auto neighborPos3 = neighborSet[minIdx - 3].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    auto w3 = getDisNorm1(curPos, neighborPos3, 1);
    int64_t sumW = w1 * w2 + w2 * w3 + w1 * w3;
    assert(sumW > 0);
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * w3 * lastPointColor1[j] + w1 * w3 * lastPointColor2[j] +
                           w1 * w2 * lastPointColor3[j] + (sumW >> 1)) /
        sumW;
    }
  }
  return predictorColor;
}

PC_COL getColorPredictorNoUpdate(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                 const SequenceParameterSet& sps, const AttributeParameterSet& aps,
                                 const vector<pointCodeWithIndex>& pointCloudCode,
                                 int& subGroupCount, vector<reflNeighborSet> neighborSet,
                                 vector<colorWithCoefNeighborSet> colorWithCoefNeighborSet,
                                 vector<colorNeighborSet> reflbuffer) {
  PC_COL predictorColor;
  auto& pointIndex = pointCloudCode[curIndex].index;
  auto& curPos = outputPointCloud[pointIndex];
  int setlength = aps.maxNumOfNeighbours;
  auto attrQP = sps.colorQuantParam;
  if (minIdx > setlength) {
    pair<int64_t, PC_COL> neis[3];
    neis[0].first = neis[1].first = neis[2].first = INT_FAST64_MAX;
    int64_t dis;
    vector<PC_COL> indexes(125);
    int count = 0;
    int bufferCount = reflbuffer.size();
    for (int i = subGroupCount; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      dis = getDisNorm1(curPos, neighborPos, 1);
      if (neis[1].first > dis) {
        if (neis[2].first == neis[1].first && neis[2].first != INT_FAST64_MAX) {
          for (int k = 0; k < 3; k++) {
            indexes[count][k] = neis[2].second[k];
          }
          count++;
        } else
          count = 0;
        if (neis[0].first > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].first = dis;
          neis[0].second = colorWithCoefNeighborSet[i].color;
          continue;
        }
        neis[2] = neis[1];
        neis[1].first = dis;
        neis[1].second = colorWithCoefNeighborSet[i].color;
        continue;
      }
      if (neis[2].first > dis) {
        neis[2].first = dis;
        neis[2].second = colorWithCoefNeighborSet[i].color;
        count = 0;
        continue;
      }
      if (neis[2].first == dis) {
        for (int k = 0; k < 3; k++) {
          indexes[count][k] = neis[2].second[k];
        }
        count++;
      }
    }
    for (int i = 0; i < bufferCount; i++) {
      auto neighborPos = reflbuffer[i].pos;
      dis = getDisNorm1(curPos, neighborPos, aps.axisBias);
      if (neis[1].first > dis) {
        if (neis[2].first == neis[1].first && neis[2].first != INT_FAST64_MAX) {
          for (int k = 0; k < 3; k++) {
            indexes[count][k] = neis[2].second[k];
          }
          count++;
        } else
          count = 0;
        if (neis[0].first > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].first = dis;
          neis[0].second = reflbuffer[i].color;
          continue;
        }
        neis[2] = neis[1];
        neis[1].first = dis;
        neis[1].second = reflbuffer[i].color;
        continue;
      }
      if (neis[2].first > dis) {
        neis[2].first = dis;
        neis[2].second = reflbuffer[i].color;
        count = 0;
        continue;
      }
      if (neis[2].first == dis) {
        for (int k = 0; k < 3; k++) {
          indexes[count][k] = neis[2].second[k];
        }
        count++;
      }
    }
    auto w3 = neis[2].first;
    auto color3 = neis[2].second;
    auto w2 = neis[1].first;
    auto color2 = neis[1].second;
    auto w1 = neis[0].first;
    auto color1 = neis[0].second;
    V3<int32_t> lastcolor;
    lastcolor[0] = color3[0];
    lastcolor[1] = color3[1];
    lastcolor[2] = color3[2];
    count = std::min(count, 13);
    for (int j = 0; j < count; j++) {
      auto temp = indexes[j];
      lastcolor[0] = lastcolor[0] + temp[0];
      lastcolor[1] = lastcolor[1] + temp[1];
      lastcolor[2] = lastcolor[2] + temp[2];
    }
    int64_t dw;

    if (w3 > w2) {
      uint64_t dw1 = w2 * w3;
      uint64_t dw2 = w1 * w3;
      uint64_t dw3 = w1 * w2;
      int countp1 = count + 1;
      if (attrQP == 0) {
        dw = (dw1 + dw2 + dw3) * (countp1);
        predictorColor[0] = (dw1 * color1[0] * (countp1) + dw2 * color2[0] * (countp1) +
                             dw3 * lastcolor[0] + (dw >> 1)) /
          dw;
        predictorColor[1] = (dw1 * color1[1] * (countp1) + dw2 * color2[1] * (countp1) +
                             dw3 * lastcolor[1] + (dw >> 1)) /
          dw;
        predictorColor[2] = (dw1 * color1[2] * (countp1) + dw2 * color2[2] * (countp1) +
                             dw3 * lastcolor[2] + (dw >> 1)) /
          dw;
      } else {
        dw = dw1 + dw2 + dw3 * countp1;
        predictorColor[0] =
          (dw1 * color1[0] + dw2 * color2[0] + dw3 * lastcolor[0] + (dw >> 1)) / dw;
        predictorColor[1] =
          (dw1 * color1[1] + dw2 * color2[1] + dw3 * lastcolor[1] + (dw >> 1)) / dw;
        predictorColor[2] =
          (dw1 * color1[2] + dw2 * color2[2] + dw3 * lastcolor[2] + (dw >> 1)) / dw;
      }
    } else if ((w3 == w2) && (w2 > w1)) {
      int countp2 = count + 2;
      if (attrQP == 0) {
        dw = (w1 + w2) * (countp2);
        predictorColor[0] =
          (w2 * color1[0] * (countp2) + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] =
          (w2 * color1[1] * (countp2) + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] =
          (w2 * color1[2] * (countp2) + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      } else {
        dw = (w1 * countp2 + w2);
        predictorColor[0] = (w2 * color1[0] + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] = (w2 * color1[1] + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] = (w2 * color1[2] + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      }
    } else {
      predictorColor[0] = (color1[0] + color2[0] + lastcolor[0] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[1] = (color1[1] + color2[1] + lastcolor[1] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[2] = (color1[2] + color2[2] + lastcolor[2] + ((count + 3) >> 1)) / (count + 3);
    }
  } else if (minIdx == 0) {
    predictorColor = {128, 128, 128};
  } else if (minIdx == 1) {
    predictorColor = colorWithCoefNeighborSet[0].color;
  } else if (minIdx == 2) {
    auto lastPointColor1 = colorWithCoefNeighborSet[1].color;
    auto lastPointColor2 = colorWithCoefNeighborSet[0].color;
    auto neighborPos1 = neighborSet[1].pos;
    auto neighborPos2 = neighborSet[0].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    int64_t sumW = w1 + w2;
    assert(sumW > 0);
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * lastPointColor1[j] + w1 * lastPointColor2[j] + (sumW >> 1)) / sumW;
    }
  } else {
    auto lastPointColor1 = colorWithCoefNeighborSet[minIdx - 1].color;
    auto lastPointColor2 = colorWithCoefNeighborSet[minIdx - 2].color;
    auto lastPointColor3 = colorWithCoefNeighborSet[minIdx - 3].color;

    auto neighborPos1 = neighborSet[minIdx - 1].pos;
    auto neighborPos2 = neighborSet[minIdx - 2].pos;
    auto neighborPos3 = neighborSet[minIdx - 3].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    auto w3 = getDisNorm1(curPos, neighborPos3, 1);
    int64_t sumW = w1 * w2 + w2 * w3 + w1 * w3;
    assert(sumW > 0);
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * w3 * lastPointColor1[j] + w1 * w3 * lastPointColor2[j] +
                           w1 * w2 * lastPointColor3[j] + (sumW >> 1)) /
        sumW;
    }
  }
  return predictorColor;
}

PC_REFL getReflectancePredictorFromColor(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                         const SequenceParameterSet& sps,
                                         const AttributeParameterSet& aps,
                                         vector<pointCodeWithIndex>& pointCloudCode,
                                         int& subGroupCount, vector<reflNeighborSet>& neighborSet,
                                         vector<colorWithCoefNeighborSet>& colorWithCoefNeighborSet,
                                         V3<int64_t> curColorWithCoef, const vector<int>& distWeight) {
  PC_REFL predRef;
  int farestpoint = 0;
  int64_t farestlength = 0;
  PC_POS farestPos;
  auto pointIndexc = pointCloudCode[curIndex].index;
  auto curPos = outputPointCloud[pointIndexc];
  int setlength = aps.maxNumOfNeighbours;
  if (minIdx > setlength) {
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    int64_t dis;
    int64_t disG;
    int count = 0;
    for (int i = setlength - 1; i >= 0; i--) {
      auto neighborPos = neighborSet[i].pos;
      if (distWeight[0] != 0 && distWeight[1] != 0 && distWeight[2] != 0)
          disG = getDisNorm1(curPos, neighborPos, distWeight);
      else
          disG = getDisNorm1(curPos, neighborPos, aps.axisBias);
      auto lastColor = colorWithCoefNeighborSet[i].colorWithCoef;
      int64_t disColor = abs(curColorWithCoef[0] - lastColor[0]) +
        abs(curColorWithCoef[1] - lastColor[1]) + abs(curColorWithCoef[2] - lastColor[2]);
      disColor = (disColor >> 10);
      dis = disG + disColor;
      if (disG > farestlength && i >= subGroupCount) {
        farestlength = disG;
        farestpoint = i;
      }
      if (neis[1].dis > dis) {
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].dis = dis;
          neis[0].index = i;
          continue;
        }
        neis[2] = neis[1];
        neis[1].dis = dis;
        neis[1].index = i;
        continue;
      }
      if (neis[2].dis > dis) {
        neis[2].dis = dis;
        neis[2].index = i;
      }
    }
    auto w1 = neis[0].dis;
    auto ref1 = neighborSet[neis[0].index].refl;
    auto w2 = neis[1].dis;
    auto ref2 = neighborSet[neis[1].index].refl;
    auto w3 = neis[2].dis;
    auto ref3 = neighborSet[neis[2].index].refl;
    assert(w1 > 0);  // assume no duplicate point
    if (aps.predFixedPointFracBit == 0) {
      auto k1 = w1 * w2;
      auto k2 = w2 * w3;
      auto k3 = w1 * w3;
      auto sumW = k1 + k2 + k3;
      predRef = (k2 * ref1 + k3 * ref2 + k1 * ref3) / sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (w1 >> 1)) / w1;
      up = ref1 * sw;
      bottom = sw;
      sw = (distanceScale + (w2 >> 1)) / w2;
      up += ref2 * sw;
      bottom += sw;
      sw = (distanceScale + (w3 >> 1)) / w3;
      up += ref3 * sw;
      bottom += sw;
      predRef = up / bottom;
    }
    if (sps.reflThreshold) {
      if (abs(ref1 - ref3) >= sps.reflThreshold) {
        predRef = ref1;
      }
    }
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
    std::swap(colorWithCoefNeighborSet[subGroupCount], colorWithCoefNeighborSet[farestpoint]);
  } else if (minIdx == 0) {
    predRef = 0;
  } else if (minIdx == 1) {
    predRef = neighborSet[0].refl;
  } else if (minIdx == 2) {
    const PC_POS last1Point = neighborSet[1].pos;
    const PC_POS last2Point = neighborSet[0].pos;
    const int64_t weight1 = (abs(curPos[0] - last1Point[0]) + abs(curPos[1] - last1Point[1]) +
                             aps.axisBias * abs(curPos[2] - last1Point[2]));
    const int64_t weight2 = (abs(curPos[0] - last2Point[0]) + abs(curPos[1] - last2Point[1]) +
                             aps.axisBias * abs(curPos[2] - last2Point[2]));
    const auto last1Reflectance = neighborSet[1].refl;
    const auto last2Reflectance = neighborSet[0].refl;
    assert(weight1 + weight2 > 0);  // assume no duplicate point
    if (aps.predFixedPointFracBit == 0) {
      int64_t sumW = weight1 + weight2;
      predRef = (weight2 * last1Reflectance + weight1 * last2Reflectance) / sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (weight1 >> 1)) / weight1;
      up = last1Reflectance * sw;
      bottom = sw;
      sw = (distanceScale + (weight2 >> 1)) / weight2;
      up += last2Reflectance * sw;
      bottom += sw;
      predRef = up / bottom;
    }
  } else {
    const PC_POS last1Point = neighborSet[minIdx - 1].pos;
    const PC_POS last2Point = neighborSet[minIdx - 2].pos;
    const PC_POS last3Point = neighborSet[minIdx - 3].pos;
    const int64_t weight1 = (abs(curPos[0] - last1Point[0]) + abs(curPos[1] - last1Point[1]) +
                             aps.axisBias * abs(curPos[2] - last1Point[2]));
    const int64_t weight2 = (abs(curPos[0] - last2Point[0]) + abs(curPos[1] - last2Point[1]) +
                             aps.axisBias * abs(curPos[2] - last2Point[2]));
    const int64_t weight3 = (abs(curPos[0] - last3Point[0]) + abs(curPos[1] - last3Point[1]) +
                             aps.axisBias * abs(curPos[2] - last3Point[2]));
    const auto last1Reflectance = neighborSet[minIdx - 1].refl;
    const auto last2Reflectance = neighborSet[minIdx - 2].refl;
    const auto last3Reflectance = neighborSet[minIdx - 3].refl;
    if (aps.predFixedPointFracBit == 0) {
      auto sumW = weight2 * weight3 + weight1 * weight3 + weight1 * weight2;
      assert(sumW > 0);  // assume no duplicate point
      predRef = (weight2 * weight3 * last1Reflectance + weight1 * weight3 * last2Reflectance +
                 weight1 * weight2 * last3Reflectance) /
        sumW;

    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (weight1 >> 1)) / weight1;
      up = last1Reflectance * sw;
      bottom = sw;
      sw = (distanceScale + (weight2 >> 1)) / weight2;
      up += last2Reflectance * sw;
      bottom += sw;
      sw = (distanceScale + (weight3 >> 1)) / weight3;
      up += last3Reflectance * sw;
      bottom += sw;
      predRef = up / bottom;
    }
    if (sps.reflThreshold) {
      if (abs(last1Reflectance - last3Reflectance) >= sps.reflThreshold) {
        predRef = last1Reflectance;
      }
    }
  }
  return predRef;
}

pair<PC_COL, PC_REFL>
  getAttributePredictorFarthest(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                const SequenceParameterSet& sps, const AttributeParameterSet& aps,
                                vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
                                vector<attrNeighborSet>& neighborSet) {
  pair<PC_COL, PC_REFL> predictorAttr;
  PC_COL predictorColor;
  PC_REFL predictorRefl;
  int farestpoint = 0;
  int64_t farestlength = 0;
  PC_POS farestPos;
  auto attrQP = sps.colorQuantParam;
  auto pointIndexc = pointCloudCode[curIndex].index;
  auto curPos = outputPointCloud[pointIndexc];
  int setlength = neighborSet.size();
  if (minIdx > setlength) {
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    int64_t dis;
    /*   int32_t indexes[125];*/
    std::vector<int32_t> indexes;
    indexes.resize(aps.maxNumOfNeighbours - 3);
    int count = 0;
    for (int i = 0; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      dis = getDisNorm1(curPos, neighborPos, 1);
      if (dis > farestlength && i >= subGroupCount) {
        farestlength = dis;
        farestpoint = i;
        farestPos = neighborPos;
      }
      if (neis[1].dis > dis) {
        if (neis[2].dis == neis[1].dis && neis[2].dis != INT_FAST64_MAX) {
          indexes[count] = neis[2].index;
          count++;
        } else
          count = 0;
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].dis = dis;
          neis[0].index = i;
          continue;
        }
        neis[2] = neis[1];
        neis[1].dis = dis;
        neis[1].index = i;
        continue;
      }
      if (neis[2].dis > dis) {
        neis[2].dis = dis;
        neis[2].index = i;
        count = 0;
        continue;
      }
      if (neis[2].dis == dis) {
        indexes[count] = i;
        count++;
      }
    }
    auto w3 = neis[2].dis;
    auto lastPointIndex3 = neis[2].index;
    auto color3 = neighborSet[lastPointIndex3].color;
    auto refl3 = neighborSet[lastPointIndex3].refl;
    auto w2 = neis[1].dis;
    auto lastPointIndex2 = neis[1].index;
    auto color2 = neighborSet[lastPointIndex2].color;
    auto refl2 = neighborSet[lastPointIndex2].refl;
    auto w1 = neis[0].dis;
    auto lastPointIndex1 = neis[0].index;
    auto color1 = neighborSet[lastPointIndex1].color;
    auto refl1 = neighborSet[lastPointIndex1].refl;
    V3<int32_t> lastcolor;
    lastcolor[0] = color3[0];
    lastcolor[1] = color3[1];
    lastcolor[2] = color3[2];
    count = std::min(count, 13);
    for (int j = 0; j < count; j++) {
      auto temp = neighborSet[indexes[j]].color;
      lastcolor[0] = lastcolor[0] + temp[0];
      lastcolor[1] = lastcolor[1] + temp[1];
      lastcolor[2] = lastcolor[2] + temp[2];
    }
    int64_t dw;

    if (w3 > w2) {
      uint64_t dw1 = w2 * w3;
      uint64_t dw2 = w1 * w3;
      uint64_t dw3 = w1 * w2;
      int countp1 = count + 1;
      if (attrQP == 0) {
        dw = (dw1 + dw2 + dw3) * (countp1);
        predictorColor[0] = (dw1 * color1[0] * (countp1) + dw2 * color2[0] * (countp1) +
                             dw3 * lastcolor[0] + (dw >> 1)) /
          dw;
        predictorColor[1] = (dw1 * color1[1] * (countp1) + dw2 * color2[1] * (countp1) +
                             dw3 * lastcolor[1] + (dw >> 1)) /
          dw;
        predictorColor[2] = (dw1 * color1[2] * (countp1) + dw2 * color2[2] * (countp1) +
                             dw3 * lastcolor[2] + (dw >> 1)) /
          dw;
      } else {
        dw = dw1 + dw2 + dw3 * countp1;
        predictorColor[0] =
          (dw1 * color1[0] + dw2 * color2[0] + dw3 * lastcolor[0] + (dw >> 1)) / dw;
        predictorColor[1] =
          (dw1 * color1[1] + dw2 * color2[1] + dw3 * lastcolor[1] + (dw >> 1)) / dw;
        predictorColor[2] =
          (dw1 * color1[2] + dw2 * color2[2] + dw3 * lastcolor[2] + (dw >> 1)) / dw;
      }
    } else if ((w3 == w2) && (w2 > w1)) {
      int countp2 = count + 2;
      if (attrQP == 0) {
        dw = (w1 + w2) * (countp2);
        predictorColor[0] =
          (w2 * color1[0] * (countp2) + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] =
          (w2 * color1[1] * (countp2) + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] =
          (w2 * color1[2] * (countp2) + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      } else {
        dw = (w1 * countp2 + w2);
        predictorColor[0] = (w2 * color1[0] + w1 * (color2[0] + lastcolor[0]) + (dw >> 1)) / dw;
        predictorColor[1] = (w2 * color1[1] + w1 * (color2[1] + lastcolor[1]) + (dw >> 1)) / dw;
        predictorColor[2] = (w2 * color1[2] + w1 * (color2[2] + lastcolor[2]) + (dw >> 1)) / dw;
      }
    } else {
      predictorColor[0] = (color1[0] + color2[0] + lastcolor[0] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[1] = (color1[1] + color2[1] + lastcolor[1] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[2] = (color1[2] + color2[2] + lastcolor[2] + ((count + 3) >> 1)) / (count + 3);
    }
    if (aps.predFixedPointFracBit == 0) {
      auto k1 = w1 * w2;
      auto k2 = w2 * w3;
      auto k3 = w1 * w3;
      auto sumW = k1 + k2 + k3;
      assert(sumW > 0);  // assume no duplicate point
      predictorRefl = (k2 * refl1 + k3 * refl2 + k1 * refl3) / sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      assert(w1 > 0);  // assume no duplicate point
      sw = (distanceScale + (w1 >> 1)) / w1;
      up = refl1 * sw;
      bottom = sw;
      sw = (distanceScale + (w2 >> 1)) / w2;
      up += refl2 * sw;
      bottom += sw;
      sw = (distanceScale + (w3 >> 1)) / w3;
      up += refl3 * sw;
      bottom += sw;
      predictorRefl = up / bottom;
    }
    if (sps.reflThreshold) {
      if (abs(refl1 - refl3) >= sps.reflThreshold) {
        predictorRefl = refl1;
      }
    }
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
  } else if (minIdx == 0) {
    predictorColor = {128, 128, 128};
    predictorRefl = 128;
  } else if (minIdx == 1) {
    predictorColor = neighborSet[0].color;
    predictorRefl = neighborSet[0].refl;
  } else if (minIdx == 2) {
    auto neighborPos1 = neighborSet[1].pos;
    auto neighborPos2 = neighborSet[0].pos;
    auto lastPointColor1 = neighborSet[1].color;
    auto lastPointColor2 = neighborSet[0].color;
    auto lastPointRefl1 = neighborSet[1].refl;
    auto lastPointRefl2 = neighborSet[0].refl;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    int64_t sumW = w1 + w2;
    assert(sumW > 0);
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * lastPointColor1[j] + w1 * lastPointColor2[j] + (sumW >> 1)) / sumW;
    }
    if (aps.predFixedPointFracBit == 0) {
      predictorRefl = (w2 * lastPointRefl1 + w1 * lastPointRefl2 + (sumW >> 1)) / sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (w1 >> 1)) / w1;
      up = lastPointRefl1 * sw;
      bottom = sw;
      sw = (distanceScale + (w2 >> 1)) / w2;
      up += lastPointRefl2 * sw;
      bottom += sw;
      predictorRefl = up / bottom;
    }
  } else {
    auto neighborPos1 = neighborSet[minIdx - 1].pos;
    auto neighborPos2 = neighborSet[minIdx - 2].pos;
    auto neighborPos3 = neighborSet[minIdx - 3].pos;
    auto lastPointColor1 = neighborSet[minIdx - 1].color;
    auto lastPointColor2 = neighborSet[minIdx - 2].color;
    auto lastPointColor3 = neighborSet[minIdx - 3].color;
    auto lastPointRefl1 = neighborSet[minIdx - 1].refl;
    auto lastPointRefl2 = neighborSet[minIdx - 2].refl;
    auto lastPointRefl3 = neighborSet[minIdx - 3].refl;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    auto w3 = getDisNorm1(curPos, neighborPos3, 1);
    int64_t sumW = w1 * w2 + w2 * w3 + w1 * w3;
    assert(sumW > 0);
    for (int j = 0; j < 3; ++j) {
      predictorColor[j] = (w2 * w3 * lastPointColor1[j] + w1 * w3 * lastPointColor2[j] +
                           w1 * w2 * lastPointColor3[j] + (sumW >> 1)) /
        sumW;
    }
    if (aps.predFixedPointFracBit == 0) {
      predictorRefl = (w2 * w3 * lastPointRefl1 + w1 * w3 * lastPointRefl2 +
                       w1 * w2 * lastPointRefl3 + (sumW >> 1)) /
        sumW;
    } else {
      int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
      int64_t up, bottom, sw;
      sw = (distanceScale + (w1 >> 1)) / w1;
      up = lastPointRefl1 * sw;
      bottom = sw;
      sw = (distanceScale + (w2 >> 1)) / w2;
      up += lastPointRefl2 * sw;
      bottom += sw;
      sw = (distanceScale + (w3 >> 1)) / w3;
      up += lastPointRefl3 * sw;
      bottom += sw;
      predictorRefl = up / bottom;
    }
  }
  predictorAttr.first = predictorColor;
  predictorAttr.second = predictorRefl;
  return predictorAttr;
}

void calculateReflTrend(const PC_POS& curPosition, const PC_POS& prePosition,
                        const PC_REFL& curReflectance, const PC_REFL& preReflectance,
                        uint64_t reflectanceDistCoef, vector<int64_t>& reflectanceRes,
                        vector<int64_t>& reflResNum, vector<int>& reflectanceDistWeight,
                        int groupSize) {
  int64_t resRelfectace = abs(curReflectance - preReflectance) << reflectanceDistCoef;
  int distX = abs(curPosition[0] - prePosition[0]);
  int distY = abs(curPosition[1] - prePosition[1]);
  int distZ = abs(curPosition[2] - prePosition[2]);
  if (distZ > distY && distZ > distX) {
    reflectanceRes[2] += resRelfectace / distZ;
    reflResNum[2]++;
  } else if (distY > distZ && distY > distX) {
    reflectanceRes[1] += resRelfectace / distY;
    reflResNum[1]++;
  } else if (distX > distZ && distX > distY) {
    reflectanceRes[0] += resRelfectace / distX;
    reflResNum[0]++;
  }
  if (reflResNum[0] + reflResNum[1] + reflResNum[2] > groupSize) {
    uint64_t reflectanceResXYZ[3] = {0, 0, 0};
    for (int k = 0; k < 3; k++) {
      if (reflResNum[k] != 0)
        reflectanceResXYZ[k] = reflectanceRes[k] / reflResNum[k];
    }
    int64_t reflectanceResXYZsum =
      reflectanceResXYZ[0] + reflectanceResXYZ[1] + reflectanceResXYZ[2];
    if (reflectanceResXYZsum != 0) {
      for (int k = 0; k < 3; k++) {
        reflectanceDistWeight[k] = (reflectanceResXYZ[k] << 7) / reflectanceResXYZsum;
      }
      reflResNum[0] = reflResNum[1] = reflResNum[2] = 0;
      reflectanceRes[0] = reflectanceRes[1] = reflectanceRes[2] = 0;
    } else {
      reflectanceDistWeight[0] = reflectanceDistWeight[1] = reflectanceDistWeight[2] = 0;
      reflResNum[0] = reflResNum[1] = reflResNum[2] = 0;
      reflectanceRes[0] = reflectanceRes[1] = reflectanceRes[2] = 0;
    }
  }
}

Void getMultiReflectancePredictorFarthest(
  int curIndex, int minIdx, TComPointCloud& outputPointCloud, const SequenceParameterSet& sps,
  const AttributeParameterSet& aps, vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
  vector<multiReflNeighborSet>& neighborSet, PC_REFL* predRef, const int multil_ID_Group_Num) {
  auto& pointIndex = pointCloudCode[curIndex].index;
  auto& curPos = outputPointCloud[pointIndex];
  int farestpoint = 0;
  int64_t farestlength = 0;
  int setlength = aps.maxNumOfNeighbours;
  int64_t distanceScale = (1 << aps.predFixedPointFracBit) - 1;
  int64_t up, bottom, k1, k2, k3, sumW;
  if (minIdx > setlength) {
    int64_t dis;
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    for (int i = 0; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      dis = getDisNorm1(curPos, neighborPos, aps.axisBias);
      if (dis > farestlength && i >= subGroupCount) {
        farestlength = dis;
        farestpoint = i;
      }
      if (neis[1].dis > dis) {
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].dis = dis;
          neis[0].index = i;
          continue;
        }
        neis[2] = neis[1];
        neis[1].dis = dis;
        neis[1].index = i;
        continue;
      }
      if (neis[2].dis > dis) {
        neis[2].dis = dis;
        neis[2].index = i;
      }
    }
    auto w1 = neis[0].dis;
    auto w2 = neis[1].dis;
    auto w3 = neis[2].dis;
    if (aps.predFixedPointFracBit == 0) {
      k1 = w1 * w2;
      k2 = w2 * w3;
      k3 = w1 * w3;
      sumW = k1 + k2 + k3;
    } else {
      k1 = (distanceScale + (w1 >> 1)) / w1;
      bottom = k1;
      k2 = (distanceScale + (w2 >> 1)) / w2;
      bottom += k2;
      k3 = (distanceScale + (w3 >> 1)) / w3;
      bottom += k3;
    }
    assert(w1 > 0);  // assume no duplicate point

    for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
      auto ref1 = neighborSet[neis[0].index].refl[multi_id];
      auto ref2 = neighborSet[neis[1].index].refl[multi_id];
      auto ref3 = neighborSet[neis[2].index].refl[multi_id];
      if (aps.predFixedPointFracBit == 0) {
        predRef[multi_id] = (k2 * ref1 + k3 * ref2 + k1 * ref3) / sumW;
      } else {
        up = ref1 * k1;
        up += ref2 * k2;
        up += ref3 * k3;
        predRef[multi_id] = up / bottom;
      }
      if (sps.reflThreshold) {
        if (abs(ref1 - ref3) >= sps.reflThreshold) {
          predRef[multi_id] = ref1;
        }
      }
    }
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
  } else if (minIdx == 0) {
    for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
      predRef[multi_id] = 0;
    }
  } else if (minIdx == 1) {
    for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
      predRef[multi_id] = neighborSet[0].refl[multi_id];
    }
  } else if (minIdx == 2) {
    const PC_POS last1Point = neighborSet[1].pos;
    const PC_POS last2Point = neighborSet[0].pos;
    const int64_t weight1 = (abs(curPos[0] - last1Point[0]) + abs(curPos[1] - last1Point[1]) +
                             aps.axisBias * abs(curPos[2] - last1Point[2]));
    const int64_t weight2 = (abs(curPos[0] - last2Point[0]) + abs(curPos[1] - last2Point[1]) +
                             aps.axisBias * abs(curPos[2] - last2Point[2]));
    const auto last1Reflectance = neighborSet[1].refl;
    const auto last2Reflectance = neighborSet[0].refl;
    assert(weight1 + weight2 > 0);  // assume no duplicate point
    if (aps.predFixedPointFracBit == 0) {
      sumW = weight1 + weight2;
    } else {
      k1 = (distanceScale + (weight1 >> 1)) / weight1;
      bottom = k1;
      k2 = (distanceScale + (weight2 >> 1)) / weight2;
      bottom += k2;
    }
    for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
      const auto last1Reflectance = neighborSet[1].refl[multi_id];
      const auto last2Reflectance = neighborSet[0].refl[multi_id];
      if (aps.predFixedPointFracBit == 0) {
        predRef[multi_id] = (weight2 * last1Reflectance + weight1 * last2Reflectance) / sumW;
      } else {
        up = last1Reflectance * k1;
        up += last2Reflectance * k2;
        predRef[multi_id] = up / bottom;
      }
    }
  } else {
    const PC_POS last1Point = neighborSet[minIdx - 1].pos;
    const PC_POS last2Point = neighborSet[minIdx - 2].pos;
    const PC_POS last3Point = neighborSet[minIdx - 3].pos;
    const int64_t weight1 = (abs(curPos[0] - last1Point[0]) + abs(curPos[1] - last1Point[1]) +
                             aps.axisBias * abs(curPos[2] - last1Point[2]));
    const int64_t weight2 = (abs(curPos[0] - last2Point[0]) + abs(curPos[1] - last2Point[1]) +
                             aps.axisBias * abs(curPos[2] - last2Point[2]));
    const int64_t weight3 = (abs(curPos[0] - last3Point[0]) + abs(curPos[1] - last3Point[1]) +
                             aps.axisBias * abs(curPos[2] - last3Point[2]));
    if (aps.predFixedPointFracBit == 0) {
      sumW = weight2 * weight3 + weight1 * weight3 + weight1 * weight2;
      assert(sumW > 0);  // assume no duplicate point
    } else {
      k1 = (distanceScale + (weight1 >> 1)) / weight1;
      bottom = k1;
      k2 = (distanceScale + (weight2 >> 1)) / weight2;
      bottom += k2;
      k3 = (distanceScale + (weight3 >> 1)) / weight3;
      bottom += k3;
    }
    for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
      const auto last1Reflectance = neighborSet[minIdx - 1].refl[multi_id];
      const auto last2Reflectance = neighborSet[minIdx - 2].refl[multi_id];
      const auto last3Reflectance = neighborSet[minIdx - 3].refl[multi_id];
      if (aps.predFixedPointFracBit == 0) {
        predRef[multi_id] =
          (weight2 * weight3 * last1Reflectance + weight1 * weight3 * last2Reflectance +
           weight1 * weight2 * last3Reflectance) /
          sumW;
      } else {
        up = last1Reflectance * k1;
        up += last2Reflectance * k2;
        up += last3Reflectance * k3;
        predRef[multi_id] = up / bottom;
      }
      if (sps.reflThreshold) {
        if (abs(last1Reflectance - last3Reflectance) >= sps.reflThreshold) {
          predRef[multi_id] = last1Reflectance;
        }
      }
    }
  }
}
