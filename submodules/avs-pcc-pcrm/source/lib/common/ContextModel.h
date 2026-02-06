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
#include "contributors.h"

///< \in TLibCommon
/**
 * struct for context management
 */
#define B_BITS 10
#define QUARTER (1 << (B_BITS - 2))

#define MAKE_CONTEXT(lg_pmps, mps, cycno) \
  (((uint16_t)(cycno) << 12) | ((uint16_t)(mps) << 0) | (uint16_t)(lg_pmps << 1))

typedef union context_t {
  struct {
    unsigned MPS : 1;       // 1  bit
    unsigned LG_PMPS : 11;  // 11 bits
    unsigned cycno : 2;     // 2  bits
  };
  uint16_t v;
} context_t;

#define NUM_OCCUPANCY_CHILD_CTX (1 << 3)
#define NUM_OCCUPANCY_CTX 20
#define NUM_child_CTX (1 << 7)

typedef struct geom_ctx_set_t {
  context_t planarMode[2];
  context_t ctx_occupancyCombineParent[NUM_child_CTX];
  context_t ctxRUB_occupancy[NUM_OCCUPANCY_CHILD_CTX][8];
  context_t ctxMemoryChannel[288];
  context_t ctx_compute[8][3];
  context_t ctx_occupancy[1 << 5][NUM_OCCUPANCY_CHILD_CTX];
  context_t ctx_occupancyCombinechild[8][7];
  context_t ctx_occupancyCombineParent1[8][9];
  context_t ctx_geom_num_dup_eq1;
  context_t ctx_geom_single_mode_flag;

  //context model for predtree/tsp coding
  context_t ctxGeomTreeType;
  context_t ctxdepth;
  context_t ctxPredTreeIsZero[3];
  context_t ctxPredTreeSign[3];
  context_t ctxPredTreeNumBits[3][10];

} geom_ctx_set_t;

typedef struct attr_ctx_set_t {
  context_t ctx_attr_residual_eq0[8];
  context_t ctx_attr_residual_flag1[4];
  context_t ctx_attr_residual_flag2[4];
  context_t parity[4];

  ///< runlength context
  context_t ctx_attr_length_eq0;
  context_t ctx_length_prefix[3];
  context_t ctx_length_suffix[2];

  ///< context using for attribute residual
  context_t ctx_attr_residual_prefix[3];
  context_t ctx_attr_residual_suffix[3];

  ///< context using for equal to zero
  context_t ctx_attr_flag1;
  context_t ctx_attr_flag2;

  ///< context using for attribute_yuv_minusone residual
  context_t ctx_attr_residual_minusone_eq0[6];
  context_t ctx_attr_residual_minusone_flag1[3];
  context_t ctx_attr_residual_minusone_flag2[3];
} attr_ctx_set_t;

typedef struct ctx_set_t_dual {
  context_t ctx_attr_residual_eq0[6];
  context_t ctx_attr_residual_eq1[3];
  context_t ctx_attr_residual_eq2[3];

  ///< runlength context
  context_t ctx_attr_length_eq0;

  ///< context using for attribute residual
  context_t ctx_attr_residual_prefix[3];
  context_t ctx_attr_residual_suffix[3];
} ctx_set_t_dual;
///< \}
