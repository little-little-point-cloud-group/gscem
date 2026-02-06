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

#include "TEncBacCore.h"
#include "common/CommonDef.h"
#include "common/ContextModel.h"
#include "common/HighLevelSyntax.h"
#include "common/TComBufferChunk.h"
#include "common/TComOccupancyMap.h"
#include "common/contributors.h"

///< \in TLibEncoder \{

/**
 * Class TEncBacTop
 * entropy encoder
 */

class TEncBacTop {
private:
  COM_BS m_bitStream;
  TEncBacCore* m_bac;
  aec_t aec;
  aec_t* p_aec;
  geom_ctx_set_t m_ctxBackup;  // use to store the context for later restoration
  int buffersize = (1 << 28);
  uint8_t memoryChannel[1024];
  uint8_t temp[1024];

  // adaptive expGolomb parameter
  vector<int> golombK = {0, 0, 0};  //Y U V
  int64_t golombkForValUpper = 0;
  int64_t golombkForValLower = 0;
  int Group_size_log2 = 1;
  int Group_size = 1;
  vector<vector<int64_t>> ExpGolombInputGroup = {{}, {}, {}};  //Y U V
  vector<int64_t> inputGroupSum = {0, 0, 0};

public:
  ///< encoding syntax
  Void computeBufferSize(int pointnum, int attrinum = 1) {
    buffersize = pointnum * 8 * attrinum;
  }
  Void codeSPS(const SequenceParameterSet& sps);
  Void codeGPS(const GeometryParameterSet& gps);
  Void codeGBH(const GeometryParameterSet& gps, const GeometryBrickHeader& gbh);
  Void codeAPS(AttributeParameterSet& aps, const SequenceParameterSet& sps);
  Void codeFrameHeader(const FrameHeader& frameHeader);
  Void codeABH(const AttributeBrickHeader& abh, const AttributeParameterSet& aps,
               const SequenceParameterSet& sps);
  Void codeUserData();
  Void encodeTerminationFlag(const Bool terminateFlag = true);

  ///< octree related syntax
  Void encodeOccUsingMemoryChannel(const UInt& occupancyCode, TComOctreePartitionParams& params,
                                   TComGeomContext& geomCtx, bool& preNodePlanarEligible);

  Void encodeOccupancyCode(const UInt& occupancyCode, const TComOctreePartitionParams& params,
                           TComGeomContext& geomCtx, bool& preNodePlanarEligible,
                           const UInt contextMode);
  Void encodeSinglePointFlag(Bool singleModeFlag);
  Void encodeSinglePointIndex(const V3<UInt>& pos, V3<UInt> nodeSizeLog2);
  Void encodeDuplicateNumber(const UInt& dupNum);

  ///< predtree/tsp related syntax
  Void encodeGeomTreeType(UInt geomTreeType);
  Void EligibleKOctreeDepthflag(bool isEligible);
  Void encodePredTreeResidual(const V3<int32_t>& residual, const V3<int32_t>& cur,
                              const V3<int32_t>& pred, const V3<int32_t>& m);
  Void encodePredTreeNumPtsInLcu(UInt numPtsInLcu);
  Int getNumBits(UInt num);

  ///< attribute related syntax
  Void encodeRunlength(int32_t& length);
  Void encodeExpGolombRunlength(unsigned int symbol, int k, context_t* p_ctxPrefix,
                                context_t* p_ctxSufffix);
  Void encodeExpGolombN(unsigned int symbol, int k, context_t* p_ctxPrefix,
                        context_t* p_ctxSufffix);
  Void encodeExpGolombAdp(const int val, const int k, const int colorType, const int ctx_id);
  Void setColorGolombKandBound(const UInt& groupSizeLog2, const UInt& GolombNum);
  Void encodeExpGolombReflN(UInt64 symbol, int k, context_t* p_ctxPrefix, context_t* p_ctxSufffix);
  Void codeAttributerResidual(const int64_t& delta, const bool& isColor, const int& colorType,
                              const int ctx_id = 0, const bool isDuplicatePoint = false,
                              const bool residualminusone = false, const UInt8& golombNum = 1,
                              const int b0 = 0);
  Void codeSign(const int64_t delta);
  Void codeAttributerResidualHaar(const int64_t& delta, const bool& isColor, const int& colorType,
                                  const bool reslayer = false, const int ctx_id = 0,
                                  const bool isDuplicatePoint = false,
                                  const bool residualminusone = false, const UInt8& golombNum = 1);
  Void codeAttributeResidualDual(const int64_t& delta, const bool& isColor, const int ctx_id = 0,
                                 const bool isDuplicatePoint = false,
                                 const bool residualminusone = false, const UInt8& golombNum = 3);
  Void codeAttributerResidualequalone(const int64_t& delta);
  Void codeAttributerResidualequaltwo(const int64_t& delta);

  Void encodeFinish();

  TEncBacTop();
  ~TEncBacTop();
  Void reset();
  Void initBac();
  Void LcuReset();
  Void setBitstreamBuffer(TComBufferChunk& buffer, const bool& initDulatAttribute = false);
  UInt64 getBitStreamLength();  ///< get number of byte written

  Void saveContext() {
    m_ctxBackup = *(p_aec->p_geometry_ctx_set);
    for (int i = 0; i < 1024; i++)
      temp[i] = memoryChannel[i];
  }

  Void restoreContext() {
    *(p_aec->p_geometry_ctx_set) = m_ctxBackup;
    for (int i = 0; i < 1024; i++)
      memoryChannel[i] = temp[i];
  }

};  ///< END CLASS TEncBacTop

///< \}
