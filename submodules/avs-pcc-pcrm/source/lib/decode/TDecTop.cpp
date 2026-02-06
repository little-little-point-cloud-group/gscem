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

#include "TDecTop.h"
#include "common/FXPoint.h"
#include "common/MD5Sum.h"
#include "common/TComBufferChunk.h"
#include "common/contributors.h"
#include <time.h>

using namespace std;

///< \in TLibDecoder \{

/**
 * Implementation of TDecTop
 * decoder class
 */

//////////////////////////////////////////////////////////////////////////
// Public class functions
//////////////////////////////////////////////////////////////////////////

Void TDecTop::getMultiAttriGroupNum(AttributeParameterSet& aps) {
  for (int multiIdx = 0; multiIdx < aps.attributeInfoNumSetMinus1[1] + 1; ++multiIdx) {
    aps.multiAttriGroupNum[aps.multiAttrGroupID[multiIdx]]++;
  }
};

Int TDecTop::decode() {
  Int numDigits = getfileNumLength();  // numDigit indicates digits of file number
  setStartFrameNum(numDigits);         // set start frame name from m_inputFileName

  Int exitState = EXIT_SUCCESS;

  MD5Sum md5Calculator;
  bool CheckMd5Flag = md5Calculator.openIStream(m_md5FileName);

  ///< Print information
  Double totalGeomUserTime = 0.0;
  Double totalColorUserTime = 0.0;
  Double totalReflUserTime = 0.0;
  Double totalAttrUserTime = 0.0;
  Double totalUserTime = 0.0;
  Bool isMatch = true;
  
  for (UInt i_bin = 0; i_bin < m_numOfBins; i_bin++) {
    bool endDecodeSequence = false;
    bool endDecodeFrame = false;
    UInt i_frame = 0;
    printFrameCutoffRule(i_frame + i_bin, numDigits);

    // update m_bitstreamFileName
    updateBinFileName(m_startFrame + i_frame + i_bin, numDigits, m_numOfBins);

    ifstream bitstreamFile(m_bitstreamFileName, ios::binary);
    if (!bitstreamFile) {
      cerr << "Error: failed to open bitstream file " << m_bitstreamFileName << " for writing!"
           << endl;
      exitState |= EXIT_FAILURE;
    }

    Double userTimeTotal = 0.0, userTimeGeometry = 0.0, userTimeAttribute = 0.0,
           userTimeColor = 0.0, userTimeReflectance = 0.0;
    clock_t userTimeTotalBegin = clock();

    TComPointCloud reconstructionCloud;
    reconstructionCloud.setNumPoint(0);
    int buffersize = (1 << 28);
    int multiIDGroupNum = 1;
    uint8_t nextStartCode = 0;
    m_attrDecoder.getColorTime() = 0;
    m_attrDecoder.getReflectanceTime() = 0;

    while (!bitstreamFile.eof()) {
      if (m_bufferChunk.readFromBitstream(bitstreamFile, buffersize, endDecodeSequence,
                                          nextStartCode, endDecodeFrame) == EXIT_FAILURE) {
        exitState |= EXIT_FAILURE;
        continue;
      }

      m_decBac.setBitstreamBuffer(m_bufferChunk);
      switch (m_bufferChunk.getBufferType()) {
      case BufferChunkType::BCT_SPS: {
        m_decBac.parseSPS(m_hls.sps);
        m_bufferChunk.reset();
        m_decBac.reset();
        break;
      }
      case BufferChunkType::BCT_GPS: {
        m_decBac.parseGPS(m_hls.gps);
        m_bufferChunk.reset();
        m_decBac.reset();
        break;
      }
      case BufferChunkType::BCT_GBH: {
        m_decBac.parseGBH(m_hls.sps, m_hls.gps, m_hls.gbh);
        buffersize = max((int)((float)m_hls.gbh.geomNumPoints * 8), 1024);
        m_bufferChunk.reset();
        m_decBac.reset();
        if (m_hls.aps.multiDataSetFlag[0] || m_hls.aps.multiDataSetFlag[1])
          multiID = 0;
        break;
      }
      case BufferChunkType::BCT_GEOM: {
        clock_t userTimeGeometryBegin = clock();
        m_decBac.initBac();
        init_aec_context_tab();
        decompressAndDecodePartition();
        assert(m_decBac.decodeTerminationFlag());
        m_bufferChunk.reset();
        m_decBac.reset();
        userTimeGeometry += (Double)(clock() - userTimeGeometryBegin) / CLOCKS_PER_SEC;
        if (m_hls.aps.attributeDataPresentFlag[0]) {
          m_pointCloudRecon.setNumMultilColor(m_hls.aps.attributeDataNumSetMinus1[0] + 1);
          reconstructionCloud.setNumMultilColor(m_hls.aps.attributeDataNumSetMinus1[0] + 1);
        }
        if (m_hls.aps.attributeDataPresentFlag[1]) {
          m_pointCloudRecon.setNumMultilRefl(m_hls.aps.attributeDataNumSetMinus1[1] + 1);
          reconstructionCloud.setNumMultilRefl(m_hls.aps.attributeDataNumSetMinus1[1] + 1);
        }
        if (!(m_hls.sps.attrPresentFlag))
          addToReconstructionCloud(&reconstructionCloud, 0);
        break;
      }
      case BufferChunkType::BCT_APS: {
        m_decBac.parseAPS(m_hls.aps, m_hls.sps);
        if (m_hls.aps.attributeDataNumSetMinus1[1] > 0) {
          getMultiAttriGroupNum(m_hls.aps);
        }
        m_hls.aps.eligibleDupPointPred = true;
        if (m_hls.sps.attrPresentFlag) {
          if (m_hls.aps.attributeDataPresentFlag[0] && m_hls.aps.attributeDataPresentFlag[1])
            m_hls.aps.eligibleDupPointPred = false;
          else
            m_hls.aps.eligibleDupPointPred = true;
        } else
          m_hls.aps.eligibleDupPointPred = false;
        m_bufferChunk.reset();
        m_decBac.reset();
        break;
      }
      case BufferChunkType::BCT_FRAME: {
        m_decBac.parseFrameHeader(m_hls.frameheader);
        m_bufferChunk.reset();
        m_decBac.reset();
        break;
      }
      case BufferChunkType::BCT_ABH_COL: {
        m_decBac.parseABH(m_hls.abh, m_hls.aps, 0);
        m_bufferChunk.reset();
        m_decBac.reset();
        break;
      }
      case BufferChunkType::BCT_ABH_REFL: {
        m_decBac.parseABH(m_hls.abh, m_hls.aps, 1);
        if (m_hls.aps.attributeDataNumSetMinus1[1] > 0)
          buffersize = (int)((float)m_hls.gbh.geomNumPoints * 8 * 5);
        m_bufferChunk.reset();
        m_decBac.reset();
        break;
      }
      case BufferChunkType::BCT_COL: {
        clock_t userTimeAttributeBegin = clock();
        m_decBac.initBac();
        init_aec_context_tab();
        if (nextStartCode != refl_slice_header_start_code) {
          decompressColor();
          ++multiID;
        } else if (m_hls.aps.transform == 1) {
          decompressColor();
        } else if (m_hls.aps.transform == 0 || m_hls.aps.transform == 2) {
          // decode ABH_REFL
          TComBufferChunk bufferChunkDual;
          if (bufferChunkDual.readFromBitstream(bitstreamFile, buffersize, endDecodeSequence,
                                                nextStartCode, endDecodeFrame) == EXIT_FAILURE) {
            exitState |= EXIT_FAILURE;
            continue;
          }
          m_decBacDual.setBitstreamBuffer(bufferChunkDual);
          m_decBacDual.parseABH(m_hls.abh, m_hls.aps, 1);
          bufferChunkDual.reset();
          m_decBacDual.reset();
          // get reflectance bitstream
          if (bufferChunkDual.readFromBitstream(bitstreamFile, buffersize, endDecodeSequence,
                                                nextStartCode, endDecodeFrame) == EXIT_FAILURE) {
            exitState |= EXIT_FAILURE;
            continue;
          }
          m_decBacDual.setBitstreamBuffer(bufferChunkDual, true);
          m_decBacDual.initBac();
          init_aec_context_tab();
          // decode attribute
          decompressAttribute();
          assert(m_decBacDual.decodeTerminationFlag());
          bufferChunkDual.reset();
          m_decBacDual.reset();
          ++multiID;
        }
        assert(m_decBac.decodeTerminationFlag());
        m_bufferChunk.reset();
        m_decBac.reset();
        userTimeColor += m_attrDecoder.getColorTime();
        userTimeReflectance += m_attrDecoder.getReflectanceTime();
        userTimeAttribute += (Double)(clock() - userTimeAttributeBegin) / CLOCKS_PER_SEC;
        if (multiID == m_hls.aps.attributeDataNumSetMinus1[0] + 1) {
          addToReconstructionCloud(&reconstructionCloud, 0);
          addToReconstructionCloud(&reconstructionCloud, 1);
        }
        break;
      }
      case BufferChunkType::BCT_REFL: {
        clock_t userTimeAttributeBegin = clock();
        if (nextStartCode != color_slice_header_start_code) {
          if (m_hls.aps.attributeDataNumSetMinus1[1] > 0 && m_hls.aps.transform == 0) {
            multiIDGroupNum = m_hls.aps.multiAttriGroupNum[m_hls.aps.multiAttrGroupID[multiID]];
          }
          m_decBac.initBac();
          init_aec_context_tab();
          decompressReflectance();
          multiID += multiIDGroupNum;
        } else if (m_hls.aps.transform == 1) {
          m_decBac.initBac();
          init_aec_context_tab();
          decompressReflectance();
        } else if (m_hls.aps.transform == 0 || m_hls.aps.transform == 2) {
          TComBufferChunk bufferChunkDual;
          swap(bufferChunkDual, m_bufferChunk);
          m_decBacDual.setBitstreamBuffer(bufferChunkDual, true);
          m_decBacDual.initBac();
          init_aec_context_tab;
          // decode ABH_COL
          if (m_bufferChunk.readFromBitstream(bitstreamFile, buffersize, endDecodeSequence,
                                              nextStartCode, endDecodeFrame) == EXIT_FAILURE) {
            exitState |= EXIT_FAILURE;
            continue;
          }
          m_decBac.setBitstreamBuffer(m_bufferChunk);
          m_decBac.parseABH(m_hls.abh, m_hls.aps, 0);
          m_bufferChunk.reset();
          m_decBac.reset();
          // get color bitstream
          if (m_bufferChunk.readFromBitstream(bitstreamFile, buffersize, endDecodeSequence,
                                              nextStartCode, endDecodeFrame) == EXIT_FAILURE) {
            exitState |= EXIT_FAILURE;
            continue;
          }
          m_decBac.setBitstreamBuffer(m_bufferChunk);
          m_decBac.initBac();
          init_aec_context_tab();
          // decode attribute
          decompressAttribute();
          assert(m_decBacDual.decodeTerminationFlag());
          bufferChunkDual.reset();
          m_decBacDual.reset();
          ++multiID;
        }
        assert(m_decBac.decodeTerminationFlag());
        m_bufferChunk.reset();
        m_decBac.reset();
        userTimeColor += m_attrDecoder.getColorTime();
        userTimeReflectance += m_attrDecoder.getReflectanceTime();
        userTimeAttribute += (Double)(clock() - userTimeAttributeBegin) / CLOCKS_PER_SEC;
        if (multiID == m_hls.aps.attributeDataNumSetMinus1[1] + 1) {
          addToReconstructionCloud(&reconstructionCloud, 0);
          addToReconstructionCloud(&reconstructionCloud, 1);
        }
        break;
      }
      case BufferChunkType::BCT_UDA: {
        m_decBac.parseUserData();
        m_bufferChunk.reset();
        m_decBac.reset();
        break;
      }
      case BufferChunkType::BCT_MAX:
        break;  // end of the sequence
      default: {
        checkCond(false, "Error: invalid bitstream type!");
        exitState |= EXIT_FAILURE;
        continue;
      }
      }

      if ((endDecodeFrame && (nextStartCode == frame_start_code)) || endDecodeSequence) {
        m_pointCloudRecon = reconstructionCloud;
        userTimeTotal = (Double)(clock() - userTimeTotalBegin) / CLOCKS_PER_SEC;
        if (m_hls.sps.attrPresentFlag) {
          if (m_hls.aps.attributeDataPresentFlag[0] && m_hls.aps.attributeDataPresentFlag[1]) {
            userTimeColor = (userTimeColor + userTimeAttribute - userTimeReflectance) / 2;
            userTimeReflectance = (userTimeReflectance + userTimeAttribute - userTimeColor) / 2;
          } else if (m_hls.aps.attributeDataPresentFlag[0]) {
            userTimeColor = userTimeAttribute;
          } else if (m_hls.aps.attributeDataPresentFlag[1]) {
            userTimeReflectance = userTimeAttribute;
          }
        }

        cout << "Geometry processing time (user): " << userTimeGeometry << " sec." << endl;
        cout << "Color processing time (user): " << userTimeColor << " sec." << endl;
        cout << "Reflectance processing time (user): " << userTimeReflectance << " sec." << endl;
        cout << "Attribute processing time (user): " << userTimeAttribute << " sec." << endl;
        cout << "Total processing time (user): " << userTimeTotal << " sec." << endl << endl;
        totalGeomUserTime += userTimeGeometry;
        totalColorUserTime += userTimeColor;
        totalReflUserTime += userTimeReflectance;
        totalAttrUserTime += userTimeAttribute;
        totalUserTime += userTimeTotal;

        geomPostprocessingAndDequantization();

        if (m_colorTransformFlag && m_pointCloudRecon.hasColors()) {
          m_pointCloudRecon.convertYUVToRGB();
        }

        if (CheckMd5Flag) {
          md5Calculator.calculateMD5(&m_pointCloudRecon);

          string md5InFile = md5Calculator.getMD5InFile();
          string md5InRec = md5Calculator.getMD5Str();

          Bool compareMD5Flag = (md5InFile == md5InRec) ? true : false;
          isMatch &= compareMD5Flag;

          if (compareMD5Flag)
            cout << "MD5 check status: Success." << endl;
          else
            cout << "MD5 check status: Fail." << endl;
        }

        if (endDecodeFrame && (nextStartCode == frame_start_code) || m_numOfBins > 1) {
          if (i_bin == 0 && i_frame == 0) {
            resetReconFileName(m_startFrame + i_bin, numDigits);
          }
        }

        ///< write recon ply
        if (m_reconFileName.length() > 0) {
          m_pointCloudRecon.writeToFile(m_reconFileName, m_writePlyInAsciiFlag);
        }

        // start another frame
        i_frame++;
        // update m_reconFileName
        updateReconFileName(m_startFrame + i_frame + i_bin, numDigits, m_numOfBins);

        if (endDecodeSequence)
          break;

        userTimeTotal = 0.0, userTimeGeometry = 0.0, userTimeAttribute = 0.0, userTimeColor = 0.0,
        userTimeReflectance = 0.0;
        userTimeTotalBegin = clock();

        reconstructionCloud.setNumPoint(0);
        m_attrDecoder.getColorTime() = 0;
        m_attrDecoder.getReflectanceTime() = 0;
      }
    }
    //}  ///< end for loop all frames
    bitstreamFile.close();
  }
  cout << "All frames geometry processing time (user): " << totalGeomUserTime << " sec." << endl;
  cout << "All frames color processing time (user): " << totalColorUserTime << " sec." << endl;
  cout << "All frames refl processing time (user): " << totalReflUserTime << " sec." << endl;
  cout << "All frames attributes processing time (user): " << totalAttrUserTime << " sec." << endl;
  cout << endl << "All frames total processing time (user): " << totalUserTime << " sec." << endl;
  if (CheckMd5Flag)
    cout << "All frames MD5 check status: " << (isMatch ? "Success." : "Fail.") << endl << endl;

  if (CheckMd5Flag)
    md5Calculator.closeFile();

  return exitState;
}

