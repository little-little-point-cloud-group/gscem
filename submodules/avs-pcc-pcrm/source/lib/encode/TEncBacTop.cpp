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
#include "common/TComRom.h"
#include "common/contributors.h"

///< \in TLibEncoder \{

/**
 * Class TEncBacTop
 * entropy encoder
 */

//////////////////////////////////////////////////////////////////////////
// Public class functions
//////////////////////////////////////////////////////////////////////////

Void TEncBacTop::codeSPS(const SequenceParameterSet& sps) {
  UInt32 geomQuantStep_upper, geomQuantStep_lower;
  Int geomBoundingBoxOrigin_x_upper, geomBoundingBoxOrigin_x_lower;
  Int geomBoundingBoxOrigin_y_upper, geomBoundingBoxOrigin_y_lower;
  Int geomBoundingBoxOrigin_z_upper, geomBoundingBoxOrigin_z_lower;
  UInt geomBoundingBoxSize_w_upper, geomBoundingBoxSize_w_lower;
  UInt geomBoundingBoxSize_h_upper, geomBoundingBoxSize_h_lower;
  UInt geomBoundingBoxSize_d_upper, geomBoundingBoxSize_d_lower;

  UInt spsStartCode_w_upper, spsStartCode_w_lower;

  geomQuantStep_upper = (*(UInt32*)(&sps.geomQuantStep) >> 16) & 0xffff;
  geomQuantStep_lower = (*(UInt32*)(&sps.geomQuantStep)) & 0xffff;
  spsStartCode_w_upper = (pcc_sequence_start_code >> 16) & 0xffff;
  spsStartCode_w_lower = pcc_sequence_start_code& 0xffff;

  m_bac->com_bsw_write(&m_bitStream, spsStartCode_w_upper, 16);  ///< the start code of sps
  m_bac->com_bsw_write(&m_bitStream, spsStartCode_w_lower, 16);  ///< the start code of sps

  m_bac->com_bsw_write(&m_bitStream, sps.level, 8);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  geomBoundingBoxOrigin_x_upper = (*(Int*)(&sps.geomBoundingBoxOrigin[0]) >> 16) & 0xffff;
  geomBoundingBoxOrigin_x_lower = (*(Int*)(&sps.geomBoundingBoxOrigin[0])) & 0xffff;
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_x_upper, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_x_lower, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  geomBoundingBoxOrigin_y_upper = (*(Int*)(&sps.geomBoundingBoxOrigin[1]) >> 16) & 0xffff;
  geomBoundingBoxOrigin_y_lower = (*(Int*)(&sps.geomBoundingBoxOrigin[1])) & 0xffff;
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_y_upper, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_y_lower, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  geomBoundingBoxOrigin_z_upper = (*(Int*)(&sps.geomBoundingBoxOrigin[2]) >> 16) & 0xffff;
  geomBoundingBoxOrigin_z_lower = (*(Int*)(&sps.geomBoundingBoxOrigin[2])) & 0xffff;
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_z_upper, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_z_lower, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  geomBoundingBoxSize_w_upper = (*(Int*)(&sps.geomBoundingBoxSize[0]) >> 16) & 0xffff;
  geomBoundingBoxSize_w_lower = (*(Int*)(&sps.geomBoundingBoxSize[0])) & 0xffff;
  m_bac->com_bsw_write_ue(&m_bitStream, geomBoundingBoxSize_w_upper);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write_ue(&m_bitStream, geomBoundingBoxSize_w_lower);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  geomBoundingBoxSize_h_upper = (*(Int*)(&sps.geomBoundingBoxSize[1]) >> 16) & 0xffff;
  geomBoundingBoxSize_h_lower = (*(Int*)(&sps.geomBoundingBoxSize[1])) & 0xffff;
  m_bac->com_bsw_write_ue(&m_bitStream, geomBoundingBoxSize_h_upper);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write_ue(&m_bitStream, geomBoundingBoxSize_h_lower);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  geomBoundingBoxSize_d_upper = (*(Int*)(&sps.geomBoundingBoxSize[2]) >> 16) & 0xffff;
  geomBoundingBoxSize_d_lower = (*(Int*)(&sps.geomBoundingBoxSize[2])) & 0xffff;
  m_bac->com_bsw_write_ue(&m_bitStream, geomBoundingBoxSize_d_upper);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write_ue(&m_bitStream, geomBoundingBoxSize_d_lower);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  m_bac->com_bsw_write(&m_bitStream, geomQuantStep_upper, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write(&m_bitStream, geomQuantStep_lower, 16);

  m_bac->com_bsw_write1(&m_bitStream, sps.geomRemoveDuplicateFlag);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  // encoding attribute params
  m_bac->com_bsw_write1(&m_bitStream, sps.attrPresentFlag);
  if (sps.attrPresentFlag) {
    m_bac->com_bsw_write_ue(&m_bitStream, sps.colorQuantParam);
    m_bac->com_bsw_write_ue(&m_bitStream, sps.reflQuantParam);
    m_bac->com_bsw_write(&m_bitStream, sps.maxNumAttrMinus1, 7);
    m_bac->com_bsw_write1(&m_bitStream, sps.sps_multi_set_flag);
  }

  m_bac->com_bsw_write_byte_align(&m_bitStream);

}

Void TEncBacTop::codeGPS(const GeometryParameterSet& gps) {
  if (gps.lcuNodeSizeLog2 == 0)
    m_bac->com_bsw_write_ue(&m_bitStream, 0);
  else
    m_bac->com_bsw_write_ue(&m_bitStream, gps.lcuNodeSizeLog2 - 1);
  m_bac->com_bsw_write_ue(&m_bitStream, gps.log2geomTreeMaxSizeMinus8);
  m_bac->com_bsw_write1(&m_bitStream, gps.im_qtbt_flag);
  m_bac->com_bsw_write1(&m_bitStream, gps.singleModeFlag);
  m_bac->com_bsw_write_ue(&m_bitStream, gps.OccupancymapSizelog2);
  m_bac->com_bsw_write1(&m_bitStream, gps.saveStateFlag);
  if (!gps.saveStateFlag)
    m_bac->com_bsw_write1(&m_bitStream, gps.lcu_dependency_flag);
  m_bac->com_bsw_write_byte_align(&m_bitStream);
}

Void TEncBacTop::codeSPSEndCode() {
  for (int32_t i = 0; i < 32; ++i) {
    m_bac->biari_encode_symbol_eq_prob_aec(p_aec, (pcc_sequence_end_code >> i) & 1);
  }
  m_bitStream.cur = p_aec->p;
}

Void TEncBacTop::codeSliceGeomEndCode() {

  for (int32_t i = 0; i < 32; ++i) {
    m_bac->biari_encode_symbol_eq_prob_aec(p_aec, (slice_geometry_end_code >> i) & 1);
  }
  m_bitStream.cur = p_aec->p;
  
}

Void TEncBacTop::codeSliceAttrEndCode() {
  for (int32_t i = 0; i < 32; ++i) {
    m_bac->biari_encode_symbol_eq_prob_aec(p_aec, (slice_attribute_end_code >> i) & 1);
  }
  m_bitStream.cur = p_aec->p;
}

Void TEncBacTop::codeGBH(const GeometryParameterSet& gps, const GeometryBrickHeader& gbh) {

  UInt sliceGeomStartCode_w_upper, sliceGeomStartCode_w_lower;
  sliceGeomStartCode_w_upper = (slice_geometry_start_code >> 16) & 0xffff;
  sliceGeomStartCode_w_lower = slice_geometry_start_code & 0xffff;
  m_bac->com_bsw_write(&m_bitStream, sliceGeomStartCode_w_upper,
                       16);  ///< the start code of slice Geometry
  m_bac->com_bsw_write(&m_bitStream, sliceGeomStartCode_w_lower,
                       16);  ///< the start code of frame head
  m_bac->com_bsw_write_ue(&m_bitStream, gbh.sliceID);
  m_bac->com_bsw_write1(&m_bitStream, gbh.geom_context_mode);
  if (gps.im_qtbt_flag) {
    m_bac->com_bsw_write_ue(&m_bitStream, gbh.im_qtbt_num_before_ot);
    m_bac->com_bsw_write_ue(&m_bitStream, gbh.im_qtbt_min_size);
  }
  if (gps.singleModeFlag) {
    m_bac->com_bsw_write1(&m_bitStream, gbh.singleModeFlagInSlice);
  }
  m_bac->com_bsw_write1(&m_bitStream, gbh.planarModeEligibleForSlice);

  Int geomBoundingBoxOrigin_x_upper, geomBoundingBoxOrigin_x_lower;
  Int geomBoundingBoxOrigin_y_upper, geomBoundingBoxOrigin_y_lower;
  Int geomBoundingBoxOrigin_z_upper, geomBoundingBoxOrigin_z_lower;
  UInt nodeSizeLog2_x_upper, nodeSizeLog2_x_lower;
  UInt nodeSizeLog2_y_upper, nodeSizeLog2_y_lower;
  UInt nodeSizeLog2_z_upper, nodeSizeLog2_z_lower;
  UInt geomNumPoints_upper, geomNumPoints_lower;

  geomBoundingBoxOrigin_x_upper = (*(Int*)(&gbh.geomBoundingBoxOrigin[0]) >> 16) & 0xffff;
  geomBoundingBoxOrigin_x_lower = (*(Int*)(&gbh.geomBoundingBoxOrigin[0])) & 0xffff;
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_x_upper, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_x_lower, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  geomBoundingBoxOrigin_y_upper = (*(Int*)(&gbh.geomBoundingBoxOrigin[1]) >> 16) & 0xffff;
  geomBoundingBoxOrigin_y_lower = (*(Int*)(&gbh.geomBoundingBoxOrigin[1])) & 0xffff;
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_y_upper, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_y_lower, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  geomBoundingBoxOrigin_z_upper = (*(Int*)(&gbh.geomBoundingBoxOrigin[2]) >> 16) & 0xffff;
  geomBoundingBoxOrigin_z_lower = (*(Int*)(&gbh.geomBoundingBoxOrigin[2])) & 0xffff;
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_z_upper, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write(&m_bitStream, geomBoundingBoxOrigin_z_lower, 16);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  nodeSizeLog2_x_upper = (*(Int*)(&gbh.nodeSizeLog2[0]) >> 16) & 0xffff;
  nodeSizeLog2_x_lower = (*(Int*)(&gbh.nodeSizeLog2[0])) & 0xffff;
  m_bac->com_bsw_write_ue(&m_bitStream, nodeSizeLog2_x_upper);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write_ue(&m_bitStream, nodeSizeLog2_x_lower);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  nodeSizeLog2_y_upper = (*(Int*)(&gbh.nodeSizeLog2[1]) >> 16) & 0xffff;
  nodeSizeLog2_y_lower = (*(Int*)(&gbh.nodeSizeLog2[1])) & 0xffff;
  m_bac->com_bsw_write_ue(&m_bitStream, nodeSizeLog2_y_upper);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write_ue(&m_bitStream, nodeSizeLog2_y_lower);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  nodeSizeLog2_z_upper = (*(Int*)(&gbh.nodeSizeLog2[2]) >> 16) & 0xffff;
  nodeSizeLog2_z_lower = (*(Int*)(&gbh.nodeSizeLog2[2])) & 0xffff;
  m_bac->com_bsw_write_ue(&m_bitStream, nodeSizeLog2_z_upper);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write_ue(&m_bitStream, nodeSizeLog2_z_lower);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  geomNumPoints_upper = (*(UInt*)(&gbh.geomNumPoints) >> 16) & 0xffff;
  geomNumPoints_lower = (*(UInt*)(&gbh.geomNumPoints)) & 0xffff;
  m_bac->com_bsw_write_ue(&m_bitStream, geomNumPoints_upper);
  m_bac->com_bsw_write1(&m_bitStream, 1);
  m_bac->com_bsw_write_ue(&m_bitStream, geomNumPoints_lower);
  m_bac->com_bsw_write1(&m_bitStream, 1);

  m_bac->com_bsw_write_byte_align(&m_bitStream);
}

Void TEncBacTop::codeAPS(AttributeParameterSet& aps, const SequenceParameterSet& sps) {
  for (int attrIdx = 0; attrIdx < (sps.maxNumAttrMinus1 + 1); attrIdx++) {
    m_bac->com_bsw_write1(&m_bitStream, aps.attributePresentFlag[attrIdx]);
    if (aps.attributePresentFlag[attrIdx]) {
      m_bac->com_bsw_write_ue(&m_bitStream, aps.attribute_num_data_set_minus1[attrIdx]);
      if (sps.sps_multi_set_flag)
        m_bac->com_bsw_write1(&m_bitStream, aps.multi_data_set_flag[attrIdx]);
      if (aps.multi_data_set_flag[attrIdx])
        m_bac->com_bsw_write_ue(&m_bitStream, aps.attribute_num_set_minus1[attrIdx]);
      for (int multiIdx = 0; multiIdx < aps.attribute_num_set_minus1[attrIdx] + 1; ++multiIdx) {
        m_bac->com_bsw_write_ue(&m_bitStream, aps.outputMultiBitDepthMinus1[attrIdx][multiIdx]);
        if (attrIdx == 0) {
          m_bac->com_bsw_write1(&m_bitStream, aps.orderMultiSwitch[multiIdx]);
          m_bac->com_bsw_write_ue(&m_bitStream, aps.colorMultiReordermode[multiIdx]);
          m_bac->com_bsw_write_ue(&m_bitStream, aps.colorMultiGolombNum[multiIdx]);
          m_bac->com_bsw_write_ue(&m_bitStream, aps.log2golombMultiGroupSize[multiIdx]);
        }
        if (attrIdx == 1) {
          m_bac->com_bsw_write_ue(&m_bitStream, aps.axisMultiBias[multiIdx]);
          m_bac->com_bsw_write_ue(&m_bitStream, aps.refMultiReordermode[multiIdx]);
          m_bac->com_bsw_write_ue(&m_bitStream, aps.refMultiGolombNum[multiIdx]);
          m_bac->com_bsw_write_ue(&m_bitStream, aps.predMultiFixedPointFracBit[multiIdx]);
          m_bac->com_bsw_write_ue(&m_bitStream, aps.multiAttriGroupID[multiIdx]);
        }
        aps.colorMultiOutputDepth[multiIdx] = aps.outputMultiBitDepthMinus1[0][multiIdx] + 1;
        aps.reflMultiOutputDepth[multiIdx] = aps.outputMultiBitDepthMinus1[1][multiIdx] + 1;

        m_bac->com_bsw_write(&m_bitStream, aps.transformMulti[attrIdx][multiIdx], 2);
        if ((aps.transformMulti[attrIdx][multiIdx] == 0) ||
            (aps.transformMulti[attrIdx][multiIdx] == 2)) {
          m_bac->com_bsw_write(&m_bitStream,
                               aps.maxMultiNumOfNeighboursLog2Minus7[attrIdx][multiIdx], 2);
         
          if (attrIdx == 0) {
            m_bac->com_bsw_write1(&m_bitStream, aps.crossMultiComponentPred[multiIdx]);
            m_bac->com_bsw_write_se(&m_bitStream, aps.chromaMultiQpOffsetCb[multiIdx]);
            m_bac->com_bsw_write_se(&m_bitStream, aps.chromaMultiQpOffsetCr[multiIdx]);
          }
          if (attrIdx == 1) {
            m_bac->com_bsw_write_ue(&m_bitStream, aps.nearestMultiPredParam1[multiIdx]);
            m_bac->com_bsw_write_ue(&m_bitStream, aps.nearestMultiPredParam2[multiIdx]);
            m_bac->com_bsw_write_ue(&m_bitStream, aps.log2predDistWeightMultiGroupSize[multiIdx]);
          }
        }
        if (aps.transformMulti[attrIdx][multiIdx] == 1) {
          m_bac->com_bsw_write_ue(&m_bitStream, aps.kMultiFracBits[attrIdx][multiIdx]);
          m_bac->com_bsw_write_ue(&m_bitStream, aps.attrMultiTransformQpDelta[attrIdx][multiIdx]);
          m_bac->com_bsw_write_ue(&m_bitStream, aps.transformMultiSegmentSize[attrIdx][multiIdx]);
          m_bac->com_bsw_write1(&m_bitStream, aps.transMultiResLayer[attrIdx][multiIdx]);
          if (attrIdx == 0) {
            m_bac->com_bsw_write_se(&m_bitStream, aps.colorMultiInitPredTransRatio[multiIdx]);
          }
          if (attrIdx == 1) {
            m_bac->com_bsw_write_se(&m_bitStream, aps.refMultiInitPredTransRatio[multiIdx]);
          }
        }

        if (aps.transformMulti[attrIdx][multiIdx] == 2) {
          m_bac->com_bsw_write_ue(&m_bitStream,
                                  aps.log2MultimaxNumofCoeffMinus8[attrIdx][multiIdx]);
          if (aps.log2MultimaxNumofCoeffMinus8[attrIdx][multiIdx]) {
            aps.maxMultiNumofCoeff[attrIdx][multiIdx] = 1
              << (aps.log2MultimaxNumofCoeffMinus8[attrIdx][multiIdx] + 8);
          }
          m_bac->com_bsw_write_se(&m_bitStream, aps.QpMultiOffsetDC[attrIdx][multiIdx]);
          m_bac->com_bsw_write_se(&m_bitStream, aps.QpMultiOffsetAC[attrIdx][multiIdx]);
          if (attrIdx == 0) {
            m_bac->com_bsw_write_ue(&m_bitStream, aps.colorMaxMultiTransNum[attrIdx][multiIdx]);
            m_bac->com_bsw_write_se(&m_bitStream, aps.chromaMultiQpOffsetDC[multiIdx]);
            m_bac->com_bsw_write_se(&m_bitStream, aps.chromaMultiQpOffsetAC[multiIdx]);
            m_bac->com_bsw_write1(&m_bitStream, aps.colorMultiQPAdjustFlag[multiIdx]);
            if (aps.colorMultiQPAdjustFlag[multiIdx]) {
              m_bac->com_bsw_write_se(&m_bitStream, aps.colorMultiQPAdjustScalar[multiIdx]);
            }
          }
          if (attrIdx == 1) {
            m_bac->com_bsw_write_ue(&m_bitStream, aps.reflMaxMultiTransNum[attrIdx][multiIdx]);
            m_bac->com_bsw_write1(&m_bitStream, aps.refMultiGroupPredict[multiIdx]);
          }
        }

        m_bac->com_bsw_write_ue(&m_bitStream,
                                aps.log2coeffMultiLengthControlMinus8[attrIdx][multiIdx]);
        if (aps.log2coeffMultiLengthControlMinus8[attrIdx][multiIdx]) {
          aps.coeffMultiLengthControl[attrIdx][multiIdx] = 1
            << (aps.log2coeffMultiLengthControlMinus8[attrIdx][multiIdx] + 8);
        }
      }
    }
  }
  if (aps.attributePresentFlag[0] && aps.attributePresentFlag[1]) {
    m_bac->com_bsw_write1(&m_bitStream, aps.crossAttrTypePred);
    if (aps.crossAttrTypePred) {
      m_bac->com_bsw_write1(&m_bitStream, aps.attrEncodeOrder);
      m_bac->com_bsw_write(&m_bitStream, aps.crossAttrTypePredParam1, 15);
      m_bac->com_bsw_write(&m_bitStream, aps.crossAttrTypePredParam2, 21);
    }
  }
  m_bac->com_bsw_write_byte_align(&m_bitStream);
}

Void TEncBacTop::codeFrameHead(const FrameHeader& frameHead) {
  UInt frameStartCode_w_upper, frameStartCode_w_lower;
  frameStartCode_w_upper = (frame_start_code >> 16) & 0xffff;
  frameStartCode_w_lower = frame_start_code & 0xffff;
  m_bac->com_bsw_write(&m_bitStream, frameStartCode_w_upper, 16);  ///< the start code of frame head
  m_bac->com_bsw_write(&m_bitStream, frameStartCode_w_lower, 16);  ///< the start code of frame head

  m_bac->com_bsw_write_ue(&m_bitStream, frameHead.frame_index);
  m_bac->com_bsw_write_ue(&m_bitStream, frameHead.num_slice_minus_one);
  m_bac->com_bsw_write1(&m_bitStream, frameHead.timestamp_flag);
  if (frameHead.timestamp_flag) {
    m_bac->com_bsw_write_ue(&m_bitStream, frameHead.timestamp);
  }

  UInt geomNumPoints_upper, geomNumPoints_lower;
  geomNumPoints_upper = (*(UInt*)(&frameHead.geomNumPoints) >> 16) & 0xffff;
  geomNumPoints_lower = (*(UInt*)(&frameHead.geomNumPoints)) & 0xffff;
  m_bac->com_bsw_write_ue(&m_bitStream, geomNumPoints_upper);
  m_bac->com_bsw_write_ue(&m_bitStream, geomNumPoints_lower);

  m_bac->com_bsw_write_byte_align(&m_bitStream);
}

Void TEncBacTop::codeABH(const AttributeBrickHeader& ah, const AttributeParameterSet& aps,
                         const SequenceParameterSet& sps) {
  UInt sliceAttrStartCode_w_upper, sliceAttrStartCode_w_lower;
  sliceAttrStartCode_w_upper = (slice_attribute_start_code >> 16) & 0xffff;
  sliceAttrStartCode_w_lower = slice_attribute_start_code & 0xffff;
  m_bac->com_bsw_write(&m_bitStream, sliceAttrStartCode_w_upper,
                       16);  ///< the start code of frame head
  m_bac->com_bsw_write(&m_bitStream, sliceAttrStartCode_w_lower,
                       16);  ///< the start code of frame head

  for (int attrIdx = 0; attrIdx < (sps.maxNumAttrMinus1 + 1); attrIdx++) {
    if (aps.attributePresentFlag[attrIdx]) {
      for (int multiIdx = 0; multiIdx < aps.attribute_num_set_minus1[attrIdx] + 1; ++multiIdx)
        m_bac->com_bsw_write_ue(&m_bitStream, ah.attribute_ID[attrIdx][multiIdx]);
    }
  }
  m_bac->com_bsw_write_ue(&m_bitStream, ah.sliceID);
  m_bac->com_bsw_write_se(&m_bitStream, ah.reflQPoffset);
  m_bac->com_bsw_write_byte_align(&m_bitStream);
}

void TEncBacTop::encodeGeomTreeType(UInt geomTreeType) {
  auto p_ctx = &p_aec->p_geometry_ctx_set->ctxGeomTreeType;
  m_bac->biari_encode_symbol_aec(p_aec, geomTreeType, p_ctx);
  m_bitStream.cur = p_aec->p;
}

void TEncBacTop::EligibleKOctreeDepthflag(bool isEligible) {
  auto p_ctx = &p_aec->p_geometry_ctx_set->ctxdepth;
  m_bac->biari_encode_symbol_aec(p_aec, isEligible, p_ctx);
  m_bitStream.cur = p_aec->p;
}

Void TEncBacTop::encodePredTreeResidual(const V3<int32_t>& residual, const V3<int32_t>& cur,
                                        const V3<int32_t>& pred,
                                        const V3<int32_t>& numbits_Lcusize_log2) {
  V3<int32_t> absRes;
  Bool possibleFlag[8] = {true, true, true, true, true, true, true, true};
  const int tempIndex[8][3] = {
    0, 2, 4, 0, 2, 5, 0, 3, 4, 0, 3, 5, 1, 2, 4, 1, 2, 5, 1, 3, 4, 1, 3, 5,
  };
  const int zeroResidual[3][4] = {4, 5, 6, 7, 2, 3, 6, 7, 1, 3, 5, 7};
  int possibleStatus = -1;
  int tempDist3[3];
  int tempDist6[6];
  int oldStatus = 0;

  for (int k = 0; k < 3; k++) {
    absRes[k] = abs(residual[k]);
  }

  int32_t value = 0;
  int32_t valueHalf = 0;
  int32_t valueRemainder = 0;
  int32_t numBits = 0;
  for (int k = 0, ctxIdx = 0; k < 3; k++) {
    const auto res = residual[k];
    const UInt isZero = (res == 0) ? 1 : 0;
    auto p_ctxIsZero = &p_aec->p_geometry_ctx_set->ctxPredTreeIsZero[k];
    m_bac->biari_encode_symbol_aec(p_aec, isZero, p_ctxIsZero);
    if (isZero) {
      possibleFlag[zeroResidual[k][0]] = false;
      possibleFlag[zeroResidual[k][1]] = false;
      possibleFlag[zeroResidual[k][2]] = false;
      possibleFlag[zeroResidual[k][3]] = false;
      continue;
    }
    if (res > 0)
      oldStatus += (1 << (2 - k));

    value = abs(res) - 1;
    valueHalf = value >> 1;
    valueRemainder = value % 2;
    numBits = getNumBits(uint32_t(valueHalf));
    auto p_ctxs = p_aec->p_geometry_ctx_set->ctxPredTreeNumBits[k];
    std::vector<bool> bitbit;
    bitbit.resize(numbits_Lcusize_log2[k]);
    for (int ctxIdx = 0, i = 0; i < numbits_Lcusize_log2[k]; i++) {
      bitbit[i]= (numBits & (1 << i));
      m_bac->biari_encode_symbol_aec(p_aec, bitbit[i], &(p_ctxs[ctxIdx]));
      if (i < 2)
        ctxIdx = (1 << (i + 1)) - 1 + bitbit[i];
      else if (i == 2)
        ctxIdx = 5 + bitbit[2] + (bitbit[1] << 1);
      else
        ctxIdx = 9;
    }
    m_bac->biari_encode_symbol_eq_prob_aec(p_aec, valueRemainder == 0);

    --numBits;
    for (int32_t i = 0; i < numBits; ++i) {
      m_bac->biari_encode_symbol_eq_prob_aec(p_aec, (valueHalf >> i) & 1);
    }
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
    if (residual[i] && (tempDist3[i] >> 31)) {
      QF |= 1 << (2 - i);
    }
  }

  int newStatus;
  for (int I = 0; I < 8; I++) {
    int i = QF ^ I;
    if ((possibleFlag[i])) {
      int dst =
        tempDist6[tempIndex[i][0]] + tempDist6[tempIndex[i][1]] + tempDist6[tempIndex[i][2]];
      if (dst >= distLimit) {
        possibleStatus++;
        if (i == oldStatus)
          newStatus = possibleStatus;
      }
    }
  }

  int bit[3];
  bit[0] = (newStatus >> 2);
  bit[1] = ((newStatus & 2) >> 1);
  bit[2] = (newStatus & 1);
  int sumofStatus = 0;

  for (int i = 0; i < 3; i++) {
    if (sumofStatus + (1 << (2 - i)) <= possibleStatus) {
      auto p_ctxSign = &p_aec->p_geometry_ctx_set->ctxPredTreeSign[i];
      m_bac->biari_encode_symbol_aec(p_aec, bit[i] > 0, p_ctxSign);
      sumofStatus += (bit[i] << (2 - i));
    }
  }
  m_bitStream.cur = p_aec->p;
}

Void TEncBacTop::encodePredTreeNumPtsInLcu(UInt numPtsInLcu) {
  //TODO: optimized the entropy coding
  assert(numPtsInLcu > 0);
  numPtsInLcu--;
  int32_t numBits = getNumBits(uint32_t(numPtsInLcu));

  for (int32_t i = 0; i < 5; ++i) {
    m_bac->biari_encode_symbol_eq_prob_aec(p_aec, (numBits >> i) & 1);
  }

  --numBits;
  for (int32_t i = 0; i < numBits; ++i) {
    m_bac->biari_encode_symbol_eq_prob_aec(p_aec, (numPtsInLcu >> i) & 1);
  }

  m_bitStream.cur = p_aec->p;
}

Int TEncBacTop::getNumBits(UInt num) {
  int numBits = 0;
  while (num) {
    num = num >> 1;
    numBits++;
  }

  return numBits;
}

Void TEncBacTop::encodeOccUsingMemoryChannel(const UInt& occupancyCode,
                                             TComOctreePartitionParams& params,
                                             TComGeomContext& geomCtx,
                                             bool& planarModeEligibleForSlice) {
  UInt* encodedChildNode = geomCtx.ctxChildOccu;
  encodedChildNode[7] = occupancyCode & 0x7f;
  UInt ctx6Parent = geomCtx.ctxParent;
  static const UInt8 adjacentCIdx[8][7] = {
    {7, 14, 21, 28, 35, 42, 49},  {14, 15, 28, 29, 42, 43, 56}, {21, 28, 23, 30, 49, 56, 51},
    {28, 29, 30, 31, 56, 57, 58}, {35, 42, 49, 56, 39, 46, 53}, {42, 43, 56, 57, 46, 47, 60},
    {49, 56, 51, 58, 53, 60, 55}, {56, 57, 58, 59, 60, 61, 62}};
  static const UInt8 minDime2ParentNeiIndex[3] = {3, 5, 6};
  static const UInt maxNumOccupiedBins[8] = {9, 4, 4, 2, 4, 2, 2, 1};

  const UInt occupancySkip = params.occupancySkip;
  int zNeighborOccupancy = 0;
  static const UInt8 zPlanarNodeIndex[5] = {3, 5, 1, 8, 9};
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

  assert(occupancyCode > 0);
  UInt numCodedBins = 0;
  UInt codedOccupancyCode = 0;
  const UInt maxCodedBins = maxNumOccupiedBins[occupancySkip];
  context_t* bit_ctx = NULL;
  UInt16 context = 0;
  int minDimension = params.nodeSizeLog2.minIndex();
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
    Bool bit = !!(occupancyCode & (1 << i));
    if (codedOccupancyCode || (numCodedBins < maxCodedBins - 1)) {
      int planarContext = 0;
      if (preNodePlanarEligible) {
        if ((zNeighborHighNodeNum < 1) && (zNeighborLowNodeNum > 1) && !!(i & 1) &&
            (!(codedOccupancyCode & 0xaa)))
          planarContext = 1;
        if ((zNeighborLowNodeNum < 1) && (zNeighborHighNodeNum > 1) &&
            ((!(i & 1)) && (!(codedOccupancyCode & 0x55))))
          planarContext = 2;
      }
      if (planarContext > 0) {
        context_t* bit_ctx = NULL;
        bit_ctx = p_aec->p_geometry_ctx_set->planarMode;
        m_bac->biari_encode_symbol_aec(p_aec, bit, bit_ctx + planarContext - 1);
   
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
        bit_ctx = p_aec->p_geometry_ctx_set->ctxMemoryChannel;
        context = ctxFromParent * 9 + ctxFromMemory;
        m_bac->biari_encode_symbol_aec(p_aec, bit, bit_ctx + context);
        *memoryVal = ((*memoryVal) << 1) | bit;
      }

    }
    codedOccupancyCode |= (bit << i);
    numCodedBins++;
  }
  m_bitStream.cur = p_aec->p;
}


