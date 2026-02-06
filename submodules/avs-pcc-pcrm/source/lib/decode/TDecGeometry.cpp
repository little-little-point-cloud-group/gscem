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

#include "TDecGeometry.h"
#include "TDecTsp.h"
#include "common/contributors.h"
#include <algorithm>
#include <queue>

///< \in TLibDecoder \{

/**
 * Implementation of TDecGeometry
 * geometry decoder
 */

//////////////////////////////////////////////////////////////////////////
// Public class functions
//////////////////////////////////////////////////////////////////////////
Int TDecGeometry::getNumBits(UInt num) {
  int numBits = 0;
  while (num) {
    num = num >> 1;
    numBits++;
  }

  return numBits;
}

Void TDecGeometry::decodeAndDecompress() {
  TComPointCloud& pcRec = *m_pointCloudRecon;
  codeBinOfIDCM infOfIDCM;
  infOfIDCM.nodeIdxOfIDCM = 0;
  infOfIDCM.NextNodeMode = 1;
  infOfIDCM.theTrueNumOf_IDCM_InLastTenNode = 0;
  infOfIDCM.oneChildNumInPreNode = 0;
  queue<TComOctreeNode> fifo;

  TComOctreeNode rootNode;
  rootNode.pos = (UInt)0;
  fifo.push(rootNode);

  pcRec.setNumPoint(m_hls->gbh.geomNumPoints);

  TComOctreePartitionParams partitionParams;
  initOctreePartitionParams(*m_hls, partitionParams);
  m_historyMap[0] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());
  m_historyMap[1] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());

  bool DcmEligibleKOctreeDepth = false;

  m_numReconPoints = 0;
  bool contextSaved = false;
  bool planarModeEligibleForSlice = m_hls->gbh.planarModeEligibleForSlice;
  for (; !fifo.empty(); fifo.pop()) {
    TComOctreeNode& currentNode = fifo.front();

    //< start LCU-based coding, within an LCU, breadth first coding is used.
    if (partitionParams.nodeSizeLog2.max() < m_hls->frameheader.lcuNodeSizeLog2) {
      if (m_hls->gps.saveStateFlag && !contextSaved) {
        m_decBac->saveContext();
        contextSaved = true;
      }
      if (!m_hls->gps.saveStateFlag && m_hls->gps.lcuDependencyFlag) {
        m_historyMap[0] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());
        m_historyMap[1] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());
        m_decBac->LcuReset();
      }
      UInt geomTreeType = m_decBac->decodeGeomTreeType();
      if (geomTreeType == 0) {
        breadthFirstOctreeLcu(currentNode, partitionParams, m_hls->gps.saveStateFlag);
      } else {
        V3<int32_t> numbits_Lcusize_log2;
        for (int k = 0; k < 3; k++) {
          numbits_Lcusize_log2[k] = getNumBits(uint32_t(partitionParams.nodeSizeLog2[k]));
        }
        decodeTspLcu(m_pointCloudRecon, m_numReconPoints, currentNode, m_hls, m_decBac,
                     numbits_Lcusize_log2);
      }

      if (contextSaved) {
        m_decBac->restoreContext();
      }
      if (!m_hls->gps.saveStateFlag && m_hls->gps.lcuDependencyFlag) {
        m_historyMap[0].release();
        m_historyMap[1].release();
      }
      continue;
    }
    int currentOccupancy = 0;
    breadthFirstOctreeNode(currentNode, partitionParams, fifo, infOfIDCM, currentOccupancy,
                           planarModeEligibleForSlice, DcmEligibleKOctreeDepth);
    partitionParams.numNodesInCurrentDepth--;
    if (partitionParams.numNodesInCurrentDepth == 0) {
      if (m_hls->gbh.singleModeFlagInSlice) {
        DcmEligibleKOctreeDepth = m_decBac->decodeKOctreeDepthflag();
      }

      updateOctreePartitionParams(partitionParams);
      m_historyMap[0] = std::move(m_historyMap[1]);
      m_historyMap[1] = unique_ptr<TComOccupancyMap>(new TComOccupancyMap());
    }
  }
  pcRec.setNumPoint(m_numReconPoints);
  return;
}

