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

#include "TEncTop.h"
#include "common/CommonDef.h"
#include "common/FXPoint.h"
#include "common/MD5Sum.h"
#include "common/TComBufferChunk.h"
#include "common/TComPointCloud.h"
#include "common/contributors.h"
#include <fstream>
#include <map>
#include <math.h>
#include <set>
#include <time.h>

using namespace std;

///< \in TLibEncoder \{
/**
 * Implementation of TEncTop
 * encoder class
 */

//////////////////////////////////////////////////////////////////////////
// Public class functions
//////////////////////////////////////////////////////////////////////////

Int TEncTop::encode() {
  Int numDigits = getfileNumLength();  // numDigit indicates digits of file number
  setStartFrameNum(numDigits);         // set start frame name from m_inputFileName

  Int exitState = EXIT_SUCCESS;

  MD5Sum md5Calculator;
  bool md5FileOpenFlag = md5Calculator.openOStream(m_MD5FileName);

  ///Pc_evalue
  pc_evalue::TMetricRes totalMetricRes;
  pc_evalue::TMetricCfg MetricParam;
  MetricParam.m_calLossless = m_calLossless;
  MetricParam.m_calColor = m_calColor;
  MetricParam.m_calReflectance = m_calReflectance;
  MetricParam.m_peakValue = m_peakValue;
  MetricParam.m_reflOutputDepth = m_hls.aps.reflOutputDepth;
  MetricParam.m_symmetry = m_symmetry;
  MetricParam.m_duplicateMode = m_duplicateMode;
  MetricParam.m_multiNeighbourMode = m_multiNeighbourMode;
  MetricParam.m_showHausdorff = m_showHausdorff;

  EncoderStatistics esTotal;

  for (UInt i_frame = 0; i_frame < m_numOfFrames; i_frame++) {
    m_frame_ID = i_frame;

    EncoderStatistics esCurFrame;
    printFrameCutoffRule(i_frame, numDigits);

    if (i_frame == 0) {
      resetBitstream_Recon_FileName(m_startFrame,
                                    numDigits);  // reset m_bitstreamFileName & m_reconFileName
    } else {
      updateIOFileName(
        m_startFrame + i_frame,
        numDigits);  // update m_inputFileName & m_bitstreamFileName & m_reconFileName
    }

    if (!m_pointCloudOrg.readFromFile(m_inputFileName, !m_hls.sps.attrPresentFlag) ||
        m_pointCloudOrg.getNumPoint() == 0) {
      cerr << "Error: failed to open ply file: " << m_inputFileName << " !" << endl;
      exitState |= EXIT_FAILURE;
      continue;  // if can not open the corresponding file then skip and move on to find next input file
    }
    m_bitstreamFile.open(m_bitstreamFileName, fstream::binary | fstream::out);
    if (!m_bitstreamFile) {
      cerr << "Error: failed to open bitstream file " << m_bitstreamFileName << " for writing!"
           << endl;
      exitState |= EXIT_FAILURE;
      continue;  // if can not open the corresponding file then skip and move on to find next input file
    }

    clock_t userTimeTotalBegin = clock();

    ///< preprocessing
    vector<PC_COL> pointCloudOrgColors = m_pointCloudOrg.getColors();
    if (m_colorTransformFlag && m_pointCloudOrg.hasColors()) {
      m_pointCloudOrg.convertRGBToYUV();
    }
    ///< determine bounding box
    PC_POS bbMin, bbMax, bbSize;
    m_pointCloudOrg.computeBoundingBox(bbMin, bbMax);
    for (Int k = 0; k < 3; k++) {
      bbMin[k] = floor(bbMin[k]);
      m_hls.sps.geomBoundingBoxOrigin[k] = Int(bbMin[k]);
    }
    bbSize = bbMax - bbMin;
    m_hls.sps.geomBoundingBoxSize[0] = Int(round(bbSize[0] / m_hls.sps.geomQuantStep)) + 1;
    m_hls.sps.geomBoundingBoxSize[1] = Int(round(bbSize[1] / m_hls.sps.geomQuantStep)) + 1;
    m_hls.sps.geomBoundingBoxSize[2] = Int(round(bbSize[2] / m_hls.sps.geomQuantStep)) + 1;
    m_hls.sps.sps_multi_set_flag = true;
    m_hls.frameHead.geomNumPoints = (UInt)m_pointCloudOrg.getNumPoint();
    ///< quantization
    geomPreprocessAndQuantization(m_hls.frameHead.geomNumPoints, m_hls.sps.geomQuantStep,
                                  esCurFrame.recolorUserTime);

    ///< determine if only geometry
    if (!m_pointCloudOrg.hasColors() && !m_pointCloudOrg.hasReflectances()) {
      m_hls.sps.attrPresentFlag = 0;
    }

    initParameters();
    init_aec_context_tab();
    m_encBac.computeBufferSize(m_pointCloudOrg.getNumPoint());

    ///< encoding sequence parameter set
    m_bufferChunk.setBufferType(BufferChunkType::BCT_SPS);
    m_encBac.setBitstreamBuffer(m_bufferChunk);
    m_encBac.codeSPS(m_hls.sps);
    m_encBac.encodeFinish();
    m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
    m_bufferChunk.reset();
    m_encBac.reset();

    ///< when lcuNodeSizeLog2 == 0, lcuNodeDepth is used to control the node size
    if (m_hls.gps.lcuNodeSizeLog2 == 0 && m_hls.gps.lcuNodeDepth > 0) {
      UInt maxBB = std::max({1U, m_hls.sps.geomBoundingBoxSize[0], m_hls.sps.geomBoundingBoxSize[1],
                             m_hls.sps.geomBoundingBoxSize[2]});
      int maxNodeSizeLog2 = ceilLog2(maxBB);

      //avoid small lcu size
      m_hls.gps.lcuNodeSizeLog2 = std::max(10U, maxNodeSizeLog2 + 1 - m_hls.gps.lcuNodeDepth);
    }

    ///< recolor
    if (m_hls.sps.geomRemoveDuplicateFlag && m_hls.sps.recolorMode == 0) {
      clock_t userTimeRecolorBegin = clock();
      recolour(m_pointCloudOrg, float(1.0 / m_hls.sps.geomQuantStep),
               m_hls.sps.geomBoundingBoxOrigin, &m_pointCloudRecon);
      esCurFrame.recolorUserTime += (Double)(clock() - userTimeRecolorBegin) / CLOCKS_PER_SEC;
    }

    ///< encoding geometry parameter set
    m_bufferChunk.setBufferType(BufferChunkType::BCT_GPS);
    m_encBac.setBitstreamBuffer(m_bufferChunk);
    m_encBac.codeGPS(m_hls.gps);
    m_encBac.encodeFinish();
    m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
    m_bufferChunk.reset();
    m_encBac.reset();

    ///< encoding attribute parameter set
    if (m_hls.sps.attrPresentFlag) {
      auto& aps = m_hls.aps;
      if (m_pointCloudOrg.hasColors()) {
        aps.attributePresentFlag[0] = 1;
        aps.outputBitDepthMinus1[0] = m_hls.aps.colorOutputDepth - 1;
        aps.maxNumOfNeighboursLog2Minus7[0] = 0;
        
        UInt colorInitOutputDepth = aps.colorOutputDepth - aps.colorInitGolombOffset;
        m_pointCloudOrg.computeColorRes(aps.colorGolombNum, colorInitOutputDepth,
                                        m_hls.sps.colorQuantParam);
        // adjust colorInitPredTransRatio
        UInt64 BBmax =
          std::max(std::max(m_hls.sps.geomBoundingBoxSize[0], m_hls.sps.geomBoundingBoxSize[1]),
                   m_hls.sps.geomBoundingBoxSize[2]);
        UInt64 BBmin =
          std::min(std::min(m_hls.sps.geomBoundingBoxSize[0], m_hls.sps.geomBoundingBoxSize[1]),
                   m_hls.sps.geomBoundingBoxSize[2]);
        if (BBmax / BBmin < 2)
          aps.colorInitPredTransRatio++;
        // calculate scalar of adjust color QP per point tools
        if (aps.colorQPAdjustFlag) {
          int ratio = m_pointCloudOrg.getNumPoint() / m_pointCloudQuant.getNumPoint();
          aps.colorQPAdjustScalar = ratio * 2;
          aps.colorQPAdjustScalar *= aps.colorQPAdjustScalar;
        }
      }
      if (m_pointCloudOrg.hasReflectances()) {
        aps.attributePresentFlag[1] = 1;
        aps.outputBitDepthMinus1[1] = m_hls.aps.reflOutputDepth - 1;
        aps.maxNumOfNeighboursLog2Minus7[1] = 0;
        UInt reflInitOutputDepth = aps.reflOutputDepth - aps.reflInitGolombOffset;
        m_pointCloudOrg.computeReflRes(aps.refGolombNum, reflInitOutputDepth,
                                       m_hls.sps.reflQuantParam);
      }
      m_hls.aps.multi_data_set_flag[0] =
        aps.attributePresentFlag[0] ? m_pointCloudQuant.getNumMultilColor() > 0 : false;
      m_hls.aps.multi_data_set_flag[1] =
        aps.attributePresentFlag[1] ? m_pointCloudQuant.getNumMultilRefl() > 0 : false;

      m_hls.aps.attribute_num_data_set_minus1[0] =
        m_hls.aps.multi_data_set_flag[0] ? m_pointCloudQuant.getNumMultilColor() - 1 : -1;
      m_hls.aps.attribute_num_data_set_minus1[1] =
        m_hls.aps.multi_data_set_flag[1] ? m_pointCloudQuant.getNumMultilRefl() - 1 : -1;

      m_hls.aps.attribute_num_set_minus1[0] =
        m_hls.aps.multi_data_set_flag[0] ? m_hls.aps.attribute_num_data_set_minus1[0] : -1;
      m_hls.aps.attribute_num_set_minus1[1] =
        m_hls.aps.multi_data_set_flag[1] ? m_hls.aps.attribute_num_data_set_minus1[1] : -1;
      FixedMultiAPs(m_hls.sps, m_hls.aps);

      m_bufferChunk.setBufferType(BufferChunkType::BCT_APS);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_encBac.codeAPS(m_hls.aps, m_hls.sps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();
      aps.maxNumOfNeighbours = 1 << (aps.maxNumOfNeighboursLog2Minus7[0] + 7);
      if (aps.transform == 1) {
        FXPoint::set_kFracBits(aps.kFracBits);
      }
    }

    ///< encoding frame header
    initFrameParameters(m_hls.frameHead);

    TComPointCloud pointCloudRecon;
    pointCloudRecon.setNumPoint(0);
    vector<TComPointCloud> pointCloudPartitionList(1);
    if (!m_sliceFlag) {
      m_sliceOrigin = {0, 0, 0};
      m_sliceID = 0;
      m_sliceBoundingBox = m_hls.sps.geomBoundingBoxSize;
    } else {
      // dividing slice
      if (m_sliceDivisionMode == 0) {
        SliceDevisionByMortonCode(m_hls.frameHead.num_slice_minus_one, pointCloudPartitionList);
      } else if (m_sliceDivisionMode == 1) {
        sliceDevisionByHistZ(m_pointCloudQuant, pointCloudPartitionList,
                             m_hls.frameHead.num_slice_minus_one, m_hls.sps.geomQuantStep,
                             numDigits);
      } else if (m_sliceDivisionMode == 2) {
        SliceDevisionByPointNum(pointCloudPartitionList, m_maxPointNumOfSlicesLog2);
      }
    }
    m_bufferChunk.setBufferType(BufferChunkType::BCT_FRAME);
    m_encBac.setBitstreamBuffer(m_bufferChunk);
    m_encBac.codeFrameHead(m_hls.frameHead);
    m_encBac.encodeFinish();
    m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
    m_bufferChunk.reset();
    m_encBac.reset();

    if (!m_sliceFlag) {
      encodeSlice(esCurFrame);
    } else {
      for (Int slice_id = 0; slice_id < m_hls.frameHead.num_slice_minus_one + 1; slice_id++) {
        const int slicePointNum = pointCloudPartitionList[slice_id].getNumPoint();
        std::cout << "slice number: " << slice_id << " with " << slicePointNum << " points"
                  << std::endl;

        m_sliceID = slice_id;
        m_pointCloudQuant = pointCloudPartitionList[slice_id];
        PC_POS sliceMin, sliceMax, sliceSize;
        m_pointCloudQuant.computeBoundingBox(sliceMin, sliceMax);
        sliceSize = sliceMax - sliceMin;
        m_sliceBoundingBox[0] = Int(round(sliceSize[0])) + 1;
        m_sliceBoundingBox[1] = Int(round(sliceSize[1])) + 1;
        m_sliceBoundingBox[2] = Int(round(sliceSize[2])) + 1;
        for (Int k = 0; k < 3; k++) {
          m_sliceOrigin[k] = floor(sliceMin[k]);
        }
        for (Int i = 0; i < slicePointNum; i++) {
          m_pointCloudQuant[i][0] -= m_sliceOrigin[0];
          m_pointCloudQuant[i][1] -= m_sliceOrigin[1];
          m_pointCloudQuant[i][2] -= m_sliceOrigin[2];
        }
        encodeSlice(esCurFrame);
        addToReconstructionCloud(&pointCloudRecon);
      }
      m_pointCloudRecon = pointCloudRecon;
    }
    std::cout << "Total bitstream size " << m_bitstreamFile.tellp() * 8 << " bits" << std::endl;
    esCurFrame.totalBits += m_bitstreamFile.tellp() * 8;
    m_bitstreamFile.close();
    esCurFrame.totalUserTime = (Double)(clock() - userTimeTotalBegin) / CLOCKS_PER_SEC;
    if (m_hls.sps.attrPresentFlag) {
      esCurFrame.colorUserTime = m_attrEncoder.getColorTime();
      esCurFrame.reflUserTime = m_attrEncoder.getReflectanceTime();
      if (m_hls.aps.attributePresentFlag[0] && m_hls.aps.attributePresentFlag[1]) {
        esCurFrame.colorUserTime =
          (esCurFrame.colorUserTime + esCurFrame.attrUserTime - esCurFrame.reflUserTime) / 2;
        esCurFrame.reflUserTime =
          (esCurFrame.reflUserTime + esCurFrame.attrUserTime - esCurFrame.colorUserTime) / 2;
      } else if (m_hls.aps.attributePresentFlag[0]) {
        esCurFrame.colorUserTime = esCurFrame.attrUserTime;
      } else if (m_hls.aps.attributePresentFlag[1]) {
        esCurFrame.reflUserTime = esCurFrame.attrUserTime;
      }
    }
    cout << "Geometry processing time (user): " << esCurFrame.geomUserTime << " sec." << endl;
    cout << "Recolor processing time (user): " << esCurFrame.recolorUserTime << " sec." << endl;
    cout << "Color processing time (user): " << esCurFrame.colorUserTime << " sec." << endl;
    cout << "Reflectance processing time (user): " << esCurFrame.reflUserTime << " sec." << endl;
    cout << "Attribute processing time (user): " << esCurFrame.attrUserTime << " sec." << endl;
    cout << "Total processing time (user): " << esCurFrame.totalUserTime << " sec." << endl << endl;
    esTotal += esCurFrame;

    geomPostprocessingAndDequantization(m_hls.sps.geomQuantStep);
    if (m_colorTransformFlag && m_pointCloudRecon.hasColors()) {
      m_pointCloudRecon.convertYUVToRGB();
    }

    if (md5FileOpenFlag) {
      md5Calculator.calculateMD5(&m_pointCloudRecon);
      md5Calculator.writeToFile();
      cout << endl;
    }

    ///< write recon ply
    if (m_reconFileName.length() > 0)
      m_pointCloudRecon.writeToFile(m_reconFileName, m_writePlyInAsciiFlag);

    ///< compute metrics
    if (m_metricsEnable) {
      cout << "Computing dmetrics..." << endl;
      pc_evalue::TMetricRes res;
      m_pointCloudOrg.setColors(pointCloudOrgColors);
      pc_evalue::computeMetric(m_pointCloudOrg, m_pointCloudRecon, MetricParam, res);
      totalMetricRes = totalMetricRes + res;
    }
  }

  cout << "All frames number of output points: " << esTotal.numReconPoints << endl;
  cout << "All frames geometry bits: " << esTotal.geomBits << " bits." << endl;
  cout << "All frames color bits: " << esTotal.colorBits << " bits." << endl;
  cout << "All frames refl bits: " << esTotal.reflBits << " bits." << endl;
  if (m_hls.aps.attributePresentFlag[0])
    for (int multiIdx = 0; multiIdx < m_hls.aps.attribute_num_data_set_minus1[0] + 1; ++multiIdx)
      cout << "MultiData " << multiIdx
           << " All frames attributes bits: " << esTotal.attrBits[multiIdx] << " bits." << endl;
  else if (m_hls.aps.attributePresentFlag[1]) {
    if (m_hls.aps.attribute_num_data_set_minus1[1]==0)
        for (int multiIdx = 0; multiIdx < m_hls.aps.attribute_num_data_set_minus1[1] + 1; ++multiIdx)
          cout << "MultiData " << multiIdx
               << " All frames attributes bits: " << esTotal.attrBits[multiIdx] << " bits." << endl;
    else {
      for (int multiIdx = 0, indexGroup = 0;
           multiIdx < m_hls.aps.attribute_num_data_set_minus1[1] + 1; indexGroup++) {
        cout << "MultiData " << multiIdx
             << " All frames attributes bits: " << esTotal.attrBits[multiIdx] << " bits." << endl;
        multiIdx += m_hls.aps.multiAttriGroupNum[indexGroup];
      }
    }
  }

  cout << "All frames total bitstream size: " << esTotal.totalBits << " bits." << endl;
  cout << "All frames geometry processing time (user): " << esTotal.geomUserTime << " sec." << endl;
  cout << "All frames recolor processing time (user): " << esTotal.recolorUserTime << " sec."
       << endl;
  cout << "All frames color processing time (user): " << esTotal.colorUserTime << " sec." << endl;
  cout << "All frames refl processing time (user): " << esTotal.reflUserTime << " sec." << endl;
  cout << "All frames attributes processing time (user): " << esTotal.attrUserTime << " sec."
       << endl;
  cout << "All frames total processing time (user): " << esTotal.totalUserTime << " sec." << endl
       << endl;

  if (m_metricsEnable) {
    totalMetricRes = totalMetricRes / m_numOfFrames;
    totalMetricRes.print(MetricParam, m_hls.aps.attribute_num_data_set_minus1[0] + 1,
                         m_hls.aps.attribute_num_data_set_minus1[1] + 1);
  }
  if (md5FileOpenFlag)
    md5Calculator.closeFile();
  return exitState;
}

Void TEncTop::getSingleAttrAPs(const SequenceParameterSet& sps, AttributeParameterSet& aps,
                               const int& attrIdx, const int& multiIdx) {
  if (attrIdx == 0) {
    aps.orderSwitch = aps.orderMultiSwitch[multiIdx];
    aps.colorReordermode = aps.colorMultiReordermode[multiIdx];
    aps.colorGolombNum = aps.colorMultiGolombNum[multiIdx];
    aps.log2golombGroupSize = aps.log2golombMultiGroupSize[multiIdx];
  }
  if (attrIdx == 1) {
    aps.axisBias = aps.axisMultiBias[multiIdx];
    aps.refReordermode = aps.refMultiReordermode[multiIdx];
    aps.refGolombNum = aps.refMultiGolombNum[multiIdx];
    aps.predFixedPointFracBit = aps.predMultiFixedPointFracBit[multiIdx];
  }
  aps.colorOutputDepth = aps.colorMultiOutputDepth[multiIdx];
  aps.reflOutputDepth = aps.reflMultiOutputDepth[multiIdx];

  aps.transform = aps.transformMulti[attrIdx][multiIdx];
  if ((aps.transform == 0) || (aps.transform == 2)) {
    aps.maxNumOfNeighboursLog2Minus7[attrIdx] =
      aps.maxMultiNumOfNeighboursLog2Minus7[attrIdx][multiIdx];
    aps.maxNumOfNeighbours = 1 << (aps.maxNumOfNeighboursLog2Minus7[attrIdx] + 7);
    
    if (attrIdx == 0) {
      aps.crossComponentPred = aps.crossMultiComponentPred[multiIdx];
      aps.chromaQpOffsetCb = aps.chromaMultiQpOffsetCb[multiIdx];
      aps.chromaQpOffsetCr = aps.chromaMultiQpOffsetCr[multiIdx];
    }
    if (attrIdx == 1) {
      aps.nearestPredParam1 = aps.nearestMultiPredParam1[multiIdx];
      aps.nearestPredParam2 = aps.nearestMultiPredParam2[multiIdx];
      aps.log2predDistWeightGroupSize = aps.log2predDistWeightMultiGroupSize[multiIdx];
    }
  }
  if (aps.transform == 1) {
    aps.kFracBits = aps.kMultiFracBits[attrIdx][multiIdx];
    aps.attrTransformQpDelta = aps.attrMultiTransformQpDelta[attrIdx][multiIdx];
    aps.transformSegmentSize = aps.transformMultiSegmentSize[attrIdx][multiIdx];
    aps.transResLayer = aps.transMultiResLayer[attrIdx][multiIdx];
    if (attrIdx == 0) {
      aps.colorInitPredTransRatio = aps.colorMultiInitPredTransRatio[multiIdx];
    }
    if (attrIdx == 1) {
      aps.refInitPredTransRatio = aps.refMultiInitPredTransRatio[multiIdx];
    }
  }

  if (aps.transform == 2) { 
    aps.log2maxNumofCoeffMinus8 = aps.log2MultimaxNumofCoeffMinus8[attrIdx][multiIdx];
    if (aps.log2maxNumofCoeffMinus8) {
      aps.maxNumofCoeff = 1 << (aps.log2maxNumofCoeffMinus8 + 8);
    }
    aps.QpOffsetDC = aps.QpMultiOffsetDC[attrIdx][multiIdx];
    aps.QpOffsetAC = aps.QpMultiOffsetAC[attrIdx][multiIdx];
    if (attrIdx == 0) {
      aps.colorMaxTransNum = aps.colorMaxMultiTransNum[attrIdx][multiIdx];
      aps.chromaQpOffsetDC = aps.chromaMultiQpOffsetDC[multiIdx];
      aps.chromaQpOffsetAC = aps.chromaMultiQpOffsetAC[multiIdx];
      aps.colorQPAdjustFlag = aps.colorMultiQPAdjustFlag[multiIdx];
      if (aps.colorQPAdjustFlag) {
        aps.colorQPAdjustScalar = aps.colorMultiQPAdjustScalar[multiIdx];
      }
    }
    if (attrIdx == 1) {
      aps.reflMaxTransNum = aps.reflMaxMultiTransNum[attrIdx][multiIdx];
      aps.refGroupPredict = aps.refMultiGroupPredict[multiIdx];
    }
  }
  aps.log2coeffLengthControlMinus8 = aps.log2coeffMultiLengthControlMinus8[attrIdx][multiIdx];
  if (aps.log2coeffLengthControlMinus8) {
    aps.coeffLengthControl = 1 << (aps.log2coeffLengthControlMinus8 + 8);
  }
}


Void TEncTop::FixedMultiAPs(const SequenceParameterSet& sps, AttributeParameterSet& aps) {
  for (int attrIdx = 0; attrIdx < (sps.maxNumAttrMinus1 + 1); attrIdx++) {
    if (aps.attributePresentFlag[attrIdx]) {
      for (int multiIdx = 0; multiIdx < aps.attribute_num_set_minus1[attrIdx] + 1; ++multiIdx) {
        aps.outputMultiBitDepthMinus1[attrIdx][multiIdx] = aps.outputBitDepthMinus1[attrIdx];
        if (attrIdx == 0) {
          aps.orderMultiSwitch[multiIdx] = aps.orderSwitch;
          aps.colorMultiReordermode[multiIdx] = aps.colorReordermode;
          aps.colorMultiGolombNum[multiIdx] = aps.colorGolombNum;
          aps.log2golombMultiGroupSize[multiIdx] = aps.log2golombGroupSize;
        }
        if (attrIdx == 1) {
          aps.axisMultiBias[multiIdx] = aps.axisBias;
          aps.refMultiReordermode[multiIdx] = aps.refReordermode;
          aps.refMultiGolombNum[multiIdx] = aps.refGolombNum;
          aps.predMultiFixedPointFracBit[multiIdx] = aps.predFixedPointFracBit;
        }

        aps.transformMulti[attrIdx][multiIdx] = aps.transform;
        if ((aps.transformMulti[attrIdx][multiIdx] == 0) ||
            (aps.transformMulti[attrIdx][multiIdx] == 2)) {
          aps.maxMultiNumOfNeighboursLog2Minus7[attrIdx][multiIdx] =
            aps.maxNumOfNeighboursLog2Minus7[attrIdx];
          aps.maxMultiNumOfNeighbours[multiIdx] = 1
            << (aps.maxMultiNumOfNeighboursLog2Minus7[attrIdx][multiIdx] + 7);
          
          if (attrIdx == 0) {
            aps.crossMultiComponentPred[multiIdx] = aps.crossComponentPred;
            aps.chromaMultiQpOffsetCb[multiIdx] = aps.chromaQpOffsetCb;
            aps.chromaMultiQpOffsetCr[multiIdx] = aps.chromaQpOffsetCr;
          }
          if (attrIdx == 1) {
            aps.nearestMultiPredParam1[multiIdx] = aps.nearestPredParam1;
            aps.nearestMultiPredParam2[multiIdx] = aps.nearestPredParam2;
            aps.log2predDistWeightMultiGroupSize[multiIdx] = aps.log2predDistWeightGroupSize;
          }
        }
        if (aps.transformMulti[attrIdx][multiIdx] == 1) {
          aps.kMultiFracBits[attrIdx][multiIdx] = aps.kFracBits;
          aps.attrMultiTransformQpDelta[attrIdx][multiIdx] = aps.attrTransformQpDelta;
          aps.transformMultiSegmentSize[attrIdx][multiIdx] = aps.transformSegmentSize;
          aps.transMultiResLayer[attrIdx][multiIdx] = aps.transResLayer;
          if (attrIdx == 0) {
            aps.colorMultiInitPredTransRatio[multiIdx] = aps.colorInitPredTransRatio;
          }
          if (attrIdx == 1) {
            aps.refMultiInitPredTransRatio[multiIdx] = aps.refInitPredTransRatio;
          }
        }
        if (aps.transformMulti[attrIdx][multiIdx] == 2) {         
          aps.log2MultimaxNumofCoeffMinus8[attrIdx][multiIdx] = aps.log2maxNumofCoeffMinus8;
          if (aps.log2MultimaxNumofCoeffMinus8[attrIdx][multiIdx]) {
            aps.maxMultiNumofCoeff[attrIdx][multiIdx] = 1
              << (aps.log2MultimaxNumofCoeffMinus8[attrIdx][multiIdx] + 8);
          }
          aps.QpMultiOffsetDC[attrIdx][multiIdx] = aps.QpOffsetDC;
          aps.QpMultiOffsetAC[attrIdx][multiIdx] = aps.QpOffsetAC;
          if (attrIdx == 0) {
            aps.colorMaxMultiTransNum[attrIdx][multiIdx] = aps.colorMaxTransNum;
            aps.chromaMultiQpOffsetDC[multiIdx] = aps.chromaQpOffsetDC;
            aps.chromaMultiQpOffsetAC[multiIdx] = aps.chromaQpOffsetAC;
            aps.colorMultiQPAdjustFlag[multiIdx] = aps.colorQPAdjustFlag;
            if (aps.colorMultiQPAdjustFlag[multiIdx]) {
              aps.colorMultiQPAdjustScalar[multiIdx] = aps.colorQPAdjustScalar;
            }
          }
          if (attrIdx == 1) {
            aps.reflMaxMultiTransNum[attrIdx][multiIdx] = aps.reflMaxTransNum;
            aps.refMultiGroupPredict[multiIdx] = aps.refGroupPredict;
          }
        }

        aps.log2coeffMultiLengthControlMinus8[attrIdx][multiIdx] = aps.log2coeffLengthControlMinus8;
        if (aps.log2coeffMultiLengthControlMinus8[attrIdx][multiIdx]) {
          aps.coeffMultiLengthControl[attrIdx][multiIdx] = 1
            << (aps.log2coeffMultiLengthControlMinus8[attrIdx][multiIdx] + 8);
        }
      }
    }
  }
  // set multiAttriGroupID and multiAttriGroupNum
  if (aps.attributePresentFlag[1] && (aps.attribute_num_set_minus1[1] > 0) && aps.transform==0) {
    int numAttriCount = 0, i = 0, multiIdx=0;
    if (aps.attribute_num_data_set_minus1[1] == 44) {
      std::vector<UInt> multiAttriIDGroup1 = {3, 1, 1, 1, 1, 1, 1, 3, 3, 5, 5, 1, 1, 1,
                                              1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3};
      while (numAttriCount < (aps.attribute_num_data_set_minus1[1] + 1)) {
        aps.multiAttriGroupNum[i] = multiAttriIDGroup1[i];
        numAttriCount += multiAttriIDGroup1[i];
        while (multiIdx < numAttriCount) {
          aps.multiAttriGroupID[multiIdx] = i;
          multiIdx++;
        }
        i++;
      }
      aps.numofMultiAttriGroup = i;
    } else if (aps.attribute_num_data_set_minus1[1] == 8) {
      std::vector<UInt> multiAttriIDGroup1 = {1, 1, 1, 1, 1, 1, 3};
      while (numAttriCount < (aps.attribute_num_data_set_minus1[1] + 1)) {
        aps.multiAttriGroupNum[i] = multiAttriIDGroup1[i];
        numAttriCount += multiAttriIDGroup1[i];
        while (multiIdx < numAttriCount) {
          aps.multiAttriGroupID[multiIdx] = i;
          multiIdx++;
        }
        i++;
      }
      aps.numofMultiAttriGroup = i;
    } else if (aps.attribute_num_data_set_minus1[1] == 42) {
      std::vector<UInt> multiAttriIDGroup1 = {3, 1, 1, 1, 1, 1, 3, 3, 5, 5, 1,
                                              4, 4, 1, 1, 1, 1, 1, 1, 1, 3};
      while (numAttriCount < (aps.attribute_num_data_set_minus1[1] + 1)) {
        aps.multiAttriGroupNum[i] = multiAttriIDGroup1[i];
        numAttriCount += multiAttriIDGroup1[i];
        while (multiIdx < numAttriCount) {
          aps.multiAttriGroupID[multiIdx] = i;
          multiIdx++;
        }
        i++;
      }
      aps.numofMultiAttriGroup = i;
    } else if (aps.attribute_num_data_set_minus1[1] == 24) {
      std::vector<UInt> multiAttriIDGroup1 = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                              1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3};
      while (numAttriCount < (aps.attribute_num_data_set_minus1[1] + 1)) {
        aps.multiAttriGroupNum[i] = multiAttriIDGroup1[i];
        numAttriCount += multiAttriIDGroup1[i];
        while (multiIdx < numAttriCount) {
          aps.multiAttriGroupID[multiIdx] = i;
          multiIdx++;
        }
        i++;
      }
      aps.numofMultiAttriGroup = i;
    } else {
      while (numAttriCount < (aps.attribute_num_data_set_minus1[1] + 1)) {
        aps.multiAttriGroupNum[i] = 1;
        numAttriCount += 1;
        while (multiIdx < numAttriCount) {
          aps.multiAttriGroupID[multiIdx] = i;
          multiIdx++;
        }
        i++;
      }
      aps.numofMultiAttriGroup = i;
    }
  }
}


