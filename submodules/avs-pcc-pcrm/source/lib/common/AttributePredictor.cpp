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

int64_t getDisNorm1(PC_POS& posA, PC_POS& posB, UInt axisBias) {
  return (int64_t)(abs(posA[0] - posB[0]) + abs(posA[1] - posB[1]) +
                   axisBias * abs(posA[2] - posB[2]));
}

int64_t getDisNorm1(PC_POS& posA, PC_POS& posB, const V3<int64_t>& distWeight) {
  int64_t dist =
    ((uint64_t)((abs(posA[0] - posB[0]) * distWeight[0] + abs(posA[1] - posB[1]) * distWeight[1] +
                 distWeight[2] * abs(posA[2] - posB[2])))) >>
    3;
  return dist > 0 ? dist : 1;
}

void calColorPredictor(const int64_t w1, const int64_t w2, const int64_t w3, const PC_COL& color1,
                       const PC_COL& color2, const V3<int32_t>& lastcolor, PC_COL& predictorColor,
                       int colorQP, int count) {
  if ((w1 == 0) || (w2 == 0) || (w3 == 0)) {
    predictorColor[0] = (color1[0] + color2[0] + lastcolor[0] + ((count + 3) >> 1)) / (count + 3);
    predictorColor[1] = (color1[1] + color2[1] + lastcolor[1] + ((count + 3) >> 1)) / (count + 3);
    predictorColor[2] = (color1[2] + color2[2] + lastcolor[2] + ((count + 3) >> 1)) / (count + 3);
  } else {
    int64_t sumW;
    int64_t dw1 = w2 * w3;
    int64_t dw2 = w1 * w3;
    int64_t dw3 = w1 * w2;
    if (w3 > w2) {
      int countp1 = count + 1;
      if (colorQP == 0) {
        sumW = (dw1 + dw2 + dw3) * (countp1);
        predictorColor[0] = (dw1 * color1[0] * (countp1) + dw2 * color2[0] * (countp1) +
                             dw3 * lastcolor[0] + (sumW >> 1)) /
          sumW;
        predictorColor[1] = (dw1 * color1[1] * (countp1) + dw2 * color2[1] * (countp1) +
                             dw3 * lastcolor[1] + (sumW >> 1)) /
          sumW;
        predictorColor[2] = (dw1 * color1[2] * (countp1) + dw2 * color2[2] * (countp1) +
                             dw3 * lastcolor[2] + (sumW >> 1)) /
          sumW;
      } else {
        sumW = dw1 + dw2 + dw3 * countp1;
        predictorColor[0] =
          (dw1 * color1[0] + dw2 * color2[0] + dw3 * lastcolor[0] + (sumW >> 1)) / sumW;
        predictorColor[1] =
          (dw1 * color1[1] + dw2 * color2[1] + dw3 * lastcolor[1] + (sumW >> 1)) / sumW;
        predictorColor[2] =
          (dw1 * color1[2] + dw2 * color2[2] + dw3 * lastcolor[2] + (sumW >> 1)) / sumW;
      }
    } else if ((w3 == w2) && (w2 > w1)) {
      int countp2 = count + 2;
      if (colorQP == 0) {
        sumW = (w1 + w2) * (countp2);
        predictorColor[0] =
          (w2 * color1[0] * (countp2) + w1 * (color2[0] + lastcolor[0]) + (sumW >> 1)) / sumW;
        predictorColor[1] =
          (w2 * color1[1] * (countp2) + w1 * (color2[1] + lastcolor[1]) + (sumW >> 1)) / sumW;
        predictorColor[2] =
          (w2 * color1[2] * (countp2) + w1 * (color2[2] + lastcolor[2]) + (sumW >> 1)) / sumW;
      } else {
        sumW = (w1 * countp2 + w2);
        predictorColor[0] = (w2 * color1[0] + w1 * (color2[0] + lastcolor[0]) + (sumW >> 1)) / sumW;
        predictorColor[1] = (w2 * color1[1] + w1 * (color2[1] + lastcolor[1]) + (sumW >> 1)) / sumW;
        predictorColor[2] = (w2 * color1[2] + w1 * (color2[2] + lastcolor[2]) + (sumW >> 1)) / sumW;
      }
    } else {
      predictorColor[0] = (color1[0] + color2[0] + lastcolor[0] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[1] = (color1[1] + color2[1] + lastcolor[1] + ((count + 3) >> 1)) / (count + 3);
      predictorColor[2] = (color1[2] + color2[2] + lastcolor[2] + ((count + 3) >> 1)) / (count + 3);
    }
  }
}

void calReflPredictor(const int64_t w1, const int64_t w2, const int64_t w3, const PC_REFL& refl1,
                      const PC_REFL& refl2, const PC_REFL& refl3, PC_REFL& predictorRefl,
                      int reflThreshold, UInt predFixedPointFracBit) {
  if (reflThreshold && abs(refl1 - refl3) >= reflThreshold) {
    predictorRefl = refl1;
  } else {
    if ((w1 == 0) || (w2 == 0) || (w3 == 0)) {
      predictorRefl = round((refl1 + refl2 + refl3) / 3);
    } else {
      if (predFixedPointFracBit == 0) {
        int64_t dw1 = w2 * w3;
        int64_t dw2 = w1 * w3;
        int64_t dw3 = w1 * w2;
        int64_t sumW = dw1 + dw2 + dw3;
        predictorRefl = (dw1 * refl1 + dw2 * refl2 + dw3 * refl3 + (sumW >> 1)) / sumW;
      } else {
        int64_t distanceScale = (1 << predFixedPointFracBit) - 1;
        int64_t up, bottom, sw;
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
    }
  }
}

void getColorPredictorFarthest(int curIndex, int minIdx, PC_POS curPos, int& subGroupCount,
                               vector<neighborSet>& neighborSet, predictOptParams& params,
                               PC_COL& predictorColor) {
  int farestpoint = 0;
  int64_t farestlength = 0;
  int setlength = params.maxNumOfNeighbours;
  int64_t dw1, dw2, dw3, sumW;
  if (minIdx > setlength) {
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    int64_t dis;
    int64_t disG;
    std::vector<int32_t> indexes;
    indexes.resize(params.maxNumOfNeighbours - 3);
    int count = 0;

    auto updateNeighbors = [&](int64_t dis, int i) {
      if (neis[1].dis > dis) {
        if (neis[2].dis == neis[1].dis && neis[2].dis != INT_FAST64_MAX) {
          indexes[count] = neis[2].index;
          count++;
        } else
          count = 0;
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0] = {dis, i};
          return;
        }
        neis[2] = neis[1];
        neis[1] = {dis, i};
        return;
      }
      if (neis[2].dis > dis) {
        neis[2] = {dis, i};
        count = 0;
        return;
      }
      if (neis[2].dis == dis) {
        indexes[count] = i;
        count++;
      }
    };

    if (!params.colorUpdateFlag) {
      for (int i = 0; i < setlength; i++) {
        auto neighborPos = neighborSet[i].pos;
        dis = getDisNorm1(curPos, neighborPos, 1);
        updateNeighbors(dis, i);
      }
    } else if (params.colorCrossAttrTypePred) {
      for (int i = 0; i < setlength; i++) {
        auto neighborPos = neighborSet[i].pos;
        disG = getDisNorm1(curPos, neighborPos, 1);
        auto lastPointRefl = neighborSet[i].reflWithCoef;
        int64_t disRefl = (abs((int64_t)(params.curReflWithCoef - lastPointRefl))) >> 10;
        dis = disG + disRefl;
        if (disG > farestlength && i >= subGroupCount) {
          farestlength = disG;
          farestpoint = i;
        }
        updateNeighbors(dis, i);
      }
    } else {  // colorUpdateFlag = true, colorCrossAttrTypePred = false
      for (int i = 0; i < setlength; i++) {
        auto neighborPos = neighborSet[i].pos;
        dis = getDisNorm1(curPos, neighborPos, 1);
        if (dis > farestlength && i >= subGroupCount) {
          farestlength = dis;
          farestpoint = i;
        }
        updateNeighbors(dis, i);
      }
    }

    auto w3 = neis[2].dis;
    auto color3 = neighborSet[neis[2].index].color;
    auto w2 = neis[1].dis;
    auto color2 = neighborSet[neis[1].index].color;
    auto w1 = neis[0].dis;
    auto color1 = neighborSet[neis[0].index].color;

    if (params.minNeighborDisFlag) {
      params.minNeighborDis = getDisNorm1(curPos, neighborSet[neis[0].index].pos, 1);
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
    calColorPredictor(w1, w2, w3, color1, color2, lastcolor, predictorColor, params.colorQP, count);

    if (params.colorUpdateFlag) {
      std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
    }
  } else if (minIdx == 0) {
    predictorColor = {128, 128, 128};
  } else if (minIdx == 1) {
    predictorColor = neighborSet[0].color;
  } else if (minIdx == 2) {
    auto color2 = neighborSet[1].color;
    auto color1 = neighborSet[0].color;
    auto neighborPos2 = neighborSet[1].pos;
    auto neighborPos1 = neighborSet[0].pos;
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    if ((w1 == 0) || (w2 == 0)) {
      predictorColor[0] = round((color1[0] + color2[0]) / 2);
      predictorColor[1] = round((color1[1] + color2[1]) / 2);
      predictorColor[2] = round((color1[2] + color2[2]) / 2);
    } else {
      sumW = w1 + w2;
      for (int j = 0; j < 3; ++j) {
        predictorColor[j] = (w2 * color1[j] + w1 * color2[j] + (sumW >> 1)) / sumW;
      }
    }
  } else {
    auto color1 = neighborSet[minIdx - 1].color;
    auto color2 = neighborSet[minIdx - 2].color;
    auto color3 = neighborSet[minIdx - 3].color;
    auto neighborPos1 = neighborSet[minIdx - 1].pos;
    auto neighborPos2 = neighborSet[minIdx - 2].pos;
    auto neighborPos3 = neighborSet[minIdx - 3].pos;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    auto w3 = getDisNorm1(curPos, neighborPos3, 1);
    if ((w1 == 0) || (w2 == 0) || (w3 == 0)) {
      predictorColor[0] = round((color1[0] + color2[0] + color3[0]) / 3);
      predictorColor[1] = round((color1[1] + color2[1] + color3[1]) / 3);
      predictorColor[2] = round((color1[2] + color2[2] + color3[2]) / 3);
    } else {
      dw1 = w2 * w3;
      dw2 = w1 * w3;
      dw3 = w1 * w2;
      sumW = dw1 + dw2 + dw3;
      for (int j = 0; j < 3; ++j) {
        predictorColor[j] =
          (dw1 * color1[j] + dw2 * color2[j] + dw3 * color3[j] + (sumW >> 1)) / sumW;
      }
    }
  }
}

void getReflPredictorFarthest(int curIndex, int minIdx, PC_POS curPos, int& subGroupCount,
                              vector<neighborSet>& neighborSet, predictOptParams& params,
                              PC_REFL& predictorRefl) {
  int farestpoint = 0;
  int64_t farestlength = 0;
  int setlength = params.maxNumOfNeighbours;

  if (minIdx > setlength) {
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    int64_t dis;

    for (int i = 0; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      dis = getDisNorm1(curPos, neighborPos, params.axisBias);
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
    auto refl1 = neighborSet[neis[0].index].refl;
    auto w2 = neis[1].dis;
    auto refl2 = neighborSet[neis[1].index].refl;
    auto w3 = neis[2].dis;
    auto refl3 = neighborSet[neis[2].index].refl;

    calReflPredictor(w1, w2, w3, refl1, refl2, refl3, predictorRefl, params.reflThreshold,
                     params.predFixedPointFracBit);
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);

  } else if (minIdx == 0) {
    predictorRefl = 0;
  } else if (minIdx == 1) {
    predictorRefl = neighborSet[0].refl;
  } else if (minIdx == 2) {
    PC_POS neighborPos1 = neighborSet[0].pos;
    PC_POS neighborPos2 = neighborSet[1].pos;
    int64_t w1 = getDisNorm1(curPos, neighborPos1, params.axisBias);
    int64_t w2 = getDisNorm1(curPos, neighborPos2, params.axisBias);
    auto refl1 = neighborSet[0].refl;
    auto refl2 = neighborSet[1].refl;

    if ((w1 == 0) || (w2 == 0)) {
      predictorRefl = round((refl1 + refl2) / 2);
    } else {
      int64_t sumW = w1 + w2;
      if (params.predFixedPointFracBit == 0) {
        predictorRefl = (w2 * refl1 + w1 * refl2 + (sumW >> 1)) / sumW;
      } else {
        int64_t distanceScale = (1 << params.predFixedPointFracBit) - 1;
        int64_t up, bottom, sw;
        sw = (distanceScale + (w1 >> 1)) / w1;
        up = refl1 * sw;
        bottom = sw;
        sw = (distanceScale + (w2 >> 1)) / w2;
        up += refl2 * sw;
        bottom += sw;
        predictorRefl = up / bottom;
      }
    }
  } else {
    PC_POS neighborPos1 = neighborSet[minIdx - 1].pos;
    PC_POS neighborPos2 = neighborSet[minIdx - 2].pos;
    PC_POS neighborPos3 = neighborSet[minIdx - 3].pos;
    int64_t w1 = getDisNorm1(curPos, neighborPos1, params.axisBias);
    int64_t w2 = getDisNorm1(curPos, neighborPos2, params.axisBias);
    int64_t w3 = getDisNorm1(curPos, neighborPos3, params.axisBias);
    auto refl1 = neighborSet[minIdx - 1].refl;
    auto refl2 = neighborSet[minIdx - 2].refl;
    auto refl3 = neighborSet[minIdx - 3].refl;
    calReflPredictor(w1, w2, w3, refl1, refl2, refl3, predictorRefl, params.reflThreshold,
                     params.predFixedPointFracBit);
  }
}

void getReflPredictorFarthestDual(int curIndex, int minIdx, PC_POS curPos, int& subGroupCount,
                                  vector<neighborSet>& neighborSet, predictOptParams& params,
                                  PC_REFL& predictorRefl) {
  int farestpoint = 0;
  int64_t farestlength = 0;
  int setlength = params.maxNumOfNeighbours;

  if (minIdx > setlength) {
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    int64_t dis;
    int64_t disG;

    auto updateNeighbors = [&](int64_t dis, int i) {
      if (neis[1].dis > dis) {
        if (neis[0].dis > dis) {
          neis[2] = neis[1];
          neis[1] = neis[0];
          neis[0].dis = dis;
          neis[0].index = i;
          return;
        }
        neis[2] = neis[1];
        neis[1].dis = dis;
        neis[1].index = i;
        return;
      }
      if (neis[2].dis > dis) {
        neis[2].dis = dis;
        neis[2].index = i;
      }
    };

    if (!params.reflUpdateFlag) {
      for (int i = 0; i < setlength; i++) {
        auto neighborPos = neighborSet[i].pos;
        if (params.reflectanceDistWeight[0] != 0 && params.reflectanceDistWeight[1] != 0 &&
            params.reflectanceDistWeight[2] != 0)
          dis = getDisNorm1(curPos, neighborPos, params.reflectanceDistWeight);
        else
          dis = getDisNorm1(curPos, neighborPos, params.axisBias);
        updateNeighbors(dis, i);
      }
    } else if (params.reflCrossAttrTypePred) {
      for (int i = 0; i < setlength; i++) {
        auto neighborPos = neighborSet[i].pos;
        if (params.reflectanceDistWeight[0] != 0 && params.reflectanceDistWeight[1] != 0 &&
            params.reflectanceDistWeight[2] != 0)
          disG = getDisNorm1(curPos, neighborPos, params.reflectanceDistWeight);
        else
          disG = getDisNorm1(curPos, neighborPos, params.axisBias);

        auto lastColor = neighborSet[i].colorWithCoef;
        auto curColorWithCoef = params.curColorWithCoef;
        int64_t disColor = abs(curColorWithCoef[0] - lastColor[0]) +
          abs(curColorWithCoef[1] - lastColor[1]) + abs(curColorWithCoef[2] - lastColor[2]);
        dis = disG + disColor;
        if (disG > farestlength && i >= subGroupCount) {
          farestlength = disG;
          farestpoint = i;
        }
        updateNeighbors(dis, i);
      }
    } else {  // reflUpdateFlag = true, reflCrossAttrTypePred = false
      for (int i = 0; i < setlength; i++) {
        auto neighborPos = neighborSet[i].pos;
        if (params.reflectanceDistWeight[0] != 0 && params.reflectanceDistWeight[1] != 0 &&
            params.reflectanceDistWeight[2] != 0)
          dis = getDisNorm1(curPos, neighborPos, params.reflectanceDistWeight);
        else
          dis = getDisNorm1(curPos, neighborPos, params.axisBias);

        if (dis > farestlength && i >= subGroupCount) {
          farestlength = dis;
          farestpoint = i;
        }
        updateNeighbors(dis, i);
      }
    }

    auto w1 = neis[0].dis;
    auto refl1 = neighborSet[neis[0].index].refl;
    auto w2 = neis[1].dis;
    auto refl2 = neighborSet[neis[1].index].refl;
    auto w3 = neis[2].dis;
    auto refl3 = neighborSet[neis[2].index].refl;
    calReflPredictor(w1, w2, w3, refl1, refl2, refl3, predictorRefl, params.reflThreshold,
                     params.predFixedPointFracBit);
    if (params.reflUpdateFlag) {
      std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
    }

  } else if (minIdx == 0) {
    predictorRefl = 0;
  } else if (minIdx == 1) {
    predictorRefl = neighborSet[0].refl;
  } else if (minIdx == 2) {
    PC_POS neighborPos1 = neighborSet[0].pos;
    PC_POS neighborPos2 = neighborSet[1].pos;
    int64_t w1 = getDisNorm1(curPos, neighborPos1, params.axisBias);
    int64_t w2 = getDisNorm1(curPos, neighborPos2, params.axisBias);
    auto refl1 = neighborSet[0].refl;
    auto refl2 = neighborSet[1].refl;
    if ((w1 == 0) || (w2 == 0)) {
      predictorRefl = round((refl1 + refl2) / 2);
    } else {
      if (params.predFixedPointFracBit == 0) {
        int64_t sumW = w1 + w2;
        predictorRefl = (w2 * refl1 + w1 * refl2 + (sumW >> 1)) / sumW;
      } else {
        int64_t distanceScale = (1 << params.predFixedPointFracBit) - 1;
        int64_t up, bottom, sw;
        sw = (distanceScale + (w1 >> 1)) / w1;
        up = refl1 * sw;
        bottom = sw;
        sw = (distanceScale + (w2 >> 1)) / w2;
        up += refl2 * sw;
        bottom += sw;
        predictorRefl = up / bottom;
      }
    }
  } else {
    PC_POS neighborPos1 = neighborSet[minIdx - 1].pos;
    PC_POS neighborPos2 = neighborSet[minIdx - 2].pos;
    PC_POS neighborPos3 = neighborSet[minIdx - 3].pos;
    int64_t w1 = getDisNorm1(curPos, neighborPos1, params.axisBias);
    int64_t w2 = getDisNorm1(curPos, neighborPos2, params.axisBias);
    int64_t w3 = getDisNorm1(curPos, neighborPos3, params.axisBias);
    auto refl1 = neighborSet[minIdx - 1].refl;
    auto refl2 = neighborSet[minIdx - 2].refl;
    auto refl3 = neighborSet[minIdx - 3].refl;
    calReflPredictor(w1, w2, w3, refl1, refl2, refl3, predictorRefl, params.reflThreshold,
                     params.predFixedPointFracBit);
  }
}

void getAttributePredictorFarthest(int curIndex, int minIdx, PC_POS curPos, int& subGroupCount,
                                   vector<neighborSet>& neighborSet, predictOptParams& params,
                                   pair<PC_COL, PC_REFL>& predictorAttr) {
  PC_COL predictorColor;
  PC_REFL predictorRefl;
  int farestpoint = 0;
  int64_t farestlength = 0;
  int setlength = neighborSet.size();
  int64_t sumW, dw1, dw2, dw3;

  if (minIdx > setlength) {
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    int64_t dis;
    /*   int32_t indexes[125];*/
    std::vector<int32_t> indexes;
    indexes.resize(params.maxNumOfNeighbours - 3);
    int count = 0;
    for (int i = 0; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      dis = getDisNorm1(curPos, neighborPos, 1);
      if (dis > farestlength && i >= subGroupCount) {
        farestlength = dis;
        farestpoint = i;
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
    auto color3 = neighborSet[neis[2].index].color;
    auto refl3 = neighborSet[neis[2].index].refl;
    auto w2 = neis[1].dis;
    auto color2 = neighborSet[neis[1].index].color;
    auto refl2 = neighborSet[neis[1].index].refl;
    auto w1 = neis[0].dis;
    auto color1 = neighborSet[neis[0].index].color;
    auto refl1 = neighborSet[neis[0].index].refl;

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

    // color
    calColorPredictor(w1, w2, w3, color1, color2, lastcolor, predictorColor, params.colorQP, count);
    // reflectance
    calReflPredictor(w1, w2, w3, refl1, refl2, refl3, predictorRefl, params.reflThreshold,
                     params.predFixedPointFracBit);
    std::swap(neighborSet[subGroupCount], neighborSet[farestpoint]);
  } else if (minIdx == 0) {
    predictorColor = {128, 128, 128};
    predictorRefl = 128;
  } else if (minIdx == 1) {
    predictorColor = neighborSet[0].color;
    predictorRefl = neighborSet[0].refl;
  } else if (minIdx == 2) {
    auto neighborPos1 = neighborSet[0].pos;
    auto neighborPos2 = neighborSet[1].pos;
    auto color1 = neighborSet[0].color;
    auto color2 = neighborSet[1].color;
    auto refl1 = neighborSet[0].refl;
    auto refl2 = neighborSet[1].refl;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);

    if ((w1 == 0) || (w2 == 0)) {
      predictorColor[0] = round((color1[0] + color2[0]) / 2);
      predictorColor[1] = round((color1[1] + color2[1]) / 2);
      predictorColor[2] = round((color1[2] + color2[2]) / 2);

      predictorRefl = round((refl1 + refl2) / 2);
    } else {
      sumW = w1 + w2;
      for (int j = 0; j < 3; ++j) {
        predictorColor[j] = (w2 * color1[j] + w1 * color2[j] + (sumW >> 1)) / sumW;
      }
      if (params.predFixedPointFracBit == 0) {
        predictorRefl = (w2 * refl1 + w1 * refl2 + (sumW >> 1)) / sumW;
      } else {
        int64_t distanceScale = (1 << params.predFixedPointFracBit) - 1;
        int64_t up, bottom, sw;
        sw = (distanceScale + (w1 >> 1)) / w1;
        up = refl1 * sw;
        bottom = sw;
        sw = (distanceScale + (w2 >> 1)) / w2;
        up += refl2 * sw;
        bottom += sw;
        predictorRefl = up / bottom;
      }
    }

  } else {
    auto neighborPos1 = neighborSet[minIdx - 1].pos;
    auto neighborPos2 = neighborSet[minIdx - 2].pos;
    auto neighborPos3 = neighborSet[minIdx - 3].pos;
    auto color1 = neighborSet[minIdx - 1].color;
    auto color2 = neighborSet[minIdx - 2].color;
    auto color3 = neighborSet[minIdx - 3].color;
    auto refl1 = neighborSet[minIdx - 1].refl;
    auto refl2 = neighborSet[minIdx - 2].refl;
    auto refl3 = neighborSet[minIdx - 3].refl;
    auto w1 = getDisNorm1(curPos, neighborPos1, 1);
    auto w2 = getDisNorm1(curPos, neighborPos2, 1);
    auto w3 = getDisNorm1(curPos, neighborPos3, 1);

    if ((w1 == 0) || (w2 == 0) || (w3 == 0)) {
      predictorColor[0] = round((color1[0] + color2[0] + color3[0]) / 3);
      predictorColor[1] = round((color1[1] + color2[1] + color3[1]) / 3);
      predictorColor[2] = round((color1[2] + color2[2] + color3[2]) / 3);
    } else {
      dw1 = w2 * w3;
      dw2 = w1 * w3;
      dw3 = w1 * w2;
      sumW = dw1 + dw2 + dw3;
      for (int j = 0; j < 3; ++j) {
        predictorColor[j] =
          (dw1 * color1[j] + dw2 * color2[j] + dw3 * color3[j] + (sumW >> 1)) / sumW;
      }
    }

    calReflPredictor(w1, w2, w3, refl1, refl2, refl3, predictorRefl, params.reflThreshold,
                     params.predFixedPointFracBit);
  }
  predictorAttr.first = predictorColor;
  predictorAttr.second = predictorRefl;
}

void calculateReflTrend(const PC_POS& curPosition, const PC_POS& prePosition,
                        const PC_REFL& curReflectance, const PC_REFL& preReflectance,
                        UInt reflectanceDistCoef, V3<int64_t>& reflectanceRes,
                        V3<int64_t>& reflResNum, V3<int64_t>& reflectanceDistWeight,
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

Void getMultiReflectancePredictorFarthest(int curIndex, int minIdx, PC_POS curPos,
                                          int& subGroupCount,
                                          vector<multiReflNeighborSet>& neighborSet,
                                          predictOptParams& params, PC_REFL* predRef,
                                          const int multil_ID_Group_Num) {
  int farestpoint = 0;
  int64_t farestlength = 0;
  int setlength = params.maxNumOfNeighbours;
  int64_t distanceScale = (1 << params.predFixedPointFracBit) - 1;
  int64_t up, bottom, dw1, dw2, dw3, sumW;
  if (minIdx > setlength) {
    int64_t dis;
    Neighbor neis[3];
    neis[0].dis = neis[1].dis = neis[2].dis = INT_FAST64_MAX;
    for (int i = 0; i < setlength; i++) {
      auto neighborPos = neighborSet[i].pos;
      dis = getDisNorm1(curPos, neighborPos, params.axisBias);
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

    if ((w1 == 0) || (w2 == 0) || (w3 == 0)) {
      for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
        auto refl1 = neighborSet[neis[0].index].refl[multi_id];
        auto refl2 = neighborSet[neis[1].index].refl[multi_id];
        auto refl3 = neighborSet[neis[2].index].refl[multi_id];
        if (params.reflThreshold) {
          if (abs(refl1 - refl3) >= params.reflThreshold) {
            predRef[multi_id] = refl1;
          }
        } else {
          predRef[multi_id] = round((refl1 + refl2 + refl3) / 3);
        }
      }
    } else {
      if (params.predFixedPointFracBit == 0) {
        dw1 = w2 * w3;
        dw2 = w1 * w3;
        dw3 = w1 * w2;
        sumW = dw1 + dw2 + dw3;
      } else {
        dw1 = (distanceScale + (w1 >> 1)) / w1;
        bottom = dw1;
        dw2 = (distanceScale + (w2 >> 1)) / w2;
        bottom += dw2;
        dw3 = (distanceScale + (w3 >> 1)) / w3;
        bottom += dw3;
      }

      for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
        auto refl1 = neighborSet[neis[0].index].refl[multi_id];
        auto refl2 = neighborSet[neis[1].index].refl[multi_id];
        auto refl3 = neighborSet[neis[2].index].refl[multi_id];
        if (params.reflThreshold) {
          if (abs(refl1 - refl3) >= params.reflThreshold) {
            predRef[multi_id] = refl1;
          }
        } else {
          if (params.predFixedPointFracBit == 0) {
            predRef[multi_id] = (dw1 * refl1 + dw2 * refl2 + dw3 * refl3 + (sumW >> 1)) / sumW;
          } else {
            up = refl1 * dw1;
            up += refl2 * dw2;
            up += refl3 * dw3;
            predRef[multi_id] = up / bottom;
          }
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
    PC_POS neighborPos1 = neighborSet[0].pos;
    PC_POS neighborPos2 = neighborSet[1].pos;
    int64_t w1 = getDisNorm1(curPos, neighborPos1, params.axisBias);
    int64_t w2 = getDisNorm1(curPos, neighborPos2, params.axisBias);

    if ((w1 == 0) || (w2 == 0)) {
      for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
        const auto refl1 = neighborSet[0].refl[multi_id];
        const auto refl2 = neighborSet[1].refl[multi_id];
        predRef[multi_id] = round((refl1 + refl2) / 2);
      }
    } else {
      if (params.predFixedPointFracBit == 0) {
        sumW = w1 + w2;
      } else {
        dw1 = (distanceScale + (w1 >> 1)) / w1;
        bottom = dw1;
        dw2 = (distanceScale + (w2 >> 1)) / w2;
        bottom += dw2;
      }
      for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
        const auto refl1 = neighborSet[0].refl[multi_id];
        const auto refl2 = neighborSet[1].refl[multi_id];
        if (params.predFixedPointFracBit == 0) {
          predRef[multi_id] = (w2 * refl1 + w1 * refl2 + (sumW >> 1)) / sumW;
        } else {
          up = refl1 * dw1;
          up += refl2 * dw2;
          predRef[multi_id] = up / bottom;
        }
      }
    }
  } else {
    PC_POS neighborPos1 = neighborSet[minIdx - 1].pos;
    PC_POS neighborPos2 = neighborSet[minIdx - 2].pos;
    PC_POS neighborPos3 = neighborSet[minIdx - 3].pos;
    int64_t w1 = getDisNorm1(curPos, neighborPos1, params.axisBias);
    int64_t w2 = getDisNorm1(curPos, neighborPos2, params.axisBias);
    int64_t w3 = getDisNorm1(curPos, neighborPos3, params.axisBias);
    if ((w1 == 0) || (w2 == 0) || (w3 == 0)) {
      for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
        const auto refl1 = neighborSet[minIdx - 1].refl[multi_id];
        const auto refl2 = neighborSet[minIdx - 2].refl[multi_id];
        const auto refl3 = neighborSet[minIdx - 3].refl[multi_id];
        if (params.reflThreshold) {
          if (abs(refl1 - refl3) >= params.reflThreshold) {
            predRef[multi_id] = refl1;
          }
        } else {
          predRef[multi_id] = round((refl1 + refl2 + refl3) / 3);
        }
      }
    } else {
      if (params.predFixedPointFracBit == 0) {
        dw1 = w2 * w3;
        dw2 = w1 * w3;
        dw3 = w1 * w2;
        sumW = dw1 + dw2 + dw3;
      } else {
        dw1 = (distanceScale + (w1 >> 1)) / w1;
        bottom = dw1;
        dw2 = (distanceScale + (w2 >> 1)) / w2;
        bottom += dw2;
        dw3 = (distanceScale + (w3 >> 1)) / w3;
        bottom += dw3;
      }
      for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
        const auto refl1 = neighborSet[minIdx - 1].refl[multi_id];
        const auto refl2 = neighborSet[minIdx - 2].refl[multi_id];
        const auto refl3 = neighborSet[minIdx - 3].refl[multi_id];
        if (params.reflThreshold) {
          if (abs(refl1 - refl3) >= params.reflThreshold) {
            predRef[multi_id] = refl1;
          }
        } else {
          if (params.predFixedPointFracBit == 0) {
            predRef[multi_id] = (dw1 * refl1 + dw2 * refl2 + dw3 * refl3 + (sumW >> 1)) / sumW;
          } else {
            up = refl1 * dw1;
            up += refl2 * dw2;
            up += refl3 * dw3;
            predRef[multi_id] = up / bottom;
          }
        }
      }
    }
  }
}
