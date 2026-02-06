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

#include "CommonDef.h"
#include "TComVector.h"
#include "contributors.h"

// clang-format off
/**
 * Sequence Parameter Set
 */
struct SequenceParameterSet {
  UInt profileId = 0;
  UInt levelId = 0;
  UInt frameRateCode = 0;
  Bool geomRemoveDuplicateFlag = true;   ///< remove duplicate points (=1) or keep duplicate points (=0)
  Bool attrPresentFlag = true;           ///< 0: without any attributes, 1: with at least one kind of attribute
  UInt maxNumAttributesMinus1 = 1;       ///< maximum number of attribute categories
  Bool multiAttributesSetFlag = 1;       ///<  0: indicate multi attribute is off, 1 indicate multi attribute is on.

  Bool recolorMode = true;               ///< 0: regular recolor, 1: fast recolor
};

/**
 * Geometry Parameter Set
 */
struct GeometryParameterSet {
  Float geomQuantStep = 0;               ///< geometry quantization stepsize (in voxelization)
  UInt geomQuantStepSignificand = 0;     ///< geometry quantization stepsize' significand
  UInt geomQuantStepExponent = 0;        ///< geometry quantization stepsize' exponent
  UInt geomMaxTreeSizeLog2Minus8 = 3;    ///< log2 maximum geometry tree size in terms of number of points minus8
  Bool implicitGeomPartitionFlag = true; ///< control flag for implicit QTBT partition
  Bool singleModeFlag =true;             ///< control flag for single point encode mode
  UInt occupancySearchRangeLog2 = 8;
  Bool saveStateFlag = false;            ///< control whether to save coding state 
  Bool lcuDependencyFlag= false;         ///< control whether to save coding state 

  UInt lcuNodeSizeLog2 = 0;              ///< the LCU size for node-based geometry coding
  UInt lcuNodeDepth = 0;                 /// the LCU size for node-based geometry coding. It is disabled if lcuNodeSizeLog2 > 0, "
  UInt imQtbtNumBeforeOt = 0;            ///< maximum number of implicit QTBT before OT
  UInt imQtbtMinSize = 0;                ///< minimum size in log2 of implicit QTBT
 
  //encoder only
  Bool planarSeqEligible = false;
  UInt geomTreeType = 1;                 ///< tree type for node-based geometry coding£¬0: octree, 1: preditive tree"
  UInt geomTreeSortMode = 1;             ///< sort method before preditive tree coding£¬0: NoSort, 1: MortonSort"
  double geomTreeDensityLow = 1e-9;      ///< tree density low threshold
  double geomTreeDensityHigh = 1e-7;     ///< tree density high threshold
};

