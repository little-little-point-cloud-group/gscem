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
#include "common/AttributePredictor.h"
#include "common/FXPoint.h"
#include "common/TComPointCloud.h"
#include "common/Transform.cpp"
#include "common/Transform.h"
#include <algorithm>
#include <numeric>
#include <time.h>
#include <vector>

using namespace std;
////Attribute predicting and compress
Void TEncAttribute::init(TComPointCloud* pointCloudOrg, TComPointCloud* pointCloudRecon,
                         HighLevelSyntax* hls, TEncBacTop* encBac, const int& m_frame_ID,
                         const int& m_numOfFrames, const int& multil_Id) {
  m_pointCloudOrg = pointCloudOrg;
  m_pointCloudRecon = pointCloudRecon;
  m_pointCloudRecon->setNumPoint(m_pointCloudOrg->getNumPoint());
  m_hls = hls;
  m_encBac = encBac;
  m_encBacDual = NULL;
  frame_Id = m_frame_ID;
  frame_Count = m_numOfFrames;
  multil_ID = multil_Id;
}

Void TEncAttribute::initDual(TComPointCloud* pointCloudOrg, TComPointCloud* pointCloudRecon,
                             HighLevelSyntax* hls, TEncBacTop* encBac, TEncBacTop* encBacDual,
                             const int& m_frame_ID, const int& m_numOfFrames,
                             const int& multil_Id) {
  m_pointCloudOrg = pointCloudOrg;
  m_pointCloudRecon = pointCloudRecon;
  m_pointCloudRecon->setNumPoint(m_pointCloudOrg->getNumPoint());
  m_hls = hls;
  m_encBac = encBac;
  if (m_hls->aps.attributePresentFlag[0] && m_hls->aps.attributePresentFlag[1]) {
    m_encBacDual = encBacDual;
  } else {
    m_encBacDual = NULL;
  }
  frame_Id = m_frame_ID;
  frame_Count = m_numOfFrames;
  multil_ID = multil_Id;
}

// Attribute compression consists of the following stages:
//  - reorder
//  - prediction
//  - residual quantization
//  - entropy encode
//  - local decode
Void TEncAttribute::predictEncodeAttribute() {
  clock_t userTimeColorBegin = clock();
  attributePredictingResidual();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
  m_colorBits = m_encBac->getBitStreamLength() * 8;
  clock_t userTimeReflectanceBegin = clock();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
  m_reflBits = m_encBac->getBitStreamLength() * 8 - m_colorBits;
}

Void TEncAttribute::transformEncodeAttribute() {
  const AttributeParameterSet& aps = m_hls->aps;
  bool isEnableCrossAttrTypePred = aps.crossAttrTypePred;
  if (!isEnableCrossAttrTypePred) {
    clock_t userTimeColorBegin = clock();
    colorWaveletTransform();
    m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
    m_colorBits = m_encBac->getBitStreamLength() * 8;
    clock_t userTimeReflectanceBegin = clock();
    reflectanceWaveletTransform();
    m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
    m_reflBits = m_encBac->getBitStreamLength() * 8 - m_colorBits;
  } else if (!aps.attrEncodeOrder) {  // firstly encode color
    clock_t userTimeColorBegin = clock();
    colorWaveletTransform();
    m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
    m_colorBits = m_encBac->getBitStreamLength() * 8;
    clock_t userTimeReflectanceBegin = clock();
    reflectanceWaveletTransformFromColor();
    m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
    m_reflBits = m_encBac->getBitStreamLength() * 8 - m_colorBits;
  } else {  // firstly encode refl, CTC default
    clock_t userTimeReflectanceBegin = clock();
    reflectanceWaveletTransform();
    m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
    m_reflBits = m_encBac->getBitStreamLength() * 8;
    clock_t userTimeColorBegin = clock();
    colorWaveletTransformFromReflectance();
    m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
    m_colorBits = m_encBac->getBitStreamLength() * 8 - m_reflBits;
  }
  m_encBac->codeSliceAttrEndCode();
  if (frame_Id == frame_Count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_encBac->codeSPSEndCode();
}

Void TEncAttribute::predictAndTransformEncodeAttribute() {
  clock_t userTimeColorBegin = clock();
  attributePredictAndTransformMemControl();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
  clock_t userTimeReflectanceBegin = clock();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}

Void TEncAttribute::predictEncodeColor() {
  clock_t userTimeColorBegin = clock();
  colorPredictingResidual();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
}

Void TEncAttribute::transformEncodeColor() {
  clock_t userTimeColorBegin = clock();
  colorWaveletTransform();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
}

Void TEncAttribute::predictAndTransformEncodeColor() {
  clock_t userTimeColorBegin = clock();
  colorPredictAndTransformMemControl();
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
}

Void TEncAttribute::predictEncodeReflectance() {
  clock_t userTimeReflectanceBegin = clock();
  reflectancePredictingResidual();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}

Void TEncAttribute::predictEncodeMultiReflectance() {
  clock_t userTimeReflectanceBegin = clock();
  multiReflectancePredictingResidual();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}

Void TEncAttribute::transformEncodeReflectance() {
  clock_t userTimeReflectanceBegin = clock();
  reflectanceWaveletTransform();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}

Void TEncAttribute::predictAndTransformEncodeReflectance() {
  clock_t userTimeReflectanceBegin = clock();
  reflectancePredictAndTransformMemControl();
  m_reflTime = (Double)(clock() - userTimeReflectanceBegin) / CLOCKS_PER_SEC;
}

void TEncAttribute::attributePredictingResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());
  // cross attribute parameter
  bool isEnableCrossAttrTypePred = aps.crossAttrTypePred;
  uint64_t crossAttrTypeLambda = 0;
  int64_t crossAttrTypeCoef = 0;
  if (isEnableCrossAttrTypePred && (!aps.attrEncodeOrder)) {
    crossAttrTypeLambda =
      (-sps.colorQuantParam * aps.crossAttrTypePredParam1 + aps.crossAttrTypePredParam2);
    UInt maxPos =
      sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2];
    uint64_t maxColor = (1 << m_hls->aps.colorOutputDepth) - 1;
    uint64_t maxColorSum = 3 * maxColor;
    crossAttrTypeCoef = round((maxPos << 10) / (double)maxColorSum);
    crossAttrTypeCoef = uint64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;
  } else if (isEnableCrossAttrTypePred && (aps.attrEncodeOrder)) {
    crossAttrTypeLambda = (-(sps.reflQuantParam + abh.reflQPoffset) * aps.crossAttrTypePredParam1 +
                           aps.crossAttrTypePredParam2);
    auto diffPos =
      sps.geomBoundingBoxSize[0] + sps.geomBoundingBoxSize[1] + sps.geomBoundingBoxSize[2];
    uint64_t maxRefl = (1 << m_hls->aps.reflOutputDepth) - 1;
    crossAttrTypeCoef = round((diffPos << 10) / (double)maxRefl);
    crossAttrTypeCoef = uint64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;
  }
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReordermode, pointCloudCode, voxelCount, 1);
  // Qp compute
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

  PC_POS prePosition = pointCloudRec[pointCloudCode[0].index];
  PC_REFL preReflectance = 0;
  int distWeightGroupSize = 1 << aps.log2predDistWeightGroupSize;
  // obtain predictors
  pair<PC_COL, PC_REFL> predictorAttr;
  PC_COL predictorColor;
  PC_REFL predictorRefl;
  int prevIndex = -1;
  int setlength = aps.maxNumOfNeighbours;
  int setCount = 0;
  // runlength
  int run_length_col = 0;
  int run_length_refl = 0;
  const bool refisGolomb = m_hls->aps.refGolombNum == 1 ? true : false;
  const bool colorisGolomb = m_hls->aps.colorGolombNum == 1 ? true : false;
  // disable cross-attribute-type-prediction
  if (!isEnableCrossAttrTypePred) {
    vector<attrNeighborSet> neighborSet;
    neighborSet.resize(setlength);
    for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
      auto pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = pointCloudRec[pointIndex];
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
      PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
      Bool isDuplicatePoint = curIndex > 0 && curPosition == pointCloudRec[prevIndex];
      if (isDuplicatePoint) {
        predictorColor = pointCloudRec.getColor(prevIndex, multil_ID);
        predictorRefl = pointCloudRec.getReflectance(prevIndex, multil_ID);
      } else {
        predictorAttr = getAttributePredictorFarthest(curIndex, curIndex, pointCloudRec, sps, aps,
                                                      pointCloudCode, setCount, neighborSet);
        predictorColor = predictorAttr.first;
        predictorRefl = predictorAttr.second;
      }
      colorPredictCode(predictorColor, curColor, colorQp, run_length_col, isDuplicatePoint);
      reflectancePredictCode(predictorRefl, curReflectance, run_length_refl, isDuplicatePoint);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = pointCloudRec[pointIndex];
      neighborSet[farthestIdx].color = curColor;
      neighborSet[farthestIdx].refl = curReflectance;
      prevIndex = pointIndex;
    }
    m_encBac->encodeRunlength(run_length_col);
    m_encBacDual->encodeRunlength(run_length_refl);
    // enable cross-attribute-type-prediction , encode the color and then encode the reflectance
  } else if (!aps.attrEncodeOrder) {
    std::vector<reflNeighborSet> neighborSet;
    neighborSet.resize(setlength);
    int setCount = 0;
    std::vector<colorWithCoefNeighborSet> colorWithCoefNeighborSet;
    colorWithCoefNeighborSet.resize(setlength);
    vector<colorNeighborSet> reflbuffer;
    for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
      auto pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = pointCloudRec[pointIndex];
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
      PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
      Bool isDuplicatePoint = curIndex > 0 && curPosition == pointCloudRec[prevIndex];
      if (isDuplicatePoint) {
        predictorColor = pointCloudRec.getColor(prevIndex, multil_ID);
      } else {
        predictorColor =
          getColorPredictorNoUpdate(curIndex, curIndex, pointCloudRec, sps, aps, pointCloudCode,
                                    setCount, neighborSet, colorWithCoefNeighborSet, reflbuffer);
      }
      colorPredictCode(predictorColor, curColor, colorQp, run_length_col, isDuplicatePoint);
      // calculate the color with coefficient, which is used in cross-attribute-type-prediction
      V3<int64_t> curColorWithCoef;
      curColorWithCoef[0] = curColor[0] * crossAttrTypeCoef;
      curColorWithCoef[1] = curColor[1] * crossAttrTypeCoef;
      curColorWithCoef[2] = curColor[2] * crossAttrTypeCoef;
      if (isDuplicatePoint) {
        predictorRefl = pointCloudRec.getReflectance(prevIndex, multil_ID);
      } else {
        // get the better refl prediction through the geometric distance and the color distance
        predictorRefl = getReflectancePredictorFromColor(
          curIndex, curIndex, pointCloudRec, sps, aps, pointCloudCode, setCount, neighborSet,
          colorWithCoefNeighborSet, curColorWithCoef, reflectanceDistWeight);
      }
      reflectancePredictCode(predictorRefl, curReflectance, run_length_refl, isDuplicatePoint);

      if (curIndex != 0)
        calculateReflTrend(curPosition, prePosition, curReflectance, preReflectance,
                           reflectanceDistCoef, reflectanceRes, reflResNum, reflectanceDistWeight,
                           distWeightGroupSize);
      prePosition = curPosition;
      preReflectance = curReflectance;

      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = pointCloudRec[pointIndex];
      neighborSet[farthestIdx].refl = curReflectance;
      colorWithCoefNeighborSet[farthestIdx].color = curColor;
      colorWithCoefNeighborSet[farthestIdx].colorWithCoef = curColorWithCoef;
      prevIndex = pointIndex;
    }
    m_encBac->encodeRunlength(run_length_col);
    m_encBacDual->encodeRunlength(run_length_refl);
    // enable cross-attribute-type-prediction , encode the reflectance and then encode the color
  } else {
    std::vector<colorNeighborSet> neighborSet;
    neighborSet.resize(setlength);
    std::vector<reflWithCoefNeighborSet> reflWithCoefNeighborSet;
    reflWithCoefNeighborSet.resize(setlength);
    vector<reflNeighborSet> reflbuffer;
    for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
      auto pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = pointCloudRec[pointIndex];
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
      PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
      Bool isDuplicatePoint = curIndex > 0 && curPosition == pointCloudRec[prevIndex];
      if (isDuplicatePoint) {
        predictorRefl = pointCloudRec.getReflectance(prevIndex, multil_ID);
      } else {
        predictorRefl = getReflectancePredictorNoUpdate(
          curIndex, curIndex, pointCloudRec, sps, aps, pointCloudCode, setCount, neighborSet,
          reflWithCoefNeighborSet, reflbuffer, reflectanceDistWeight);
      }
      reflectancePredictCode(predictorRefl, curReflectance, run_length_refl, isDuplicatePoint);
      // calculate the reflectance with coefficient, which is used in cross-attribute-type-prediction
      int64_t curReflWithCoef = curReflectance * crossAttrTypeCoef;
      //
      if (curIndex != 0)
        calculateReflTrend(curPosition, prePosition, curReflectance, preReflectance,
                           reflectanceDistCoef, reflectanceRes, reflResNum, reflectanceDistWeight,
                           distWeightGroupSize);
      prePosition = curPosition;
      preReflectance = curReflectance;
      if (isDuplicatePoint) {
        predictorColor = pointCloudRec.getColor(prevIndex, multil_ID);
      } else {
        // get the better color prediction through the geometric distance and the reflectance distance
        predictorColor = getColorPredictorFromReflectance(
          curIndex, curIndex, pointCloudRec, sps, aps, pointCloudCode, setCount, neighborSet,
          reflWithCoefNeighborSet, curReflWithCoef);
      }
      colorPredictCode(predictorColor, curColor, colorQp, run_length_col, isDuplicatePoint);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = pointCloudRec[pointIndex];
      neighborSet[farthestIdx].color = curColor;
      reflWithCoefNeighborSet[farthestIdx].refl = curReflectance;
      reflWithCoefNeighborSet[farthestIdx].reflWithCoef = curReflWithCoef;
      prevIndex = pointIndex;
    }
    m_encBacDual->encodeRunlength(run_length_refl);
    m_encBac->encodeRunlength(run_length_col);
  }

  m_encBac->codeSliceAttrEndCode();
  if (frame_Id == frame_Count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_encBac->codeSPSEndCode();
}