//////////////////////////////////////////////////////////////////////////
// Private class functions
//////////////////////////////////////////////////////////////////////////

Void TDecTop::decompressAndDecodePartition() {
  m_pointCloudRecon.clear();
  m_geomDecoder.init(&m_pointCloudRecon, &m_hls, &m_decBac);
  m_geomDecoder.decodeAndDecompress();
  m_geomDecoder.clear();
}

Void TDecTop::geomPostprocessingAndDequantization() {
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

Void TDecTop::decompressAttribute() {
  m_attrDecoder.initDual(&m_pointCloudRecon, &m_hls, &m_decBac, &m_decBacDual, multiID);
  if (m_hls.aps.transform == 0) {
    m_attrDecoder.predictDecodeAttribute();
  } else if (m_hls.aps.transform == 2) {
    m_attrDecoder.predictAndTransformDecodeAttribute();
  }
}

Void TDecTop::decompressColor() {
  m_attrDecoder.init(&m_pointCloudRecon, &m_hls, &m_decBac, multiID);
  if (m_hls.aps.transform == 0) {
    m_attrDecoder.predictDecodeColor();
  } else if (m_hls.aps.transform == 1) {
    if (m_hls.aps.attributeDataPresentFlag[1] && m_hls.aps.attrEncodeOrder &&
        m_hls.aps.crossAttrTypePred) {
      m_attrDecoder.transformDecodeColorFromReflectance();
    } else
      m_attrDecoder.transformDecodeColor();

  } else if (m_hls.aps.transform == 2) {
    m_attrDecoder.predictAndTransformDecodeColor();
  }
}

Void TDecTop::decompressReflectance() {
  m_attrDecoder.init(&m_pointCloudRecon, &m_hls, &m_decBac, multiID);
  if (m_hls.aps.transform == 0) {
    if (m_hls.aps.attributeDataNumSetMinus1[1] > 0 &&
        m_hls.aps.multiAttriGroupNum[m_hls.aps.multiAttrGroupID[multiID]] > 1)
      m_attrDecoder.predictDecodeMultiReflectance();
    else
      m_attrDecoder.predictDecodeReflectance();
  } else if (m_hls.aps.transform == 1) {
    if (m_hls.aps.attributeDataPresentFlag[0] && !m_hls.aps.attrEncodeOrder &&
        m_hls.aps.crossAttrTypePred) {
      m_attrDecoder.transformDecodeReflectanceFromColor();
    } else
      m_attrDecoder.transformDecodeReflectance();
  } else if (m_hls.aps.transform == 2) {
    m_attrDecoder.predictAndTransformDecodeReflectance();
  }
}

Void TDecTop::addToReconstructionCloud(TComPointCloud* reconstructionCloud, const Int geom0attr1) {
  int voxelCount_add = m_pointCloudRecon.getNumPoint();
  int voxelClout_re = reconstructionCloud->getNumPoint();

  if (geom0attr1 == 0) {
    if (m_hls.gbh.geomBoundingBoxOrigin.max())
      for (size_t idx = 0; idx < voxelCount_add; ++idx)
        for (size_t k = 0; k < 3; ++k)
          m_pointCloudRecon[idx][k] += m_hls.gbh.geomBoundingBoxOrigin[k];
    reconstructionCloud->init(voxelClout_re + voxelCount_add, m_pointCloudRecon.getNumMultilColor(),
                              m_pointCloudRecon.getNumMultilRefl(),
                              m_hls.aps.attributeDataPresentFlag[0],
                              m_hls.aps.attributeDataPresentFlag[1]);
    std::copy(m_pointCloudRecon.positions().begin(), m_pointCloudRecon.positions().end(),
              std::next(reconstructionCloud->positions().begin(), voxelClout_re));
  } else if (geom0attr1 == 1) {
    if (reconstructionCloud->hasColors())
      std::copy(m_pointCloudRecon.getColors().begin(), m_pointCloudRecon.getColors().end(),
                std::prev(reconstructionCloud->getColors().end(),
                          voxelCount_add * reconstructionCloud->getNumMultilColor()));
    if (reconstructionCloud->hasReflectances())
      std::copy(m_pointCloudRecon.getReflectances().begin(),
                m_pointCloudRecon.getReflectances().end(),
                std::prev(reconstructionCloud->getReflectances().end(),
                          voxelCount_add * reconstructionCloud->getNumMultilRefl()));
  }
}
//! \}
