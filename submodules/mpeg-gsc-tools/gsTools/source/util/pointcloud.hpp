
/* The copyright in this software is being made available under the BSD
 * Licence, included below.  This software may be subject to other third
 * party and contributor rights, including patent rights, and no such
 * rights are granted under this licence.
 *
 * Copyright (c) 2025, ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of the ISO/IEC nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "util/common.hpp"
#include "util/cameraPosition.hpp"

//============================================================================

struct alignas( 16 ) GaussianData {
  glm::vec4 xyz_;
  glm::vec4 scale_;
  glm::quat rotate_;
  glm::vec4 sh_[16];
  float     opacity_;

  inline void trace() const {
    printf( "  xyz  = %14.10f %14.10f %14.10f \n", xyz_.x, xyz_.y, xyz_.z );
    for ( int j = 0; j < 16; j++ ) printf( "  sh%02d = %14.10f %14.10f %14.10f \n", j, sh_[j].x, sh_[j].y, sh_[j].z );
    printf( "  opac = %14.10f \n", opacity_ );
    printf( "  scal = %14.10f %14.10f %14.10f \n", scale_.x, scale_.y, scale_.z );
    printf( "  rot  = %14.10f %14.10f %14.10f %14.10f \n", rotate_.w, rotate_.x, rotate_.y, rotate_.z );
    fflush( stdout );
  }
};

//============================================================================

static bool getTokens( const char* str, std::vector<std::string>& tokens ) {
  if ( !tokens.empty() ) tokens.clear();
  std::string  buf    = "";
  size_t       i      = 0;
  const size_t length = ::strlen( str );
  while ( i < length ) {
    if ( compareSeparators( str[i] ) ) {
      buf += str[i];
    } else if ( buf.length() > 0 ) {
      tokens.push_back( buf );
      buf = "";
    }
    i++;
  }
  if ( !buf.empty() ) tokens.push_back( buf );
  return !tokens.empty();
}

//============================================================================

class Pointcloud {
 public:
  Pointcloud() {}

  //============================================================================

  void               resize( size_t size ) { gs_.resize( size ); }
  void               clear() { gs_.clear(); }
  inline auto&       gaussianData() { return gs_; }
  inline size_t      size() const { return gs_.size(); }
  inline auto&       operator[]( const size_t index ) const { return gs_[index].xyz_; }
  inline auto&       operator[]( const size_t index ) { return gs_[index].xyz_; }
  const auto&        xyz( int32_t index ) const { return gs_[index].xyz_; }
  const auto&        sh( int32_t index ) const { return gs_[index].sh_; }
  const auto&        opacity( int32_t index ) const { return gs_[index].opacity_; }
  const auto&        scale( int32_t index ) const { return gs_[index].scale_; }
  const auto&        rot( int32_t index ) const { return gs_[index].rotate_; }
  inline auto&       cameraPosition() { return cameraPosition_; }
  inline const auto& cameraPosition() const { return cameraPosition_; }
  void               copy( const Pointcloud& src, size_t index ) { gs_.push_back( src.gs_[index] ); }

  //============================================================================

  inline void trace( const std::string& string = "", int maxPoint = 16 ) const {
    printf( "PC %-20s: XYZ = %8zu \n", string.c_str(), size() );
    if ( maxPoint == -1 ) maxPoint = (int)size();
    for ( int i = 0; i < ( std::min )( maxPoint, (int)size() ); i++ )
      printf( "Point[%6d]: xyz = %12f %12f %12f rot = %12f %12f %12f %12f \n", i, gs_[i].xyz_.x, gs_[i].xyz_.y,
              gs_[i].xyz_.z, gs_[i].rotate_.w, gs_[i].rotate_.x, gs_[i].rotate_.y, gs_[i].rotate_.z );
    fflush( stdout );
  }

  //============================================================================

  const std::vector<std::string> g_names = {
      "x",         "y",         "z",         "nx",        "ny",        "nz",        "f_dc_0",    "f_dc_1",
      "f_dc_2",    "f_rest_0",  "f_rest_1",  "f_rest_2",  "f_rest_3",  "f_rest_4",  "f_rest_5",  "f_rest_6",
      "f_rest_7",  "f_rest_8",  "f_rest_9",  "f_rest_10", "f_rest_11", "f_rest_12", "f_rest_13", "f_rest_14",
      "f_rest_15", "f_rest_16", "f_rest_17", "f_rest_18", "f_rest_19", "f_rest_20", "f_rest_21", "f_rest_22",
      "f_rest_23", "f_rest_24", "f_rest_25", "f_rest_26", "f_rest_27", "f_rest_28", "f_rest_29", "f_rest_30",
      "f_rest_31", "f_rest_32", "f_rest_33", "f_rest_34", "f_rest_35", "f_rest_36", "f_rest_37", "f_rest_38",
      "f_rest_39", "f_rest_40", "f_rest_41", "f_rest_42", "f_rest_43", "f_rest_44", "opacity",   "scale_0",
      "scale_1",   "scale_2",   "rot_0",     "rot_1",     "rot_2",     "rot_3" };

  //============================================================================

  bool load( const std::string& filename, const int32_t index ) {
    auto          name = createFilename( filename, index );
    std::ifstream ifs( name, std::ifstream::in | std::ifstream::binary );
    if ( !ifs.is_open() ) {
      printf( "Can't load %s \n", name.c_str() );
      return false;
    }
    struct AttributeInfo {
      std::string name_;
      int32_t     byteCount_;
      int32_t     index_;
    };
    std::vector<AttributeInfo> attributesInfo;
    attributesInfo.reserve( 16 );
    const size_t             maxBufferSize = 4096;
    char                     tmp[maxBufferSize];
    std::vector<std::string> tokens;
    ifs.getline( tmp, maxBufferSize );
    getTokens( tmp, tokens );
    if ( tokens.empty() || tokens[0] != "ply" ) {
      std::cout << "Error: corrupted file!" << std::endl;
      return false;
    }
    bool   isAscii          = false;
    double version          = 1.0;
    size_t pointCount       = 0;
    bool   isVertexProperty = true;
    while ( 1 ) {
      if ( ifs.eof() ) {
        std::cout << "Error: corrupted header!" << std::endl;
        return false;
      }
      ifs.getline( tmp, maxBufferSize );
      getTokens( tmp, tokens );
      if ( tokens.empty() || tokens[0] == "comment" ) { continue; }
      if ( tokens[0] == "format" ) {
        if ( tokens.size() != 3 ) {
          std::cout << "Error: corrupted format info!" << std::endl;
          return false;
        }
        isAscii = tokens[1] == "ascii";
        version = atof( tokens[2].c_str() );
      } else if ( tokens[0] == "element" ) {
        if ( tokens.size() != 3 ) {
          std::cout << "Error: corrupted element info!" << std::endl;
          return false;
        }
        if ( tokens[1] == "vertex" ) {
          pointCount = atoi( tokens[2].c_str() );
        } else {
          isVertexProperty = false;
        }
      } else if ( tokens[0] == "property" && isVertexProperty ) {
        if ( tokens.size() != 3 ) {
          std::cout << "Error: corrupted property info!" << std::endl;
          return false;
        }
        const std::string& type = tokens[1];
        attributesInfo.resize( attributesInfo.size() + 1 );
        AttributeInfo& att = attributesInfo.back();
        att.name_          = tokens[2];
        if ( type == "float64" || type == "uint64" || type == "int64" )
          att.byteCount_ = 8;
        else if ( type == "float" || type == "float32" || type == "uint32" || type == "int32" )
          att.byteCount_ = 4;
        else if ( type == "uint16" || type == "int16" )
          att.byteCount_ = 2;
        else if ( type == "uchar" || type == "uint8" || type == "char" || type == "int8" )
          att.byteCount_ = 1;
      } else if ( tokens[0] == "end_header" ) {
        break;
      }
    }
    if ( version != 1.0 ) {
      std::cout << "Error: non-supported version!" << std::endl;
      return false;
    }
    for ( auto& el : attributesInfo ) {
      el.index_ = -1;
      for ( int32_t i = 0; i < (int32_t)g_names.size(); i++ )
        if ( el.name_ == g_names[i] ) el.index_ = i;
    }
    resize( pointCount );
    if ( isAscii ) {
      size_t i              = 0;
      size_t attributeCount = attributesInfo.size();
      while ( !ifs.eof() && i < pointCount ) {
        ifs.getline( tmp, maxBufferSize );
        getTokens( tmp, tokens );
        if ( tokens.empty() ) { continue; }
        if ( tokens.size() < attributeCount ) { return false; }
        for ( const auto& el : attributesInfo ) setValue( i, el.index_, (float)atof( tokens[el.index_].c_str() ) );
        ++i;
      }
    } else {
      const int         size = std::accumulate( std::begin( attributesInfo ), std::end( attributesInfo ), 0,
                                        []( int a, const AttributeInfo& el ) { return a + (int)el.byteCount_; } );
      std::vector<char> data( pointCount * size );
      ifs.read( data.data(), data.size() );
      auto* ptr = data.data();
      for ( size_t i = 0; i < pointCount; ++i ) {
        for ( const auto& el : attributesInfo ) {
          setValue( i, el.index_, *reinterpret_cast<float*>( ptr ) );
          ptr += el.byteCount_;
        }
      }
      ifs.close();
    }
    return true;
  }

  //============================================================================

  void saveHeader( std::ofstream& fout, const bool ascii ) {
    fout << "ply" << std::endl;
    if ( cameraPosition_.size() != 0 ) {
      fout << "comment camera_position_header "    //
           << std::right << std::fixed             //
           << std::setw( 8 ) << "imageId"          //
           << " " << std::setw( 8 ) << "cameraId"  //
           << " " << std::setw( 16 ) << "pos.x"    //
           << " " << std::setw( 16 ) << "pos.y"    //
           << " " << std::setw( 16 ) << "pos.z"    //
           << " " << std::setw( 16 ) << "quat.w"   //
           << " " << std::setw( 16 ) << "quat.x"   //
           << " " << std::setw( 16 ) << "quat.y"   //
           << " " << std::setw( 16 ) << "quat.z"   //
           << " " << std::setw( 16 ) << "focal.x"  //
           << " " << std::setw( 16 ) << "focal.y"  //
           << " " << std::setw( 16 ) << "name"     //
           << std::endl;
      for ( auto& el : cameraPosition_ ) {
        fout << "comment camera_position        "                    //
             << std::right << std::fixed << std::setprecision( 10 )  //
             << std::setw( 8 ) << el.imageId_ << " "                 //
             << std::setw( 8 ) << el.cameraId_ << " "                //
             << std::setw( 16 ) << el.position_.x << " "             //
             << std::setw( 16 ) << el.position_.y << " "             //
             << std::setw( 16 ) << el.position_.z << " "             //
             << std::setw( 16 ) << el.quaternion_.w << " "           //
             << std::setw( 16 ) << el.quaternion_.x << " "           //
             << std::setw( 16 ) << el.quaternion_.y << " "           //
             << std::setw( 16 ) << el.quaternion_.z << " "           //
             << std::setw( 16 ) << el.focal_.x << " "                //
             << std::setw( 16 ) << el.focal_.y << " "                //
             << std::setw( 16 ) << " " + el.name_ << std::endl;
      }
    }
    if ( ascii )
      fout << "format ascii 1.0" << std::endl;
    else
      fout << "format binary_" << ( isLittleEndian() ? "little" : "big" ) << "_endian 1.0" << std::endl;
    fout << "element vertex " << size() << std::endl;
    for ( auto& el : g_names ) fout << "property " << ( ascii ? "float" : "float" ) << " " << el << std::endl;
    fout << "end_header" << std::endl;
  }

  //============================================================================

  void saveBody( std::ofstream& fout, const bool ascii ) {
    if ( ascii ) {
      fout << std::fixed << std::setprecision( 5 );
      for ( auto& el : gs_ ) {
        fout << el.xyz_.x << " " << el.xyz_.y << " " << el.xyz_.z << " ";
        fout << "0 0 0 ";
        for ( int i = 0; i < 16; i++ ) fout << el.sh_[i].x << " " << el.sh_[i].y << " " << el.sh_[i].z << " ";
        fout << el.opacity_ << " ";
        fout << el.scale_.x << " " << el.scale_.y << " " << el.scale_.z << " ";
        fout << el.rotate_.w << " " << el.rotate_.x << " " << el.rotate_.y << " " << el.rotate_.z << "\n";
      }
    } else {
      std::vector<char> data( size() * ( 3 + 3 + 1 + 3 + 45 + 3 + 4 ) * sizeof( float ) );
      auto              ptr = data.data();
      glm::vec3         nor( 0 );
      int               index = 0;
      for ( auto& el : gs_ ) {
        memcpy( ptr, reinterpret_cast<char*>( &el.xyz_ ), sizeof( float ) * 3 );
        ptr += 3 * sizeof( float );
        memcpy( ptr, reinterpret_cast<char*>( &nor ), sizeof( float ) * 3 );
        ptr += 3 * sizeof( float );
        for ( size_t j = 0; j < 16; j++ ) {
          memcpy( ptr, reinterpret_cast<char*>( &el.sh_[j] ), sizeof( float ) * 3 );
          ptr += 3 * sizeof( float );
        }
        memcpy( ptr, reinterpret_cast<char*>( &el.opacity_ ), sizeof( float ) );
        ptr += 1 * sizeof( float );
        memcpy( ptr, reinterpret_cast<char*>( &el.scale_ ), sizeof( float ) * 3 );
        ptr += 3 * sizeof( float );
        memcpy( ptr, reinterpret_cast<char*>( &el.rotate_.w ), sizeof( float ) );
        ptr += sizeof( float );
        memcpy( ptr, reinterpret_cast<char*>( &el.rotate_.x ), sizeof( float ) );
        ptr += sizeof( float );
        memcpy( ptr, reinterpret_cast<char*>( &el.rotate_.y ), sizeof( float ) );
        ptr += sizeof( float );
        memcpy( ptr, reinterpret_cast<char*>( &el.rotate_.z ), sizeof( float ) );
        ptr += sizeof( float );
        index++;
      }
      fout.write( data.data(), data.size() );
    }
  }

  //============================================================================

  bool save( const std::string& filename, const int32_t index, const bool ascii = false ) {
    const auto    name = createFilename( filename, index );
    std::ofstream fout( name, ascii ? std::ofstream::out : ( std::ofstream::out | std::ofstream::binary ) );
    if ( !fout.is_open() ) { return false; }
    saveHeader( fout, ascii );
    saveBody( fout, ascii );
    fout.close();
    return true;
  }

  //============================================================================

 private:
  //============================================================================

  inline void setValue( size_t pointIndex, int32_t elementIndex, float value ) {
    if ( elementIndex < 3 ) {
      gs_[pointIndex].xyz_[elementIndex] = value;
    } else if ( elementIndex < 6 ) {
      // gs_[pointIndex].nor_[elementIndex - 3] = value;
    } else if ( elementIndex < 54 ) {
      int j                     = ( elementIndex - 6 ) / 3;
      int k                     = ( elementIndex - 6 ) % 3;
      gs_[pointIndex].sh_[j][k] = value;
    } else if ( elementIndex < 55 ) {
      gs_[pointIndex].opacity_ = value;
    } else if ( elementIndex < 58 ) {
      gs_[pointIndex].scale_[elementIndex - 55] = value;
    } else if ( elementIndex < 62 ) {
      if ( elementIndex == 58 ) {
        gs_[pointIndex].rotate_.w = value;
      } else if ( elementIndex == 59 ) {
        gs_[pointIndex].rotate_.x = value;
      } else if ( elementIndex == 60 ) {
        gs_[pointIndex].rotate_.y = value;
      } else if ( elementIndex == 61 ) {
        gs_[pointIndex].rotate_.z = value;
      }
    } else {
      printf( "setValue wrong index > 62 \n" );
      exit( -1 );
    }
  }

  //============================================================================

  std::vector<GaussianData> gs_;
  CameraPosition            cameraPosition_;
};

//============================================================================
