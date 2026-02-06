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

#include "TComOccupancyMap.h"

///< \in TLibCommon \{

/**
 * Class TComOccupancyMap
 * occupancy map for accessing neighborings
 */

//////////////////////////////////////////////////////////////////////////
// Public class functions
//////////////////////////////////////////////////////////////////////////

Bool TComOccupancyMap::getChildOccupied(const Int32 x, const Int32 y, const Int32 z,
                                        const UInt occupancySkipParent) const {
  if (occupancySkipParent == 0) {
    return getChildOccupied(x, y, z);
  } else {
    const Int shiftX = (occupancySkipParent & 4) ? 0 : 1;
    const Int shiftY = (occupancySkipParent & 2) ? 0 : 1;
    const Int shiftZ = (occupancySkipParent & 1) ? 0 : 1;
    return getChildOccupied(x, y, z, shiftX, shiftY, shiftZ);
  }
}

Bool TComOccupancyMap::getChildOccupied(const Int32 x, const Int32 y, const Int32 z) const {
  if (x < 0 || y < 0 || z < 0)
    return false;
  OccupancyInfo hi;
  if (m_map->get(x >> 1, y >> 1, z >> 1, hi)) {
    return (hi.occupancy >> getBitIndex(x, y, z)) & 1;
  }
  return false;
}

Bool TComOccupancyMap::getChildOccupied(const Int32 x, const Int32 y, const Int32 z,
                                        const Int shiftX, const Int shiftY,
                                        const Int shiftZ) const {
  if (x < 0 || y < 0 || z < 0)
    return false;
  OccupancyInfo hi;
  if (m_map->get(x >> shiftX, y >> shiftY, z >> shiftZ, hi)) {
    return (hi.occupancy >> getBitIndex(shiftX ? x : 0, shiftY ? y : 0, shiftZ ? z : 0)) & 1;
  }
  return false;
}

UInt8 TComOccupancyMap::getOccupancy(const Int32 x, const Int32 y, const Int32 z) const {
  if (x < 0 || y < 0 || z < 0)
    return 0;
  OccupancyInfo hi;
  if (m_map->get(x, y, z, hi)) {
    return hi.occupancy;
  }
  return 0;
}

Void TComOccupancyMap::insert(const Int32 x, const Int32 y, const Int32 z,
                              const UInt8 OcuupancymapSizelog2, const UInt8 occupancyCode) {
  assert(x >= 0 && y >= 0 && z >= 0);
  m_map->insert(x, y, z, OcuupancymapSizelog2, occupancyCode);
}

Int TComOccupancyMap::getBitIndex(const Int32 x, const Int32 y, const Int32 z) const {
  return ((x & 1) << 2) | ((y & 1) << 1) | (z & 1);
}

///< \}