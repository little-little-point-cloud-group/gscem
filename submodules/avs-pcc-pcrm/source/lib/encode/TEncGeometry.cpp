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

#include "TEncGeometry.h"
#include "TEncTsp.h"
#include "common/CommonDef.h"
#include "common/contributors.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <queue>

///< \in TLibEncoder

/**
 * Implementation of TEncGeometry
 * geometry encoder
 */

//////////////////////////////////////////////////////////////////////////
// Public class functions
//////////////////////////////////////////////////////////////////////////

Void TEncGeometry::init(TComPointCloud* pointCloudOrg, TComPointCloud* pointCloudRecon,
                        HighLevelSyntax* hls, TEncBacTop* encBac) {
  m_pointCloudOrg = pointCloudOrg;
  m_pointCloudRecon = pointCloudRecon;
  m_pointCloudRecon->setNumPoint(m_pointCloudOrg->getNumPoint());
  if (pointCloudOrg->hasColors()) {
    m_pointCloudRecon->setNumMultilColor(m_pointCloudOrg->getNumMultilColor());
  }
  if (pointCloudOrg->hasReflectances()) {
    m_pointCloudRecon->setNumMultilRefl(m_pointCloudOrg->getNumMultilRefl());
  }
  m_hls = hls;
  m_encBac = encBac;

  if (pointCloudOrg->hasColors()) {
    pointCloudRecon->addColors();
  }
  if (pointCloudOrg->hasReflectances()) {
    pointCloudRecon->addReflectances();
  }
}

Void TEncGeometry::clear() {
  m_historyMap[0].release();
  m_historyMap[1].release();
}