Void TEncBacTop::encodeOccupancyCode(const UInt& occupancyCode,
                                      const TComOctreePartitionParams& params,
                                      TComGeomContext& geomCtx, bool& planarModeEligibleForSlice,
                                      const UInt contextMode) {
  UInt* encodedChildNode = geomCtx.ctxChildOccu;
  encodedChildNode[7] = occupancyCode & 0x7f;
  UInt ctxParent = geomCtx.ctxParent;
  const UInt* ctxParentAdv1 = geomCtx.ctxParentAdv1;
  const UInt* ctxChildD1 = geomCtx.ctxChildD1;
  UInt8 ctxChildNode[8] = {0};
  const UInt occupancySkip = params.occupancySkip;
  for (Int i = 0; i < 8; i++) {
    for (Int j = 0; j < 3; j++) {
      ctxChildNode[i] |= (!!(ctxChildD1[j] & (1 << i))) << j;
    }
  }
  static const UInt8 adjacentIdx[8][3] = {{1, 2, 4}, {0, 3, 5}, {0, 3, 6}, {1, 2, 7},
                                          {0, 5, 6}, {1, 4, 7}, {2, 4, 7}, {3, 5, 6}};
  static const UInt8 adjacentPosIdx[8][3] = {{2, 1, 0}, {5, 1, 0}, {4, 2, 0}, {4, 5, 0},
                                             {3, 2, 1}, {3, 5, 1}, {3, 4, 2}, {3, 4, 5}};

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

  assert(occupancyCode > 0);
  UInt numCodedBins = 0;
  UInt codedOccupancyCode = 0;
  const UInt maxCodedBins = maxNumOccupiedBins[occupancySkip];

  UInt16 childInformation = 0;
  UInt8 context = 0;
  int minDimension = params.nodeSizeLog2.minIndex();
  for (Int i = 0; i < 8; i++) {
    if (occupancySkip != 0) {
      if ((occupancySkip & 1) && (i & 1))  ///< skip when z = 1
        continue;
      if ((occupancySkip & 2) && (i & 2))  ///< skip when y = 1
        continue;
      if ((occupancySkip & 4) && (i & 4))  ///< skip when x = 1
        continue;
    }

    Bool bit = !!(occupancyCode & (1 << i));
    if (codedOccupancyCode || (numCodedBins < maxCodedBins - 1)) {
      UInt16 ctxChild = 0;
      UInt8 ctxm = 0;

      context_t* bit_ctx = NULL;
      int planarContext = 0;
      if (preNodePlanarEligible) {
        if ((zNeighborHighNodeNum < 1) && (zNeighborLowNodeNum > 1) &&
            !!(i & 1) &&  //预测为低且不在低平面
            (!(codedOccupancyCode & 0xaa)))
          planarContext = 1;
        if ((zNeighborLowNodeNum < 1) && (zNeighborHighNodeNum > 1) &&
            ((!(i & 1)) && (!(codedOccupancyCode & 0x55))))
          planarContext = 2;
      }
      if (planarContext > 0) {
        bit_ctx = p_aec->p_geometry_ctx_set->planarMode;
        context = planarContext - 1;

        m_bac->biari_encode_symbol_aec(p_aec, bit, bit_ctx + context);
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
          bit_ctx = p_aec->p_geometry_ctx_set->ctx_occupancyCombinechild[ctxParentAdv1[i]];

          context = ctxChildNode[i] - 1;
          m_bac->biari_encode_symbol_aec(p_aec, bit, bit_ctx + context);

        } else if (ctxChild) {
          bit_ctx = p_aec->p_geometry_ctx_set->ctx_occupancyCombineParent1[ctxParentAdv1[i]];

          childInformation = ctxChild;
          uint8_t* memoryVal = memoryChannel;
          memoryVal += childInformation;
          UInt8 ctxFromMemory = popcnt8(*memoryVal);
          context = ctxFromMemory;

          *memoryVal = ((*memoryVal) << 1) | bit;
          m_bac->biari_encode_symbol_aec(p_aec, bit, bit_ctx + context);

        } else
        {
          bit_ctx = p_aec->p_geometry_ctx_set->ctxRUB_occupancy[i];
          context = ctxParentAdv1[i];
          m_bac->biari_encode_symbol_aec(p_aec, bit, bit_ctx + context);
        }
      }
    }
    codedOccupancyCode |= (bit << i);
    numCodedBins++;
    if (bit) {
      for (Int k = 0; k < 3; k++) {
        assert(!((ctxChildNode[adjacentIdx[i][k]] >> adjacentPosIdx[i][k]) & 1));
        ctxChildNode[adjacentIdx[i][k]] |= 1 << adjacentPosIdx[i][k];
      }
    }
  }

  m_bitStream.cur = p_aec->p;
}



