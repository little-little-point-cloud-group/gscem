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
  //set adaptive expGolomb decoder parameter
  if (m_hls->aps.attributeDataPresentFlag[0]) {
    m_encBac->setColorGolombKandBound(m_hls->aps.golombGroupSizeLog2, m_hls->aps.colorGolombNum);
    if (m_encBacDual) {
      m_encBacDual->setColorGolombKandBound(m_hls->aps.golombGroupSizeLog2,
                                            m_hls->aps.colorGolombNum);
    }
  }
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
  if (m_hls->aps.attributeDataPresentFlag[0] && m_hls->aps.attributeDataPresentFlag[1]) {
    m_encBacDual = encBacDual;
  } else {
    m_encBacDual = NULL;
  }
  frame_Id = m_frame_ID;
  frame_Count = m_numOfFrames;
  multil_ID = multil_Id;
  //set adaptive expGolomb decoder parameter
  if (m_hls->aps.attributeDataPresentFlag[0]) {
    m_encBac->setColorGolombKandBound(m_hls->aps.golombGroupSizeLog2, m_hls->aps.colorGolombNum);
    if (m_encBacDual) {
      m_encBacDual->setColorGolombKandBound(m_hls->aps.golombGroupSizeLog2,
                                            m_hls->aps.colorGolombNum);
    }
  }
}

// Attribute compression consists of the following stages:
//  - reorder
//  - prediction
//  - residual quantization
//  - entropy encode
//  - local decode
Void TEncAttribute::dualEncodeAttribute() {
  clock_t userTimeColorBegin = clock();
  if (m_hls->aps.transform == 0) {
    attributePredictingResidual();
  } else if (m_hls->aps.transform == 2) {
    attributePredictAndTransformMemControl();
  }
  m_colorTime = (Double)(clock() - userTimeColorBegin) / CLOCKS_PER_SEC;
  clock_t userTimeReflBegin = clock();
  m_reflTime = (Double)(clock() - userTimeReflBegin) / CLOCKS_PER_SEC;
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

Void TEncAttribute::transformEncodeColorFromReflectance() {
  clock_t userTimeColorBegin = clock();
  colorWaveletTransformFromReflectance();
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

Void TEncAttribute::transformEncodeReflectanceFromColor() {
  clock_t userTimeReflectanceBegin = clock();
  reflectanceWaveletTransformFromColor();
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
  const FrameHeader& frameheader = m_hls->frameheader;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());

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
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReorderMode, pointCloudCode, voxelCount,
          aps.axisBias);
  // Qp compute
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
  // Initialize predictors
  pair<PC_COL, PC_REFL> predictorAttr;
  PC_COL predictorColor;
  PC_REFL predictorRefl;
  int prevIndex = -1;
  int setlength = aps.maxNumOfNeighbours;
  int setCount = 0;

  vector<neighborSet> neighborSet;
  neighborSet.resize(setlength);
  PC_POS prePosition = pointCloudRec[pointCloudCode[0].index];
  PC_REFL preReflectance = 0;
  // runlength
  int run_length_col = 0;
  int run_length_refl = 0;
  const bool refisGolomb = m_hls->aps.reflGolombNum == 1 ? true : false;
  const bool colorisGolomb = m_hls->aps.colorGolombNum == 1 ? true : false;
  // disable cross-attribute-type-prediction
  if (!aps.crossAttrTypePred) {
    for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
      auto pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = pointCloudRec[pointIndex];
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
      PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
      Bool isDuplicatePoint =
        curIndex > 0 && curPosition == pointCloudRec[prevIndex] && m_hls->aps.eligibleDupPointPred;
      if (isDuplicatePoint) {
        predictorColor = pointCloudRec.getColor(prevIndex, multil_ID);
        predictorRefl = pointCloudRec.getReflectance(prevIndex, multil_ID);
      } else {
        getAttributePredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                                      predictOptParams, predictorAttr);
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
    for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
      auto pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = pointCloudRec[pointIndex];
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
      PC_REFL& curRefl = pointCloudRec.getReflectance(pointIndex, multil_ID);
      Bool isDuplicatePoint =
        curIndex > 0 && curPosition == pointCloudRec[prevIndex] && m_hls->aps.eligibleDupPointPred;
      if (isDuplicatePoint) {
        predictorColor = pointCloudRec.getColor(prevIndex, multil_ID);
      } else {
        getColorPredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                                  predictOptParams, predictorColor);
      }
      colorPredictCode(predictorColor, curColor, colorQp, run_length_col, isDuplicatePoint);
      // calculate the color with coefficient, which is used in cross-attribute-type-prediction
      predictOptParams.curColorWithCoef[0] = curColor[0] * crossAttrTypeCoef;
      predictOptParams.curColorWithCoef[1] = curColor[1] * crossAttrTypeCoef;
      predictOptParams.curColorWithCoef[2] = curColor[2] * crossAttrTypeCoef;
      if (isDuplicatePoint) {
        predictorRefl = pointCloudRec.getReflectance(prevIndex, multil_ID);
      } else {
        // get the better refl prediction through the geometric distance and the color distance
        getReflPredictorFarthestDual(curIndex, curIndex, curPosition, setCount, neighborSet,
                                     predictOptParams, predictorRefl);
      }
      reflectancePredictCode(predictorRefl, curRefl, run_length_refl, isDuplicatePoint);

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
    m_encBac->encodeRunlength(run_length_col);
    m_encBacDual->encodeRunlength(run_length_refl);
    // enable cross-attribute-type-prediction , encode the reflectance and then encode the color
  } else {
    for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = pointCloudRec[pointIndex];
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
      PC_REFL& curRefl = pointCloudRec.getReflectance(pointIndex, multil_ID);
      Bool isDuplicatePoint =
        curIndex > 0 && curPosition == pointCloudRec[prevIndex] && m_hls->aps.eligibleDupPointPred;
      if (isDuplicatePoint) {
        predictorRefl = pointCloudRec.getReflectance(prevIndex, multil_ID);
      } else {
        getReflPredictorFarthestDual(curIndex, curIndex, curPosition, setCount, neighborSet,
                                     predictOptParams, predictorRefl);
      }
      reflectancePredictCode(predictorRefl, curRefl, run_length_refl, isDuplicatePoint);
      // calculate the reflectance with coefficient, which is used in cross-attribute-type-prediction
      predictOptParams.curReflWithCoef = curRefl * crossAttrTypeCoef;

      if (curIndex != 0)
        calculateReflTrend(curPosition, prePosition, curRefl, preReflectance,
                           log2_reflectanceDistCoef, reflectanceRes, reflResNum,
                           predictOptParams.reflectanceDistWeight, distWeightGroupSize);
      if (isDuplicatePoint) {
        predictorColor = pointCloudRec.getColor(prevIndex, multil_ID);
      } else {
        // get the better color prediction through the geometric distance and the reflectance distance
        getColorPredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                                  predictOptParams, predictorColor);
      }
      colorPredictCode(predictorColor, curColor, colorQp, run_length_col, isDuplicatePoint);
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
    m_encBacDual->encodeRunlength(run_length_refl);
    m_encBac->encodeRunlength(run_length_col);
  }
}

