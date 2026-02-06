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
                         int multiID) {
  m_pointCloudRecon = pointCloudRecon;
  m_hls = hls;
  m_decBac = decBac;
  m_decBacDual = NULL;
  multil_ID = multiID;
  //set adaptive expGolomb decoder parameter
  if (m_hls->aps.attributeDataPresentFlag[0]) {
    m_decBac->setColorGolombKandBound(m_hls->aps.golombGroupSizeLog2, m_hls->aps.colorGolombNum);
    if (m_decBacDual) {
      m_decBacDual->setColorGolombKandBound(m_hls->aps.golombGroupSizeLog2,
                                            m_hls->aps.colorGolombNum);
    }
  }
}

Void TDecAttribute::initDual(TComPointCloud* pointCloudRecon, HighLevelSyntax* hls,
                             TDecBacTop* decBac, TDecBacTop* decBacDual, int multiID) {
  m_pointCloudRecon = pointCloudRecon;
  m_hls = hls;
  m_decBac = decBac;
  if (m_hls->aps.attributeDataPresentFlag[0] && m_hls->aps.attributeDataPresentFlag[1]) {
    m_decBacDual = decBacDual;
  } else {
    m_decBacDual = NULL;
  }
  multil_ID = multiID;
  //set adaptive expGolomb decoder parameter
  if (m_hls->aps.attributeDataPresentFlag[0]) {
    m_decBac->setColorGolombKandBound(m_hls->aps.golombGroupSizeLog2, m_hls->aps.colorGolombNum);
    if (m_decBacDual) {
      m_decBacDual->setColorGolombKandBound(m_hls->aps.golombGroupSizeLog2,
                                            m_hls->aps.colorGolombNum);
    }
  }
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
  if (!aps.crossAttrTypePred) {
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

Void TDecAttribute::transformDecodeColorFromReflectance() {
  clock_t userTimeColorBegin = clock();
  colorInverseWaveletTransformFromReflectance();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
}

Void TDecAttribute::transformDecodeReflectanceFromColor() {
  clock_t userTimeReflectanceBegin = clock();
  reflectanceInverseWaveletTransformFromColor();
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
  reOrder(outputPointCloud.positions(), aps.reflReorderMode, pointCloudCode, voxelCount,
          aps.axisBias);

  // obtain reflectance predictor
  PC_REFL predictorRefl;
  int64_t codedValue;
  std::vector<neighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int setCount = 0;
  int prevIndex = -1;
  PC_REFL lastref = 0;

  // predict parameters
  predictOptParams predictOptParams;
  predictOptParams.colorQP = 0;
  predictOptParams.maxNumOfNeighbours = aps.maxNumOfNeighbours;
  predictOptParams.minNeighborDisFlag = 0;
  predictOptParams.minNeighborDis = 0;
  predictOptParams.reflThreshold =
    (aps.reflQuantParam + abh.QpOffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  predictOptParams.predFixedPointFracBit = aps.predFixedPointFracBit;
  predictOptParams.axisBias = aps.axisBias;
  predictOptParams.reflCrossAttrTypePred = false;
  predictOptParams.reflUpdateFlag = true;
  predictOptParams.reflectanceDistWeight = {0, 0, 0};
  predictOptParams.curReflWithCoef = 0;
  predictOptParams.curColorWithCoef = {0, 0, 0};

  ///<decode zero_cnt of run_length
  int run_length = m_decBac->parseRunlength();
  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    auto pointIndex = pointCloudCode[curIndex].index;
    PC_REFL& currentValue = outputPointCloud.getReflectance(pointIndex, multil_ID);
    const PC_POS& curPosition = outputPointCloud[pointIndex];
    Bool isDuplicatePoint =
      curIndex > 0 && curPosition == outputPointCloud[prevIndex] && m_hls->aps.eligibleDupPointPred;

    if (isDuplicatePoint) {
      predictorRefl = lastref;
    } else {
      getReflPredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                               predictOptParams, predictorRefl);
    }
    isDuplicatePoint &= multil_ID == 0;
    // Entropy decode
    if (run_length > 0) {
      codedValue = 0;
      --run_length;
    } else {
      codedValue = m_decBac->parseAttr(false, 3, 0, isDuplicatePoint, false, aps.reflGolombNum);
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
}

void TDecAttribute::attributeInversePredictResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const FrameHeader& frameheader = m_hls->frameheader;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();
  outputPointCloud.addReflectances();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReorderMode, pointCloudCode, voxelCount,
          aps.axisBias);
  //Qp compute
  quantizedQP colorQp;
  colorQp.attrQuantForLuma = aps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

  //dist weight calculation patameter
  uint64_t boundingSize =
    ((uint64_t)frameheader.geomBoundingBoxSize[0] * (uint64_t)frameheader.geomBoundingBoxSize[1] +
     (uint64_t)frameheader.geomBoundingBoxSize[0] * (uint64_t)frameheader.geomBoundingBoxSize[2] +
     (uint64_t)frameheader.geomBoundingBoxSize[1] * (uint64_t)frameheader.geomBoundingBoxSize[2])
    << 1;
  uint64_t reflectanceDistCoef =
    (boundingSize / frameheader.geomNumPoints > 0) ? boundingSize / frameheader.geomNumPoints : 1;
  UInt log2_reflectanceDistCoef = ceilLog2(reflectanceDistCoef);

  V3<int64_t> reflectanceRes = {0, 0, 0};
  V3<int64_t> reflResNum = {0, 0, 0};
  V3<int> reflectanceDistWeight = {0, 0, 0};

  PC_POS prePosition = outputPointCloud[pointCloudCode[0].index];
  PC_REFL preReflectance = 0;
  int distWeightGroupSize = 1 << aps.predDistWeightGroupSizeLog2;

  // predict parameters
  predictOptParams predictOptParams;
  predictOptParams.colorQP = aps.colorQuantParam;
  predictOptParams.maxNumOfNeighbours = aps.maxNumOfNeighbours;
  predictOptParams.minNeighborDisFlag = aps.colorQPAdjustFlag || aps.chromaDeadzoneFlag;
  predictOptParams.minNeighborDis = 0;
  predictOptParams.reflThreshold =
    (aps.reflQuantParam + abh.QpOffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  predictOptParams.predFixedPointFracBit = aps.predFixedPointFracBit;
  predictOptParams.axisBias = aps.axisBias;
  predictOptParams.reflectanceDistWeight = {0, 0, 0};
  predictOptParams.curReflWithCoef = 0;
  predictOptParams.curColorWithCoef = {0, 0, 0};

  // cross attribute parameter
  uint64_t crossAttrTypeLambda = 0;
  int64_t crossAttrTypeCoef = 0;
  if (aps.crossAttrTypePred && !aps.attrEncodeOrder) {
    crossAttrTypeLambda =
      (-aps.colorQuantParam * aps.crossAttrTypePredParam1 + aps.crossAttrTypePredParam2);
    UInt maxPos = frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
      frameheader.geomBoundingBoxSize[2];
    uint64_t maxColor = (1 << m_hls->aps.colorOutputDepth) - 1;
    uint64_t maxColorSum = 3 * maxColor;
    crossAttrTypeCoef = round((maxPos << 10) / (double)maxColorSum);
    crossAttrTypeCoef = uint64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;

    predictOptParams.colorCrossAttrTypePred = false;
    predictOptParams.colorUpdateFlag = false;
    predictOptParams.reflCrossAttrTypePred = true;
    predictOptParams.reflUpdateFlag = true;
  } else if (aps.crossAttrTypePred && aps.attrEncodeOrder) {
    crossAttrTypeLambda = (-(aps.reflQuantParam + abh.QpOffset) * aps.crossAttrTypePredParam1 +
                           aps.crossAttrTypePredParam2);
    auto diffPos = frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
      frameheader.geomBoundingBoxSize[2];
    uint64_t maxRefl = (1 << m_hls->aps.reflOutputDepth) - 1;
    crossAttrTypeCoef = round((diffPos << 10) / (double)maxRefl);
    crossAttrTypeCoef = uint64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;

    predictOptParams.colorCrossAttrTypePred = true;
    predictOptParams.colorUpdateFlag = true;
    predictOptParams.reflCrossAttrTypePred = false;
    predictOptParams.reflUpdateFlag = false;
  }
  //obtain predictors
  int setlength = aps.maxNumOfNeighbours;
  int setCount = 0;
  vector<neighborSet> neighborSet;
  neighborSet.resize(setlength);
  pair<PC_COL, PC_REFL> predictorAttr;
  PC_COL predictorColor;
  PC_REFL predictorRefl;
  V3<int64_t> codedValue;
  int64_t codedValueRefl;
  int prevIndex = -1;

  // runlength
  int run_length_col = 0;
  int run_length_refl = 0;
  const bool refisGolomb = aps.reflGolombNum == 1 ? true : false;
  const bool colorisGolomb = aps.colorGolombNum == 1 ? true : false;
  bool os = aps.orderSwitch;
  bool colorLengthControl = false;
  bool reflLengthControl = false;

  // disable cross-attribute-type-prediction
  if (!aps.crossAttrTypePred) {
    neighborSet.resize(setlength);
    run_length_col = m_decBac->parseRunlength();
    run_length_refl = m_decBacDual->parseRunlength();
    for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      PC_COL& curColor = outputPointCloud.getColor(pointIndex, multil_ID);
      PC_REFL& curRefl = outputPointCloud.getReflectance(pointIndex, multil_ID);
      Bool isDuplicatePoint = curIndex > 0 && curPosition == outputPointCloud[prevIndex] &&
        m_hls->aps.eligibleDupPointPred;
      if (isDuplicatePoint) {
        predictorColor = outputPointCloud.getColor(prevIndex, multil_ID);
        predictorRefl = outputPointCloud.getReflectance(prevIndex, multil_ID);
      } else {
        getAttributePredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                                      predictOptParams, predictorAttr);
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
          parseColorResidualCorrelationCode(codedValue, os, isDuplicatePoint, aps.colorGolombNum);
          if (os)
            std::swap(codedValue[0], codedValue[1]);
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
            m_decBacDual->parseRefl(countOfZeros, isDuplicatePoint, false, aps.reflGolombNum);
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
    run_length_col = m_decBac->parseRunlength();
    run_length_refl = m_decBacDual->parseRunlength();
    for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      PC_COL& curColor = outputPointCloud.getColor(pointIndex, multil_ID);
      Bool isDuplicatePoint = curIndex > 0 && curPosition == outputPointCloud[prevIndex] &&
        m_hls->aps.eligibleDupPointPred;
      if (isDuplicatePoint) {
        predictorColor = outputPointCloud.getColor(prevIndex, multil_ID);
      } else {
        getColorPredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                                  predictOptParams, predictorColor);
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
          parseColorResidualCorrelationCode(codedValue, os, isDuplicatePoint, aps.colorGolombNum);
          if (os)
            std::swap(codedValue[0], codedValue[1]);
          run_length_col = m_decBac->parseRunlength();
        }
      }
      colorReconstruction(predictorColor, codedValue, curColor, colorQp);
      // calculate the color with coefficient, which is used in cross-attribute-type-prediction
      predictOptParams.curColorWithCoef[0] = curColor[0] * crossAttrTypeCoef;
      predictOptParams.curColorWithCoef[1] = curColor[1] * crossAttrTypeCoef;
      predictOptParams.curColorWithCoef[2] = curColor[2] * crossAttrTypeCoef;
      PC_REFL& curRefl = outputPointCloud.getReflectance(pointIndex, multil_ID);
      if (isDuplicatePoint) {
        predictorRefl = outputPointCloud.getReflectance(prevIndex, multil_ID);
      } else {
        // get the better refl prediction through the geometric distance and the color distance
        getReflPredictorFarthestDual(curIndex, curIndex, curPosition, setCount, neighborSet,
                                     predictOptParams, predictorRefl);
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
            m_decBacDual->parseRefl(countOfZeros, isDuplicatePoint, false, aps.reflGolombNum);
          run_length_refl = m_decBacDual->parseRunlength();
        }
      }
      reflectanceReconstruction(predictorRefl, codedValueRefl, curRefl);
      if (curIndex != 0)
        calculateReflTrend(curPosition, prePosition, curRefl, preReflectance,
                           log2_reflectanceDistCoef, reflectanceRes, reflResNum,
                           predictOptParams.reflectanceDistWeight, distWeightGroupSize);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = curPosition;
      neighborSet[farthestIdx].refl = curRefl;
      neighborSet[farthestIdx].color = curColor;
      neighborSet[farthestIdx].colorWithCoef = predictOptParams.curColorWithCoef;
      prevIndex = pointIndex;
      prePosition = curPosition;
      preReflectance = curRefl;
    }
    // enable cross-attribute-type-prediction , encode the reflectance and then encode the color
  } else if (aps.attrEncodeOrder) {
    run_length_refl = m_decBacDual->parseRunlength();
    run_length_col = m_decBac->parseRunlength();
    for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      Bool isDuplicatePoint = curIndex > 0 && curPosition == outputPointCloud[prevIndex] &&
        m_hls->aps.eligibleDupPointPred;
      PC_REFL& curRefl = outputPointCloud.getReflectance(pointIndex, multil_ID);
      if (isDuplicatePoint) {
        predictorRefl = outputPointCloud.getReflectance(prevIndex, multil_ID);
      } else {
        getReflPredictorFarthestDual(curIndex, curIndex, curPosition, setCount, neighborSet,
                                     predictOptParams, predictorRefl);
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
            m_decBacDual->parseRefl(countOfZeros, isDuplicatePoint, false, aps.reflGolombNum);
          run_length_refl = m_decBacDual->parseRunlength();
        }
      }
      reflectanceReconstruction(predictorRefl, codedValueRefl, curRefl);
      // calculate the reflectance with coefficient, which is used in cross-attribute-type-prediction
      predictOptParams.curReflWithCoef = curRefl * crossAttrTypeCoef;

      if (curIndex != 0)
        calculateReflTrend(curPosition, prePosition, curRefl, preReflectance,
                           log2_reflectanceDistCoef, reflectanceRes, reflResNum,
                           predictOptParams.reflectanceDistWeight, distWeightGroupSize);

      PC_COL& curColor = outputPointCloud.getColor(pointIndex, multil_ID);
      V3<int64_t> codedValue;
      if (isDuplicatePoint) {
        predictorColor = outputPointCloud.getColor(prevIndex, multil_ID);
      } else {
        // get the better color prediction through the geometric distance and the reflectance distance
        getColorPredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                                  predictOptParams, predictorColor);
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
          parseColorResidualCorrelationCode(codedValue, os, isDuplicatePoint, aps.colorGolombNum);
          if (os)
            std::swap(codedValue[0], codedValue[1]);
          run_length_col = m_decBac->parseRunlength();
        }
      }
      colorReconstruction(predictorColor, codedValue, curColor, colorQp);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = curPosition;
      neighborSet[farthestIdx].color = curColor;
      neighborSet[farthestIdx].refl = curRefl;
      neighborSet[farthestIdx].reflWithCoef = predictOptParams.curReflWithCoef;
      prevIndex = pointIndex;
      prePosition = curPosition;
      preReflectance = curRefl;
    }
  }
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
  reOrder(outputPointCloud.positions(), aps.reflReorderMode, pointCloudCode, voxelCount,
          aps.axisBias);

  // get reflectance predictor
  PC_REFL predictorRefl;
  int64_t codedValue;
  std::vector<neighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int setCount = 0;
  int prevIndex = -1;
  PC_REFL lastref = 0;
  // runlength
  bool isLengthControl = false;
  int run_length = m_decBac->parseRunlength();

  // predict parameters
  predictOptParams predictOptParams;
  predictOptParams.colorQP = 0;
  predictOptParams.maxNumOfNeighbours = aps.maxNumOfNeighbours;
  predictOptParams.minNeighborDisFlag = 0;
  predictOptParams.minNeighborDis = 0;
  predictOptParams.reflThreshold =
    (aps.reflQuantParam + abh.QpOffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  predictOptParams.predFixedPointFracBit = aps.predFixedPointFracBit;
  predictOptParams.axisBias = aps.axisBias;
  predictOptParams.reflCrossAttrTypePred = false;
  predictOptParams.reflUpdateFlag = true;
  predictOptParams.reflectanceDistWeight = {0, 0, 0};
  predictOptParams.curReflWithCoef = 0;
  predictOptParams.curColorWithCoef = {0, 0, 0};

  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    auto pointIndex = pointCloudCode[curIndex].index;
    PC_REFL& currentValue = outputPointCloud.getReflectance(pointIndex, multil_ID);
    const PC_POS& curPosition = outputPointCloud[pointIndex];
    Bool isDuplicatePoint =
      curIndex > 0 && curPosition == outputPointCloud[prevIndex] && m_hls->aps.eligibleDupPointPred;

    if (isDuplicatePoint) {
      predictorRefl = lastref;
    } else {
      getReflPredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                               predictOptParams, predictorRefl);
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
        codedValue = m_decBac->parseAttr(false, 3, 0, isDuplicatePoint, false, aps.reflGolombNum);
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
}

