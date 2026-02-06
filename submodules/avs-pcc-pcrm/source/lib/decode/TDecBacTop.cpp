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
#include "TDecBacCore.h"
#include "common/TComRom.h"
#include "common/contributors.h"

///< \in TLibDecoder \{

/**
 * Class TDecBacTop
 * entropy decoder
 */

//////////////////////////////////////////////////////////////////////////
// Public class functions
//////////////////////////////////////////////////////////////////////////

Void TDecBacTop::parseSPS(SequenceParameterSet& sps) {
  sps.profileId = m_bac->com_bsr_read(&m_bitStream, 4);
  sps.levelId = m_bac->com_bsr_read(&m_bitStream, 8);
  sps.frameRateCode = m_bac->com_bsr_read(&m_bitStream, 4);
  sps.geomRemoveDuplicateFlag = m_bac->com_bsr_read1(&m_bitStream);
  sps.attrPresentFlag = m_bac->com_bsr_read1(&m_bitStream);
  if (sps.attrPresentFlag) {
    sps.maxNumAttributesMinus1 = m_bac->com_bsr_read(&m_bitStream, 7);
    sps.multiAttributesSetFlag = m_bac->com_bsr_read1(&m_bitStream);
  }
  m_bac->com_bsr_read_byte_align(&m_bitStream);
}

Void TDecBacTop::parseGPS(GeometryParameterSet& gps) {
  gps.geomQuantStepSignificand = (UInt32)m_bac->com_bsr_read(&m_bitStream, 21);
  m_bac->com_bsr_read1(&m_bitStream);
  gps.geomQuantStepExponent = (UInt32)m_bac->com_bsr_read(&m_bitStream, 5);
  gps.geomMaxTreeSizeLog2Minus8 = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
  gps.implicitGeomPartitionFlag = !!m_bac->com_bsr_read1(&m_bitStream);
  gps.singleModeFlag = !!m_bac->com_bsr_read1(&m_bitStream);
  gps.occupancySearchRangeLog2 = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
  gps.saveStateFlag = !!m_bac->com_bsr_read1(&m_bitStream);
  if (!gps.saveStateFlag)
    gps.lcuDependencyFlag = !!m_bac->com_bsr_read1(&m_bitStream);
  m_bac->com_bsr_read_byte_align(&m_bitStream);
}

