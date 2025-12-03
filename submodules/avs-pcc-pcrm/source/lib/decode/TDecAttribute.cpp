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

#include "TDecAttribute.h"
#include "common/AttributePredictor.h"
#include "common/FXPoint.h"
#include "common/Transform.cpp"
#include "common/Transform.h"
#include <algorithm>
#include <numeric>
#include <time.h>
#include <vector>

Void TDecAttribute::init(TComPointCloud* pointCloudRecon, HighLevelSyntax* hls, TDecBacTop* decBac,
                         const int& m_frameID, const int& m_numFrames, int multiID) {
  m_pointCloudRecon = pointCloudRecon;
  m_hls = hls;
  m_decBac = decBac;
  m_decBacDual = NULL;
  frame_Idx = m_frameID;
  frame_count = m_numFrames;
  multil_ID = multiID;
}

Void TDecAttribute::initDual(TComPointCloud* pointCloudRecon, HighLevelSyntax* hls,
                             TDecBacTop* decBac, TDecBacTop* decBacDual, const int& m_frameID,
                             const int& m_numFrames, int multiID) {
  m_pointCloudRecon = pointCloudRecon;
  m_hls = hls;
  m_decBac = decBac;
  if (m_hls->aps.attributePresentFlag[0] && m_hls->aps.attributePresentFlag[1]) {
    m_decBacDual = decBacDual;
  } else {
    m_decBacDual = NULL;
  }
  frame_Idx = m_frameID;
  frame_count = m_numFrames;
  multil_ID = multiID;
}

// Attribute decompression consists of the following stages:
//  - Reorder
//  - Entropy decode
//  - Inverse quantization
//  - Attribute reconstruction
Void TDecAttribute::predictDecodeAttribute() {
  clock_t userTimeColorBegin = clock();
  attributeInversePredictResidual();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
  clock_t userTimeReflectanceBegin = clock();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}
// Transform based approach consists of the following stages:
//  - Reorder
//  - Entropy decode of transform coefficients and residuals
//  - Inverse quantization of transform coefficients and residuals
//  - Inverse transform
//  - Attribute reconstruction
Void TDecAttribute::transformDecodeAttribute() {
  const AttributeParameterSet& aps = m_hls->aps;
  bool isEnableCrossAttrTypePred = aps.crossAttrTypePred;
  if (!isEnableCrossAttrTypePred) {
    clock_t userTimeColorBegin = clock();
    colorInverseWaveletTransform();
    m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
    clock_t userTimeReflectanceBegin = clock();
    reflectanceInverseWaveletTransform();
    m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
  } else if (!aps.attrEncodeOrder) {  // firstly decode color
    clock_t userTimeColorBegin = clock();
    colorInverseWaveletTransform();
    m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
    clock_t userTimeReflectanceBegin = clock();
    reflectanceInverseWaveletTransformFromColor();
    m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
  } else {  // firstly decode refl
    clock_t userTimeReflectanceBegin = clock();
    reflectanceInverseWaveletTransform();
    m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
    clock_t userTimeColorBegin = clock();
    colorInverseWaveletTransformFromReflectance();
    m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
  }
}

Void TDecAttribute::predictAndTransformDecodeAttribute() {
  clock_t userTimeColorBegin = clock();
  AttributeInversePredictAndTransformMemControl();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
  clock_t userTimeReflectanceBegin = clock();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}

Void TDecAttribute::predictDecodeColor() {
  clock_t userTimeColorBegin = clock();
  colorInversePredictResidual();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
}

Void TDecAttribute::transformDecodeColor() {
  clock_t userTimeColorBegin = clock();
  colorInverseWaveletTransform();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
}

Void TDecAttribute::predictAndTransformDecodeColor() {
  clock_t userTimeColorBegin = clock();
  colorInversePredictAndTransformMemControl();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
}

Void TDecAttribute::predictDecodeReflectance() {
  clock_t userTimeReflectanceBegin = clock();
    reflectanceInversePredictResidual();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}

Void TDecAttribute::transformDecodeReflectance() {
  clock_t userTimeReflectanceBegin = clock();
  reflectanceInverseWaveletTransform();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}

Void TDecAttribute::predictAndTransformDecodeReflectance() {
  clock_t userTimeReflectanceBegin = clock();
  ReflectanceInversePredictAndTransformMemControl();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}

Void TDecAttribute::predictDecodeMultiReflectance() {
  clock_t userTimeReflectanceBegin = clock();
  multiReflectanceInversePredictResidual();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}