Void TDecGeometry::init(TComPointCloud* pointCloudRecon, HighLevelSyntax* hls, TDecBacTop* decBac) {
  m_pointCloudRecon = pointCloudRecon;
  m_hls = hls;
  m_decBac = decBac;
}

Void TDecGeometry::clear() {
  m_historyMap[0].release();
  m_historyMap[1].release();
}

//////////////////////////////////////////////////////////////////////////
// Private class functions
//////////////////////////////////////////////////////////////////////////

Void TDecGeometry::breadthFirstOctreeNode(const TComOctreeNode& currentNode,
                                          TComOctreePartitionParams& params,
                                          queue<TComOctreeNode>& fifo, codeBinOfIDCM& infOfIDCM,
                                          int& currentOccupancy, bool& preNodePlanarEligible,
                                          bool& DcmEligibleKOctreeDepth) {
  Bool singlePointFlag = false;
  Bool currentMode = infOfIDCM.NextNodeMode;
  Bool whetherCurrentNodeIDCMEligible = false;
  bool singlePointLevelEigible = false;

  singlePointLevelEigible = m_hls->gbh.singleModeFlagInSlice && DcmEligibleKOctreeDepth;

  bool currentNodeNeedPopcnt = currentMode && singlePointLevelEigible;

  UInt occupancyCode = 0;

  if (singlePointLevelEigible) {
    singlePointFlag =
      singlePointMode(currentNode, params, infOfIDCM, occupancyCode, whetherCurrentNodeIDCMEligible,
                      currentNodeNeedPopcnt, currentMode);
  }
  if (!singlePointFlag) {
    TComGeomContext geomCtx;
    getContextInforFast(m_historyMap, params, currentNode, geomCtx, m_hls->gbh.contextMode,
                        m_hls->gps.occupancySearchRangeLog2, preNodePlanarEligible);
    if (m_hls->gbh.contextMode == 1) {
      occupancyCode = m_decBac->decodeOccUsingMemoryChannel(params, geomCtx, preNodePlanarEligible);
    } else {
      occupancyCode = m_decBac->decodeOccupancyCode(params, geomCtx, preNodePlanarEligible,
                                                    m_hls->gbh.contextMode);
    }

    if (currentNodeNeedPopcnt) {
      if ((bitCount(occupancyCode) == 1))
        infOfIDCM.oneChildNumInPreNode++;
    }

    for (Int i = 0; i < 8; i++) {
      if (!(occupancyCode & (1 << i)))
        continue;

      Int x = !!(i & 4);
      Int y = !!(i & 2);
      Int z = !!(i & 1);

      if (params.childSizeLog2 == 0) {  ///< reaching the leaf nodes
        decodeLeafNode(currentNode, x, y, z);
        continue;
      }

      /// create new child
      fifo.emplace();
      auto& childNode = fifo.back();

      childNode.pos[0] = currentNode.pos[0] + (x << params.childSizeLog2[0]);
      childNode.pos[1] = currentNode.pos[1] + (y << params.childSizeLog2[1]);
      childNode.pos[2] = currentNode.pos[2] + (z << params.childSizeLog2[2]);
      childNode.parentOccupancy = occupancyCode;
      childNode.parentNodeIDCMEligible = whetherCurrentNodeIDCMEligible;
      params.numNodesInNextDepth++;
    }
  }
  currentOccupancy = occupancyCode;
  updateContextInfor(m_historyMap[1], params, currentNode, m_hls->gps.occupancySearchRangeLog2,
                     occupancyCode);
}

