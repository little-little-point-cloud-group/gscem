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

#include "TEncAttribute.h"
#include "TEncBacTop.h"
#include "TEncCfg.h"
#include "TEncGeometry.h"
#include "common/TComPcEvalue.h"
#include "common/TComPointCloud.h"
#include "common/contributors.h"

///< \in TLibEncoder \{

/**
 * Class EncoderStatistics
 */
class EncoderStatistics 
{
public:
  UInt64 geomBits = 0;
  UInt64 colorBits = 0;
  UInt64 reflBits = 0;
  UInt64 attrBits[NUM_MULTIATTRIBUTE] = {0};
  UInt64 totalBits = 0;
  Double geomUserTime = 0.0;
  Double recolorUserTime = 0.0;
  Double colorUserTime = 0.0;
  Double reflUserTime = 0.0;
  Double attrUserTime = 0.0;
  Double totalUserTime = 0.0;
  UInt64 numReconPoints = 0;

  void operator+=(const EncoderStatistics& other) {
    geomBits += other.geomBits;
    colorBits += other.colorBits;
    reflBits += other.reflBits;
    for (int multiIdx = 0; multiIdx < NUM_MULTIATTRIBUTE; ++multiIdx)
      attrBits[multiIdx] += other.attrBits[multiIdx];
    totalBits += other.totalBits;
    geomUserTime += other.geomUserTime;
    recolorUserTime += other.recolorUserTime;
    colorUserTime += other.colorUserTime;
    reflUserTime += other.reflUserTime;
    attrUserTime += other.attrUserTime;
    totalUserTime += other.totalUserTime;
    numReconPoints += other.numReconPoints;
  }
};

/**
 * Class TEncTop
 * encoder class
 */

class TEncTop : public TEncCfg {
private:
  TComPointCloud m_pointCloudOrg;    ///< input point cloud
  TComPointCloud m_pointCloudQuant;  ///< quantized point cloud
  TComPointCloud m_pointCloudRecon;  ///< output point cloud

  TEncGeometry m_geomEncoder;    ///< geometry encoder
  TEncAttribute m_attrEncoder;   ///< attribute encoder
  TEncBacTop m_encBac;           ///< AVS3 binary arithmetic encoder
  TEncBacTop m_encBacDual;       ///< AVS3 binary arithmetic encoder
  TComBufferChunk m_bufferChunk; ///< bitstream buffer chunk
  ofstream m_bitstreamFile;      ///< file bitstream

  int m_frame_ID;
  int m_frame_count;
  int m_sliceID;
  int multiDataID;
  V3<Int> m_sliceOrigin;
  V3<UInt> m_sliceBoundingBox;

public:
  TEncTop() = default;
  ~TEncTop() = default;
  Int encode();  ///< main encoding function