void TDecAttribute::reflectanceInversePredictResidualDual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addReflectances();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.refReordermode, pointCloudCode, voxelCount,
          aps.axisBias);
  m_hls->sps.reflThreshold =
    (sps.reflQuantParam + abh.reflQPoffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  // obtain reflectance predictor
  int64_t predictorRefl;
  int64_t codedValue;
  std::vector<reflNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int setCount = 0;
  int prevIndex = -1;
  PC_REFL lastref = 0;
  ///<decode zero_cnt of run_length
  const bool isGolomb = aps.refGolombNum == 1 ? true : false;
  int run_length = m_decBac->parseRunlength();

  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    auto pointIndex = pointCloudCode[curIndex].index;
      PC_REFL& currentValue = outputPointCloud.getReflectance(pointIndex, multil_ID);
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      Bool isDuplicatePoint = curIndex > 0 && curPosition == outputPointCloud[prevIndex];

      if (isDuplicatePoint) {
        predictorRefl = lastref;
      } else {
        predictorRefl = getReflectancePredictorFarthest(curIndex, curIndex, outputPointCloud, sps,
                                                        aps, pointCloudCode, setCount, neighborSet);
      }
      isDuplicatePoint &= multil_ID == 0;
      // Entropy decode
      if (run_length > 0) {
        codedValue = 0;
        --run_length;
      } else {
        codedValue = m_decBac->parseAttr(false, 3, 0, isDuplicatePoint, false, aps.refGolombNum);
        run_length = m_decBac->parseRunlength();
      }
      reflectanceReconstruction(predictorRefl, codedValue, currentValue);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = outputPointCloud[pointIndex];
      neighborSet[farthestIdx].refl = currentValue;
      prevIndex = pointIndex;
      lastref = currentValue;
  }
 
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::attributeInversePredictResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();
  outputPointCloud.addReflectances();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReordermode, pointCloudCode, voxelCount,
                  1);
  //Qp compute
  quantizedQP colorQp;
  colorQp.attrQuantForLuma = sps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

  m_hls->sps.reflThreshold =
    (sps.reflQuantParam + abh.reflQPoffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  //dist weight calculation patameter
  auto boudingSize = (sps.geomBoundingBoxSize[0] * sps.geomBoundingBoxSize[1] +
                      sps.geomBoundingBoxSize[0] * sps.geomBoundingBoxSize[2] +
                      sps.geomBoundingBoxSize[1] * sps.geomBoundingBoxSize[2]) *
    2;
  uint64_t reflectanceDistCoef = log2(round(boudingSize / voxelCount));

  std::vector<int64_t> reflectanceRes = {0, 0, 0};
  std::vector<int64_t> reflResNum = {0, 0, 0};
  std::vector<int> reflectanceDistWeight = {0, 0, 0};

  PC_POS prePosition = outputPointCloud[pointCloudCode[0].index];
  PC_REFL preReflectance = 0;
  int distWeightGroupSize = 1 << aps.log2predDistWeightGroupSize;
  // cross attribute parameter
  bool isEnableCrossAttrTypePred = aps.crossAttrTypePred;
  uint64_t crossAttrTypeLambda = 0;
  int64_t crossAttrTypeCoef = 0;
  if (isEnableCrossAttrTypePred && (aps.attrEncodeOrder == 0)) {
    crossAttrTypeLambda =
      (-sps.colorQuantParam * aps.crossAttrTypePredParam1 + aps.crossAttrTypePredParam2);
    UInt maxPos =
      sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2];
    uint64_t maxColor = (1 << m_hls->aps.colorOutputDepth) - 1;
    uint64_t maxColorSum = 3 * maxColor;
    crossAttrTypeCoef = round((maxPos << 10) / (double)maxColorSum);
    crossAttrTypeCoef = uint64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;
  } else if (isEnableCrossAttrTypePred && (aps.attrEncodeOrder == 1)) {
    crossAttrTypeLambda = (-(sps.reflQuantParam + abh.reflQPoffset) * aps.crossAttrTypePredParam1 +
                           aps.crossAttrTypePredParam2);
    auto diffPos =
      sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2];
    uint64_t maxRefl = (1 << m_hls->aps.reflOutputDepth) - 1;
    crossAttrTypeCoef = round((diffPos << 10) / (double)maxRefl);
    crossAttrTypeCoef = uint64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;
  }
  //obtain predictors
  int setlength = aps.maxNumOfNeighbours;
  int setCount = 0;
  pair<PC_COL, PC_REFL> predictorAttr;
  PC_COL predictorColor;
  PC_REFL predictorRefl;
  V3<int64_t> codedValue;
  int64_t codedValueRefl;
  int prevIndex = -1;

  // runlength
  int run_length_col = 0;
  int run_length_refl = 0;
  const bool refisGolomb = aps.refGolombNum == 1 ? true : false;
  const bool colorisGolomb = aps.colorGolombNum == 1 ? true : false;
  bool os = aps.orderSwitch;
  bool colorLengthControl = false;
  bool reflLengthControl = false;

  // disable cross-attribute-type-prediction
  if (!isEnableCrossAttrTypePred) {
    vector<attrNeighborSet> neighborSet;
    neighborSet.resize(setlength);
    run_length_col = m_decBac->parseRunlength();
    run_length_refl = m_decBacDual->parseRunlength();
    for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      PC_COL& curColor = outputPointCloud.getColor(pointIndex, multil_ID);
      PC_REFL& curRefl = outputPointCloud.getReflectance(pointIndex, multil_ID);
      Bool isDuplicatePoint = curIndex > 0 && curPosition == outputPointCloud[prevIndex];
      if (isDuplicatePoint) {
        predictorColor = outputPointCloud.getColor(prevIndex, multil_ID);
        predictorRefl = outputPointCloud.getReflectance(prevIndex, multil_ID);
      } else {
        predictorAttr = getAttributePredictorFarthest(curIndex, curIndex, outputPointCloud, sps,
                                                      aps, pointCloudCode, setCount, neighborSet);
        predictorColor = predictorAttr.first;
        predictorRefl = predictorAttr.second;
      }
      // Entropy decode
      // length control
      if ((aps.coeffLengthControl > 0) && (run_length_col == aps.coeffLengthControl)) {
        colorLengthControl = true;
        --run_length_col;
      }
      if (run_length_col > 0) {
        codedValue = 0;
        --run_length_col;
      } else {
        ///<decode attribute correlation coding
        if (colorLengthControl) {
          codedValue = 0;
          run_length_col = m_decBac->parseRunlength();
          colorLengthControl = false;
        } else {
          if (!os) {
            parseColorResidualCorrelationCode(codedValue, isDuplicatePoint, aps.colorGolombNum);
          } else {
            parseColorResidualCorrelationCodeOS(codedValue, isDuplicatePoint, aps.colorGolombNum);
          }
          run_length_col = m_decBac->parseRunlength();
        }
      }
      colorReconstruction(predictorColor, codedValue, curColor, colorQp);
      // length control
      if ((aps.coeffLengthControl > 0) && (run_length_refl == aps.coeffLengthControl)) {
        reflLengthControl = true;
        --run_length_refl;
      }
      if (run_length_refl > 0) {
        codedValueRefl = 0;
        --run_length_refl;
      } else {
        if (reflLengthControl) {
          codedValueRefl = 0;
          run_length_refl = m_decBacDual->parseRunlength();
          reflLengthControl = false;
        } else {
          int countOfZeros = 0;
          codedValueRefl =
            m_decBacDual->parseRefl(countOfZeros, isDuplicatePoint, false, aps.refGolombNum);
          run_length_refl = m_decBacDual->parseRunlength();
        }
      }
      reflectanceReconstruction(predictorRefl, codedValueRefl, curRefl);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = outputPointCloud[pointIndex];
      neighborSet[farthestIdx].color = curColor;
      neighborSet[farthestIdx].refl = curRefl;
      prevIndex = pointIndex;
    }
    // enable cross-attribute-type-prediction , encode the color and then encode the reflectance
  } else if (!aps.attrEncodeOrder) {
    std::vector<reflNeighborSet> neighborSet;
    neighborSet.resize(setlength);
    std::vector<colorWithCoefNeighborSet> colorWithCoefNeighborSet;
    colorWithCoefNeighborSet.resize(setlength);
    vector<colorNeighborSet> reflbuffer;
    run_length_col = m_decBac->parseRunlength();
    run_length_refl = m_decBacDual->parseRunlength();
    for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      PC_COL& curColor = outputPointCloud.getColor(pointIndex, multil_ID);
      Bool isDuplicatePoint = curIndex > 0 && curPosition == outputPointCloud[prevIndex];
      if (isDuplicatePoint) {
        predictorColor = outputPointCloud.getColor(prevIndex, multil_ID);
      } else {
        predictorColor =
          getColorPredictorNoUpdate(curIndex, curIndex, outputPointCloud, sps, aps, pointCloudCode,
                                    setCount, neighborSet, colorWithCoefNeighborSet, reflbuffer);
      }
      // Entropy decode
      // length control
      if ((aps.coeffLengthControl > 0) && (run_length_col == aps.coeffLengthControl)) {
        colorLengthControl = true;
        --run_length_col;
      }
      if (run_length_col > 0) {
        codedValue = 0;
        --run_length_col;
      } else {
        if (colorLengthControl) {
          codedValue = 0;
          run_length_col = m_decBac->parseRunlength();
          colorLengthControl = false;
        } else {
          ///<decode attribute correlation coding
          if (!os) {
            parseColorResidualCorrelationCode(codedValue, isDuplicatePoint, aps.colorGolombNum);
          } else {
            parseColorResidualCorrelationCodeOS(codedValue, isDuplicatePoint, aps.colorGolombNum);
          }
          run_length_col = m_decBac->parseRunlength();
        }
      }
      colorReconstruction(predictorColor, codedValue, curColor, colorQp);
      // calculate the color with coefficient, which is used in cross-attribute-type-prediction
      V3<int64_t> curColorWithCoef;
      curColorWithCoef[0] = curColor[0] * crossAttrTypeCoef;
      curColorWithCoef[1] = curColor[1] * crossAttrTypeCoef;
      curColorWithCoef[2] = curColor[2] * crossAttrTypeCoef;
      PC_REFL& curRefl = outputPointCloud.getReflectance(pointIndex, multil_ID);
      if (isDuplicatePoint) {
        predictorRefl = outputPointCloud.getReflectance(prevIndex, multil_ID);
      } else {
        // get the better refl prediction through the geometric distance and the color distance
        predictorRefl = getReflectancePredictorFromColor(
          curIndex, curIndex, outputPointCloud, sps, aps, pointCloudCode, setCount, neighborSet,
          colorWithCoefNeighborSet, curColorWithCoef, reflectanceDistWeight);
      }
      // length control
      if ((aps.coeffLengthControl > 0) && (run_length_refl == aps.coeffLengthControl)) {
        reflLengthControl = true;
        --run_length_refl;
      }
      if (run_length_refl > 0) {
        codedValueRefl = 0;
        --run_length_refl;
      } else {
        if (reflLengthControl) {
          codedValueRefl = 0;
          run_length_refl = m_decBacDual->parseRunlength();
          reflLengthControl = false;
        } else {
          int countOfZeros = 0;
          codedValueRefl =
            m_decBacDual->parseRefl(countOfZeros, isDuplicatePoint, false, aps.refGolombNum);
          run_length_refl = m_decBacDual->parseRunlength();
        }
      }
      reflectanceReconstruction(predictorRefl, codedValueRefl, curRefl);
      if (curIndex != 0)
        calculateReflTrend(curPosition, prePosition, curRefl, preReflectance, reflectanceDistCoef,
                           reflectanceRes, reflResNum, reflectanceDistWeight, distWeightGroupSize);
      prePosition = curPosition;
      preReflectance = curRefl;
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = outputPointCloud[pointIndex];
      neighborSet[farthestIdx].refl = curRefl;
      colorWithCoefNeighborSet[farthestIdx].color = curColor;
      colorWithCoefNeighborSet[farthestIdx].colorWithCoef = curColorWithCoef;
      prevIndex = pointIndex;
    }
    // enable cross-attribute-type-prediction , encode the reflectance and then encode the color
  } else if (aps.attrEncodeOrder) {
    std::vector<colorNeighborSet> neighborSet;
    neighborSet.resize(setlength);
    std::vector<reflWithCoefNeighborSet> reflWithCoefNeighborSet;
    reflWithCoefNeighborSet.resize(setlength);
    vector<reflNeighborSet> reflbuffer;
    run_length_refl = m_decBacDual->parseRunlength();
    run_length_col = m_decBac->parseRunlength();
    for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      Bool isDuplicatePoint = curIndex > 0 && curPosition == outputPointCloud[prevIndex];
      PC_REFL& curRefl = outputPointCloud.getReflectance(pointIndex, multil_ID);
      if (isDuplicatePoint) {
        predictorRefl = outputPointCloud.getReflectance(prevIndex, multil_ID);
      } else {
        predictorRefl = getReflectancePredictorNoUpdate(
          curIndex, curIndex, outputPointCloud, sps, aps, pointCloudCode, setCount, neighborSet,
          reflWithCoefNeighborSet, reflbuffer, reflectanceDistWeight);
      }
      // length control
      if ((aps.coeffLengthControl > 0) && (run_length_refl == aps.coeffLengthControl)) {
        reflLengthControl = true;
        --run_length_refl;
      }
      if (run_length_refl > 0) {
        codedValueRefl = 0;
        --run_length_refl;
      } else {
        if (reflLengthControl) {
          codedValueRefl = 0;
          run_length_refl = m_decBacDual->parseRunlength();
          reflLengthControl = false;
        } else {
          int countOfZeros = 0;
          codedValueRefl =
            m_decBacDual->parseRefl(countOfZeros, isDuplicatePoint, false, aps.refGolombNum);
          run_length_refl = m_decBacDual->parseRunlength();
        }
      }
      reflectanceReconstruction(predictorRefl, codedValueRefl, curRefl);
      // calculate the reflectance with coefficient, which is used in cross-attribute-type-prediction
      uint64_t curReflWithCoef = curRefl * crossAttrTypeCoef;
      //
      if (curIndex != 0)
        calculateReflTrend(curPosition, prePosition, curRefl, preReflectance, reflectanceDistCoef,
                           reflectanceRes, reflResNum, reflectanceDistWeight, distWeightGroupSize);
      prePosition = curPosition;
      preReflectance = curRefl;
      PC_COL& curColor = outputPointCloud.getColor(pointIndex, multil_ID);
      V3<int64_t> codedValue;
      if (isDuplicatePoint) {
        predictorColor = outputPointCloud.getColor(prevIndex, multil_ID);
      } else {
        // get the better color prediction through the geometric distance and the reflectance distance
        predictorColor = getColorPredictorFromReflectance(
          curIndex, curIndex, outputPointCloud, sps, aps, pointCloudCode, setCount, neighborSet,
          reflWithCoefNeighborSet, curReflWithCoef);
      }
      // Entropy decode
      // length control
      if ((aps.coeffLengthControl > 0) && (run_length_col == aps.coeffLengthControl)) {
        colorLengthControl = true;
        --run_length_col;
      }
      if (run_length_col > 0) {
        codedValue = 0;
        --run_length_col;
      } else {
        if (colorLengthControl) {
          codedValue = 0;
          run_length_col = m_decBac->parseRunlength();
          colorLengthControl = false;
        } else {
          ///<decode attribute correlation coding
          if (!os) {
            parseColorResidualCorrelationCode(codedValue, isDuplicatePoint, aps.colorGolombNum);
          } else {
            parseColorResidualCorrelationCodeOS(codedValue, isDuplicatePoint, aps.colorGolombNum);
          }
          run_length_col = m_decBac->parseRunlength();
        }
      }
      colorReconstruction(predictorColor, codedValue, curColor, colorQp);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = outputPointCloud[pointIndex];
      neighborSet[farthestIdx].color = curColor;
      reflWithCoefNeighborSet[farthestIdx].refl = curRefl;
      reflWithCoefNeighborSet[farthestIdx].reflWithCoef = curReflWithCoef;
      prevIndex = pointIndex;
    }
  }
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::reflectanceInversePredictResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addReflectances();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.refReordermode, pointCloudCode, voxelCount,
          aps.axisBias);
  m_hls->sps.reflThreshold =
    (sps.reflQuantParam + abh.reflQPoffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  // get reflectance predictor
  int64_t predictorRefl;
  int64_t codedValue;
  std::vector<reflNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int setCount = 0;
  int prevIndex = -1;
  PC_REFL lastref = 0;
  // runlength
  const bool isGolomb = aps.refGolombNum == 1 ? true : false;
  bool isLengthControl = false;
  int run_length = m_decBac->parseRunlength();
  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    auto pointIndex = pointCloudCode[curIndex].index;
      PC_REFL& currentValue = outputPointCloud.getReflectance(pointIndex, multil_ID);
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      Bool isDuplicatePoint = curIndex > 0 && curPosition == outputPointCloud[prevIndex];

     if (isDuplicatePoint) {
        predictorRefl = lastref;
     } else {
        predictorRefl = getReflectancePredictorFarthest(curIndex, curIndex, outputPointCloud, sps,
                                                        aps, pointCloudCode, setCount, neighborSet);
     }
      isDuplicatePoint &= multil_ID == 0;
      // Entropy decode
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        isLengthControl = true;
        --run_length;
      }
      if (run_length > 0) {
        codedValue = 0;
        --run_length;
      } else {
        if (isLengthControl) {
          codedValue = 0;
          run_length = m_decBac->parseRunlength();
          isLengthControl = false;
        } else {
          codedValue = m_decBac->parseAttr(false, 3, 0, isDuplicatePoint, false, aps.refGolombNum);
          run_length = m_decBac->parseRunlength();
        }
      }
      reflectanceReconstruction(predictorRefl, codedValue, currentValue);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = outputPointCloud[pointIndex];
      neighborSet[farthestIdx].refl = currentValue;
      prevIndex = pointIndex;
      lastref = currentValue;
  }
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::colorInversePredictResidual() {
    const SequenceParameterSet& sps = m_hls->sps;
    const AttributeParameterSet& aps = m_hls->aps;
    TComPointCloud& outputPointCloud = *m_pointCloudRecon;
    const int voxelCount = int(outputPointCloud.getNumPoint());
    outputPointCloud.addColors();
    // Reorder
    std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
    reOrder(outputPointCloud.positions(), aps.colorReordermode, pointCloudCode, voxelCount,
                    1);
    //Qp compute
    quantizedQP colorQp;
    colorQp.attrQuantForLuma = sps.colorQuantParam;
    colorQp.attrQuantForChromaCb =
      TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
    colorQp.attrQuantForChromaCr =
      TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

    // obtain color predictor
    PC_COL predictorColor;
    V3<int64_t> codedValue;
    std::vector<colorNeighborSet> neighborSet;
    int setlength = aps.maxNumOfNeighbours;
    neighborSet.resize(setlength);
    PC_COL lastColor;
    int prevIndex = -1;
    int setCount = 0;
    // runlength
    int run_length = 0;
    const bool isGolomb = aps.colorGolombNum == 1 ? true : false;
    bool isLengthControl = false;
    run_length = m_decBac->parseRunlength();
    bool os = aps.orderSwitch;
    for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      PC_COL& curColor = outputPointCloud.getColor(pointIndex, multil_ID);
      Bool isDuplicatePoint = curIndex > 0 && curPosition == outputPointCloud[prevIndex];
      if (isDuplicatePoint) {
        predictorColor = lastColor;
      } else {
        predictorColor = getColorPredictorFarthest(curIndex, curIndex, outputPointCloud, sps, aps,
                                                   pointCloudCode, setCount, neighborSet);
      }
      // length control
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        isLengthControl = true;
        --run_length;
      }
      if (run_length > 0) {
        codedValue = 0;
        --run_length;
      } else {
        if (isLengthControl) {
          codedValue = 0;
          run_length = m_decBac->parseRunlength();
          isLengthControl = false;
        } else {
          ///<decode attribute correlation coding
          if (!os) {
            parseColorResidualCorrelationCode(codedValue, isDuplicatePoint, aps.colorGolombNum);
          } else {
            parseColorResidualCorrelationCodeOS(codedValue, isDuplicatePoint, aps.colorGolombNum);
          }
          run_length = m_decBac->parseRunlength();
        }
      }
      colorReconstruction(predictorColor, codedValue, curColor, colorQp);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = outputPointCloud[pointIndex];
      neighborSet[farthestIdx].color = curColor;
      prevIndex = pointIndex;
      lastColor = curColor;
    }
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::colorInversePredictAndTransformMemControl() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();
  //Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReordermode, pointCloudCode, voxelCount,
                  1);
  //Qp compute
  quantizedQP colorQp;
  colorQp.attrQuantForLuma = sps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

  // grouping
  UInt maxBB = std::max(
    {1U, sps.geomBoundingBoxSize[0], sps.geomBoundingBoxSize[1], sps.geomBoundingBoxSize[2]});
  int maxNodeSizeLog2 = ceilLog2(maxBB);
  int groupShiftBits = std::max(3, 3 * (maxNodeSizeLog2 - ((ceilLog2(voxelCount / 4) + 1) >> 1)));
  // adjust color QP per point tool
  int64_t minNeighborDis = 0;
  bool colorQPAdjustSliceFlag = aps.colorQPAdjustFlag;
  bool colorQPAdjustFlag = false;
  int scalar = 1;
  if (colorQPAdjustSliceFlag)
    scalar = aps.colorQPAdjustScalar;
  int colorQPAdjustDis = std::max(4, (1 << (groupShiftBits / 3 + 4)) / scalar);
  vector<int> length;
  vector<int> numofGroupCount;
  getLength(pointCloudCode, length, numofGroupCount, aps.maxNumofCoeff, groupShiftBits,
            aps.colorMaxTransNum);
  int subGroupCount = 0;
  int subgroupIndex = 0;
  // obtain color predictor
  PC_COL predictorColor;
  std::vector<int> transformPointIdx;
  std::vector<colorNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  // transform and entropy coding parameter
  int64_t transformBuf[3][8] = {};
  int64_t transformPredBuf[3][8] = {};
  const int maxNumofCoeff = max(aps.maxNumofCoeff, aps.colorMaxTransNum);
  int* CoeffGroup = new int[maxNumofCoeff * 3]();
  const bool colorGolomb = aps.colorGolombNum <= 2 ? true : false;

  int run_length = m_decBac->parseRunlength();
  bool isLengthControl = false;
  int dcIndex = 0;
  int acIndex = numofGroupCount[0];
  int numofCoeff = accumulate(length.begin(), length.begin() + numofGroupCount[0], 0);
  int lengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;
  runlengthDecodeMemControl(numofCoeff, CoeffGroup, run_length, lengthControl, isLengthControl,
                            true);
  int groupCount = 0;
  for (int curIndex = 0; curIndex < voxelCount;) {
    subGroupCount = 0;
    if (subgroupIndex == numofGroupCount[groupCount]) {
      setCoeffIndex(groupCount, numofGroupCount, length, dcIndex, acIndex, numofCoeff);
      assert(numofCoeff <= maxNumofCoeff);
      if (numofCoeff > 0)
        runlengthDecodeMemControl(numofCoeff, CoeffGroup, run_length, lengthControl,
                                  isLengthControl, true);
    }
    while (subGroupCount < length[subgroupIndex]) {
      transformPointIdx.push_back(curIndex);
      predictorColor =
        getColorPredictorFarthest(curIndex, transformPointIdx[0], outputPointCloud, sps, aps,
                                  pointCloudCode, subGroupCount, neighborSet, minNeighborDis);
      if (subGroupCount == 0) {
        for (int k = 0; k < 3; k++) {
          transformBuf[k][subGroupCount] = CoeffGroup[dcIndex * 3 + k];
          transformPredBuf[k][subGroupCount] = predictorColor[k];
        }
        ++dcIndex;
      } else {
        for (int k = 0; k < 3; k++) {
          transformBuf[k][subGroupCount] = CoeffGroup[acIndex * 3 + k];
          transformPredBuf[k][subGroupCount] = predictorColor[k];
        }
        ++acIndex;
      }
      ++subGroupCount;
      curIndex++;
    }
    if (colorQPAdjustSliceFlag)
      colorQPAdjustFlag = minNeighborDis > colorQPAdjustDis;
    colorReconstructionTrans(pointCloudCode, transformPointIdx, transformBuf, transformPredBuf,
                             colorQp, subGroupCount, neighborSet, colorQPAdjustFlag);
    transformPointIdx.erase(transformPointIdx.begin(), transformPointIdx.end());
    subgroupIndex++;
  }
  delete[] CoeffGroup;
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::ReflectanceInversePredictAndTransformMemControl() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addReflectances();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.refReordermode, pointCloudCode, voxelCount,
                  aps.axisBias);
  // weighted average parameter
  m_hls->sps.reflThreshold =
    (sps.reflQuantParam + abh.reflQPoffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  // grouping
  int MaxBits = ceilLog2((UInt64)sps.geomBoundingBoxSize[0] * (UInt64)sps.geomBoundingBoxSize[1] *
                         (UInt64)sps.geomBoundingBoxSize[2]);
  int MinBits = ceilLog2(voxelCount);
  int shift = ((sps.reflQuantParam + abh.reflQPoffset) >= 32) ? 12 : -6;
  int shiftBits = aps.reflMaxTransNum == 4 ? std::max(3, 3 * ((MaxBits - MinBits) / 3)) + shift : 3;
  std::vector<int> length;
  vector<int> numofGroupCount;
  getLengthRef(shift, pointCloudCode, length, numofGroupCount, aps.maxNumofCoeff, shiftBits,
               aps.reflMaxTransNum, true);
  int subGroupCount = 0;
  int subgroupIndex = 0;
  // obtain reflectance predictor
  int64_t predictorRefl;
  int64_t codedValue;
  std::vector<reflNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int prevIndex = -1;
  PC_REFL lastref = 0;
  std::vector<int> transformPointIdx;
  // transform and entropy coding parameter
  int64_t transformBuf[1][8] = {};
  int64_t transformPredBuf[1][8] = {};
  const bool refGolomb = aps.refGolombNum == 1 ? true : false;

  int run_length = m_decBac->parseRunlength();
  const int maxNumofCoeff = max(aps.maxNumofCoeff, aps.reflMaxTransNum);
  int* CoeffGroup = new int[maxNumofCoeff]();
  int dcIndex = 0;
  int acIndex = numofGroupCount[0];
  int numofCoeff = accumulate(length.begin(), length.begin() + numofGroupCount[0], 0);
  bool isLengthControl = false;
  int lengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;
  runlengthDecodeMemControl(numofCoeff, CoeffGroup, run_length, lengthControl, isLengthControl,
                            false);
  int groupCount = 0;
  //Inverse Transform
  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    if (subgroupIndex == numofGroupCount[groupCount]) {
      setCoeffIndex(groupCount, numofGroupCount, length, dcIndex, acIndex, numofCoeff);
      assert(numofCoeff <= maxNumofCoeff);
      if (numofCoeff > 0)
        runlengthDecodeMemControl(numofCoeff, CoeffGroup, run_length, lengthControl,
                                  isLengthControl, false);
    }
    if (subGroupCount == 0) {
      transformBuf[0][subGroupCount] = CoeffGroup[dcIndex];
      ++dcIndex;
    } else {
      transformBuf[0][subGroupCount] = CoeffGroup[acIndex];
      ++acIndex;
    }
    auto pointIndex = pointCloudCode[curIndex].index;
    PC_REFL& currentValue = outputPointCloud.getReflectance(pointIndex, multil_ID);
    const PC_POS& curPosition = outputPointCloud[pointIndex];
    Bool isDuplicatePoint = curIndex > length[0] && curPosition == outputPointCloud[prevIndex];

    if (isDuplicatePoint) {
      predictorRefl = lastref;
      isDuplicatePoint &= multil_ID == 0;
      reflectanceReconstruction(predictorRefl, transformBuf[0][subGroupCount], currentValue);
      subgroupIndex++;
      prevIndex = pointIndex;
      lastref = currentValue;
    } else {
      transformPointIdx.push_back(curIndex);
      if (aps.refGroupPredict && (length[subgroupIndex] < 3)) {
        if (subGroupCount == 0) {
          predictorRefl =
            getReflectancePredictorFarthest(curIndex, transformPointIdx[0], outputPointCloud, sps,
                                            aps, pointCloudCode, subGroupCount, neighborSet);
        }
      } else {
        predictorRefl =
          getReflectancePredictorFarthest(curIndex, transformPointIdx[0], outputPointCloud, sps,
                                          aps, pointCloudCode, subGroupCount, neighborSet);
      }
      transformPredBuf[0][subGroupCount] = predictorRefl;
      ++subGroupCount;
      if (subGroupCount == length[subgroupIndex]) {
        reflectanceReconstructionTrans(pointCloudCode, transformPointIdx, transformBuf,
                                       transformPredBuf, neighborSet, lastref);
        subGroupCount = 0;
        subgroupIndex++;
        prevIndex = pointIndex;
        transformPointIdx.erase(transformPointIdx.begin(), transformPointIdx.end());
      }
    }
  }
  delete[] CoeffGroup;
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::AttributeInversePredictAndTransformMemControl() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;

  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();
  outputPointCloud.addReflectances();

  //Hilbert Sort
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReordermode, pointCloudCode, voxelCount,
                  1);

  //Qp compute
  quantizedQP colorQp;
  colorQp.attrQuantForLuma = sps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);
  // weighted average parameter
  m_hls->sps.reflThreshold =
    (sps.reflQuantParam + abh.reflQPoffset) * aps.nearestPredParam1 + aps.nearestPredParam2;

  //dist weight calculation patameter
  auto boudingSize = (sps.geomBoundingBoxSize[0] * sps.geomBoundingBoxSize[1] +
                      sps.geomBoundingBoxSize[0] * sps.geomBoundingBoxSize[2] +
                      sps.geomBoundingBoxSize[1] * sps.geomBoundingBoxSize[2]) *
    2;
  uint64_t reflectanceDistCoef = log2(round(boudingSize / voxelCount));

  std::vector<int64_t> reflectanceRes = {0, 0, 0};
  std::vector<int64_t> reflResNum = {0, 0, 0};
  std::vector<int> reflectanceDistWeight = {0, 0, 0};

  PC_POS prePosition = outputPointCloud[pointCloudCode[0].index];
  PC_REFL preReflectance = 0;
  int distWeightGroupSize = 1 << aps.log2predDistWeightGroupSize;
  // color grouping
  PC_COL predictorColor;
  int64_t transformBufColor[3][8] = {};
  int64_t transformPredBufColor[3][8] = {};
  vector<int> lengthColor;
  std::vector<int> transformPointIdxColor;
  transformPointIdxColor.reserve(8);
  std::vector<colorNeighborSet> neighborSetColor;
  neighborSetColor.resize(aps.maxNumOfNeighbours);
  std::vector<reflWithCoefNeighborSet> reflWithCoefNeighborSet;
  reflWithCoefNeighborSet.resize(aps.maxNumOfNeighbours);
  int countColor = 0;
  UInt maxBB = std::max(
    {1U, sps.geomBoundingBoxSize[0], sps.geomBoundingBoxSize[1], sps.geomBoundingBoxSize[2]});
  int maxNodeSizeLog2 = ceilLog2(maxBB);
  int groupShiftBits = std::max(3, 3 * (maxNodeSizeLog2 - ((ceilLog2(voxelCount / 4) + 1) >> 1)));
  // adjust color QP per point tool
  int64_t minNeighborDis = 0;
  bool colorQPAdjustSliceFlag = aps.colorQPAdjustFlag;
  bool colorQPAdjustFlag = false;
  int scalar = 1;
  if (colorQPAdjustSliceFlag)
    scalar = aps.colorQPAdjustScalar;
  int colorQPAdjustDis = std::max(4, (1 << (groupShiftBits / 3 + 4)) / scalar);
  int subgroupIndexColor = 0;
  vector<int> numofGroupCountColor;
  getLength(pointCloudCode, lengthColor, numofGroupCountColor, aps.maxNumofCoeff, groupShiftBits,
            aps.colorMaxTransNum);
  // ref grouping
  vector<int> lengthRefl;
  vector<int> numofGroupCountRefl;
  vector<int> transformPointIdxRefl;
  transformPointIdxRefl.reserve(8);
  std::vector<reflNeighborSet> neighborSetRefl;
  neighborSetRefl.resize(aps.maxNumOfNeighbours);
  int setlength = aps.maxNumOfNeighbours;
  std::vector<colorWithCoefNeighborSet> colorWithCoefNeighborSet;
  colorWithCoefNeighborSet.resize(aps.maxNumOfNeighbours);
  bool dupPredFlagRefl = 0;
  int ther =
    (sps.reflQuantParam + abh.reflQPoffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  PC_REFL lastref;
  int64_t transformBufRefl[3][8] = {};
  int64_t transformPredBufRefl[3][8] = {};
  PC_REFL predictorRefl;
  int countRefl = 0;
  int subgroupIndexRefl = 0;
  int MaxBits = ceilLog2((UInt64)sps.geomBoundingBoxSize[0] * (UInt64)sps.geomBoundingBoxSize[1] *
                         (UInt64)sps.geomBoundingBoxSize[2]);
  int MinBits = ceilLog2(voxelCount);
  int shift = ((sps.reflQuantParam + abh.reflQPoffset) >= 32) ? 12 : -6;
  groupShiftBits =
    aps.reflMaxTransNum == 4 ? std::max(3, 3 * ((MaxBits - MinBits) / 3)) + shift : 3;

  if (aps.reflMaxTransNum > 1) {
    getLengthRef(shift, pointCloudCode, lengthRefl, numofGroupCountRefl, aps.maxNumofCoeff,
                 groupShiftBits, aps.reflMaxTransNum, false);
  } else {
    lengthRefl.resize(voxelCount, 1);
    numofGroupCountRefl.resize((voxelCount / aps.maxNumofCoeff + 1));
    if (aps.maxNumofCoeff < voxelCount) {
      numofGroupCountRefl[0] = aps.maxNumofCoeff;
      for (int i = 1; i < voxelCount / aps.maxNumofCoeff; i++) {
        numofGroupCountRefl[i] = aps.maxNumofCoeff + numofGroupCountRefl[i - 1];
      }
      numofGroupCountRefl[voxelCount / aps.maxNumofCoeff] = voxelCount;
    } else {
      numofGroupCountRefl[0] = voxelCount;
    }
  }
  bool isEnableCrossAttrTypePred = aps.crossAttrTypePred;
  bool attrEncodeOrder = aps.attrEncodeOrder;

  uint64_t crossAttrTypeLambda = 0;
  int64_t crossAttrTypeCoef = 0;
  vector<V3<int64_t>> colorWithCoef(0);
  vector<uint64_t> reflWithCoef(0);
  if (isEnableCrossAttrTypePred && (!aps.attrEncodeOrder)) {
    crossAttrTypeLambda =
      (-sps.colorQuantParam * aps.crossAttrTypePredParam1 + aps.crossAttrTypePredParam2);
    UInt maxPos =
      sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2];
    uint64_t maxColor = (1 << m_hls->aps.colorOutputDepth) - 1;
    uint64_t maxColorSum = 3 * maxColor;
    crossAttrTypeCoef = round((maxPos << 10) / (double)maxColorSum);
    crossAttrTypeCoef = uint64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;
    colorWithCoef.resize(voxelCount);
  } else if (isEnableCrossAttrTypePred && (aps.attrEncodeOrder)) {
    crossAttrTypeLambda = (-(sps.reflQuantParam + abh.reflQPoffset) * aps.crossAttrTypePredParam1 +
                           aps.crossAttrTypePredParam2);
    auto diffPos =
      sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2];
    uint64_t maxRefl = (1 << m_hls->aps.reflOutputDepth) - 1;
    crossAttrTypeCoef = round((diffPos << 10) / (double)maxRefl);
    crossAttrTypeCoef = uint64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;
    reflWithCoef.resize(voxelCount, 0);
  }

  const bool colorGolomb = aps.colorGolombNum <= 2 ? true : false;
  int run_length_col = m_decBac->parseRunlength();
  const bool refGolomb = aps.refGolombNum == 1 ? true : false;
  int run_length_refl = m_decBacDual->parseRunlength();
  int LengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;
  bool isLengthControlRefl = false;
  bool isLengthControlColor = false;
  const int maxNumofCoeff = max(aps.maxNumofCoeff, (UInt)8);
  int* CoeffGroupColor = new int[maxNumofCoeff * 3]();
  int dcIndexColor = 0;
  int acIndexColor = numofGroupCountColor[0];
  int numofCoeffColor = 0;
  int groupCountColor = 0;
  int countNumColor = lengthColor[0];
  int* CoeffGroupRefl = new int[maxNumofCoeff]();
  int dcIndexRefl = 0;
  int acIndexRefl = numofGroupCountRefl[0];
  int numofCoeffRefl = 0;
  int groupCountRefl = 0;
  int countNumRefl = lengthRefl[0];
  int codedValueRefl;

  numofCoeffRefl =
    accumulate(lengthRefl.begin(), lengthRefl.begin() + numofGroupCountRefl[0], 0);
  runlengthDecodeMemControl(numofCoeffRefl, CoeffGroupRefl, run_length_refl, LengthControl,
                            isLengthControlRefl, false);

  numofCoeffColor =
    accumulate(lengthColor.begin(), lengthColor.begin() + numofGroupCountColor[0], 0);
  runlengthDecodeMemControl(numofCoeffColor, CoeffGroupColor, run_length_col, LengthControl,
                            isLengthControlColor, true);

  //Predicting
  //InverseTransform
  //disable cross-attribute-type-prediction
  if (!isEnableCrossAttrTypePred) {
    for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
      //predict and code
      auto pointIndex = pointCloudCode[curIndex].index;
      transformPointIdxRefl.push_back(curIndex);
      transformPointIdxColor.push_back(curIndex);
      if (aps.refGroupPredict && (countNumRefl < 3)) {
        if (countRefl == 0) {
          predictorRefl =
            getReflectancePredictorFarthest(curIndex, transformPointIdxRefl[0], outputPointCloud,
                                            sps, aps, pointCloudCode, countRefl, neighborSetRefl);
        }
      } else {
        predictorRefl =
          getReflectancePredictorFarthest(curIndex, transformPointIdxRefl[0], outputPointCloud, sps,
                                          aps, pointCloudCode, countRefl, neighborSetRefl);
      }

      if (countRefl == 0) {
        transformBufRefl[0][countRefl] = CoeffGroupRefl[dcIndexRefl];
        ++dcIndexRefl;
      } else {
        transformBufRefl[0][countRefl] = CoeffGroupRefl[acIndexRefl];
        ++acIndexRefl;
      }
      transformPredBufRefl[0][countRefl] = predictorRefl;
      countRefl++;

      if (countRefl == countNumRefl) {
        reflectanceReconstructionTrans(pointCloudCode, transformPointIdxRefl, transformBufRefl,
                                       transformPredBufRefl, neighborSetRefl, lastref);
        countRefl = 0;
        subgroupIndexRefl++;
        if (subgroupIndexRefl < lengthRefl.size())
          countNumRefl = lengthRefl[subgroupIndexRefl];
        transformPointIdxRefl.erase(transformPointIdxRefl.begin(), transformPointIdxRefl.end());
        if (subgroupIndexRefl == numofGroupCountRefl[groupCountRefl]) {
          setCoeffIndex(groupCountRefl, numofGroupCountRefl, lengthRefl, dcIndexRefl, acIndexRefl,
                        numofCoeffRefl);
          assert(numofCoeffRefl <= maxNumofCoeff);
          if (subgroupIndexRefl < lengthRefl.size()) {
            if (numofCoeffRefl > 0)
              runlengthDecodeMemControl(numofCoeffRefl, CoeffGroupRefl, run_length_refl,
                                        LengthControl, isLengthControlRefl, false);
          }
        }
      }

      predictorColor =
        getColorPredictorFarthest(curIndex, transformPointIdxColor[0], outputPointCloud, sps, aps,
                                  pointCloudCode, countColor, neighborSetColor, minNeighborDis);
      if (countColor == 0) {
        for (int k = 0; k < 3; k++) {
          transformBufColor[k][countColor] = CoeffGroupColor[dcIndexColor * 3 + k];
          transformPredBufColor[k][countColor] = predictorColor[k];
        }
        ++dcIndexColor;
      } else {
        for (int k = 0; k < 3; k++) {
          transformBufColor[k][countColor] = CoeffGroupColor[acIndexColor * 3 + k];
          transformPredBufColor[k][countColor] = predictorColor[k];
        }
        ++acIndexColor;
      }
      countColor++;

      if (countColor == countNumColor) {
        if (colorQPAdjustSliceFlag)
          colorQPAdjustFlag = minNeighborDis > colorQPAdjustDis;
        colorReconstructionTrans(pointCloudCode, transformPointIdxColor, transformBufColor,
                                 transformPredBufColor, colorQp, countColor, neighborSetColor,
                                 colorQPAdjustFlag);
        transformPointIdxColor.erase(transformPointIdxColor.begin(), transformPointIdxColor.end());
        subgroupIndexColor++;
        countColor = 0;
        if (subgroupIndexColor < lengthColor.size())
          countNumColor = lengthColor[subgroupIndexColor];
        if (subgroupIndexColor == numofGroupCountColor[groupCountColor]) {
          setCoeffIndex(groupCountColor, numofGroupCountColor, lengthColor, dcIndexColor,
                        acIndexColor, numofCoeffColor);
          assert(numofCoeffColor <= maxNumofCoeff);
          if (subgroupIndexColor < lengthColor.size()) {
            if (numofCoeffColor > 0)
              runlengthDecodeMemControl(numofCoeffColor, CoeffGroupColor, run_length_col,
                                        LengthControl, isLengthControlColor, true);
          }
        }
      }
    }
  }
  // predict reflectance using color
  else if (!attrEncodeOrder) {
    for (int curIndexColor = 0, curIndexRefl = 0; curIndexRefl < voxelCount;) {
      while (curIndexColor < (curIndexRefl + countNumRefl) ||
             (countColor < countNumColor && countColor > 0)) {
        if (countColor == 0 && (curIndexColor > (curIndexRefl + countNumRefl)))
          break;
        transformPointIdxColor.push_back(curIndexColor);

        predictorColor = getColorPredictorFarthest(curIndexColor, transformPointIdxColor[0],
                                                   outputPointCloud, sps, aps, pointCloudCode,
                                                   countColor, neighborSetColor, minNeighborDis);
        if (countColor == 0) {
          for (int k = 0; k < 3; k++) {
            transformBufColor[k][countColor] = CoeffGroupColor[dcIndexColor * 3 + k];
            transformPredBufColor[k][countColor] = predictorColor[k];
          }
          ++dcIndexColor;
        } else {
          for (int k = 0; k < 3; k++) {
            transformBufColor[k][countColor] = CoeffGroupColor[acIndexColor * 3 + k];
            transformPredBufColor[k][countColor] = predictorColor[k];
          }
          ++acIndexColor;
        }
        countColor++;

        if (countColor == countNumColor) {
          if (colorQPAdjustSliceFlag)
            colorQPAdjustFlag = minNeighborDis > colorQPAdjustDis;
          colorReconstructionTrans(pointCloudCode, transformPointIdxColor, transformBufColor,
                                   transformPredBufColor, colorQp, countColor, neighborSetColor,
                                   colorQPAdjustFlag);
          for (int i = 0; i < countNumColor; i++) {
            auto pointIndex = pointCloudCode[transformPointIdxColor[i]].index;
            colorWithCoef[pointIndex][0] =
              outputPointCloud.getColor(pointIndex, multil_ID)[0] * crossAttrTypeCoef;
            colorWithCoef[pointIndex][1] =
              outputPointCloud.getColor(pointIndex, multil_ID)[1] * crossAttrTypeCoef;
            colorWithCoef[pointIndex][2] =
              outputPointCloud.getColor(pointIndex, multil_ID)[2] * crossAttrTypeCoef;
          }
          transformPointIdxColor.erase(transformPointIdxColor.begin(),
                                       transformPointIdxColor.end());
          subgroupIndexColor++;
          countColor = 0;
          if (subgroupIndexColor < lengthColor.size())
            countNumColor = lengthColor[subgroupIndexColor];
          if (subgroupIndexColor == numofGroupCountColor[groupCountColor]) {
            setCoeffIndex(groupCountColor, numofGroupCountColor, lengthColor, dcIndexColor,
                          acIndexColor, numofCoeffColor);
            assert(numofCoeffColor <= maxNumofCoeff);
            if (numofCoeffColor > 0)
              runlengthDecodeMemControl(numofCoeffColor, CoeffGroupColor, run_length_col,
                                        LengthControl, isLengthControlColor, true);
          }
        }
        countRefl++;
        curIndexColor++;
      }
      // reflectance predict and transform
      countRefl = 0;
      if (subgroupIndexRefl == numofGroupCountRefl[groupCountRefl]) {
        setCoeffIndex(groupCountRefl, numofGroupCountRefl, lengthRefl, dcIndexRefl, acIndexRefl,
                      numofCoeffRefl);
        assert(numofCoeffRefl <= maxNumofCoeff);

        if (numofCoeffRefl > 0)
          runlengthDecodeMemControl(numofCoeffRefl, CoeffGroupRefl, run_length_refl, LengthControl,
                                    isLengthControlRefl, false);
      }
      while (countRefl < countNumRefl) {
        transformPointIdxRefl.push_back(curIndexRefl);
        auto pointIndex = pointCloudCode[curIndexRefl].index;
        if (aps.refGroupPredict && (countNumRefl < 3)) {
          if (countRefl == 0) {
            predictorRefl = getReflectancePredictorFromColor(
              curIndexRefl, transformPointIdxRefl[0], outputPointCloud, sps, aps, pointCloudCode,
              countRefl, neighborSetRefl, colorWithCoefNeighborSet, colorWithCoef[pointIndex],
              reflectanceDistWeight);
          }
        } else {
          predictorRefl = getReflectancePredictorFromColor(
            curIndexRefl, transformPointIdxRefl[0], outputPointCloud, sps, aps, pointCloudCode,
            countRefl, neighborSetRefl, colorWithCoefNeighborSet, colorWithCoef[pointIndex],
            reflectanceDistWeight);
        }

        if (countRefl == 0) {
          transformBufRefl[0][countRefl] = CoeffGroupRefl[dcIndexRefl];
          ++dcIndexRefl;
        } else {
          transformBufRefl[0][countRefl] = CoeffGroupRefl[acIndexRefl];
          ++acIndexRefl;
        }
        transformPredBufRefl[0][countRefl] = predictorRefl;
        countRefl++;
        curIndexRefl++;
      }
      reflectanceReconstructionTrans(pointCloudCode, transformPointIdxRefl, transformBufRefl,
                                     transformPredBufRefl, neighborSetRefl, lastref);

      if (transformPointIdxRefl[0] <= setlength) {
        for (int i = 0; i < countRefl; ++i) {
          auto pointIndex = pointCloudCode[transformPointIdxRefl[i]].index;
          colorWithCoefNeighborSet[transformPointIdxRefl[i] % setlength].colorWithCoef =
            colorWithCoef[pointIndex];
        }
      } else {
        for (int i = 0; i < countRefl; ++i) {
          auto pointIndex = pointCloudCode[transformPointIdxRefl[i]].index;
          colorWithCoefNeighborSet[i].colorWithCoef = colorWithCoef[pointIndex];
          PC_REFL curReflectance = outputPointCloud.getReflectance(pointIndex, multil_ID);
          calculateReflTrend(outputPointCloud[pointIndex], prePosition, curReflectance,
                             preReflectance, reflectanceDistCoef, reflectanceRes, reflResNum,
                             reflectanceDistWeight, distWeightGroupSize);

          preReflectance = curReflectance;
          prePosition = outputPointCloud[pointIndex];
        }
      }
      transformPointIdxRefl.erase(transformPointIdxRefl.begin(), transformPointIdxRefl.end());
      subgroupIndexRefl++;
      countRefl = 0;
      if (subgroupIndexRefl < lengthRefl.size())
        countNumRefl = lengthRefl[subgroupIndexRefl];
    }
  }  // predict color using reflectance CTC default
  else {
    for (int curIndexColor = 0, curIndexRefl = 0; curIndexColor < voxelCount;) {
      //reflectance predict and transform
      while (curIndexRefl < (curIndexColor + countNumColor) ||
             (countRefl < countNumRefl && countRefl > 0)) {
        if (countRefl == 0 && (curIndexRefl > (curIndexColor + countNumColor)))
          break;
        transformPointIdxRefl.push_back(curIndexRefl);

        if (aps.refGroupPredict && (countNumRefl < 3)) {
          if (countRefl == 0) {
            if (aps.log2predDistWeightGroupSize)
              predictorRefl = getReflectancePredictorFarthest(
                curIndexRefl, transformPointIdxRefl[0], outputPointCloud, sps, aps, pointCloudCode,
                countRefl, neighborSetRefl, reflectanceDistWeight);
            else
              predictorRefl = getReflectancePredictorFarthest(
                curIndexRefl, transformPointIdxRefl[0], outputPointCloud, sps, aps, pointCloudCode,
                countRefl, neighborSetRefl);
          }
        } else {
          if (aps.log2predDistWeightGroupSize)
            predictorRefl = getReflectancePredictorFarthest(
              curIndexRefl, transformPointIdxRefl[0], outputPointCloud, sps, aps, pointCloudCode,
              countRefl, neighborSetRefl, reflectanceDistWeight);
          else
            predictorRefl = getReflectancePredictorFarthest(
              curIndexRefl, transformPointIdxRefl[0], outputPointCloud, sps, aps, pointCloudCode,
              countRefl, neighborSetRefl);
        }

        if (countRefl == 0) {
          transformBufRefl[0][countRefl] = CoeffGroupRefl[dcIndexRefl];
          ++dcIndexRefl;
        } else {
          transformBufRefl[0][countRefl] = CoeffGroupRefl[acIndexRefl];
          ++acIndexRefl;
        }
        transformPredBufRefl[0][countRefl] = predictorRefl;
        countRefl++;

        if (countRefl == countNumRefl) {
          reflectanceReconstructionTrans(pointCloudCode, transformPointIdxRefl, transformBufRefl,
                                         transformPredBufRefl, neighborSetRefl, lastref);
          for (int i = 0; i < countNumRefl; i++) {
            auto pointIndex = pointCloudCode[transformPointIdxRefl[i]].index;
            PC_REFL curReflectance = outputPointCloud.getReflectance(pointIndex, multil_ID);
            reflWithCoef[pointIndex] = curReflectance * crossAttrTypeCoef;
            calculateReflTrend(outputPointCloud[pointIndex], prePosition, curReflectance,
                               preReflectance, reflectanceDistCoef, reflectanceRes, reflResNum,
                               reflectanceDistWeight, distWeightGroupSize);

            preReflectance = curReflectance;
            prePosition = outputPointCloud[pointIndex];
          }
          countRefl = 0;
          subgroupIndexRefl++;
          if (subgroupIndexRefl < lengthRefl.size())
            countNumRefl = lengthRefl[subgroupIndexRefl];
          transformPointIdxRefl.erase(transformPointIdxRefl.begin(), transformPointIdxRefl.end());
          if (subgroupIndexRefl == numofGroupCountRefl[groupCountRefl]) {
            setCoeffIndex(groupCountRefl, numofGroupCountRefl, lengthRefl, dcIndexRefl, acIndexRefl,
                          numofCoeffRefl);
            assert(numofCoeffRefl <= maxNumofCoeff);

            if (numofCoeffRefl > 0)
              runlengthDecodeMemControl(numofCoeffRefl, CoeffGroupRefl, run_length_refl,
                                        LengthControl, isLengthControlRefl, false);
          }
        }
        countColor++;
        curIndexRefl++;
      }

      // color predict and transform
      countColor = 0;
      if (subgroupIndexColor == numofGroupCountColor[groupCountColor]) {
        setCoeffIndex(groupCountColor, numofGroupCountColor, lengthColor, dcIndexColor,
                      acIndexColor, numofCoeffColor);
        assert(numofCoeffColor <= maxNumofCoeff);
        if (numofCoeffColor > 0)
          runlengthDecodeMemControl(numofCoeffColor, CoeffGroupColor, run_length_col, LengthControl,
                                    isLengthControlColor, true);
      }
      while (countColor < countNumColor) {
        transformPointIdxColor.push_back(curIndexColor);
        auto pointIndex = pointCloudCode[curIndexColor].index;
        predictorColor = getColorPredictorFromReflectance(
          curIndexColor, transformPointIdxColor[0], outputPointCloud, sps, aps, pointCloudCode,
          countColor, neighborSetColor, reflWithCoefNeighborSet, reflWithCoef[pointIndex],
          minNeighborDis);

        if (countColor == 0) {
          for (int k = 0; k < 3; k++) {
            transformBufColor[k][countColor] = CoeffGroupColor[dcIndexColor * 3 + k];
            transformPredBufColor[k][countColor] = predictorColor[k];
          }
          ++dcIndexColor;
        } else {
          for (int k = 0; k < 3; k++) {
            transformBufColor[k][countColor] = CoeffGroupColor[acIndexColor * 3 + k];
            transformPredBufColor[k][countColor] = predictorColor[k];
          }
          ++acIndexColor;
        }
        countColor++;
        curIndexColor++;
      }
      if (colorQPAdjustSliceFlag)
        colorQPAdjustFlag = minNeighborDis > colorQPAdjustDis;
      colorReconstructionTrans(pointCloudCode, transformPointIdxColor, transformBufColor,
                               transformPredBufColor, colorQp, countColor, neighborSetColor,
                               colorQPAdjustFlag);

      if (transformPointIdxColor[0] <= setlength) {
        for (int i = 0; i < countColor; ++i) {
          auto pointIndex = pointCloudCode[transformPointIdxColor[i]].index;
          reflWithCoefNeighborSet[transformPointIdxColor[i] % setlength].reflWithCoef =
            reflWithCoef[pointIndex];
        }
      } else {
        for (int i = 0; i < countColor; ++i) {
          auto pointIndex = pointCloudCode[transformPointIdxColor[i]].index;
          reflWithCoefNeighborSet[i].reflWithCoef = reflWithCoef[pointIndex];
        }
      }
      transformPointIdxColor.erase(transformPointIdxColor.begin(), transformPointIdxColor.end());
      subgroupIndexColor++;
      countColor = 0;
      if (subgroupIndexColor < lengthColor.size())
        countNumColor = lengthColor[subgroupIndexColor];
    }
  }
  delete[] CoeffGroupColor;
  delete[] CoeffGroupRefl;
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::reflectanceReconstruction(const int64_t& predictor, const int64_t& codedValue,
                                              PC_REFL& reconValue) {
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (m_hls->aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (m_hls->aps.reflOutputDepth)) - 1;
  }
  int64_t delta = codedValue;
  int sign = (delta < 0) ? -1 : 1;
  uint64_t absDelta = std::abs(delta);
  //inverse quantitzation
  uint64_t inverseResidualQuant =
    InverseQuantizeResidual(absDelta, m_hls->sps.reflQuantParam + m_hls->abh.reflQPoffset);
  int64_t residual = inverseResidualQuant * sign;
  reconValue = TComClip((Int64)ClipMin, (Int64)ClipMax, residual + predictor);
}