void TEncAttribute::reflectancePredictingResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.refReordermode, pointCloudCode, voxelCount, aps.axisBias);
  m_hls->sps.reflThreshold =
    (sps.reflQuantParam + abh.reflQPoffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  // obtain reflectance predictor
  int64_t predictorRefl;
  std::vector<reflNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int setCount = 0;
  int prevIndex = -1;
  PC_REFL lastref = 0;
  // runlength
  int run_length = 0;
  const bool isGolomb = m_hls->aps.refGolombNum == 1 ? true : false;
  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    auto pointIndex = pointCloudCode[curIndex].index;
      PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
      const PC_POS& curPosition = pointCloudRec[pointIndex];
      Bool isDuplicatePoint = curIndex > 0 && curPosition == pointCloudRec[prevIndex];

    if (isDuplicatePoint) {
        predictorRefl = lastref;
    } else {
        predictorRefl = getReflectancePredictorFarthest(curIndex, curIndex, pointCloudRec, sps, aps,
                                                        pointCloudCode, setCount, neighborSet);
    }
      isDuplicatePoint &= multil_ID == 0;
      reflectancePredictCode(predictorRefl, curReflectance, run_length, isDuplicatePoint);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = pointCloudRec[pointIndex];
      neighborSet[farthestIdx].refl = curReflectance;
      prevIndex = pointIndex;
      lastref = curReflectance;
  }
  m_encBac->encodeRunlength(run_length);

  m_encBac->codeSliceAttrEndCode();
  if (frame_Id == frame_Count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_encBac->codeSPSEndCode();
}