void TDecAttribute::colorInversePredictResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReorderMode, pointCloudCode, voxelCount, 1);
  //Qp compute
  quantizedQP colorQp;
  colorQp.attrQuantForLuma = aps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

  // obtain color predictor
  PC_COL predictorColor;
  V3<int64_t> codedValue;
  std::vector<neighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  PC_COL lastColor;
  int prevIndex = -1;

  int setCount = 0;
  // runlength
  int run_length = 0;
  bool isLengthControl = false;
  run_length = m_decBac->parseRunlength();
  bool os = aps.orderSwitch;

  // predict parameters
  predictOptParams predictOptParams;
  predictOptParams.colorQP = aps.colorQuantParam;
  predictOptParams.maxNumOfNeighbours = aps.maxNumOfNeighbours;
  predictOptParams.minNeighborDisFlag = aps.colorQPAdjustFlag || aps.chromaDeadzoneFlag;
  predictOptParams.minNeighborDis = 0;
  predictOptParams.reflThreshold = 0;
  predictOptParams.predFixedPointFracBit = aps.predFixedPointFracBit;
  predictOptParams.axisBias = 0;
  predictOptParams.colorCrossAttrTypePred = false;
  predictOptParams.colorUpdateFlag = true;
  predictOptParams.reflectanceDistWeight = {0, 0, 0};
  predictOptParams.curReflWithCoef = 0;
  predictOptParams.curColorWithCoef = {0, 0, 0};

  for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
    int pointIndex = pointCloudCode[curIndex].index;
    const PC_POS& curPosition = outputPointCloud[pointIndex];
    PC_COL& curColor = outputPointCloud.getColor(pointIndex, multil_ID);
    Bool isDuplicatePoint =
      curIndex > 0 && curPosition == outputPointCloud[prevIndex] && m_hls->aps.eligibleDupPointPred;
    if (isDuplicatePoint) {
      predictorColor = lastColor;
    } else {
      getColorPredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                                predictOptParams, predictorColor);
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
        parseColorResidualCorrelationCode(codedValue, os, isDuplicatePoint, aps.colorGolombNum);
        if (os)
          std::swap(codedValue[0], codedValue[1]);
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
}