void TDecAttribute::colorReconstruction(const PC_COL& predictor, const V3<int64_t>& codedValue,
                                        PC_COL& reconValue, const quantizedQP& colorQp) {
  int residualPrevComponent = 0;
  bool ccp = m_hls->aps.crossComponentPred;
  UInt colorQuantParam;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;
  for (int i = 0; i < 3; i++) {
    colorQuantParam = (i == 0)
      ? colorQp.attrQuantForLuma
      : ((i == 1) ? colorQp.attrQuantForChromaCb : colorQp.attrQuantForChromaCr);
    int64_t delta = codedValue[i];
    int sign = (delta < 0) ? -1 : 1;
    uint64_t absDelta = std::abs(delta);
    //inverse quantitzation
    uint64_t inverseResidualQuant = InverseQuantizeResidual(absDelta, colorQuantParam);
    int64_t residual = inverseResidualQuant * sign;
    reconValue[i] =
      TComClip((Int64)ClipMin, (Int64)ClipMax, residual + predictor[i] + residualPrevComponent);
    if (ccp && i <= 1) {
      residualPrevComponent = (Int)reconValue[i] - (Int)predictor[i];
    }
  }
}

void TDecAttribute::parseColorResidualCorrelationCode(V3<int64_t>& codedValue,
                                                      const bool isDuplicatePoint,
                                                      const UInt& golombNum) {
  bool isColor = true;
  bool residualminusone = true;
  int flagy_r = m_decBac->parseAttrequalone0();
  if (flagy_r == 1) {
    codedValue[0] = 0;
  } else {
    codedValue[0] =
      m_decBac->parseAttr(isColor, 0, 1, 0 == 0 && isDuplicatePoint, residualminusone, golombNum);
  }
  if (codedValue[0] == 0) {
    int flagyu_rg = m_decBac->parseAttrequaltwo0();
    if (flagyu_rg == 1) {
      codedValue[1] = 0;
      codedValue[2] =
        m_decBac->parseAttr(isColor, 2, 0, 2 == 0 && isDuplicatePoint, residualminusone, golombNum);
      m_decBac->parseSign(codedValue[2]);
    } else {
      codedValue[1] =
        m_decBac->parseAttr(isColor, 1, 2, 1 == 0 && isDuplicatePoint, residualminusone, golombNum);
      codedValue[2] = m_decBac->parseAttr(isColor, 2, 0, 2 == 0 && isDuplicatePoint,
                                          !residualminusone, golombNum);
      m_decBac->parseSign(codedValue[1]);
      if (codedValue[2] != 0) {
        m_decBac->parseSign(codedValue[2]);
      }
    }
  } else {
    codedValue[1] =
      m_decBac->parseAttr(isColor, 1, 1, 1 == 0 && isDuplicatePoint, !residualminusone, golombNum);

    int b0 = 0;
    if (abs(codedValue[0]) > abs(codedValue[1])) {
      b0 = 1;
    } else {
      b0 = 0;
    }
    codedValue[2] = m_decBac->parseAttr(isColor, 2, 2, 2 == 0 && isDuplicatePoint,
                                        !residualminusone, golombNum, b0);
    m_decBac->parseSign(codedValue[0]);
    if (codedValue[1] != 0) {
      m_decBac->parseSign(codedValue[1]);
    }
    if (codedValue[2] != 0) {
      m_decBac->parseSign(codedValue[2]);
    }
  }
}