Void TEncBacTop::encodeSinglePointFlag(Bool singleModeFlag) {
  m_bac->biari_encode_symbol_aec(p_aec, singleModeFlag,
                                 &p_aec->p_geometry_ctx_set->ctx_geom_single_mode_flag);
}

Void TEncBacTop::encodeSinglePointIndex(const V3<UInt>& pos, V3<UInt> nodeSizeLog2) {
  while (nodeSizeLog2 > 0) {
    for (Int i = 0; i < 3; ++i) {
      if (nodeSizeLog2[i] > 0) {
        const auto bitMask = 1 << (nodeSizeLog2[i] - 1);
        m_bac->biari_encode_symbol_eq_prob_aec(p_aec, !!(pos[i] & bitMask));
        --nodeSizeLog2[i];
      }
    }
  }
  m_bitStream.cur = p_aec->p;
}

Void TEncBacTop::encodeDuplicateNumber(const UInt& dupNum) {
  assert(dupNum >= 1);
  auto p_ctx = &p_aec->p_geometry_ctx_set->ctx_geom_num_dup_eq1;
  m_bac->biari_encode_symbol_aec(p_aec, dupNum == 1, p_ctx);
  if (dupNum > 1)
    m_bac->sbac_write_ue_ep(&m_bitStream, dupNum - 2, p_aec);
}