void TDecAttribute::colorInversePredictAndTransformMemControl() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const FrameHeader& frameheader = m_hls->frameheader;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();
  //Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReorderMode, pointCloudCode, voxelCount, 1);
  //Qp compute
  quantizedQP colorQp;
  colorQp.attrQuantForLuma = aps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

  // grouping
  int maxNodeSizeLog2 = std::max(
    {1U, m_hls->gbh.nodeSizeLog2[0], m_hls->gbh.nodeSizeLog2[1], m_hls->gbh.nodeSizeLog2[2]});
  int groupShiftBits = std::max(3, 3 * (maxNodeSizeLog2 - ((ceilLog2(voxelCount / 4) + 1) >> 1)));

  // adjust color QP per point tool
  bool colorQPAdjustSliceFlag = aps.colorQPAdjustFlag;
  bool colorQPAdjustFlag = false;
  int scalar = 1;
  if (colorQPAdjustSliceFlag)
    scalar = abh.colorQPAdjustScalar;
  int colorQPAdjustDis = std::max(4, (1 << (groupShiftBits / 3 + 4)) / scalar);
  vector<int> length;
  vector<int> numofGroupCount;
  getLength(pointCloudCode, length, numofGroupCount, aps.maxNumofCoeff, groupShiftBits,
            aps.colorMaxTransNum);
  int subGroupCount = 0;
  int subgroupIndex = 0;
  // obtain color predictor
  PC_COL predictorColor;
  std::vector<neighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  // transform and entropy coding parameter
  std::vector<int> transformPointIdx;
  int64_t transformBuf[3][8] = {};
  int64_t transformPredBuf[3][8] = {};
  const int maxNumofCoeff = max(aps.maxNumofCoeff, aps.colorMaxTransNum);
  int* CoeffGroup = new int[maxNumofCoeff * 3]();

  int run_length = m_decBac->parseRunlength();
  bool isLengthControl = false;
  int dcIndex = 0;
  int acIndex = numofGroupCount[0];
  int numofCoeff = accumulate(length.begin(), length.begin() + numofGroupCount[0], 0);
  int lengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;
  runlengthDecodeMemControl(numofCoeff, CoeffGroup, run_length, lengthControl, isLengthControl,
                            true);
  int groupCount = 0;

  // predict parameters
  predictOptParams predictOptParams;
  predictOptParams.colorQP = aps.colorQuantParam;
  predictOptParams.maxNumOfNeighbours = aps.maxNumOfNeighbours;
  predictOptParams.minNeighborDisFlag = aps.colorQPAdjustFlag || aps.chromaDeadzoneFlag;
  predictOptParams.minNeighborDis = 0;
  predictOptParams.reflThreshold = 0;
  predictOptParams.predFixedPointFracBit = aps.predFixedPointFracBit;
  predictOptParams.axisBias = 0;
  predictOptParams.colorCrossAttrTypePred = false;
  predictOptParams.colorUpdateFlag = true;
  predictOptParams.reflectanceDistWeight = {0, 0, 0};
  predictOptParams.curReflWithCoef = 0;
  predictOptParams.curColorWithCoef = {0, 0, 0};

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
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      transformPointIdx.push_back(curIndex);
      getColorPredictorFarthest(curIndex, transformPointIdx[0], curPosition, subGroupCount,
                                neighborSet, predictOptParams, predictorColor);
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
      colorQPAdjustFlag = predictOptParams.minNeighborDis > colorQPAdjustDis;
    colorReconstructionTrans(pointCloudCode, transformPointIdx, transformBuf, transformPredBuf,
                             colorQp, subGroupCount, neighborSet, colorQPAdjustFlag);
    transformPointIdx.erase(transformPointIdx.begin(), transformPointIdx.end());
    subgroupIndex++;
  }
  delete[] CoeffGroup;
}