void TDecAttribute::parseColorResidualCorrelationCodeOS(V3<int64_t>& codedValue,
                                                        const bool isDuplicatePoint,
                                                        const UInt& golombNum) {
  bool isColor = true;
  bool residualminusone = true;
  int flagu_g = m_decBac->parseAttrequalone0();
  if (flagu_g == 1) {
    codedValue[1] = 0;
  } else {
    codedValue[1] =
      m_decBac->parseAttr(isColor, 1, 1, 1 == 0 && isDuplicatePoint, residualminusone, golombNum);
  }
  if (codedValue[1] == 0) {
    int flaguy_gr = m_decBac->parseAttrequaltwo0();
    if (flaguy_gr == 1) {
      codedValue[0] = 0;
      codedValue[2] =
        m_decBac->parseAttr(isColor, 2, 0, 2 == 0 && isDuplicatePoint, residualminusone, golombNum);
      m_decBac->parseSign(codedValue[2]);
    } else {
      codedValue[0] =
        m_decBac->parseAttr(isColor, 0, 2, 0 == 0 && isDuplicatePoint, residualminusone, golombNum);

      codedValue[2] = m_decBac->parseAttr(isColor, 2, 0, 2 == 0 && isDuplicatePoint,
                                          !residualminusone, golombNum);
      m_decBac->parseSign(codedValue[0]);
      if (codedValue[2] != 0) {
        m_decBac->parseSign(codedValue[2]);
      }
    }
  } else {
    codedValue[0] =
      m_decBac->parseAttr(isColor, 0, 1, 0 == 0 && isDuplicatePoint, !residualminusone, golombNum);

    int b0 = 0;
    if (abs(codedValue[1]) > abs(codedValue[0])) {
      b0 = 1;
    } else {
      b0 = 0;
    }
    codedValue[2] = m_decBac->parseAttr(isColor, 2, 2, 2 == 0 && isDuplicatePoint,
                                        !residualminusone, golombNum, b0);
    m_decBac->parseSign(codedValue[1]);
    if (codedValue[0] != 0) {
      m_decBac->parseSign(codedValue[0]);
    }
    if (codedValue[2] != 0) {
      m_decBac->parseSign(codedValue[2]);
    }
  }
}