Void TEncTop::encodeSlice(EncoderStatistics& es) {
  auto& aps = m_hls.aps;

  ///< node size (log2) for xyz dimensions
  UInt maxBB = std::max({1U, m_sliceBoundingBox[0], m_sliceBoundingBox[1], m_sliceBoundingBox[2]});
  V3<UInt> nodeSizeLog2;
  if (m_hls.gps.im_qtbt_flag) {
    nodeSizeLog2[0] = ceilLog2(m_sliceBoundingBox[0]);
    nodeSizeLog2[1] = ceilLog2(m_sliceBoundingBox[1]);
    nodeSizeLog2[2] = ceilLog2(m_sliceBoundingBox[2]);
  } else {
    nodeSizeLog2 = ceilLog2(maxBB);
  }

  ///<encoding geometry brick header
  m_bufferChunk.setBufferType(BufferChunkType::BCT_GBH);
  m_encBac.setBitstreamBuffer(m_bufferChunk);
  m_hls.gbh.sliceID = m_sliceID;
  m_hls.gbh.geomBoundingBoxOrigin = m_sliceOrigin;
  m_hls.gbh.nodeSizeLog2 = nodeSizeLog2;
  m_hls.gbh.geomNumPoints = (UInt)m_pointCloudQuant.getNumPoint();
  int allDiemNodeSizeLog = 0;
  allDiemNodeSizeLog = nodeSizeLog2[0] + nodeSizeLog2[1] + nodeSizeLog2[2];
  Int64 currnentVolumePerPoint = 0;
  Int64 voxelSize = 1;
  voxelSize <<= allDiemNodeSizeLog;
  currnentVolumePerPoint = voxelSize / m_hls.gbh.geomNumPoints;

  m_hls.gbh.ifSparse1 = !!(currnentVolumePerPoint > 430000);

  bool ifSparse = false;
  ifSparse = !!(currnentVolumePerPoint > 120000);

  bool planarModeEligible = m_hls.gps.planarSeqEligible && (currnentVolumePerPoint < 280000) &&
    (currnentVolumePerPoint > 750);

  //density  change
  Int64 voxelSize1 = 1;
  Int64 currnentVolumePerPoint1 = 0;
  Int64 a = 1;
  Int64 b = 1;
  Int64 c = 1;
  a <<= (nodeSizeLog2[0] + nodeSizeLog2[1]);
  b <<= (nodeSizeLog2[0] + nodeSizeLog2[2]);
  c <<= (nodeSizeLog2[1] + nodeSizeLog2[2]);
  voxelSize1 = a + b + c;
  currnentVolumePerPoint1 = voxelSize1 / m_hls.gbh.geomNumPoints;
  bool dense = false;
  dense = !!(currnentVolumePerPoint1 < 7);
  if (dense) {
    m_hls.gbh.geom_context_mode = 1;
    m_hls.gbh.im_qtbt_num_before_ot = 5;
    m_hls.gbh.im_qtbt_min_size = 0;
  } else {
    m_hls.gbh.geom_context_mode = 0;
    m_hls.gbh.im_qtbt_num_before_ot = 0;
    m_hls.gbh.im_qtbt_min_size = 0;
  }

  if (ifSparse)
    m_hls.gbh.singleModeFlagInSlice = 1;
  else
    m_hls.gbh.singleModeFlagInSlice = 0;



  m_hls.gbh.planarModeEligibleForSlice = planarModeEligible;
  m_encBac.codeGBH(m_hls.gps, m_hls.gbh);
  m_encBac.encodeFinish();
  m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
  m_bufferChunk.reset();
  m_encBac.reset();

  ///< start geometry coding
  clock_t userTimeGeometryBegin = clock();
  m_bufferChunk.setBufferType(BufferChunkType::BCT_GEOM);
  m_encBac.setBitstreamBuffer(m_bufferChunk);
  m_encBac.initBac();
  compressAndEncodePartition();
  m_encBac.encodeTerminationFlag();
  m_encBac.encodeFinish();
  m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
  cout << "Number of output points: " << m_pointCloudRecon.getNumPoint() << endl;
  cout << "Geometry bits: " << m_encBac.getBitStreamLength() * 8 << " bits." << endl;
  cout << "Geometry bpp: "
       << (Double)(m_encBac.getBitStreamLength()) * 8 / m_pointCloudQuant.getNumPoint() << " bpp."
       << endl;
  es.geomBits += m_encBac.getBitStreamLength() * 8;
  m_bufferChunk.reset();
  m_encBac.reset();
  es.geomUserTime += (Double)(clock() - userTimeGeometryBegin) / CLOCKS_PER_SEC;
  es.numReconPoints += m_pointCloudRecon.getNumPoint();

  //encoding attribute brick header
  if (m_hls.sps.attrPresentFlag) {
    m_encBac.computeAttributeID(m_hls.abh, m_hls.aps, m_hls.sps);
    m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH);
    m_encBac.setBitstreamBuffer(m_bufferChunk);
    m_hls.abh.sliceID = m_sliceID;
    m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
    m_encBac.encodeFinish();
    m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
    m_bufferChunk.reset();
    m_encBac.reset();
  }

  ///< start attribute coding
  int colorBitstreamSzie[NUM_MULTIATTRIBUTE] = {0};
  int refBitstreamSzie[NUM_MULTIATTRIBUTE] = {0};
  if (m_hls.sps.attrPresentFlag) {
    clock_t userTimeAttributeBegin = clock();
    m_attrEncoder.getColorBits() = 0;
    m_attrEncoder.getReflectanceBits() = 0;
    m_attrEncoder.getColorTime() = 0;
    m_attrEncoder.getReflectanceTime() = 0;
    if (m_hls.aps.attributePresentFlag[0] && m_hls.aps.attributePresentFlag[1]) {
      multiDataID = 0;
      getSingleAttrAPs(m_hls.sps, m_hls.aps, 0, m_hls.abh.attribute_ID[0][multiDataID]);
      getSingleAttrAPs(m_hls.sps, m_hls.aps, 1, m_hls.abh.attribute_ID[1][multiDataID]);
      m_bufferChunk.setBufferType(BufferChunkType::BCT_COL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_encBac.initBac();
      //set adaptive expGolomb encoder parameter
      m_encBac.setGolombGroupSize(aps.log2golombGroupSize);
      m_encBac.setColorGolombKandBound(aps.colorGolombNum);

      TComBufferChunk bufferChunkDual(BufferChunkType::BCT_REFL);
      if ((aps.transform == 0) || (aps.transform == 2)) {
        m_encBacDual.setBitstreamBuffer(bufferChunkDual,true);
        m_encBacDual.initBac();
        //set adaptive expGolomb encoder parameter
        m_encBacDual.setGolombGroupSize(aps.log2golombGroupSize);
        m_encBacDual.setColorGolombKandBound(aps.colorGolombNum);
      }

      compressAndEncodeAttribute();
      m_encBac.encodeTerminationFlag();
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());

      if ((aps.transform == 0) || (aps.transform == 2)) {
        m_attrEncoder.getColorBits() = m_encBac.getBitStreamLength() * 8;
        m_encBacDual.encodeTerminationFlag();
        m_encBacDual.encodeFinish();
        bufferChunkDual.writeToBitstream(&m_bitstreamFile, m_encBacDual.getBitStreamLength());
        m_attrEncoder.getReflectanceBits() = m_encBacDual.getBitStreamLength() * 8;
        bufferChunkDual.reset();
        m_encBacDual.reset();
      }
      colorBitstreamSzie[multiDataID] = m_attrEncoder.getColorBits();
      refBitstreamSzie[multiDataID] = m_attrEncoder.getReflectanceBits();
      m_bufferChunk.reset();
      m_encBac.reset();
    }
    if (m_hls.aps.attributePresentFlag[0] && !m_hls.aps.attributePresentFlag[1]) {
      multiDataID = 0;
      m_attrEncoder.getColorBits() = 0;
      while (multiDataID < m_hls.aps.attribute_num_data_set_minus1[0] + 1) {
        getSingleAttrAPs(m_hls.sps, m_hls.aps, 0, m_hls.abh.attribute_ID[0][multiDataID]);

        m_bufferChunk.setBufferType(BufferChunkType::BCT_COL);
        m_encBac.setBitstreamBuffer(m_bufferChunk);
        m_encBac.initBac();
        //set adaptive expGolomb encoder parameter
        m_encBac.setGolombGroupSize(aps.log2golombGroupSize);
        m_encBac.setColorGolombKandBound(aps.colorGolombNum);

        compressAndEncodeColor();
        m_encBac.encodeTerminationFlag();
        m_encBac.encodeFinish();
        m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
        m_attrEncoder.getColorBits() += m_encBac.getBitStreamLength() * 8;
        m_bufferChunk.reset();
        m_encBac.reset();
        colorBitstreamSzie[multiDataID] += m_attrEncoder.getColorBits();
        cout << "MultiData " << multiDataID << " Attributes_color "
             << "bits: " << m_attrEncoder.getColorBits() << " bits." << endl;
        cout << "MultiData " << multiDataID << " Attributes_color bpp: "
             << (Double)(m_attrEncoder.getColorBits()) / m_pointCloudQuant.getNumPoint() << " bpp."
             << endl;
        ++multiDataID;
      }
      std::cout << "current slice attribute user time: " << m_attrEncoder.getColorTime() << " sec."
                << endl;
    }
    cout << "Attributes_color bits: " << m_attrEncoder.getColorBits() << " bits." << endl;
    cout << "Attributes_color bpp: "
         << (Double)(m_attrEncoder.getColorBits()) / m_pointCloudQuant.getNumPoint() << " bpp."
         << endl;
    if (!m_hls.aps.attributePresentFlag[0] && m_hls.aps.attributePresentFlag[1]) {
      multiDataID = 0;
      m_attrEncoder.getReflectanceBits() = 0;
      int multiDataGroupNum = 1, multiDataGroupIndex=0;
      if (m_hls.aps.attribute_num_data_set_minus1[1] > 0)
        m_encBac.computeBufferSize(m_pointCloudOrg.getNumPoint(), 5);
      while (multiDataID < m_hls.aps.attribute_num_data_set_minus1[1] + 1) {
        getSingleAttrAPs(m_hls.sps, m_hls.aps, 1, m_hls.abh.attribute_ID[1][multiDataID]);
        if (m_hls.aps.attribute_num_data_set_minus1[1] > 0 && m_hls.aps.transform == 0) {
          multiDataGroupNum = m_hls.aps.multiAttriGroupNum[multiDataGroupIndex];
          multiDataGroupIndex++;
        }
        m_bufferChunk.setBufferType(BufferChunkType::BCT_REFL);
        m_encBac.setBitstreamBuffer(m_bufferChunk);
        m_encBac.initBac();

        compressAndEncodeReflectance();
        m_encBac.encodeTerminationFlag();
        m_encBac.encodeFinish();
        m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
        m_attrEncoder.getReflectanceBits() += m_encBac.getBitStreamLength() * 8;
        refBitstreamSzie[multiDataID] += m_attrEncoder.getReflectanceBits();
        cout << "MultiData " << multiDataID << " Attributes_refl "
             << "bits :" << m_attrEncoder.getReflectanceBits() << " bits." << endl;
        cout << "MultiData " << multiDataID << " Attributes_refl bpp: "
             << (Double)(m_attrEncoder.getReflectanceBits()) / m_pointCloudQuant.getNumPoint()
             << " bpp." << endl;
        multiDataID += multiDataGroupNum;
        m_bufferChunk.reset();
        m_encBac.reset();
      }
      std::cout << "current slice attribute user time: " << m_attrEncoder.getReflectanceTime()
                << " sec." << endl;
    }
    cout << "Attributes_refl bits: " << m_attrEncoder.getReflectanceBits() << " bits." << endl;
    cout << "Attributes_refl bpp: "
         << (Double)(m_attrEncoder.getReflectanceBits()) / m_pointCloudQuant.getNumPoint()
         << " bpp." << endl;

    UInt64 getAttributeBits[NUM_MULTIATTRIBUTE] = {0};
    if (m_hls.aps.attributePresentFlag[0]) {
      for (int multiIdx = 0; multiIdx < m_hls.aps.attribute_num_data_set_minus1[0] + 1; ++multiIdx)
        getAttributeBits[multiIdx] = colorBitstreamSzie[multiIdx];
      for (int multiIdx = 0; multiIdx < m_hls.aps.attribute_num_data_set_minus1[0] + 1; ++multiIdx)
        es.attrBits[multiIdx] += colorBitstreamSzie[multiIdx];
    }
    if (m_hls.aps.attributePresentFlag[1]) {
      for (int multiIdx = 0; multiIdx < m_hls.aps.attribute_num_data_set_minus1[1] + 1; ++multiIdx)
        getAttributeBits[multiIdx] = refBitstreamSzie[multiIdx];
      for (int multiIdx = 0; multiIdx < m_hls.aps.attribute_num_data_set_minus1[1] + 1; ++multiIdx)
        es.attrBits[multiIdx] += refBitstreamSzie[multiIdx];
    }
    es.colorBits += m_attrEncoder.getColorBits();
    es.reflBits += m_attrEncoder.getReflectanceBits();
    es.attrUserTime += (Double)(clock() - userTimeAttributeBegin) / CLOCKS_PER_SEC;
  }
}