Void TEncBacTop::encodeRunlength(int32_t& length) {
  const bool isZero = length == 0;
  m_bac->biari_encode_symbol_aec(p_aec, isZero, &p_aec->p_attribute_ctx_set->ctx_attr_length_eq0);
  if (isZero)
    return;
  encodeExpGolombRunlength(length - 1, 2, p_aec->p_attribute_ctx_set->ctx_length_prefix,
                           p_aec->p_attribute_ctx_set->ctx_length_suffix);

  m_bitStream.cur = p_aec->p;
}

Void TEncBacTop::encodeExpGolombRunlength(unsigned int symbol, int k, context_t* p_ctxPrefix,
                                          context_t* p_ctxSufffix) {
  const int k0 = k;
  while (1) {
    if (symbol >= (1u << k)) {
      m_bac->biari_encode_symbol_aec(p_aec, 1, &p_ctxPrefix[std::min(k - k0, 2)]);
      symbol -= (1u << k);
      k++;
    } else {
      m_bac->biari_encode_symbol_aec(p_aec, 0, &p_ctxPrefix[std::min(k - k0, 2)]);
      while (k--)
        m_bac->biari_encode_symbol_aec(p_aec, (symbol >> k) & 1, &p_ctxSufffix[std::min(k, 1)]);
      break;
    }
  }
}


