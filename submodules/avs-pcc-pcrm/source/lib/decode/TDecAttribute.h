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

#include "TDecBacTop.h"
#include "common/CommonDef.h"
#include "common/HighLevelSyntax.h"
#include "common/TComPointCloud.h"

class TDecAttribute {
private:
  TComPointCloud* m_pointCloudRecon;  ///< pointer to output point cloud
  HighLevelSyntax* m_hls;             ///< pointer to high-level syntax parameters
  TDecBacTop* m_decBac;               ///< pointer to bac
  TDecBacTop* m_decBacDual;           ///< pointer to bac2
  double m_colorTime;                 ///< color decoding time
  double m_reflTime;                  ///< reflectance decoding time
  int frame_Idx;
  int frame_count;
  int multil_ID;

public:
  TDecAttribute() = default;
  ~TDecAttribute() = default;

  double& getColorTime() {
    return m_colorTime;
  }
  double& getReflectanceTime() {
    return m_reflTime;
  }

  Void init(TComPointCloud* pointCloudRecon, HighLevelSyntax* hls, TDecBacTop* decBac,
            int multi_ID);
  Void initDual(TComPointCloud* pointCloudRecon, HighLevelSyntax* hls, TDecBacTop* decBac,
                TDecBacTop* decBacDual, int multi_ID);

  Void predictDecodeAttribute();
  Void transformDecodeAttribute();
  Void predictAndTransformDecodeAttribute();

  Void predictDecodeColor();
  Void transformDecodeColor();
  Void transformDecodeColorFromReflectance();
  Void predictAndTransformDecodeColor();

  Void predictDecodeReflectance();
  Void transformDecodeReflectance();
  Void transformDecodeReflectanceFromColor();
  Void predictAndTransformDecodeReflectance();

  Void predictDecodeMultiReflectance();
  Void multiReflectanceInversePredictResidual();
  void multiReflectanceReconstruction(PC_REFL* predictor, PC_REFL* currValue, PC_REFL* reconValue,
                                      int multil_ID_Group_Num);

  Void attributeInversePredictResidual();
  Void reflectanceInversePredictResidual();
  Void reflectanceInversePredictResidualDual();
  Void colorInversePredictResidual();

  Void colorInversePredictAndTransformMemControl();
  Void ReflectanceInversePredictAndTransformMemControl();
  Void AttributeInversePredictAndTransformMemControl();

  Void reflectanceInverseWaveletTransform();
  Void colorInverseWaveletTransform();
  Void reflectanceInverseWaveletTransformFromColor();
  Void colorInverseWaveletTransformFromReflectance();

  void colorReconstruction(const PC_COL& predictor, const V3<int64_t>& codedValue,
                           PC_COL& reconValue, const quantizedQP& colorQp);

  void reflectanceReconstruction(const int64_t& predictor, const int64_t& codedValue,
                                 PC_REFL& reconValue);

  void parseColorResidualCorrelationCode(V3<int64_t>& codedValue, const bool& os,
                                         const bool isDuplicatePoint, const UInt& golombNum);

  void colorReconstructionTrans(std::vector<pointCodeWithIndex>& pointCloudHilbert,
                                std::vector<int>& transformPointIdx, int64_t transformBuf[][8],
                                int64_t transformPredBuf[][8], const quantizedQP& colorQp,
                                int& count, std::vector<neighborSet>& neighborSet,
                                bool colorQPAdjustFlag = false);

  void reflectanceReconstructionTrans(std::vector<pointCodeWithIndex>& pointCloudHilbert,
                                      std::vector<int>& transformPointIdx,
                                      int64_t transformBuf[1][8], int64_t transformPredBuf[1][8],
                                      std::vector<neighborSet>& neighborSet, PC_REFL& lastref);

  void runlengthDecodeMemControl(int& pointCount, int* Coefficients, int& run_length,
                                 int lengthControl, bool& isLengthControl,
                                 const bool isColor = false);

  void setCoeffIndex(int& groupCount, const vector<int>& numofGroupCount, const vector<int>& length,
                     int& dcIndex, int& acIndex, int& numofCoeff);
};  ///< END CLASS TDecGeometry