void TEncAttribute::colorPredictingResidual() {
    const SequenceParameterSet& sps = m_hls->sps;
    const AttributeParameterSet& aps = m_hls->aps;
    TComPointCloud& pointCloudRec = *m_pointCloudRecon;
    const int voxelCount = int(pointCloudRec.getNumPoint());
    // Reorder
    std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
    reOrder(pointCloudRec.positions(), aps.colorReordermode, pointCloudCode, voxelCount, 1);
    //Qp compute
    quantizedQP colorQp;
    colorQp.attrQuantForLuma = sps.colorQuantParam;
    colorQp.attrQuantForChromaCb =
      TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
    colorQp.attrQuantForChromaCr =
      TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

    // obtain color predictor
    PC_COL predictorColor;
    std::vector<colorNeighborSet> neighborSet;
    int setlength = aps.maxNumOfNeighbours;
    neighborSet.resize(setlength);
    PC_COL lastColor;
    int prevIndex = -1;
    int setCount = 0;
    int run_length = 0;
    const bool isGolomb = aps.colorGolombNum == 1 ? true : false;
    for (int curIndex = 0; curIndex < voxelCount; curIndex++) {
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = pointCloudRec[pointIndex];
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
      Bool isDuplicatePoint = curIndex > 0 && curPosition == pointCloudRec[prevIndex];
      if (isDuplicatePoint) {
        predictorColor = lastColor;
      } else {
        predictorColor = getColorPredictorFarthest(curIndex, curIndex, pointCloudRec, sps, aps,
                                                   pointCloudCode, setCount, neighborSet);
      }
      colorPredictCode(predictorColor, curColor, colorQp, run_length, isDuplicatePoint);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = pointCloudRec[pointIndex];
      neighborSet[farthestIdx].color = curColor;
      prevIndex = pointIndex;
      lastColor = curColor;
    }
    m_encBac->encodeRunlength(run_length);
  m_encBac->codeSliceAttrEndCode();
  if (frame_Id == frame_Count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_encBac->codeSPSEndCode();
}

void TEncAttribute::colorPredictAndTransformMemControl() {
  std::cout << "ColorPredictingAndTransformMemControl" << std::endl;
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());
  //Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReordermode, pointCloudCode, voxelCount, 1);
  // Qp compute
  quantizedQP colorQp;
  colorQp.attrQuantForLuma = sps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

  //quantization deadzone offset
  if (sps.colorQuantParam <= 48) {
    m_hls->sps.colorOffsetParamLumaAc =
      (0.2 + ((48 - sps.colorQuantParam) >> 3) * 0.01);  // Quantization Shift
    m_hls->sps.colorOffsetParamChromaAc = (0.02 + ((48 - sps.colorQuantParam) >> 3) * 0.08);
    m_hls->sps.colorOffsetParamLumaDc =
      (0.18 + ((48 - sps.colorQuantParam) >> 3) * 0.008);  // Quantization Shift
    m_hls->sps.colorOffsetParamChromaDc = (0.02 + ((48 - sps.colorQuantParam) >> 3) * 0.06);
  } else {
    m_hls->sps.colorOffsetParamLumaAc = 0.2;  // Quantization Shift
    m_hls->sps.colorOffsetParamChromaAc = 0.02;
    m_hls->sps.colorOffsetParamLumaDc = 0.18;  // Quantization Shift
    m_hls->sps.colorOffsetParamChromaDc = 0.02;
  }
  //  grouping
  UInt maxBB = std::max(
    {1U, sps.geomBoundingBoxSize[0], sps.geomBoundingBoxSize[1], sps.geomBoundingBoxSize[2]});
  int maxNodeSizeLog2 = ceilLog2(maxBB);
  int groupShiftBits = std::max(3, 3 * (maxNodeSizeLog2 - ((ceilLog2(voxelCount / 4) + 1) >> 1)));
  // adjust color QP per point
  int64_t minNeighborDis = 0;
  bool colorQPAdjustSliceFlag = aps.colorQPAdjustFlag;
  bool colorQPAdjustFlag = false;
  int scalar = 1;
  if (colorQPAdjustSliceFlag)
    scalar = aps.colorQPAdjustScalar;
  int colorQPAdjustDis = std::max(4, (1 << (groupShiftBits / 3 + 4)) / scalar);
  bool deadZoneChromaSliceFlag = aps.chromaDeadzoneFlag;
  bool deadZoneChromaFlag = false;
  int deadZoneChromaDis = std::max(4, (1 << (groupShiftBits / 3 + 3)) / scalar);
  vector<int> length;
  vector<int> numofGroupCount;
  getLength(pointCloudCode, length, numofGroupCount, aps.maxNumofCoeff, groupShiftBits,
            aps.colorMaxTransNum);

  int subgroupIndex = 0;
  int subGroupCount = 0;
  // obtain color predictor
  PC_COL predictorColor;
  std::vector<int> transformPointIdx;
  std::vector<colorNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  // transform and entropy coding parameter
  int64_t transformBuf[3][8] = {};
  int64_t transformPredBuf[3][8] = {};
  int dcIndex = 0;
  int acIndex = numofGroupCount[0];
  int numofCoeff = 0;
  int groupCount = 1;
  const int maxNumofCoeff = max(aps.maxNumofCoeff, aps.colorMaxTransNum);
  int* CoeffGroup = new int[maxNumofCoeff * 3]();

  int run_length = 0;
  const bool colorGolomb = aps.colorGolombNum <= 2 ? true : false;
  int lengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;

  for (int curIndex = 0; curIndex < voxelCount;) {
    subGroupCount = 0;
    while (subGroupCount < length[subgroupIndex]) {
      transformPointIdx.push_back(curIndex);
      int pointIndex = pointCloudCode[curIndex].index;
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
      predictorColor =
        getColorPredictorFarthest(curIndex, transformPointIdx[0], pointCloudRec, sps, aps,
                                  pointCloudCode, subGroupCount, neighborSet, minNeighborDis);
      for (int k = 0; k < 3; k++) {
        transformBuf[k][subGroupCount] = curColor[k];
        transformPredBuf[k][subGroupCount] = predictorColor[k];
        transformBuf[k][subGroupCount] -= transformPredBuf[k][subGroupCount];
      }
      ++subGroupCount;
      curIndex++;
    }
    if (colorQPAdjustSliceFlag)
      colorQPAdjustFlag = minNeighborDis > colorQPAdjustDis;
    if (deadZoneChromaSliceFlag)
      deadZoneChromaFlag = minNeighborDis > deadZoneChromaDis;

    colorTransformCode(pointCloudCode, transformPointIdx, dcIndex, acIndex, transformBuf,
                       transformPredBuf, colorQp, CoeffGroup, neighborSet, true, colorQPAdjustFlag,
                       deadZoneChromaFlag);

    transformPointIdx.erase(transformPointIdx.begin(), transformPointIdx.end());
    numofCoeff += length[subgroupIndex];
    subgroupIndex++;
    if (subgroupIndex == numofGroupCount[groupCount - 1]) {
      assert(numofCoeff <= maxNumofCoeff);
      runlengthEncodeMemControl(CoeffGroup, numofCoeff, run_length, lengthControl, true);
      numofCoeff = 0;
      setCoeffIndex(groupCount, numofGroupCount, length, dcIndex, acIndex);
    }
  }
  m_encBac->encodeRunlength(run_length);
  delete[] CoeffGroup;
  m_encBac->codeSliceAttrEndCode();
  if (frame_Id == frame_Count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_encBac->codeSPSEndCode();
}

void TEncAttribute::reflectancePredictAndTransformMemControl() {
  std::cout << "ReflectancePredictingAndTransformMemControl" << std::endl;
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.refReordermode, pointCloudCode, voxelCount,
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
  int subgroupIndex = 0;
  int subGroupCount = 0;
  //obtain the reflectance predictor
  int64_t predictorRefl;
  std::vector<int> transformPointIdx;
  std::vector<reflNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int prevIndex = -1;
  PC_REFL lastref = 0;
  // transform and entropy coding parameter
  int64_t transformBuf[1][8] = {};
  int64_t transformPredBuf[1][8] = {};
  const int maxNumofCoeff = max(aps.maxNumofCoeff, aps.reflMaxTransNum);
  int* CoeffGroup = new int[maxNumofCoeff]();
  int dcIndex = 0;
  int acIndex = numofGroupCount[0];
  int numofCoeff = 0;
  int groupCount = 1;

  int run_length = 0;
  int64_t value = 0;
  const bool refGolomb = aps.refGolombNum == 1 ? true : false;
  int lengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;
  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    //determine whether to use adaptive decision
    auto pointIndex = pointCloudCode[curIndex].index;
    PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
    const PC_POS& curPosition = pointCloudRec[pointIndex];
    Bool isDuplicatePoint =
      (curIndex > (length[0] - 1)) && (curPosition == pointCloudRec[prevIndex]);
    if (isDuplicatePoint) {
      predictorRefl = lastref;
      isDuplicatePoint &= multil_ID == 0;
      reflectanceCode(predictorRefl, curReflectance, run_length, value, isDuplicatePoint);
      CoeffGroup[dcIndex++] = value;
      numofCoeff += length[subgroupIndex];
      subgroupIndex++;
      prevIndex = pointIndex;
      lastref = curReflectance;
    } else {
      transformPointIdx.push_back(curIndex);
      if (aps.refGroupPredict && (length[subgroupIndex] < 3)) {
        if (subGroupCount == 0) {
          predictorRefl =
            getReflectancePredictorFarthest(curIndex, transformPointIdx[0], pointCloudRec, sps, aps,
                                            pointCloudCode, subGroupCount, neighborSet);
        }
      } else {
        predictorRefl =
          getReflectancePredictorFarthest(curIndex, transformPointIdx[0], pointCloudRec, sps, aps,
                                          pointCloudCode, subGroupCount, neighborSet);
      }
      transformBuf[0][subGroupCount] = curReflectance;
      transformPredBuf[0][subGroupCount] = predictorRefl;
      transformBuf[0][subGroupCount] -= transformPredBuf[0][subGroupCount];
      ++subGroupCount;
      if (subGroupCount == length[subgroupIndex]) {
        reflectanceTransformCodeMemControl(pointCloudCode, transformPointIdx, dcIndex, acIndex,
                                           transformBuf, transformPredBuf, CoeffGroup, neighborSet,
                                           lastref);
        subGroupCount = 0;
        transformPointIdx.erase(transformPointIdx.begin(), transformPointIdx.end());
        numofCoeff += length[subgroupIndex];
        subgroupIndex++;
        prevIndex = pointIndex;
      }
    }
    if (subgroupIndex == numofGroupCount[groupCount - 1]) {
      assert(numofCoeff <= maxNumofCoeff);
      runlengthEncodeMemControl(CoeffGroup, numofCoeff, run_length, lengthControl, false);
      numofCoeff = 0;
      setCoeffIndex(groupCount, numofGroupCount, length, dcIndex, acIndex);
    }
  }
  m_encBac->encodeRunlength(run_length);
  delete[] CoeffGroup;
  m_encBac->codeSliceAttrEndCode();
  if (frame_Id == frame_Count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_encBac->codeSPSEndCode();
}

void TEncAttribute::attributePredictAndTransformMemControl() {
  std::cout << "attributePredictAndTransformMemControl" << std::endl;
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());

  //HilbertAddr Sorting
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReordermode, pointCloudCode, voxelCount, 1);

  quantizedQP colorQp;
  colorQp.attrQuantForLuma = sps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);
  const bool colorGolomb = aps.colorGolombNum <= 2 ? true : false;
  const bool refGolomb = aps.refGolombNum == 1 ? true : false;
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

  PC_POS prePosition = pointCloudRec[pointCloudCode[0].index];
  PC_REFL preReflectance = 0;
  int distWeightGroupSize = 1 << aps.log2predDistWeightGroupSize;

  //quantization deadzone offset
  if (sps.colorQuantParam <= 48) {
    m_hls->sps.colorOffsetParamLumaAc =
      (0.2 + ((48 - sps.colorQuantParam) >> 3) * 0.01);  // Quantization Shift
    m_hls->sps.colorOffsetParamChromaAc = (0.02 + ((48 - sps.colorQuantParam) >> 3) * 0.08);
    m_hls->sps.colorOffsetParamLumaDc =
      (0.18 + ((48 - sps.colorQuantParam) >> 3) * 0.008);  // Quantization Shift
    m_hls->sps.colorOffsetParamChromaDc = (0.02 + ((48 - sps.colorQuantParam) >> 3) * 0.06);
  } else {
    m_hls->sps.colorOffsetParamLumaAc = 0.2;  // Quantization Shift
    m_hls->sps.colorOffsetParamChromaAc = 0.02;
    m_hls->sps.colorOffsetParamLumaDc = 0.18;  // Quantization Shift
    m_hls->sps.colorOffsetParamChromaDc = 0.02;
  }
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
  // adjust color QP per point
  int64_t minNeighborDis = 0;
  bool colorQPAdjustSliceFlag = aps.colorQPAdjustFlag;
  bool colorQPAdjustFlag = false;
  int scalar = 1;
  if (colorQPAdjustSliceFlag)
    scalar = aps.colorQPAdjustScalar;
  int colorQPAdjustDis = std::max(4, (1 << (groupShiftBits / 3 + 4)) / scalar);
  bool deadZoneChromaSliceFlag = aps.chromaDeadzoneFlag;
  bool deadZoneChromaFlag = false;
  int deadZoneChromaDis = std::max(4, (1 << (groupShiftBits / 3 + 3)) / scalar);
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

  int run_length_col = 0;  ///<zero count
  int run_length_refl = 0;
  int LengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;
  const int maxNumofCoeff = max(aps.maxNumofCoeff, (UInt)8);
  int* CoeffGroupColor = new int[maxNumofCoeff * 3]();
  int dcIndexColor = 0;
  int acIndexColor = numofGroupCountColor[0];
  int numofCoeffColor = 0;
  int groupCountColor = 1;
  int countNumColor = lengthColor[0];

  int* CoeffGroupRefl = new int[maxNumofCoeff]();
  int dcIndexRefl = 0;
  int acIndexRefl = numofGroupCountRefl[0];
  int numofCoeffRefl = 0;
  int groupCountRefl = 1;
  int countNumRefl = lengthRefl[0];

  // disable cross-attribute-type-prediction
  if (!isEnableCrossAttrTypePred) {
    for (int idx = 0; idx < voxelCount; ++idx) {
      auto pointIndex = pointCloudCode[idx].index;
      PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
      transformPointIdxRefl.push_back(idx);
      transformPointIdxColor.push_back(idx);
      if (aps.refGroupPredict && (countNumRefl < 3)) {
        if (countRefl == 0) {
          predictorRefl =
            getReflectancePredictorFarthest(idx, transformPointIdxRefl[0], pointCloudRec, sps, aps,
                                            pointCloudCode, countRefl, neighborSetRefl);
        }
      } else {
        predictorRefl =
          getReflectancePredictorFarthest(idx, transformPointIdxRefl[0], pointCloudRec, sps, aps,
                                          pointCloudCode, countRefl, neighborSetRefl);
      }

      transformBufRefl[0][countRefl] = curReflectance;
      transformPredBufRefl[0][countRefl] = predictorRefl;
      transformBufRefl[0][countRefl] -= transformPredBufRefl[0][countRefl];
      ++countRefl;

      if (countRefl == countNumRefl) {
        reflectanceTransformCodeMemControl(pointCloudCode, transformPointIdxRefl, dcIndexRefl,
                                           acIndexRefl, transformBufRefl, transformPredBufRefl,
                                           CoeffGroupRefl, neighborSetRefl, lastref);
        countRefl = 0;
        numofCoeffRefl += countNumRefl;
        subgroupIndexRefl++;
        if (subgroupIndexRefl < lengthRefl.size())
          countNumRefl = lengthRefl[subgroupIndexRefl];
        transformPointIdxRefl.erase(transformPointIdxRefl.begin(), transformPointIdxRefl.end());
        if (subgroupIndexRefl == numofGroupCountRefl[groupCountRefl - 1]) {
          assert(numofCoeffRefl <= maxNumofCoeff);
          runlengthEncodeMemControl(CoeffGroupRefl, numofCoeffRefl, run_length_refl, LengthControl,
                                    false);
          numofCoeffRefl = 0;
          setCoeffIndex(groupCountRefl, numofGroupCountRefl, lengthRefl, dcIndexRefl, acIndexRefl);
        }
      }

      predictorColor =
        getColorPredictorFarthest(idx, transformPointIdxColor[0], pointCloudRec, sps, aps,
                                  pointCloudCode, countColor, neighborSetColor, minNeighborDis);

      for (int k = 0; k < 3; k++) {
        transformBufColor[k][countColor] = curColor[k];
        transformPredBufColor[k][countColor] = predictorColor[k];
        transformBufColor[k][countColor] -= transformPredBufColor[k][countColor];
      }
      ++countColor;

      if (countColor == countNumColor) {
        if (colorQPAdjustSliceFlag)
          colorQPAdjustFlag = minNeighborDis > colorQPAdjustDis;
        if (deadZoneChromaSliceFlag)
          deadZoneChromaFlag = minNeighborDis > deadZoneChromaDis;
        colorTransformCode(pointCloudCode, transformPointIdxColor, dcIndexColor, acIndexColor,
                           transformBufColor, transformPredBufColor, colorQp, CoeffGroupColor,
                           neighborSetColor, true, colorQPAdjustFlag, deadZoneChromaFlag);
        transformPointIdxColor.erase(transformPointIdxColor.begin(), transformPointIdxColor.end());
        numofCoeffColor += countNumColor;
        subgroupIndexColor++;
        if (subgroupIndexColor < lengthColor.size())
          countNumColor = lengthColor[subgroupIndexColor];
        countColor = 0;
        // entropy code CoeffGroupColor
        if (subgroupIndexColor == numofGroupCountColor[groupCountColor - 1]) {
          assert(numofCoeffColor <= maxNumofCoeff);
          runlengthEncodeMemControl(CoeffGroupColor, numofCoeffColor, run_length_col, LengthControl,
                                    true);
          numofCoeffColor = 0;
          setCoeffIndex(groupCountColor, numofGroupCountColor, lengthColor, dcIndexColor,
                        acIndexColor);
        }
      }
    }
    m_encBac->encodeRunlength(run_length_col);
    m_encBacDual->encodeRunlength(run_length_refl);
  }  // predict reflectance using color
  else if (!attrEncodeOrder) {
    for (int curIndexColor = 0, curIndexRefl = 0; curIndexRefl < voxelCount;) {
      // reflectance predict
      while (curIndexColor < (curIndexRefl + countNumRefl) ||
             (countColor < countNumColor && countColor > 0)) {
        if (countColor == 0 && (curIndexColor > (curIndexRefl + countNumRefl)))
          break;
        auto pointIndex = pointCloudCode[curIndexColor].index;
        PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
        transformPointIdxColor.push_back(curIndexColor);
        predictorColor = getColorPredictorFarthest(curIndexColor, transformPointIdxColor[0],
                                                   pointCloudRec, sps, aps, pointCloudCode,
                                                   countColor, neighborSetColor, minNeighborDis);

        for (int k = 0; k < 3; k++) {
          transformBufColor[k][countColor] = curColor[k];
          transformPredBufColor[k][countColor] = predictorColor[k];
          transformBufColor[k][countColor] -= transformPredBufColor[k][countColor];
        }
        countColor++;
        if (countColor == countNumColor) {
          if (colorQPAdjustSliceFlag)
            colorQPAdjustFlag = minNeighborDis > colorQPAdjustDis;
          if (deadZoneChromaSliceFlag)
            deadZoneChromaFlag = minNeighborDis > deadZoneChromaDis;
          colorTransformCode(pointCloudCode, transformPointIdxColor, dcIndexColor, acIndexColor,
                             transformBufColor, transformPredBufColor, colorQp, CoeffGroupColor,
                             neighborSetColor, true, colorQPAdjustFlag, deadZoneChromaFlag);
          for (int i = 0; i < countNumColor; i++) {
            auto pointIndex = pointCloudCode[transformPointIdxColor[i]].index;
            colorWithCoef[pointIndex][0] =
              pointCloudRec.getColor(pointIndex, multil_ID)[0] * crossAttrTypeCoef;
            colorWithCoef[pointIndex][1] =
              pointCloudRec.getColor(pointIndex, multil_ID)[1] * crossAttrTypeCoef;
            colorWithCoef[pointIndex][2] =
              pointCloudRec.getColor(pointIndex, multil_ID)[2] * crossAttrTypeCoef;
          }
          numofCoeffColor += countNumColor;
          subgroupIndexColor++;
          countColor = 0;
          if (subgroupIndexColor < lengthColor.size())
            countNumColor = lengthColor[subgroupIndexColor];
          transformPointIdxColor.erase(transformPointIdxColor.begin(),
                                       transformPointIdxColor.end());
          if (subgroupIndexColor == numofGroupCountColor[groupCountColor - 1]) {
            assert(numofCoeffColor <= maxNumofCoeff);
            runlengthEncodeMemControl(CoeffGroupColor, numofCoeffColor, run_length_col,
                                      LengthControl, true);
            numofCoeffColor = 0;
            setCoeffIndex(groupCountColor, numofGroupCountColor, lengthColor, dcIndexColor,
                          acIndexColor);
          }
        }
        countRefl++;
        curIndexColor++;
      }
      // color predict
      countRefl = 0;
      while (countRefl < countNumRefl) {
        auto pointIndex = pointCloudCode[curIndexRefl].index;
        transformPointIdxRefl.push_back(curIndexRefl);
        PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
        if (aps.refGroupPredict && (countNumRefl < 3)) {
          if (countRefl == 0) {
            predictorRefl = getReflectancePredictorFromColor(
              curIndexRefl, transformPointIdxRefl[0], pointCloudRec, sps, aps, pointCloudCode,
              countRefl, neighborSetRefl, colorWithCoefNeighborSet, colorWithCoef[pointIndex],
              reflectanceDistWeight);
          }
        } else {
          predictorRefl = getReflectancePredictorFromColor(
            curIndexRefl, transformPointIdxRefl[0], pointCloudRec, sps, aps, pointCloudCode,
            countRefl, neighborSetRefl, colorWithCoefNeighborSet, colorWithCoef[pointIndex],
            reflectanceDistWeight);
        }
        transformBufRefl[0][countRefl] = curReflectance;
        transformPredBufRefl[0][countRefl] = predictorRefl;
        transformBufRefl[0][countRefl] -= transformPredBufRefl[0][countRefl];
        countRefl++;
        curIndexRefl++;
      }
      // transform and save coefficients in CoeffGroupColor
      reflectanceTransformCodeMemControl(pointCloudCode, transformPointIdxRefl, dcIndexRefl,
                                         acIndexRefl, transformBufRefl, transformPredBufRefl,
                                         CoeffGroupRefl, neighborSetRefl, lastref);
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
          PC_REFL curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
          calculateReflTrend(pointCloudRec[pointIndex], prePosition, curReflectance, preReflectance,
                             reflectanceDistCoef, reflectanceRes, reflResNum, reflectanceDistWeight,
                             distWeightGroupSize);
          preReflectance = curReflectance;
          prePosition = pointCloudRec[pointIndex];
        }
      }

      transformPointIdxRefl.erase(transformPointIdxRefl.begin(), transformPointIdxRefl.end());
      numofCoeffRefl += countNumRefl;
      subgroupIndexRefl++;
      if (subgroupIndexRefl < lengthRefl.size())
        countNumRefl = lengthRefl[subgroupIndexRefl];
      // entropy code CoeffGroupColor
      if (subgroupIndexRefl == numofGroupCountRefl[groupCountRefl - 1]) {
        assert(numofCoeffColor <= maxNumofCoeff);
        runlengthEncodeMemControl(CoeffGroupRefl, numofCoeffRefl, run_length_refl, LengthControl,
                                  false);
        numofCoeffRefl = 0;
        setCoeffIndex(groupCountRefl, numofGroupCountRefl, lengthRefl, dcIndexRefl, acIndexRefl);
      }
    }
    m_encBac->encodeRunlength(run_length_col);
    m_encBacDual->encodeRunlength(run_length_refl);
  }  // predict color using reflectance CTC default
  else {
    for (int curIndexColor = 0, curIndexRefl = 0; curIndexColor < voxelCount;) {
      // reflectance predict
      while (curIndexRefl < (curIndexColor + countNumColor) ||
             (countRefl < countNumRefl && countRefl > 0)) {
        if (countRefl == 0 && (curIndexRefl > (curIndexColor + countNumColor)))
          break;
        auto pointIndex = pointCloudCode[curIndexRefl].index;
        PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
        transformPointIdxRefl.push_back(curIndexRefl);
        if (aps.refGroupPredict && (countNumRefl < 3)) {
          if (countRefl == 0) {
            if (aps.log2predDistWeightGroupSize)
              predictorRefl = getReflectancePredictorFarthest(
                curIndexRefl, transformPointIdxRefl[0], pointCloudRec, sps, aps, pointCloudCode,
                countRefl, neighborSetRefl, reflectanceDistWeight);
            else
              predictorRefl = getReflectancePredictorFarthest(
                curIndexRefl, transformPointIdxRefl[0], pointCloudRec, sps, aps, pointCloudCode,
                countRefl, neighborSetRefl);
          }
        } else {
          if (aps.log2predDistWeightGroupSize)
            predictorRefl = getReflectancePredictorFarthest(
              curIndexRefl, transformPointIdxRefl[0], pointCloudRec, sps, aps, pointCloudCode,
              countRefl, neighborSetRefl, reflectanceDistWeight);
          else
            predictorRefl =
              getReflectancePredictorFarthest(curIndexRefl, transformPointIdxRefl[0], pointCloudRec,
                                              sps, aps, pointCloudCode, countRefl, neighborSetRefl);
        }

        transformBufRefl[0][countRefl] = curReflectance;
        transformPredBufRefl[0][countRefl] = predictorRefl;
        transformBufRefl[0][countRefl] -= transformPredBufRefl[0][countRefl];
        countRefl++;

        if (countRefl == countNumRefl) {
          reflectanceTransformCodeMemControl(pointCloudCode, transformPointIdxRefl, dcIndexRefl,
                                             acIndexRefl, transformBufRefl, transformPredBufRefl,
                                             CoeffGroupRefl, neighborSetRefl, lastref);
          for (int i = 0; i < countNumRefl; i++) {
            auto pointIndex = pointCloudCode[transformPointIdxRefl[i]].index;
            PC_REFL curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
            reflWithCoef[pointIndex] = curReflectance * crossAttrTypeCoef;
            calculateReflTrend(pointCloudRec[pointIndex], prePosition, curReflectance,
                               preReflectance, reflectanceDistCoef, reflectanceRes, reflResNum,
                               reflectanceDistWeight, distWeightGroupSize);
            preReflectance = curReflectance;
            prePosition = pointCloudRec[pointIndex];
          }
          countRefl = 0;
          numofCoeffRefl += countNumRefl;
          subgroupIndexRefl++;
          if (subgroupIndexRefl < lengthRefl.size())
            countNumRefl = lengthRefl[subgroupIndexRefl];
          transformPointIdxRefl.erase(transformPointIdxRefl.begin(), transformPointIdxRefl.end());
          if (subgroupIndexRefl == numofGroupCountRefl[groupCountRefl - 1]) {
            assert(numofCoeffRefl <= maxNumofCoeff);
            runlengthEncodeMemControl(CoeffGroupRefl, numofCoeffRefl, run_length_refl,
                                      LengthControl, false);
            numofCoeffRefl = 0;
            setCoeffIndex(groupCountRefl, numofGroupCountRefl, lengthRefl, dcIndexRefl,
                          acIndexRefl);
          }
        }
        countColor++;
        curIndexRefl++;
      }
      // color predict
      countColor = 0;
      while (countColor < countNumColor) {
        transformPointIdxColor.push_back(curIndexColor);
        auto pointIndex = pointCloudCode[curIndexColor].index;
        PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
        predictorColor = getColorPredictorFromReflectance(
          curIndexColor, transformPointIdxColor[0], pointCloudRec, sps, aps, pointCloudCode,
          countColor, neighborSetColor, reflWithCoefNeighborSet, reflWithCoef[pointIndex],
          minNeighborDis);
        for (int k = 0; k < 3; k++) {
          transformBufColor[k][countColor] = curColor[k];
          transformPredBufColor[k][countColor] = predictorColor[k];
          transformBufColor[k][countColor] -= transformPredBufColor[k][countColor];
        }
        countColor++;
        curIndexColor++;
      }
      // transform and save coefficients in CoeffGroupColor
      if (colorQPAdjustSliceFlag)
        colorQPAdjustFlag = minNeighborDis > colorQPAdjustDis;
      if (deadZoneChromaSliceFlag)
        deadZoneChromaFlag = minNeighborDis > deadZoneChromaDis;
      colorTransformCode(pointCloudCode, transformPointIdxColor, dcIndexColor, acIndexColor,
                         transformBufColor, transformPredBufColor, colorQp, CoeffGroupColor,
                         neighborSetColor, true, colorQPAdjustFlag, deadZoneChromaFlag);
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
      numofCoeffColor += countNumColor;
      subgroupIndexColor++;
      if (subgroupIndexColor < lengthColor.size())
        countNumColor = lengthColor[subgroupIndexColor];
      // entropy code CoeffGroupColor
      if (subgroupIndexColor == numofGroupCountColor[groupCountColor - 1]) {
        assert(numofCoeffColor <= maxNumofCoeff);
        runlengthEncodeMemControl(CoeffGroupColor, numofCoeffColor, run_length_col, LengthControl,
                                  true);
        numofCoeffColor = 0;
        setCoeffIndex(groupCountColor, numofGroupCountColor, lengthColor, dcIndexColor,
                      acIndexColor);
      }
    }
    //RunlengthEncoder
    m_encBac->encodeRunlength(run_length_col);
    m_encBacDual->encodeRunlength(run_length_refl);
  }
  delete[] CoeffGroupColor;
  delete[] CoeffGroupRefl;

  m_encBac->codeSliceAttrEndCode();
  if (frame_Id == frame_Count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_encBac->codeSPSEndCode();
}