Void TEncBacTop::encodeExpGolomb(unsigned int symbol, int k, context_t* p_ctxPrefix,
                                 context_t* p_ctxSufffix) {
  while (1) {
    if (symbol >= (1u << k)) {
      m_bac->biari_encode_symbol_aec(p_aec, 1, p_ctxPrefix);
      symbol -= (1u << k);
      k++;
    } else {
      m_bac->biari_encode_symbol_aec(p_aec, 0, p_ctxPrefix);
      while (k--)
        m_bac->biari_encode_symbol_aec(p_aec, (symbol >> k) & 1, p_ctxSufffix);
      break;
    }
  }
}

Void TEncBacTop::encodeExpGolombN(unsigned int symbol, int k, context_t* p_ctxPrefix,
    context_t* p_ctxSufffix) {
    int count = 4;
    while (1) {
        if (symbol >= (1u << k)) {
            if (count) {
                m_bac->biari_encode_symbol_aec(p_aec, 1, p_ctxPrefix);
                count--;
            }
            else {
                m_bac->biari_encode_symbol_eq_prob_aec(p_aec, 1);
            }
            symbol -= (1u << k);
            k++;
        }
        else {
            if (count) {
                m_bac->biari_encode_symbol_aec(p_aec, 0, p_ctxPrefix);
                count--;
            }
            else {
                m_bac->biari_encode_symbol_eq_prob_aec(p_aec, 0);
            }
            while (k--) {
                if (count) {
                    m_bac->biari_encode_symbol_aec(p_aec, (symbol >> k) & 1, p_ctxSufffix);
                    count--;
                }
                else {
                    m_bac->biari_encode_symbol_eq_prob_aec(p_aec, (symbol >> k) & 1);
                }
            }
            break;
        }
    }
}