Int TEncGeometry::compressAndEncodeGeometry() {
  const TComPointCloud& pcOrg = *m_pointCloudOrg;
  TComPointCloud& pcRec = *m_pointCloudRecon;
  codeBinOfIDCM infOfIDCM;
  infOfIDCM.nodeIdxOfIDCM = 0;
  infOfIDCM.NextNodeMode = 1;
  infOfIDCM.theTrueNumOf_IDCM_InLastTenNode = 0;
  infOfIDCM.oneChildNumInPreNode = 0;
  queue<TComOctreeNode> fifo;
  initFifoWithRootNode(fifo);

  TComOctreePartitionParams partitionParams;
  initOctreePartitionParams(*m_hls, partitionParams);
  m_historyMap[0] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());
  m_historyMap[1] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());

  m_numReconPoints = 0;
  bool contextSaved = false;
  bool planarModeEligibleForSlice = m_hls->gbh.planarModeEligibleForSlice;

  bool DcmEligibleKOctreeDepth = false;
  int m = pcOrg.getNumPoint();
  int numPointscodebydcm = 0;
  int numSubnodes = 0;

  for (; !fifo.empty(); fifo.pop()) {
    TComOctreeNode& currentNode = fifo.front();

    //< start LCU-based coding, within an LCU, breadth first coding is used.
    if (partitionParams.nodeSizeLog2.max() < m_hls->gps.lcuNodeSizeLog2) {
      if (m_hls->gps.saveStateFlag && !contextSaved) {
        m_encBac->saveContext();
        contextSaved = true;
      }
      if (!m_hls->gps.saveStateFlag && m_hls->gps.lcu_dependency_flag) {
        m_historyMap[0] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());
        m_historyMap[1] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());
        m_encBac->LcuReset();
      }
      bool usePredTree = true;
      //when two thresholds are all zeros, geomTreeType is used to decide tree type
      if (m_hls->gps.geomTreeDensityLow == 0 && m_hls->gps.geomTreeDensityHigh == 0) {
        usePredTree = (m_hls->gps.geomTreeType == 1);
      } else {
        //compute bounding box and the point density within the bounding box
        PC_POS bbMin, bbMax, bbSize, One(1);
        pcOrg.computeLcuBoundingBox(bbMin, bbMax, currentNode.childIdxBegin,
                                    currentNode.childIdxEnd);
        bbSize = bbMax + One - bbMin;
        size_t lcuSize = currentNode.childIdxEnd - currentNode.childIdxBegin;
        double lcuDensity = 1.0 * lcuSize / bbSize[0] / bbSize[1] / bbSize[2];

        usePredTree = (lcuDensity > m_hls->gps.geomTreeDensityLow) &&
          (lcuDensity < m_hls->gps.geomTreeDensityHigh);
      }

      m_encBac->encodeGeomTreeType(usePredTree ? 1 : 0);
      if (!usePredTree) {
        breadthFirstOctreeLcu(currentNode, partitionParams, m_hls->gps.saveStateFlag);
      } else {
        TreeEncoderParams encParams;
        encParams.sortMode = TreeEncoderParams::SortMode(m_hls->gps.geomTreeSortMode);
        UInt log2geomTreeMaxSizeMinus8 = m_hls->gps.log2geomTreeMaxSizeMinus8;
        encParams.maxPtsPerTree = 1 << (log2geomTreeMaxSizeMinus8 + 8);
        //TSP based predictive tree coding
        encodeTspLcu(m_pointCloudOrg, m_pointCloudRecon, m_numReconPoints, currentNode, m_hls,
                     m_encBac, partitionParams, encParams);
      }

      if (contextSaved) {
        m_encBac->restoreContext();
      }
      if (!m_hls->gps.saveStateFlag && m_hls->gps.lcu_dependency_flag) {
        m_historyMap[0].release();
        m_historyMap[1].release();
      }
      continue;
    }
    int currentOccupancy = 0;
    breadthFirstOctreeNode(currentNode, partitionParams, fifo, infOfIDCM, currentOccupancy,
                           planarModeEligibleForSlice, numPointscodebydcm, numSubnodes,
                           DcmEligibleKOctreeDepth);
    partitionParams.numNodesInCurrentDepth--;
    if (partitionParams.numNodesInCurrentDepth == 0) {
      if (m_hls->gbh.singleModeFlagInSlice) {
        if (m_hls->gbh.ifSparse1) {
          DcmEligibleKOctreeDepth = ((m - numPointscodebydcm) * 10 < numSubnodes * 500 ? 1 : 0);
          m_encBac->EligibleKOctreeDepthflag(DcmEligibleKOctreeDepth);
          numSubnodes = 0;
        } else {
          DcmEligibleKOctreeDepth = ((m - numPointscodebydcm) * 10 < numSubnodes * 15 ? 1 : 0);
          m_encBac->EligibleKOctreeDepthflag(DcmEligibleKOctreeDepth);
          numSubnodes = 0;
        }
      }
      updateOctreePartitionParams(partitionParams);
      m_historyMap[0] = std::move(m_historyMap[1]);
      m_historyMap[1] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());
    }
  }
  pcRec.setNumPoint(m_numReconPoints);

  m_encBac->codeSliceGeomEndCode();  ///< current slice geometry end code
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// Private class functions
//////////////////////////////////////////////////////////////////////////

Void TEncGeometry::initFifoWithRootNode(queue<TComOctreeNode>& fifo) {
  const TComPointCloud& pcOrg = *m_pointCloudOrg;
  TComOctreeNode rootNode;
  rootNode.pos = 0;
  rootNode.childIdxBegin = 0;
  rootNode.childIdxEnd = m_pointCloudOrg->getNumPoint();
  fifo.push(rootNode);
}