void TDecAttribute::colorReconstructionTrans(
  std::vector<pointCodeWithIndex>& pointCloudHilbert, std::vector<int>& transformPointIdx,
  int64_t transformBuf[][8], int64_t transformPredBuf[][8], const quantizedQP& colorQp, int& count,
  std::vector<colorNeighborSet>& neighborSet, bool colorQPAdjustFlag) {
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const AttributeParameterSet& aps = m_hls->aps;
  UInt colorQuantParam;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;
  int transform_shift = 9;
  Int64 add = 1 << (transform_shift * 2 - 1);
  int transform_shift_QP = transform_shift * 8;
  int resCodeQPOffset = 8;
  for (int idx = 0; idx < count; ++idx) {
    for (int k = 0; k < 3; k++) {
      int64_t delta = transformBuf[k][idx];
      int sign = (delta < 0) ? -1 : 1;
      uint64_t absDelta = std::abs(delta);
      if (idx == 0) {
        colorQuantParam = (k == 0)
          ? colorQp.attrQuantForLuma + transform_shift_QP + aps.QpOffsetDC
          : ((k == 1) ? colorQp.attrQuantForChromaCb + transform_shift_QP + aps.chromaQpOffsetDC
                      : colorQp.attrQuantForChromaCr + transform_shift_QP + aps.chromaQpOffsetDC);
        if (colorQPAdjustFlag)
          colorQuantParam = max(transform_shift_QP, (int)colorQuantParam - resCodeQPOffset);
      } else
        colorQuantParam = (k == 0)
          ? colorQp.attrQuantForLuma + transform_shift_QP + aps.QpOffsetAC
          : ((k == 1) ? colorQp.attrQuantForChromaCb + transform_shift_QP + aps.chromaQpOffsetAC
                      : colorQp.attrQuantForChromaCr + transform_shift_QP + aps.chromaQpOffsetAC);

      //inverse quantitzation
      uint64_t inverseResidualQuant = InverseQuantizeResidual(absDelta, colorQuantParam);
      int64_t residual = inverseResidualQuant * sign;
      transformBuf[k][idx] = residual;
    }
  }
  invTransform(transformBuf, count, 3);
  int setlength = neighborSet.size();
  for (int i = 0; i < count; ++i) {
    int pointIndex = pointCloudHilbert[transformPointIdx[i]].index;
    PC_COL& color = outputPointCloud.getColor(pointIndex, multil_ID);
    for (int k = 0; k < 3; ++k) {
      transformBuf[k][i] = (transformBuf[k][i] + add) >> transform_shift * 2;
      transformBuf[k][i] += transformPredBuf[k][i];
      transformBuf[k][i] = TComClip((Int64)ClipMin, (Int64)ClipMax, transformBuf[k][i]);
      color[k] = transformBuf[k][i];
    }
    if (transformPointIdx[0] <= setlength) {
      neighborSet[transformPointIdx[i] % setlength].pos = outputPointCloud[pointIndex];
      neighborSet[transformPointIdx[i] % setlength].color = color;
    } else {
      neighborSet[i].pos = outputPointCloud[pointIndex];
      neighborSet[i].color = color;
    }
  }
}