Void TEncBacTop::encodeExpGolombAdp(const int val, const int k, const int colorType, const int ctx_id) {
    golombK[colorType] = golombK[colorType] > 0 ? golombK[colorType] : 1;
    encodeExpGolombN(val, golombK[colorType],
        &p_aec->p_attribute_ctx_set->ctx_attr_residual_prefix[ctx_id],
        &p_aec->p_attribute_ctx_set->ctx_attr_residual_suffix[ctx_id]);
    if (ExpGolombInputGroup[colorType].size() < Group_size) {
        inputGroupSum[colorType] += val;
        ExpGolombInputGroup[colorType].push_back(val);
    }
    else {
        auto valFirst = ExpGolombInputGroup[colorType].begin();
        inputGroupSum[colorType] -= *valFirst;
        ExpGolombInputGroup[colorType].erase(valFirst);
        inputGroupSum[colorType] += val;
        ExpGolombInputGroup[colorType].push_back(val);
        int64_t inputGroupAvg = inputGroupSum[colorType] >> Group_size_shift;
        golombK[colorType] = k;
        if (inputGroupAvg < golombkForValLower)
            golombK[colorType]--;
        else if (inputGroupAvg > golombkForValUpper)
            golombK[colorType]++;
    }
}

Void TEncBacTop::encodeExpGolombRefl(unsigned int symbol, int k, context_t* p_ctxPrefix,
                                     context_t* p_ctxSufffix) {
  int k0 = k;
  int kmax = k;
  while (1) {
    if (symbol >= (1u << k)) {
      if (k == k0) {
        m_bac->biari_encode_symbol_aec(p_aec, 1, &p_ctxPrefix[0]);
      } else if (k == k0 + 1) {
        m_bac->biari_encode_symbol_aec(p_aec, 1, &p_ctxPrefix[1]);
      } else {
        m_bac->biari_encode_symbol_aec(p_aec, 1, &p_ctxPrefix[2]);
      }
      symbol -= (1u << k);
      k++;
      kmax = k;
    } else {
      if (k == k0) {
        m_bac->biari_encode_symbol_aec(p_aec, 0, &p_ctxPrefix[0]);
      } else if (k == k0 + 1) {
        m_bac->biari_encode_symbol_aec(p_aec, 0, &p_ctxPrefix[1]);
      } else {
        m_bac->biari_encode_symbol_aec(p_aec, 0, &p_ctxPrefix[2]);
      }
      while (k--) {
        if (k == kmax - 1) {
          m_bac->biari_encode_symbol_aec(p_aec, (symbol >> k) & 1, &p_ctxSufffix[0]);
        } else if (k == kmax - 2) {
          m_bac->biari_encode_symbol_aec(p_aec, (symbol >> k) & 1, &p_ctxSufffix[1]);
        } else {
          m_bac->biari_encode_symbol_aec(p_aec, (symbol >> k) & 1, &p_ctxSufffix[2]);
        }
      }
      break;
    }
  }
}

Void TEncBacTop::encodeExpGolombReflN(UInt64 symbol, int k, context_t* p_ctxPrefix,
                                        context_t* p_ctxSufffix) {
  int k0 = k;
  int kmax = k;
  int count = 4;
  while (1) {
    if (symbol >= (1LL << k)) {
      if (count) {
        if (k == k0) {
          m_bac->biari_encode_symbol_aec(p_aec, 1, &p_ctxPrefix[0]);
        } else if (k == k0 + 1) {
          m_bac->biari_encode_symbol_aec(p_aec, 1, &p_ctxPrefix[1]);
        } else {
          m_bac->biari_encode_symbol_aec(p_aec, 1, &p_ctxPrefix[2]);
        }
        count--;
      } else {
        m_bac->biari_encode_symbol_eq_prob_aec(p_aec, 1);
      }
      symbol -= (1LL << k);
      k++;
      kmax = k;
    } else {
      if (count) {
        if (k == k0) {
          m_bac->biari_encode_symbol_aec(p_aec, 0, &p_ctxPrefix[0]);
        } else if (k == k0 + 1) {
          m_bac->biari_encode_symbol_aec(p_aec, 0, &p_ctxPrefix[1]);
        } else {
          m_bac->biari_encode_symbol_aec(p_aec, 0, &p_ctxPrefix[2]);
        }
        count--;
      } else {
        m_bac->biari_encode_symbol_eq_prob_aec(p_aec, 0);
      }
      while (k--) {
        if (count) {
          if (k == kmax - 1) {
            m_bac->biari_encode_symbol_aec(p_aec, (symbol >> k) & 1, &p_ctxSufffix[0]);
          } else if (k == kmax - 2) {
            m_bac->biari_encode_symbol_aec(p_aec, (symbol >> k) & 1, &p_ctxSufffix[1]);
          } else {
            m_bac->biari_encode_symbol_aec(p_aec, (symbol >> k) & 1, &p_ctxSufffix[2]);
          }
          count--;
        } else {
          m_bac->biari_encode_symbol_eq_prob_aec(p_aec, (symbol >> k) & 1);
        }
      }
      break;
    }
  }
}

Void TEncBacTop::setGolombGroupSize(const UInt& groupSizeShift) {
  Group_size = 1 << groupSizeShift;
  Group_size_shift = groupSizeShift;
}

Void TEncBacTop::setColorGolombKandBound(const UInt& GolombNum) {
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
}