Void TEncGeometry::breadthFirstOctreeNode(const TComOctreeNode& currentNode,
                                          TComOctreePartitionParams& params,
                                          queue<TComOctreeNode>& fifo, codeBinOfIDCM& infOfIDCM,
                                          int& currentOccupancy, bool& preNodePlanarEligible,
                                          int& numdcmPoints, int& numSubnodes,
                                          bool& DcmEligibleKOctreeDepth) {
  Bool singlePointFlag = false;

  UInt occupancyCode = 0;
  bool currentMode = infOfIDCM.NextNodeMode;
  bool whetherCurrentNodeIDCMEligible = false;
  bool singlePointLevelEigible = false;

  singlePointLevelEigible = m_hls->gbh.singleModeFlagInSlice && DcmEligibleKOctreeDepth;

  bool currentNodeNeedPopcnt = currentMode && singlePointLevelEigible;

  if (singlePointLevelEigible) {
    singlePointFlag =
      singlePointMode(currentNode, params, infOfIDCM, occupancyCode, whetherCurrentNodeIDCMEligible,
                      currentNodeNeedPopcnt, currentMode);
    numdcmPoints++;
  }

  if (!singlePointFlag) {  // not a singleMode

    if (singlePointLevelEigible)
      numdcmPoints--;
    std::array<int, 8> childrenCount = {};

    treePartition(currentNode, params.childSizeLog2, params.occupancySkip, childrenCount);

    for (Int i = 0; i < 8; i++) {
      if (childrenCount[i] > 0) {
        occupancyCode |= 1 << i;
        numSubnodes++;
      }
    }
    TComGeomContext geomCtx;
    getContextInforFast(m_historyMap, params, currentNode, geomCtx, m_hls->gbh.geom_context_mode,
                        m_hls->gps.OccupancymapSizelog2, preNodePlanarEligible);
    if (m_hls->gbh.geom_context_mode == 1) {
      m_encBac->encodeOccUsingMemoryChannel(occupancyCode, params, geomCtx, preNodePlanarEligible);
    } else {
      m_encBac->encodeOccupancyCode(occupancyCode, params, geomCtx, preNodePlanarEligible,
                                    m_hls->gbh.geom_context_mode);
    }

    if ((currentNodeNeedPopcnt)) {
      if ((bitCount(occupancyCode) == 1))
        infOfIDCM.oneChildNumInPreNode++;
    }
    UInt childPointStartIdx = currentNode.childIdxBegin;
    for (Int i = 0; i < 8; i++) {
      if (childrenCount[i] == 0)
        continue;

      if (params.childSizeLog2 == 0) {
        encodeLeafNode(childPointStartIdx, childrenCount[i]);
        numdcmPoints = numdcmPoints + childrenCount[i];
        numSubnodes--;
        continue;
      }

      /// create new child
      fifo.emplace();
      auto& childNode = fifo.back();
      initChildNode(currentNode, childNode, params.childSizeLog2, i, childPointStartIdx,
                    childrenCount, occupancyCode, whetherCurrentNodeIDCMEligible);
      params.numNodesInNextDepth++;
    }
  }
  currentOccupancy = occupancyCode;
  updateContextInfor(m_historyMap[1], params, currentNode, m_hls->gps.OccupancymapSizelog2,
                     occupancyCode);
}

Bool TEncGeometry::singlePointMode(const TComOctreeNode& currentNode,
                                   TComOctreePartitionParams& params, codeBinOfIDCM& infOfIDCM,
                                   UInt& occupancyCode, bool& whetherCurrentNodeIDCMEligible,
                                   bool& currentNodeNeedPopcnt, bool currentMode) {
  UInt8& oneChildNumInPreNode = infOfIDCM.oneChildNumInPreNode;
  bool singlePointFlag = false;
  bool& nextNodeMode = infOfIDCM.NextNodeMode;
  UInt8& nodeIdxOfIDCM = infOfIDCM.nodeIdxOfIDCM;
  UInt8& theTrueNumOf_IDCM_InLastTenNode = infOfIDCM.theTrueNumOf_IDCM_InLastTenNode;
  if (currentMode == 0) {
    singlePointFlag =
      handleSingleMode(currentNode, params.nodeSizeLog2, params.singleModeFlagParent, occupancyCode,
                       params.childSizeLog2);
    nodeIdxOfIDCM++;
    if (singlePointFlag) {
      theTrueNumOf_IDCM_InLastTenNode++;
    }
    if (nodeIdxOfIDCM == 10) {
      nextNodeMode = !(theTrueNumOf_IDCM_InLastTenNode >= 3);
      nodeIdxOfIDCM = 0;
      theTrueNumOf_IDCM_InLastTenNode = 0;
    } else
      nextNodeMode = 0;
    whetherCurrentNodeIDCMEligible = true;
  } else {
    nodeIdxOfIDCM++;
    if (nodeIdxOfIDCM == 5) {
      nodeIdxOfIDCM = 0;
      if (oneChildNumInPreNode >= 4) {
        singlePointFlag =
          handleSingleMode(currentNode, params.nodeSizeLog2, params.singleModeFlagParent,
                           occupancyCode, params.childSizeLog2);
        nextNodeMode = !singlePointFlag;
        whetherCurrentNodeIDCMEligible = true;
      }
      oneChildNumInPreNode = 0;
      currentNodeNeedPopcnt = 0;  //n-th node's occupancy is not need to be count
    }
  }
  return singlePointFlag;
}