void TDecAttribute::reflectanceReconstructionTrans(
  std::vector<pointCodeWithIndex>& pointCloudHilbert, std::vector<int>& transformPointIdx,
  int64_t transformBuf[1][8], int64_t transformPredBuf[1][8],
  std::vector<reflNeighborSet>& neighborSet, PC_REFL& lastref) {
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (m_hls->aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (m_hls->aps.reflOutputDepth)) - 1;
  }
  //Int16 RefMax = -ClipMax;
  //Int16 RefMin = ClipMax;
  Int transQuantParam;
  int count = transformPointIdx.size();
  int transform_shift = 9;
  Int64 add = 1 << (transform_shift * 2 - 1);
  //inverse quantitzation
  int num = std::max(1, count);
  for (int idx = 0; idx < num; ++idx) {
    if (idx == 0)
      transQuantParam =
        m_hls->sps.reflQuantParam + m_hls->abh.reflQPoffset + 72 + m_hls->aps.QpOffsetDC;
    else
      transQuantParam =
        m_hls->sps.reflQuantParam + m_hls->abh.reflQPoffset + 72 + m_hls->aps.QpOffsetAC;
    int64_t delta = transformBuf[0][idx];
    int sign = (delta < 0) ? -1 : 1;
    uint64_t absDelta = std::abs(delta);
    //inverse quantitzation
    uint64_t inverseResidualQuant = InverseQuantizeResidual(absDelta, transQuantParam);
    int64_t residual = inverseResidualQuant * sign;
    transformBuf[0][idx] = residual;
  }
  invTransform(transformBuf, count, 1);
  int setlength = neighborSet.size();
  for (int idx = 0; idx < count; ++idx) {
    auto pointIndex = pointCloudHilbert[transformPointIdx[idx]].index;
    PC_REFL& curReflectance = outputPointCloud.getReflectance(pointIndex, multil_ID);
    transformBuf[0][idx] = (transformBuf[0][idx] + add) >> transform_shift * 2;
    transformBuf[0][idx] += transformPredBuf[0][idx];
    transformBuf[0][idx] = TComClip((Int64)ClipMin, (Int64)ClipMax, transformBuf[0][idx]);
    curReflectance = transformBuf[0][idx];
    if (transformPointIdx[0] <= setlength) {
      neighborSet[transformPointIdx[idx] % setlength].pos = outputPointCloud[pointIndex];
      neighborSet[transformPointIdx[idx] % setlength].refl = curReflectance;
    } else {
      neighborSet[idx].pos = outputPointCloud[pointIndex];
      neighborSet[idx].refl = curReflectance;
    }
  }
  lastref = transformBuf[0][count - 1];
}

void TDecAttribute::setCoeffIndex(int& groupCount, const vector<int>& numofGroupCount,
                                  const vector<int>& length, int& dcIndex, int& acIndex, 
	                              int& numofCoeff) {
  if (groupCount < numofGroupCount.size() - 1) {
    int beginIndex = numofGroupCount[groupCount];
    groupCount++;
    int endIndex = numofGroupCount[groupCount];
    numofCoeff = accumulate(length.begin() + beginIndex, length.begin() + endIndex, 0);
    if (groupCount % 2) {
      dcIndex = numofCoeff - (numofGroupCount[groupCount] - numofGroupCount[groupCount - 1]);
      acIndex = 0;
    } else {
      dcIndex = 0;
      acIndex = numofGroupCount[groupCount] - numofGroupCount[groupCount - 1];
    }
  } else {
    numofCoeff = 0;
  }
}

void TDecAttribute::runlengthDecodeMemControl(int& pointCount, int* Coefficients, int& run_length,
                                              int lengthControl, bool& isLengthControl,
                                              const bool isColor) {
  const AttributeParameterSet& aps = m_hls->aps;
  if (isColor) {
    auto colorGolombNum = aps.colorGolombNum;
    const bool colorGolomb = colorGolombNum <= 2 ? true : false;
    bool os = aps.orderSwitch;
    for (int n = 0; n < pointCount; ++n) {
      V3<int64_t> values;
      if ((aps.coeffLengthControl > 0) && (run_length == lengthControl)) {
        isLengthControl = true;
        --run_length;
      }
      if (run_length > 0) {
        values = 0;
        --run_length;
      } else {
        if (isLengthControl) {
          values = 0;
          run_length = m_decBac->parseRunlength();
          isLengthControl = false;
        } else {
          if (!os) {
            parseColorResidualCorrelationCode(values, false, colorGolombNum);
          } else {
            parseColorResidualCorrelationCodeOS(values, false, colorGolombNum);
          }
          run_length = m_decBac->parseRunlength();
        }
      }
      for (int d = 0; d < 3; ++d)
        Coefficients[3 * n + d] = values[d];
    }
  } else {
    auto refGolombNum = aps.refGolombNum;
    const bool refGolomb = refGolombNum == 1 ? true : false;
    for (int n = 0; n < pointCount; ++n) {
      int64_t values;
      if ((aps.coeffLengthControl > 0) && (run_length == lengthControl)) {
        isLengthControl = true;
        --run_length;
      }
      if (run_length > 0) {
        values = 0;
        --run_length;
      } else {
        if (isLengthControl) {
          values = 0;
          if (m_decBacDual != NULL)
            run_length = m_decBacDual->parseRunlength();
          else
            run_length = m_decBac->parseRunlength();
          isLengthControl = false;
        } else {
          if (m_decBacDual != NULL) {
            values = m_decBacDual->parseAttr(false, 3, 0, false, false, refGolombNum);
            run_length = m_decBacDual->parseRunlength();
          } else {
            values = m_decBac->parseAttr(false, 3, 0, false, false, refGolombNum);
            run_length = m_decBac->parseRunlength();
          }
        }
      }
      Coefficients[n] = values;
    }
  }
}

//----------------------------------------------------------------------------------