//////////////////////////////////////////////////////////////////////////
// Private class functions
//////////////////////////////////////////////////////////////////////////

Void TEncTop::initParameters() {
  m_hls.sps.level = 0;
  m_hls.sps.spsID = 0;

  m_hls.gps.gpsID = 0;
  m_hls.gps.spsID = 0;

  m_hls.aps.apsID = 0;
  m_hls.aps.spsID = 0;
}
Void TEncTop::initFrameParameters(FrameHeader& frameHead) {
  frameHead.frame_index = 0;
  frameHead.num_slice_minus_one = 0;
  frameHead.timestamp_flag = true;
  if (frameHead.timestamp_flag)
    frameHead.timestamp = 0;
}

Void TEncTop::compressAndEncodePartition() {
  m_geomEncoder.init(&m_pointCloudQuant, &m_pointCloudRecon, &m_hls, &m_encBac);
  m_geomEncoder.compressAndEncodeGeometry();
  m_geomEncoder.clear();
}

Void TEncTop::fastRecolor(const std::pair<const PC_POS, std::vector<int>> it,
                          const Int& uniquePointNumber, const Float& qs) {
  ///< fast recolor by weighted average in terms of the distance to current pos
  vector<Int64> weight;
  Int64 weightSum = 0;
  Double eps = 0.1;
  PC_POS posDequant = m_pointCloudQuant[uniquePointNumber] * qs;
  posDequant[0] += m_hls.sps.geomBoundingBoxOrigin[0];
  posDequant[1] += m_hls.sps.geomBoundingBoxOrigin[1];
  posDequant[2] += m_hls.sps.geomBoundingBoxOrigin[2];

  for (const auto& idx : it.second) {
    PC_POS posOrg = m_pointCloudOrg[idx];
    Double d = (posOrg - posDequant).getNorm1();
    Int64 w = (Int64)round(1024.0 / (eps + d / qs));
    weight.push_back(w);
    weightSum += w;
  }

  if (m_pointCloudOrg.hasColors()) {
    for (int multiIdx = 0; multiIdx < m_pointCloudOrg.getNumMultilColor(); ++multiIdx) {
      V3<Double> avg_col = 0;
      Int cnt = 0;
      for (const auto& idx : it.second) {
        Int64 w = weight[cnt++];
        avg_col[0] += m_pointCloudOrg.getColor(idx, multiIdx)[0] * w;
        avg_col[1] += m_pointCloudOrg.getColor(idx, multiIdx)[1] * w;
        avg_col[2] += m_pointCloudOrg.getColor(idx, multiIdx)[2] * w;
      }
      avg_col /= Double(weightSum);
      m_pointCloudQuant.getColor(uniquePointNumber, multiIdx)[0] =
        (UInt8)TComClip(0.0, 255.0, std::round(avg_col[0]));
      m_pointCloudQuant.getColor(uniquePointNumber, multiIdx)[1] =
        (UInt8)TComClip(0.0, 255.0, std::round(avg_col[1]));
      m_pointCloudQuant.getColor(uniquePointNumber, multiIdx)[2] =
        (UInt8)TComClip(0.0, 255.0, std::round(avg_col[2]));
    }
  }

  if (m_pointCloudOrg.hasReflectances()) {
    for (int multiIdx = 0; multiIdx < m_pointCloudOrg.getNumMultilRefl(); ++multiIdx) {
      Double avg_ref = 0;
      Int cnt = 0;
      for (const auto& idx : it.second) {
        Int64 w = weight[cnt++];
        avg_ref += m_pointCloudOrg.getReflectance(idx, multiIdx) * w;
      }
      avg_ref /= weightSum;
      avg_ref = TComClip(double(std::numeric_limits<PC_REFL>::min()),
                         double(std::numeric_limits<PC_REFL>::max()), std::round(avg_ref));

      m_pointCloudQuant.getReflectance(uniquePointNumber, multiIdx) = (PC_REFL)avg_ref;
    }
  }
}