Void TDecBacTop::parseAPS(AttributeParameterSet& aps, SequenceParameterSet& sps) {
  for (int attrIdx = 0; attrIdx < (sps.maxNumAttributesMinus1 + 1); attrIdx++) {
    aps.attributeDataPresentFlag[attrIdx] = !!m_bac->com_bsr_read1(&m_bitStream);
    if (aps.attributeDataPresentFlag[attrIdx]) {
      aps.attributeDataNumSetMinus1[attrIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
      if ((attrIdx == 1) && (aps.attributeDataNumSetMinus1[attrIdx] > 0)) {
        for (int i = 0; i < aps.attributeDataNumSetMinus1[attrIdx] + 1; ++i) {
          aps.multiAttrGroupID[i] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
        }
      }
      if (sps.multiAttributesSetFlag)
        aps.multiDataSetFlag[attrIdx] = !!m_bac->com_bsr_read1(&m_bitStream);
      if (aps.multiDataSetFlag[attrIdx])
        aps.attributeInfoNumSetMinus1[attrIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
      for (int multiIdx = 0; multiIdx < aps.attributeInfoNumSetMinus1[attrIdx] + 1; ++multiIdx) {
        aps.outputMultiBitDepthMinus1[attrIdx][multiIdx] =
          (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
        aps.attrMultiQuantParam[attrIdx][multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
        if (attrIdx == 0) {
          aps.orderMultiSwitch[multiIdx] = !!m_bac->com_bsr_read1(&m_bitStream);
          aps.colorMultiReordermode[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
          aps.colorMultiGolombNum[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
          aps.golombMultiGroupSizeLog2[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
        }
        if (attrIdx == 1) {
          aps.axisMultiBiasMinus1[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
          aps.reflMultiReordermode[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
          aps.reflMultiGolombNum[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
          aps.predMultiFixedPointFracBit[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
        }

        aps.colorMultiOutputDepth[multiIdx] = aps.outputMultiBitDepthMinus1[0][multiIdx] + 1;
        aps.reflMultiOutputDepth[multiIdx] = aps.outputMultiBitDepthMinus1[1][multiIdx] + 1;
        aps.colorMultiQuantParam[multiIdx] = aps.attrMultiQuantParam[0][multiIdx];
        aps.reflMultiQuantParam[multiIdx] = aps.attrMultiQuantParam[1][multiIdx];

        aps.transformMulti[attrIdx][multiIdx] = (UInt)m_bac->com_bsr_read(&m_bitStream, 2);
        if ((aps.transformMulti[attrIdx][multiIdx] == 0) ||
            (aps.transformMulti[attrIdx][multiIdx] == 2)) {
          aps.maxMultiNumOfNeighboursLog2Minus7[attrIdx][multiIdx] =
            m_bac->com_bsr_read(&m_bitStream, 2);
          if (attrIdx == 0) {
            aps.crossMultiComponentPred[multiIdx] = !!m_bac->com_bsr_read1(&m_bitStream);
            aps.chromaMultiQpOffsetCb[multiIdx] = m_bac->com_bsr_read_se(&m_bitStream);
            aps.chromaMultiQpOffsetCr[multiIdx] = m_bac->com_bsr_read_se(&m_bitStream);
          }
          if (attrIdx == 1) {
            aps.nearestMultiPredParam1[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
            aps.nearestMultiPredParam2[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
            aps.predDistWeightMultiGroupSizeLog2[multiIdx] =
              (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
          }
        }
        if (aps.transformMulti[attrIdx][multiIdx] == 1) {
          UInt transformSegmentSize_upper = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
          m_bac->com_bsr_read1(&m_bitStream);
          UInt transformSegmentSize_lower = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
          m_bac->com_bsr_read1(&m_bitStream);
          aps.transformMultiSegmentSize[attrIdx][multiIdx] =
            (transformSegmentSize_upper << 16) + transformSegmentSize_lower;
          aps.kMultiFracBits[attrIdx][multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
          aps.attrMultiTransformQpDelta[attrIdx][multiIdx] =
            (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
          aps.transMultiResLayer[attrIdx][multiIdx] = m_bac->com_bsr_read1(&m_bitStream);
        }

        if (aps.transformMulti[attrIdx][multiIdx] == 2) {
          aps.MultimaxNumofCoeffLog2Minus8[attrIdx][multiIdx] =
            (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
          if (aps.MultimaxNumofCoeffLog2Minus8[attrIdx][multiIdx]) {
            aps.maxMultiNumofCoeff[attrIdx][multiIdx] = 1
              << (aps.MultimaxNumofCoeffLog2Minus8[attrIdx][multiIdx] + 8);
          }
          aps.QpMultiOffsetDC[attrIdx][multiIdx] = m_bac->com_bsr_read_se(&m_bitStream);
          aps.QpMultiOffsetAC[attrIdx][multiIdx] = m_bac->com_bsr_read_se(&m_bitStream);
          if (attrIdx == 0) {
            aps.colorMaxMultiTransNum[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
            aps.chromaMultiQpOffsetDC[multiIdx] = m_bac->com_bsr_read_se(&m_bitStream);
            aps.chromaMultiQpOffsetAC[multiIdx] = m_bac->com_bsr_read_se(&m_bitStream);
            aps.colorMultiQPAdjustFlag[multiIdx] = m_bac->com_bsr_read1(&m_bitStream);
          }
          if (attrIdx == 1) {
            aps.reflMaxMultiTransNum[multiIdx] = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
            aps.reflMultiGroupPredict[multiIdx] = m_bac->com_bsr_read1(&m_bitStream);
          }
        }

        aps.coeffMultiLengthControlLog2Minus8[attrIdx][multiIdx] =
          (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
        if (aps.coeffMultiLengthControlLog2Minus8[attrIdx][multiIdx]) {
          aps.coeffMultiLengthControl[attrIdx][multiIdx] = 1
            << (aps.coeffMultiLengthControlLog2Minus8[attrIdx][multiIdx] + 8);
        }
      }
    }
  }
  if ((aps.attributeDataNumSetMinus1[0] == 0) && (aps.attributeDataNumSetMinus1[1] == 0)) {
    aps.crossAttrTypePred = !!m_bac->com_bsr_read1(&m_bitStream);
    if (aps.crossAttrTypePred) {
      aps.attrEncodeOrder = (UInt)m_bac->com_bsr_read1(&m_bitStream);
      aps.crossAttrTypePredParam1 = (UInt)m_bac->com_bsr_read(&m_bitStream, 15);
      m_bac->com_bsr_read1(&m_bitStream);
      aps.crossAttrTypePredParam2 = (UInt)m_bac->com_bsr_read(&m_bitStream, 21);
      m_bac->com_bsr_read1(&m_bitStream);
    }
  }
  m_bac->com_bsr_read_byte_align(&m_bitStream);
}

Void TDecBacTop::parseFrameHeader(FrameHeader& frameheader) {
  frameheader.frameIndex = m_bac->com_bsr_read_ue(&m_bitStream);
  m_bac->com_bsr_read1(&m_bitStream);
  frameheader.frameNumSliceMinus1 = m_bac->com_bsr_read_ue(&m_bitStream);

  frameheader.lcuNodeSizeLog2 = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
  if (frameheader.lcuNodeSizeLog2 > 0) {
    frameheader.lcuNodeSizeLog2++;
  }

  UInt np_upper = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  UInt np_lower = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  frameheader.geomNumPoints = (np_upper << 16) + np_lower;

  Int bb_x_upper = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  Int bb_x_lower = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  frameheader.geomBoundingBoxOrigin[0] = (bb_x_upper << 16) + bb_x_lower;

  Int bb_y_upper = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  Int bb_y_lower = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  frameheader.geomBoundingBoxOrigin[1] = (bb_y_upper << 16) + bb_y_lower;

  Int bb_z_upper = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  Int bb_z_lower = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  frameheader.geomBoundingBoxOrigin[2] = (bb_z_upper << 16) + bb_z_lower;

  UInt bbs_w_upper = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  UInt bbs_w_lower = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  frameheader.geomBoundingBoxSize[0] = (bbs_w_upper << 16) + bbs_w_lower;

  UInt bbs_h_upper = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  UInt bbs_h_lower = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  frameheader.geomBoundingBoxSize[1] = (bbs_h_upper << 16) + bbs_h_lower;

  UInt bbs_d_upper = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  UInt bbs_d_lower = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  frameheader.geomBoundingBoxSize[2] = (bbs_d_upper << 16) + bbs_d_lower;

  m_bac->com_bsr_read_byte_align(&m_bitStream);
}

Void TDecBacTop::parseGBH(SequenceParameterSet& sps, GeometryParameterSet& gps,
                          GeometryBrickHeader& gbh) {
  gbh.sliceID = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
  m_bac->com_bsr_read1(&m_bitStream);
  gbh.contextMode = !!m_bac->com_bsr_read1(&m_bitStream);
  if (gps.implicitGeomPartitionFlag) {
    gbh.imQtbtNumBeforeOt = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
    gbh.imQtbtMinSize = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
  }
  if (gps.singleModeFlag) {
    gbh.singleModeFlagInSlice = !!m_bac->com_bsr_read1(&m_bitStream);
  }
  gbh.planarModeEligibleForSlice = !!m_bac->com_bsr_read1(&m_bitStream);

  m_bac->com_bsr_read1(&m_bitStream);
  Int bb_x_upper = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  Int bb_x_lower = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  gbh.geomBoundingBoxOrigin[0] = (bb_x_upper << 16) + bb_x_lower;

  Int bb_y_upper = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  Int bb_y_lower = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  gbh.geomBoundingBoxOrigin[1] = (bb_y_upper << 16) + bb_y_lower;

  Int bb_z_upper = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  Int bb_z_lower = (Int)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  gbh.geomBoundingBoxOrigin[2] = (bb_z_upper << 16) + bb_z_lower;

  gbh.nodeSizeLog2[0] = (UInt)m_bac->com_bsr_read(&m_bitStream, 6);
  gbh.nodeSizeLog2[1] = (UInt)m_bac->com_bsr_read(&m_bitStream, 6);
  gbh.nodeSizeLog2[2] = (UInt)m_bac->com_bsr_read(&m_bitStream, 6);
  m_bac->com_bsr_read1(&m_bitStream);

  UInt np_upper = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  UInt np_lower = (UInt)m_bac->com_bsr_read(&m_bitStream, 16);
  m_bac->com_bsr_read1(&m_bitStream);
  gbh.geomNumPoints = (np_upper << 16) + np_lower;

  m_bac->com_bsr_read_byte_align(&m_bitStream);
}

Void TDecBacTop::parseABH(AttributeBrickHeader& abh, AttributeParameterSet& aps, UInt attrIdx) {
  abh.sliceID = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
  m_bac->com_bsr_read1(&m_bitStream);
  abh.attributeID = (UInt)m_bac->com_bsr_read_ue(&m_bitStream);
  getSingleAttrAPs(aps, attrIdx, abh.attributeID);

  abh.QpOffset = (UInt)m_bac->com_bsr_read_se(&m_bitStream);
  abh.colorInitPredTransRatio = m_bac->com_bsr_read_se(&m_bitStream);
  abh.reflInitPredTransRatio = m_bac->com_bsr_read_se(&m_bitStream);
  if (aps.colorQPAdjustFlag) {
    abh.colorQPAdjustScalar = m_bac->com_bsr_read_ue(&m_bitStream);
  }
  m_bac->com_bsr_read_byte_align(&m_bitStream);
}

Void TDecBacTop::getSingleAttrAPs(AttributeParameterSet& aps, const int& attrIdx,
                                  const int& multiIdx) {
  if (attrIdx == 0) {
    aps.orderSwitch = aps.orderMultiSwitch[multiIdx];
    aps.colorReorderMode = aps.colorMultiReordermode[multiIdx];
    aps.colorGolombNum = aps.colorMultiGolombNum[multiIdx];
    aps.golombGroupSizeLog2 = aps.golombMultiGroupSizeLog2[multiIdx];
  }
  if (attrIdx == 1) {
    aps.axisBias = aps.axisMultiBiasMinus1[multiIdx] + 1;
    aps.reflReorderMode = aps.reflMultiReordermode[multiIdx];
    aps.reflGolombNum = aps.reflMultiGolombNum[multiIdx];
    aps.predFixedPointFracBit = aps.predMultiFixedPointFracBit[multiIdx];
  }

  aps.colorOutputDepth = aps.colorMultiOutputDepth[multiIdx];
  aps.reflOutputDepth = aps.reflMultiOutputDepth[multiIdx];
  aps.colorQuantParam = aps.colorMultiQuantParam[multiIdx];
  aps.reflQuantParam = aps.reflMultiQuantParam[multiIdx];

  aps.transform = aps.transformMulti[attrIdx][multiIdx];
  if ((aps.transform == 0) || (aps.transform == 2)) {
    aps.maxNumOfNeighbours = 1 << (aps.maxMultiNumOfNeighboursLog2Minus7[attrIdx][multiIdx] + 7);
    if (attrIdx == 0) {
      aps.crossComponentPred = aps.crossMultiComponentPred[multiIdx];
      aps.chromaQpOffsetCb = aps.chromaMultiQpOffsetCb[multiIdx];
      aps.chromaQpOffsetCr = aps.chromaMultiQpOffsetCr[multiIdx];
    }
    if (attrIdx == 1) {
      aps.nearestPredParam1 = aps.nearestMultiPredParam1[multiIdx];
      aps.nearestPredParam2 = aps.nearestMultiPredParam2[multiIdx];
      aps.predDistWeightGroupSizeLog2 = aps.predDistWeightMultiGroupSizeLog2[multiIdx];
    }
  }
  if (aps.transform == 1) {
    aps.kFracBits = aps.kMultiFracBits[attrIdx][multiIdx];
    aps.attrTransQpDelta = aps.attrMultiTransformQpDelta[attrIdx][multiIdx];
    aps.transformSegmentSize = aps.transformMultiSegmentSize[attrIdx][multiIdx];
    aps.transResLayer = aps.transMultiResLayer[attrIdx][multiIdx];
    FXPoint::set_kFracBits(aps.kFracBits);
  }
  if (aps.transform == 2) {
    aps.maxNumofCoeffLog2Minus8 = aps.MultimaxNumofCoeffLog2Minus8[attrIdx][multiIdx];
    if (aps.maxNumofCoeffLog2Minus8) {
      aps.maxNumofCoeff = 1 << (aps.maxNumofCoeffLog2Minus8 + 8);
    }
    aps.QpOffsetDC = aps.QpMultiOffsetDC[attrIdx][multiIdx];
    aps.QpOffsetAC = aps.QpMultiOffsetAC[attrIdx][multiIdx];
    if (attrIdx == 0) {
      aps.colorMaxTransNum = aps.colorMaxMultiTransNum[multiIdx];
      aps.chromaQpOffsetDC = aps.chromaMultiQpOffsetDC[multiIdx];
      aps.chromaQpOffsetAC = aps.chromaMultiQpOffsetAC[multiIdx];
      aps.colorQPAdjustFlag = aps.colorMultiQPAdjustFlag[multiIdx];
    }
    if (attrIdx == 1) {
      aps.reflMaxTransNum = aps.reflMaxMultiTransNum[multiIdx];
      aps.reflGroupPredict = aps.reflMultiGroupPredict[multiIdx];
    }
  }

  aps.coeffLengthControlLog2Minus8 = aps.coeffMultiLengthControlLog2Minus8[attrIdx][multiIdx];
  if (aps.coeffLengthControlLog2Minus8) {
    aps.coeffLengthControl = 1 << (aps.coeffLengthControlLog2Minus8 + 8);
  }
}

Int TDecBacTop::parseRunlength() {
  Int val = 0;
  if (m_bac->biari_decode_symbol(p_aec, &p_aec->attribute_syn_ctx.ctx_attr_length_eq0))
    return 0;
  else {
    val = parseExpGolombRunlength(2, p_aec->attribute_syn_ctx.ctx_length_prefix,
                                  p_aec->attribute_syn_ctx.ctx_length_suffix);
    return (1 + val);
  }
  return val + 1;
}

int TDecBacTop::parseExpGolombRunlength(int k, context_t* p_ctxPrefix, context_t* p_ctxSufffix) {
  const int k0 = k;
  unsigned int l;
  int symbol = 0;
  int binary_symbol = 0;
  do {
    l = m_bac->biari_decode_symbol(p_aec, &p_ctxPrefix[std::min(k - k0, 2)]);
    if (l == 1) {
      symbol += (1 << k);
      k++;
    }
  } while (l != 0);
  while (k--)  //next binary part
    if (m_bac->biari_decode_symbol(p_aec, &p_ctxSufffix[std::min(k, 1)]) == 1) {
      binary_symbol |= (1 << k);
    }
  return static_cast<unsigned int>(symbol + binary_symbol);
}

int TDecBacTop::parseExpGolombN(int k, context_t* p_ctxPrefix, context_t* p_ctxSufffix) {
  int count = 4;
  unsigned int l;
  int symbol = 0;
  int binary_symbol = 0;
  do {
    if (count) {
      l = m_bac->biari_decode_symbol(p_aec, p_ctxPrefix);
      count--;
    } else {
      l = m_bac->biari_decode_symbol_eq_prob(p_aec);
    }
    if (l == 1) {
      symbol += (1 << k);
      k++;
    }
  } while (l != 0);
  while (k--) {  //next binary part
    if (count) {
      if (m_bac->biari_decode_symbol(p_aec, p_ctxSufffix) == 1) {
        binary_symbol |= (1 << k);
      }
      count--;
    } else {
      if (m_bac->biari_decode_symbol_eq_prob(p_aec) == 1) {
        binary_symbol |= (1 << k);
      }
    }
  }
  return static_cast<unsigned int>(symbol + binary_symbol);
}

Void TDecBacTop::setColorGolombKandBound(const UInt& groupSizeLog2, const UInt& GolombNum) {
  Group_size = 1 << groupSizeLog2;
  Group_size_log2 = groupSizeLog2;
  golombK[0] = GolombNum;
  golombK[1] = GolombNum;
  golombK[2] = GolombNum;
  if (GolombNum > 1) {
    golombkForValUpper = (1 << (GolombNum - 1)) + (1 << (GolombNum - 2));
    golombkForValLower = (1 << (GolombNum - 1)) - (1 << (GolombNum - 2));
  } else {
    golombkForValUpper = 1;
    golombkForValLower = 1;
  }
  ExpGolombInputGroup = {{}, {}, {}};
  inputGroupSum = {0, 0, 0};
}

int TDecBacTop::parseExpGolombAdp(const int k, const int colorType, const int ctx_id) {
  golombK[colorType] = golombK[colorType] > 0 ? golombK[colorType] : 1;
  int golombDeVal =
    parseExpGolombN(golombK[colorType], &p_aec->attribute_syn_ctx.ctx_attr_residual_prefix[ctx_id],
                    &p_aec->attribute_syn_ctx.ctx_attr_residual_suffix[ctx_id]);
  if (ExpGolombInputGroup[colorType].size() < Group_size) {
    inputGroupSum[colorType] += golombDeVal;
    ExpGolombInputGroup[colorType].push_back(golombDeVal);
  } else {
    auto valFirst = ExpGolombInputGroup[colorType].begin();
    inputGroupSum[colorType] -= *valFirst;
    ExpGolombInputGroup[colorType].erase(valFirst);
    inputGroupSum[colorType] += golombDeVal;
    ExpGolombInputGroup[colorType].push_back(golombDeVal);
    int64_t inputGroupAvg = inputGroupSum[colorType] / Group_size;
    golombK[colorType] = k;
    if (inputGroupAvg < golombkForValLower)
      golombK[colorType]--;
    else if (inputGroupAvg > golombkForValUpper)
      golombK[colorType]++;
  }
  return golombDeVal;
}

UInt64 TDecBacTop::parseExpGolombReflN(int k, context_t* p_ctxPrefix, context_t* p_ctxSufffix) {
  int count = 4;
  int k0 = k;
  int kmax = k;
  UInt64 l;
  UInt64 symbol = 0;
  UInt64 binary_symbol = 0;
  unsigned int p;
  do {
    if (count) {
      if (k == k0) {
        l = m_bac->biari_decode_symbol(p_aec, &p_ctxPrefix[0]);
      } else if (k == k0 + 1) {
        l = m_bac->biari_decode_symbol(p_aec, &p_ctxPrefix[1]);
      } else {
        l = m_bac->biari_decode_symbol(p_aec, &p_ctxPrefix[2]);
      }
      count--;
    } else {
      l = m_bac->biari_decode_symbol_eq_prob(p_aec);
    }
    if (l == 1) {
      symbol += (1LL << k);
      k++;
      kmax = k;
    }
  } while (l != 0);
  while (k--) {  //next binary part
    if (count) {
      if (k == kmax - 1) {
        p = m_bac->biari_decode_symbol(p_aec, &p_ctxSufffix[0]);
      } else if (k == kmax - 2) {
        p = m_bac->biari_decode_symbol(p_aec, &p_ctxSufffix[1]);
      } else {
        p = m_bac->biari_decode_symbol(p_aec, &p_ctxSufffix[2]);
      }
      count--;
    } else {
      p = m_bac->biari_decode_symbol_eq_prob(p_aec);
    }
    if (p == 1) {
      binary_symbol |= (1LL << k);
    }
  }
  return static_cast<UInt64>(symbol + binary_symbol);
}

Int TDecBacTop::parseRefl(const int ctx_id, const bool isDuplicatePoint,
                          const bool residualminusone_flag, const UInt& golombNum) {
  Int val = 0;
  bool is_zero = false;
  Int sign_bit = 1;
  if (!isDuplicatePoint) {
    sign_bit = m_bac->biari_decode_symbol_eq_prob(p_aec);
  }
  int parity = m_bac->biari_decode_symbol(p_aec, &p_aec->attribute_syn_ctx_dual.parity[ctx_id]);
  if (m_bac->biari_decode_symbol(p_aec,
                                 &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_flag1[ctx_id])) {
    if (m_bac->biari_decode_symbol(
          p_aec, &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_flag2[ctx_id])) {
      int golombDeVal =
        parseExpGolombReflN(golombNum, p_aec->attribute_syn_ctx_dual.ctx_attr_residual_prefix,
                            p_aec->attribute_syn_ctx_dual.ctx_attr_residual_suffix);
      val = 5 + parity + (golombDeVal << 1);
    } else {
      val = 3 + parity;
    }
  } else {
    val = 1 + parity;
  }
  val = (sign_bit == 1) ? val : -val;
  return val;
}

Int TDecBacTop::parseColor(const int ctx_id, const bool isDuplicatePoint,
                           const bool residualminusone_flag) {
  if (!residualminusone_flag) {
    Int val = 0;

    if (!m_bac->biari_decode_symbol(
          p_aec,
          &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_eq0[ctx_id + (3 * isDuplicatePoint)])) {
      Int sign_bit = m_bac->biari_decode_symbol_eq_prob(p_aec);
      if (!m_bac->biari_decode_symbol(
            p_aec, &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_flag1[ctx_id])) {
        if (!m_bac->biari_decode_symbol(
              p_aec, &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_flag2[ctx_id])) {
          val = parseExpGolombN(1, &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_prefix[ctx_id],
                                &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_suffix[ctx_id]) +
            3;
        } else {
          val = 2;
        }
      } else {
        val = 1;
      }
      val = (sign_bit == 1) ? val : -val;
    }
    return val;
  } else {
    Int val = 0;
    Int sign_bit = 1;
    if (!m_bac->biari_decode_symbol(
          p_aec,
          &p_aec->attribute_syn_ctx_dual
             .ctx_attr_residual_minusone_eq0[ctx_id + (3 * isDuplicatePoint)])) {
      sign_bit = m_bac->biari_decode_symbol_eq_prob(p_aec);
      if (!m_bac->biari_decode_symbol(
            p_aec, &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_minusone_flag1[ctx_id])) {
        if (!m_bac->biari_decode_symbol(
              p_aec, &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_minusone_flag2[ctx_id])) {
          val = parseExpGolombN(1, &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_prefix[ctx_id],
                                &p_aec->attribute_syn_ctx_dual.ctx_attr_residual_suffix[ctx_id]) +
            3;
        } else {
          val = 2;
        }
      } else {
        val = 1;
      }
    } else {
      sign_bit = m_bac->biari_decode_symbol_eq_prob(p_aec);
    }
    val = val + 1;
    val = (sign_bit == 1) ? val : -val;
    return val;
  }
}

int64_t TDecBacTop::parseAttr(const bool& isColor, const int& colorType, const int ctx_id,
                              const bool isDuplicatePoint, const bool residualminusone_flag,
                              const UInt& golombNum, const int b0) {
  int ExpGolombNumber = golombNum;
  if (!isColor) {
    int64_t val = 0;
    bool is_zero = false;
    Int sign_bit = 1;
    if (!isDuplicatePoint) {
      sign_bit = m_bac->biari_decode_symbol_eq_prob(p_aec);
    }
    int parity = m_bac->biari_decode_symbol(p_aec, &p_aec->attribute_syn_ctx.parity[ctx_id]);
    if (m_bac->biari_decode_symbol(p_aec,
                                   &p_aec->attribute_syn_ctx.ctx_attr_residual_flag1[ctx_id])) {
      if (m_bac->biari_decode_symbol(p_aec,
                                     &p_aec->attribute_syn_ctx.ctx_attr_residual_flag2[ctx_id])) {
        UInt64 golombDeVal =
          parseExpGolombReflN(ExpGolombNumber, p_aec->attribute_syn_ctx.ctx_attr_residual_prefix,
                              p_aec->attribute_syn_ctx.ctx_attr_residual_suffix);
        val = 5 + parity + (golombDeVal << 1);

      } else {
        val = 3 + parity;
      }
    } else {
      val = 1 + parity;
    }
    val = (sign_bit == 1) ? val : -val;
    return val;
  } else {
    if (!residualminusone_flag) {
      Int val = 0;
      bool tempflag = 0;
      if (b0 == 1) {
        if (isDuplicatePoint == 0) {
          tempflag =
            m_bac->biari_decode_symbol(p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_eq0[6]);

        } else {
          tempflag =
            m_bac->biari_decode_symbol(p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_eq0[7]);
        }

      } else {
        tempflag = m_bac->biari_decode_symbol(
          p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_eq0[ctx_id + (3 * isDuplicatePoint)]);
      }

      if (!tempflag)

      {
        if (m_bac->biari_decode_symbol(
              p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_flag1[ctx_id + b0])) {
          int parity =
            m_bac->biari_decode_symbol(p_aec, &p_aec->attribute_syn_ctx.parity[ctx_id + b0]);
          if (m_bac->biari_decode_symbol(
                p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_flag2[ctx_id + b0])) {
            golombK[colorType] = golombK[colorType] > 0 ? golombK[colorType] : 1;
            int golombDeVal = parseExpGolombAdp(ExpGolombNumber, colorType, ctx_id);
            val = 4 + parity + (golombDeVal << 1);

          } else {
            val = 2 + parity;
          }
        } else {
          val = 1;
        }
      }
      return val;
    } else {
      Int val = 0;
      if (!m_bac->biari_decode_symbol(
            p_aec,
            &p_aec->attribute_syn_ctx
               .ctx_attr_residual_minusone_eq0[ctx_id + (3 * isDuplicatePoint)])) {
        if (m_bac->biari_decode_symbol(
              p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_minusone_flag1[ctx_id])) {
          int parity = m_bac->biari_decode_symbol(p_aec, &p_aec->attribute_syn_ctx.parity[ctx_id]);
          if (m_bac->biari_decode_symbol(
                p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_minusone_flag2[ctx_id])) {
            int golombDeVal = parseExpGolombAdp(ExpGolombNumber, colorType, ctx_id);
            val = 4 + parity + (golombDeVal << 1);
          } else {
            val = 2 + parity;
          }
        } else {
          val = 1;
        }
      }
      val = val + 1;
      return val;
    }
  }
}

Void TDecBacTop::parseSign(int64_t& delta) {
  if (delta != 0) {
    int sign_bit = m_bac->biari_decode_symbol_eq_prob(p_aec);
    delta = (sign_bit == 1) ? delta : -delta;
  }
}

Int TDecBacTop::parseAttrHaar(const bool& isColor, const int& colorType, const bool reslayer,
                              const int ctx_id, const bool isDuplicatePoint,
                              const bool residualminusone_flag, const UInt& golombNum) {
  if (!isColor) {
    Int val = 0;
    bool is_zero = false;
    Int sign_bit = 1;
    if (!isDuplicatePoint) {
      sign_bit = m_bac->biari_decode_symbol_eq_prob(p_aec);
    }
    if (!m_bac->biari_decode_symbol(p_aec,
                                    &p_aec->attribute_syn_ctx.ctx_attr_residual_flag1[ctx_id])) {
      if (!m_bac->biari_decode_symbol(p_aec,
                                      &p_aec->attribute_syn_ctx.ctx_attr_residual_flag2[ctx_id])) {
        if (reslayer) {
          val = parseExpGolombReflN(golombNum, p_aec->attribute_syn_ctx.ctx_attr_residual_prefix,
                                    p_aec->attribute_syn_ctx.ctx_attr_residual_suffix) +
            3;
        } else {
          int golombDeVal =
            parseExpGolombReflN(golombNum, p_aec->attribute_syn_ctx.ctx_attr_residual_prefix,
                                p_aec->attribute_syn_ctx.ctx_attr_residual_suffix);
          val = golombDeVal + 3;
        }

      } else {
        val = 2;
      }
    } else {
      val = 1;
    }
    val = (sign_bit == 1) ? val : -val;
    return val;
  } else {
    if (!residualminusone_flag) {
      Int val = 0;

      if (!m_bac->biari_decode_symbol(
            p_aec,
            &p_aec->attribute_syn_ctx.ctx_attr_residual_eq0[ctx_id + (3 * isDuplicatePoint)])) {
        if (!m_bac->biari_decode_symbol(
              p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_flag1[ctx_id])) {
          if (!m_bac->biari_decode_symbol(
                p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_flag2[ctx_id])) {
            if (reslayer) {
              val = parseExpGolombN(golombNum,
                                    &p_aec->attribute_syn_ctx.ctx_attr_residual_prefix[ctx_id],
                                    &p_aec->attribute_syn_ctx.ctx_attr_residual_suffix[ctx_id]) +
                3;
            } else {
              int golombDeVal = parseExpGolombAdp(golombNum, colorType, ctx_id);
              val = golombDeVal + 3;
            }
          } else {
            val = 2;
          }
        } else {
          val = 1;
        }
      }
      return val;
    } else {
      Int val = 0;
      if (!m_bac->biari_decode_symbol(
            p_aec,
            &p_aec->attribute_syn_ctx
               .ctx_attr_residual_minusone_eq0[ctx_id + (3 * isDuplicatePoint)])) {
        if (!m_bac->biari_decode_symbol(
              p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_minusone_flag1[ctx_id])) {
          if (!m_bac->biari_decode_symbol(
                p_aec, &p_aec->attribute_syn_ctx.ctx_attr_residual_minusone_flag2[ctx_id])) {
            if (reslayer) {
              val = parseExpGolombN(golombNum,
                                    &p_aec->attribute_syn_ctx.ctx_attr_residual_prefix[ctx_id],
                                    &p_aec->attribute_syn_ctx.ctx_attr_residual_suffix[ctx_id]) +
                3;
            } else {
              int golombDeVal = parseExpGolombAdp(golombNum, colorType, ctx_id);
              val = golombDeVal + 3;
            }
          } else {
            val = 2;
          }
        } else {
          val = 1;
        }
      }
      val = val + 1;
      return val;
    }
  }
}

Int TDecBacTop::parseAttrequaltwo() {
  int val = m_bac->biari_decode_symbol(p_aec, &p_aec->attribute_syn_ctx.ctx_attr_flag2);
  return val;
}
Int TDecBacTop::parseAttrequalone() {
  Int val = m_bac->biari_decode_symbol(p_aec, &p_aec->attribute_syn_ctx.ctx_attr_flag1);
  return val;
}

UInt TDecBacTop::decodeGeomTreeType() {
  UInt geomTreeType = m_bac->biari_decode_symbol(p_aec, &p_aec->geometry_syn_ctx.ctxGeomTreeType);
  return geomTreeType;
}

Bool TDecBacTop::decodeKOctreeDepthflag() {
  bool m = m_bac->biari_decode_symbol(p_aec, &p_aec->geometry_syn_ctx.ctxdepth);
  return m;
}

V3<int32_t> TDecBacTop::decodePredTreeResidual(const V3<int32_t>& cur, const V3<int32_t>& pred,
                                               const V3<int32_t>& numbits_Lcusize_log2) {
  V3<int32_t> residual;
  V3<int32_t> absRes;
  Bool possibleFlag[8] = {true, true, true, true, true, true, true, true};
  const int tempIndex[8][3] = {
    0, 2, 4, 0, 2, 5, 0, 3, 4, 0, 3, 5, 1, 2, 4, 1, 2, 5, 1, 3, 4, 1, 3, 5,
  };
  const int zeroResidual[3][4] = {4, 5, 6, 7, 2, 3, 6, 7, 1, 3, 5, 7};
  const int signofStatus[8][3] = {-1, -1, -1, -1, -1, 1, -1, 1, -1, -1, 1, 1,
                                  1,  -1, -1, 1,  -1, 1, 1,  1, -1, 1,  1, 1};
  int NewStatus[8];
  int tempDist3[3];
  int tempDist6[6];
  int possibleStatus = -1;

  int32_t numBits = 0;
  int32_t res = 0;
  int32_t resRemainder = 0;
  for (int k = 0, ctxIdx = 0; k < 3; k++) {
    auto p_ctxIsZero = &p_aec->geometry_syn_ctx.ctxPredTreeIsZero[k];
    if (m_bac->biari_decode_symbol(p_aec, p_ctxIsZero)) {
      residual[k] = 0;
      possibleFlag[zeroResidual[k][0]] = false;
      possibleFlag[zeroResidual[k][1]] = false;
      possibleFlag[zeroResidual[k][2]] = false;
      possibleFlag[zeroResidual[k][3]] = false;
      absRes[k] = 0;
      continue;
    }
    auto p_ctxs = p_aec->geometry_syn_ctx.ctxPredTreeNumBits[k];
    std::vector<bool> bitbit;
    bitbit.resize(numbits_Lcusize_log2[k]);
    int32_t numBits = 0;
    for (int Idx = 0, i = 0; i < numbits_Lcusize_log2[k]; i++) {
      bitbit[i] = m_bac->biari_decode_symbol(p_aec, &(p_ctxs[Idx]));
      numBits = numBits | (bitbit[i] << i);
      if (i < 2)
        Idx = (1 << (i + 1)) - 1 + bitbit[i];
      else if (i == 2)
        Idx = 5 + bitbit[2] + (bitbit[1] << 1);
      else
        Idx = 9;
    }

    resRemainder = m_bac->biari_decode_symbol_eq_prob(p_aec);
    --numBits;
    if (numBits <= 0) {
      res = 1 + numBits;
    } else {
      res = (1 << numBits);
      for (int i = 0; i < numBits; i++) {
        res += m_bac->biari_decode_symbol_eq_prob(p_aec) << i;
      }
    }

    res = (res << 1) + ((resRemainder == 0) ? 1 : 0) + 1;
    absRes[k] = res;
  }

  tempDist3[0] = cur[0] - pred[0];
  tempDist3[1] = cur[1] - pred[1];
  tempDist3[2] = cur[2] - pred[2];
  tempDist6[0] = abs(tempDist3[0] - absRes[0]);
  tempDist6[1] = abs(tempDist3[0] + absRes[0]);
  tempDist6[2] = abs(tempDist3[1] - absRes[1]);
  tempDist6[3] = abs(tempDist3[1] + absRes[1]);
  tempDist6[4] = abs(tempDist3[2] - absRes[2]);
  tempDist6[5] = abs(tempDist3[2] + absRes[2]);
  int distLimit = abs(tempDist3[0]) + abs(tempDist3[1]) + abs(tempDist3[2]);

  int QF = 0;
  for (int i = 0; i < 3; i++) {
    if (absRes[i] && (tempDist3[i] >> 31)) {
      QF |= 1 << (2 - i);
    }
  }

  //remark
  for (int I = 0; I < 8; I++) {
    int i = QF ^ I;
    if ((possibleFlag[i])) {
      int dst =
        tempDist6[tempIndex[i][0]] + tempDist6[tempIndex[i][1]] + tempDist6[tempIndex[i][2]];
      if (dst >= distLimit) {
        possibleStatus++;
        NewStatus[possibleStatus] = i;
      }
    }
  }
  int sumofStatus = 0;
  for (int i = 0; i < 3; i++) {
    if (sumofStatus + (1 << (2 - i)) <= possibleStatus) {
      auto p_ctxSign = &p_aec->geometry_syn_ctx.ctxPredTreeSign[i];
      int temp = m_bac->biari_decode_symbol(p_aec, p_ctxSign);
      sumofStatus += (temp << (2 - i));
    }
  }

  residual[0] = absRes[0] * signofStatus[NewStatus[sumofStatus]][0];
  residual[1] = absRes[1] * signofStatus[NewStatus[sumofStatus]][1];
  residual[2] = absRes[2] * signofStatus[NewStatus[sumofStatus]][2];

  return residual;
}

UInt TDecBacTop::decodePredTreeNumPtsInLcu() {
  int32_t numBits;
  numBits = m_bac->biari_decode_symbol_eq_prob(p_aec);
  numBits += m_bac->biari_decode_symbol_eq_prob(p_aec) << 1;
  numBits += m_bac->biari_decode_symbol_eq_prob(p_aec) << 2;
  numBits += m_bac->biari_decode_symbol_eq_prob(p_aec) << 3;
  numBits += m_bac->biari_decode_symbol_eq_prob(p_aec) << 4;

  int32_t numPtrsInLcu = 0;
  --numBits;
  if (numBits <= 0) {
    numPtrsInLcu = 2 + numBits;
  } else {
    numPtrsInLcu = 1 + (1 << numBits);
    for (int i = 0; i < numBits; i++) {
      numPtrsInLcu += m_bac->biari_decode_symbol_eq_prob(p_aec) << i;
    }
  }
  return numPtrsInLcu;
}

UInt TDecBacTop::decodeOccUsingMemoryChannel(TComOctreePartitionParams& params,
                                             TComGeomContext& geomCtx,
                                             bool& planarModeEligibleForSlice) {
  UInt occupancy = 0;
  UInt16 context = 0;
  int minDimension = params.nodeSizeLog2.minIndex();
  UInt* encodedChildNode = geomCtx.ctxChildOccu;
  UInt ctx6Parent = geomCtx.ctxParent;
  const UInt occupancySkip = params.occupancySkip;
  static const UInt8 adjacentCIdx[8][7] = {
    {7, 14, 21, 28, 35, 42, 49},  {14, 15, 28, 29, 42, 43, 56}, {21, 28, 23, 30, 49, 56, 51},
    {28, 29, 30, 31, 56, 57, 58}, {35, 42, 49, 56, 39, 46, 53}, {42, 43, 56, 57, 46, 47, 60},
    {49, 56, 51, 58, 53, 60, 55}, {56, 57, 58, 59, 60, 61, 62}};
  static const UInt8 minDime2ParentNeiIndex[3] = {3, 5, 6};
  static const UInt maxNumOccupiedBins[8] = {9, 4, 4, 2, 4, 2, 2, 1};

  static const UInt8 zPlanarNodeIndex[5] = {1, 3, 5, 8, 9};
  int zNeighborOccupancy = 0;
  int zNeighborLowNodeNum = 0;
  int zNeighborHighNodeNum = 0;
  bool preNodePlanarEligible = false;
  if (planarModeEligibleForSlice && (!(occupancySkip & 1))) {
    bool isHighPlanarZ = false;
    bool isLowPlanarZ = false;
    int numOfZPreNode = 0;
    for (int i = 0; i < 5; i++) {
      zNeighborOccupancy = encodedChildNode[zPlanarNodeIndex[i]];
      isHighPlanarZ = (!(zNeighborOccupancy & 0x55)) && (!!(zNeighborOccupancy & 0xaa));
      isLowPlanarZ = (!!(zNeighborOccupancy & 0x55)) && (!(zNeighborOccupancy & 0xaa));
      if (isHighPlanarZ || isLowPlanarZ) {
        numOfZPreNode++;
      }

      zNeighborLowNodeNum += bitCount(zNeighborOccupancy & 0x55);
      zNeighborHighNodeNum += bitCount(zNeighborOccupancy & 0xaa);
    }
    preNodePlanarEligible = !!(numOfZPreNode > 2);
  }

  context_t* bit_ctx = NULL;
  auto p_ctx = p_aec->geometry_syn_ctx.ctx_occupancy;
  UInt numCodedBins = 0;
  UInt maxCodedBins = maxNumOccupiedBins[occupancySkip];
  const UInt* ctx26Parent = geomCtx.ctxParentAdv;
  int ctxFrom6Nei =
    (!!(ctx6Parent & 0x0003)) && (!!(ctx6Parent & 0x000c)) && (!!(ctx6Parent & 0x0030));
  for (Int i = 0; i < 8; i++) {
    if (occupancySkip != 0) {
      if ((occupancySkip & 1) && (i & 1))  ///< skip when z = 1
        continue;
      if ((occupancySkip & 2) && (i & 2))  ///< skip when y = 1
        continue;
      if ((occupancySkip & 4) && (i & 4))  ///< skip when x = 1
        continue;
    }
    Bool bit = true;
    if (occupancy || (numCodedBins < maxCodedBins - 1)) {
      int planarContext = 0;
      if (!(occupancySkip & 1) && preNodePlanarEligible) {
        if ((zNeighborHighNodeNum < 1) && (zNeighborLowNodeNum > 1) && !!(i & 1) &&
            (!(occupancy & 0xaa)))
          planarContext = 1;
        if ((zNeighborLowNodeNum < 1) && (zNeighborHighNodeNum > 1) &&
            ((!(i & 1)) && (!(occupancy & 0x55))))
          planarContext = 2;
      }
      if (planarContext > 0) {
        bit_ctx = p_aec->geometry_syn_ctx.planarMode;
        context = planarContext - 1;
        bit = m_bac->biari_decode_symbol(p_aec, bit_ctx + context);

      } else {
        UInt8 ctxFrom3FaceNei = 0;
        UInt8 ctxFrom3EdgeNei = 0;
        for (int j = 0; j < 3; j++) {
          ctxFrom3FaceNei |= (!!((ctx26Parent[i] >> j) & 0x01) << (2 - j));
          ctxFrom3EdgeNei |= (!!((ctx26Parent[i] >> (j + 3)) & 0x01) << (2 - j));
        }
        UInt8 ctxFromParent = contextMap(ctxFrom3FaceNei, ctxFrom3EdgeNei) * 2 + ctxFrom6Nei;

        UInt16 childInformation = 0;
        const UInt8* adjChidIdx = adjacentCIdx[i];
        for (size_t idx = 0; idx < 7; ++idx) {
          childInformation |=
            !!(encodedChildNode[adjChidIdx[idx] >> 3] & (1 << (adjChidIdx[idx] & 7))) << idx;
        }
        childInformation |= !!(encodedChildNode[3] & (1 << (i))) << 7;
        childInformation |= !!(encodedChildNode[5] & (1 << (i))) << 8;
        childInformation |= !!(encodedChildNode[6] & (1 << (i))) << 9;
        uint8_t* memoryVal = memoryChannel;
        memoryVal += childInformation;
        UInt8 ctxFromMemory = popcnt8(*memoryVal);
        bit_ctx = p_aec->geometry_syn_ctx.ctxMemoryChannel;
        context = ctxFromParent * 9 + ctxFromMemory;
        bit = m_bac->biari_decode_symbol(p_aec, bit_ctx + context);
        *memoryVal = ((*memoryVal) << 1) | bit;
      }
    }
    occupancy |= bit << i;
    numCodedBins++;
    encodedChildNode[7] = occupancy;
  }

  return occupancy;
}

UInt TDecBacTop::decodeOccupancyCode(const TComOctreePartitionParams& params,
                                     TComGeomContext& geomCtx, bool& planarModeEligibleForSlice,
                                     const UInt contextMode) {
  UInt occupancy = 0;
  UInt8 context = 0;
  UInt16 childInformation = 0;
  int minDimension = params.nodeSizeLog2.minIndex();
  const UInt* ctxChildD1 = geomCtx.ctxChildD1;
  UInt8 ctxChildNode[8] = {0};
  for (Int i = 0; i < 8; i++) {
    for (Int j = 0; j < 3; j++) {
      ctxChildNode[i] |= (!!(ctxChildD1[j] & (1 << i))) << j;
    }
  }

  static const UInt8 adjacentIdx[8][3] = {{1, 2, 4}, {0, 3, 5}, {0, 3, 6}, {1, 2, 7},
                                          {0, 5, 6}, {1, 4, 7}, {2, 4, 7}, {3, 5, 6}};
  static const UInt8 adjacentPosIdx[8][3] = {{2, 1, 0}, {5, 1, 0}, {4, 2, 0}, {4, 5, 0},
                                             {3, 2, 1}, {3, 5, 1}, {3, 4, 2}, {3, 4, 5}};

  UInt* encodedChildNode = geomCtx.ctxChildOccu;
  UInt ctxParent = geomCtx.ctxParent;
  const UInt* ctxParentAdv1 = geomCtx.ctxParentAdv1;
  const UInt occupancySkip = params.occupancySkip;
  static const UInt8 adjacentCIdx1[8][7] = {
    {28, 42, 49, 7, 14, 21, 35},  {29, 43, 56, 14, 15, 28, 42}, {30, 56, 61, 21, 28, 23, 49},
    {31, 57, 58, 28, 29, 30, 56}, {56, 46, 53, 35, 42, 49, 39}, {57, 47, 60, 42, 43, 56, 46},
    {58, 60, 55, 49, 56, 51, 53}, {59, 61, 62, 56, 57, 58, 60}};

  static const UInt8 minDime2ParentNeiIndex[3] = {3, 5, 6};
  static const UInt maxNumOccupiedBins[8] = {9, 4, 4, 2, 4, 2, 2, 1};
  static const UInt8 zPlanarNodeIndex[5] = {1, 3, 5, 8, 9};
  int zNeighborOccupancy = 0;
  int zNeighborLowNodeNum = 0;
  int zNeighborHighNodeNum = 0;
  bool preNodePlanarEligible = false;
  if (planarModeEligibleForSlice && (!(occupancySkip & 1))) {
    bool isHighPlanarZ = false;
    bool isLowPlanarZ = false;
    int numOfZPreNode = 0;
    for (int i = 0; i < 5; i++) {
      zNeighborOccupancy = encodedChildNode[zPlanarNodeIndex[i]];
      isHighPlanarZ = (!(zNeighborOccupancy & 0x55)) && (!!(zNeighborOccupancy & 0xaa));
      isLowPlanarZ = (!!(zNeighborOccupancy & 0x55)) && (!(zNeighborOccupancy & 0xaa));
      if (isHighPlanarZ || isLowPlanarZ) {
        numOfZPreNode++;
      }

      zNeighborLowNodeNum += bitCount(zNeighborOccupancy & 0x55);
      zNeighborHighNodeNum += bitCount(zNeighborOccupancy & 0xaa);
    }
    preNodePlanarEligible = !!(numOfZPreNode > 2);
  }

  auto p_ctx = p_aec->geometry_syn_ctx.ctx_occupancy;
  UInt numCodedBins = 0;
  UInt maxCodedBins = maxNumOccupiedBins[occupancySkip];

  for (Int i = 0; i < 8; i++) {
    if (occupancySkip != 0) {
      if ((occupancySkip & 1) && (i & 1))  ///< skip when z = 1
        continue;
      if ((occupancySkip & 2) && (i & 2))  ///< skip when y = 1
        continue;
      if ((occupancySkip & 4) && (i & 4))  ///< skip when x = 1
        continue;
    }

    Bool bit = true;
    if (occupancy || (numCodedBins < maxCodedBins - 1)) {
      UInt16 ctxChild = 0;
      UInt8 ctxm = 0;
      context_t* bit_ctx = NULL;
      int planarContext = 0;
      if (!(occupancySkip & 1) && preNodePlanarEligible) {
        if ((zNeighborHighNodeNum < 1) && (zNeighborLowNodeNum > 1) && !!(i & 1) &&
            (!(occupancy & 0xaa)))
          planarContext = 1;
        if ((zNeighborLowNodeNum < 1) && (zNeighborHighNodeNum > 1) &&
            ((!(i & 1)) && (!(occupancy & 0x55))))
          planarContext = 2;
      }
      if (planarContext > 0) {
        bit_ctx = p_aec->geometry_syn_ctx.planarMode;
        context = planarContext - 1;
        bit = m_bac->biari_decode_symbol(p_aec, bit_ctx + context);

      } else {
        const UInt8* adjChidIdx = adjacentCIdx1[i];
        for (size_t idx = 3; idx < 7; ++idx) {
          ctxChild |= !!(encodedChildNode[adjChidIdx[idx] >> 3] & (1 << (adjChidIdx[idx] & 7)))
            << (idx - 3);
        }

        ctxChild |= !!(encodedChildNode[minDime2ParentNeiIndex[minDimension]] & (1 << (i))) << 4;
        ctxChild |= !!(encodedChildNode[3] & (1 << (i))) << 5;
        ctxChild |= !!(encodedChildNode[5] & (1 << (i))) << 6;
        ctxChild |= !!(encodedChildNode[6] & (1 << (i))) << 7;
        ctxChild |= !!(encodedChildNode[1] & (1 << (i))) << 8;
        ctxChild |= !!(encodedChildNode[2] & (1 << (i))) << 9;

        if (ctxChildNode[i]) {
          bit_ctx = p_aec->geometry_syn_ctx.ctx_occupancyCombinechild[ctxParentAdv1[i]];

          context = ctxChildNode[i] - 1;
          bit = m_bac->biari_decode_symbol(p_aec, bit_ctx + context);
        } else

          if (ctxChild) {
          bit_ctx = p_aec->geometry_syn_ctx.ctx_occupancyCombineParent1[ctxParentAdv1[i]];
          childInformation = ctxChild;
          uint8_t* memoryVal = memoryChannel;
          memoryVal += childInformation;
          UInt8 ctxFromMemory = popcnt8(*memoryVal);
          context = ctxFromMemory;
          bit = m_bac->biari_decode_symbol(p_aec, bit_ctx + context);
          *memoryVal = ((*memoryVal) << 1) | bit;
        } else {
          bit_ctx = p_aec->geometry_syn_ctx.ctxRUB_occupancy[i];
          context = ctxParentAdv1[i];
          bit = m_bac->biari_decode_symbol(p_aec, bit_ctx + context);
        }
      }
    }
    occupancy |= bit << i;
    numCodedBins++;
    encodedChildNode[7] = occupancy;

    if (bit) {
      for (Int k = 0; k < 3; k++) {
        assert(!((ctxChildNode[adjacentIdx[i][k]] >> adjacentPosIdx[i][k]) & 1));
        ctxChildNode[adjacentIdx[i][k]] |= 1 << adjacentPosIdx[i][k];
      }
    }
  }
  return occupancy;
}

Bool TDecBacTop::decodeSinglePointFlag() {
  return !!m_bac->biari_decode_symbol(p_aec, &p_aec->geometry_syn_ctx.ctx_geom_single_mode_flag);
}

V3<UInt> TDecBacTop::decodeSinglePointIndex(V3<UInt> nodeSizeLog2) {
  V3<UInt> pos;
  while (nodeSizeLog2 > 0) {
    for (Int i = 0; i < 3; ++i) {
      if (nodeSizeLog2[i] > 0) {
        pos[i] |= m_bac->biari_decode_symbol_eq_prob(p_aec) << (nodeSizeLog2[i] - 1);
        --nodeSizeLog2[i];
      }
    }
  }
  return pos;
}

UInt TDecBacTop::decodeDuplicateNumber() {
  UInt numDup = 1;
  auto p_ctx = &p_aec->geometry_syn_ctx.ctx_geom_num_dup_eq1;
  if (!m_bac->biari_decode_symbol(p_aec, p_ctx)) {
    unsigned int l;
    int symbol = 0;
    int k = 0;
    int binary_symbol = 0;
    do {
      l = m_bac->biari_decode_symbol_eq_prob(p_aec);
      if (l == 1) {
        symbol += (1 << k);
        k++;
      }
    } while (l != 0);
    while (k--)  //next binary part
      if (m_bac->biari_decode_symbol_eq_prob(p_aec) == 1) {
        binary_symbol |= (1 << k);
      }
    numDup = symbol + binary_symbol + 2;
  }
  return numDup;
}

Bool TDecBacTop::decodeTerminationFlag() {
  return !!m_bac->biari_decode_final(p_aec);
}

TDecBacTop::TDecBacTop() {
  m_bac = nullptr;
  reset();
}

TDecBacTop::~TDecBacTop() {
  if (m_bac) {
    delete m_bac;
    m_bac = nullptr;
  }
}

Void TDecBacTop::initBac() {
  m_bac = new TDecBacCore;
  m_bac->dec_sbac_init(&m_bitStream);
}

Void TDecBacTop::reset() {
  if (m_bac) {
    delete m_bac;
    m_bac = nullptr;
  }
  memset(&m_bitStream, 0, sizeof(m_bitStream));
  aec.init();
  p_aec = &aec;
  std::fill(begin(memoryChannel), end(memoryChannel), 15);
}

Void TDecBacTop::LcuReset() {
  std::fill(begin(memoryChannel), end(memoryChannel), 15);
  m_bac->init_geometry_contexts(p_aec);
}

Void TDecBacTop::setBitstreamBuffer(TComBufferChunk& buffer, const bool& initDulatAttribute) {
  m_bac->com_bsr_init(&m_bitStream, (UInt8*)buffer.addr, buffer.ssize, NULL);

  if (buffer.getBufferType() == BufferChunkType::BCT_GEOM) {
    m_bac->init_geometry_contexts(p_aec);
    m_bac->aec_start_decoding(p_aec, m_bitStream.beg, 0,
                              buffer.ssize);  // TODO:加上
  } else if (buffer.getBufferType() == BufferChunkType::BCT_COL ||
             buffer.getBufferType() == BufferChunkType::BCT_REFL) {
    m_bac->init_attribute_contexts(p_aec, initDulatAttribute);
    m_bac->aec_start_decoding(p_aec, m_bitStream.beg, 0,
                              buffer.ssize);  // TODO:加上
  }
}

Void TDecBacTop::parseUserData(){};
///< \{