void TEncAttribute::reflectancePredictCode(const int64_t& predictor, PC_REFL& currValue,
                                           int& run_length, const bool isDuplicatePoint) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const bool isGolomb = aps.refGolombNum == 1 ? true : false;
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (aps.reflOutputDepth)) - 1;
  }
  int64_t delta = currValue - predictor;
  if (isDuplicatePoint && (delta < 0)) {
    delta = 0;
  }
  UInt golombNum = aps.refGolombNum;
  // Residual quantization
  int residualSign = delta < 0 ? -1 : 1;
  uint64_t absResidual = std::abs(delta);
  uint64_t residualQuant;
  residualQuant = QuantizaResidual(absResidual, sps.reflQuantParam + abh.reflQPoffset, 1);
  int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
  // Entropy encode
  if (signResidualQuant == 0)
    ++run_length;
  else {
    if (m_encBacDual != NULL) {
      m_encBacDual->encodeRunlength(run_length);
      ///<if used forceAttribute predict
      run_length = 0;
      m_encBacDual->codeAttributeResidualDual(signResidualQuant, false, 0, isDuplicatePoint, false,
                                              golombNum);
    } else {
      m_encBac->encodeRunlength(run_length);
      ///<if used forceAttribute predict
      run_length = 0;
      m_encBac->codeAttributerResidual(signResidualQuant, false, 3, 0, isDuplicatePoint, false,
                                       golombNum);
    }
  }
  // lengthControl
  if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
    if (m_encBacDual != NULL) {
      m_encBacDual->encodeRunlength(run_length);
    } else {
      m_encBac->encodeRunlength(run_length);
    }
    run_length = 0;
  }
  // Local decode
  uint64_t inverseResidualQuant =
    InverseQuantizeResidual(residualQuant, sps.reflQuantParam + abh.reflQPoffset);
  int64_t recResidual = inverseResidualQuant * residualSign;
  currValue = TComClip((Int64)ClipMin, (Int64)ClipMax, recResidual + predictor);
}