void TEncAttribute::reflectancePredictingResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const FrameHeader& frameheader = m_hls->frameheader;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());

  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.reflReorderMode, pointCloudCode, voxelCount, aps.axisBias);

  // Initialize reflectance predictor
  PC_REFL predictorRefl;
  std::vector<neighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int setCount = 0;
  int prevIndex = -1;
  PC_REFL lastref = 0;

  // runlength
  int run_length = 0;

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
    int pointIndex = pointCloudCode[curIndex].index;
    const PC_POS& curPosition = pointCloudRec[pointIndex];
    PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
    Bool isDuplicatePoint =
      curIndex > 0 && curPosition == pointCloudRec[prevIndex] && m_hls->aps.eligibleDupPointPred;
    if (isDuplicatePoint) {
      predictorRefl = lastref;
    } else {
      getReflPredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                               predictOptParams, predictorRefl);
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
}

void TEncAttribute::colorPredictingResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReorderMode, pointCloudCode, voxelCount, 1);
  //Qp compute
  quantizedQP colorQp;
  colorQp.attrQuantForLuma = aps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

  // obtain color predictor
  PC_COL predictorColor;
  std::vector<neighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  PC_COL lastColor;
  int prevIndex = -1;
  int setCount = 0;
  int run_length = 0;

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
    const PC_POS& curPosition = pointCloudRec[pointIndex];
    PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
    Bool isDuplicatePoint =
      curIndex > 0 && curPosition == pointCloudRec[prevIndex] && m_hls->aps.eligibleDupPointPred;
    if (isDuplicatePoint) {
      predictorColor = lastColor;
    } else {
      getColorPredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                                predictOptParams, predictorColor);
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
}

