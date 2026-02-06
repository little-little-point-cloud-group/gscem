#pragma once
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

#include "common/HighLevelSyntax.h"
#include "common/TComPointCloud.h"

int64_t getDisNorm1(PC_POS& posA, PC_POS& posB, UInt axisBias);
int64_t getDisNorm1(PC_POS& posA, PC_POS& posB, const V3<int64_t>& distWeight);

void calColorPredictor(const int64_t w1, const int64_t w2, const int64_t w3, const PC_COL& color1,
                       const PC_COL& color2, const V3<int32_t>& lastcolor, PC_COL& predictorColor,
                       int colorQP, int count);
void calReflPredictor(const int64_t w1, const int64_t w2, const int64_t w3, const PC_REFL& refl1,
                      const PC_REFL& refl2, const PC_REFL& refl3, PC_REFL& predictorRefl,
                      int reflThreshold, UInt predFixedPointFracBit);

void getColorPredictorFarthest(int curIndex, int minIdx, PC_POS curPos, int& subGroupCount,
                               vector<neighborSet>& neighborSet, predictOptParams& params,
                               PC_COL& predictorColor);

void getReflPredictorFarthest(int curIndex, int minIdx, PC_POS curPos, int& subGroupCount,
                              vector<neighborSet>& neighborSet, predictOptParams& params,
                              PC_REFL& predictorRefl);

void getReflPredictorFarthestDual(int curIndex, int minIdx, PC_POS curPos, int& subGroupCount,
                                  vector<neighborSet>& neighborSet, predictOptParams& params,
                                  PC_REFL& predictorRefl);

void getAttributePredictorFarthest(int curIndex, int minIdx, PC_POS curPos, int& subGroupCount,
                                   vector<neighborSet>& neighborSet, predictOptParams& params,
                                   pair<PC_COL, PC_REFL>& predictorAttr);

void calculateReflTrend(const PC_POS& curPosition, const PC_POS& prePosition,
                        const PC_REFL& curReflectance, const PC_REFL& preReflectance,
                        UInt reflectanceDistCoef, V3<int64_t>& reflectanceRes,
                        V3<int64_t>& reflResNum, V3<int64_t>& reflectanceDistWeight, int groupSize);

Void getMultiReflectancePredictorFarthest(int curIndex, int minIdx, PC_POS curPos,
                                          int& subGroupCount,
                                          vector<multiReflNeighborSet>& neighborSet,
                                          predictOptParams& params, PC_REFL* predRef,
                                          const int multil_ID_Group_Num);