void TEncAttribute::colorPredictCode(const PC_COL& predictor, PC_COL& currValue,
                                     const quantizedQP& colorQp, int& run_length,
                                     const bool isDuplicatePoint) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const bool isGolomb = aps.colorGolombNum == 1 ? true : false;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;
  int residualPrevComponent = 0;
  bool ccp = aps.crossComponentPred;
  bool os = aps.orderSwitch;
  UInt golombNum = aps.colorGolombNum;

  UInt colorQuantParam;
  V3<int64_t> values;
  int64_t signResidualQuantvalue[3];
  for (int i = 0; i < 3; i++) {
    int64_t delta = currValue[i] - predictor[i];

    if ((i == 0) && isDuplicatePoint && (delta < 0)) {
      delta = 0;
    }
    delta -= residualPrevComponent;
    colorQuantParam = (i == 0)
      ? colorQp.attrQuantForLuma
      : ((i == 1) ? colorQp.attrQuantForChromaCb : colorQp.attrQuantForChromaCr);
    // Residual quantization
    int residualSign = delta < 0 ? -1 : 1;
    uint64_t absResidual = std::abs(delta);
    uint64_t residualQuant;
    residualQuant = QuantizaResidual(absResidual, colorQuantParam, 1);
    int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
    signResidualQuantvalue[i] = signResidualQuant;
    values[i] = signResidualQuant;
    // Local decode
    uint64_t inverseResidualQuant = InverseQuantizeResidual(residualQuant, colorQuantParam);
    int64_t recResidual = inverseResidualQuant * residualSign;
    currValue[i] =
      TComClip((Int64)ClipMin, (Int64)ClipMax, recResidual + predictor[i] + residualPrevComponent);
    if (ccp && i <= 1) {
      residualPrevComponent = (Int)currValue[i] - (Int)predictor[i];
    }
  }
  // Entropy encode
  if (!values[0] && !values[1] && !values[2])
    ++run_length;
  else {
    ///<encode zero_cnt
    m_encBac->encodeRunlength(run_length);
    int countOfZeros = 0;
    run_length = 0;
    ///<attribute correlation coding
    if (!os) {
      colorResidualCorrelationCode(signResidualQuantvalue, isDuplicatePoint, golombNum);
    } else {
      colorResidualCorrelationCodeOS(signResidualQuantvalue, isDuplicatePoint, golombNum);
    }
  }
  // lengthControl
  if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
    m_encBac->encodeRunlength(run_length);
    run_length = 0;
  }
}

void TEncAttribute::colorResidualCorrelationCode(const int64_t signResidualQuantvalue[3],
                                                 const bool isDuplicatePoint,
                                                 const UInt& golombNum) {
  int flagyu_rg = 0;
  int flagy_r = 0;
  if (signResidualQuantvalue[0] == 0 && signResidualQuantvalue[1] == 0) {
    flagyu_rg = 0;
  } else {
    flagyu_rg = 1;
  }
  if (signResidualQuantvalue[0] == 0) {
    flagy_r = 0;
  } else {
    flagy_r = 1;
  }
  //code Y/R
  m_encBac->codeAttributerResidualequalone0(flagy_r);
  bool iscolor = true;
  bool residualminusone = true;
  if (signResidualQuantvalue[0] == 0) {
    //code U/G
    m_encBac->codeAttributerResidualequaltwo0(flagyu_rg);
    if (signResidualQuantvalue[1] == 0) {
      m_encBac->codeAttributerResidual(signResidualQuantvalue[2], iscolor, 2, 0,
                                       2 == 0 && isDuplicatePoint, residualminusone, golombNum);
      m_encBac->codeSign(signResidualQuantvalue[2]);
    } else {
      m_encBac->codeAttributerResidual(signResidualQuantvalue[1], iscolor, 1, 2,
                                       1 == 0 && isDuplicatePoint, residualminusone, golombNum);
      m_encBac->codeAttributerResidual(signResidualQuantvalue[2], iscolor, 2, 0,
                                       2 == 0 && isDuplicatePoint, !residualminusone, golombNum);
      m_encBac->codeSign(signResidualQuantvalue[1]);
      if (signResidualQuantvalue[2] != 0) {
        m_encBac->codeSign(signResidualQuantvalue[2]);
      }
    }

  } else {
    m_encBac->codeAttributerResidual(signResidualQuantvalue[0], iscolor, 0, 1,
                                     0 == 0 && isDuplicatePoint, residualminusone, golombNum);
    m_encBac->codeAttributerResidual(signResidualQuantvalue[1], iscolor, 1, 1,
                                     1 == 0 && isDuplicatePoint, !residualminusone, golombNum);

    int b0 = 0;
    if (abs(signResidualQuantvalue[0]) > abs(signResidualQuantvalue[1])) {
      b0 = 1;
    } else {
      b0 = 0;
    }
    m_encBac->codeAttributerResidual(signResidualQuantvalue[2], iscolor, 2, 2,
                                     2 == 0 && isDuplicatePoint, !residualminusone, golombNum, b0);
    m_encBac->codeSign(signResidualQuantvalue[0]);
    if (signResidualQuantvalue[1] != 0) {
      m_encBac->codeSign(signResidualQuantvalue[1]);
    }
    if (signResidualQuantvalue[2] != 0) {
      m_encBac->codeSign(signResidualQuantvalue[2]);
    }
  }
}

void TEncAttribute::colorTransformCode(
  std::vector<pointCodeWithIndex>& pointCloudCode, std::vector<int>& transformPointIdx,
  int& dcIndex, int& acIndex, int64_t transformBuf[][8], int64_t transformPredBuf[][8],
  const quantizedQP& colorQp, int* Coefficients, std::vector<colorNeighborSet>& neighborSet,
  const bool isLengthControl, bool colorQPAdjustFlag, bool deadZoneChromaFlag) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;
  int count = transformPointIdx.size();
  int offsetShift = 0.2 * (1 << encoderShiftBit);  // Quantization Shift

  int offsetShiftLuma1 =
    sps.colorOffsetParamLumaAc * (1 << encoderShiftBit);  // Quantization Shift Luma AC
  int offsetShiftChroma1 =
    sps.colorOffsetParamChromaAc * (1 << encoderShiftBit);  // Quantization Shift Chroma AC

  int offsetShiftLuma2 =
    sps.colorOffsetParamLumaDc * (1 << encoderShiftBit);  // Quantization Shift Luma DC
  int offsetShiftChroma2 =
    sps.colorOffsetParamChromaDc * (1 << encoderShiftBit);  // Quantization Shift Chroma DC

  if (deadZoneChromaFlag) {
    offsetShiftChroma2 = 0.1 * (1 << encoderShiftBit);
  }
  int resCodeQPOffset = 8;
  UInt colorQuantParam;
  V3<int64_t> Color;
  V3<int64_t> reconstructedColor;
  int transform_shift = 9;
  Int64 add = 1 << (transform_shift * 2 - 1);
  int transform_shift_QP = transform_shift * 8;
  Transform(transformBuf, count, 3);
  for (int curIndex = 0; curIndex < count; curIndex++) {
    for (int k = 0; k < 3; k++) {
      int64_t delta = transformBuf[k][curIndex];
      if (curIndex == 0) {
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
      int residualSign = delta < 0 ? -1 : 1;
      uint64_t absResidual = std::abs(delta);
      uint64_t residualQuant;
      int64_t signResidualQuant;
      if (curIndex == 0) {
        if (k == 0)
          residualQuant = QuantizaResidual(absResidual, colorQuantParam, offsetShiftLuma2);
        else
          residualQuant = QuantizaResidual(absResidual, colorQuantParam, offsetShiftChroma2);
      } else {
        if (k == 0)
          residualQuant = QuantizaResidual(absResidual, colorQuantParam, offsetShiftLuma1);
        else
          residualQuant = QuantizaResidual(absResidual, colorQuantParam, offsetShiftChroma1);
      }
      signResidualQuant = residualSign * (int64_t)residualQuant;
      if (curIndex == 0)
        Coefficients[dcIndex * 3 + k] = signResidualQuant;
      else
        Coefficients[acIndex * 3 + k] = signResidualQuant;
      // Local decode
      uint64_t inverseResidualQuant = InverseQuantizeResidual(residualQuant, colorQuantParam);
      int64_t recResidual = inverseResidualQuant * residualSign;
      transformBuf[k][curIndex] = recResidual;
    }
    if (curIndex == 0) {
      ++dcIndex;
    } else {
      if (isLengthControl)
        ++acIndex;
      else
        --acIndex;
    }
  }
  int setlength = neighborSet.size();
  invTransform(transformBuf, count, 3);
  for (int i = 0; i < count; ++i) {
    int pointIndex = pointCloudCode[transformPointIdx[i]].index;
    PC_COL& color = pointCloudRec.getColor(pointIndex, multil_ID);
    for (int k = 0; k < 3; ++k) {
      transformBuf[k][i] = (transformBuf[k][i] + add) >> transform_shift * 2;
      transformBuf[k][i] += transformPredBuf[k][i];
      transformBuf[k][i] = TComClip((Int64)ClipMin, (Int64)ClipMax, transformBuf[k][i]);
      color[k] = transformBuf[k][i];
    }
    if (transformPointIdx[0] <= setlength) {
      neighborSet[transformPointIdx[i] % setlength].pos = pointCloudRec[pointIndex];
      neighborSet[transformPointIdx[i] % setlength].color = color;
    } else {
      neighborSet[i].pos = pointCloudRec[pointIndex];
      neighborSet[i].color = color;
    }
  }
}

void TEncAttribute::reflectanceCode(const int64_t& predictor, PC_REFL& currValue, int& run_length,
                                    int64_t& value, const bool isDuplicatePoint) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const bool refGolomb = aps.refGolombNum == 1 ? true : false;
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (m_hls->aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (m_hls->aps.reflOutputDepth)) - 1;
  }
  int64_t delta = currValue - predictor;
  if (isDuplicatePoint && (delta < 0)) {
    delta = 0;
  }
  // Residual quantization
  int residualSign = delta < 0 ? -1 : 1;
  uint64_t absResidual = std::abs(delta);
  uint64_t residualQuant;
  residualQuant = QuantizaResidual(absResidual, sps.reflQuantParam + abh.reflQPoffset, 1);
  int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
  value = signResidualQuant;
  if (aps.attributePresentFlag[0]) {
    // Entropy encode
    if (signResidualQuant == 0)
      ++run_length;
    else {
      if (m_encBacDual != NULL) {
        m_encBacDual->encodeRunlength(run_length);
        run_length = 0;
        m_encBacDual->codeAttributeResidualDual(signResidualQuant, false, 0, isDuplicatePoint,
                                                false, aps.refGolombNum);
      } else {
        m_encBac->encodeRunlength(run_length);
        run_length = 0;
        m_encBac->codeAttributerResidual(signResidualQuant, false, 3, 0, isDuplicatePoint, false,
                                         aps.refGolombNum);
      }
    }
  }
  // Local decode
  uint64_t inverseResidualQuant =
    InverseQuantizeResidual(residualQuant, sps.reflQuantParam + abh.reflQPoffset);
  int64_t recResidual = inverseResidualQuant * residualSign;
  currValue = TComClip((Int64)ClipMin, (Int64)ClipMax, recResidual + predictor);
}