Void TEncTop::geomPreprocessAndQuantization(UInt& geoNumPoint, const Float& qs,
                                            Double& userTimeRecolor) {
  m_pointCloudQuant = m_pointCloudOrg;

  ///< shift to origin
  const TSize pointCount = m_pointCloudOrg.getNumPoint();
  for (Int i = 0; i < pointCount; i++) {
    m_pointCloudQuant[i][0] -= m_hls.sps.geomBoundingBoxOrigin[0];
    m_pointCloudQuant[i][1] -= m_hls.sps.geomBoundingBoxOrigin[1];
    m_pointCloudQuant[i][2] -= m_hls.sps.geomBoundingBoxOrigin[2];
  }

  ///< geometry quantization
  for (Int i = 0; i < pointCount; i++) {
    m_pointCloudQuant[i][0] = round(m_pointCloudQuant[i][0] / qs);
    m_pointCloudQuant[i][1] = round(m_pointCloudQuant[i][1] / qs);
    m_pointCloudQuant[i][2] = round(m_pointCloudQuant[i][2] / qs);
  }

  ///< remove duplicate points here
  if (m_hls.sps.geomRemoveDuplicateFlag && (m_pointCloudQuant.getNumPoint() != 1)) {
    if (m_hls.sps.recolorMode == 0 || m_hls.sps.attrPresentFlag == 0 ||
        (!(m_pointCloudOrg.hasColors() || m_pointCloudOrg.hasReflectances()))) {
      set<PC_POS> uniquePoints;
      Int uniquePointNumber = 0;
      for (Int i = 0; i < pointCount; i++) {
        if (uniquePoints.find(m_pointCloudQuant[i]) == uniquePoints.end()) {
          uniquePoints.insert(m_pointCloudQuant[i]);
          m_pointCloudQuant.swapPoints(i, uniquePointNumber);
          uniquePointNumber++;
        }
      }
      m_pointCloudQuant.setNumPoint(uniquePointNumber);
      geoNumPoint = uniquePointNumber;
    } else {
      //fast recolor method
      clock_t userTimeRecolorBegin = clock();
      map<PC_POS, vector<Int>> uniquePoints;
      for (Int i = 0; i < pointCount; i++) {
        auto it = uniquePoints.find(m_pointCloudQuant[i]);
        if (it == uniquePoints.end())
          uniquePoints.insert(pair<PC_POS, vector<Int>>(m_pointCloudQuant[i], vector<Int>({i})));
        else
          it->second.push_back(i);
      }
      Int uniquePointNumber = 0;
      for (const auto& it : uniquePoints) {
        m_pointCloudQuant[uniquePointNumber] = it.first;

        if (m_hls.sps.recolorMode == 1) {
          fastRecolor(it, uniquePointNumber, qs);
        }
        uniquePointNumber++;
      }
      m_pointCloudQuant.setNumPoint(uniquePointNumber);
      geoNumPoint = uniquePointNumber;
      userTimeRecolor = (Double)(clock() - userTimeRecolorBegin) / CLOCKS_PER_SEC;
    }
  }
}

