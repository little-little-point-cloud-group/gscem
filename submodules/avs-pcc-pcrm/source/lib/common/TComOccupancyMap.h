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

#include "PointCloudMortonTable.h"
#include "TComHashMap.h"
#include "TComOctree.h"
#include "TComPointCloud.h"
#include "TComVector.h"
#include "TypeDef.h"
#include <assert.h>
#include <memory>

using namespace std;

///< \in TLibCommon \{

/**
 * Class TComOccupancyMap
 * geometry occupancy map
 */

struct OccupancyInfo {
  UInt8 occupancy;

  OccupancyInfo(const UInt8 occupancy)
    : occupancy(occupancy) {}

  OccupancyInfo()
    : occupancy(0) {}
};

struct TComGeomContext {
  UInt neighborMask;
  UInt ctxParent;
  UInt ctxParentAdv[8];
  UInt ctxParentAdv1[8];
  UInt ctx7EdgeNeighborMask;
  UInt ctxChildD1[6];
  UInt ctxChildOccu[11];
  TComGeomContext() {
    neighborMask = 0;
    ctx7EdgeNeighborMask = 0;
    ctxParent = 0;
    fill(begin(ctxChildD1), end(ctxChildD1), 0);
    fill(begin(ctxParentAdv), end(ctxParentAdv), 0);
    fill(begin(ctxChildOccu), end(ctxChildOccu), 0);
    fill(begin(ctxParentAdv1), end(ctxParentAdv1), 0);
  }
};
class TComOccupancyMap {
private:
  unique_ptr<TComHashMapDynamic<OccupancyInfo>> m_map;

public:
  TComOccupancyMap() {
    init();
  }

  ~TComOccupancyMap() {
    clear();
  }

  Void init() {
    m_map = unique_ptr<TComHashMapDynamic<OccupancyInfo>>(new TComHashMapDynamic<OccupancyInfo>);
  }

  Void clear() {
    m_map.release();
  }

  unique_ptr<TComHashMapDynamic<OccupancyInfo>>& getMap() {
    return m_map;
  }

  Bool getChildOccupied(const Int32 x, const Int32 y, const Int32 z) const;

  Bool getChildOccupied(const Int32 x, const Int32 y, const Int32 z,
                        const UInt occupancySkipParent) const;

  Bool getChildOccupied(const Int32 x, const Int32 y, const Int32 z, const Int shiftX,
                        const Int shiftY, const Int shiftZ) const;

  UInt8 getOccupancy(const Int32 x, const Int32 y, const Int32 z) const;

  Void insert(const Int32 x, const Int32 y, const Int32 z, const UInt8 OcuupancymapSizelog2, 
              const UInt8 occupancyCode = 0);

private:
  Int getBitIndex(const Int32 x, const Int32 y, const Int32 z) const;

};  ///< END CLASS TComOccupancyMap



