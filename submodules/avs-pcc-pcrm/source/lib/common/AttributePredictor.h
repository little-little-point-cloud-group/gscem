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

int64_t getDisNorm1(PC_POS& posA, PC_POS& posB, int axisBias);
int64_t getDisNorm1(PC_POS& posA, PC_POS& posB, const vector<int>& distWeight);

PC_COL getColorPredictorFarthest(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                 const SequenceParameterSet& sps, const AttributeParameterSet& aps,
                                 vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
                                 vector<colorNeighborSet>& neighborSet);

PC_COL getColorPredictorFarthest(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                 const SequenceParameterSet& sps, const AttributeParameterSet& aps,
                                 vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
                                 vector<colorNeighborSet>& neighborSet, int64_t& minNeighborDis);

PC_REFL getReflectancePredictorFarthest(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                        const SequenceParameterSet& sps,
                                        const AttributeParameterSet& aps,
                                        vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount, vector<reflNeighborSet>& neighborSet);
PC_REFL getReflectancePredictorFarthest(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                        const SequenceParameterSet& sps,
                                        const AttributeParameterSet& aps,
                                        vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount, vector<reflNeighborSet>& neighborSet,
                                        const vector<int>& distWeight);

PC_REFL getReflectancePredictorNoUpdate(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                        const SequenceParameterSet& sps,
                                        const AttributeParameterSet& aps,
                                        const vector<pointCodeWithIndex>& pointCloudCode,
                                        int& subGroupCount, vector<colorNeighborSet> neighborSet,
                                        vector<reflWithCoefNeighborSet> reflWithCoefNeighborSet,
                                        vector<reflNeighborSet> reflbuffer, const vector<int>& distWeight);

PC_COL getColorPredictorFromReflectance(
  int curIndex, int minIdx, TComPointCloud& outputPointCloud, const SequenceParameterSet& sps,
  const AttributeParameterSet& aps, vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
  vector<colorNeighborSet>& neighborSet, vector<reflWithCoefNeighborSet>& reflWithCoefNeighborSet,
  const uint64_t curReflWithCoef);

PC_COL getColorPredictorFromReflectance(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                 const SequenceParameterSet& sps, const AttributeParameterSet& aps,
                                 vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
                                 vector<colorNeighborSet>& neighborSet,
                                 vector<reflWithCoefNeighborSet>& reflWithCoefNeighborSet,
                                        const uint64_t curReflWithCoef, int64_t& minNeighborDis);

PC_COL getColorPredictorNoUpdate(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                 const SequenceParameterSet& sps, const AttributeParameterSet& aps,
                                 const vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
                                 vector<reflNeighborSet> neighborSet,
                                 vector<colorWithCoefNeighborSet> colorWithCoefNeighborSet,
                                 vector<colorNeighborSet> reflbuffer);

PC_REFL getReflectancePredictorFromColor(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                  const SequenceParameterSet& sps, const AttributeParameterSet& aps,
                                  vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
                                  vector<reflNeighborSet>& neighborSet,
                                  vector<colorWithCoefNeighborSet>& colorWithCoefNeighborSet,
                                  V3<int64_t> curColorWithCoef, const vector<int>& distWeight);
  
pair<PC_COL, PC_REFL>
  getAttributePredictorFarthest(int curIndex, int minIdx, TComPointCloud& outputPointCloud,
                                const SequenceParameterSet& sps, const AttributeParameterSet& aps,
                                vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
                                vector<attrNeighborSet>& neighborSet);

void calculateReflTrend(const PC_POS& curPosition, const PC_POS& prePosition, const PC_REFL& curReflectance,
    const PC_REFL& preReflectance, uint64_t reflectanceDistCoef, vector<int64_t>& reflectanceRes,
    vector<int64_t>& reflResNum, vector<int>& reflectanceDistWeight, int groupSize);

Void getMultiReflectancePredictorFarthest(
  int curIndex, int minIdx, TComPointCloud& outputPointCloud, const SequenceParameterSet& sps,
  const AttributeParameterSet& aps, vector<pointCodeWithIndex>& pointCloudCode, int& subGroupCount,
  vector<multiReflNeighborSet>& neighborSet, PC_REFL* predRef, const int multil_ID_Group_Num);