void TDecAttribute::colorInverseWaveletTransform() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameHead = m_hls->frameHead;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const bool isGolomb = aps.colorGolombNum <= 2 ? true : false;
  bool isLengthControl = false;
  bool os = aps.orderSwitch;
  cout << "colorInverseWaveletTransform" << endl;
  int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReordermode, pointCloudCode, voxelCount, 1);

  // Allocate arrays.
  int attribCount = 3;
  FXPoint* attributesBuf = new FXPoint[voxelCount * attribCount];
  int* integerizedAttributesBuf = new int[voxelCount * attribCount];
  int* positionBuf = new int[voxelCount * 3];
  for (int n = 0; n < voxelCount; n++) {
    const auto& point = outputPointCloud[pointCloudCode[n].index];
    positionBuf[3 * n] = point[0];
    positionBuf[3 * n + 1] = point[1];
    positionBuf[3 * n + 2] = point[2];
  }
  std::fill_n(attributesBuf, voxelCount * attribCount, FXPoint(0));
  bool resLayer = aps.transResLayer;
  int resLayerQuantParam = sps.colorQuantParam;
  int coeffQuantParam = sps.colorQuantParam;
  if (resLayer)
    coeffQuantParam += aps.attrTransformQpDelta;
  V3<int> preColor;
  V3<int64_t> Values;
  PC_COL recColor;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;
  UInt64 meanBB =
    (sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2]) / 3;
  UInt disThInit = meanBB * meanBB;
  if (aps.colorInitPredTransRatio >= 0)
    disThInit = disThInit << aps.colorInitPredTransRatio;
  else if (aps.colorInitPredTransRatio < 0)
    disThInit = disThInit >> std::abs(aps.colorInitPredTransRatio);
  disThInit = std::max({(UInt)1, disThInit / frameHead.geomNumPoints});
  int segmentLen = (aps.transformSegmentSize == 0) ? voxelCount : aps.transformSegmentSize;
  int numSegments = (voxelCount + segmentLen - 1) / segmentLen;
  for (int segIndex = 0; segIndex < numSegments; segIndex++) {
    int segmentStartPosition = segIndex * segmentLen;
    FXPoint* attributes = attributesBuf + segmentStartPosition * attribCount;
    int* integerizedAttributes = integerizedAttributesBuf + segmentStartPosition * attribCount;
    int* segmentPosition = positionBuf + segmentStartPosition * 3;
    int segmentVoxelCount = segmentLen;
    if (segIndex == (numSegments - 1)) {
      segmentVoxelCount = voxelCount - (numSegments - 1) * segmentLen;
    }
    int run_length = m_decBac->parseRunlength();
    for (int n = 0; n < segmentVoxelCount; ++n) {
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        isLengthControl = true;
        --run_length;
      }
      if (run_length > 0) {
        Values = 0;
        --run_length;
      } else {
        if (isLengthControl) {
          Values = 0;
          run_length = m_decBac->parseRunlength();
          isLengthControl = false;
        } else {
          if (!os) {
            parseColorResidualCorrelationCode(Values, false, aps.colorGolombNum);
          } else {
            parseColorResidualCorrelationCodeOS(Values, false, aps.colorGolombNum);
          }
          run_length = m_decBac->parseRunlength();
        }
      }
      for (int kk = 0; kk < 3; ++kk) {
        int sign = (Values[kk] < 0) ? -1 : 1;
        uint64_t absDelta = std::abs(Values[kk]);
        integerizedAttributes[n * attribCount + kk] =
          sign * InverseQuantizeResidual(absDelta, coeffQuantParam);
      }
    }

    WaveletCoreInverseTransform(attributes, 3, segmentVoxelCount, integerizedAttributes, sps, aps,abh,
                                segmentPosition, disThInit);
    if (resLayer) {
      cout << "ResLayer Needed!" << endl;
      run_length = m_decBac->parseRunlength();
      isLengthControl = false;
      for (int n = 0; n < segmentVoxelCount; ++n) {
        V3<int64_t> Values;
        if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
          isLengthControl = true;
          --run_length;
        }
        if (run_length > 0) {
          Values = 0;
          --run_length;
        } else {
          if (isLengthControl) {
            Values = 0;
            run_length = m_decBac->parseRunlength();
            isLengthControl = false;
          } else {
            if (!os) {
              parseColorResidualCorrelationCode(Values, false, aps.colorGolombNum);
            } else {
              parseColorResidualCorrelationCodeOS(Values, false, aps.colorGolombNum);
            }
            run_length = m_decBac->parseRunlength();
          }
        }
        for (int kk = 0; kk < 3; ++kk) {
          int sign = (Values[kk] < 0) ? -1 : 1;
          uint64_t absDelta = std::abs(Values[kk]);
          integerizedAttributes[n * attribCount + kk] =
            sign * InverseQuantizeResidual(absDelta, resLayerQuantParam);
        }
      }
    }
    for (int n = 0; n < segmentVoxelCount; n++) {
      for (int kk = 0; kk < attribCount; kk++) {
        preColor[kk] = attributes[n * attribCount + kk].round();
        if (resLayer)
          preColor[kk] += integerizedAttributes[n * attribCount + kk];
      }
      for (int kk = 0; kk < attribCount; kk++)
        recColor[kk] = (Int16)TComClip((Int64)ClipMin, (Int64)ClipMax, (Int64)preColor[kk]);
      outputPointCloud.setColor(pointCloudCode[n + segmentStartPosition].index, recColor,
                                multil_ID);
    }
  }
  // De-allocate arrays.
  delete[] attributesBuf;
  delete[] integerizedAttributesBuf;
  delete[] positionBuf;
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::colorInverseWaveletTransformFromReflectance() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameHead = m_hls->frameHead;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const bool isGolomb = aps.colorGolombNum <= 2 ? true : false;
  bool isLengthControl = false;
  bool os = aps.orderSwitch;
  cout << "reflectanceInverseWaveletTransformFromColor" << endl;
  int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();

  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReordermode, pointCloudCode, voxelCount, 1);

  // Allocate arrays.
  int attribCount = 3;
  FXPoint* attributesBuf = new FXPoint[voxelCount * attribCount];
  int* integerizedAttributesBuf = new int[voxelCount * attribCount];
  int* positionBuf = new int[voxelCount * 3];

  // new
  int* reflectanceBuf = new int[voxelCount];
  bool isEnableCrossAttrTypePred = aps.crossAttrTypePred;
  uint64_t crossAttrTypeLambda = 0;
  int64_t crossAttrTypeCoef = 0;
  crossAttrTypeLambda = (-(sps.reflQuantParam + abh.reflQPoffset) * aps.crossAttrTypePredParam1 +
                         aps.crossAttrTypePredParam2);
  auto diffPos =
    sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2];
  uint64_t maxRefl = (1 << m_hls->aps.reflOutputDepth) - 1;
  crossAttrTypeCoef = round((diffPos << 10) / (double)maxRefl);
  crossAttrTypeCoef = int64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;

  for (int n = 0; n < voxelCount; n++) {
    const auto& point = outputPointCloud[pointCloudCode[n].index];
    positionBuf[3 * n] = point[0];
    positionBuf[3 * n + 1] = point[1];
    positionBuf[3 * n + 2] = point[2];
    const auto reflectance =
      outputPointCloud.getReflectance(pointCloudCode[n].index, multil_ID) * crossAttrTypeCoef;
    reflectanceBuf[n] = reflectance;
  }
  std::fill_n(attributesBuf, voxelCount * attribCount, FXPoint(0));
  bool resLayer = aps.transResLayer;
  int resLayerQuantParam = sps.colorQuantParam;
  int coeffQuantParam = sps.colorQuantParam;
  if (resLayer)
    coeffQuantParam += aps.attrTransformQpDelta;
  V3<int> preColor;
  V3<int64_t> Values;
  PC_COL recColor;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;

  UInt64 meanBB =
    (sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2]) / 3;
  UInt disThInit = meanBB * meanBB;
  if (aps.colorInitPredTransRatio >= 0)
    disThInit = disThInit << aps.colorInitPredTransRatio;
  else if (aps.colorInitPredTransRatio < 0)
    disThInit = disThInit >> std::abs(aps.colorInitPredTransRatio);
  disThInit = std::max({(UInt)1, disThInit / frameHead.geomNumPoints});

  int segmentLen = (aps.transformSegmentSize == 0) ? voxelCount : aps.transformSegmentSize;
  int numSegments = (voxelCount + segmentLen - 1) / segmentLen;
  for (int segIndex = 0; segIndex < numSegments; segIndex++) {
    int segmentStartPosition = segIndex * segmentLen;
    FXPoint* attributes = attributesBuf + segmentStartPosition * attribCount;
    int* reflectances = reflectanceBuf + segmentStartPosition;
    int* integerizedAttributes = integerizedAttributesBuf + segmentStartPosition * attribCount;
    int* segmentPosition = positionBuf + segmentStartPosition * 3;
    int segmentVoxelCount = segmentLen;
    if (segIndex == (numSegments - 1)) {
      segmentVoxelCount = voxelCount - (numSegments - 1) * segmentLen;
    }

    int run_length = m_decBac->parseRunlength();
    for (int n = 0; n < segmentVoxelCount; ++n) {
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        isLengthControl = true;
        --run_length;
      }
      if (run_length > 0) {
        Values = 0;
        --run_length;
      } else {
        if (isLengthControl) {
          Values = 0;
          run_length = m_decBac->parseRunlength();
          isLengthControl = false;
        } else {
          if (!os) {
            parseColorResidualCorrelationCode(Values, false, aps.colorGolombNum);
          } else {
            parseColorResidualCorrelationCodeOS(Values, false, aps.colorGolombNum);
          }
          run_length = m_decBac->parseRunlength();
        }
      }
      for (int kk = 0; kk < 3; ++kk) {
        int sign = (Values[kk] < 0) ? -1 : 1;
        uint64_t absDelta = std::abs(Values[kk]);
        integerizedAttributes[n * attribCount + kk] =
          sign * InverseQuantizeResidual(absDelta, coeffQuantParam);
      }
    }

    WaveletCoreInverseTransform(attributes, 3, segmentVoxelCount, integerizedAttributes, sps, aps,abh,
                                segmentPosition, disThInit, reflectances, 1);
    if (resLayer) {
      cout << "ResLayer Needed!" << endl;
      run_length = m_decBac->parseRunlength();
      isLengthControl = false;
      for (int n = 0; n < segmentVoxelCount; ++n) {
        V3<int64_t> Values;
        if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
          isLengthControl = true;
          --run_length;
        }
        if (run_length > 0) {
          Values = 0;
          --run_length;
        } else {
          if (isLengthControl) {
            Values = 0;
            run_length = m_decBac->parseRunlength();
            isLengthControl = false;
          } else {
            if (!os) {
              parseColorResidualCorrelationCode(Values, false, aps.colorGolombNum);
            } else {
              parseColorResidualCorrelationCodeOS(Values, false, aps.colorGolombNum);
            }
            run_length = m_decBac->parseRunlength();
          }
        }
        for (int kk = 0; kk < 3; ++kk) {
          int sign = (Values[kk] < 0) ? -1 : 1;
          uint64_t absDelta = std::abs(Values[kk]);
          integerizedAttributes[n * attribCount + kk] =
            sign * InverseQuantizeResidual(absDelta, resLayerQuantParam);
        }
      }
    }

    for (int n = 0; n < segmentVoxelCount; n++) {
      for (int kk = 0; kk < attribCount; kk++) {
        preColor[kk] = attributes[n * attribCount + kk].round();
        if (resLayer)
          preColor[kk] += integerizedAttributes[n * attribCount + kk];
      }
      for (int kk = 0; kk < attribCount; kk++)
        recColor[kk] = (Int16)TComClip((Int64)ClipMin, (Int64)ClipMax, (Int64)preColor[kk]);
      outputPointCloud.setColor(pointCloudCode[n + segmentStartPosition].index, recColor,
                                multil_ID);
    }
  }

  // De-allocate arrays.

  delete[] attributesBuf;
  delete[] integerizedAttributesBuf;
  delete[] positionBuf;
  delete[] reflectanceBuf;
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::reflectanceInverseWaveletTransformFromColor() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameHead = m_hls->frameHead;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  cout << "reflectanceInverseWaveletTransformFromColor" << endl;
  int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addReflectances();

  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.refReordermode, pointCloudCode, voxelCount,
          aps.axisBias);

  // Allocate arrays.
  FXPoint* attributesBuf = new FXPoint[voxelCount];
  int* integerizedAttributesBuf = new int[voxelCount];
  std::fill_n(attributesBuf, voxelCount, FXPoint(0));
  int* positionBuf = new int[voxelCount * 3];

  int* colorBuf = new int[voxelCount * 3];
  uint64_t crossAttrTypeLambda = 0;
  int64_t crossAttrTypeCoef = 0;
  vector<V3<int64_t>> colorWithCoef(0);
  crossAttrTypeLambda = (-(sps.reflQuantParam + abh.reflQPoffset) * aps.crossAttrTypePredParam1 +
                         aps.crossAttrTypePredParam2);
  UInt maxPos =
    sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2];
  uint64_t maxColor = (1 << m_hls->aps.colorOutputDepth) - 1;
  uint64_t maxColorSum = 3 * maxColor;
  crossAttrTypeCoef = round((maxPos << 10) / (double)maxColorSum);
  crossAttrTypeCoef = int64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;

  for (int n = 0; n < voxelCount; n++) {
    const auto color = outputPointCloud.getColor(pointCloudCode[n].index, multil_ID);
    colorBuf[n * 3] = (color[0] * crossAttrTypeCoef);
    colorBuf[n * 3 + 1] = (color[1] * crossAttrTypeCoef);
    colorBuf[n * 3 + 2] = (color[2] * crossAttrTypeCoef);
    const auto& point = outputPointCloud[pointCloudCode[n].index];
    positionBuf[3 * n] = point[0];
    positionBuf[3 * n + 1] = point[1];
    positionBuf[3 * n + 2] = point[2];
  }

  // Entropy decode
  bool resLayer = aps.transResLayer;
  int resLayerQuantParam = sps.reflQuantParam + abh.reflQPoffset;
  int coeffQuantParam = sps.reflQuantParam + abh.reflQPoffset;
  int golombnum = resLayer ? 3 : aps.refGolombNum;
  const bool isGolomb = golombnum <= 2 ? true : false;
  bool isLengthControl = false;
  if (resLayer)
    coeffQuantParam += aps.attrTransformQpDelta;
  int64_t r = 0;
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (m_hls->aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (m_hls->aps.reflOutputDepth)) - 1;
  }

  UInt64 meanBB =
    (sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2]) / 3;
  int disThInit = meanBB * meanBB / frameHead.geomNumPoints;
  if (aps.refInitPredTransRatio >= 0)
    disThInit = disThInit << aps.refInitPredTransRatio;
  else if (aps.refInitPredTransRatio < 0)
    disThInit = disThInit >> std::abs(aps.refInitPredTransRatio);
  disThInit = std::max({1, disThInit});

  int segmentLen = (aps.transformSegmentSize == 0) ? voxelCount : aps.transformSegmentSize;
  int numSegments = (voxelCount + segmentLen - 1) / segmentLen;
  for (int segIndex = 0; segIndex < numSegments; segIndex++) {
    int segmentStartPosition = segIndex * segmentLen;
    FXPoint* attributes = attributesBuf + segmentStartPosition;
    int* colors = colorBuf + segmentStartPosition * 3;
    int* integerizedAttributes = integerizedAttributesBuf + segmentStartPosition;
    int* segmentPosition = positionBuf + segmentStartPosition * 3;
    int segmentVoxelCount = segmentLen;
    if (segIndex == (numSegments - 1)) {
      segmentVoxelCount = voxelCount - (numSegments - 1) * segmentLen;
    }

    int run_length = m_decBac->parseRunlength();
    for (int n = 0; n < segmentVoxelCount; ++n) {
      int64_t delta = 0;
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        isLengthControl = true;
        --run_length;
      }
      if (run_length > 0) {
        delta = 0;
        --run_length;
      } else {
        if (isLengthControl) {
          delta = 0;
          run_length = m_decBac->parseRunlength();
          isLengthControl = false;
        } else {
          delta = m_decBac->parseAttr(false, 3, 0, false, false, golombnum);
          run_length = m_decBac->parseRunlength();
        }
      }
      int sign = (delta < 0) ? -1 : 1;
      uint64_t absDelta = std::abs(delta);
      integerizedAttributes[n] = sign * InverseQuantizeResidual(absDelta, coeffQuantParam);
    }

    WaveletCoreInverseTransform(attributes, 1, segmentVoxelCount, integerizedAttributes, sps, aps,abh,
                                segmentPosition, disThInit, colors, 3);
    if (resLayer) {
      cout << "ResLayer Needed!" << endl;
      run_length = m_decBac->parseRunlength();
      isLengthControl = false;
      for (int n = 0; n < segmentVoxelCount; ++n) {
        int64_t delta = 0;
        if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
          isLengthControl = true;
          --run_length;
        }
        if (run_length > 0) {
          delta = 0;
          --run_length;
        } else {
          if (isLengthControl) {
            delta = 0;
            run_length = m_decBac->parseRunlength();
            isLengthControl = false;
          } else {
            delta = m_decBac->parseAttr(false, 3, 0, false, false, golombnum);
            run_length = m_decBac->parseRunlength();
          }
        }
        int sign = (delta < 0) ? -1 : 1;
        uint64_t absDelta = std::abs(delta);
        integerizedAttributes[n] = sign * InverseQuantizeResidual(absDelta, resLayerQuantParam);
      }
    }

    for (int n = 0; n < segmentVoxelCount; n++) {
      r = attributes[n].round();
      if (resLayer)
        r = integerizedAttributes[n] + (int)r;
      const PC_REFL reflectance = (PC_REFL)TComClip((Int64)ClipMin, (Int64)ClipMax, r);
      outputPointCloud.setReflectance(pointCloudCode[n + segmentStartPosition].index, reflectance,
                                      multil_ID);
    }
  }

  // De-allocate arrays.
  delete[] attributesBuf;
  delete[] integerizedAttributesBuf;
  delete[] positionBuf;
  delete[] colorBuf;
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}
//------------------------------------------------------------------------------------------------------------------