void TEncAttribute::colorPredictAndTransformMemControl() {
  std::cout << "ColorPredictingAndTransformMemControl" << std::endl;
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const FrameHeader& frameheader = m_hls->frameheader;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());
  //Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReorderMode, pointCloudCode, voxelCount, 1);
  // Qp compute
  quantizedQP colorQp;
  colorQp.attrQuantForLuma = aps.colorQuantParam;
  colorQp.attrQuantForChromaCb =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCb);
  colorQp.attrQuantForChromaCr =
    TComClip(0, 63, (Int)colorQp.attrQuantForLuma + aps.chromaQpOffsetCr);

  //quantization deadzone offset
  colorQuantShift colorQuantShift;
  if (aps.colorQuantParam <= 48) {
    colorQuantShift.colorOffsetParamLumaAc =
      (0.2 + ((48 - aps.colorQuantParam) >> 3) * 0.01);  // Quantization Shift
    colorQuantShift.colorOffsetParamChromaAc = (0.02 + ((48 - aps.colorQuantParam) >> 3) * 0.08);
    colorQuantShift.colorOffsetParamLumaDc =
      (0.18 + ((48 - aps.colorQuantParam) >> 3) * 0.008);  // Quantization Shift
    colorQuantShift.colorOffsetParamChromaDc = (0.02 + ((48 - aps.colorQuantParam) >> 3) * 0.06);
  } else {
    colorQuantShift.colorOffsetParamLumaAc = 0.2;  // Quantization Shift
    colorQuantShift.colorOffsetParamChromaAc = 0.02;
    colorQuantShift.colorOffsetParamLumaDc = 0.18;  // Quantization Shift
    colorQuantShift.colorOffsetParamChromaDc = 0.02;
  }
  //  grouping
  int maxNodeSizeLog2 = std::max(
    {1U, m_hls->gbh.nodeSizeLog2[0], m_hls->gbh.nodeSizeLog2[1], m_hls->gbh.nodeSizeLog2[2]});
  int groupShiftBits = std::max(3, 3 * (maxNodeSizeLog2 - ((ceilLog2(voxelCount / 4) + 1) >> 1)));
  // adjust color QP per point
  bool colorQPAdjustSliceFlag = aps.colorQPAdjustFlag;
  bool colorQPAdjustFlag = false;
  int scalar = 1;
  if (colorQPAdjustSliceFlag)
    scalar = abh.colorQPAdjustScalar;
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
  std::vector<neighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);

  // transform and entropy coding parameter
  std::vector<int> transformPointIdx;
  int64_t transformBuf[3][8] = {};
  int64_t transformPredBuf[3][8] = {};
  int dcIndex = 0;
  int acIndex = numofGroupCount[0];
  int numofCoeff = 0;
  int groupCount = 1;
  const int maxNumofCoeff = max(aps.maxNumofCoeff, aps.colorMaxTransNum);
  int* CoeffGroup = new int[maxNumofCoeff * 3]();

  int run_length = 0;
  int lengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;

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
    while (subGroupCount < length[subgroupIndex]) {
      transformPointIdx.push_back(curIndex);
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = pointCloudRec[pointIndex];
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
      getColorPredictorFarthest(curIndex, transformPointIdx[0], curPosition, subGroupCount,
                                neighborSet, predictOptParams, predictorColor);
      for (int k = 0; k < 3; k++) {
        transformBuf[k][subGroupCount] = curColor[k];
        transformPredBuf[k][subGroupCount] = predictorColor[k];
        transformBuf[k][subGroupCount] -= transformPredBuf[k][subGroupCount];
      }
      ++subGroupCount;
      curIndex++;
    }
    if (colorQPAdjustSliceFlag)
      colorQPAdjustFlag = predictOptParams.minNeighborDis > colorQPAdjustDis;
    if (deadZoneChromaSliceFlag)
      deadZoneChromaFlag = predictOptParams.minNeighborDis > deadZoneChromaDis;

    colorTransformCode(pointCloudCode, transformPointIdx, dcIndex, acIndex, transformBuf,
                       transformPredBuf, colorQp, CoeffGroup, neighborSet, colorQuantShift, true,
                       colorQPAdjustFlag, deadZoneChromaFlag);

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
}