Void TEncTop::geomPreprocessAndQuantizationRetainingDuplicatePoints(const Float& qs) {
  m_pointCloudQuant = m_pointCloudOrg;

  ///< shift to origin
  const TSize pointCount = m_pointCloudQuant.getNumPoint();
  for (Int i = 0; i < pointCount; i++) {
    m_pointCloudQuant[i][0] -= m_hls.sps.geomBoundingBoxOrigin[0];
    m_pointCloudQuant[i][1] -= m_hls.sps.geomBoundingBoxOrigin[1];
    m_pointCloudQuant[i][2] -= m_hls.sps.geomBoundingBoxOrigin[2];
  }

  ///< geometry quantization
  for (Int i = 0; i < pointCount; i++) {
    m_pointCloudQuant[i][0] = round(m_pointCloudQuant[i][0] / qs);
    m_pointCloudQuant[i][1] = round(m_pointCloudQuant[i][1] / qs);
    m_pointCloudQuant[i][2] = round(m_pointCloudQuant[i][2] / qs);
  }
}

Void TEncTop::geomPostprocessingAndDequantization(const Float& qs) {
  const TSize numPoints = m_pointCloudRecon.getNumPoint();

  ///< geometry dequantization
  if (qs > 1) {
    for (TSize i = 0; i < numPoints; i++) {
      m_pointCloudRecon[i][0] = round(m_pointCloudRecon[i][0] * qs * 1e6) / 1e6;
      m_pointCloudRecon[i][1] = round(m_pointCloudRecon[i][1] * qs * 1e6) / 1e6;
      m_pointCloudRecon[i][2] = round(m_pointCloudRecon[i][2] * qs * 1e6) / 1e6;
    }
  } else {
    for (TSize i = 0; i < numPoints; i++) {
      m_pointCloudRecon[i][0] = m_pointCloudRecon[i][0] * qs;
      m_pointCloudRecon[i][1] = m_pointCloudRecon[i][1] * qs;
      m_pointCloudRecon[i][2] = m_pointCloudRecon[i][2] * qs;
    }
  }

  ///< shift back to world coordinates
  for (TSize i = 0; i < numPoints; i++) {
    m_pointCloudRecon[i][0] += m_hls.sps.geomBoundingBoxOrigin[0];
    m_pointCloudRecon[i][1] += m_hls.sps.geomBoundingBoxOrigin[1];
    m_pointCloudRecon[i][2] += m_hls.sps.geomBoundingBoxOrigin[2];
  }
}