void TDecAttribute::ReflectanceInversePredictAndTransformMemControl() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const FrameHeader& frameheader = m_hls->frameheader;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addReflectances();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.reflReorderMode, pointCloudCode, voxelCount,
          aps.axisBias);

  // grouping
  int MaxBits =
    m_hls->gbh.nodeSizeLog2[0] + m_hls->gbh.nodeSizeLog2[1] + m_hls->gbh.nodeSizeLog2[2];
  MaxBits = TComClip(0, 32, MaxBits);
  int MinBits = ceilLog2(voxelCount);
  int shift = ((aps.reflQuantParam + abh.QpOffset) >= 32) ? 12 : -6;
  int shiftBits = aps.reflMaxTransNum == 4 ? std::max(3, 3 * ((MaxBits - MinBits) / 3)) + shift : 3;
  std::vector<int> length;
  vector<int> numofGroupCount;
  getLengthRef(shift, pointCloudCode, length, numofGroupCount, aps.maxNumofCoeff, shiftBits,
               aps.reflMaxTransNum, true);
  int subGroupCount = 0;
  int subgroupIndex = 0;
  // Initialize reflectance predictor
  PC_REFL predictorRefl;
  int64_t codedValue;
  std::vector<neighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int prevIndex = -1;
  PC_REFL lastref = 0;

  // predict parameters
  predictOptParams predictOptParams;
  predictOptParams.colorQP = aps.reflQuantParam;
  predictOptParams.maxNumOfNeighbours = aps.maxNumOfNeighbours;
  predictOptParams.minNeighborDisFlag = aps.colorQPAdjustFlag || aps.chromaDeadzoneFlag;
  predictOptParams.minNeighborDis = 0;
  predictOptParams.reflThreshold =
    (aps.reflQuantParam + abh.QpOffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  predictOptParams.predFixedPointFracBit = aps.predFixedPointFracBit;
  predictOptParams.axisBias = aps.axisBias;
  predictOptParams.reflCrossAttrTypePred = false;
  predictOptParams.reflUpdateFlag = true;
  predictOptParams.reflectanceDistWeight = {0, 0, 0};
  predictOptParams.curReflWithCoef = 0;
  predictOptParams.curColorWithCoef = {0, 0, 0};

  // transform and entropy coding parameter
  std::vector<int> transformPointIdx;
  int64_t transformBuf[1][8] = {};
  int64_t transformPredBuf[1][8] = {};

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
    Bool isDuplicatePoint = curIndex > length[0] && curPosition == outputPointCloud[prevIndex] &&
      m_hls->aps.eligibleDupPointPred;

    if (isDuplicatePoint) {
      predictorRefl = lastref;
      isDuplicatePoint &= multil_ID == 0;
      reflectanceReconstruction(predictorRefl, transformBuf[0][subGroupCount], currentValue);
      subgroupIndex++;
      prevIndex = pointIndex;
      lastref = currentValue;
    } else {
      transformPointIdx.push_back(curIndex);
      if (aps.reflGroupPredict && (length[subgroupIndex] < 3)) {
        if (subGroupCount == 0) {
          getReflPredictorFarthest(curIndex, transformPointIdx[0], curPosition, subGroupCount,
                                   neighborSet, predictOptParams, predictorRefl);
        }
      } else {
        getReflPredictorFarthest(curIndex, transformPointIdx[0], curPosition, subGroupCount,
                                 neighborSet, predictOptParams, predictorRefl);
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
}

void TDecAttribute::AttributeInversePredictAndTransformMemControl() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const FrameHeader& frameheader = m_hls->frameheader;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;

  const int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();
  outputPointCloud.addReflectances();

  //Hilbert Sort
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReorderMode, pointCloudCode, voxelCount,
          aps.axisBias);

  //Qp compute
  quantizedQP colorQp;
  colorQp.attrQuantForLuma = aps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

  //dist weight calculation patameter
  uint64_t boundingSize =
    ((uint64_t)frameheader.geomBoundingBoxSize[0] * (uint64_t)frameheader.geomBoundingBoxSize[1] +
     (uint64_t)frameheader.geomBoundingBoxSize[0] * (uint64_t)frameheader.geomBoundingBoxSize[2] +
     (uint64_t)frameheader.geomBoundingBoxSize[1] * (uint64_t)frameheader.geomBoundingBoxSize[2])
    << 1;
  uint64_t reflectanceDistCoef =
    (boundingSize / frameheader.geomNumPoints > 0) ? boundingSize / frameheader.geomNumPoints : 1;
  UInt log2_reflectanceDistCoef = ceilLog2(reflectanceDistCoef);

  V3<int64_t> reflectanceRes = {0, 0, 0};
  V3<int64_t> reflResNum = {0, 0, 0};
  int distWeightGroupSize = 1 << aps.predDistWeightGroupSizeLog2;

  // color grouping
  int countColor = 0;
  PC_COL predictorColor;
  int64_t transformBufColor[3][8] = {};
  int64_t transformPredBufColor[3][8] = {};
  vector<int> lengthColor;
  std::vector<int> transformPointIdxColor;
  transformPointIdxColor.reserve(8);

  int maxNodeSizeLog2 = std::max(
    {1U, m_hls->gbh.nodeSizeLog2[0], m_hls->gbh.nodeSizeLog2[1], m_hls->gbh.nodeSizeLog2[2]});
  int groupShiftBits = std::max(3, 3 * (maxNodeSizeLog2 - ((ceilLog2(voxelCount / 4) + 1) >> 1)));

  // adjust color QP per point tool
  bool colorQPAdjustSliceFlag = aps.colorQPAdjustFlag;
  bool colorQPAdjustFlag = false;
  int scalar = 1;
  if (colorQPAdjustSliceFlag)
    scalar = abh.colorQPAdjustScalar;
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
  bool dupPredFlagRefl = 0;
  PC_REFL lastref;
  int64_t transformBufRefl[3][8] = {};
  int64_t transformPredBufRefl[3][8] = {};
  PC_REFL predictorRefl;
  int countRefl = 0;
  int subgroupIndexRefl = 0;
  int MaxBits =
    m_hls->gbh.nodeSizeLog2[0] + m_hls->gbh.nodeSizeLog2[1] + m_hls->gbh.nodeSizeLog2[2];
  MaxBits = TComClip(0, 32, MaxBits);
  int MinBits = ceilLog2(voxelCount);
  int shift = ((aps.reflQuantParam + abh.QpOffset) >= 32) ? 12 : -6;
  groupShiftBits =
    aps.reflMaxTransNum == 4 ? std::max(3, 3 * ((MaxBits - MinBits) / 3)) + shift : 3;

  getLengthRef(shift, pointCloudCode, lengthRefl, numofGroupCountRefl, aps.maxNumofCoeff,
               groupShiftBits, aps.reflMaxTransNum, false);
  // predict parameters
  predictOptParams predictOptParams;
  predictOptParams.colorQP = aps.colorQuantParam;
  predictOptParams.maxNumOfNeighbours = aps.maxNumOfNeighbours;
  predictOptParams.minNeighborDisFlag = aps.colorQPAdjustFlag || aps.chromaDeadzoneFlag;
  predictOptParams.minNeighborDis = 0;
  predictOptParams.reflThreshold = 0;
  predictOptParams.predFixedPointFracBit = aps.predFixedPointFracBit;
  predictOptParams.axisBias = aps.axisBias;
  predictOptParams.reflectanceDistWeight = {0, 0, 0};
  predictOptParams.curReflWithCoef = 0;
  predictOptParams.curColorWithCoef = {0, 0, 0};
  predictOptParams.colorCrossAttrTypePred = false;
  predictOptParams.reflCrossAttrTypePred = false;
  predictOptParams.colorUpdateFlag = true;
  predictOptParams.reflUpdateFlag = true;

  vector<V3<int64_t>> colorWithCoef(0);
  vector<uint64_t> reflWithCoef(0);
  uint64_t crossAttrTypeLambda = 0;
  int64_t crossAttrTypeCoef = 0;
  if (aps.crossAttrTypePred && (!aps.attrEncodeOrder)) {
    crossAttrTypeLambda =
      (-aps.colorQuantParam * aps.crossAttrTypePredParam1 + aps.crossAttrTypePredParam2);
    UInt maxPos = frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
      frameheader.geomBoundingBoxSize[2];
    uint64_t maxColor = (1 << m_hls->aps.colorOutputDepth) - 1;
    uint64_t maxColorSum = 3 * maxColor;
    crossAttrTypeCoef = round((maxPos << 10) / (double)maxColorSum);
    crossAttrTypeCoef = uint64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;
    colorWithCoef.resize(voxelCount);
    predictOptParams.reflCrossAttrTypePred = true;
  } else if (aps.crossAttrTypePred && aps.attrEncodeOrder) {
    crossAttrTypeLambda = (-(aps.reflQuantParam + abh.QpOffset) * aps.crossAttrTypePredParam1 +
                           aps.crossAttrTypePredParam2);
    auto diffPos = frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
      frameheader.geomBoundingBoxSize[2];
    uint64_t maxRefl = (1 << m_hls->aps.reflOutputDepth) - 1;
    crossAttrTypeCoef = round((diffPos << 10) / (double)maxRefl);
    crossAttrTypeCoef = uint64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;
    reflWithCoef.resize(voxelCount, 0);
    predictOptParams.colorCrossAttrTypePred = true;
  }

  //Initialize predictors
  PC_POS prePosition = outputPointCloud[pointCloudCode[0].index];
  PC_REFL preReflectance = 0;
  int setlength = aps.maxNumOfNeighbours;
  std::vector<neighborSet> neighborSetRefl;
  neighborSetRefl.resize(setlength);
  std::vector<neighborSet> neighborSetColor;
  neighborSetColor.resize(setlength);

  int run_length_col = m_decBac->parseRunlength();
  int run_length_refl = m_decBacDual->parseRunlength();
  int lengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;
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

  numofCoeffRefl = accumulate(lengthRefl.begin(), lengthRefl.begin() + numofGroupCountRefl[0], 0);
  runlengthDecodeMemControl(numofCoeffRefl, CoeffGroupRefl, run_length_refl, lengthControl,
                            isLengthControlRefl, false);

  numofCoeffColor =
    accumulate(lengthColor.begin(), lengthColor.begin() + numofGroupCountColor[0], 0);
  runlengthDecodeMemControl(numofCoeffColor, CoeffGroupColor, run_length_col, lengthControl,
                            isLengthControlColor, true);

  //Predicting
  //InverseTransform
  //disable cross-attribute-type-prediction
  if (!aps.crossAttrTypePred) {
    for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
      //predict and code
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = outputPointCloud[pointIndex];
      transformPointIdxRefl.push_back(curIndex);
      transformPointIdxColor.push_back(curIndex);
      if (aps.reflGroupPredict && (countNumRefl < 3)) {
        if (countRefl == 0) {
          getReflPredictorFarthestDual(curIndex, transformPointIdxColor[0], curPosition, countRefl,
                                       neighborSetRefl, predictOptParams, predictorRefl);
        }
      } else {
        getReflPredictorFarthestDual(curIndex, transformPointIdxColor[0], curPosition, countRefl,
                                     neighborSetRefl, predictOptParams, predictorRefl);
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
                                        lengthControl, isLengthControlRefl, false);
          }
        }
      }
      getColorPredictorFarthest(curIndex, transformPointIdxColor[0], curPosition, countColor,
                                neighborSetColor, predictOptParams, predictorColor);

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
          colorQPAdjustFlag = predictOptParams.minNeighborDis > colorQPAdjustDis;

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
                                        lengthControl, isLengthControlColor, true);
          }
        }
      }
    }
  }
  // predict reflectance using color
  else if (!aps.attrEncodeOrder) {
    for (int curIndexColor = 0, curIndexRefl = 0; curIndexRefl < voxelCount;) {
      while (curIndexColor < (curIndexRefl + countNumRefl) ||
             (countColor < countNumColor && countColor > 0)) {
        if (countColor == 0 && (curIndexColor > (curIndexRefl + countNumRefl)))
          break;
        transformPointIdxColor.push_back(curIndexColor);
        int pointIndex = pointCloudCode[curIndexColor].index;
        const PC_POS& curPosition = outputPointCloud[pointIndex];
        getColorPredictorFarthest(curIndexColor, transformPointIdxColor[0], curPosition, countColor,
                                  neighborSetColor, predictOptParams, predictorColor);
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
            colorQPAdjustFlag = predictOptParams.minNeighborDis > colorQPAdjustDis;

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
                                        lengthControl, isLengthControlColor, true);
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
          runlengthDecodeMemControl(numofCoeffRefl, CoeffGroupRefl, run_length_refl, lengthControl,
                                    isLengthControlRefl, false);
      }
      while (countRefl < countNumRefl) {
        transformPointIdxRefl.push_back(curIndexRefl);
        auto pointIndex = pointCloudCode[curIndexRefl].index;
        const PC_POS& curPosition = outputPointCloud[pointIndex];
        predictOptParams.curColorWithCoef = colorWithCoef[pointIndex];
        if (aps.reflGroupPredict && (countNumRefl < 3)) {
          if (countRefl == 0) {
            getReflPredictorFarthestDual(curIndexRefl, transformPointIdxRefl[0], curPosition,
                                         countRefl, neighborSetRefl, predictOptParams,
                                         predictorRefl);
          }
        } else {
          getReflPredictorFarthestDual(curIndexRefl, transformPointIdxRefl[0], curPosition,
                                       countRefl, neighborSetRefl, predictOptParams, predictorRefl);
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
          neighborSetRefl[transformPointIdxRefl[i] % setlength].colorWithCoef =
            colorWithCoef[pointIndex];
        }
      } else {
        for (int i = 0; i < countRefl; ++i) {
          auto pointIndex = pointCloudCode[transformPointIdxRefl[i]].index;
          neighborSetRefl[i].colorWithCoef = colorWithCoef[pointIndex];
          PC_REFL curReflectance = outputPointCloud.getReflectance(pointIndex, multil_ID);
          calculateReflTrend(outputPointCloud[pointIndex], prePosition, curReflectance,
                             preReflectance, log2_reflectanceDistCoef, reflectanceRes, reflResNum,
                             predictOptParams.reflectanceDistWeight, distWeightGroupSize);

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
        auto pointIndex = pointCloudCode[curIndexRefl].index;
        const PC_POS& curPosition = outputPointCloud[pointIndex];
        transformPointIdxRefl.push_back(curIndexRefl);

        if (aps.reflGroupPredict && (countNumRefl < 3)) {
          if (countRefl == 0) {
            getReflPredictorFarthestDual(curIndexRefl, transformPointIdxRefl[0], curPosition,
                                         countRefl, neighborSetRefl, predictOptParams,
                                         predictorRefl);
          }
        } else {
          getReflPredictorFarthestDual(curIndexRefl, transformPointIdxRefl[0], curPosition,
                                       countRefl, neighborSetRefl, predictOptParams, predictorRefl);
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
                               preReflectance, log2_reflectanceDistCoef, reflectanceRes, reflResNum,
                               predictOptParams.reflectanceDistWeight, distWeightGroupSize);

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
                                        lengthControl, isLengthControlRefl, false);
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
          runlengthDecodeMemControl(numofCoeffColor, CoeffGroupColor, run_length_col, lengthControl,
                                    isLengthControlColor, true);
      }
      while (countColor < countNumColor) {
        transformPointIdxColor.push_back(curIndexColor);
        int pointIndex = pointCloudCode[curIndexColor].index;
        const PC_POS& curPosition = outputPointCloud[pointIndex];
        predictOptParams.curReflWithCoef = reflWithCoef[pointIndex];
        getColorPredictorFarthest(curIndexColor, transformPointIdxColor[0], curPosition, countColor,
                                  neighborSetColor, predictOptParams, predictorColor);

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
        colorQPAdjustFlag = predictOptParams.minNeighborDis > colorQPAdjustDis;
      colorReconstructionTrans(pointCloudCode, transformPointIdxColor, transformBufColor,
                               transformPredBufColor, colorQp, countColor, neighborSetColor,
                               colorQPAdjustFlag);

      if (transformPointIdxColor[0] <= setlength) {
        for (int i = 0; i < countColor; ++i) {
          auto pointIndex = pointCloudCode[transformPointIdxColor[i]].index;
          neighborSetColor[transformPointIdxColor[i] % setlength].reflWithCoef =
            reflWithCoef[pointIndex];
        }
      } else {
        for (int i = 0; i < countColor; ++i) {
          auto pointIndex = pointCloudCode[transformPointIdxColor[i]].index;
          neighborSetColor[i].reflWithCoef = reflWithCoef[pointIndex];
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
    InverseQuantizeResidual(absDelta, m_hls->aps.reflQuantParam + m_hls->abh.QpOffset);
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

void TDecAttribute::parseColorResidualCorrelationCode(V3<int64_t>& codedValue, const bool& os,
                                                      const bool isDuplicatePoint,
                                                      const UInt& golombNum) {
  V3<bool> codeSign = true;
  if (isDuplicatePoint) {
    if (os)
      codeSign[1] = false;
    else
      codeSign[0] = false;
  }
  bool isColor = true;
  bool residualminusone = true;
  int flagy_r = m_decBac->parseAttrequalone();
  if (flagy_r == 1) {
    codedValue[0] = 0;
  } else {
    codedValue[0] =
      m_decBac->parseAttr(isColor, 0, 1, 0 == 0 && isDuplicatePoint, residualminusone, golombNum);
  }
  if (codedValue[0] == 0) {
    int flagyu_rg = m_decBac->parseAttrequaltwo();
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
      if (codeSign[1])
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
    if (codeSign[0])
      m_decBac->parseSign(codedValue[0]);
    if (codedValue[1] != 0 && codeSign[1]) {
      m_decBac->parseSign(codedValue[1]);
    }
    if (codedValue[2] != 0) {
      m_decBac->parseSign(codedValue[2]);
    }
  }
}

void TDecAttribute::colorReconstructionTrans(
  std::vector<pointCodeWithIndex>& pointCloudHilbert, std::vector<int>& transformPointIdx,
  int64_t transformBuf[][8], int64_t transformPredBuf[][8], const quantizedQP& colorQp, int& count,
  std::vector<neighborSet>& neighborSet, bool colorQPAdjustFlag) {
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
  int64_t transformBuf[1][8], int64_t transformPredBuf[1][8], std::vector<neighborSet>& neighborSet,
  PC_REFL& lastref) {
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
        m_hls->aps.reflQuantParam + m_hls->abh.QpOffset + 72 + m_hls->aps.QpOffsetDC;
    else
      transQuantParam =
        m_hls->aps.reflQuantParam + m_hls->abh.QpOffset + 72 + m_hls->aps.QpOffsetAC;
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
          parseColorResidualCorrelationCode(values, os, false, colorGolombNum);
          if (os)
            std::swap(values[0], values[1]);
          run_length = m_decBac->parseRunlength();
        }
      }
      for (int d = 0; d < 3; ++d)
        Coefficients[3 * n + d] = values[d];
    }
  } else {
    auto reflGolombNum = aps.reflGolombNum;
    const bool refGolomb = reflGolombNum == 1 ? true : false;
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
            values = m_decBacDual->parseAttr(false, 3, 0, false, false, reflGolombNum);
            run_length = m_decBacDual->parseRunlength();
          } else {
            values = m_decBac->parseAttr(false, 3, 0, false, false, reflGolombNum);
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
  const FrameHeader& frameheader = m_hls->frameheader;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;

  bool isLengthControl = false;
  bool os = aps.orderSwitch;
  cout << "colorInverseWaveletTransform" << endl;
  int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReorderMode, pointCloudCode, voxelCount, 1);

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
  int resLayerQuantParam = aps.colorQuantParam;
  int coeffQuantParam = aps.colorQuantParam;
  if (resLayer)
    coeffQuantParam += aps.attrTransQpDelta;
  V3<int> preColor;
  V3<int64_t> Values;
  PC_COL recColor;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;
  UInt64 meanBB = (frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
                   frameheader.geomBoundingBoxSize[2]) /
    3;
  UInt disThInit = meanBB * meanBB;
  if (abh.colorInitPredTransRatio >= 0)
    disThInit = disThInit << abh.colorInitPredTransRatio;
  else if (abh.colorInitPredTransRatio < 0)
    disThInit = disThInit >> std::abs(abh.colorInitPredTransRatio);
  disThInit = std::max({(UInt)1, disThInit / frameheader.geomNumPoints});
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
          parseColorResidualCorrelationCode(Values, os, false, aps.colorGolombNum);
          if (os)
            std::swap(Values[0], Values[1]);
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

    WaveletCoreInverseTransform(attributes, 3, segmentVoxelCount, integerizedAttributes, sps, aps,
                                abh, segmentPosition, disThInit);
    if (resLayer) {
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
            parseColorResidualCorrelationCode(Values, os, false, aps.colorGolombNum);
            if (os)
              std::swap(Values[0], Values[1]);
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
}

void TDecAttribute::colorInverseWaveletTransformFromReflectance() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameheader = m_hls->frameheader;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;

  bool isLengthControl = false;
  bool os = aps.orderSwitch;
  cout << "colorInverseWaveletTransformFromReflectance" << endl;
  int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addColors();

  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.colorReorderMode, pointCloudCode, voxelCount, 1);

  // Allocate arrays.
  int attribCount = 3;
  FXPoint* attributesBuf = new FXPoint[voxelCount * attribCount];
  int* integerizedAttributesBuf = new int[voxelCount * attribCount];
  int* positionBuf = new int[voxelCount * 3];

  // new
  int* reflectanceBuf = new int[voxelCount];
  uint64_t crossAttrTypeLambda = 0;
  int64_t crossAttrTypeCoef = 0;
  crossAttrTypeLambda = (-(aps.reflQuantParam + abh.QpOffset) * aps.crossAttrTypePredParam1 +
                         aps.crossAttrTypePredParam2);
  auto diffPos = frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
    frameheader.geomBoundingBoxSize[2];
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
  int resLayerQuantParam = aps.colorQuantParam;
  int coeffQuantParam = aps.colorQuantParam;
  if (resLayer)
    coeffQuantParam += aps.attrTransQpDelta;
  V3<int> preColor;
  V3<int64_t> Values;
  PC_COL recColor;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;

  UInt64 meanBB = (frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
                   frameheader.geomBoundingBoxSize[2]) /
    3;
  UInt disThInit = meanBB * meanBB;
  if (abh.colorInitPredTransRatio >= 0)
    disThInit = disThInit << abh.colorInitPredTransRatio;
  else if (abh.colorInitPredTransRatio < 0)
    disThInit = disThInit >> std::abs(abh.colorInitPredTransRatio);
  disThInit = std::max({(UInt)1, disThInit / frameheader.geomNumPoints});

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
          parseColorResidualCorrelationCode(Values, os, false, aps.colorGolombNum);
          if (os)
            std::swap(Values[0], Values[1]);
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

    WaveletCoreInverseTransform(attributes, 3, segmentVoxelCount, integerizedAttributes, sps, aps,
                                abh, segmentPosition, disThInit, reflectances, 1);
    if (resLayer) {
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
            parseColorResidualCorrelationCode(Values, os, false, aps.colorGolombNum);
            if (os)
              std::swap(Values[0], Values[1]);
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
}