void TEncAttribute::reflectanceTransformCodeMemControl(
  std::vector<pointCodeWithIndex>& pointCloudCode, std::vector<int>& transformPointIdx,
  int& dcIndex, int& acIndex, int64_t transformBuf[1][8], int64_t transformPredBuf[1][8],
  int* Coefficients, std::vector<reflNeighborSet>& neighborSet, PC_REFL& lastref) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (m_hls->aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (m_hls->aps.reflOutputDepth)) - 1;
  }
  UInt transQuantParam;
  int count = transformPointIdx.size();
  int offsetShift = (count == 1) ? 1 : 0.2 * (int64_t(1) << encoderShiftBit);  // Quantization Shift
  int transform_shift = 9;
  Int64 add = 1 << (transform_shift * 2 - 1);
  Transform(transformBuf, count, 1);
  int num = std::max(1, count);
  for (int idx = 0; idx < num; ++idx) {
    int64_t delta = transformBuf[0][idx];
    // quant
    if (idx == 0)
      transQuantParam = sps.reflQuantParam + abh.reflQPoffset + 72 + aps.QpOffsetDC;
    else
      transQuantParam = sps.reflQuantParam + abh.reflQPoffset + 72 + aps.QpOffsetAC;
    int residualSign = delta < 0 ? -1 : 1;
    uint64_t absResidual = std::abs(delta);
    uint64_t residualQuant = QuantizaResidual(absResidual, transQuantParam, offsetShift);
    int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
    if (idx == 0)
      Coefficients[dcIndex++] = signResidualQuant;
    else
      Coefficients[acIndex++] = signResidualQuant;
    uint64_t inverseResidualQuant = InverseQuantizeResidual(residualQuant, transQuantParam);
    int64_t recResidual = inverseResidualQuant * residualSign;
    transformBuf[0][idx] = recResidual;
  }
  int setlength = neighborSet.size();
  invTransform(transformBuf, count, 1);
  for (int idx = 0; idx < count; ++idx) {
    auto pointIndex = pointCloudCode[transformPointIdx[idx]].index;
    PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
    transformBuf[0][idx] = (transformBuf[0][idx] + add) >> transform_shift * 2;
    transformBuf[0][idx] += transformPredBuf[0][idx];
    transformBuf[0][idx] = (PC_REFL)TComClip((Int64)ClipMin, (Int64)ClipMax, transformBuf[0][idx]);
    curReflectance = transformBuf[0][idx];
    if (transformPointIdx[0] <= setlength) {
      neighborSet[transformPointIdx[idx] % setlength].pos = pointCloudRec[pointIndex];
      neighborSet[transformPointIdx[idx] % setlength].refl = curReflectance;
    } else {
      neighborSet[idx].pos = pointCloudRec[pointIndex];
      neighborSet[idx].refl = curReflectance;
    }
  }
  lastref = transformBuf[0][count - 1];
}

void TEncAttribute::setCoeffIndex(int& groupCount, const vector<int>& numofGroupCount,
                                  const vector<int>& length, int& dcIndex, int& acIndex) {
  if (groupCount < numofGroupCount.size()) {
    if (groupCount % 2) {
      int sumofGroup = numofGroupCount[groupCount] - numofGroupCount[groupCount - 1];
      int sumofCoeff = accumulate(length.begin() + numofGroupCount[groupCount - 1],
                              length.begin() + numofGroupCount[groupCount], 0);
      dcIndex = sumofCoeff - sumofGroup;
      acIndex = 0;
    } else {
      dcIndex = 0;
      acIndex = numofGroupCount[groupCount] - numofGroupCount[groupCount - 1];
    }
    groupCount++;
  }
}

void TEncAttribute::runlengthEncodeMemControl(int* Coefficients, int pointCount, int& run_length,
                                              int lengthControl, const bool isColor) {
  const AttributeParameterSet& aps = m_hls->aps;
  if (isColor) {
    auto colorGolombNum = aps.colorGolombNum;
    const bool colorGolomb = colorGolombNum <= 2 ? true : false;
    bool os = aps.orderSwitch;
    int64_t values[3];
    for (int n = 0; n < pointCount; ++n) {
      for (int d = 0; d < 3; ++d) {
        values[d] = Coefficients[3 * n + d];
      }
      if (!values[0] && !values[1] && !values[2])
        ++run_length;
      else {
        m_encBac->encodeRunlength(run_length);
        if (!os) {
          colorResidualCorrelationCode(values, false, colorGolombNum);
        } else {
          colorResidualCorrelationCodeOS(values, false, colorGolombNum);
        }
        run_length = 0;
      }
      if ((aps.coeffLengthControl > 0) && (run_length == lengthControl)) {
        m_encBac->encodeRunlength(run_length);
        run_length = 0;
      }
    }
  } else {
    auto refGolombNum = aps.refGolombNum;
    const bool refGolomb = refGolombNum == 1 ? true : false;
    PC_REFL values;
    for (int n = 0; n < pointCount; ++n) {
      values = Coefficients[n];
      if (!values)
        ++run_length;
      else {
        if (m_encBacDual != NULL) {
          m_encBacDual->encodeRunlength(run_length);
          m_encBacDual->codeAttributerResidual(values, false, 3, 0, false, false, refGolombNum);
          run_length = 0;
        } else {
          m_encBac->encodeRunlength(run_length);
          m_encBac->codeAttributerResidual(values, false, 3, 0, false, false, refGolombNum);
          run_length = 0;
        }
      }
      if ((aps.coeffLengthControl > 0) && (run_length == lengthControl)) {
        if (m_encBacDual != NULL) {
          m_encBacDual->encodeRunlength(run_length);
          run_length = 0;
        } else {
          m_encBac->encodeRunlength(run_length);
          run_length = 0;
        }
      }
    }
  }
}

void TEncAttribute::colorResidualCorrelationCodeOS(const int64_t signResidualQuantvalue[3],
                                                   const bool isDuplicatePoint,
                                                   const UInt& golombNum) {
  int flaguy_gr = 0;
  int flagu_g = 0;
  if (signResidualQuantvalue[1] == 0 && signResidualQuantvalue[0] == 0) {
    flaguy_gr = 0;
  } else {
    flaguy_gr = 1;
  }
  if (signResidualQuantvalue[1] == 0) {
    flagu_g = 0;
  } else {
    flagu_g = 1;
  }
  //code U/G
  m_encBac->codeAttributerResidualequalone0(flagu_g);
  bool iscolor = true;
  bool residualminusone = true;
  if (signResidualQuantvalue[1] == 0) {
    //code Y/R
    m_encBac->codeAttributerResidualequaltwo0(flaguy_gr);
    if (signResidualQuantvalue[0] == 0) {
      m_encBac->codeAttributerResidual(signResidualQuantvalue[2], iscolor, 2, 0,
                                       2 == 0 && isDuplicatePoint, residualminusone, golombNum);
      m_encBac->codeSign(signResidualQuantvalue[2]);
    } else {
      m_encBac->codeAttributerResidual(signResidualQuantvalue[0], iscolor, 0, 2,
                                       0 == 0 && isDuplicatePoint, residualminusone, golombNum);
      m_encBac->codeAttributerResidual(signResidualQuantvalue[2], iscolor, 2, 0,
                                       2 == 0 && isDuplicatePoint, !residualminusone, golombNum);
      m_encBac->codeSign(signResidualQuantvalue[0]);
      if (signResidualQuantvalue[2] != 0) {
        m_encBac->codeSign(signResidualQuantvalue[2]);
      }
    }

  } else {
    m_encBac->codeAttributerResidual(signResidualQuantvalue[1], iscolor, 1, 1,
                                     1 == 0 && isDuplicatePoint, residualminusone, golombNum);
    m_encBac->codeAttributerResidual(signResidualQuantvalue[0], iscolor, 0, 1,
                                     0 == 0 && isDuplicatePoint, !residualminusone, golombNum);

    int b0 = 0;
    if (abs(signResidualQuantvalue[1]) > abs(signResidualQuantvalue[0])) {
      b0 = 1;
    } else {
      b0 = 0;
    }
    m_encBac->codeAttributerResidual(signResidualQuantvalue[2], iscolor, 2, 2,
                                     2 == 0 && isDuplicatePoint, !residualminusone, golombNum, b0);
    m_encBac->codeSign(signResidualQuantvalue[1]);
    if (signResidualQuantvalue[0] != 0) {
      m_encBac->codeSign(signResidualQuantvalue[0]);
    }
    if (signResidualQuantvalue[2] != 0) {
      m_encBac->codeSign(signResidualQuantvalue[2]);
    }
  }
}

void TEncAttribute::colorWaveletTransform() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameHead = m_hls->frameHead;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  int voxelCount = int(pointCloudRec.getNumPoint());
  const bool isGolomb = aps.colorGolombNum <= 2 ? true : false;
  bool os = aps.orderSwitch;
  cout << "colorWaveletTransform" << endl;
  // Allocate arrays.
  int attribCount = 3;
  FXPoint* attributesBuf =
    new FXPoint[voxelCount * attribCount];  // attributes of the whole point clouds
  int* integerizedAttributesBuf =
    new int[voxelCount *
            attribCount];  // transform coefficients / attribute residues of the whole point clouds
  int* recAttributesBuf =
    new int[voxelCount * attribCount]();  // reconstructed attributes of the whole point clouds
  int* positionBuf = new int[voxelCount * 3]();  // position coordinates of the whole point clouds
  bool resLayer = aps.transResLayer;
  int resLayerQuantParam = sps.colorQuantParam;
  int preColor[3];
  int64_t values[3];
  PC_COL recColor;
  PC_COL oriColor;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;

  //Hilbert Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReordermode, pointCloudCode, voxelCount, 1);
  for (int n = 0; n < voxelCount; n++) {
    const auto color = pointCloudRec.getColor(pointCloudCode[n].index, multil_ID);
    attributesBuf[n * attribCount] = FXPoint(color[0]);
    attributesBuf[n * attribCount + 1] = FXPoint(color[1]);
    attributesBuf[n * attribCount + 2] = FXPoint(color[2]);
    const auto& point = pointCloudRec[pointCloudCode[n].index];
    positionBuf[3 * n] = point[0];
    positionBuf[3 * n + 1] = point[1];
    positionBuf[3 * n + 2] = point[2];
  }
  // Transform.
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
    int* recAttributes = recAttributesBuf + segmentStartPosition * attribCount;
    int* position = positionBuf + segmentStartPosition * 3;
    int segmentVoxelCount = segmentLen;
    if (segIndex == (numSegments - 1)) {
      segmentVoxelCount = voxelCount - (numSegments - 1) * segmentLen;
    }
    // Transform.

    WaveletCoreTransform(attributes, 3, segmentVoxelCount, integerizedAttributes, sps, aps,abh,
                         position, recAttributes, disThInit);
    // Entropy encode.
    int run_length = 0;  //segmentVoxelCount
    for (int n = 0; n < segmentVoxelCount; ++n) {
      for (int kk = 0; kk < attribCount; kk++) {
        values[kk] = static_cast<int64_t>(integerizedAttributes[n * attribCount + kk]);
      }
      if (!values[0] && !values[1] && !values[2])
        ++run_length;
      else {
        m_encBac->encodeRunlength(run_length);
        if (!os) {
          colorResidualCorrelationCode(values, false, aps.colorGolombNum);
        } else {
          colorResidualCorrelationCodeOS(values, false, aps.colorGolombNum);
        }
        run_length = 0;
      }
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        m_encBac->encodeRunlength(run_length);
        run_length = 0;
      }
    }
    m_encBac->encodeRunlength(run_length);

    // Add residues for limitedlosy and lossless attribute compression
    if (resLayer) {
      cout << "resLayer Needed!" << endl;
      for (int n = 0; n < segmentVoxelCount; n++) {
        oriColor = pointCloudRec.getColor(pointCloudCode[n].index, multil_ID);
        for (int kk = 0; kk < attribCount; kk++) {
          integerizedAttributes[n * attribCount + kk] =
            oriColor[kk] - recAttributes[n * attribCount + kk];
        }
      }
      int run_length = 0;
      for (int n = 0; n < segmentVoxelCount; ++n) {
        for (int kk = 0; kk < attribCount; kk++) {
          int64_t detail = static_cast<int64_t>(integerizedAttributes[n * attribCount + kk]);
          int residualSign = detail < 0 ? -1 : 1;
          uint64_t absResidual = std::abs(detail);
          uint64_t residualQuant = QuantizaResidual(absResidual, resLayerQuantParam, 1);
          int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
          values[kk] = signResidualQuant;
          integerizedAttributes[n * attribCount + kk] =
            residualSign * InverseQuantizeResidual(residualQuant, resLayerQuantParam);
        }
        if (!values[0] && !values[1] && !values[2])
          ++run_length;
        else {
          m_encBac->encodeRunlength(run_length);
          if (!os) {
            colorResidualCorrelationCode(values, false, aps.colorGolombNum);
          } else {
            colorResidualCorrelationCodeOS(values, false, aps.colorGolombNum);
          }
          run_length = 0;
        }
        if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
          m_encBac->encodeRunlength(run_length);
          run_length = 0;
        }
      }
      m_encBac->encodeRunlength(run_length);
    }
    // Reconstruction process
    for (int n = 0; n < segmentVoxelCount; n++) {
      for (int kk = 0; kk < attribCount; kk++) {
        preColor[kk] = recAttributes[n * attribCount + kk];
        if (resLayer)
          preColor[kk] += integerizedAttributes[n * attribCount + kk];
      }
      for (int kk = 0; kk < attribCount; kk++) {
        recColor[kk] = (Int16)TComClip((Int64)ClipMin, (Int64)ClipMax, (Int64)preColor[kk]);
      }
      pointCloudRec.setColor(pointCloudCode[n + segmentStartPosition].index, recColor, multil_ID);
    }
  }
  // De-allocate arrays.
  delete[] attributesBuf;
  delete[] integerizedAttributesBuf;
  delete[] positionBuf;
  delete[] recAttributesBuf;

  m_encBac->codeSliceAttrEndCode();
  if (frame_Id == frame_Count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_encBac->codeSPSEndCode();
}