void TDecAttribute::reflectanceInverseWaveletTransform() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameHead = m_hls->frameHead;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  cout << "reflectanceInverseWaveletTransform" << endl;
  int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addReflectances();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.refReordermode, pointCloudCode, voxelCount,
          aps.axisBias);
  // Allocate arrays.
  FXPoint* attributesBuf = new FXPoint[voxelCount];
  int64_t* integerizedAttributesBuf = new int64_t[voxelCount];
  std::fill_n(attributesBuf, voxelCount, FXPoint(0));
  int* positionBuf = new int[voxelCount * 3];
  for (int n = 0; n < voxelCount; n++) {
    const auto& point = outputPointCloud[pointCloudCode[n].index];
    positionBuf[3 * n] = point[0];
    positionBuf[3 * n + 1] = point[1];
    positionBuf[3 * n + 2] = point[2];
  }
  // Entropy decode
  bool resLayer = aps.transResLayer;
  int resLayerQuantParam = sps.reflQuantParam + abh.reflQPoffset;
  int coeffQuantParam = sps.reflQuantParam + abh.reflQPoffset;
  int golombnum = resLayer ? 3 : aps.refGolombNum;
  const bool isGolomb = golombnum <= 2 ? true : false;
  bool isLengthControl = false;
  if (resLayer)
    coeffQuantParam += aps.attrTransformQpDelta;
  int64_t r = 0;
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (m_hls->aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (m_hls->aps.reflOutputDepth)) - 1;
  }

  UInt64 meanBB =
    (sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2]) / 3;
  int disThInit = meanBB * meanBB / frameHead.geomNumPoints;
  if (aps.refInitPredTransRatio >= 0)
    disThInit = disThInit << aps.refInitPredTransRatio;
  else if (aps.refInitPredTransRatio < 0)
    disThInit = disThInit >> std::abs(aps.refInitPredTransRatio);
  disThInit = std::max({1, disThInit});
  int segmentLen = (aps.transformSegmentSize == 0) ? voxelCount : aps.transformSegmentSize;
  int numSegments = (voxelCount + segmentLen - 1) / segmentLen;
  for (int segIndex = 0; segIndex < numSegments; segIndex++) {
    int segmentStartPosition = segIndex * segmentLen;
    FXPoint* attributes = attributesBuf + segmentStartPosition;
    int64_t* integerizedAttributes = integerizedAttributesBuf + segmentStartPosition;
    int* segmentPosition = positionBuf + segmentStartPosition * 3;
    int segmentVoxelCount = segmentLen;
    if (segIndex == (numSegments - 1)) {
      segmentVoxelCount = voxelCount - (numSegments - 1) * segmentLen;
    }
    int run_length = m_decBac->parseRunlength();
    for (int n = 0; n < segmentVoxelCount; ++n) {
      int64_t delta = 0;
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        isLengthControl = true;
        --run_length;
      }
      if (run_length > 0) {
        delta = 0;
        --run_length;
      } else {
        if (isLengthControl) {
          delta = 0;
          run_length = m_decBac->parseRunlength();
          isLengthControl = false;
        } else {
          delta = m_decBac->parseAttr(false, 3, 0, false, false, golombnum);
          run_length = m_decBac->parseRunlength();
        }
      }
      int sign = (delta < 0) ? -1 : 1;
      uint64_t absDelta = std::abs(delta);
      integerizedAttributes[n] = sign * InverseQuantizeResidual(absDelta, coeffQuantParam);
    }

    WaveletCoreInverseTransform(attributes, 1, segmentVoxelCount, integerizedAttributes, sps, aps,abh,
                                segmentPosition, disThInit);
    if (resLayer) {
      cout << "ResLayer Needed!" << endl;
      run_length = m_decBac->parseRunlength();
      isLengthControl = false;
      for (int n = 0; n < segmentVoxelCount; ++n) {
        auto pointIndex = pointCloudCode[n + segmentStartPosition].index;
        int64_t delta = 0;
        if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
          isLengthControl = true;
          --run_length;
        }
        if (run_length > 0) {
          delta = 0;
          --run_length;
        } else {
          if (isLengthControl) {
            delta = 0;
            run_length = m_decBac->parseRunlength();
            isLengthControl = false;
          } else {
            delta = m_decBac->parseAttr(false, 3, 0, false, false, golombnum);
            run_length = m_decBac->parseRunlength();
          }
        }
        int sign = (delta < 0) ? -1 : 1;
        uint64_t absDelta = std::abs(delta);
        integerizedAttributes[n] = sign * InverseQuantizeResidual(absDelta, resLayerQuantParam);
      }
    }
    for (int n = 0; n < segmentVoxelCount; n++) {
      r = attributes[n].round();
      if (resLayer)
        r = integerizedAttributes[n] + (int)r;
      const PC_REFL reflectance = (PC_REFL)TComClip((Int64)ClipMin, (Int64)ClipMax, r);
      outputPointCloud.setReflectance(pointCloudCode[n + segmentStartPosition].index, reflectance,
                                      multil_ID);
    }
  }
  // De-allocate arrays.
  delete[] attributesBuf;
  delete[] integerizedAttributesBuf;
  delete[] positionBuf;
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::parseColorResidualCorrelationCodeHaar(V3<int64_t>& codedValue,
                                                          const bool reslayer,
                                                          const bool isDuplicatePoint,
                                                          const UInt& golombNum) {
  bool isColor = true;
  bool residualminusone = true;
  int flagy_r = m_decBac->parseAttrequalone0();
  if (flagy_r == 1) {
    codedValue[0] = 0;
  } else {
    codedValue[0] = m_decBac->parseAttrHaar(isColor, 0, reslayer, 1, 0 == 0 && isDuplicatePoint,
                                            residualminusone, golombNum);
  }
  if (codedValue[0] == 0) {
    int flagyu_rg = m_decBac->parseAttrequaltwo0();
    if (flagyu_rg == 1) {
      codedValue[1] = 0;
      codedValue[2] = m_decBac->parseAttrHaar(isColor, 2, reslayer, 0, 2 == 0 && isDuplicatePoint,
                                              residualminusone, golombNum);
      m_decBac->parseSign(codedValue[2]);
    } else {
      codedValue[1] = m_decBac->parseAttrHaar(isColor, 1, reslayer, 2, 1 == 0 && isDuplicatePoint,
                                              residualminusone, golombNum);
      codedValue[2] = m_decBac->parseAttrHaar(isColor, 2, reslayer, 0, 2 == 0 && isDuplicatePoint,
                                              !residualminusone, golombNum);
      m_decBac->parseSign(codedValue[1]);
      if (codedValue[2] != 0) {
        m_decBac->parseSign(codedValue[2]);
      }
    }
  } else {
    codedValue[1] = m_decBac->parseAttrHaar(isColor, 1, reslayer, 1, 1 == 0 && isDuplicatePoint,
                                            !residualminusone, golombNum);
    codedValue[2] = m_decBac->parseAttrHaar(isColor, 2, reslayer, 2, 2 == 0 && isDuplicatePoint,
                                            !residualminusone, golombNum);
    m_decBac->parseSign(codedValue[0]);
    if (codedValue[1] != 0) {
      m_decBac->parseSign(codedValue[1]);
    }
    if (codedValue[2] != 0) {
      m_decBac->parseSign(codedValue[2]);
    }
  }
}

void TDecAttribute::multiReflectanceInversePredictResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addReflectances();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.refReordermode, pointCloudCode, voxelCount,
          aps.axisBias);
  m_hls->sps.reflThreshold =
    (sps.reflQuantParam + abh.reflQPoffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  // get reflectance predictor
  PC_REFL predictorRefl[5];
  PC_REFL codedValue[5];
  PC_REFL reconValue[5];
  std::vector<multiReflNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int setCount = 0;
  int multil_ID_Group_Num = aps.multiAttriGroupNum[aps.multiAttriGroupID[multil_ID]];
  // runlength
  const bool isGolomb = aps.refGolombNum == 1 ? true : false;
  bool isLengthControl = false;
  int run_length = m_decBac->parseRunlength();
  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    auto pointIndex = pointCloudCode[curIndex].index;
    PC_REFL& currentValue = outputPointCloud.getReflectance(pointIndex, multil_ID);
    const PC_POS& curPosition = outputPointCloud[pointIndex];

    getMultiReflectancePredictorFarthest(curIndex, curIndex, outputPointCloud, sps, aps,
                                        pointCloudCode, setCount, neighborSet, predictorRefl,
                                        multil_ID_Group_Num);

    for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
      // Entropy decode
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        isLengthControl = true;
        --run_length;
      }
      if (run_length > 0) {
        codedValue[multi_id] = 0;
        --run_length;
      } else {
        if (isLengthControl) {
          codedValue[multi_id] = 0;
          run_length = m_decBac->parseRunlength();
          isLengthControl = false;
        } else {
          codedValue[multi_id] = m_decBac->parseAttr(false, 3, 0, false, false, aps.refGolombNum);
          run_length = m_decBac->parseRunlength();
        }
      }
    }
    
    multiReflectanceReconstruction(predictorRefl, codedValue, reconValue, multil_ID_Group_Num);
    // update neighborSet
    int farthestIdx = 0;
    if (curIndex < setlength) {
      farthestIdx = curIndex;
    }
    for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
      outputPointCloud.setReflectance(pointIndex, reconValue[multi_id], multi_id + multil_ID);
      neighborSet[farthestIdx].pos = outputPointCloud[pointIndex];
      neighborSet[farthestIdx].refl[multi_id] = reconValue[multi_id];
    }
  }
  m_decBac->parseSliceAttrEndCode();
  if (frame_Idx == frame_count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_decBac->parseSPSEndCode();
}

void TDecAttribute::multiReflectanceReconstruction(PC_REFL* predictor, PC_REFL* codedValue,
                                                   PC_REFL* reconValue, int multil_ID_Group_Num) {
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (m_hls->aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (m_hls->aps.reflOutputDepth)) - 1;
  }
  int residualPrevComponent = 0;
  bool ccp = false;
  if (false)  //m_hls->aps.crossComponentPred
    ccp = true;
  for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
    int64_t delta = codedValue[multi_id];
    int sign = (delta < 0) ? -1 : 1;
    uint64_t absDelta = std::abs(delta);
    //inverse quantitzation
    uint64_t inverseResidualQuant =
      InverseQuantizeResidual(absDelta, m_hls->sps.reflQuantParam + m_hls->abh.reflQPoffset);
    int64_t residual = inverseResidualQuant * sign;
    reconValue[multi_id] = TComClip((Int64)ClipMin, (Int64)ClipMax,
                                    residual + predictor[multi_id] + residualPrevComponent);
    if (ccp && multi_id < (multil_ID_Group_Num - 1)) {
      residualPrevComponent = (Int)reconValue[multi_id] - (Int)predictor[multi_id];
    }
  }
}