Void TEncBacTop::codeAttributeResidualDual(const int64_t& delta, const bool& isColor,
                                           const int ctx_id, const bool isDuplicatePoint,
                                           const bool residualminusone_flag,
                                           const UInt8& golombNUm) {
  int sign_bit = (delta > 0) ? 1 : 0;
  int abs_delta = (delta > 0) ? delta : -delta;
  int parity = (abs_delta - 1) % 2;
  if (!isDuplicatePoint) {
    m_bac->biari_encode_symbol_eq_prob_aec(p_aec, sign_bit);
  }
  m_bac->biari_encode_symbol_aec(p_aec, parity, &p_aec->p_attribute_ctx_set_dual->parity[ctx_id]);
  m_bac->biari_encode_symbol_aec(p_aec, abs_delta > 2,
                                 &p_aec->p_attribute_ctx_set_dual->ctx_attr_residual_flag1[ctx_id]);
  if (abs_delta > 2) {
    m_bac->biari_encode_symbol_aec(p_aec, abs_delta > 4, &p_aec->p_attribute_ctx_set_dual->ctx_attr_residual_flag2[ctx_id]);
    if (abs_delta > 4) {
      int val = (abs_delta - 5) >> 1;  // 2, 3,4,5, 6, 7...-->0,0,1,1,2,2,...
      encodeExpGolombReflN(val, golombNUm,
                          p_aec->p_attribute_ctx_set_dual->ctx_attr_residual_prefix,
                          p_aec->p_attribute_ctx_set_dual->ctx_attr_residual_suffix);
    }
  }
  m_bitStream.cur = p_aec->p;
}


Void TEncBacTop::codeAttributerResidual(const int64_t& delta, const bool& isColor,
                                        const int& colorType, const int ctx_id,
                                        const bool isDuplicatePoint,
                                        const bool residualminusone_flag, const UInt8& golombNum,
                                        const int b0) {
  const int ExpGolombNumber = golombNum;
   if (!isColor) {
    int sign_bit = (delta > 0) ? 1 : 0;
    UInt64 abs_delta = (delta > 0) ? delta : -delta;
    int parity = (abs_delta - 1) % 2;
    if (!isDuplicatePoint) {
      m_bac->biari_encode_symbol_eq_prob_aec(p_aec, sign_bit);
    }
    m_bac->biari_encode_symbol_aec(p_aec, parity, &p_aec->p_attribute_ctx_set->parity[ctx_id]);
    m_bac->biari_encode_symbol_aec(p_aec, abs_delta > 2,
                                   &p_aec->p_attribute_ctx_set->ctx_attr_residual_flag1[ctx_id]);
    if (abs_delta > 2) {
      m_bac->biari_encode_symbol_aec(p_aec, abs_delta > 4,
                                     &p_aec->p_attribute_ctx_set->ctx_attr_residual_flag2[ctx_id]);
      if (abs_delta > 4) {
        UInt64 val = (abs_delta - 5) >> 1;  // 2, 3,4,5, 6, 7...-->0,0,1,1,2,2,...
          encodeExpGolombReflN(val, ExpGolombNumber,
                              p_aec->p_attribute_ctx_set->ctx_attr_residual_prefix,
                              p_aec->p_attribute_ctx_set->ctx_attr_residual_suffix);
      }
    }
    m_bitStream.cur = p_aec->p;

  } 
   else {
    if (!residualminusone_flag) {
      //m_residualStats.collectStats(abs(delta));
      if (b0 == 1) {
        if (isDuplicatePoint == 0) {
          m_bac->biari_encode_symbol_aec(p_aec, delta == 0,
                                         &p_aec->p_attribute_ctx_set->ctx_attr_residual_eq0[6]);

        } else {
          m_bac->biari_encode_symbol_aec(p_aec, delta == 0,
                                         &p_aec->p_attribute_ctx_set->ctx_attr_residual_eq0[7]);
        }
      } else {
        m_bac->biari_encode_symbol_aec(
          p_aec, delta == 0,
          &p_aec->p_attribute_ctx_set->ctx_attr_residual_eq0[ctx_id + (3 * isDuplicatePoint)]);
      }

      if (delta != 0) {
        int abs_delta = (delta > 0) ? delta : -delta;
        int parity = abs_delta % 2;

        m_bac->biari_encode_symbol_aec(p_aec, abs_delta > 1, &p_aec->p_attribute_ctx_set->ctx_attr_residual_flag1[ctx_id + b0]);
        if (abs_delta > 1) {
          m_bac->biari_encode_symbol_aec(p_aec, parity,
                                         &p_aec->p_attribute_ctx_set->parity[ctx_id + b0]);
          m_bac->biari_encode_symbol_aec(p_aec, abs_delta > 3,
            &p_aec->p_attribute_ctx_set->ctx_attr_residual_flag2[ctx_id + b0]);
          if (abs_delta > 3) {
            int val = (abs_delta - 4) >> 1;
            encodeExpGolombAdp(val, ExpGolombNumber, colorType, ctx_id);
          }
        }
      }
      m_bitStream.cur = p_aec->p;
    } else {
      //m_residualStats.collectStats(abs(delta));
      int abs_delta = (delta > 0) ? delta : -delta;
      abs_delta = abs_delta - 1;
      int parity = abs_delta % 2;
      m_bac->biari_encode_symbol_aec(
        p_aec, abs_delta == 0,
        &p_aec->p_attribute_ctx_set
           ->ctx_attr_residual_minusone_eq0[ctx_id + (3 * isDuplicatePoint)]);
      if (abs_delta != 0) {
        m_bac->biari_encode_symbol_aec(p_aec, abs_delta > 1,
          &p_aec->p_attribute_ctx_set->ctx_attr_residual_minusone_flag1[ctx_id]);
        if (abs_delta > 1) {
          m_bac->biari_encode_symbol_aec(p_aec, parity,
                                         &p_aec->p_attribute_ctx_set->parity[ctx_id]);
          m_bac->biari_encode_symbol_aec(
            p_aec, abs_delta > 3,
            &p_aec->p_attribute_ctx_set->ctx_attr_residual_minusone_flag2[ctx_id]);
          if (abs_delta > 3) {
            int val = (abs_delta - 4) >> 1;
            encodeExpGolombAdp(val, ExpGolombNumber, colorType, ctx_id);
          }
        }
      }
      m_bitStream.cur = p_aec->p;
    }
  }
}

Void TEncBacTop::codeSign(const int64_t delta) {
  if (delta != 0) {
    int sign_bit = (delta > 0) ? 1 : 0;
    m_bac->biari_encode_symbol_eq_prob_aec(p_aec, sign_bit);
  }
}