Bool TEncGeometry::handleSingleMode(const TComOctreeNode& currentNode, const V3<UInt>& nodeSizeLog2,
                                    const Bool singleModeFlagParent, UInt& occupancyCode,
                                    const V3<UInt>& childSizeLog2) {
  Bool singlePointFlag = (currentNode.childIdxEnd - currentNode.childIdxBegin == 1);

  Bool singlePointFlagInferred = false;
  if (currentNode.parentNodeIDCMEligible) {
    // if parent node has only one occupied child, current node is not single
    // node for sure
    int parent_child_count = bitCount(currentNode.parentOccupancy);
    if (parent_child_count == 1) {
      singlePointFlagInferred = true;
    }
  }
  if (!singlePointFlagInferred) {
    m_encBac->encodeSinglePointFlag(singlePointFlag);
  }

  if (singlePointFlag) {
    const TComPointCloud& pcOrg = *m_pointCloudOrg;
    TComPointCloud& pcRec = *m_pointCloudRecon;
    V3<UInt> pos;
    pos[0] = (UInt)pcOrg[currentNode.childIdxBegin][0];
    pos[1] = (UInt)pcOrg[currentNode.childIdxBegin][1];
    pos[2] = (UInt)pcOrg[currentNode.childIdxBegin][2];
    m_encBac->encodeSinglePointIndex(pos, nodeSizeLog2);
    V3<UInt> lowerPos;
    lowerPos[0] = (pos[0] - currentNode.pos[0]);
    lowerPos[1] = (pos[1] - currentNode.pos[1]);
    lowerPos[2] = (pos[2] - currentNode.pos[2]);
    occupancyCode = 1 << ((lowerPos[0] >> childSizeLog2[0]) << 2) +
        ((lowerPos[1] >> childSizeLog2[1]) << 1) + ((lowerPos[2] >> childSizeLog2[2]));
    pcRec[m_numReconPoints] = pcOrg[currentNode.childIdxBegin];
    if (!m_hls->sps.geomRemoveDuplicateFlag || m_hls->sps.recolorMode) {
      ///< keep duplicate points
      if (pcOrg.hasColors()) {
        for (int multiIdx = 0; multiIdx < m_pointCloudOrg->getNumMultilColor(); ++multiIdx) {
          const auto color0 = pcOrg.getColor(currentNode.childIdxBegin, multiIdx);
          pcRec.setColor(m_numReconPoints, color0, multiIdx);
        }
      }
      if (pcOrg.hasReflectances()) {
        for (int multiIdx = 0; multiIdx < m_pointCloudOrg->getNumMultilRefl(); ++multiIdx) {
          const auto reflectance0 = pcOrg.getReflectance(currentNode.childIdxBegin, multiIdx);
          pcRec.setReflectance(m_numReconPoints, reflectance0, multiIdx);
        }
      }
    }
    ++m_numReconPoints;
  }
  return singlePointFlag;
}

