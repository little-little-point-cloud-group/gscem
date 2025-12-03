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

#include "TDecBacCore.h"
#include "common/CommonDef.h"
#include "common/ContextModel.h"
#include "common/HighLevelSyntax.h"
#include "common/TComBufferChunk.h"
#include "common/TComOccupancyMap.h"
#include "common/contributors.h"

#define CHECK_ALL_CTX 0

///< \in TLibDecoder \{

/**
 * Class TDecBacTop
 * entropy decoder
 */

class TDecBacTop {
private:
  COM_BS m_bitStream;
  TDecBacCore* m_bac;
  aec_t aec;
  aec_t* p_aec;
  geom_ctx_set_t m_ctxBackup;  // use to store the context for later restoration

  // adaptive expGolomb parameter
  vector<int> golombK = {0, 0, 0};  //Y U V 
  int64_t golombkForValUpper = 0;
  int64_t golombkForValLower = 0;

  int Group_size_shift = 1;
  int Group_size = 1;
  uint8_t memoryChannel[1024];
  uint8_t temp[1024];
  vector<vector<int64_t>> ExpGolombInputGroup = {{}, {}, {}};  //Y U V 
  vector<int64_t> inputGroupSum = {0, 0, 0};

public:
  ///< decoding syntax
  Void parseSPS(SequenceParameterSet& sps);
  Void parseGPS(GeometryParameterSet& gps);
  Void parseGBH(SequenceParameterSet& sps,GeometryParameterSet& gps, GeometryBrickHeader& gbh);
  Void parseSliceGeomEndCode();
  Void parseSliceAttrEndCode();
  Void parseSPSEndCode();
  Void parseAPS(AttributeParameterSet& aps, SequenceParameterSet& sps);
  Void parseFrameHeader(FrameHeader& frameHead);
  Void parseABH(AttributeBrickHeader& abh, const AttributeParameterSet& aps,
                const SequenceParameterSet& sps);
  UInt decodeOccupancyCode(const TComOctreePartitionParams& params, TComGeomContext& geomCtx,
                            bool& preNodePlanarEligible, const UInt contextMode);
  UInt decodeOccUsingMemoryChannel(TComOctreePartitionParams& params,
                                               TComGeomContext& geomCtx,
                                               bool& preNodePlanarEligible);
  Bool decodeSinglePointFlag();
  V3<UInt> decodeSinglePointIndex(V3<UInt> nodeSizeLog2);
  UInt decodeDuplicateNumber();

  // <predtree/tsp related syntax
  UInt decodeGeomTreeType();
  bool decodeKOctreeDepthflag();
  V3<int32_t> decodePredTreeResidual(const V3<int32_t>& cur, const V3<int32_t>& pred,
                                     const V3<int32_t>& m);
  UInt decodePredTreeNumPtsInLcu();
  UInt parseRunlengthGroup();
  Bool decodeTerminationFlag();
  Int parseRunlength();
  Int parseExpGolombRunlength(int k, context_t* p_ctxPrefix, context_t* p_ctxSufffix);
  Int parseExpGolomb(int k, context_t* p_ctxPrefix, context_t* p_ctxSufffix);
  Int parseExpGolombN(int k, context_t* p_ctxPrefix, context_t* p_ctxSufffix);
  Int parseExpGolombAdp(const int k, const int colorType, const int ctx_id);
  Int parseExpGolombRefl(int k, context_t* p_ctxPrefix, context_t* p_ctxSufffix);
  UInt64 parseExpGolombReflN(int k, context_t* p_ctxPrefix, context_t* p_ctxSufffix);

  Void setGolombGroupSize(const UInt& groupSizeShift);
  Void setColorGolombKandBound(const UInt& GolombNum);

  int64_t parseAttr(const bool& isColor, const int& colorType, const int ctx_id = 0,
                const bool isDuplicatePoint = false, const bool residualminusone = false,
                const UInt& golombNum = 1, const int b0 = 0);
  void parseSign(int64_t& delta);
  Int parseAttrHaar(const bool& isColor,const int& colorType, const bool reslayer=false, const int ctx_id = 0, const bool isDuplicatePoint = false,
                    const bool residualminusone = false, const UInt& golombNum = 1);  
  Int parseRefl(const int ctx_id = 0, const bool isDuplicatePoint = false,
                const bool residualminusone = false, const UInt& golombNum = 3);
  Int parseColor(const int ctx_id = 0, const bool isDuplicatePoint = false,
                 const bool residualminusone = false);
  Int parseAttrequaltwo0();
  Int parseAttrequalone0();
  Int parseAttrequal0();
  TDecBacTop();
  ~TDecBacTop();
  Void reset();
  Void LcuReset();
  Void initBac();
  Void setBitstreamBuffer(TComBufferChunk& buffer, const bool& initDulatAttribute = false);

  void saveContext() {
    m_ctxBackup = p_aec->geometry_syn_ctx;
    for (int i = 0; i < 1024; i++)
      temp[i] = memoryChannel[i];
  }

  void restoreContext() {
    p_aec->geometry_syn_ctx = m_ctxBackup;
    for (int i = 0; i < 1024; i++)
      memoryChannel[i] = temp[i];
  }

};  ///< END CLASS TDecBacTop

///< \{