Void TEncTop::compressAndEncodeAttribute() {
  m_attrEncoder.initDual(&m_pointCloudRecon, &m_pointCloudRecon, &m_hls, &m_encBac, &m_encBacDual,
                         m_frame_ID, m_numOfFrames, multiDataID);
  if (m_hls.aps.transform == 0) {
    m_attrEncoder.predictEncodeAttribute();
  } else if (m_hls.aps.transform == 1) {
    m_attrEncoder.transformEncodeAttribute();
  } else if (m_hls.aps.transform == 2) {
    m_attrEncoder.predictAndTransformEncodeAttribute();
  }
}

Void TEncTop::compressAndEncodeColor() {
  m_attrEncoder.init(&m_pointCloudRecon, &m_pointCloudRecon, &m_hls, &m_encBac, m_frame_ID,
                     m_numOfFrames, multiDataID);
  if (m_hls.aps.transform == 0) {
    m_attrEncoder.predictEncodeColor();
  } else if (m_hls.aps.transform == 1) {
    m_attrEncoder.transformEncodeColor();
  } else if (m_hls.aps.transform == 2) {
    m_attrEncoder.predictAndTransformEncodeColor();
  }
}

Void TEncTop::compressAndEncodeReflectance() {
  m_attrEncoder.init(&m_pointCloudRecon, &m_pointCloudRecon, &m_hls, &m_encBac, m_frame_ID,
                     m_numOfFrames, multiDataID);
  if (m_hls.aps.transform == 0) {
    if (m_hls.aps.attribute_num_data_set_minus1[1] > 0 &&
        m_hls.aps.multiAttriGroupNum[m_hls.aps.multiAttriGroupID[multiDataID]] > 1)
      m_attrEncoder.predictEncodeMultiReflectance();
    else
      m_attrEncoder.predictEncodeReflectance(); 
  } else if (m_hls.aps.transform == 1) {
    m_attrEncoder.transformEncodeReflectance();
  } else if (m_hls.aps.transform == 2) {
    m_attrEncoder.predictAndTransformEncodeReflectance();
  }
}