Void TEncGeometry::encodeLeafNode(UInt& childPointStartIdx, const UInt& childNum) {
  TComPointCloud& pcOrg = *m_pointCloudOrg;
  TComPointCloud& pcRec = *m_pointCloudRecon;

  ///< reaching the leaf nodes
  if (m_hls->sps.geomRemoveDuplicateFlag) {
    ///< remove duplicate points
    assert(childNum == 1);
    const auto& idx = childPointStartIdx;
    pcRec[m_numReconPoints] = pcOrg[idx];
    if (pcOrg.hasColors()) {
      for (int multilIdx = 0; multilIdx < m_pointCloudOrg->getNumMultilColor(); ++multilIdx) {
        const auto color0 = pcOrg.getColor(idx, multilIdx);
        pcRec.setColor(m_numReconPoints, color0, multilIdx);
      }
    }
    if (pcOrg.hasReflectances()) {
      for (int multilIdx = 0; multilIdx < m_pointCloudOrg->getNumMultilRefl(); ++multilIdx) {
        const auto reflectance0 = pcOrg.getReflectance(idx, multilIdx);
        pcRec.setReflectance(m_numReconPoints, reflectance0, multilIdx);
      }
    }
    m_numReconPoints++;
    ++childPointStartIdx;
  } else {
    ///< keep duplicate points
    m_encBac->encodeDuplicateNumber(UInt(childNum));
    // sort duplicate points attribute
    if (childNum > 1) {
      UInt m_numReconStartIdx = m_numReconPoints;
      for (int idx = 0; idx < childNum; ++idx)
        pcRec[m_numReconPoints + idx] = pcOrg[idx + childPointStartIdx];
      if (pcOrg.hasColors()) {
        for (int multilIdx = 0; multilIdx < m_pointCloudOrg->getNumMultilColor(); ++multilIdx) {
          vector<PC_COL> colors(childNum);
          for (int idx = 0; idx < childNum; ++idx) {
            const auto color0 = pcOrg.getColor(idx + childPointStartIdx, multilIdx);
            colors[idx] = color0;
          }
          std::sort(colors.begin(), colors.end());
          for (int idx = 0; idx < childNum; ++idx) {
            pcRec.setColor(m_numReconPoints + idx, colors[idx], multilIdx);
          }
        }
      }
      if (pcOrg.hasReflectances()) {
        for (int multilIdx = 0; multilIdx < m_pointCloudOrg->getNumMultilRefl(); ++multilIdx) {
          vector<PC_REFL> refls(childNum);
          for (int idx = 0; idx < childNum; ++idx) {
            const auto ref0 = pcOrg.getReflectance(idx + childPointStartIdx, multilIdx);
            refls[idx] = ref0;
          }
          std::sort(refls.begin(), refls.end());
          for (int idx = 0; idx < childNum; ++idx) {
            pcRec.setReflectance(m_numReconPoints + idx, refls[idx], multilIdx);
          }
        }
      }
      m_numReconPoints += childNum;
    } else {
      const auto& idx = childPointStartIdx;
      pcRec[m_numReconPoints] = pcOrg[idx];
      if (pcOrg.hasColors()) {
        for (int multilIdx = 0; multilIdx < m_pointCloudOrg->getNumMultilColor(); ++multilIdx) {
          const auto color0 = pcOrg.getColor(idx, multilIdx);
          pcRec.setColor(m_numReconPoints, color0, multilIdx);
        }
      }
      if (pcOrg.hasReflectances()) {
        for (int multilIdx = 0; multilIdx < m_pointCloudOrg->getNumMultilRefl(); ++multilIdx) {
          const auto reflectance0 = pcOrg.getReflectance(idx, multilIdx);
          pcRec.setReflectance(m_numReconPoints, reflectance0, multilIdx);
        }
      }
      m_numReconPoints++;
    }
    childPointStartIdx += childNum;
  }
}

