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

#include "common/CommonDef.h"
#include "common/TComBitStream.h"
#include "contributors.h"

///< \in TLibEncoder

/**
 * Class TComBufferChunk
 * bitstream buffer chunk
 */
#define slice_geometry_start_code (0x00)
#define slice_geometry_end_code   (0x8F) ///< slice end code is same 
#define slice_attribute_start_code (0x3F)
#define slice_attribute_end_code (0x8F)

#define slice_start_code_lower  (0x00)
#define slice_start_code_upper  (0x7F)
#define slice_end_code          (0x8F)
#define pcc_sequence_start_code (0xB0)
#define pcc_sequence_end_code   (0xB1)
#define geometry_start_code     (0xB2)
#define attribute_start_code    (0xB3)
#define frame_start_code        (0xB4)
#define user_data_start_code    (0xB5)
#define pcc_edit_code           (0xB6)

class TComBufferChunk : public TComBitstream {
private:
  BufferChunkType m_bufferType;

public:
  TComBufferChunk() = default;
  TComBufferChunk(BufferChunkType bufferType);
  ~TComBufferChunk() = default;

  Void writeToBitstream(ofstream* outBitstream, UInt64 length);
  Int readFromBitstream(ifstream& inBitstream, int buffersize);
  Void setBufferType(BufferChunkType bufferType);
  BufferChunkType getBufferType();

private:
  Int readBufferChunk(ifstream& inBitstream, TSize& bufferChunkSize);
  TSize initParsingConvertPayloadToRBSP(const TSize uiBytesRead, UChar* pBuffer, UChar* pBuffer2);

};  ///< END CLASS TComBufferChunk

///! \}