Void TEncTop::SliceDevisionByMortonCode(int slicenum,
                                        vector<TComPointCloud>& pointCloudPartitionList) {
  // Morton sorting
  int32_t voxelCount = m_pointCloudQuant.getNumPoint();
  std::vector<pointCodeWithIndex> pointCloudMorton(voxelCount);
  for (int n = 0; n < voxelCount; n++) {
    const auto point = m_pointCloudQuant[n];
    const int32_t x = (int32_t)point[0];
    const int32_t y = (int32_t)point[1];
    const int32_t z = (int32_t)point[2];
    pointCloudMorton[n].code = mortonAddr(x, y, z);
    pointCloudMorton[n].index = n;
  }
  std::sort(pointCloudMorton.begin(), pointCloudMorton.end());

  //to make sure the num to move right
  int64_t movedigits = 0;
  while (((pointCloudMorton[voxelCount - 1].code >> movedigits) -
          (pointCloudMorton[0].code >> movedigits)) >= slicenum)
    movedigits++;
  int slicesize = ((pointCloudMorton[voxelCount - 1].code >> movedigits) -
                   (pointCloudMorton[0].code >> movedigits)) +
    1;

  //to cteate slice
  int64_t fatherMorton = (pointCloudMorton[0].code >> movedigits);
  auto end = std::lower_bound(pointCloudMorton.begin(), pointCloudMorton.end(),
                              (fatherMorton) << movedigits) -
    pointCloudMorton.begin();
  for (int slice_id = 0; fatherMorton <= (pointCloudMorton[voxelCount - 1].code >> movedigits);
       slice_id++, fatherMorton++) {
    auto begin = end;
    end = std::lower_bound(pointCloudMorton.begin() + begin, pointCloudMorton.end(),
                           (fatherMorton + 1) << movedigits) -
      pointCloudMorton.begin();
    if (begin == end) {
      slice_id--;
      continue;
    }

    int index = 0;
    pointCloudPartitionList[slice_id].setNumPoint(end - begin);
    if (m_pointCloudQuant.hasColors())
      pointCloudPartitionList[slice_id].addColors();

    if (m_pointCloudQuant.hasReflectances())
      pointCloudPartitionList[slice_id].addReflectances();

    for (int i = begin, j = 0; i < end && i < voxelCount; i++, j++) {
      pointCloudPartitionList[slice_id][j] = m_pointCloudQuant[pointCloudMorton[i].index];
      if (m_pointCloudQuant.hasColors()) {
        for (int multiIdx = 0; multiIdx < m_pointCloudQuant.getNumMultilColor(); ++multiIdx)
          pointCloudPartitionList[slice_id].setColor(
            j, m_pointCloudQuant.getColor(pointCloudMorton[i].index, multiIdx), multiIdx);
      }

      if (m_pointCloudQuant.hasReflectances()) {
        for (int multiIdx = 0; multiIdx < m_pointCloudQuant.getNumMultilRefl(); ++multiIdx)
          pointCloudPartitionList[slice_id].setReflectance(
            j, m_pointCloudQuant.getReflectance(pointCloudMorton[i].index, multiIdx), multiIdx);
      }
    }
  }
  pointCloudPartitionList.erase(
    std::remove_if(pointCloudPartitionList.begin(), pointCloudPartitionList.end(),
                   [](const TComPointCloud& partion) { return partion.getNumPoint() == 0; }),
    pointCloudPartitionList.end());
  m_hls.frameHead.num_slice_minus_one = pointCloudPartitionList.size() - 1;
}

Void TEncTop::addToReconstructionCloud(TComPointCloud* reconstructionCloud) 
{
  int voxelCount_add = m_pointCloudRecon.getNumPoint();
  int voxelClout_re = reconstructionCloud->getNumPoint();
  if (m_hls.gbh.geomBoundingBoxOrigin.max())
    for (size_t idx = 0; idx < voxelCount_add; ++idx)
      for (size_t k = 0; k < 3; ++k)
        m_pointCloudRecon[idx][k] += m_hls.gbh.geomBoundingBoxOrigin[k];

  if (!m_hls.gbh.sliceID) {
    *reconstructionCloud = m_pointCloudRecon;
    return;
  }
  reconstructionCloud->init(voxelClout_re + voxelCount_add, m_pointCloudRecon.hasColors(),
                            m_pointCloudRecon.hasReflectances());
  std::copy(m_pointCloudRecon.positions().begin(), m_pointCloudRecon.positions().end(),
            std::next(reconstructionCloud->positions().begin(), voxelClout_re));
  if (reconstructionCloud->hasColors())
    std::copy(m_pointCloudRecon.getColors().begin(), m_pointCloudRecon.getColors().end(),
              std::next(reconstructionCloud->getColors().begin(),
                        voxelClout_re * reconstructionCloud->getNumMultilColor()));
  if (reconstructionCloud->hasReflectances())
    std::copy(m_pointCloudRecon.getReflectances().begin(),
              m_pointCloudRecon.getReflectances().end(),
              std::next(reconstructionCloud->getReflectances().begin(),
                        voxelClout_re * reconstructionCloud->getNumMultilRefl()));
}

Void TEncTop::pointCloudPack(TComPointCloud& pointCloudRecon, const TComPointCloud& pointCloudAdd) {
  vector<PC_POS> pos_org(pointCloudRecon.positions()), pos_add(pointCloudAdd.positions());
  vector<PC_REFL> refl_org(pointCloudRecon.getReflectances()),
    refl_add(pointCloudAdd.getReflectances());
  vector<PC_COL> col_org(pointCloudRecon.getColors()), col_add(pointCloudAdd.getColors());

  pos_org.insert(pos_org.end(), pos_add.begin(), pos_add.end());
  refl_org.insert(refl_org.end(), refl_add.begin(), refl_add.end());
  col_org.insert(col_org.end(), col_add.begin(), col_add.end());

  pointCloudRecon.setPoses(pos_org);
  pointCloudRecon.setReflectances(refl_org);
  pointCloudRecon.setColors(col_org);
}

Void TEncTop::histogramZ(PC_POS posMin, PC_POS posMax, const TComPointCloud pc,
                         vector<int32_t>& groundZ) {
  Int rangeZ = posMax[2] - posMin[2];
  vector<Int> listNum(rangeZ + 1, 0);

  for (Int i = 0; i < pc.getNumPoint(); i++) {
    listNum[posMax[2] - pc[i][2]]++;
  }

  vector<Int> hist0ToMin = {};
  Int histWidth = 50;

  for (Int hw = posMax[2] + histWidth * 4; hw < (rangeZ - histWidth); hw += histWidth) {
    if (hw > (rangeZ - histWidth)) {
      break;
    }
    Int numPerHistWidth = 0;
    for (Int i = 0; i < histWidth; i++) {
      numPerHistWidth += listNum[hw + i];
    }
    hist0ToMin.push_back(numPerHistWidth);
  }
  auto maxNum = max_element(hist0ToMin.begin(), hist0ToMin.end());
  Int maxPos = distance(hist0ToMin.begin(), maxNum);
  Int groundZPeak = (posMax[2] + 50 * (maxPos + 1));
  vector<Int> hist0ToGroundPeak = {};
  for (Int hw = posMax[2] + histWidth; hw < groundZPeak; hw += histWidth) {
    if (hw > groundZPeak) {
      break;
    }
    Int numPerHistWidth = 0;
    for (Int i = 0; i < histWidth; i++) {
      numPerHistWidth += listNum[hw + i];
    }
    hist0ToGroundPeak.push_back(numPerHistWidth);
  }
  auto minNum = min_element(hist0ToGroundPeak.begin(), hist0ToGroundPeak.end());

  vector<Int> groundZRange;
  Int m = 2;
  for (Int i = 0; i < hist0ToMin.size(); i++) {
    if (hist0ToMin[i] > ((*minNum) * m)) {
      groundZRange.push_back(posMax[2] + 50 * i);
    }
  }
  if (groundZRange.size() == 1 || groundZRange.size() == 0) {
    m = 1.5;
    for (Int i = 0; i < hist0ToMin.size(); i++) {
      if (hist0ToMin[i] > ((*minNum) * m)) {
        groundZRange.push_back(posMax[2] + 50 * i);
      }
    }
  }
  groundZ[0] = posMax[2] - groundZRange[groundZRange.size() - 1];
  groundZ[1] = posMax[2] - groundZRange[0];
}

Void TEncTop::groundSlices(vector<TComPointCloud>& groundList, int32_t dis, int32_t groundZMin,
                           const TComPointCloud pointCloudRes) {
  const TSize pointCount = pointCloudRes.getNumPoint();
  TComPointCloud pcRest = pointCloudRes;
  Int groundNum = groundList.size();
  for (Int n = 0; n < (groundNum - 1); n++) {
    Int count = 0;
    vector<PC_POS> pos_res = {};
    vector<vector<PC_REFL>> refl_res = {};
    groundList[n].setNumPoint(pointCount);
    if (m_hls.sps.attrPresentFlag)
      groundList[n].setNumMultilRefl(pointCloudRes.getNumMultilRefl());
    for (Int i = 0; i < pcRest.getNumPoint(); i++) {
      if (pcRest[i][2] > (groundZMin + dis * n) && pcRest[i][2] < (groundZMin + dis * (n + 1))) {
        groundList[n].setPos(count, pcRest[i]);
        if (m_hls.sps.attrPresentFlag) {
          for (int multiIdx = 0; multiIdx < pcRest.getNumMultilRefl(); ++multiIdx)
            groundList[n].setReflectance(count, pcRest.getReflectance(i, multiIdx), multiIdx);
        }

        count++;
      } else {
        pos_res.push_back(pcRest[i]);
        if (m_hls.sps.attrPresentFlag) {
          vector<PC_REFL> refls;
          for (int multiIdx = 0; multiIdx < pcRest.getNumMultilRefl(); ++multiIdx)
            refls.push_back(pcRest.getReflectance(i, multiIdx));
          refl_res.push_back(refls);
        }
      }
    }
    pcRest.clear();
    pcRest.setPoses(pos_res);
    pcRest.setNumPoint(pos_res.size());
    for (int i; i < pos_res.size(); i++) {
      for (int multiIdx = 0; multiIdx < pcRest.getNumMultilRefl(); ++multiIdx) {
        pcRest.setReflectance(i, refl_res[i][multiIdx], multiIdx);
      }
    }
    if (n == (groundNum - 2)) {
      groundList[n + 1] = pcRest;
      groundList[n].setNumPoint(count);
      if (m_hls.sps.attrPresentFlag)
        groundList[n].setNumMultilRefl(pointCloudRes.getNumMultilRefl());
    } else {
      groundList[n].setNumPoint(count);
      if (m_hls.sps.attrPresentFlag)
        groundList[n].setNumMultilRefl(pointCloudRes.getNumMultilRefl());
    }
  }
}