  Void encodeSlice(EncoderStatistics& es);  ///< main function for encoding one slice
  Void getSingleAttrAPs(const SequenceParameterSet& sps, AttributeParameterSet& aps,
                        const int& attrIdx, const int& multilIdx);
  Void FixedMultiAPs(const SequenceParameterSet& sps, AttributeParameterSet& aps);
  Int getFrameNum() {
    return m_numOfFrames;
  }  // get m_numOfFrames
  Int getstartFrame() {
    return m_startFrame;
  }  // get m_startFrame
  // set start frame name from m_inputFileName
  void setStartFrameNum(Int numDigits) {
    char* numofStartFrame = new char[numDigits + 1];
    Int numStartPos = Int(m_inputFileName.find_last_of('.')) - numDigits;
#pragma warning(push)
#pragma warning(disable : 4996)
    std::strcpy(numofStartFrame, m_inputFileName.substr(numStartPos, numDigits).c_str());
#pragma warning(pop)
    m_startFrame = atoi(numofStartFrame);
    if(m_inputFileName.substr(numStartPos-3, 3).compare("vox") == 0 || m_inputFileName.substr(numStartPos-3, 3).compare("VOX") == 0){
        m_startFrame = 1;
    }
    delete[] numofStartFrame;
  }
  // get digit of number of file number
  Int getfileNumLength() {
    Int dotLocation = Int(m_inputFileName.find_last_of('.'));
    Int numDigits = 0;
    Int fileNameLength = Int(m_inputFileName.length());
    dotLocation--;
    while (m_inputFileName[dotLocation - numDigits] >= '0' &&
           m_inputFileName[dotLocation - numDigits] <= '9') {
      numDigits++;
      if ((numDigits + 4) >= fileNameLength)
        break;
    }
    return (numDigits);
  }
  // update m_inputFileName & m_bitstreamFileName & m_reconFileName based on file number
  void updateIOFileName(Int i_frameNum, Int numDigits) {
    assert(numDigits > 0);

    char* c_frameNum = new char[numDigits + 1];
    snprintf(c_frameNum, numDigits + 1, "%0*d", numDigits, i_frameNum);

    if (m_inputFileName.length() > 0)
      m_inputFileName.replace(m_inputFileName.find_last_of('.') - numDigits, numDigits, c_frameNum);
    if (m_bitstreamFileName.length() > 0)
      m_bitstreamFileName.replace(m_bitstreamFileName.find_last_of('.') - numDigits, numDigits,
                                  c_frameNum);
    if (m_reconFileName.length() > 0)
      m_reconFileName.replace(m_reconFileName.find_last_of('.') - numDigits, numDigits, c_frameNum);

    delete[] c_frameNum;
  }
  // reset m_bitstreamFileName & m_reconFileName based on file number
  void resetBitstream_Recon_FileName(Int i_frameNum, Int numDigits) {
    if (m_numOfFrames <= 1)
      return;

    char* c_frameNum = new char[numDigits + 1];
    snprintf(c_frameNum, numDigits + 1, "%0*d", numDigits, i_frameNum);

    string add;
    assert(numDigits > 0);
    if (m_bitstreamFileName.length() > 0) {
      add = "-" + string(c_frameNum);
      m_bitstreamFileName.insert(m_bitstreamFileName.find_last_of('.'), add);
    }

    if (m_reconFileName.length() > 0) {
      add = "-" + string(c_frameNum);
      m_reconFileName.insert(m_reconFileName.find_last_of('.'), add);
    }

    delete[] c_frameNum;
  }

  // print cut-off rule in log for each frame
  void printFrameCutoffRule(Int i_frameNum, Int numDigits) {
    char* c_frameNum = new char[numDigits + 1];
    //char c_frameNum[512];
    snprintf(c_frameNum, numDigits + 1, "%0*d", numDigits, i_frameNum + m_startFrame);
    string cutoffRule = "------------ frame" + string(c_frameNum) + " ------------";
    cout << endl << cutoffRule << endl;
    delete[] c_frameNum;
  }

private:
  Void initParameters();
  Void initFrameParameters(FrameHeader& frameHead);
  Void compressAndEncodePartition();
  Void geomPreprocessAndQuantization(UInt& geoNumPoint, const Float& qs, Double& userTimeRecolor);
  Void geomPreprocessAndQuantizationRetainingDuplicatePoints(const Float& qs);
  Void geomPostprocessingAndDequantization(const Float& qs);
  Void compressAndEncodeAttribute();
  Void compressAndEncodeColor();
  Void compressAndEncodeReflectance();
  Void fastRecolor(const std::pair<const PC_POS, std::vector<int>> it, const Int& uniquePointNumber,
                   const Float& qs);
  Void SliceDevisionByMortonCode(int slicenum, vector<TComPointCloud>& pointCloudPartitionList);
  Void addToReconstructionCloud(TComPointCloud* reconstructionCloud);
  Void sliceDevisionByHistZ(const TComPointCloud pointCloudOrg,
                            vector<TComPointCloud>& pointCloudPartitionList, Int numOfSlice,
                            const Float& geomQs, Int numDigits);
  Void SliceDevisionByPointNum(vector<TComPointCloud>& pointCloudPartitionList,
                               UInt maxPointNumOfSlicesLog2);
  Void pointCloudPack(TComPointCloud& pointCloudRecon, const TComPointCloud& pointCloudAdd);
  Void histogramZ(PC_POS posMin, PC_POS posMax, const TComPointCloud pc, vector<int32_t>& groundZ);
  Void groundSlices(vector<TComPointCloud>& groundList, int32_t dis, int32_t groundZMin,
                    const TComPointCloud pointCloudRes);
};  ///< END CLASS TEccTop

///< \}