void TEncAttribute::reflectancePredictAndTransformMemControl() {
  std::cout << "ReflectancePredictingAndTransformMemControl" << std::endl;
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const FrameHeader& frameheader = m_hls->frameheader;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.reflReorderMode, pointCloudCode, voxelCount, aps.axisBias);
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
  int subgroupIndex = 0;
  int subGroupCount = 0;
  //obtain the reflectance predictor
  PC_REFL predictorRefl;
  std::vector<neighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
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

  // transform and entropy coding parameter
  std::vector<int> transformPointIdx;
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
  int lengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;
  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    auto pointIndex = pointCloudCode[curIndex].index;
    PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
    const PC_POS& curPosition = pointCloudRec[pointIndex];
    Bool isDuplicatePoint = (curIndex > (length[0] - 1)) &&
      (curPosition == pointCloudRec[prevIndex]) && m_hls->aps.eligibleDupPointPred;
    if (isDuplicatePoint) {
      predictorRefl = lastref;
      isDuplicatePoint &= multil_ID == 0;
      reflectanceCode(predictorRefl, curReflectance, value, isDuplicatePoint);
      CoeffGroup[dcIndex++] = value;
      numofCoeff += length[subgroupIndex];
      subgroupIndex++;
      prevIndex = pointIndex;
      lastref = curReflectance;
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
      transformBuf[0][subGroupCount] = curReflectance;
      transformPredBuf[0][subGroupCount] = predictorRefl;
      transformBuf[0][subGroupCount] -= transformPredBuf[0][subGroupCount];
      ++subGroupCount;
      if (subGroupCount == length[subgroupIndex]) {
        reflectanceTransformCode(pointCloudCode, transformPointIdx, dcIndex, acIndex, transformBuf,
                                 transformPredBuf, CoeffGroup, neighborSet, lastref);
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
}

void TEncAttribute::attributePredictAndTransformMemControl() {
  std::cout << "attributePredictAndTransformMemControl" << std::endl;
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const FrameHeader& frameheader = m_hls->frameheader;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());

  //HilbertAddr Sorting
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReorderMode, pointCloudCode, voxelCount,
          aps.axisBias);

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

  //quantization deadzone offset
  colorQuantShift colorQuantShift;
  if (aps.colorQuantParam <= 48) {
    colorQuantShift.colorOffsetParamLumaAc =
      (0.2 + ((48 - aps.colorQuantParam) >> 3) * 0.01);  // Quantization Shift
    colorQuantShift.colorOffsetParamChromaAc = (0.02 + ((48 - aps.colorQuantParam) >> 3) * 0.08);
    colorQuantShift.colorOffsetParamLumaDc =
      (0.18 + ((48 - aps.colorQuantParam) >> 3) * 0.008);  // Quantization Shift
    colorQuantShift.colorOffsetParamChromaDc = (0.02 + ((48 - aps.colorQuantParam) >> 3) * 0.06);
  } else {
    colorQuantShift.colorOffsetParamLumaAc = 0.2;  // Quantization Shift
    colorQuantShift.colorOffsetParamChromaAc = 0.02;
    colorQuantShift.colorOffsetParamLumaDc = 0.18;  // Quantization Shift
    colorQuantShift.colorOffsetParamChromaDc = 0.02;
  }
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

  // adjust color QP per point
  bool colorQPAdjustSliceFlag = aps.colorQPAdjustFlag;
  bool colorQPAdjustFlag = false;
  int scalar = 1;
  if (colorQPAdjustSliceFlag)
    scalar = abh.colorQPAdjustScalar;
  int colorQPAdjustDis = std::max(4, (1 << (groupShiftBits / 3 + 4)) / scalar);
  bool deadZoneChromaSliceFlag = aps.chromaDeadzoneFlag;
  bool deadZoneChromaFlag = false;
  int deadZoneChromaDis = std::max(4, (1 << (groupShiftBits / 3 + 3)) / scalar);
  int subgroupIndexColor = 0;
  vector<int> numofGroupCountColor;
  getLength(pointCloudCode, lengthColor, numofGroupCountColor, aps.maxNumofCoeff, groupShiftBits,
            aps.colorMaxTransNum);

  // ref grouping
  PC_REFL predictorRefl;
  vector<int> lengthRefl;
  vector<int> numofGroupCountRefl;
  vector<int> transformPointIdxRefl;
  transformPointIdxRefl.reserve(8);
  bool dupPredFlagRefl = 0;
  PC_REFL lastref;
  int64_t transformBufRefl[3][8] = {};
  int64_t transformPredBufRefl[3][8] = {};

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
  PC_POS prePosition = pointCloudRec[pointCloudCode[0].index];
  PC_REFL preReflectance = 0;
  int setlength = aps.maxNumOfNeighbours;
  std::vector<neighborSet> neighborSetRefl;
  neighborSetRefl.resize(setlength);
  std::vector<neighborSet> neighborSetColor;
  neighborSetColor.resize(setlength);

  int run_length_col = 0;  ///<zero count
  int run_length_refl = 0;
  int lengthControl = aps.maxNumofCoeff * aps.coeffLengthControl;
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
  if (!aps.crossAttrTypePred) {
    for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
      int pointIndex = pointCloudCode[curIndex].index;
      const PC_POS& curPosition = pointCloudRec[pointIndex];
      PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
      PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
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
      transformBufRefl[0][countRefl] = curReflectance;
      transformPredBufRefl[0][countRefl] = predictorRefl;
      transformBufRefl[0][countRefl] -= transformPredBufRefl[0][countRefl];
      ++countRefl;
      if (countRefl == countNumRefl) {
        reflectanceTransformCode(pointCloudCode, transformPointIdxRefl, dcIndexRefl, acIndexRefl,
                                 transformBufRefl, transformPredBufRefl, CoeffGroupRefl,
                                 neighborSetRefl, lastref);
        countRefl = 0;
        numofCoeffRefl += countNumRefl;
        subgroupIndexRefl++;
        if (subgroupIndexRefl < lengthRefl.size())
          countNumRefl = lengthRefl[subgroupIndexRefl];
        transformPointIdxRefl.erase(transformPointIdxRefl.begin(), transformPointIdxRefl.end());
        if (subgroupIndexRefl == numofGroupCountRefl[groupCountRefl - 1]) {
          assert(numofCoeffRefl <= maxNumofCoeff);
          runlengthEncodeMemControl(CoeffGroupRefl, numofCoeffRefl, run_length_refl, lengthControl,
                                    false);
          numofCoeffRefl = 0;
          setCoeffIndex(groupCountRefl, numofGroupCountRefl, lengthRefl, dcIndexRefl, acIndexRefl);
        }
      }
      getColorPredictorFarthest(curIndex, transformPointIdxColor[0], curPosition, countColor,
                                neighborSetColor, predictOptParams, predictorColor);
      for (int k = 0; k < 3; k++) {
        transformBufColor[k][countColor] = curColor[k];
        transformPredBufColor[k][countColor] = predictorColor[k];
        transformBufColor[k][countColor] -= transformPredBufColor[k][countColor];
      }
      ++countColor;

      if (countColor == countNumColor) {
        if (colorQPAdjustSliceFlag)
          colorQPAdjustFlag = predictOptParams.minNeighborDis > colorQPAdjustDis;
        if (deadZoneChromaSliceFlag)
          deadZoneChromaFlag = predictOptParams.minNeighborDis > deadZoneChromaDis;

        colorTransformCode(pointCloudCode, transformPointIdxColor, dcIndexColor, acIndexColor,
                           transformBufColor, transformPredBufColor, colorQp, CoeffGroupColor,
                           neighborSetColor, colorQuantShift, true, colorQPAdjustFlag,
                           deadZoneChromaFlag);
        transformPointIdxColor.erase(transformPointIdxColor.begin(), transformPointIdxColor.end());
        numofCoeffColor += countNumColor;
        subgroupIndexColor++;
        if (subgroupIndexColor < lengthColor.size())
          countNumColor = lengthColor[subgroupIndexColor];
        countColor = 0;
        // entropy code CoeffGroupColor
        if (subgroupIndexColor == numofGroupCountColor[groupCountColor - 1]) {
          assert(numofCoeffColor <= maxNumofCoeff);
          runlengthEncodeMemControl(CoeffGroupColor, numofCoeffColor, run_length_col, lengthControl,
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
  else if (!aps.attrEncodeOrder) {
    for (int curIndexColor = 0, curIndexRefl = 0; curIndexRefl < voxelCount;) {
      // color predict
      while (curIndexColor < (curIndexRefl + countNumRefl) ||
             (countColor < countNumColor && countColor > 0)) {
        if (countColor == 0 && (curIndexColor > (curIndexRefl + countNumRefl)))
          break;
        int pointIndex = pointCloudCode[curIndexColor].index;
        const PC_POS& curPosition = pointCloudRec[pointIndex];
        PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
        transformPointIdxColor.push_back(curIndexColor);
        getColorPredictorFarthest(curIndexColor, transformPointIdxColor[0], curPosition, countColor,
                                  neighborSetColor, predictOptParams, predictorColor);

        for (int k = 0; k < 3; k++) {
          transformBufColor[k][countColor] = curColor[k];
          transformPredBufColor[k][countColor] = predictorColor[k];
          transformBufColor[k][countColor] -= transformPredBufColor[k][countColor];
        }
        countColor++;
        if (countColor == countNumColor) {
          if (colorQPAdjustSliceFlag)
            colorQPAdjustFlag = predictOptParams.minNeighborDis > colorQPAdjustDis;
          if (deadZoneChromaSliceFlag)
            deadZoneChromaFlag = predictOptParams.minNeighborDis > deadZoneChromaDis;

          colorTransformCode(pointCloudCode, transformPointIdxColor, dcIndexColor, acIndexColor,
                             transformBufColor, transformPredBufColor, colorQp, CoeffGroupColor,
                             neighborSetColor, colorQuantShift, true, colorQPAdjustFlag,
                             deadZoneChromaFlag);
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
                                      lengthControl, true);
            numofCoeffColor = 0;
            setCoeffIndex(groupCountColor, numofGroupCountColor, lengthColor, dcIndexColor,
                          acIndexColor);
          }
        }
        countRefl++;
        curIndexColor++;
      }
      // reflectance predict
      countRefl = 0;
      while (countRefl < countNumRefl) {
        int pointIndex = pointCloudCode[curIndexRefl].index;
        const PC_POS& curPosition = pointCloudRec[pointIndex];
        transformPointIdxRefl.push_back(curIndexRefl);
        predictOptParams.curColorWithCoef = colorWithCoef[pointIndex];
        PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
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
        transformBufRefl[0][countRefl] = curReflectance;
        transformPredBufRefl[0][countRefl] = predictorRefl;
        transformBufRefl[0][countRefl] -= transformPredBufRefl[0][countRefl];
        countRefl++;
        curIndexRefl++;
      }
      // transform and save coefficients in CoeffGroupColor
      reflectanceTransformCode(pointCloudCode, transformPointIdxRefl, dcIndexRefl, acIndexRefl,
                               transformBufRefl, transformPredBufRefl, CoeffGroupRefl,
                               neighborSetRefl, lastref);
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
          PC_REFL curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
          calculateReflTrend(pointCloudRec[pointIndex], prePosition, curReflectance, preReflectance,
                             log2_reflectanceDistCoef, reflectanceRes, reflResNum,
                             predictOptParams.reflectanceDistWeight, distWeightGroupSize);
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
        runlengthEncodeMemControl(CoeffGroupRefl, numofCoeffRefl, run_length_refl, lengthControl,
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
        int pointIndex = pointCloudCode[curIndexRefl].index;
        const PC_POS& curPosition = pointCloudRec[pointIndex];
        PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
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

        transformBufRefl[0][countRefl] = curReflectance;
        transformPredBufRefl[0][countRefl] = predictorRefl;
        transformBufRefl[0][countRefl] -= transformPredBufRefl[0][countRefl];
        countRefl++;

        if (countRefl == countNumRefl) {
          reflectanceTransformCode(pointCloudCode, transformPointIdxRefl, dcIndexRefl, acIndexRefl,
                                   transformBufRefl, transformPredBufRefl, CoeffGroupRefl,
                                   neighborSetRefl, lastref);
          for (int i = 0; i < countNumRefl; i++) {
            auto pointIndex = pointCloudCode[transformPointIdxRefl[i]].index;
            PC_REFL curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
            reflWithCoef[pointIndex] = curReflectance * crossAttrTypeCoef;
            calculateReflTrend(pointCloudRec[pointIndex], prePosition, curReflectance,
                               preReflectance, log2_reflectanceDistCoef, reflectanceRes, reflResNum,
                               predictOptParams.reflectanceDistWeight, distWeightGroupSize);
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
                                      lengthControl, false);
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
        int pointIndex = pointCloudCode[curIndexColor].index;
        const PC_POS& curPosition = pointCloudRec[pointIndex];
        PC_COL& curColor = pointCloudRec.getColor(pointIndex, multil_ID);
        predictOptParams.curReflWithCoef = reflWithCoef[pointIndex];
        getColorPredictorFarthest(curIndexColor, transformPointIdxColor[0], curPosition, countColor,
                                  neighborSetColor, predictOptParams, predictorColor);
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
        colorQPAdjustFlag = predictOptParams.minNeighborDis > colorQPAdjustDis;
      if (deadZoneChromaSliceFlag)
        deadZoneChromaFlag = predictOptParams.minNeighborDis > deadZoneChromaDis;
      colorTransformCode(pointCloudCode, transformPointIdxColor, dcIndexColor, acIndexColor,
                         transformBufColor, transformPredBufColor, colorQp, CoeffGroupColor,
                         neighborSetColor, colorQuantShift, true, colorQPAdjustFlag,
                         deadZoneChromaFlag);

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
      numofCoeffColor += countNumColor;
      subgroupIndexColor++;
      if (subgroupIndexColor < lengthColor.size())
        countNumColor = lengthColor[subgroupIndexColor];
      // entropy code CoeffGroupColor
      if (subgroupIndexColor == numofGroupCountColor[groupCountColor - 1]) {
        assert(numofCoeffColor <= maxNumofCoeff);
        runlengthEncodeMemControl(CoeffGroupColor, numofCoeffColor, run_length_col, lengthControl,
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
}

void TEncAttribute::reflectancePredictCode(const int64_t& predictor, PC_REFL& currValue,
                                           int& run_length, const bool isDuplicatePoint) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
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
  UInt golombNum = aps.reflGolombNum;
  // Residual quantization
  int residualSign = delta < 0 ? -1 : 1;
  uint64_t absResidual = std::abs(delta);
  uint64_t residualQuant;
  residualQuant = QuantizaResidual(absResidual, aps.reflQuantParam + abh.QpOffset, 1);
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
    InverseQuantizeResidual(residualQuant, aps.reflQuantParam + abh.QpOffset);
  int64_t recResidual = inverseResidualQuant * residualSign;
  currValue = TComClip((Int64)ClipMin, (Int64)ClipMax, recResidual + predictor);
}

void TEncAttribute::colorPredictCode(const PC_COL& predictor, PC_COL& currValue,
                                     const quantizedQP& colorQp, int& run_length,
                                     const bool isDuplicatePoint) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
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
    V3<bool> codeSign(true);
    if (isDuplicatePoint)
      determineNeedCodeSign(codeSign, os);
    if (os)
      std::swap(signResidualQuantvalue[0], signResidualQuantvalue[1]);
    colorResidualCorrelationCode(signResidualQuantvalue, isDuplicatePoint, golombNum, codeSign);
  }
  // lengthControl
  if ((aps.coeffLengthControl > 0) && (run_length == aps.coeffLengthControl)) {
    m_encBac->encodeRunlength(run_length);
    run_length = 0;
  }
}

void TEncAttribute::determineNeedCodeSign(V3<bool>& codeSign, const bool& os) {
  codeSign = {true, true, true};
  if (os)
    codeSign[1] = false;
  else
    codeSign[0] = false;
}

void TEncAttribute::colorResidualCorrelationCode(const int64_t signResidualQuantvalue[3],
                                                 const bool isDuplicatePoint, const UInt& golombNum,
                                                 const V3<bool>& codeSign) {
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
  m_encBac->codeAttributerResidualequalone(flagy_r);
  bool iscolor = true;
  bool residualminusone = true;
  if (signResidualQuantvalue[0] == 0) {
    //code U/G
    m_encBac->codeAttributerResidualequaltwo(flagyu_rg);
    if (signResidualQuantvalue[1] == 0) {
      m_encBac->codeAttributerResidual(signResidualQuantvalue[2], iscolor, 2, 0,
                                       2 == 0 && isDuplicatePoint, residualminusone, golombNum);
      m_encBac->codeSign(signResidualQuantvalue[2]);
    } else {
      m_encBac->codeAttributerResidual(signResidualQuantvalue[1], iscolor, 1, 2,
                                       1 == 0 && isDuplicatePoint, residualminusone, golombNum);
      m_encBac->codeAttributerResidual(signResidualQuantvalue[2], iscolor, 2, 0,
                                       2 == 0 && isDuplicatePoint, !residualminusone, golombNum);
      if (codeSign[1])
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
    if (codeSign[0])
      m_encBac->codeSign(signResidualQuantvalue[0]);

    if (signResidualQuantvalue[1] != 0 && codeSign[1]) {
      m_encBac->codeSign(signResidualQuantvalue[1]);
    }
    if (signResidualQuantvalue[2] != 0) {
      m_encBac->codeSign(signResidualQuantvalue[2]);
    }
  }
}

void TEncAttribute::colorTransformCode(std::vector<pointCodeWithIndex>& pointCloudCode,
                                       std::vector<int>& transformPointIdx, int& dcIndex,
                                       int& acIndex, int64_t transformBuf[][8],
                                       int64_t transformPredBuf[][8], const quantizedQP& colorQp,
                                       int* Coefficients, std::vector<neighborSet>& neighborSet,
                                       colorQuantShift& colorQuantShift, const bool isLengthControl,
                                       bool colorQPAdjustFlag, bool deadZoneChromaFlag) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;
  int count = transformPointIdx.size();
  int offsetShift = 0.2 * (1 << encoderShiftBit);  // Quantization Shift

  int offsetShiftLuma1 =
    colorQuantShift.colorOffsetParamLumaAc * (1 << encoderShiftBit);  // Quantization Shift Luma AC
  int offsetShiftChroma1 = colorQuantShift.colorOffsetParamChromaAc *
    (1 << encoderShiftBit);  // Quantization Shift Chroma AC
  int offsetShiftLuma2 =
    colorQuantShift.colorOffsetParamLumaDc * (1 << encoderShiftBit);  // Quantization Shift Luma DC
  int offsetShiftChroma2 = colorQuantShift.colorOffsetParamChromaDc *
    (1 << encoderShiftBit);  // Quantization Shift Chroma DC

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

void TEncAttribute::reflectanceCode(const int64_t& predictor, PC_REFL& currValue, int64_t& value,
                                    const bool isDuplicatePoint) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
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
  residualQuant = QuantizaResidual(absResidual, aps.reflQuantParam + abh.QpOffset, 1);
  int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
  value = signResidualQuant;
  // Local decode
  uint64_t inverseResidualQuant =
    InverseQuantizeResidual(residualQuant, aps.reflQuantParam + abh.QpOffset);
  int64_t recResidual = inverseResidualQuant * residualSign;
  currValue = TComClip((Int64)ClipMin, (Int64)ClipMax, recResidual + predictor);
}

void TEncAttribute::reflectanceTransformCode(std::vector<pointCodeWithIndex>& pointCloudCode,
                                             std::vector<int>& transformPointIdx, int& dcIndex,
                                             int& acIndex, int64_t transformBuf[1][8],
                                             int64_t transformPredBuf[1][8], int* Coefficients,
                                             std::vector<neighborSet>& neighborSet,
                                             PC_REFL& lastref) {
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
      transQuantParam = aps.reflQuantParam + abh.QpOffset + 72 + aps.QpOffsetDC;
    else
      transQuantParam = aps.reflQuantParam + abh.QpOffset + 72 + aps.QpOffsetAC;
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
        V3<bool> codeSign(true);
        if (os)
          std::swap(values[0], values[1]);
        colorResidualCorrelationCode(values, false, colorGolombNum, codeSign);

        run_length = 0;
      }
      if ((aps.coeffLengthControl > 0) && (run_length == lengthControl)) {
        m_encBac->encodeRunlength(run_length);
        run_length = 0;
      }
    }
  } else {
    auto reflGolombNum = aps.reflGolombNum;
    const bool refGolomb = reflGolombNum == 1 ? true : false;
    PC_REFL values;
    for (int n = 0; n < pointCount; ++n) {
      values = Coefficients[n];
      if (!values)
        ++run_length;
      else {
        if (m_encBacDual != NULL) {
          m_encBacDual->encodeRunlength(run_length);
          m_encBacDual->codeAttributerResidual(values, false, 3, 0, false, false, reflGolombNum);
          run_length = 0;
        } else {
          m_encBac->encodeRunlength(run_length);
          m_encBac->codeAttributerResidual(values, false, 3, 0, false, false, reflGolombNum);
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

void TEncAttribute::colorWaveletTransform() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameheader = m_hls->frameheader;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  int voxelCount = int(pointCloudRec.getNumPoint());
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
  int resLayerQuantParam = aps.colorQuantParam;
  int preColor[3];
  int64_t values[3];
  PC_COL recColor;
  PC_COL oriColor;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;

  //Hilbert Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReorderMode, pointCloudCode, voxelCount, 1);
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
    int* recAttributes = recAttributesBuf + segmentStartPosition * attribCount;
    int* position = positionBuf + segmentStartPosition * 3;
    int segmentVoxelCount = segmentLen;
    if (segIndex == (numSegments - 1)) {
      segmentVoxelCount = voxelCount - (numSegments - 1) * segmentLen;
    }
    // Transform.

    WaveletCoreTransform(attributes, 3, segmentVoxelCount, integerizedAttributes, sps, aps, abh,
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
        V3<bool> codeSign(true);
        if (os)
          std::swap(values[0], values[1]);
        colorResidualCorrelationCode(values, false, aps.colorGolombNum, codeSign);

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
          V3<bool> codeSign(true);
          if (os)
            std::swap(values[0], values[1]);
          colorResidualCorrelationCode(values, false, aps.colorGolombNum, codeSign);
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
}

void TEncAttribute::reflectanceWaveletTransform() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  const FrameHeader& frameheader = m_hls->frameheader;
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
  int golombnum = resLayer ? 3 : aps.reflGolombNum;

  //Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.reflReorderMode, pointCloudCode, voxelCount, aps.axisBias);
  for (int n = 0; n < voxelCount; n++) {
    const auto reflectance = pointCloudRec.getReflectance(pointCloudCode[n].index, multil_ID);
    attributesBuf[n] = FXPoint(reflectance);
    const auto& point = pointCloudRec[pointCloudCode[n].index];
    positionBuf[3 * n] = point[0];
    positionBuf[3 * n + 1] = point[1];
    positionBuf[3 * n + 2] = point[2];
  }
  // Transform.
  int resLayerQuantParam = aps.reflQuantParam + abh.QpOffset;
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
      for (int n = 0; n < segmentVoxelCount; n++) {
        int reflectance_rec = recAttributes[n];
        const auto reflectance_ori =
          pointCloudRec.getReflectance(pointCloudCode[n].index, multil_ID);
        integerizedAttributes[n] = (int)reflectance_ori - reflectance_rec;
      }
      int run_length = 0;
      for (int n = 0; n < segmentVoxelCount; ++n) {
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
}

void TEncAttribute::colorWaveletTransformFromReflectance() {
  cout << "colorWaveletTransformFromReflectance" << endl;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const FrameHeader& frameheader = m_hls->frameheader;
  const AttributeBrickHeader& abh = m_hls->abh;

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

  uint64_t crossAttrTypeLambda = 0;
  int64_t crossAttrTypeCoef = 0;
  crossAttrTypeLambda = (-(aps.reflQuantParam + abh.QpOffset) * aps.crossAttrTypePredParam1 +
                         aps.crossAttrTypePredParam2);
  auto diffPos = frameheader.geomBoundingBoxSize[0] + frameheader.geomBoundingBoxSize[1] +
    frameheader.geomBoundingBoxSize[2];
  uint64_t maxRefl = (1 << m_hls->aps.reflOutputDepth) - 1;
  crossAttrTypeCoef = round((diffPos << 10) / (double)maxRefl);
  crossAttrTypeCoef = int64_t(crossAttrTypeCoef * crossAttrTypeLambda + 524288) >> 20;

  bool resLayer = aps.transResLayer;
  int resLayerQuantParam = aps.colorQuantParam;
  Int64 ClipMin = 0;
  Int64 ClipMax = (1 << (m_hls->aps.colorOutputDepth)) - 1;
  int preColor[3];
  int64_t values[3];
  PC_COL recColor;
  PC_COL oriColor;
  //Hilbert Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.colorReorderMode, pointCloudCode, voxelCount, 1);

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
        V3<bool> codeSign(true);
        if (os)
          std::swap(values[0], values[1]);
        colorResidualCorrelationCode(values, false, aps.colorGolombNum, codeSign);

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
          V3<bool> codeSign(true);
          if (os)
            std::swap(values[0], values[1]);
          colorResidualCorrelationCode(values, false, aps.colorGolombNum, codeSign);

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
  const FrameHeader& frameheader = m_hls->frameheader;
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
  int golombnum = resLayer ? 3 : aps.reflGolombNum;

  //Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.reflReorderMode, pointCloudCode, voxelCount, aps.axisBias);

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
  int resLayerQuantParam = aps.reflQuantParam + abh.QpOffset;
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
      for (int n = 0; n < segmentVoxelCount; n++) {
        int reflectance_rec = recAttributes[n];
        const auto reflectance_ori =
          pointCloudRec.getReflectance(pointCloudCode[n].index, multil_ID);
        integerizedAttributes[n] = (int)reflectance_ori - reflectance_rec;
      }
      int run_length = 0;
      for (int n = 0; n < segmentVoxelCount; ++n) {
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

// new
void TEncAttribute::multiReflectancePredictingResidual() {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  TComPointCloud& pointCloudRec = *m_pointCloudRecon;
  const int voxelCount = int(pointCloudRec.getNumPoint());
  // Reorder
  std::vector<pointCodeWithIndex> pointCloudCode(voxelCount);
  reOrder(pointCloudRec.positions(), aps.reflReorderMode, pointCloudCode, voxelCount, aps.axisBias);

  // Initialize reflectance predictor
  int multil_ID_Group_Num = aps.multiAttriGroupNum[aps.multiAttrGroupID[multil_ID]];
  PC_REFL predictorRefl[5];
  PC_REFL recReflectance[5];
  std::vector<multiReflNeighborSet> neighborSet;
  int setlength = aps.maxNumOfNeighbours;
  neighborSet.resize(setlength);
  int setCount = 0;

  // predict parameters
  predictOptParams predictOptParams;
  predictOptParams.colorQP = 0;
  predictOptParams.maxNumOfNeighbours = aps.maxNumOfNeighbours;
  predictOptParams.minNeighborDisFlag = 0;
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
  int run_length = 0;
  for (int curIndex = 0; curIndex < voxelCount; ++curIndex) {
    auto pointIndex = pointCloudCode[curIndex].index;
    PC_REFL& curReflectance = pointCloudRec.getReflectance(pointIndex, multil_ID);
    const PC_POS& curPosition = pointCloudRec[pointIndex];
    getMultiReflectancePredictorFarthest(curIndex, curIndex, curPosition, setCount, neighborSet,
                                         predictOptParams, predictorRefl, multil_ID_Group_Num);
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
}

void TEncAttribute::multiReflectancePredictCode(PC_REFL* predictor, PC_REFL* currValue,
                                                int& run_length, int multil_ID_Group_Num) {
  const SequenceParameterSet& sps = m_hls->sps;
  const AttributeParameterSet& aps = m_hls->aps;
  const AttributeBrickHeader& abh = m_hls->abh;
  UInt golombNum = aps.reflGolombNum;
  Int64 ClipMin = INT32_MIN;
  Int64 ClipMax = INT32_MAX;
  if (aps.reflOutputDepth < 16) {
    ClipMin = 0;
    ClipMax = (1 << (aps.reflOutputDepth)) - 1;
  }

  int64_t signResidualQuantvalue[10];
  int residualPrevComponent = 0;
  for (int multi_id = 0; multi_id < multil_ID_Group_Num; multi_id++) {
    int64_t delta = currValue[multi_id] - predictor[multi_id];
    delta -= residualPrevComponent;
    // Residual quantization
    int residualSign = delta < 0 ? -1 : 1;
    uint64_t absResidual = std::abs(delta);
    uint64_t residualQuant;
    residualQuant = QuantizaResidual(absResidual, aps.reflQuantParam + abh.QpOffset, 1);
    int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
    signResidualQuantvalue[multi_id] = signResidualQuant;
    // Local decode
    uint64_t inverseResidualQuant =
      InverseQuantizeResidual(residualQuant, aps.reflQuantParam + abh.QpOffset);
    int64_t recResidual = inverseResidualQuant * residualSign;
    currValue[multi_id] = TComClip((Int64)ClipMin, (Int64)ClipMax,
                                   recResidual + predictor[multi_id] + residualPrevComponent);
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
