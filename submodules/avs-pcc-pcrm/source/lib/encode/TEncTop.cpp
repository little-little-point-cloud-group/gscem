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
  UInt64 prevFrameBits = 0;

  ///< init geom rate control para (cat2)
 
  Double control_a = 28.9;     // geom rate control  model para a
  Double control_b = -0.166;   //geom rc control model para b
  Double control_c = -10.143;  //geom rc control model para c

  ///< init attribute rate control para (cat2)

  double control_ap = 55;
  double control_bp = -2;

  for (UInt i_frame = 0; i_frame < m_numOfFrames; i_frame++) {
    m_frame_ID = i_frame;
    EncoderStatistics esCurFrame;
    printFrameCutoffRule(i_frame, numDigits);
    if (m_splitBinFlag) {
      prevFrameBits = 0;
    }

    if (i_frame == 0) {
      resetBitstream_Recon_FileName(m_startFrame, numDigits,
                                    m_splitBinFlag);  // reset m_bitstreamFileName & m_reconFileName
    } else {
      updateIOFileName(
        m_startFrame + i_frame, numDigits,
        m_splitBinFlag);  // update m_inputFileName & m_bitstreamFileName & m_reconFileName
    }

    if (!m_pointCloudOrg.readFromFile(m_inputFileName, !m_hls.sps.attrPresentFlag) ||
        m_pointCloudOrg.getNumPoint() == 0) {
      cerr << "Error: failed to open ply file: " << m_inputFileName << " !" << endl;
      exitState |= EXIT_FAILURE;
      continue;  // if can not open the corresponding file then skip and move on to find next input file
    }

    ///< determine if only geometry
    if (!m_pointCloudOrg.hasColors() && !m_pointCloudOrg.hasReflectances()) {
      m_hls.sps.attrPresentFlag = 0;
    }

    clock_t userTimeTotalBegin = clock();

   



    if (i_frame == 0 || m_splitBinFlag) {
      m_bitstreamFile.open(m_bitstreamFileName, fstream::binary | fstream::out);
      if (!m_bitstreamFile) {
        cerr << "Error: failed to open bitstream file " << m_bitstreamFileName << " for writing!"
             << endl;
        exitState |= EXIT_FAILURE;
      }



      ///< calculate geom qs  
      Double qg_max = 800;
      Double qg_min = 1;
      Double temp_rg;
      
      while (abs(qg_max - qg_min) > 0.01) {
        temp_rg = control_a * pow((qg_max + qg_min) / 2, control_b) + control_c;
        if (temp_rg > m_geomTarbpp)
          qg_min = (qg_max + qg_min) / 2;
        if (temp_rg <= m_geomTarbpp)
          qg_max = (qg_max + qg_min) / 2;
      }

      ///< set geom qs 
      if (m_geomTarbpp != 0)
      {
        m_hls.gps.geomQuantStep = 1.0 * int(qg_max * 100000) / 100000;
        unsigned ns = 0;
        unsigned mask = 1;
        unsigned U1 = m_hls.gps.geomQuantStep;
        while (U1 > 1) {
          ns++;
          U1 = U1 >> 1;
        }
        unsigned U = m_hls.gps.geomQuantStep * pow(2, 20 - ns) + 0.5;
        while (U & mask == 0) {
          ns++;
          U = U >> 1;
        }
        unsigned n = 20 - ns;
        m_hls.gps.geomQuantStepSignificand = U;
        m_hls.gps.geomQuantStepExponent = n;
      }
        

      ///< calculate attribute qp 
      ///< set attribute para b 
      if (i_frame ==0 ) {
        control_bp = -0.003 * m_hls.gps.geomQuantStep - 0.6;

      }
      ///< set attribute qp  
      if (m_attrTarbpp !=0)
        m_hls.aps.reflQuantParam = m_hls.aps.colorQuantParam = control_ap * exp(control_bp * m_attrTarbpp) + 0.5;



      ///< encoding sequence parameter set
      m_bufferChunk.setBufferType(BufferChunkType::BCT_SPS);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_encBac.codeSPS(m_hls.sps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();

      ///< encoding geometry parameter set
      m_bufferChunk.setBufferType(BufferChunkType::BCT_GPS);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_encBac.codeGPS(m_hls.gps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();

      // encoding attribute parameter set
      if (m_hls.sps.attrPresentFlag) {
        auto& aps = m_hls.aps;
        if (m_pointCloudOrg.hasColors()) {
          aps.attributeDataPresentFlag[0] = 1;
          aps.outputBitDepthMinus1[0] = m_hls.aps.colorOutputDepth - 1;
          aps.attrQuantParam[0] = m_hls.aps.colorQuantParam;
          UInt colorInitOutputDepth = aps.colorOutputDepth - aps.colorInitGolombOffset;
          m_pointCloudOrg.computeColorRes(aps.colorGolombNum, colorInitOutputDepth,
                                          m_hls.aps.colorQuantParam);
        }
        if (m_pointCloudOrg.hasReflectances()) {
          aps.attributeDataPresentFlag[1] = 1;
          aps.outputBitDepthMinus1[1] = m_hls.aps.reflOutputDepth - 1;
          aps.attrQuantParam[1] = m_hls.aps.reflQuantParam;
          UInt reflInitOutputDepth = aps.reflOutputDepth - aps.reflInitGolombOffset;
          m_pointCloudOrg.computeReflRes(aps.reflGolombNum, reflInitOutputDepth,
                                         m_hls.aps.reflQuantParam);
        }
        m_hls.aps.multiDataSetFlag[0] =
          aps.attributeDataPresentFlag[0] ? m_pointCloudOrg.getNumMultilColor() > 0 : false;
        m_hls.aps.multiDataSetFlag[1] =
          aps.attributeDataPresentFlag[1] ? m_pointCloudOrg.getNumMultilRefl() > 0 : false;

        m_hls.aps.attributeDataNumSetMinus1[0] =
          m_hls.aps.multiDataSetFlag[0] ? m_pointCloudOrg.getNumMultilColor() - 1 : -1;
        m_hls.aps.attributeDataNumSetMinus1[1] =
          m_hls.aps.multiDataSetFlag[1] ? m_pointCloudOrg.getNumMultilRefl() - 1 : -1;

        m_hls.aps.attributeInfoNumSetMinus1[0] =
          m_hls.aps.multiDataSetFlag[0] ? m_hls.aps.attributeDataNumSetMinus1[0] : -1;
        m_hls.aps.attributeInfoNumSetMinus1[1] =
          m_hls.aps.multiDataSetFlag[1] ? m_hls.aps.attributeDataNumSetMinus1[1] : -1;
        FixedMultiAPs(m_hls.sps, m_hls.aps);
        m_hls.aps.eligibleDupPointPred = true;
        if (m_hls.sps.attrPresentFlag) {
          if (m_hls.aps.attributeDataPresentFlag[0] && m_hls.aps.attributeDataPresentFlag[1])
            m_hls.aps.eligibleDupPointPred = false;
          else
            m_hls.aps.eligibleDupPointPred = true;
        } else
          m_hls.aps.eligibleDupPointPred = false;

        m_bufferChunk.setBufferType(BufferChunkType::BCT_APS);
        m_encBac.setBitstreamBuffer(m_bufferChunk);
        m_encBac.codeAPS(m_hls.aps, m_hls.sps);
        m_encBac.encodeFinish();
        m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
        m_bufferChunk.reset();
        m_encBac.reset();

        if (aps.transform == 1) {
          FXPoint::set_kFracBits(aps.kFracBits);
        }
      }
      ///< encoding userdata start (turn off)
      if (false) {
        m_bufferChunk.setBufferType(BufferChunkType::BCT_UDA);
        m_encBac.setBitstreamBuffer(m_bufferChunk);
        m_encBac.codeUserData();
        m_encBac.encodeFinish();
        m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
        m_bufferChunk.reset();
        m_encBac.reset();
      }
      ///< encoding userdata end
    }

    ///< preprocessing
    vector<PC_COL> pointCloudOrgColors = m_pointCloudOrg.getColors();
    if (m_colorTransformFlag && m_pointCloudOrg.hasColors()) {
      m_pointCloudOrg.convertRGBToYUV();
    }
    ///< init frame header
    initFrameParameters(m_hls.frameheader);

    ///< quantization
    geomPreprocessAndQuantization(m_hls.frameheader.geomNumPoints, m_hls.gps.geomQuantStep,
                                  esCurFrame.recolorUserTime);
    // adjust colorInitPredTransRatio
    if (m_hls.aps.attributeDataPresentFlag[0]) {
      UInt64 BBmax = std::max(std::max(m_hls.frameheader.geomBoundingBoxSize[0],
                                       m_hls.frameheader.geomBoundingBoxSize[1]),
                              m_hls.frameheader.geomBoundingBoxSize[2]);
      UInt64 BBmin = std::min(std::min(m_hls.frameheader.geomBoundingBoxSize[0],
                                       m_hls.frameheader.geomBoundingBoxSize[1]),
                              m_hls.frameheader.geomBoundingBoxSize[2]);
      if (BBmax / BBmin < 2)
        m_hls.abh.colorInitPredTransRatio++;
      // calculate scalar of adjust color QP per point tools
      if (m_hls.aps.colorQPAdjustFlag) {
        int ratio = m_pointCloudOrg.getNumPoint() / m_pointCloudQuant.getNumPoint();
        m_hls.abh.colorQPAdjustScalar = ratio * 2;
        m_hls.abh.colorQPAdjustScalar *= m_hls.abh.colorQPAdjustScalar;
      }
    }

    init_aec_context_tab();
    m_encBac.computeBufferSize(m_pointCloudOrg.getNumPoint());

    ///< recolor
    if (m_hls.sps.geomRemoveDuplicateFlag && m_hls.sps.recolorMode == 0) {
      clock_t userTimeRecolorBegin = clock();
      recolor(m_pointCloudOrg, float(1.0 / m_hls.gps.geomQuantStep),
              m_hls.frameheader.geomBoundingBoxOrigin, &m_pointCloudRecon);
      esCurFrame.recolorUserTime += (Double)(clock() - userTimeRecolorBegin) / CLOCKS_PER_SEC;
    }

    TComPointCloud pointCloudRecon;
    pointCloudRecon.setNumPoint(0);
    vector<TComPointCloud> pointCloudPartitionList(1);
    if (!m_sliceFlag) {
      m_sliceOrigin = {0, 0, 0};
      m_sliceID = 0;
      m_sliceBoundingBox = m_hls.frameheader.geomBoundingBoxSize;
    } else {
      // dividing slice
      SliceDevisionByPointNum(pointCloudPartitionList, m_maxPointNumOfSlicesLog2);
    }
    // encode frameHeader
    m_bufferChunk.setBufferType(BufferChunkType::BCT_FRAME);
    m_encBac.setBitstreamBuffer(m_bufferChunk);
    m_encBac.codeFrameHeader(m_hls.frameheader);
    m_encBac.encodeFinish();
    m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
    m_bufferChunk.reset();
    m_encBac.reset();

    ///< encoding userdata start (turn off)
    if (false) {
      m_bufferChunk.setBufferType(BufferChunkType::BCT_UDA);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_encBac.codeUserData();
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();
    }
    ///< encoding userdata end

    if (!m_sliceFlag) {
      encodeSlice(esCurFrame);
    } else {
      for (Int slice_id = 0; slice_id < m_hls.frameheader.frameNumSliceMinus1 + 1; slice_id++) {
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
    esCurFrame.totalBits = m_bitstreamFile.tellp() * 8 - prevFrameBits;
    std::cout << "Total bitstream size " << esCurFrame.totalBits << " bits" << std::endl;
    prevFrameBits += esCurFrame.totalBits;

    esCurFrame.totalUserTime = (Double)(clock() - userTimeTotalBegin) / CLOCKS_PER_SEC;
    if (m_hls.sps.attrPresentFlag) {
      esCurFrame.colorUserTime = m_attrEncoder.getColorTime();
      esCurFrame.reflUserTime = m_attrEncoder.getReflectanceTime();
      if (m_hls.aps.attributeDataPresentFlag[0] && m_hls.aps.attributeDataPresentFlag[1]) {
        esCurFrame.colorUserTime =
          (esCurFrame.colorUserTime + esCurFrame.attrUserTime - esCurFrame.reflUserTime) / 2;
        esCurFrame.reflUserTime =
          (esCurFrame.reflUserTime + esCurFrame.attrUserTime - esCurFrame.colorUserTime) / 2;
      } else if (m_hls.aps.attributeDataPresentFlag[0]) {
        esCurFrame.colorUserTime = esCurFrame.attrUserTime;
      } else if (m_hls.aps.attributeDataPresentFlag[1]) {
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

    ///< geom para update
    Double real_geom_bpp = esCurFrame.geomBits * 1.0 / m_pointCloudOrg.getNumPoint();
    Double comp_geom_bpp = control_a * pow(m_hls.gps.geomQuantStep, control_b) + control_c;
    control_a = control_a + 0.003 * (log(real_geom_bpp) - log(comp_geom_bpp)) * control_a; 
    control_b = control_b + 0.003 * (log(real_geom_bpp) - log(comp_geom_bpp)) * log(m_hls.gps.geomQuantStep);
    control_c = control_c + 0.006 * (real_geom_bpp - comp_geom_bpp);
    ///< attribute para update
    double real_refl_bpp = 1.0 * esCurFrame.reflBits / m_pointCloudOrg.getNumPoint();
    double temp_b = log(m_hls.aps.reflQuantParam * 1.0 / 55) / real_refl_bpp;
    ///< attribute para clip
    if (temp_b - control_bp > 8)
      control_bp = control_bp + 8;
    else {
      if (control_bp - temp_b > 8)
        control_bp = control_bp - 8;
      else
        control_bp = temp_b;
    }

    geomPostprocessingAndDequantization();
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

    if (i_frame == m_numOfFrames - 1 || m_splitBinFlag) {
      //< write the sps_end_code
      m_bufferChunk.writeFinalCodeToBitstream(&m_bitstreamFile);
      esTotal.totalBits = m_bitstreamFile.tellp() * 8;
      m_bitstreamFile.close();
    }
  }

  cout << "All frames number of output points: " << esTotal.numReconPoints << endl;
  cout << "All frames geometry bits: " << esTotal.geomBits << " bits." << endl;
  cout << "All frames color bits: " << esTotal.colorBits << " bits." << endl;
  cout << "All frames refl bits: " << esTotal.reflBits << " bits." << endl;
  if (m_hls.aps.attributeDataPresentFlag[0])
    for (int multiIdx = 0; multiIdx < m_hls.aps.attributeDataNumSetMinus1[0] + 1; ++multiIdx)
      cout << "MultiData " << multiIdx
           << " All frames attributes bits: " << esTotal.attrBits[multiIdx] << " bits." << endl;
  else if (m_hls.aps.attributeDataPresentFlag[1]) {
    if (m_hls.aps.attributeDataNumSetMinus1[1] == 0)
      for (int multiIdx = 0; multiIdx < m_hls.aps.attributeDataNumSetMinus1[1] + 1; ++multiIdx)
        cout << "MultiData " << multiIdx
             << " All frames attributes bits: " << esTotal.attrBits[multiIdx] << " bits." << endl;
    else {
      for (int multiIdx = 0, indexGroup = 0; multiIdx < m_hls.aps.attributeDataNumSetMinus1[1] + 1;
           indexGroup++) {
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
    totalMetricRes.print(MetricParam, m_hls.aps.attributeDataNumSetMinus1[0] + 1,
                         m_hls.aps.attributeDataNumSetMinus1[1] + 1);
  }
  if (md5FileOpenFlag)
    md5Calculator.closeFile();
  return exitState;
}

Void TEncTop::getSingleAttrAPs(const SequenceParameterSet& sps, AttributeParameterSet& aps,
                               const int& attrIdx, const int& multiIdx) {
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

Void TEncTop::FixedMultiAPs(const SequenceParameterSet& sps, AttributeParameterSet& aps) {
  ///< if multil attribute parameyers have parsed from the configuration
  if (!aps.updateMultilAttrParams) {
    for (int attrIdx = 0; attrIdx < (sps.maxNumAttributesMinus1 + 1); attrIdx++) {
      if (aps.attributeDataPresentFlag[attrIdx]) {
        for (int multiIdx = 0; multiIdx < aps.attributeInfoNumSetMinus1[attrIdx] + 1; ++multiIdx) {
          aps.outputMultiBitDepthMinus1[attrIdx][multiIdx] = aps.outputBitDepthMinus1[attrIdx];
          aps.attrMultiQuantParam[attrIdx][multiIdx] = aps.attrQuantParam[attrIdx];
          if (attrIdx == 0) {
            aps.orderMultiSwitch[multiIdx] = aps.orderSwitch;
            aps.colorMultiReordermode[multiIdx] = aps.colorReorderMode;
            aps.colorMultiGolombNum[multiIdx] = aps.colorGolombNum;
            aps.golombMultiGroupSizeLog2[multiIdx] = aps.golombGroupSizeLog2;
          }
          if (attrIdx == 1) {
            aps.axisMultiBiasMinus1[multiIdx] = aps.axisBiasMinus1;
            aps.reflMultiReordermode[multiIdx] = aps.reflReorderMode;
            aps.reflMultiGolombNum[multiIdx] = aps.reflGolombNum;
            aps.predMultiFixedPointFracBit[multiIdx] = aps.predFixedPointFracBit;
          }
          aps.transformMulti[attrIdx][multiIdx] = aps.transform;

          if ((aps.transformMulti[attrIdx][multiIdx] == 0) ||
              (aps.transformMulti[attrIdx][multiIdx] == 2)) {
            aps.maxMultiNumOfNeighboursLog2Minus7[attrIdx][multiIdx] =
              aps.maxNumOfNeighboursLog2Minus7;
            if (attrIdx == 0) {
              aps.crossMultiComponentPred[multiIdx] = aps.crossComponentPred;
              aps.chromaMultiQpOffsetCb[multiIdx] = aps.chromaQpOffsetCb;
              aps.chromaMultiQpOffsetCr[multiIdx] = aps.chromaQpOffsetCr;
            }
            if (attrIdx == 1) {
              aps.nearestMultiPredParam1[multiIdx] = aps.nearestPredParam1;
              aps.nearestMultiPredParam2[multiIdx] = aps.nearestPredParam2;
              aps.predDistWeightMultiGroupSizeLog2[multiIdx] = aps.predDistWeightGroupSizeLog2;
            }
          }
          if (aps.transformMulti[attrIdx][multiIdx] == 1) {
            aps.kMultiFracBits[attrIdx][multiIdx] = aps.kFracBits;
            aps.attrMultiTransformQpDelta[attrIdx][multiIdx] = aps.attrTransQpDelta;
            aps.transformMultiSegmentSize[attrIdx][multiIdx] = aps.transformSegmentSize;
            aps.transMultiResLayer[attrIdx][multiIdx] = aps.transResLayer;
          }
          if (aps.transformMulti[attrIdx][multiIdx] == 2) {
            aps.MultimaxNumofCoeffLog2Minus8[attrIdx][multiIdx] = aps.maxNumofCoeffLog2Minus8;
            if (aps.MultimaxNumofCoeffLog2Minus8[attrIdx][multiIdx]) {
              aps.maxMultiNumofCoeff[attrIdx][multiIdx] = 1
                << (aps.MultimaxNumofCoeffLog2Minus8[attrIdx][multiIdx] + 8);
            }
            aps.QpMultiOffsetDC[attrIdx][multiIdx] = aps.QpOffsetDC;
            aps.QpMultiOffsetAC[attrIdx][multiIdx] = aps.QpOffsetAC;
            if (attrIdx == 0) {
              aps.colorMaxMultiTransNum[multiIdx] = aps.colorMaxTransNum;
              aps.chromaMultiQpOffsetDC[multiIdx] = aps.chromaQpOffsetDC;
              aps.chromaMultiQpOffsetAC[multiIdx] = aps.chromaQpOffsetAC;
              aps.colorMultiQPAdjustFlag[multiIdx] = aps.colorQPAdjustFlag;
            }
            if (attrIdx == 1) {
              aps.reflMaxMultiTransNum[multiIdx] = aps.reflMaxTransNum;
              aps.reflMultiGroupPredict[multiIdx] = aps.reflGroupPredict;
            }
          }

          aps.coeffMultiLengthControlLog2Minus8[attrIdx][multiIdx] =
            aps.coeffLengthControlLog2Minus8;
          if (aps.coeffMultiLengthControlLog2Minus8[attrIdx][multiIdx]) {
            aps.coeffMultiLengthControl[attrIdx][multiIdx] = 1
              << (aps.coeffMultiLengthControlLog2Minus8[attrIdx][multiIdx] + 8);
          }
        }
      }
    }
  }

  // set multiAttrGroupID and multiAttriGroupNum
  if (aps.attributeDataPresentFlag[1] && (aps.attributeInfoNumSetMinus1[1] > 0) &&
      aps.transform == 0) {
    for (int multiIdx = 0; multiIdx < aps.attributeInfoNumSetMinus1[1] + 1; ++multiIdx) {
      if (multiIdx > 0 && aps.multiAttrGroupID[multiIdx] < aps.multiAttrGroupID[multiIdx - 1])
        break;
      aps.multiAttriGroupNum[aps.multiAttrGroupID[multiIdx]]++;
    }
  }
}

Void TEncTop::encodeSlice(EncoderStatistics& es) {
  auto& aps = m_hls.aps;

  ///< node size (log2) for xyz dimensions
  UInt maxBB = std::max({1U, m_sliceBoundingBox[0], m_sliceBoundingBox[1], m_sliceBoundingBox[2]});
  V3<UInt> nodeSizeLog2;
  if (m_hls.gps.implicitGeomPartitionFlag) {
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
    m_hls.gbh.contextMode = 1;
    m_hls.gbh.imQtbtNumBeforeOt = m_hls.gps.imQtbtNumBeforeOt;
    m_hls.gbh.imQtbtMinSize = m_hls.gps.imQtbtMinSize;
  } else {
    m_hls.gbh.contextMode = 0;
    m_hls.gbh.imQtbtNumBeforeOt = 0;
    m_hls.gbh.imQtbtMinSize = 0;
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

  ///< start attribute coding
  if (m_hls.sps.attrPresentFlag) {
    int colorBitstreamSzie[NUM_MULTIATTRIBUTE] = {0};
    int refBitstreamSzie[NUM_MULTIATTRIBUTE] = {0};
    clock_t userTimeAttributeBegin = clock();
    m_attrEncoder.getColorBits() = 0;
    m_attrEncoder.getReflectanceBits() = 0;
    m_attrEncoder.getColorTime() = 0;
    m_attrEncoder.getReflectanceTime() = 0;
    multiDataID = 0;
    if (m_hls.aps.attributeDataPresentFlag[0] && m_hls.aps.attributeDataPresentFlag[1]) {
      getSingleAttrAPs(m_hls.sps, m_hls.aps, 0, multiDataID);
      getSingleAttrAPs(m_hls.sps, m_hls.aps, 1, multiDataID);
      compressAndEncodeAttribute();
      colorBitstreamSzie[multiDataID] = m_attrEncoder.getColorBits();
      refBitstreamSzie[multiDataID] = m_attrEncoder.getReflectanceBits();
      m_bufferChunk.reset();
      m_encBac.reset();
    }

    if (m_hls.aps.attributeDataPresentFlag[0] && !m_hls.aps.attributeDataPresentFlag[1]) {
      while (multiDataID < m_hls.aps.attributeDataNumSetMinus1[0] + 1) {
        //encoding attribute brick header
        m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH_COL);
        m_encBac.setBitstreamBuffer(m_bufferChunk);
        m_hls.abh.sliceID = m_sliceID;
        m_hls.abh.attributeID = multiDataID;
        m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
        m_encBac.encodeFinish();
        m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
        m_bufferChunk.reset();
        m_encBac.reset();

        ///< start attribute coding
        getSingleAttrAPs(m_hls.sps, m_hls.aps, 0, multiDataID);
        m_bufferChunk.setBufferType(BufferChunkType::BCT_COL);
        m_encBac.setBitstreamBuffer(m_bufferChunk);
        m_encBac.initBac();

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

    if (!m_hls.aps.attributeDataPresentFlag[0] && m_hls.aps.attributeDataPresentFlag[1]) {
      int multiDataGroupNum = 1, multiDataGroupIndex = 0;
      if (m_hls.aps.attributeDataNumSetMinus1[1] > 0)
        m_encBac.computeBufferSize(m_pointCloudOrg.getNumPoint(), 5);
      while (multiDataID < m_hls.aps.attributeDataNumSetMinus1[1] + 1) {
        //encoding attribute brick header
        m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH_REFL);
        m_encBac.setBitstreamBuffer(m_bufferChunk);
        m_hls.abh.sliceID = m_sliceID;
        m_hls.abh.attributeID = multiDataID;
        m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
        m_encBac.encodeFinish();
        m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
        m_bufferChunk.reset();
        m_encBac.reset();

        ///< start attribute coding
        getSingleAttrAPs(m_hls.sps, m_hls.aps, 1, multiDataID);
        if (m_hls.aps.attributeDataNumSetMinus1[1] > 0 && m_hls.aps.transform == 0) {
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
    if (m_hls.aps.attributeDataPresentFlag[0]) {
      for (int multiIdx = 0; multiIdx < m_hls.aps.attributeDataNumSetMinus1[0] + 1; ++multiIdx)
        getAttributeBits[multiIdx] = colorBitstreamSzie[multiIdx];
      for (int multiIdx = 0; multiIdx < m_hls.aps.attributeDataNumSetMinus1[0] + 1; ++multiIdx)
        es.attrBits[multiIdx] += colorBitstreamSzie[multiIdx];
    }
    if (m_hls.aps.attributeDataPresentFlag[1]) {
      for (int multiIdx = 0; multiIdx < m_hls.aps.attributeDataNumSetMinus1[1] + 1; ++multiIdx)
        getAttributeBits[multiIdx] = refBitstreamSzie[multiIdx];
      for (int multiIdx = 0; multiIdx < m_hls.aps.attributeDataNumSetMinus1[1] + 1; ++multiIdx)
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

Void TEncTop::initFrameParameters(FrameHeader& frameheader) {
  frameheader.frameIndex = m_frame_ID;
  frameheader.frameNumSliceMinus1 = 0;

  ///< determine bounding box
  PC_POS bbMin, bbMax, bbSize;
  m_pointCloudOrg.computeBoundingBox(bbMin, bbMax);
  for (Int k = 0; k < 3; k++) {
    bbMin[k] = floor(bbMin[k]);
    frameheader.geomBoundingBoxOrigin[k] = Int(bbMin[k]);
  }
  bbSize = bbMax - bbMin;
  frameheader.geomBoundingBoxSize[0] = Int(round(bbSize[0] / m_hls.gps.geomQuantStep)) + 1;
  frameheader.geomBoundingBoxSize[1] = Int(round(bbSize[1] / m_hls.gps.geomQuantStep)) + 1;
  frameheader.geomBoundingBoxSize[2] = Int(round(bbSize[2] / m_hls.gps.geomQuantStep)) + 1;
  frameheader.geomNumPoints = (UInt)m_pointCloudOrg.getNumPoint();

  ///< when lcuNodeSizeLog2 == 0, lcuNodeDepth is used to control the node size
  frameheader.lcuNodeSizeLog2 = m_hls.gps.lcuNodeSizeLog2;
  if (frameheader.lcuNodeSizeLog2 == 0 && m_hls.gps.lcuNodeDepth > 0) {
    UInt maxBB = std::max({1U, m_hls.frameheader.geomBoundingBoxSize[0],
                           m_hls.frameheader.geomBoundingBoxSize[1],
                           m_hls.frameheader.geomBoundingBoxSize[2]});
    int maxNodeSizeLog2 = ceilLog2(maxBB);

    //avoid small lcu size
    frameheader.lcuNodeSizeLog2 = std::max(10U, maxNodeSizeLog2 + 1 - m_hls.gps.lcuNodeDepth);
  }
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
  posDequant[0] += m_hls.frameheader.geomBoundingBoxOrigin[0];
  posDequant[1] += m_hls.frameheader.geomBoundingBoxOrigin[1];
  posDequant[2] += m_hls.frameheader.geomBoundingBoxOrigin[2];

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
    m_pointCloudQuant[i][0] -= m_hls.frameheader.geomBoundingBoxOrigin[0];
    m_pointCloudQuant[i][1] -= m_hls.frameheader.geomBoundingBoxOrigin[1];
    m_pointCloudQuant[i][2] -= m_hls.frameheader.geomBoundingBoxOrigin[2];
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

Void TEncTop::geomPostprocessingAndDequantization() {
  const TSize numPoints = m_pointCloudRecon.getNumPoint();

  ///< geometry dequantization
  for (TSize i = 0; i < numPoints; i++) {
    m_pointCloudRecon[i][0] = m_pointCloudRecon[i][0] * m_hls.gps.geomQuantStepSignificand /
      (1 << m_hls.gps.geomQuantStepExponent);
    m_pointCloudRecon[i][1] = m_pointCloudRecon[i][1] * m_hls.gps.geomQuantStepSignificand /
      (1 << m_hls.gps.geomQuantStepExponent);
    m_pointCloudRecon[i][2] = m_pointCloudRecon[i][2] * m_hls.gps.geomQuantStepSignificand /
      (1 << m_hls.gps.geomQuantStepExponent);
  }

  ///< shift back to world coordinates
  for (TSize i = 0; i < numPoints; i++) {
    m_pointCloudRecon[i][0] += m_hls.frameheader.geomBoundingBoxOrigin[0];
    m_pointCloudRecon[i][1] += m_hls.frameheader.geomBoundingBoxOrigin[1];
    m_pointCloudRecon[i][2] += m_hls.frameheader.geomBoundingBoxOrigin[2];
  }
}

Void TEncTop::compressAndEncodeAttribute() {
  if (m_hls.aps.transform == 0 ||
      m_hls.aps.transform == 2) {  //encode attribute with two entropy codecs
    m_attrEncoder.initDual(&m_pointCloudRecon, &m_pointCloudRecon, &m_hls, &m_encBac, &m_encBacDual,
                           m_frame_ID, m_numOfFrames, multiDataID);
    TComBufferChunk bufferChunkDual;
    if (m_hls.aps.crossAttrTypePred &&
        m_hls.aps.attrEncodeOrder) {  //encode reflectance first, color sencond
      //write reflectance slice header
      bufferChunkDual.setBufferType(BufferChunkType::BCT_ABH_REFL);
      m_encBacDual.setBitstreamBuffer(bufferChunkDual, true);
      m_hls.abh.sliceID = m_sliceID;
      m_hls.abh.attributeID = multiDataID;
      m_encBacDual.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
      m_encBacDual.encodeFinish();
      bufferChunkDual.writeToBitstream(&m_bitstreamFile, m_encBacDual.getBitStreamLength());
      bufferChunkDual.reset();
      m_encBacDual.reset();
      //init reflectance slice data
      bufferChunkDual.setBufferType(BufferChunkType::BCT_REFL);
      m_encBacDual.setBitstreamBuffer(bufferChunkDual, true);
      m_encBacDual.initBac();
      //init color slice data
      m_bufferChunk.setBufferType(BufferChunkType::BCT_COL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_encBac.initBac();
      //encode attribute
      m_attrEncoder.dualEncodeAttribute();
      //write reflectance slice data
      m_encBacDual.encodeTerminationFlag();
      m_encBacDual.encodeFinish();
      bufferChunkDual.writeToBitstream(&m_bitstreamFile, m_encBacDual.getBitStreamLength());
      m_attrEncoder.getReflectanceBits() = m_encBacDual.getBitStreamLength() * 8;
      bufferChunkDual.reset();
      m_encBacDual.reset();
      //write color slice header
      bufferChunkDual.setBufferType(BufferChunkType::BCT_ABH_COL);
      m_encBacDual.setBitstreamBuffer(bufferChunkDual, true);
      m_hls.abh.sliceID = m_sliceID;
      m_hls.abh.attributeID = multiDataID;
      m_encBacDual.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
      m_encBacDual.encodeFinish();
      bufferChunkDual.writeToBitstream(&m_bitstreamFile, m_encBacDual.getBitStreamLength());
      bufferChunkDual.reset();
      m_encBacDual.reset();
      //write color slice data
      m_encBac.encodeTerminationFlag();
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_attrEncoder.getColorBits() = m_encBac.getBitStreamLength() * 8;
      m_bufferChunk.reset();
      m_encBac.reset();
    } else {  //encode color first, reflectance sencond
      //write color slice header
      m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH_COL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_hls.abh.sliceID = m_sliceID;
      m_hls.abh.attributeID = multiDataID;
      m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();
      //init color slice data
      m_bufferChunk.setBufferType(BufferChunkType::BCT_COL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_encBac.initBac();
      //init reflectance slice data
      bufferChunkDual.setBufferType(BufferChunkType::BCT_REFL);
      m_encBacDual.setBitstreamBuffer(bufferChunkDual, true);
      m_encBacDual.initBac();
      //encode attribute
      m_attrEncoder.dualEncodeAttribute();
      //write color slice data
      m_encBac.encodeTerminationFlag();
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_attrEncoder.getColorBits() = m_encBac.getBitStreamLength() * 8;
      m_bufferChunk.reset();
      m_encBac.reset();
      //write reflectance slice header
      m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH_REFL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_hls.abh.sliceID = m_sliceID;
      m_hls.abh.attributeID = multiDataID;
      m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();
      //write reflectance slice data
      m_encBacDual.encodeTerminationFlag();
      m_encBacDual.encodeFinish();
      bufferChunkDual.writeToBitstream(&m_bitstreamFile, m_encBacDual.getBitStreamLength());
      m_attrEncoder.getReflectanceBits() = m_encBacDual.getBitStreamLength() * 8;
      bufferChunkDual.reset();
      m_encBacDual.reset();
    }
  } else if (m_hls.aps.transform == 1) {  //encode attribute with one entropy codec
    m_attrEncoder.init(&m_pointCloudRecon, &m_pointCloudRecon, &m_hls, &m_encBac, m_frame_ID,
                       m_numOfFrames, multiDataID);

    if (!m_hls.aps.crossAttrTypePred) {
      //write color slice header
      m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH_COL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_hls.abh.sliceID = m_sliceID;
      m_hls.abh.attributeID = multiDataID;
      m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();
      // encode color
      TComBufferChunk bufferChunkCol(BufferChunkType::BCT_COL);
      m_encBac.setBitstreamBuffer(bufferChunkCol);
      m_encBac.initBac();
      m_attrEncoder.transformEncodeColor();
      m_encBac.encodeTerminationFlag();
      m_encBac.encodeFinish();
      m_attrEncoder.getColorBits() = m_encBac.getBitStreamLength() * 8;
      bufferChunkCol.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      bufferChunkCol.reset();
      m_encBac.reset();
      //write reflectance slice header
      m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH_REFL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_hls.abh.sliceID = m_sliceID;
      m_hls.abh.attributeID = multiDataID;
      m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();
      // encode reflectance
      TComBufferChunk bufferChunkRefl(BufferChunkType::BCT_REFL);
      m_encBac.setBitstreamBuffer(bufferChunkRefl);
      m_encBac.initBac();
      m_attrEncoder.transformEncodeReflectance();
      m_encBac.encodeTerminationFlag();
      m_encBac.encodeFinish();
      m_attrEncoder.getReflectanceBits() = m_encBac.getBitStreamLength() * 8;
      bufferChunkRefl.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      bufferChunkRefl.reset();
      m_encBac.reset();
    } else if (!m_hls.aps.attrEncodeOrder) {  // firstly encode color
      //write color slice header
      m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH_COL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_hls.abh.sliceID = m_sliceID;
      m_hls.abh.attributeID = multiDataID;
      m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();
      // encode color
      TComBufferChunk bufferChunkCol(BufferChunkType::BCT_COL);
      m_encBac.setBitstreamBuffer(bufferChunkCol);
      m_encBac.initBac();
      m_attrEncoder.transformEncodeColor();
      m_encBac.encodeTerminationFlag();
      m_encBac.encodeFinish();
      m_attrEncoder.getColorBits() = m_encBac.getBitStreamLength() * 8;
      bufferChunkCol.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      bufferChunkCol.reset();
      m_encBac.reset();
      //write reflectance slice header
      m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH_REFL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_hls.abh.sliceID = m_sliceID;
      m_hls.abh.attributeID = multiDataID;
      m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();
      // encode reflectance
      TComBufferChunk bufferChunkRefl(BufferChunkType::BCT_REFL);
      m_encBac.setBitstreamBuffer(bufferChunkRefl);
      m_encBac.initBac();
      m_attrEncoder.transformEncodeReflectanceFromColor();
      m_encBac.encodeTerminationFlag();
      m_encBac.encodeFinish();
      m_attrEncoder.getReflectanceBits() = m_encBac.getBitStreamLength() * 8;
      bufferChunkRefl.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      bufferChunkRefl.reset();
      m_encBac.reset();
    } else {  // firstly encode refl, CTC default
      //write reflectance slice header
      m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH_REFL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_hls.abh.sliceID = m_sliceID;
      m_hls.abh.attributeID = multiDataID;
      m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();
      // encode reflectance
      TComBufferChunk bufferChunkRefl(BufferChunkType::BCT_REFL);
      m_encBac.setBitstreamBuffer(bufferChunkRefl);
      m_encBac.initBac();
      m_attrEncoder.transformEncodeReflectance();
      m_encBac.encodeTerminationFlag();
      m_encBac.encodeFinish();
      m_attrEncoder.getReflectanceBits() = m_encBac.getBitStreamLength() * 8;
      bufferChunkRefl.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      bufferChunkRefl.reset();
      m_encBac.reset();
      //write color slice header
      m_bufferChunk.setBufferType(BufferChunkType::BCT_ABH_COL);
      m_encBac.setBitstreamBuffer(m_bufferChunk);
      m_hls.abh.sliceID = m_sliceID;
      m_hls.abh.attributeID = multiDataID;
      m_encBac.codeABH(m_hls.abh, m_hls.aps, m_hls.sps);
      m_encBac.encodeFinish();
      m_bufferChunk.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      m_bufferChunk.reset();
      m_encBac.reset();
      // encode color
      TComBufferChunk bufferChunkCol(BufferChunkType::BCT_COL);
      m_encBac.setBitstreamBuffer(bufferChunkCol);
      m_encBac.initBac();
      m_attrEncoder.transformEncodeColorFromReflectance();
      m_encBac.encodeTerminationFlag();
      m_encBac.encodeFinish();
      m_attrEncoder.getColorBits() = m_encBac.getBitStreamLength() * 8;
      bufferChunkCol.writeToBitstream(&m_bitstreamFile, m_encBac.getBitStreamLength());
      bufferChunkCol.reset();
      m_encBac.reset();
    }
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
    if (m_hls.aps.attributeDataNumSetMinus1[1] > 0 &&
        m_hls.aps.multiAttriGroupNum[m_hls.aps.multiAttrGroupID[multiDataID]] > 1)
      m_attrEncoder.predictEncodeMultiReflectance();
    else
      m_attrEncoder.predictEncodeReflectance();
  } else if (m_hls.aps.transform == 1) {
    m_attrEncoder.transformEncodeReflectance();
  } else if (m_hls.aps.transform == 2) {
    m_attrEncoder.predictAndTransformEncodeReflectance();
  }
}

Void TEncTop::addToReconstructionCloud(TComPointCloud* reconstructionCloud) {
  int voxelCount_add = m_pointCloudRecon.getNumPoint();
  int voxelClout_re = reconstructionCloud->getNumPoint();
  if (m_hls.gbh.geomBoundingBoxOrigin.max())
    for (size_t idx = 0; idx < voxelCount_add; ++idx)
      for (size_t k = 0; k < 3; ++k)
        m_pointCloudRecon[idx][k] += m_hls.gbh.geomBoundingBoxOrigin[k];
  reconstructionCloud->init(voxelClout_re + voxelCount_add, m_pointCloudRecon.getNumMultilColor(),
                            m_pointCloudRecon.getNumMultilRefl(), m_pointCloudRecon.hasColors(),
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
  m_hls.frameheader.frameNumSliceMinus1 = pointCloudPartitionList.size() - 1;
}

///< \}