inline Void getContextInforFast(const unique_ptr<TComOccupancyMap>* occupancyMap,
                                 const TComOctreePartitionParams& params,
                                 const TComOctreeNode& currentNode, TComGeomContext& geomCtx,
                                const UInt contextMode, const UInt OcuupancymapSizelog2,
                                bool& preNodePlanarEligible) {
  UInt depth = params.depth;
  UInt occupancySkip = params.occupancySkip;
  UInt occupancySkipParent = params.occupancySkipParent;
  V3<UInt> nodeSizeLog2 = params.nodeSizeLog2;
  UInt* ctx7EdgeNeighborMask = &geomCtx.ctx7EdgeNeighborMask;

  //< get context infor from neighboring parent nodes
  UInt* neighborMask = &geomCtx.neighborMask;
  UInt* ctxParent = &geomCtx.ctxParent;
  UInt* ctxParentAdv = geomCtx.ctxParentAdv;
  UInt* ctxParentAdv1 = geomCtx.ctxParentAdv1;
  UInt* ctxChildD1 = geomCtx.ctxChildD1;
  UInt* ctxChildOccu = geomCtx.ctxChildOccu;

  V3<UInt> pos = currentNode.pos;
  V3<UInt> pos_ = pos >> nodeSizeLog2;
  const Int x = pos_[0];
  const Int y = pos_[1];
  const Int z = pos_[2];

  int fatherX = (x >> 1);
  int fatherY = (y >> 1);
  int fatherZ = (z >> 1);
  int occupancyIndex = ((pos_[0] & 1) << 2) + ((pos_[1] & 1) << 1) + (pos_[2] & 1);

  int occupancyCache[8];

  if (depth > 0) {
    const TComOccupancyMap& om = *occupancyMap[0];

    if (contextMode == 1) {
      Bool parentCtx27[27] = {false};
      if (!occupancySkipParent) {
        for (int i = 0; i < fatherNumber[occupancyIndex][0]; i++) {
          int dx = fatherX + hashShfit[occupancyIndex][i][0];
          int dy = fatherY + hashShfit[occupancyIndex][i][1];
          int dz = fatherZ + hashShfit[occupancyIndex][i][2];
          occupancyCache[i] = om.getOccupancy(dx, dy, dz);
        }
        *ctxParent |=
          ((occupancyCache[fatherIndex[occupancyIndex][0]] >> childPosition[occupancyIndex][0]) & 1)
          ? 32
          : 0;
        *ctxParent |=
          ((occupancyCache[fatherIndex[occupancyIndex][1]] >> childPosition[occupancyIndex][1]) & 1)
          ? 8
          : 0;
        *ctxParent |=
          ((occupancyCache[fatherIndex[occupancyIndex][2]] >> childPosition[occupancyIndex][2]) & 1)
          ? 2
          : 0;

        for (int i = fatherNumber[occupancyIndex][0]; i < fatherNumber[occupancyIndex][1]; i++) {
          int dx = fatherX + hashShfit[occupancyIndex][i][0];
          int dy = fatherY + hashShfit[occupancyIndex][i][1];
          int dz = fatherZ + hashShfit[occupancyIndex][i][2];
          occupancyCache[i] = om.getOccupancy(dx, dy, dz);
        }
        *ctxParent |=
          ((occupancyCache[fatherIndex[occupancyIndex][3]] >> childPosition[occupancyIndex][3]) & 1)
          ? 16
          : 0;
        *ctxParent |=
          ((occupancyCache[fatherIndex[occupancyIndex][4]] >> childPosition[occupancyIndex][4]) & 1)
          ? 4
          : 0;
        *ctxParent |=
          ((occupancyCache[fatherIndex[occupancyIndex][5]] >> childPosition[occupancyIndex][5]) & 1)
          ? 1
          : 0;

      } else {
        //obuf
        const Int shiftX = (occupancySkipParent & 4) ? 0 : 1;
        const Int shiftY = (occupancySkipParent & 2) ? 0 : 1;
        const Int shiftZ = (occupancySkipParent & 1) ? 0 : 1;
        *ctxParent |= om.getChildOccupied(x + 1, y, z, shiftX, shiftY, shiftZ) ? 32 : 0;
        *ctxParent |= om.getChildOccupied(x - 1, y, z, shiftX, shiftY, shiftZ) ? 16 : 0;
        *ctxParent |= om.getChildOccupied(x, y + 1, z, shiftX, shiftY, shiftZ) ? 8 : 0;
        *ctxParent |= om.getChildOccupied(x, y - 1, z, shiftX, shiftY, shiftZ) ? 4 : 0;
        *ctxParent |= om.getChildOccupied(x, y, z + 1, shiftX, shiftY, shiftZ) ? 2 : 0;
        *ctxParent |= om.getChildOccupied(x, y, z - 1, shiftX, shiftY, shiftZ) ? 1 : 0;
      }

      //obuf

      for (Int i = 0; i < 18; i++) {
        int dx = neighborDeltaX[i];
        int dy = neighborDeltaY[i];
        int dz = neighborDeltaZ[i];
        parentCtx27[neighborIndex[i]] =
          om.getChildOccupied(x + dx, y + dy, z + dz, occupancySkipParent);
      }

      for (Int iChild = 0; iChild < 8; iChild++) {
        if (occupancySkip != 0) {
          if ((occupancySkip & 1) && (iChild & 1))  ///< skip when z = 1
            continue;
          if ((occupancySkip & 2) && (iChild & 2))  ///< skip when y = 1
            continue;
          if ((occupancySkip & 4) && (iChild & 4))  ///< skip when x = 1
            continue;
        }
        // check 6 nearest parent-level neighbors sharing a face or edge with current child node
        for (Int i = 0; i < 6; i++) {
          ctxParentAdv[iChild] |= parentCtx27[childNeighborIdx[iChild][i]] << i;
        }
      }
    } else {
      Bool parentCtx27[27] = {false};
      Bool parentCtx27cc[27] = {false};
      if (!occupancySkipParent) {
        for (Int i = 0; i < 6; i++) {
          int dx = neighborDeltaX1[i];
          int dy = neighborDeltaY1[i];
          int dz = neighborDeltaZ1[i];
          parentCtx27[neighborIndex1[i]] =
            om.getChildOccupied(x + dx, y + dy, z + dz, occupancySkipParent);
        }

      } else {
        for (Int i = 0; i < 6; i++) {
          int dx = neighborDeltaX1[i];
          int dy = neighborDeltaY1[i];
          int dz = neighborDeltaZ1[i];

          parentCtx27[neighborIndex1[i]] =
            om.getChildOccupied(x + dx, y + dy, z + dz, occupancySkipParent);
        }
      }

      for (Int iChild = 0; iChild < 8; iChild++) {
        if (occupancySkip != 0) {
          if ((occupancySkip & 1) && (iChild & 1))  ///< skip when z = 1
            continue;
          if ((occupancySkip & 2) && (iChild & 2))  ///< skip when y = 1
            continue;
          if ((occupancySkip & 4) && (iChild & 4))  ///< skip when x = 1
            continue;
        }
        // check 3 nearest parent-level neighbors sharing a face with current child node
        for (Int i = 0; i < 3; i++) {
          ctxParentAdv1[iChild] |= parentCtx27[childNeighborIdx[iChild][i]] << i;
        }
      }
    } 
  }

  //< get context infor from neighboring child nodes
  TComOccupancyMap& om = *occupancyMap[1];
  if (om.getMap()->isAtBoundaryCheckPoint(pos_, OcuupancymapSizelog2)) {
    om.getMap()->shrinkSize();
    om.getMap()->setBoundaryCheckPoint(pos_, OcuupancymapSizelog2);
  }

  if (contextMode == 1) {
    ctxChildOccu[0] = om.getOccupancy(x - 1, y - 1, z - 1);
    ctxChildOccu[1] = om.getOccupancy(x - 1, y - 1, z);
    ctxChildOccu[2] = om.getOccupancy(x - 1, y, z - 1);
    ctxChildOccu[3] = om.getOccupancy(x - 1, y, z);
    ctxChildOccu[4] = om.getOccupancy(x, y - 1, z - 1);
    ctxChildOccu[5] = om.getOccupancy(x, y - 1, z);
    ctxChildOccu[6] = om.getOccupancy(x, y, z - 1);
    if (preNodePlanarEligible) {
      ctxChildOccu[1] = om.getOccupancy(x - 1, y - 1, z);
      ctxChildOccu[8] = om.getOccupancy(x - 2, y, z);
      ctxChildOccu[9] = om.getOccupancy(x, y - 2, z);
    }
  } else {
    UInt8 occNei;
    ctxChildOccu[0] = om.getOccupancy(x - 1, y - 1, z - 1);
    ctxChildOccu[1] = om.getOccupancy(x - 1, y - 1, z);
    ctxChildOccu[2] = om.getOccupancy(x - 1, y, z - 1);
    ctxChildOccu[3] = om.getOccupancy(x - 1, y, z);
    ctxChildOccu[4] = om.getOccupancy(x, y - 1, z - 1);
    ctxChildOccu[5] = om.getOccupancy(x, y - 1, z);
    ctxChildOccu[6] = om.getOccupancy(x, y, z - 1);
    ///< neighboring node (dx = -1)
    occNei = ctxChildOccu[3];
    if (!(occupancySkip & 4))
      occNei = (occNei & 0xF0) >> 4;
    else
      occNei = (occNei & 0x0F);
    ctxChildD1[0] |= occNei;
    ///< neighboring node (dy = -1)
    occNei = ctxChildOccu[5];
    if (!(occupancySkip & 2))
      occNei = (occNei & 0xCC) >> 2;
    else
      occNei = (occNei & 0x33);
    ctxChildD1[1] |= occNei;

    ///< neighboring node (dz = -1)
    occNei = ctxChildOccu[6];
    if (!(occupancySkip & 1))
      occNei = (occNei & 0xAA) >> 1;
    else
      occNei = (occNei & 0x55);
    ctxChildD1[2] |= occNei;
    if (preNodePlanarEligible) {
      ctxChildOccu[8] = om.getOccupancy(x - 2, y, z);
      ctxChildOccu[9] = om.getOccupancy(x, y - 2, z);
      ctxChildOccu[10] = om.getOccupancy(x, y, z - 2);
    }
  } 
}