/**
* Attribute Parameter Set
*/
struct AttributeParameterSet {
  Bool updateMultilAttrParams = false;
  //multiple 
  UInt attributeDataPresentFlag[NUM_ATTRIBUTE] = {0,0};
  Int attributeDataNumSetMinus1[NUM_ATTRIBUTE] = {-1,-1};
  UInt multiAttrGroupID[NUM_ATTRIBUTE] = {0,0};
  Bool multiDataSetFlag[NUM_ATTRIBUTE]= {0,0};
  UInt attributeInfoNumSetMinus1[NUM_ATTRIBUTE] = {0,0};
  UInt outputMultiBitDepthMinus1[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  UInt attrMultiQuantParam[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  Bool orderMultiSwitch[NUM_MULTIATTRIBUTE] = {0};
  UInt colorMultiReordermode[NUM_MULTIATTRIBUTE] = {0};
  UInt colorMultiGolombNum[NUM_MULTIATTRIBUTE] = {0};
  UInt golombMultiGroupSizeLog2[NUM_MULTIATTRIBUTE] = {0};
  UInt axisMultiBiasMinus1[NUM_MULTIATTRIBUTE] = {0};
  UInt reflMultiReordermode[NUM_MULTIATTRIBUTE] = {0};
  UInt reflMultiGolombNum[NUM_MULTIATTRIBUTE] = {0};
  UInt predMultiFixedPointFracBit[NUM_MULTIATTRIBUTE] = {0};
  UInt transformMulti[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  UInt maxMultiNumOfNeighboursLog2Minus7[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  Bool crossMultiComponentPred[NUM_MULTIATTRIBUTE] = {0};
  Int chromaMultiQpOffsetCb[NUM_MULTIATTRIBUTE] = {0};
  Int chromaMultiQpOffsetCr[NUM_MULTIATTRIBUTE] = {0};
  UInt nearestMultiPredParam1[NUM_MULTIATTRIBUTE] = {0};
  UInt nearestMultiPredParam2[NUM_MULTIATTRIBUTE] = {0};
  UInt predDistWeightMultiGroupSizeLog2[NUM_MULTIATTRIBUTE] = {0};
  UInt transformMultiSegmentSize[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  UInt kMultiFracBits[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  UInt attrMultiTransformQpDelta[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  Bool transMultiResLayer[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  UInt MultimaxNumofCoeffLog2Minus8[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  Int QpMultiOffsetDC[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  Int QpMultiOffsetAC[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  UInt colorMaxMultiTransNum[NUM_MULTIATTRIBUTE] = {0};
  Int chromaMultiQpOffsetDC[NUM_MULTIATTRIBUTE] = {0};
  Int chromaMultiQpOffsetAC[NUM_MULTIATTRIBUTE] = {0};
  Bool colorMultiQPAdjustFlag[NUM_MULTIATTRIBUTE] = {0};
  UInt reflMaxMultiTransNum[NUM_MULTIATTRIBUTE] = {0};
  Bool reflMultiGroupPredict[NUM_MULTIATTRIBUTE] = {0};
  UInt coeffMultiLengthControlLog2Minus8[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};

  UInt outputBitDepthMinus1[NUM_ATTRIBUTE] = {0,0};
  UInt attrQuantParam[NUM_ATTRIBUTE] = {0,0};
  UInt coeffMultiLengthControl[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  UInt maxMultiNumofCoeff[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0},{0}};
  UInt colorMultiOutputDepth[NUM_MULTIATTRIBUTE] = {0};
  UInt reflMultiOutputDepth[NUM_MULTIATTRIBUTE] = {0};
  UInt colorMultiQuantParam[NUM_MULTIATTRIBUTE] = {0};
  UInt reflMultiQuantParam[NUM_MULTIATTRIBUTE] = {0};
  UInt multiAttriGroupNum[NUM_MULTIATTRIBUTE] = {0};

  //cfg files 
  UInt colorOutputDepth = 8;
  UInt reflOutputDepth = 16;
  UInt colorQuantParam = 1;
  UInt reflQuantParam = 1;
  Bool orderSwitch = true;
  UInt colorReorderMode = 1;
  UInt colorGolombNum = 0;
  UInt golombGroupSizeLog2 = 4;
  UInt axisBiasMinus1 = 0;
  UInt reflReorderMode = 1;
  UInt reflGolombNum = 0;
  UInt predFixedPointFracBit = 0;
  UInt transform = 0;
  UInt maxNumOfNeighboursLog2Minus7 = 0;
  Bool crossComponentPred = false;
  Int chromaQpOffsetCb = 0;
  Int chromaQpOffsetCr = 0;
  UInt nearestPredParam1 = 0;
  UInt nearestPredParam2 = 0;
  UInt predDistWeightGroupSizeLog2 = 0;
  UInt transformSegmentSize = 0;
  UInt kFracBits = 0;
  UInt attrTransQpDelta = 0;
  Bool transResLayer = false;
  UInt maxNumofCoeffLog2Minus8 = 0;
  Int QpOffsetDC = 0;
  Int QpOffsetAC = 0;
  UInt colorMaxTransNum = 0;
  Int chromaQpOffsetDC = 0;
  Int chromaQpOffsetAC = 0;
  Bool colorQPAdjustFlag = false;
  UInt reflMaxTransNum = 0;
  Bool reflGroupPredict = false;
  UInt coeffLengthControlLog2Minus8 = 0;
  Bool attrEncodeOrder = true;
  Bool crossAttrTypePred = true;
  UInt crossAttrTypePredParam1 = 3277;
  UInt crossAttrTypePredParam2 = 1258291;

  UInt maxNumOfNeighbours = 128;
  UInt axisBias = 1;
  UInt maxNumofCoeff = 1;
  UInt coeffLengthControl = 1;

  //encoder only
  UInt colorInitGolombOffset = 0;
  UInt reflInitGolombOffset = 0; 
  UInt deadZoneLen = 0;
  Bool chromaDeadzoneFlag = false;
  Bool eligibleDupPointPred = true;
};

/**
 * Frame Parameter Set
 */
struct FrameHeader
{
  UInt frameIndex = 0;
  UInt frameNumSliceMinus1 = 0;
  UInt lcuNodeSizeLog2 = 0;         ///< the LCU size for node-based geometry coding. set here for flexible adjust.
  UInt geomNumPoints = 0;              ///< total number of points
  V3<Int> geomBoundingBoxOrigin = { 0,0,0 };  ///< geometry origin
  V3<UInt> geomBoundingBoxSize = { 0,0,0 };   ///< geometry bounding box size
};

/**
 * Geometry Brick Set
 */
struct GeometryBrickHeader {
  UInt sliceID = 0;
  Bool contextMode = 0;       ///< geometry context mode
  UInt imQtbtNumBeforeOt = 0;   ///< maximum number of implicit QTBT before OT
  UInt imQtbtMinSize = 0;        ///< minimum size in log2 of implicit QTBT
  V3<Int> geomBoundingBoxOrigin = {0,0,0};  ///< slice geometry origin
  V3<UInt> nodeSizeLog2 = {0,0,0};          ///< slice  node size (log2) for xyz dimensions
  UInt geomNumPoints = 0;             ///< number of points in a slice
  
  UInt singleModeFlagInSlice = 1;   ///< control flag for single point encode mode in Slice
  Bool planarModeEligibleForSlice = false;
  Bool ifSparse1 = true; 
};

/**
 * Attribute Brick Header

 */
struct AttributeBrickHeader {
  UInt sliceID = 0;
  UInt attributeID = 0;
  Int QpOffset = 0;
  Int colorInitPredTransRatio = 2;
  Int reflInitPredTransRatio = 0;
  UInt colorQPAdjustScalar = 0;
};

/**
 * High-Level Syntax
 * contains SPS, GPS, ...
 */
struct HighLevelSyntax {
  SequenceParameterSet sps;
  GeometryParameterSet gps;
  AttributeParameterSet aps;
  FrameHeader frameheader;
  GeometryBrickHeader gbh;
  AttributeBrickHeader abh;
};
// clang-format on
