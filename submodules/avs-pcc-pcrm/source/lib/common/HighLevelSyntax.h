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
  UInt spsID;
  UInt level;

  V3<Int> geomBoundingBoxOrigin;  ///< geometry origin
  V3<UInt> geomBoundingBoxSize;   ///< geometry bounding box size

  Float geomQuantStep;            ///< geometry quantization stepsize (in voxelization)
  Bool geomRemoveDuplicateFlag;   ///< remove duplicate points (=1) or keep duplicate points (=0)
  Bool attrPresentFlag;           ///< 0: without any attributes, 1: with at least one kind of attribute
  Bool recolorMode;               ///< 0: regular recolor, 1: fast recolor
  UInt colorQuantParam;           ///< color quantization stepsize (attribute resudial value)
  UInt reflQuantParam;            ///< reflectance quantization stepsize (attribute resudial value)
  UInt maxNumAttrMinus1;          ///< maximum number of attribute categories
  Int reflThreshold;                ///< color Qstep
  Float colorOffsetParamLumaAc;
  Float colorOffsetParamLumaDc;
  Float colorOffsetParamChromaAc;
  Float colorOffsetParamChromaDc;

  Bool sps_multi_set_flag;    ///<  0: indicate multi attribute is off, 1 indicate multi attribute is on.

};

/**
 * Geometry Parameter Set
 */
struct GeometryParameterSet {
  UInt gpsID;
  UInt spsID;
  UInt lcuNodeSizeLog2;         ///< the LCU size for node-based geometry coding
  UInt lcuNodeDepth;            /// the LCU size for node-based geometry coding. It is disabled if lcuNodeSizeLog2 > 0, "
                                /// When lcuNodeSizeLog2 = 0, it is enabled. The actual lcuNodeSizeLog2 = maxNodeSize + 1 - lcuNodeDepth") 
  UInt geomTreeType;            ///< tree type for node-based geometry coding£¬0: octree, 1: preditive tree"
  UInt geomTreeSortMode;        ///< sort method before preditive tree coding£¬0: NoSort, 1: MortonSort"
  UInt log2geomTreeMaxSizeMinus8;///< log2 maximum geometry tree size in terms of number of points minus8
  double geomTreeDensityLow;    ///< tree density low threshold
  double geomTreeDensityHigh;   ///< tree density high threshold
  Bool im_qtbt_flag;            ///< control flag for implicit QTBT partition
  UInt im_qtbt_num_before_ot;   ///< maximum number of implicit QTBT before OT
  UInt im_qtbt_min_size;        ///< minimum size in log2 of implicit QTBT
  Bool singleModeFlag;          ///< control flag for single point encode mode
  Bool saveStateFlag;           ///< control whether to save coding state 
  Bool lcu_dependency_flag;///< control whether to save coding state 
  Bool planarSeqEligible;
  UInt OccupancymapSizelog2;
};

/**

 * Geometry Brick Set

 */
struct GeometryBrickHeader {
  UInt gbhID;
  UInt sliceID;
  V3<Int> geomBoundingBoxOrigin;  ///< slice geometry origin
  V3<UInt> nodeSizeLog2;          ///< slice  node size (log2) for xyz dimensions
  UInt geomNumPoints;             ///< number of points in a slice
  UInt geom_context_mode;       ///< geometry context mode
  UInt singleModeFlagInSlice;   ///< control flag for single point encode mode in Slice
  Bool planarModeEligibleForSlice;
  Bool ifSparse1; 
  UInt im_qtbt_num_before_ot;   ///< maximum number of implicit QTBT before OT
  UInt im_qtbt_min_size;        ///< minimum size in log2 of implicit QTBT
};