inline UInt computeChildContext(const TComGeomContext& geomCtx) {
  UInt8 context = 0;
  const UInt neighborMask = geomCtx.neighborMask;
  const UInt ctx7EdgeNeighbor = geomCtx.ctx7EdgeNeighborMask;
  if ((neighborMask & 0x01) || (ctx7EdgeNeighbor)) {
    context = 0;
  } else {
    context = 1 + !!(neighborMask & 0x02);
  }
  return context;
}
/**
  * update context infor
  */
inline Void updateContextInfor(unique_ptr<TComOccupancyMap>& occupancyMap,
                               const TComOctreePartitionParams& params,
                               const TComOctreeNode& currentNode, const UInt OcuupancymapSizelog2,
                               UInt occupancyCode) {
  V3<UInt> pos = currentNode.pos;
  const Int x = pos[0] >> params.nodeSizeLog2[0];
  const Int y = pos[1] >> params.nodeSizeLog2[1];
  const Int z = pos[2] >> params.nodeSizeLog2[2];

  occupancyMap->insert(x, y, z,OcuupancymapSizelog2, occupancyCode);
}
inline int popcnt8(uint8_t x) {
  uint32_t val = x * 0x08040201u;
  val >>= 3;
  val &= 0x11111111u;
  val *= 0x11111111u;
  return val >> 28;
}
inline int contextMap(UInt8 neiFromFaceMode, UInt8 neiFromEdgeMode) {
    assert(neiFromFaceMode < 8);
    assert(neiFromEdgeMode < 8);
    int map[8][8] = {{0, 1, 1, 1, 1, 1, 1, 1},{2, 5, 5, 9, 4, 10, 10, 13},
        {2, 5, 4, 10, 5, 9, 10, 13},{3, 7, 8, 11, 8, 11, 12, 14},
        {2, 4, 5, 10, 5, 10, 9, 13},{3, 8, 7, 11, 8, 12, 11, 14},
        {3, 8, 8, 12, 7, 11, 11, 14}, {6, 15, 15, 15, 15, 15, 15, 15}};
    return map[neiFromFaceMode][neiFromEdgeMode];
}
///< \}
