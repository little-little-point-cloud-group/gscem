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

#include "TEncBacTop.h"
#include "common/HighLevelSyntax.h"
#include "common/TComPointCloud.h"

class TEncAttribute {
private:
  TComPointCloud* m_pointCloudOrg;    ///< pointer to input point cloud
  TComPointCloud* m_pointCloudRecon;  ///< pointer to output point cloud
  HighLevelSyntax* m_hls;             ///< pointer to high-level syntax parameters
  TEncBacTop* m_encBac;               ///< pointer to bac
  TEncBacTop* m_encBacDual;           ///< pointer to bac
  size_t m_colorBits;                 ///< color bits
  size_t m_reflBits;                  ///< reflectance bits
  double m_colorTime;                 ///< color encoding time
  double m_reflTime;                  ///< reflectance encoding time
  int frame_Id;
  int frame_Count;
  int multil_ID;

public:
  TEncAttribute() = default;
  ~TEncAttribute() = default;

  size_t& getColorBits() {
    return m_colorBits;
  }
  size_t& getReflectanceBits() {
    return m_reflBits;
  }

  double& getColorTime() {
    return m_colorTime;
  }
  double& getReflectanceTime() {
    return m_reflTime;
  }

  Void init(TComPointCloud* pointCloudOrg, TComPointCloud* pointCloudRecon, HighLevelSyntax* hls,
            TEncBacTop* encBac, const int& frame_Id, const int& frame_count, const int& multil_Id);
  Void initDual(TComPointCloud* pointCloudOrg, TComPointCloud* pointCloudRecon,
                HighLevelSyntax* hls, TEncBacTop* encBac, TEncBacTop* encBacDual,
                const int& frame_Id, const int& frame_count, const int& multil_Id);

  Void dualEncodeAttribute();

  Void predictEncodeColor();
  Void transformEncodeColor();
  Void transformEncodeColorFromReflectance();
  Void predictAndTransformEncodeColor();

  Void predictEncodeReflectance();
  Void transformEncodeReflectance();
  Void transformEncodeReflectanceFromColor();
  Void predictAndTransformEncodeReflectance();

  Void predictEncodeMultiReflectance();
  Void multiReflectancePredictingResidual();
  Void multiReflectancePredictCode(PC_REFL* predictor, PC_REFL* currValue, int& run_length,
                                   int multil_ID_Group_Num);

  Void reflectancePredictingResidual();
  Void colorPredictingResidual();
  Void attributePredictingResidual();

  Void colorPredictAndTransformMemControl();
  Void reflectancePredictAndTransformMemControl();
  Void attributePredictAndTransformMemControl();

  Void reflectanceWaveletTransform();
  Void colorWaveletTransform();
  Void reflectanceWaveletTransformFromColor();
  Void colorWaveletTransformFromReflectance();

  void colorPredictCode(const PC_COL& predictor, PC_COL& currValue, const quantizedQP& colorQp,
                        int& run_length, const bool isDuplicatePoint = false);

  void reflectancePredictCode(const int64_t& predictor, PC_REFL& currValue, int& run_length,
                              const bool isDuplicatePoint = false);

  void colorResidualCorrelationCode(const int64_t signResidualQuantvalue[3],
                                    const bool isDuplicatePoint, const UInt& golombNum,
                                    const V3<bool>& codeSign);

  void determineNeedCodeSign(V3<bool>& codeSign, const bool& os);

  void colorTransformCode(std::vector<pointCodeWithIndex>& pointCloudCode,
                          std::vector<int>& transformPointIdx, int& dcIndex, int& acIndex,
                          int64_t transformBuf[][8], int64_t transformPredBuf[][8],
                          const quantizedQP& colorQp, int* Coefficients,
                          std::vector<neighborSet>& neighborSet, colorQuantShift& colorQuantShift,
                          const bool isLengthControl = false, bool colorQPAdjustFlag = false,
                          bool deadZoneChromaFlag = false);

  void reflectanceCode(const int64_t& predictor, PC_REFL& currValue, int64_t& value,
                       const bool isDuplicatePoint);

  void reflectanceTransformCode(std::vector<pointCodeWithIndex>& pointCloudCode,
                                std::vector<int>& transformPointIdx, int& dcIndex, int& acIndex,
                                int64_t transformBuf[1][8], int64_t transformPredBuf[1][8],
                                int* Coefficients, std::vector<neighborSet>& neighborSet,
                                PC_REFL& lastref);

  void runlengthEncodeMemControl(int* Coefficients, int pointCount, int& run_length,
                                 int lengthControl, const bool isColor = false);

  void setCoeffIndex(int& groupCount, const vector<int>& numofGroupCount, const vector<int>& length,
                     int& dcIndex, int& acIndex);
};