Bool TDecGeometry::singlePointMode(const TComOctreeNode& currentNode,
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
      handleSingleMode(currentNode, params.nodeSizeLog2, params.childSizeLog2, occupancyCode);
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
          handleSingleMode(currentNode, params.nodeSizeLog2, params.childSizeLog2, occupancyCode);
        nextNodeMode = !singlePointFlag;
        whetherCurrentNodeIDCMEligible = true;
      }
      oneChildNumInPreNode = 0;
      currentNodeNeedPopcnt = 0;  //n-th node's occupancy is not need to be count
    }
  }
  return singlePointFlag;
}

Bool TDecGeometry::handleSingleMode(const TComOctreeNode& currentNode, const V3<UInt> nodeSizeLog2,
                                    const V3<UInt> childSizeLog2, UInt& occupancyCode) {
  Bool singlePointFlagInferred = false;
  if (currentNode.parentNodeIDCMEligible) {
    // if parent node has only one occupied child, current node is not single
    // node for sure
    int parent_child_count = bitCount(currentNode.parentOccupancy);
    if (parent_child_count == 1) {
      singlePointFlagInferred = true;
    }
  }

  Bool singlePointFlag = false;
  if (!singlePointFlagInferred) {
    singlePointFlag = m_decBac->decodeSinglePointFlag();
  }
  if (singlePointFlag) {
    TComPointCloud& pcRec = *m_pointCloudRecon;
    V3<UInt> lowerPos = m_decBac->decodeSinglePointIndex(nodeSizeLog2);
    pcRec[m_numReconPoints][0] = (Double)currentNode.pos[0] + lowerPos[0];
    pcRec[m_numReconPoints][1] = (Double)currentNode.pos[1] + lowerPos[1];
    pcRec[m_numReconPoints][2] = (Double)currentNode.pos[2] + lowerPos[2];
    ++m_numReconPoints;

    occupancyCode = 1 << ((lowerPos[0] >> childSizeLog2[0]) << 2) +
        ((lowerPos[1] >> childSizeLog2[1]) << 1) + ((lowerPos[2] >> childSizeLog2[2]));
  }
  return singlePointFlag;
}

Void TDecGeometry::decodeLeafNode(const TComOctreeNode& currentNode, const Int x, const Int y,
                                  const Int z) {
  TComPointCloud& pcRec = *m_pointCloudRecon;

  const PC_POS point({Double(currentNode.pos[0]) + x, Double(currentNode.pos[1]) + y,
                      Double(currentNode.pos[2]) + z});
  if (m_hls->sps.geomRemoveDuplicateFlag) {
    pcRec[m_numReconPoints++] = point;
  } else {
    UInt dupNum = m_decBac->decodeDuplicateNumber();
    for (UInt i = 0; i < dupNum; i++)
      pcRec[m_numReconPoints++] = point;
  }
}

Void TDecGeometry::breadthFirstOctreeLcu(const TComOctreeNode& node,
                                         TComOctreePartitionParams params, Bool saveStateFlag) {
  queue<TComOctreeNode> fifo;
  fifo.push(node);
  params.numNodesInCurrentDepth = 1;
  params.numNodesInNextDepth = 0;
  bool historyMapSaved = false;
  codeBinOfIDCM infOfIDCM;
  infOfIDCM.nodeIdxOfIDCM = 0;
  infOfIDCM.NextNodeMode = 1;
  infOfIDCM.theTrueNumOf_IDCM_InLastTenNode = 0;
  infOfIDCM.oneChildNumInPreNode = 0;
  bool planarModeEligibleForSlice = m_hls->gbh.planarModeEligibleForSlice;

  bool DcmEligibleKOctreeDepth = false;

  for (; !fifo.empty(); fifo.pop()) {
    TComOctreeNode currentNode = fifo.front();
    int currentOccupancy = 0;
    breadthFirstOctreeNode(currentNode, params, fifo, infOfIDCM, currentOccupancy,
                           planarModeEligibleForSlice, DcmEligibleKOctreeDepth);
    params.numNodesInCurrentDepth--;
    if (params.numNodesInCurrentDepth == 0) {
      if (m_hls->gbh.singleModeFlagInSlice) {
        DcmEligibleKOctreeDepth = m_decBac->decodeKOctreeDepthflag();
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
