
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

#if defined( WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

//============================================================================

static inline bool isLittleEndian() {
  uint32_t num = 1;
  return ( *( reinterpret_cast<char*>( &num ) ) ) == 1;
}

//============================================================================

static inline bool exists( const std::string& name ) { return ( !name.empty() ) && std::filesystem::exists( name ); }

//============================================================================

static std::string dirname( const std::string& name ) { return std::filesystem::path( name ).parent_path().string(); }

//============================================================================

static std::string basename( const std::string& name ) { return std::filesystem::path( name ).stem().string(); }

//============================================================================

static std::string removeExtension( const std::string& name ) {
  size_t dotPosition = name.find_last_of( '.' );
  if ( dotPosition != std::string::npos ) { return name.substr( 0, dotPosition ); }
  return name;
}

//============================================================================

static std::string extension( const std::string& eFilename ) {
  auto position = eFilename.find_last_of( '.' );
  if ( position != std::string::npos ) {
    return eFilename.substr( position + 1 );
  } else {
    return std::string( "" );
  }
}

//============================================================================

static void mkdir( const std::string& name ) {
  if ( !name.empty() ) std::filesystem::create_directory( name );
}

//============================================================================

#ifdef WIN32
#define snprintf _snprintf
#endif
template <typename... Args>
std::string stringFormat( const char* pFormat, Args... args ) {
  size_t      iSize = snprintf( nullptr, 0, pFormat, args... );
  std::string pString;
  pString.reserve( iSize + 1 );
  pString.resize( iSize );
  snprintf( &pString[0], iSize + 1, pFormat, args... );
  return pString;
}

//============================================================================

static std::string createFilename( const std::string& pFilename, int iFrameIndex ) {
  std::string sFilename = stringFormat( pFilename.c_str(), iFrameIndex );
#ifdef MSVC
  char pNewName[512];
  GetFullPathName( sFilename.c_str(), 512, pNewName, NULL );
  sFilename = pNewName;
#endif
  return sFilename;
}

//============================================================================

static bool compareSeparators( const char c ) { return c != ' ' && c != '\t' && c != '\r'; }

//============================================================================

template <typename T>
static void operator+=( std::vector<T>& a, const std::vector<T>& b ) {
  a.insert( a.end(), b.begin(), b.end() );
}

//============================================================================

template <typename T>
static bool operator==( const std::vector<T>& a, const std::vector<T>& b ) {
  int32_t error = 0;
  bool    match = true;
  if ( a.size() != b.size() ) {
    printf( "Error: src/rec size are different : %7zu != %7zu \n", a.size(), b.size() );
    match = false;
  }
  for ( size_t i = 0; i < a.size(); i++ )
    if ( a[i] != b[i] ) {
      printf( "Error: src[%4zu] != rec[%4zu] <=> %7d != %7d \n", i, i, (int32_t)a[i], (int32_t)b[i] );
      match = false;
      error++;
      if ( error == 32 ) {
        printf( "...\n" );
        fflush( stdout );
        break;
      }
    }
  return match;
}

//============================================================================

template <typename T>
bool isEqual( const T* src, const T* rec, size_t size ) {
  bool match = true;
  for ( size_t i = 0; i < size; i++ )
    if ( src[i] != rec[i] ) {
      printf( "Error: src[%4zu] != rec[%4zu] <=> %9zu != %9zu \n", i, i, (uint64_t)src[i], (uint64_t)rec[i] );
      match = false;
    }
  return match;
}

//============================================================================

static void write( std::string filename, const std::vector<uint8_t>& buffer ) {
  std::ofstream file( filename, std::ios::out | std::ios::binary );
  if ( file.is_open() ) {
    file.write( (char*)buffer.data(), buffer.size() );
    file.close();
  } else {
    printf( "can't write %s \n", filename.c_str() );
    fflush( stdout );
    exit( -1 );
  }
}

//============================================================================

static std::vector<uint8_t> read( std::string filename ) {
  std::vector<uint8_t> buffer;
  std::ifstream        file( filename, std::ios::in | std::ios::binary );
  if ( file.is_open() ) {
    auto length = file.tellg();
    file.seekg( 0, std::ios::end );
    length = file.tellg() - length;
    buffer.resize( length, 0 );
    file.seekg( 0, std::ios_base::beg );
    file.read( (char*)buffer.data(), buffer.size() );
    file.close();
  } else {
    printf( "can't read %s \n", filename.c_str() );
    fflush( stdout );
    exit( -1 );
  }
  return buffer;
}

//============================================================================

static void log( const std::string& name, const std::vector<std::vector<uint8_t>>& ctx ) {
  printf( "  %s = {  \n", name.c_str() );
  for ( const auto& line : ctx ) {
    printf( "    { " );
    int i = 0;
    for ( const auto& el : line ) {
      printf( "%3u, ", el );
      if ( i % 16 == 15 && i + 1 != (int)line.size() ) printf( "\n      " );
      i++;
    }
    printf( "},\n" );
  }
  printf( "  };\n" );
  fflush( stdout );
}

//============================================================================

static void log( glm::mat3& m ) {
  printf( "mat3 %12.8f %12.8f %12.8f \n", m[0][0], m[0][1], m[0][2] );
  printf( "     %12.8f %12.8f %12.8f \n", m[1][0], m[1][1], m[1][2] );
  printf( "     %12.8f %12.8f %12.8f \n", m[2][0], m[2][1], m[2][2] );
}

//============================================================================

static void log( glm::mat4& m ) {
  printf( "mat4 %12.8f %12.8f %12.8f %12.8f \n", m[0][0], m[0][1], m[0][2], m[0][3] );
  printf( "     %12.8f %12.8f %12.8f %12.8f \n", m[1][0], m[1][1], m[1][2], m[1][3] );
  printf( "     %12.8f %12.8f %12.8f %12.8f \n", m[2][0], m[2][1], m[2][2], m[2][3] );
  printf( "     %12.8f %12.8f %12.8f %12.8f \n", m[3][0], m[3][1], m[3][2], m[3][3] );
}

//============================================================================

static void log( glm::quat& q ) { printf( "quat %12.8f %12.8f %12.8f %12.8f \n", q.x, q.y, q.z, q.w ); }
static void log( glm::vec3& v ) { printf( "vec3 %12.8f %12.8f %12.8f \n", v.x, v.y, v.z ); }
static void log( glm::vec4& v ) { printf( "vec4 %12.8f %12.8f %12.8f  %12.8f \n", v.x, v.y, v.z, v.w ); }

//============================================================================

static std::ostream& operator<<( std::ostream& os, const glm::vec2& vec ) {
  for ( int i = 0; i < vec.length(); i++ ) os << vec[i] << " ";
  return os;
}

//============================================================================

static std::ostream& operator<<( std::ostream& os, const glm::vec3& vec ) {
  for ( int i = 0; i < vec.length(); i++ ) os << vec[i] << " ";
  return os;
}

//============================================================================

static std::ostream& operator<<( std::ostream& os, const glm::vec4& vec ) {
  for ( int i = 0; i < vec.length(); i++ ) os << vec[i] << " ";
  return os;
}

//============================================================================

static std::ostream& operator<<( std::ostream& os, const glm::quat& q ) {
  os << q.w << " " << q.x << " " << q.y << " " << q.z << " ";
  return os;
}

//============================================================================

template <typename T>
static std::ostream& operator<<( std::ostream& os, const std::vector<T>& vec ) {
  for ( size_t i = 0; i < vec.size(); i++ ) os << vec[i] << " ";
  return os;
}

//============================================================================