Void TEncTop::sliceDevisionByHistZ(const TComPointCloud pointCloudOrg,
                                   vector<TComPointCloud>& pointCloudPartitionList, Int numOfSlice,
                                   const Float& geomQs, Int numDigits) {
  const TSize pointCount = pointCloudOrg.getNumPoint();
  TComPointCloud pointCloudRes = pointCloudOrg;

  if (numOfSlice == 1) {
    pointCloudPartitionList[0] = pointCloudRes;
    return;
  }

  ///< pointCloudPartitionList[0] = pointCloudLidar (z = 0)
  Int count = 0;
  vector<PC_POS> pos_res = {};
  vector<vector<PC_REFL>> refl_res = {};
  pointCloudPartitionList[0].setNumPoint(pointCount);
  if (m_hls.sps.attrPresentFlag)
    pointCloudPartitionList[0].setNumMultilRefl(pointCloudRes.getNumMultilRefl());

  for (Int i = 0; i < pointCloudRes.getNumPoint(); i++) {
    if (pointCloudRes[i][2] == 0) {
      pointCloudPartitionList[0].setPos(count, pointCloudRes[i]);
      for (int multiIdx = 0; multiIdx = pointCloudRes.getNumMultilRefl(); ++multiIdx)
        pointCloudPartitionList[0].setReflectance(count, pointCloudRes.getReflectance(i, multiIdx),
                                                  multiIdx);
      count++;
    } else {
      pos_res.push_back(pointCloudRes[i]);
      vector<PC_REFL> refls;
      for (int multiIdx = 0; multiIdx = pointCloudRes.getNumMultilRefl(); ++multiIdx)
        refls.push_back(pointCloudRes.getReflectance(i, multiIdx));
      refl_res.push_back(refls);
    }
  }
  pointCloudPartitionList[0].setNumPoint(count);
  if (m_hls.sps.attrPresentFlag)
    pointCloudPartitionList[0].setNumMultilRefl(pointCloudRes.getNumMultilRefl());
  pointCloudRes.clear();
  pointCloudRes.setPoses(pos_res);
  if (m_hls.sps.attrPresentFlag) {
    for (int i; i < pos_res.size(); i++) {
      for (int multiIdx = 0; multiIdx < pointCloudRes.getNumMultilRefl(); ++multiIdx) {
        pointCloudRes.setReflectance(i, refl_res[i][multiIdx], multiIdx);
      }
    }

  }
  pointCloudRes.setNumPoint(pos_res.size());

  ///< histogramZ generation
  PC_POS posMin, posMax, posRange;
  vector<int32_t> groundZ(2);
  pointCloudRes.computeBoundingBox(posMin, posMax);
  posRange = posMax - posMin;

  histogramZ(posMin, posMax, pointCloudRes, groundZ);

  count = 0;
  pos_res = {};
  refl_res = {};
  int32_t groundZMin = groundZ[0];
  int32_t groundZMax = groundZ[1];
  const TSize pointCnt = pointCloudRes.getNumPoint();
  pointCloudPartitionList[1].setNumPoint(pointCnt);

  if (m_hls.sps.attrPresentFlag)
    pointCloudPartitionList[1].setNumMultilRefl(pointCloudRes.getNumMultilRefl());

  for (Int i = 0; i < pointCloudRes.getNumPoint(); i++) {
    if (pointCloudRes[i][2] < groundZMin || pointCloudRes[i][2] > groundZMax) {
      pointCloudPartitionList[1].setPos(count, pointCloudRes[i]);
      if (m_hls.sps.attrPresentFlag) {
        for (int multiIdx = 0; multiIdx = pointCloudRes.getNumMultilRefl(); ++multiIdx)
          pointCloudPartitionList[1].setReflectance(
            count, pointCloudRes.getReflectance(i, multiIdx), multiIdx);
      }
      count++;
    } else {
      pos_res.push_back(pointCloudRes[i]);
      if (m_hls.sps.attrPresentFlag) {
        vector<PC_REFL> refls;
        for (int multiIdx = 0; multiIdx = pointCloudRes.getNumMultilRefl(); ++multiIdx)
          refls.push_back(pointCloudRes.getReflectance(i, multiIdx));
        refl_res.push_back(refls);
      }
    }
  }
  pointCloudPartitionList[1].setNumPoint(count);
  if (m_hls.sps.attrPresentFlag)
    pointCloudPartitionList[1].setNumMultilRefl(pointCloudRes.getNumMultilRefl());

  pointCloudRes.clear();
  pointCloudRes.setPoses(pos_res);
  if (m_hls.sps.attrPresentFlag) {
    for (int i; i < pos_res.size(); i++) {
      for (int multiIdx = 0; multiIdx < pointCloudRes.getNumMultilRefl(); ++multiIdx) {
        pointCloudRes.setReflectance(i, refl_res[i][multiIdx], multiIdx);
      }
    }
  }
  pointCloudRes.setNumPoint(pos_res.size());

  if (geomQs == 1 && numDigits != 0) {
    ///< lossless geom: pointCloudPartitionList[0] = pointCloudLidar
    ///<                pointCloudPartitionList[1] = pointCloudRest
    ///<                pointCloudPartitionList[i] = pointCloudGround, i = 2...N
    if (numOfSlice == 2) {
      pointCloudPack(pointCloudPartitionList[0], pointCloudPartitionList[1]);
      pointCloudPartitionList[0].setNumPoint(pointCloudPartitionList[0].getNumPoint() +
                                             pointCloudPartitionList[1].getNumPoint());
      pointCloudPartitionList[1] = pointCloudRes;
      pointCloudPartitionList[1].setNumPoint(pointCloudRes.getNumPoint());
    } else if ((numOfSlice - 2) == 1) {
      pointCloudPartitionList[2] = pointCloudRes;
    } else {
      vector<TComPointCloud> groundList(numOfSlice - 2);
      int32_t dis = (groundZMax - groundZMin) / (numOfSlice - 2);
      groundSlices(groundList, dis, groundZMin, pointCloudRes);
      for (Int n = 2; n < numOfSlice; n++) {
        pointCloudPartitionList[n] = groundList[n - 2];
      }
    }

  } else {

    pointCloudPack(pointCloudPartitionList[0], pointCloudPartitionList[1]);
    pointCloudPartitionList[0].setNumPoint(pointCloudPartitionList[0].getNumPoint() +
                                           pointCloudPartitionList[1].getNumPoint());
    if (numOfSlice == 2) {
      pointCloudPartitionList[1] = pointCloudRes;
    } else {
      vector<TComPointCloud> groundList(numOfSlice - 1);
      int32_t dis = (groundZMax - groundZMin) / (numOfSlice - 1);

      groundSlices(groundList, dis, groundZMin, pointCloudRes);

      for (Int n = 1; n < numOfSlice; n++) {
        pointCloudPartitionList[n] = groundList[n - 1];
      }
    }
  }
}

Void TEncTop::SliceDevisionByPointNum(vector<TComPointCloud>& pointCloudPartitionList,
                                      UInt maxPointNumOfSlicesLog2) {
  // Hilbert sorting
  int64_t voxelCount = m_pointCloudQuant.getNumPoint();
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(m_pointCloudQuant.positions(), 1, pointCloudCode, voxelCount, 1);

  // compute slicePointNum and sliceNum
  int64_t slicePointNum = voxelCount;
  int64_t sliceNum = 1;
  while (slicePointNum > int64_t(1) << maxPointNumOfSlicesLog2) {
    slicePointNum = (slicePointNum >> 1) + 1;
    sliceNum = sliceNum << 1;
  }
  pointCloudPartitionList.resize(sliceNum);

  // cteate slices
  for (int slice_id = 0; slice_id < sliceNum; slice_id++) {
    int64_t begin = std::min(slice_id * slicePointNum, voxelCount);
    int64_t end = std::min(slice_id * slicePointNum + slicePointNum, voxelCount);
    pointCloudPartitionList[slice_id].setNumPoint(end - begin);
    if (m_pointCloudQuant.hasColors()) {
      pointCloudPartitionList[slice_id].setNumMultilColor(m_pointCloudQuant.getNumMultilColor());
      pointCloudPartitionList[slice_id].addColors();
    }
    if (m_pointCloudQuant.hasReflectances()) {
      pointCloudPartitionList[slice_id].setNumMultilRefl(m_pointCloudQuant.getNumMultilRefl());
      pointCloudPartitionList[slice_id].addReflectances();
    }
    for (int i = begin, j = 0; i < end; i++, j++) {
      pointCloudPartitionList[slice_id][j] = m_pointCloudQuant[pointCloudCode[i].index];
      if (m_pointCloudQuant.hasColors()) {
        for (int multiIdx = 0; multiIdx < m_pointCloudQuant.getNumMultilColor(); ++multiIdx) {
          pointCloudPartitionList[slice_id].setColor(
            j, m_pointCloudQuant.getColor(pointCloudCode[i].index, multiIdx), multiIdx);
        }
      }
      if (m_pointCloudQuant.hasReflectances()) {
        for (int multiIdx = 0; multiIdx < m_pointCloudQuant.getNumMultilRefl(); ++multiIdx)
          pointCloudPartitionList[slice_id].setReflectance(
            j, m_pointCloudQuant.getReflectance(pointCloudCode[i].index, multiIdx), multiIdx);
      }
    }
  }
  pointCloudPartitionList.erase(
    std::remove_if(pointCloudPartitionList.begin(), pointCloudPartitionList.end(),
                   [](const TComPointCloud& partion) { return partion.getNumPoint() == 0; }),
    pointCloudPartitionList.end());
  m_hls.frameHead.num_slice_minus_one = pointCloudPartitionList.size() - 1;
}

///< \}