Void TEncGeometry::initChildNode(const TComOctreeNode& currentNode, TComOctreeNode& childNode,
                                 const V3<UInt>& childSizeLog2, const int childIdx,
                                 UInt& startChildIdx, const std::array<int, 8>& childrenCount,
                                 const UInt8 occupancyCode, Bool whetherParentNodeIDCMEligible) {
  Int x = !!(childIdx & 4);
  Int y = !!(childIdx & 2);
  Int z = !!(childIdx & 1);

  childNode.pos[0] = currentNode.pos[0] + (x << childSizeLog2[0]);
  childNode.pos[1] = currentNode.pos[1] + (y << childSizeLog2[1]);
  childNode.pos[2] = currentNode.pos[2] + (z << childSizeLog2[2]);

  childNode.childIdxBegin = startChildIdx;
  startChildIdx += childrenCount[childIdx];
  childNode.childIdxEnd = startChildIdx;
  childNode.parentOccupancy = occupancyCode;
  childNode.parentNodeIDCMEligible = whetherParentNodeIDCMEligible;
}
///>===================================================================================
Void TEncGeometry::treePartition(const TComOctreeNode& currentNode,
                                 const V3<UInt>& childNodeSizeLog2, const UInt& occupancySkip,
                                 std::array<int, 8>& childrenCount) {
  V3<UInt> bitMask = 1 << childNodeSizeLog2;
  for (int k = 0; k < 3; k++) {
    if (occupancySkip & (4 >> k))
      bitMask[k] = 0;
  }
  octreeDivision(m_pointCloudOrg->positions().begin() + currentNode.childIdxBegin,
                 m_pointCloudOrg->positions().begin() + currentNode.childIdxEnd, childrenCount,
                 currentNode.childIdxBegin, [=](PC_POS point) {
                   Int childNodeIdx = (!!(Int(point[0]) & bitMask[0])) << 2;
                   childNodeIdx |= (!!(Int(point[1]) & bitMask[1])) << 1;
                   childNodeIdx |= (!!(Int(point[2]) & bitMask[2]));
                   return childNodeIdx;
                 });
}
Void TEncGeometry::breadthFirstOctreeLcu(const TComOctreeNode& node,
                                         TComOctreePartitionParams params, Bool saveStateFlag) {
  queue<TComOctreeNode> fifo;
  fifo.push(node);

  bool DcmEligibleKOctreeDepth = false;
  int m = node.childIdxEnd - node.childIdxBegin;
  int numPointscodebydcm = 0;
  int numSubnodes = 0;

  params.numNodesInCurrentDepth = 1;
  params.numNodesInNextDepth = 0;
  bool historyMapSaved = false;
  bool planarModeEligibleForSlice = m_hls->gbh.planarModeEligibleForSlice;
  codeBinOfIDCM infOfIDCM;
  infOfIDCM.nodeIdxOfIDCM = 0;
  infOfIDCM.NextNodeMode = 1;
  infOfIDCM.theTrueNumOf_IDCM_InLastTenNode = 0;
  infOfIDCM.oneChildNumInPreNode = 0;
  for (; !fifo.empty(); fifo.pop()) {
    TComOctreeNode currentNode = fifo.front();
    int currentOccupancy = 0;
    breadthFirstOctreeNode(currentNode, params, fifo, infOfIDCM, currentOccupancy,
                           planarModeEligibleForSlice, numPointscodebydcm, numSubnodes,
                           DcmEligibleKOctreeDepth);

    params.numNodesInCurrentDepth--;
    if (params.numNodesInCurrentDepth == 0) {
      if (m_hls->gbh.singleModeFlagInSlice) {
        if (m_hls->gbh.ifSparse1) {
          DcmEligibleKOctreeDepth = ((m - numPointscodebydcm) * 10 < numSubnodes * 500 ? 1 : 0);
          m_encBac->EligibleKOctreeDepthflag(DcmEligibleKOctreeDepth);
          numSubnodes = 0;
        } else {
          DcmEligibleKOctreeDepth = ((m - numPointscodebydcm) * 10 < numSubnodes * 15 ? 1 : 0);
          m_encBac->EligibleKOctreeDepthflag(DcmEligibleKOctreeDepth);
          numSubnodes = 0;
        }
      }
      updateOctreePartitionParams(params);

      if (saveStateFlag && !historyMapSaved) {
        m_historyMapBackup = std::move(m_historyMap[0]);
        historyMapSaved = true;
      }
      m_historyMap[0] = std::move(m_historyMap[1]);
      m_historyMap[1] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());
    }
  }

  if (historyMapSaved) {
    m_historyMap[0] = std::move(m_historyMapBackup);
    m_historyMap[1] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());
  }
}
///< \}