void TDecAttribute::reflectanceInverseWaveletTransformFromColor() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameheader = m_hls->frameheader;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  cout << "reflectanceInverseWaveletTransformFromColor" << endl;
  int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addReflectances();

  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.reflReorderMode, pointCloudCode, voxelCount,
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
  crossAttrTypeLambda = (-(aps.reflQuantParam + abh.QpOffset) * aps.crossAttrTypePredParam1 +
                         aps.crossAttrTypePredParam2);
  UInt maxPos = frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
    frameheader.geomBoundingBoxSize[2];
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
  int resLayerQuantParam = aps.reflQuantParam + abh.QpOffset;
  int coeffQuantParam = aps.reflQuantParam + abh.QpOffset;
  int golombnum = resLayer ? 3 : aps.reflGolombNum;

  bool isLengthControl = false;
  if (resLayer)
    coeffQuantParam += aps.attrTransQpDelta;
  int64_t r = 0;
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (m_hls->aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (m_hls->aps.reflOutputDepth)) - 1;
  }

  UInt64 meanBB = (frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
                   frameheader.geomBoundingBoxSize[2]) /
    3;
  int disThInit = meanBB * meanBB / frameheader.geomNumPoints;
  if (abh.reflInitPredTransRatio >= 0)
    disThInit = disThInit << abh.reflInitPredTransRatio;
  else if (abh.reflInitPredTransRatio < 0)
    disThInit = disThInit >> std::abs(abh.reflInitPredTransRatio);
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

    WaveletCoreInverseTransform(attributes, 1, segmentVoxelCount, integerizedAttributes, sps, aps,
                                abh, segmentPosition, disThInit, colors, 3);
    if (resLayer) {
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
}
//------------------------------------------------------------------------------------------------------------------