/**
* Attribute Parameter Set
*/
struct AttributeParameterSet {
  UInt apsID;
  UInt spsID;
  UInt maxNumOfNeighboursLog2Minus7[NUM_MULTIATTRIBUTE] = {0};
  UInt maxNumOfNeighbours;
  Bool crossComponentPred;
  Bool orderSwitch;
  Int chromaQpOffsetCb;
  Int chromaQpOffsetCr;
  UInt outputBitDepthMinus1[NUM_MULTIATTRIBUTE] = {0};
  UInt colorOutputDepth;
  UInt reflOutputDepth;
  UInt colorInitGolombOffset;
  UInt reflInitGolombOffset;
  UInt nearestPredParam1;
  UInt nearestPredParam2;
  UInt axisBias;
  UInt transform;
  UInt transformSegmentSize;
  UInt attrTransformQpDelta;      ///< attribute transform coefficent quantization QP delta, relative to attrQuantParam
  Int QpOffsetDC;
  Int QpOffsetAC;
  Int chromaQpOffsetDC;
  Int chromaQpOffsetAC;
  UInt colorMaxTransNum;
  UInt reflMaxTransNum;
  UInt maxNumofCoeff = 1;
  UInt log2maxNumofCoeffMinus8;
  UInt coeffLengthControl = 1;
  UInt log2coeffLengthControlMinus8;
  UInt attributePresentFlag[NUM_MULTIATTRIBUTE] = {0};
  UInt colorReordermode;
  UInt refReordermode;
  bool refGroupPredict;
  bool attrEncodeOrder;
  bool crossAttrTypePred;
  UInt crossAttrTypePredParam1;
  UInt crossAttrTypePredParam2;
  UInt colorGolombNum;
  UInt refGolombNum;
  UInt deadZoneLen;
  Int colorInitPredTransRatio;
  Int refInitPredTransRatio;
  bool transResLayer;
  UInt log2golombGroupSize;
  bool chromaDeadzoneFlag;       // encode only
  bool colorQPAdjustFlag;
  UInt colorQPAdjustScalar;
  UInt kFracBits;
  UInt predFixedPointFracBit;
  UInt log2predDistWeightGroupSize;
  UInt maxMultiNumOfNeighboursLog2Minus7[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt maxMultiNumOfNeighbours[NUM_MULTIATTRIBUTE] = {0};
  Bool crossMultiComponentPred[NUM_MULTIATTRIBUTE] = {0};
  Bool orderMultiSwitch[NUM_MULTIATTRIBUTE] = {0};
  Int chromaMultiQpOffsetCb[NUM_MULTIATTRIBUTE] = {0};
  Int chromaMultiQpOffsetCr[NUM_MULTIATTRIBUTE] = {0};
  UInt outputMultiBitDepthMinus1[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt colorMultiOutputDepth[NUM_MULTIATTRIBUTE] = {0};
  UInt reflMultiOutputDepth[NUM_MULTIATTRIBUTE] = {0};
  UInt nearestMultiPredParam1[NUM_MULTIATTRIBUTE] = {0};
  UInt nearestMultiPredParam2[NUM_MULTIATTRIBUTE] = {0};
  UInt axisMultiBias[NUM_MULTIATTRIBUTE] = {0};
  UInt transformMulti[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt kMultiFracBits[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt attrMultiTransformQpDelta[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};     ///< attribute transform coefficent quantization QP delta, relative to attrQuantParam
  UInt transformMultiSegmentSize[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt reflMaxMultiTransNum[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt colorMaxMultiTransNum[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt log2MultimaxNumofCoeffMinus8[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt maxMultiNumofCoeff[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{1}};
  Int QpMultiOffsetDC[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  Int QpMultiOffsetAC[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  bool transMultiResLayer[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt log2coeffMultiLengthControlMinus8[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt coeffMultiLengthControl[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{1}};
  Int chromaMultiQpOffsetDC[NUM_MULTIATTRIBUTE] = {0};
  Int chromaMultiQpOffsetAC[NUM_MULTIATTRIBUTE] = {0};
  UInt attributeMultiPresentFlag[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE] = {{0}};
  UInt colorMultiReordermode[NUM_MULTIATTRIBUTE] = {0};
  UInt refMultiReordermode[NUM_MULTIATTRIBUTE] = {0};
  bool refMultiGroupPredict[NUM_MULTIATTRIBUTE] = {0};
  bool attrMultiEncodeOrder[NUM_MULTIATTRIBUTE] = {0};
  UInt colorMultiGolombNum[NUM_MULTIATTRIBUTE] = {0};
  UInt refMultiGolombNum[NUM_MULTIATTRIBUTE] = {0};
  UInt deadMultiZoneLen[NUM_MULTIATTRIBUTE] = {0};
  UInt log2golombMultiGroupSize[NUM_MULTIATTRIBUTE] = {0};
  bool chromaMultiDeadzoneFlag[NUM_MULTIATTRIBUTE] = {0};       // encode only 
  UInt log2predDistWeightMultiGroupSize[NUM_MULTIATTRIBUTE] = {0};
  Int colorMultiInitPredTransRatio[NUM_MULTIATTRIBUTE] = {0};
  Int refMultiInitPredTransRatio[NUM_MULTIATTRIBUTE] = {0};
  bool colorMultiQPAdjustFlag[NUM_MULTIATTRIBUTE] = {0};
  UInt colorMultiQPAdjustScalar[NUM_MULTIATTRIBUTE] = {0};
  UInt predMultiFixedPointFracBit[NUM_MULTIATTRIBUTE] = {0};
  bool multi_data_set_flag[NUM_MULTIATTRIBUTE]= {0};
  UInt attribute_num_data_set_minus1[NUM_MULTIATTRIBUTE] = {0};
  UInt attribute_num_set_minus1[NUM_MULTIATTRIBUTE] = {0};
  UInt multiAttriGroupID[NUM_MULTIATTRIBUTE] = {0};
  UInt multiAttriGroupNum[NUM_MULTIATTRIBUTE] = {0};
  UInt numofMultiAttriGroup=0;
};

struct FrameHeader
{
  UInt frame_index;
  UInt num_slice_minus_one;
  Bool timestamp_flag;
  UInt32 timestamp;
  UInt geomNumPoints;             ///< total number of points
};
/**

 * Attribute Brick Header

 */
struct AttributeBrickHeader {
  UInt abhID;
  UInt sliceID;
  Int reflQPoffset;
  UInt attribute_ID[NUM_ATTRIBUTE][NUM_MULTIATTRIBUTE]={{0}};
};

/**
 * High-Level Syntax
 * contains SPS, GPS, ...
 */
struct HighLevelSyntax {
  SequenceParameterSet sps;
  GeometryParameterSet gps;
  AttributeParameterSet aps;
  FrameHeader  frameHead;
  GeometryBrickHeader gbh;
  AttributeBrickHeader abh;
};
// clang-format on