void TEncAttribute::reflectanceWaveletTransform() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameHead = m_hls->frameHead;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  int voxelCount = int(pointCloudRec.getNumPoint());
  cout << "reflectanceWaveletTransform" << endl;
  // Allocate arrays.
  FXPoint* attributesBuf = new FXPoint[voxelCount];  // attributes of the whole point clouds
  int64_t* integerizedAttributesBuf = new int64_t
    [voxelCount];  // transform coefficients / attribute residues of the whole point clouds
  int* recAttributesBuf =
    new int[voxelCount]();  // reconstructed attributes of the whole point clouds
  int* positionBuf = new int[voxelCount * 3]();  // position coordinates of the whole point clouds
  bool resLayer = aps.transResLayer;
  int golombnum = resLayer ? 3 : aps.refGolombNum;
  const bool isGolomb = golombnum <= 2 ? true : false;
  //Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.refReordermode, pointCloudCode, voxelCount, aps.axisBias);
  for (int n = 0; n < voxelCount; n++) {
    const auto reflectance = pointCloudRec.getReflectance(pointCloudCode[n].index, multil_ID);
    attributesBuf[n] = FXPoint(reflectance);
    const auto& point = pointCloudRec[pointCloudCode[n].index];
    positionBuf[3 * n] = point[0];
    positionBuf[3 * n + 1] = point[1];
    positionBuf[3 * n + 2] = point[2];
  }
  // Transform.
  int resLayerQuantParam = sps.reflQuantParam + abh.reflQPoffset;
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
    int* recAttributes = recAttributesBuf + segmentStartPosition;
    int* position = positionBuf + segmentStartPosition * 3;
    int segmentVoxelCount = segmentLen;
    if (segIndex == (numSegments - 1)) {
      segmentVoxelCount = voxelCount - (numSegments - 1) * segmentLen;
    }
    // Transform.
    WaveletCoreTransform(attributes, 1, segmentVoxelCount, integerizedAttributes, sps, aps, abh,
                         position, recAttributes, disThInit);
    // Entropy encode.
    int run_length = 0;
    for (int n = 0; n < segmentVoxelCount; ++n) {
      int64_t signResidualQuant = static_cast<int64_t>(integerizedAttributes[n]);
      if (signResidualQuant == 0)
        ++run_length;
      else {
        m_encBac->encodeRunlength(run_length);
        run_length = 0;
        m_encBac->codeAttributerResidual(signResidualQuant, false, 3, 0, false, false, golombnum);
      }
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        m_encBac->encodeRunlength(run_length);
        run_length = 0;
      }
    }
    m_encBac->encodeRunlength(run_length);
    // Add residues for limitedlosy and lossless attribute compression
    if (resLayer) {
      cout << "ResLayer Needed!" << endl;
      for (int n = 0; n < voxelCount; n++) {
        int reflectance_rec = recAttributes[n];
        const auto reflectance_ori =
          pointCloudRec.getReflectance(pointCloudCode[n].index, multil_ID);
        integerizedAttributes[n] = (int)reflectance_ori - reflectance_rec;
      }
      int run_length = 0;
      for (int n = 0; n < voxelCount; ++n) {
        auto pointIndex = pointCloudCode[n + segmentStartPosition].index;
        int64_t detail = static_cast<int64_t>(integerizedAttributes[n]);
        int residualSign = detail < 0 ? -1 : 1;
        uint64_t absResidual = std::abs(detail);
        uint64_t residualQuant = QuantizaResidual(absResidual, resLayerQuantParam, 1);
        int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
        integerizedAttributes[n] =
          residualSign * InverseQuantizeResidual(residualQuant, resLayerQuantParam);
        auto pos = pointCloudRec[pointIndex];
        if (signResidualQuant == 0)
          ++run_length;
        else {
          m_encBac->encodeRunlength(run_length);
          run_length = 0;
          m_encBac->codeAttributerResidual(signResidualQuant, false, 3, 0, false, false, golombnum);
        }
        if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
          m_encBac->encodeRunlength(run_length);
          run_length = 0;
        }
      }
      m_encBac->encodeRunlength(run_length);
    }
    // Reconstruction process
    for (int n = 0; n < segmentVoxelCount; n++) {
      int64_t r = recAttributes[n];
      if (resLayer) {
        r = integerizedAttributes[n] + (int)r;
      }
      const PC_REFL reflectance = (PC_REFL)TComClip((Int64)ClipMin, (Int64)ClipMax, r);
      pointCloudRec.setReflectance(pointCloudCode[n + segmentStartPosition].index, reflectance,
                                   multil_ID);
    }
  }
  // De-allocate arrays.
  delete[] attributesBuf;
  delete[] integerizedAttributesBuf;
  delete[] positionBuf;
  delete[] recAttributesBuf;

  m_encBac->codeSliceAttrEndCode();
  if (frame_Id == frame_Count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_encBac->codeSPSEndCode();
}

void TEncAttribute::colorWaveletTransformFromReflectance() {
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameHead = m_hls->frameHead;
  const bool isGolomb = aps.colorGolombNum <= 2 ? true : false;
  cout << "colorWaveletTransformFromReflectance" << endl;
  int voxelCount = int(pointCloudRec.getNumPoint());
  bool os = aps.orderSwitch;

  // Allocate arrays.
  int attribCount = 3;
  FXPoint* attributesBuf =
    new FXPoint[voxelCount * attribCount];  // attributes of the whole point clouds
  int* integerizedAttributesBuf =
    new int[voxelCount *
            attribCount];  // transform coefficients / attribute residues of the whole point clouds
  int* recAttributesBuf =
    new int[voxelCount * attribCount]();  // reconstructed attributes of the whole point clouds
  int* positionBuf = new int[voxelCount * 3]();  // position coordinates of the whole point clouds

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

  bool resLayer = aps.transResLayer;
  int resLayerQuantParam = sps.colorQuantParam;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;
  int preColor[3];
  int64_t values[3];
  PC_COL recColor;
  PC_COL oriColor;
  //Hilbert Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReordermode, pointCloudCode, voxelCount, 1);

  for (int n = 0; n < voxelCount; n++) {
    const auto color = pointCloudRec.getColor(pointCloudCode[n].index, multil_ID);
    attributesBuf[n * attribCount] = FXPoint(color[0]);
    attributesBuf[n * attribCount + 1] = FXPoint(color[1]);
    attributesBuf[n * attribCount + 2] = FXPoint(color[2]);
    const auto reflectance =
      pointCloudRec.getReflectance(pointCloudCode[n].index, multil_ID) * crossAttrTypeCoef;
    reflectanceBuf[n] = reflectance;
    const auto& point = pointCloudRec[pointCloudCode[n].index];
    positionBuf[3 * n] = point[0];
    positionBuf[3 * n + 1] = point[1];
    positionBuf[3 * n + 2] = point[2];
  }

  // Transform.
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
    int* reflectances = reflectanceBuf + segmentStartPosition;
    int* recAttributes = recAttributesBuf + segmentStartPosition * attribCount;
    int* position = positionBuf + segmentStartPosition * 3;

    int segmentVoxelCount = segmentLen;
    if (segIndex == (numSegments - 1)) {
      segmentVoxelCount = voxelCount - (numSegments - 1) * segmentLen;
    }

    // Transform.
    WaveletCoreTransform(attributes, 3, segmentVoxelCount, integerizedAttributes, sps, aps, abh,
                         position, recAttributes, disThInit, reflectances, 1);
    // Entropy encode.
    int run_length = 0;  //segmentVoxelCount
    for (int n = 0; n < segmentVoxelCount; ++n) {
      for (int kk = 0; kk < attribCount; kk++) {
        values[kk] = static_cast<int64_t>(integerizedAttributes[n * attribCount + kk]);
      }
      if (!values[0] && !values[1] && !values[2])
        ++run_length;
      else {
        m_encBac->encodeRunlength(run_length);
        if (!os) {
          colorResidualCorrelationCode(values, false, aps.colorGolombNum);
        } else {
          colorResidualCorrelationCodeOS(values, false, aps.colorGolombNum);
        }
        run_length = 0;
      }
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        m_encBac->encodeRunlength(run_length);
        run_length = 0;
      }
    }
    m_encBac->encodeRunlength(run_length);

    // Add residues for limitedlosy and lossless attribute compression
    if (resLayer) {
      cout << "resLayer Needed!" << endl;
      for (int n = 0; n < segmentVoxelCount; n++) {
        oriColor = pointCloudRec.getColor(pointCloudCode[n].index, multil_ID);
        for (int kk = 0; kk < attribCount; kk++) {
          integerizedAttributes[n * attribCount + kk] =
            oriColor[kk] - recAttributes[n * attribCount + kk];
        }
      }
      int run_length = 0;
      for (int n = 0; n < segmentVoxelCount; ++n) {
        for (int kk = 0; kk < attribCount; kk++) {
          int64_t detail = static_cast<int64_t>(integerizedAttributes[n * attribCount + kk]);
          int residualSign = detail < 0 ? -1 : 1;
          uint64_t absResidual = std::abs(detail);
          uint64_t residualQuant = QuantizaResidual(absResidual, resLayerQuantParam, 1);
          int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
          values[kk] = signResidualQuant;
          integerizedAttributes[n * attribCount + kk] =
            residualSign * InverseQuantizeResidual(residualQuant, resLayerQuantParam);
        }
        if (!values[0] && !values[1] && !values[2])
          ++run_length;
        else {
          m_encBac->encodeRunlength(run_length);
          if (!os) {
            colorResidualCorrelationCode(values, false, aps.colorGolombNum);
          } else {
            colorResidualCorrelationCodeOS(values, false, aps.colorGolombNum);
          }
          run_length = 0;
        }
        if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
          m_encBac->encodeRunlength(run_length);
          run_length = 0;
        }
      }
      m_encBac->encodeRunlength(run_length);
    }

    // Reconstruction process
    for (int n = 0; n < segmentVoxelCount; n++) {
      for (int kk = 0; kk < attribCount; kk++) {
        preColor[kk] = recAttributes[n * attribCount + kk];
        if (resLayer)
          preColor[kk] += integerizedAttributes[n * attribCount + kk];
      }

      for (int kk = 0; kk < attribCount; kk++) {
        recColor[kk] = (Int16)TComClip((Int64)ClipMin, (Int64)ClipMax, (Int64)preColor[kk]);
      }

      pointCloudRec.setColor(pointCloudCode[n + segmentStartPosition].index, recColor, multil_ID);
    }
  }
  // De-allocate arrays.
  delete[] attributesBuf;
  delete[] integerizedAttributesBuf;
  delete[] positionBuf;
  delete[] recAttributesBuf;
  delete[] reflectanceBuf;
}