void TDecAttribute::reflectanceInverseWaveletTransform() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameheader = m_hls->frameheader;
  TComPointCloud& outputPointCloud = *m_pointCloudRecon;
  cout << "reflectanceInverseWaveletTransform" << endl;
  int voxelCount = int(outputPointCloud.getNumPoint());
  outputPointCloud.addReflectances();
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(outputPointCloud.positions(), aps.reflReorderMode, pointCloudCode, voxelCount,
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
  int resLayerQuantParam = aps.reflQuantParam + abh.QpOffset;
  int coeffQuantParam = aps.reflQuantParam + abh.QpOffset;
  int golombnum = resLayer ? 3 : aps.reflGolombNum;

  bool isLengthControl = false;
  if (resLayer)
    coeffQuantParam += aps.attrTransQpDelta;
  int64_t r = 0;
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (m_hls->aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (m_hls->aps.reflOutputDepth)) - 1;
  }

  UInt64 meanBB = (frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
                   frameheader.geomBoundingBoxSize[2]) /
    3;
  int disThInit = meanBB * meanBB / frameheader.geomNumPoints;
  if (abh.reflInitPredTransRatio >= 0)
    disThInit = disThInit << abh.reflInitPredTransRatio;
  else if (abh.reflInitPredTransRatio < 0)
    disThInit = disThInit >> std::abs(abh.reflInitPredTransRatio);
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

    WaveletCoreInverseTransform(attributes, 1, segmentVoxelCount, integerizedAttributes, sps, aps,
                                abh, segmentPosition, disThInit);
    if (resLayer) {
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
  reOrder(outputPointCloud.positions(), aps.reflReorderMode, pointCloudCode, voxelCount,
          aps.axisBias);

  // get reflectance predictor
  PC_REFL predictorRefl[5];
  PC_REFL codedValue[5];
  PC_REFL reconValue[5];
  std::vector<multiReflNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int setCount = 0;
  int multil_ID_Group_Num = aps.multiAttriGroupNum[aps.multiAttrGroupID[multil_ID]];

  // predict parameters
  predictOptParams predictOptParams;
  predictOptParams.colorQP = 0;
  predictOptParams.maxNumOfNeighbours = aps.maxNumOfNeighbours;
  predictOptParams.minNeighborDisFlag = aps.colorQPAdjustFlag || aps.chromaDeadzoneFlag;
  predictOptParams.minNeighborDis = 0;
  predictOptParams.reflThreshold = 0;
  predictOptParams.predFixedPointFracBit = aps.predFixedPointFracBit;
  predictOptParams.axisBias = aps.axisBias;
  predictOptParams.reflCrossAttrTypePred = false;
  predictOptParams.reflUpdateFlag = true;
  predictOptParams.reflectanceDistWeight = {0, 0, 0};
  predictOptParams.curReflWithCoef = 0;
  predictOptParams.curColorWithCoef = {0, 0, 0};

  // runlength
  bool isLengthControl = false;
  int run_length = m_decBac->parseRunlength();
  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    auto pointIndex = pointCloudCode[curIndex].index;
    PC_REFL& currentValue = outputPointCloud.getReflectance(pointIndex, multil_ID);
    const PC_POS& curPosition = outputPointCloud[pointIndex];
    getMultiReflectancePredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                                         predictOptParams, predictorRefl, multil_ID_Group_Num);

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
          codedValue[multi_id] = m_decBac->parseAttr(false, 3, 0, false, false, aps.reflGolombNum);
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
      InverseQuantizeResidual(absDelta, m_hls->aps.reflQuantParam + m_hls->abh.QpOffset);
    int64_t residual = inverseResidualQuant * sign;
    reconValue[multi_id] = TComClip((Int64)ClipMin, (Int64)ClipMax,
                                    residual + predictor[multi_id] + residualPrevComponent);
    if (ccp && multi_id < (multil_ID_Group_Num - 1)) {
      residualPrevComponent = (Int)reconValue[multi_id] - (Int)predictor[multi_id];
    }
  }
}