Void TEncBacTop::codeAttributerResidualHaar(const int64_t& delta, const bool& isColor,
                                            const int& colorType, const bool reslayer,
                                            const int ctx_id,
                                            const bool isDuplicatePoint,
                                            const bool residualminusone_flag,
                                            const UInt8& golombNUm) {
  if (!isColor) {
    int sign_bit = (delta > 0) ? 1 : 0;
    int abs_delta = (delta > 0) ? delta : -delta;
    if (!isDuplicatePoint) {
      m_bac->biari_encode_symbol_eq_prob_aec(p_aec, sign_bit);
    }

    m_bac->biari_encode_symbol_aec(p_aec, abs_delta == 1,
                                   &p_aec->p_attribute_ctx_set->ctx_attr_residual_flag1[ctx_id]);
    if (abs_delta > 1) {
      m_bac->biari_encode_symbol_aec(p_aec, abs_delta == 2,
                                     &p_aec->p_attribute_ctx_set->ctx_attr_residual_flag2[ctx_id]);
      if (abs_delta > 2) {
        if (reslayer) {  
          //encodeExpGolomb(abs_delta - 3, golombNUm,
          //                &p_aec->p_ctx_set->ctx_attr_residual_prefix[ctx_id],
           //               &p_aec->p_ctx_set->ctx_attr_residual_suffix[ctx_id]);
          encodeExpGolombReflN(abs_delta - 3, golombNUm,
                              p_aec->p_attribute_ctx_set->ctx_attr_residual_prefix,
                              p_aec->p_attribute_ctx_set->ctx_attr_residual_suffix);
        } else {
          unsigned int val = abs_delta - 3;
          //encodeExpGolomb(val, golombK[colorType],
          //                &p_aec->p_ctx_set->ctx_attr_residual_prefix[ctx_id],
          //                &p_aec->p_ctx_set->ctx_attr_residual_suffix[ctx_id]);
          encodeExpGolombReflN(val, golombNUm,
                              p_aec->p_attribute_ctx_set->ctx_attr_residual_prefix,
                              p_aec->p_attribute_ctx_set->ctx_attr_residual_suffix);
        }
      }
    }
    m_bitStream.cur = p_aec->p;

  } else {
    if (!residualminusone_flag) {
      //m_residualStats.collectStats(abs(delta));
      m_bac->biari_encode_symbol_aec(
        p_aec, delta == 0,
        &p_aec->p_attribute_ctx_set->ctx_attr_residual_eq0[ctx_id + (3 * isDuplicatePoint)]);

      if (delta != 0) {
        int abs_delta = (delta > 0) ? delta : -delta;

        m_bac->biari_encode_symbol_aec(p_aec, abs_delta == 1, &p_aec->p_attribute_ctx_set->ctx_attr_residual_flag1[ctx_id]);
        if (abs_delta > 1) {
          m_bac->biari_encode_symbol_aec(p_aec, abs_delta == 2, &p_aec->p_attribute_ctx_set->ctx_attr_residual_flag2[ctx_id]);
          if (abs_delta > 2) {
            if (reslayer) {
              encodeExpGolombN(abs_delta - 3, golombNUm,
                              &p_aec->p_attribute_ctx_set->ctx_attr_residual_prefix[ctx_id],
                              &p_aec->p_attribute_ctx_set->ctx_attr_residual_suffix[ctx_id]);
            } else {
              unsigned int val = abs_delta - 3;
              encodeExpGolombAdp(val, golombNUm, colorType, ctx_id);
            }
          }
        }
      }
      m_bitStream.cur = p_aec->p;
    } else {
      //m_residualStats.collectStats(abs(delta));
      int abs_delta = (delta > 0) ? delta : -delta;
      abs_delta = abs_delta - 1;
      m_bac->biari_encode_symbol_aec(
        p_aec, abs_delta == 0,
        &p_aec->p_attribute_ctx_set
           ->ctx_attr_residual_minusone_eq0[ctx_id + (3 * isDuplicatePoint)]);
      if (abs_delta != 0) {
        m_bac->biari_encode_symbol_aec(p_aec, abs_delta == 1,
          &p_aec->p_attribute_ctx_set->ctx_attr_residual_minusone_flag1[ctx_id]);
        if (abs_delta > 1) {
          m_bac->biari_encode_symbol_aec(
            p_aec, abs_delta == 2,
            &p_aec->p_attribute_ctx_set->ctx_attr_residual_minusone_flag2[ctx_id]);
          if (abs_delta > 2) {
            if (reslayer) {
              encodeExpGolombN(abs_delta - 3, golombNUm,
                              &p_aec->p_attribute_ctx_set->ctx_attr_residual_prefix[ctx_id],
                              &p_aec->p_attribute_ctx_set->ctx_attr_residual_suffix[ctx_id]);
            } else {
              unsigned int val = abs_delta - 3;
              encodeExpGolombAdp(val, golombNUm, colorType, ctx_id);
            }
          }
        }
      }
      m_bitStream.cur = p_aec->p;
    }
  }
}

Void TEncBacTop::codeAttributerResidualequaltwo0(const int64_t& delta) {
  m_bac->biari_encode_symbol_aec(p_aec, delta == 0, &p_aec->p_attribute_ctx_set->ctx_attr_flag2);
  m_bitStream.cur = p_aec->p;
}

Void TEncBacTop::codeAttributerResidualequalone0(const int64_t& delta) {
  m_bac->biari_encode_symbol_aec(p_aec, delta == 0, &p_aec->p_attribute_ctx_set->ctx_attr_flag1);
  m_bitStream.cur = p_aec->p;
}

Void TEncBacTop::codeAttributerResidualequal0(const int64_t& delta) {
  m_bac->biari_encode_symbol_aec(p_aec, delta == 0, &p_aec->p_attribute_ctx_set->ctx_attr_flag4);
  m_bitStream.cur = p_aec->p;
}

Void TEncBacTop::encodeCoeffNumber(const UInt64 CoeffNum) {
  //m_bac->com_bsw_write_ue(&m_bitStream, CoeffNum);
  m_bac->sbac_write_ue_ep(&m_bitStream, CoeffNum, p_aec);
};

Void TEncBacTop::encodeTerminationFlag(Bool terminateFlag) {
  m_bac->biari_encode_symbol_final_aec(p_aec, terminateFlag ? 1 : 0);
  m_bac->aec_done(p_aec);

  m_bitStream.cur = p_aec->p;
}

Void TEncBacTop::computeAttributeID(AttributeBrickHeader& abh, const AttributeParameterSet& aps,
                                    const SequenceParameterSet& sps) {
  for (int attrIdx = 0; attrIdx < (sps.maxNumAttrMinus1 + 1); attrIdx++) {
    if (aps.attributePresentFlag[attrIdx]) {
      for (int multiIdx = 0; multiIdx < aps.attribute_num_set_minus1[attrIdx] + 1; ++multiIdx)
        abh.attribute_ID[attrIdx][multiIdx] = multiIdx;
    }
  }
}

UInt8* TEncBacTop::getBitStreamCur() {
  return m_bitStream.cur;
}
Void TEncBacTop::encodeFinish() {
  if (m_bac)
    m_bac->enc_sbac_finish(&m_bitStream);

  m_bac->Demulate(&m_bitStream);

  m_bac->com_bsw_deinit(&m_bitStream);  ///< flush the buffer
}

TEncBacTop::TEncBacTop() {
  m_bac = nullptr;
  reset();
}

TEncBacTop::~TEncBacTop() {
  if (m_bac) {
    delete m_bac;
    m_bac = nullptr;
  }
}

Void TEncBacTop::setBitstreamBuffer(TComBufferChunk& buffer, const bool& initDulatAttribute) {
  if (buffer.getBufferType() == BufferChunkType::BCT_GEOM ||
      buffer.getBufferType() == BufferChunkType::BCT_ATTR ||
      buffer.getBufferType() == BufferChunkType::BCT_COL ||
      buffer.getBufferType() == BufferChunkType::BCT_REFL)
    buffer.allocateBuffSize(buffersize);
  else
    buffer.allocateBuffSize(MAX_HEADER_BS_BUF);

  m_bac->com_bsw_init(&m_bitStream, (UInt8*)buffer.addr, (UInt8*)buffer.addr2, buffer.bsize, NULL);
  if (buffer.getBufferType() == BufferChunkType::BCT_GEOM) {
    m_bac->init_geometry_contexts(p_aec);
    m_bac->aec_start(p_aec, m_bitStream.beg, m_bitStream.end, 1);
  } else if (buffer.getBufferType() == BufferChunkType::BCT_ATTR ||
             buffer.getBufferType() == BufferChunkType::BCT_COL ||
             buffer.getBufferType() == BufferChunkType::BCT_REFL) {
    m_bac->init_attribute_contexts(p_aec, initDulatAttribute);
    m_bac->aec_start(p_aec, m_bitStream.beg, m_bitStream.end, 1);
  }
}

Void TEncBacTop::initBac() {
  m_bac = new TEncBacCore;
  m_bac->enc_sbac_init();
}

Void TEncBacTop::reset() {
  if (m_bac) {
    delete m_bac;
    m_bac = nullptr;
  }
  m_bitStream.init();
  aec.init();
  p_aec = &aec;
  std::fill(begin(memoryChannel), end(memoryChannel), 15);
}
Void TEncBacTop::LcuReset() {
  std::fill(begin(memoryChannel), end(memoryChannel), 15);
  m_bac->init_geometry_contexts(p_aec);
}
UInt64 TEncBacTop::getBitStreamLength() {
  return m_bitStream.cur - m_bitStream.beg;
}

///< \{
