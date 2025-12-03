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

#include "TEncBacTop.h"
#include "common/HighLevelSyntax.h"
#include "common/TComOccupancyMap.h"
#include "common/TComOctree.h"
#include "common/TComPointCloud.h"
#include "common/contributors.h"
#include <queue>

///< \in TLibEncoder

/**
 * Implementation of TEncGeometry
 * geometry encoder
 */

class TEncGeometry {
private:
  TComPointCloud* m_pointCloudOrg;                  ///< pointer to input point cloud
  TComPointCloud* m_pointCloudRecon;                ///< pointer to output point cloud
  UInt m_numReconPoints;                            ///< number of reconstructed points
  HighLevelSyntax* m_hls;                           ///< pointer to high-level syntax parameters
  TEncBacTop* m_encBac;                             ///< pointer to bac
  unique_ptr<TComOccupancyMap> m_historyMap[2];     ///< context information
  unique_ptr<TComOccupancyMap> m_historyMapBackup;  ///< backup hashmap for later restoration

public:
  TEncGeometry() = default;
  ~TEncGeometry() = default;

  Void init(TComPointCloud* pointCloudOrg, TComPointCloud* pointCloudRecon, HighLevelSyntax* hls,
            TEncBacTop* encBac);

  Int compressAndEncodeGeometry();
  Void clear();

private:
  Void initFifoWithRootNode(queue<TComOctreeNode>& fifo);
  Void breadthFirstOctreeNode(const TComOctreeNode& currentNode, TComOctreePartitionParams& params,
                               queue<TComOctreeNode>& fifo, codeBinOfIDCM& infOfIDCM,
                               int& currentOccupancy, bool& preNodePlanarEligible,
                               int& numdcmPoints, int& numSubnodes, bool& DcmEligibleKOctreeDepth);
  Bool handleSingleMode(const TComOctreeNode& currentNode, const V3<UInt>& nodeSizeLog2,
                        const Bool singleModeFlagParent, UInt& occupancyCode,
                        const V3<UInt>& childSizeLog2);
  Void encodeLeafNode(UInt& childPointStartIdx, const UInt& childNum);
  Void initChildNode(const TComOctreeNode& currentNode, TComOctreeNode& childNode,
                     const V3<UInt>& childSizeLog2, const int childIdx, UInt& startChildIdx,
                     const std::array<int, 8>& childrenCount, const UInt8 occupancyCode,
                     Bool whetherParentNodeIDCMEligible);
  Bool singlePointMode(const TComOctreeNode& currentNode,
                                     TComOctreePartitionParams& params, codeBinOfIDCM& infOfIDCM,
                                     UInt& occupancyCode, bool& whetherCurrentNodeIDCMEligible,
                                     bool& currentNodeNeedPopcnt, bool currentMode);
  ///<Swap point and Attribute
  template<typename iterator, typename ValueOp, std::size_t neighborCnt>
  void octreeDivision(iterator begin, iterator end, std::array<int, neighborCnt>& neighborCounts,
                      const UInt& childIdxBegin, ValueOp value_of) {
    for (auto iter = begin; iter != end; ++iter)
      ++neighborCounts[value_of(*iter)];

    std::array<iterator, neighborCnt> ptrsGeometry = {{begin}};
    for (int idx = 1; idx < neighborCnt; ++idx)
      ptrsGeometry[idx] = std::next(ptrsGeometry[idx - 1], neighborCounts[idx - 1]);
    iterator originGeomEnd = begin;   

    std::unique_ptr<std::array<std::vector<PC_COL>::iterator, neighborCnt>> ptrsColors;
    std::unique_ptr<std::array<std::vector<PC_REFL>::iterator, neighborCnt>> ptrsRefls;
    if (m_pointCloudOrg->hasColors()) {
      int count = 0;
      auto numMulti_color = m_pointCloudOrg->getNumMultilColor();
      ptrsColors = std::unique_ptr<std::array<std::vector<PC_COL>::iterator, neighborCnt>>(
        new std::array<std::vector<PC_COL>::iterator, neighborCnt>);
      (*ptrsColors)[0] = m_pointCloudOrg->getColors().begin() + childIdxBegin * numMulti_color;
      for (int idx = 1; idx < neighborCnt; ++idx) {
        (*ptrsColors)[idx] =
          std::next((*ptrsColors)[idx - 1], neighborCounts[idx - 1] * numMulti_color);
      }
    }
    if (m_pointCloudOrg->hasReflectances()) {
      auto numMulti_refl = m_pointCloudOrg->getNumMultilRefl();
      ptrsRefls = std::unique_ptr<std::array<std::vector<PC_REFL>::iterator, neighborCnt>>(
        new std::array<std::vector<PC_REFL>::iterator, neighborCnt>);
      (*ptrsRefls)[0] = m_pointCloudOrg->getReflectances().begin() + childIdxBegin * numMulti_refl;
      for (int idx = 1; idx < neighborCnt; ++idx){
        (*ptrsRefls)[idx] =
          std::next((*ptrsRefls)[idx - 1], neighborCounts[idx - 1] * numMulti_refl);
      }  
    }
    for (int i = 0; i < neighborCnt; ++i) {
      std::advance(originGeomEnd, neighborCounts[i]);
      while (ptrsGeometry[i] != originGeomEnd) {
        int radix = value_of(*ptrsGeometry[i]);
        std::iter_swap(ptrsGeometry[i], ptrsGeometry[radix]);
        if (m_pointCloudOrg->hasColors()) {
          auto numMulti_color = m_pointCloudOrg->getNumMultilColor();
          for (int idxMulti = 0; idxMulti < numMulti_color; idxMulti++) {
            std::iter_swap((*ptrsColors)[i] + idxMulti, (*ptrsColors)[radix] + idxMulti);
          }
          (*ptrsColors)[radix] = (*ptrsColors)[radix] + numMulti_color;
        }
        if (m_pointCloudOrg->hasReflectances()) {
          auto numMulti_refl = m_pointCloudOrg->getNumMultilRefl();
          for (int idxMulti = 0; idxMulti < numMulti_refl; idxMulti++) {
            std::iter_swap((*ptrsRefls)[i] + idxMulti, (*ptrsRefls)[radix] + idxMulti);
          }
          (*ptrsRefls)[radix] = (*ptrsRefls)[radix] + numMulti_refl;
        }
        ++ptrsGeometry[radix];
      }
    }  
  }
  Void treePartition(const TComOctreeNode& currentNode, const V3<UInt>& childNodeSizeLog2,
                     const UInt& occupancySkip, std::array<int, 8>& neighborCounts);
  Void breadthFirstOctreeLcu(const TComOctreeNode& node, TComOctreePartitionParams params,
                             Bool saveStateFlag);
};  ///< END CLASS TEncGeometry

///< \}
