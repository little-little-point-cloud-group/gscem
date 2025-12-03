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

#include "common/TComPcEvalue.h"
// set start frame name from m_inputFileName
UInt setStartFrameNum(const string& m_inputFileName, Int numDigits) {
  UInt m_startFrame = 0;
  char* numofStartFrame = new char[numDigits + 1];
  Int numStartPos = Int(m_inputFileName.find_last_of('.')) - numDigits;
#pragma warning(push)
#pragma warning(disable : 4996)
  std::strcpy(numofStartFrame, m_inputFileName.substr(numStartPos, numDigits).c_str());
#pragma warning(pop)
  m_startFrame = atoi(numofStartFrame);
  if (m_inputFileName.substr(numStartPos - 3, 3).compare("vox") == 0 ||
      m_inputFileName.substr(numStartPos - 3, 3).compare("VOX") == 0) {
    m_startFrame = 1;
  }
  delete[] numofStartFrame;
  return m_startFrame;
}
// get digit of number of file number
Int getfileNumLength(const string& m_inputFileName) {
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
void updateIOFileName(Int i_frameNum, Int numDigits, string& m_inputFileName,
                      string& m_reconFileName) {
  assert(numDigits > 0);

  char* c_frameNum = new char[numDigits + 1];
  snprintf(c_frameNum, numDigits + 1, "%0*d", numDigits, i_frameNum);

  if (m_inputFileName.length() > 0)
    m_inputFileName.replace(m_inputFileName.find_last_of('.') - numDigits, numDigits, c_frameNum);

  if (m_reconFileName.length() > 0)
    m_reconFileName.replace(m_reconFileName.find_last_of('.') - numDigits, numDigits, c_frameNum);

  delete[] c_frameNum;
}
// reset m_bitstreamFileName & m_reconFileName based on file number
void resetBitstream_Recon_FileName(Int i_frameNum, Int numDigits, const UInt& m_numOfFrames,
                                   string& m_reconFileName) {
  if (m_numOfFrames <= 1)
    return;

  char* c_frameNum = new char[numDigits + 1];
  snprintf(c_frameNum, numDigits + 1, "%0*d", numDigits, i_frameNum);

  string add;
  assert(numDigits > 0);
  if (m_reconFileName.length() > 0) {
    add = "-" + string(c_frameNum);
    m_reconFileName.insert(m_reconFileName.find_last_of('.'), add);
  }

  delete[] c_frameNum;
}

// print cut-off rule in log for each frame
void printFrameCutoffRule(Int i_frameNum, Int numDigits, const UInt& m_startFrame) {
  char* c_frameNum = new char[numDigits + 1];
  //char c_frameNum[512];
  snprintf(c_frameNum, numDigits + 1, "%0*d", numDigits, i_frameNum + m_startFrame);
  string cutoffRule = "------------ frame" + string(c_frameNum) + " ------------";
  cout << endl << cutoffRule << endl;
  delete[] c_frameNum;
}
int main(int argc, char* argv[]) {
  /// print information
  cout << "" << AVS_PCC_SW_NAME << ": pc_evalue " << AVS_PCC_VERSION << " ";
  cout << NVM_ONOS << NVM_COMPILEDBY << NVM_BITS << endl;

  pc_evalue::TMetricCfg metricCfg;
  pc_evalue::TMetricRes metricResult;

  /// parse configuration
  if (!metricCfg.parseCfg(argc, argv))
    return EXIT_FAILURE;

  double wallTime;
  long wallTimeBegin = clock();
  ///Pc_evalue
  pc_evalue::TMetricRes totalMetricRes;
  pc_evalue::TMetricCfg MetricParam;
  MetricParam.m_calLossless = metricCfg.m_calLossless;
  MetricParam.m_calColor = metricCfg.m_calColor;
  MetricParam.m_calReflectance = metricCfg.m_calReflectance;
  MetricParam.m_peakValue = metricCfg.m_peakValue;
  MetricParam.m_reflOutputDepth = metricCfg.m_reflOutputDepth;
  MetricParam.m_symmetry = metricCfg.m_symmetry;
  MetricParam.m_duplicateMode = metricCfg.m_duplicateMode;
  MetricParam.m_multiNeighbourMode = metricCfg.m_multiNeighbourMode;
  MetricParam.m_showHausdorff = metricCfg.m_showHausdorff;
  Int numDigits =
    getfileNumLength(metricCfg.m_srcFile);  // numDigit indicates digits of file number
  UInt m_startFrame =
    setStartFrameNum(metricCfg.m_srcFile, numDigits);  // set start frame name from m_inputFileName
  string inputFileName =
    metricCfg.m_srcFile;
  string reconFileName = metricCfg.m_disFile;
  int numMulti_color = 0;
  int numMulti_refl = 0;
  for (UInt i_frame = 0; i_frame < metricCfg.m_numOfFrames; i_frame++) {
      
    printFrameCutoffRule(i_frame, numDigits, m_startFrame);
    if (i_frame == 0) {
      resetBitstream_Recon_FileName(m_startFrame, numDigits, metricCfg.m_numOfFrames,
                                    reconFileName);
    } else {
      updateIOFileName(m_startFrame + i_frame, numDigits, inputFileName, reconFileName);
    }
    TComPointCloud src, dis;
    if (!src.readFromFile(inputFileName)) {
      cout << "Error: failed to read ply file1!" << endl;
      return 0;
    }
    if (!dis.readFromFile(reconFileName)) {
      cout << "Error: failed to read ply file2!" << endl;
      return 0;
    }
    if (src.hasColors())
      numMulti_color = src.getNumMultilColor();
    if (src.hasReflectances())
      numMulti_refl = src.getNumMultilRefl();
    pc_evalue::computeMetric(src, dis, metricCfg, metricResult);
    totalMetricRes = totalMetricRes + metricResult;
  }
  totalMetricRes = totalMetricRes / metricCfg.m_numOfFrames;
  totalMetricRes.print(MetricParam, numMulti_color, numMulti_refl);
  wallTime = (double)(clock() - wallTimeBegin) / CLOCKS_PER_SEC;
  cout << "Processing time (wall): " << wallTime << " sec." << endl;
}