void TEncAttribute::reflectanceWaveletTransformFromColor() {
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameHead = m_hls->frameHead;
  cout << "reflectanceWaveletTransformFromColor" << endl;
  int voxelCount = int(pointCloudRec.getNumPoint());

  // Allocate arrays.
  FXPoint* attributesBuf = new FXPoint[voxelCount];  // attributes of the whole point clouds
  int* integerizedAttributesBuf =
    new int[voxelCount];  // transform coefficients / attribute residues of the whole point clouds
  int* recAttributesBuf =
    new int[voxelCount]();  // reconstructed attributes of the whole point clouds
  int* positionBuf = new int[voxelCount * 3]();  // position coordinates of the whole point clouds
  bool resLayer = aps.transResLayer;
  int golombnum = resLayer ? 3 : aps.refGolombNum;
  const bool isGolomb = golombnum <= 2 ? true : false;
  //Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.refReordermode, pointCloudCode, voxelCount, aps.axisBias);

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
    const auto reflectance = pointCloudRec.getReflectance(pointCloudCode[n].index, multil_ID);
    attributesBuf[n] = FXPoint(reflectance);
    const auto color = pointCloudRec.getColor(pointCloudCode[n].index, multil_ID);
    colorBuf[n * 3] = (color[0] * crossAttrTypeCoef);
    colorBuf[n * 3 + 1] = (color[1] * crossAttrTypeCoef);
    colorBuf[n * 3 + 2] = (color[2] * crossAttrTypeCoef);
    const auto& point = pointCloudRec[pointCloudCode[n].index];
    positionBuf[3 * n] = point[0];
    positionBuf[3 * n + 1] = point[1];
    positionBuf[3 * n + 2] = point[2];
  }
  // Transform.
  int resLayerQuantParam = sps.reflQuantParam + abh.reflQPoffset;
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
    int* integerizedAttributes = integerizedAttributesBuf + segmentStartPosition;
    int* colors = colorBuf + segmentStartPosition * 3;
    int* recAttributes = recAttributesBuf + segmentStartPosition;
    int* position = positionBuf + segmentStartPosition * 3;

    int segmentVoxelCount = segmentLen;
    if (segIndex == (numSegments - 1)) {
      segmentVoxelCount = voxelCount - (numSegments - 1) * segmentLen;
    }

    // Transform.
    WaveletCoreTransform(attributes, 1, segmentVoxelCount, integerizedAttributes, sps, aps, abh,
                         position, recAttributes, disThInit, colors, 3);
    // Entropy encode.
    int run_length = 0;
    for (int n = 0; n < segmentVoxelCount; ++n) {
      int64_t signResidualQuant = static_cast<int64_t>(integerizedAttributes[n]);
      if (signResidualQuant == 0)
        ++run_length;
      else {
        m_encBac->encodeRunlength(run_length);
        run_length = 0;
        m_encBac->codeAttributerResidual(signResidualQuant, false, 3, 0, false, false, golombnum);
      }
      if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
        m_encBac->encodeRunlength(run_length);
        run_length = 0;
      }
    }
    m_encBac->encodeRunlength(run_length);

    // Add residues for limitedlosy and lossless attribute compression
    if (resLayer) {
      cout << "ResLayer Needed!" << endl;
      for (int n = 0; n < voxelCount; n++) {
        int reflectance_rec = recAttributes[n];
        const auto reflectance_ori =
          pointCloudRec.getReflectance(pointCloudCode[n].index, multil_ID);
        integerizedAttributes[n] = (int)reflectance_ori - reflectance_rec;
      }
      int run_length = 0;
      for (int n = 0; n < voxelCount; ++n) {
        int64_t detail = static_cast<int64_t>(integerizedAttributes[n]);
        int residualSign = detail < 0 ? -1 : 1;
        uint64_t absResidual = std::abs(detail);
        uint64_t residualQuant = QuantizaResidual(absResidual, resLayerQuantParam, 1);
        int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
        integerizedAttributes[n] =
          residualSign * InverseQuantizeResidual(residualQuant, resLayerQuantParam);
        if (signResidualQuant == 0)
          ++run_length;
        else {
          m_encBac->encodeRunlength(run_length);
          run_length = 0;
          m_encBac->codeAttributerResidual(signResidualQuant, false, 3, 0, false, false, golombnum);
        }
        if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
          m_encBac->encodeRunlength(run_length);
          run_length = 0;
        }
      }
      m_encBac->encodeRunlength(run_length);
    }
    // Reconstruction process
    for (int n = 0; n < segmentVoxelCount; n++) {
      int64_t r = recAttributes[n];
      if (resLayer) {
        r = integerizedAttributes[n] + (int)r;
      }
      const PC_REFL reflectance = (PC_REFL)TComClip((Int64)ClipMin, (Int64)ClipMax, r);
      pointCloudRec.setReflectance(pointCloudCode[n + segmentStartPosition].index, reflectance,
                                   multil_ID);
    }
  }
  // De-allocate arrays.
  delete[] attributesBuf;
  delete[] integerizedAttributesBuf;
  delete[] positionBuf;
  delete[] recAttributesBuf;
  delete[] colorBuf;
}

void TEncAttribute::colorResidualCorrelationCodeHaar(const int64_t signResidualQuantvalue[3],
                                                     const bool reslayer,
                                                     const bool isDuplicatePoint,
                                                     const UInt& golombNum) {
  int flagyu_rg = 0;
  int flagy_r = 0;
  if (signResidualQuantvalue[0] == 0 && signResidualQuantvalue[1] == 0) {
    flagyu_rg = 0;
  } else {
    flagyu_rg = 1;
  }
  if (signResidualQuantvalue[0] == 0) {
    flagy_r = 0;
  } else {
    flagy_r = 1;
  }
  //code Y/R
  m_encBac->codeAttributerResidualequalone0(flagy_r);
  bool iscolor = true;
  bool residualminusone = true;
  if (signResidualQuantvalue[0] == 0) {
    //code U/G
    m_encBac->codeAttributerResidualequaltwo0(flagyu_rg);
    if (signResidualQuantvalue[1] == 0) {
      m_encBac->codeAttributerResidualHaar(signResidualQuantvalue[2], iscolor, 2, reslayer, 0,
                                           2 == 0 && isDuplicatePoint, residualminusone, golombNum);
      m_encBac->codeSign(signResidualQuantvalue[2]);
    } else {
      m_encBac->codeAttributerResidualHaar(signResidualQuantvalue[1], iscolor, 1, reslayer, 2,
                                           1 == 0 && isDuplicatePoint, residualminusone, golombNum);
      m_encBac->codeAttributerResidualHaar(signResidualQuantvalue[2], iscolor, 2, reslayer, 0,
                                           2 == 0 && isDuplicatePoint, !residualminusone,
                                           golombNum);
      m_encBac->codeSign(signResidualQuantvalue[1]);
      if (signResidualQuantvalue[2] != 0) {
        m_encBac->codeSign(signResidualQuantvalue[2]);
      }
    }

  } else {
    m_encBac->codeAttributerResidualHaar(signResidualQuantvalue[0], iscolor, 0, reslayer, 1,
                                         0 == 0 && isDuplicatePoint, residualminusone, golombNum);
    m_encBac->codeAttributerResidualHaar(signResidualQuantvalue[1], iscolor, 1, reslayer, 1,
                                         1 == 0 && isDuplicatePoint, !residualminusone, golombNum);
    m_encBac->codeAttributerResidualHaar(signResidualQuantvalue[2], iscolor, 2, reslayer, 2,
                                         2 == 0 && isDuplicatePoint, !residualminusone, golombNum);
    m_encBac->codeSign(signResidualQuantvalue[0]);
    if (signResidualQuantvalue[1] != 0) {
      m_encBac->codeSign(signResidualQuantvalue[1]);
    }
    if (signResidualQuantvalue[2] != 0) {
      m_encBac->codeSign(signResidualQuantvalue[2]);
    }
  }
}

// new
void TEncAttribute::multiReflectancePredictingResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.refReordermode, pointCloudCode, voxelCount, aps.axisBias);
  m_hls->sps.reflThreshold =
    (sps.reflQuantParam + abh.reflQPoffset) * aps.nearestPredParam1 + aps.nearestPredParam2;
  // obtain reflectance predictor
  int multil_ID_Group_Num = aps.multiAttriGroupNum[aps.multiAttriGroupID[multil_ID]];
  PC_REFL predictorRefl[5];
  PC_REFL recReflectance[5];
  std::vector<multiReflNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int setCount = 0;
  // runlength
  int run_length = 0;
  const bool isGolomb = m_hls->aps.refGolombNum == 1 ? true : false;
  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    auto pointIndex = pointCloudCode[curIndex].index;
    PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
    const PC_POS& curPosition = pointCloudRec[pointIndex];

    getMultiReflectancePredictorFarthest(curIndex, curIndex, pointCloudRec, sps, aps,
                                        pointCloudCode, setCount, neighborSet, predictorRefl,
                                        multil_ID_Group_Num); 

    for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
      recReflectance[multi_id] = pointCloudRec.getReflectance(pointIndex, multi_id + multil_ID);
    }
    multiReflectancePredictCode(predictorRefl, recReflectance, run_length, multil_ID_Group_Num);
    for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
      pointCloudRec.setReflectance(pointIndex, recReflectance[multi_id], multi_id + multil_ID);
      int farthestIdx = 0;
      if (curIndex < setlength) {
        farthestIdx = curIndex;
      }
      neighborSet[farthestIdx].pos = pointCloudRec[pointIndex];
      neighborSet[farthestIdx].refl[multi_id] = recReflectance[multi_id];
    }
  }
  m_encBac->encodeRunlength(run_length);

  m_encBac->codeSliceAttrEndCode();
  if (frame_Id == frame_Count - 1 && m_hls->abh.sliceID == m_hls->frameHead.num_slice_minus_one)
    m_encBac->codeSPSEndCode();
}

void TEncAttribute::multiReflectancePredictCode(PC_REFL* predictor, PC_REFL* currValue,
                                                int& run_length, int multil_ID_Group_Num) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const bool isGolomb = aps.refGolombNum == 1 ? true : false;
  UInt golombNum = aps.refGolombNum;
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (aps.reflOutputDepth)) - 1;
  }

  // Coefficient decorrelated
  bool ccp = false;
  if(false)//if (aps.crossComponentPred)
    ccp = true;
  int64_t signResidualQuantvalue[10];
  int residualPrevComponent = 0;
  for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
    int64_t delta = currValue[multi_id] - predictor[multi_id];
    delta -= residualPrevComponent;
    // Residual quantization
    int residualSign = delta < 0 ? -1 : 1;
    uint64_t absResidual = std::abs(delta);
    uint64_t residualQuant;
    residualQuant = QuantizaResidual(absResidual, sps.reflQuantParam + abh.reflQPoffset, 1);
    int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
    signResidualQuantvalue[multi_id] = signResidualQuant;
    // Local decode
    uint64_t inverseResidualQuant =
      InverseQuantizeResidual(residualQuant, sps.reflQuantParam + abh.reflQPoffset);
    int64_t recResidual = inverseResidualQuant * residualSign;
    currValue[multi_id] = TComClip((Int64)ClipMin, (Int64)ClipMax,
                                   recResidual + predictor[multi_id] + residualPrevComponent);
    if (ccp && multi_id < (multil_ID_Group_Num - 1)) {
      residualPrevComponent = (Int)currValue[multi_id] - (Int)predictor[multi_id];
    }
  }

  // Entropy encode
  for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
    auto signResidualQuant = signResidualQuantvalue[multi_id];
    //cout << signResidualQuant << " ";
    if (signResidualQuant == 0)
      ++run_length;
    else {
      if (m_encBacDual != NULL) {
        m_encBacDual->encodeRunlength(run_length);
        ///<if used forceAttribute predict
        run_length = 0;
        m_encBacDual->codeAttributeResidualDual(signResidualQuant, false, 0, false, false,
                                                golombNum);
      } else {
        m_encBac->encodeRunlength(run_length);
        ///<if used forceAttribute predict
        run_length = 0;
        m_encBac->codeAttributerResidual(signResidualQuant, false, 3, 0, false, false, golombNum);
      }
    }
    // lengthControl
    if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
      if (m_encBacDual != NULL) {
        m_encBacDual->encodeRunlength(run_length);
      } else {
        m_encBac->encodeRunlength(run_length);
      }
      run_length = 0;
    }
  }
}
