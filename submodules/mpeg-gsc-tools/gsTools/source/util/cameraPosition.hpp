
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
#include "json/json.hpp"

//============================================================================

struct CameraData {
  int        cameraId_ = 0;
  glm::vec2  focal_    = { 0.f, 0.f };
  glm::vec2  center_   = { 0.f, 0.f };
  glm::ivec2 size_     = { 0., 0. };
};

//============================================================================

struct ImageData {
  int         imageId_    = 0;
  glm::quat   quaternion_ = { 0.f, 0.f, 0.f, 0.f };
  glm::vec3   position_   = { 0.f, 0.f, 0.f };
  int         cameraId_   = 0;
  glm::vec2   focal_      = { 0.f, 0.f };
  std::string name_       = {};
};

//============================================================================

enum class CameraModel : int {
  kInvalid             = -1,
  kSimplePinhole       = 0,
  kPinhole             = 1,
  kSimpleRadial        = 2,
  kRadial              = 3,
  kOpenCV              = 4,
  kOpenCVFisheye       = 5,
  kFullOpenCV          = 6,
  kFOV                 = 7,
  kSimpleRadialFisheye = 8,
  kRadialFisheye       = 9,
};

//============================================================================

static CameraModel getModelId( const std::string& modelName ) {
  if ( modelName == "SIMPLE_PINHOLE" ) return CameraModel::kSimplePinhole;
  if ( modelName == "PINHOLE" ) return CameraModel::kPinhole;
  if ( modelName == "SIMPLE_RADIAL" ) return CameraModel::kSimpleRadial;
  if ( modelName == "RADIAL" ) return CameraModel::kRadial;
  if ( modelName == "OPENCV" ) return CameraModel::kOpenCV;
  if ( modelName == "OPENCV_FISHEYE" ) return CameraModel::kOpenCVFisheye;
  if ( modelName == "FULL_OPENCV" ) return CameraModel::kFullOpenCV;
  if ( modelName == "FOV" ) return CameraModel::kFOV;
  if ( modelName == "SIMPLE_RADIAL_FISHEYE" ) return CameraModel::kSimpleRadialFisheye;
  if ( modelName == "RADIAL_FISHEYE" ) return CameraModel::kRadialFisheye;
  /* otherwise                           */ return CameraModel::kInvalid;
}

//============================================================================

class CameraPosition {
 public:
  CameraPosition() {}

  CameraPosition& operator=( const CameraPosition& other ) {
    if ( this == &other ) return *this;
    imageData_ = other.imageData_;
    return *this;
  }
  auto         size() const { return imageData_.size(); }
  inline auto  operator[]( const size_t index ) const { return imageData_[index]; }
  inline auto& operator[]( const size_t index ) { return imageData_[index]; }
  auto         begin() { return imageData_.begin(); }
  const auto   begin() const { return imageData_.begin(); }
  auto         end() { return imageData_.end(); }
  const auto   end() const { return imageData_.end(); }

  //============================================================================

  void readCamera( const std::string& camera ) {
    auto ext = extension( camera );
    if ( ext == "txt" )
      readCameraTxt( camera );
    else if ( ext == "bin" )
      readCameraBin( camera );
    else
      printf( "%s not supported for camera \n", ext.c_str() );
  }

  //============================================================================

  void readImage( const std::string& image ) {
    auto ext = extension( image );
    if ( ext == "txt" )
      readImageTxt( image );
    else if ( ext == "bin" )
      readImageBin( image );
    else
      printf( "%s not supported for image \n", ext.c_str() );
  }

  //============================================================================

  void readJson( const std::string& filename ) {
    std::ifstream file( filename );
    if ( !file.is_open() ) {
      std::cerr << "Error : Can't open " << filename << std::endl;
      return;
    }
    nlohmann::json j;
    file >> j;
    for ( const auto& item : j ) {
      ImageData el;
      el.imageId_  = item["id"];
      el.cameraId_ = item["id"];
      el.name_     = item["img_name"];
      el.position_ = { item["position"][0], item["position"][1], item["position"][2] };
      el.focal_    = { item["fx"], item["fy"] };
      glm::mat3 mat;
      for ( int i = 0; i < 3; ++i )
        for ( int j = 0; j < 3; ++j ) mat[i][j] = item["rotation"][i][j];
      el.quaternion_ = glm::quat( mat );
      if ( el.imageId_ == 0 ) {
        log( el.position_ );
        log( mat );
        log( el.quaternion_ );
      }
      imageData_.push_back( el );
    }
  }

  //============================================================================

  void trace() {
    printf( "Log camera information: numer of position = %zu \n", imageData_.size() );
    for ( const auto& el : imageData_ ) {
      printf(
          "%4d: Quat = %8.4f %8.4f %8.4f %8.4f Pos = %8.4f %8.4f %8.4f Focal = %8.4f %8.4f CameraId = %8d Name = %s \n",
          el.imageId_, el.quaternion_.w, el.quaternion_.x, el.quaternion_.y, el.quaternion_.z, el.position_.x,
          el.position_.y, el.position_.z, el.focal_.x, el.focal_.y, el.cameraId_, el.name_.c_str() );
    }
    fflush( stdout );
  }

  //============================================================================

 private:
  //============================================================================

  void readCameraTxt( const std::string& filename ) {
    std::ifstream file( filename );
    if ( !file.is_open() ) {
      std::cerr << "Error : Can't open " << filename << std::endl;
      return;
    }
    std::string line;
    while ( std::getline( file, line ) ) {
      if ( line.empty() || line[0] == '#' ) { continue; }
      std::istringstream iss( line );
      int                cameraId = 0, width = 0, height = 0;
      double             focalX = 0, focalY = 0, centerX = 0, centerY = 0;
      std::string        model = "";
      iss >> cameraId;
      iss >> model;
      iss >> width;
      iss >> height;
      switch ( getModelId( model ) ) {
        case CameraModel::kSimplePinhole:
          iss >> focalX;
          iss >> centerX;
          iss >> centerY;
          focalY = focalX;
          break;
        case CameraModel::kPinhole:
          iss >> focalX;
          iss >> focalY;
          iss >> centerX;
          iss >> centerY;
          break;
        case CameraModel::kSimpleRadial:
          iss >> focalX;
          iss >> centerX;
          iss >> centerY;
          focalY = focalX;
          break;
        case CameraModel::kRadial:
          iss >> focalX;
          iss >> centerX;
          iss >> centerY;
          focalY = focalX;
          break;
        case CameraModel::kOpenCV:
          iss >> focalX;
          iss >> focalY;
          iss >> centerX;
          iss >> centerY;
          break;
        case CameraModel::kOpenCVFisheye:
          iss >> focalX;
          iss >> focalY;
          iss >> centerX;
          iss >> centerY;
          break;
        case CameraModel::kFullOpenCV:
          iss >> focalX;
          iss >> focalY;
          iss >> centerX;
          iss >> centerY;
          break;
        case CameraModel::kFOV:
          iss >> focalX;
          iss >> focalY;
          iss >> centerX;
          iss >> centerY;
          break;
        case CameraModel::kSimpleRadialFisheye:
          iss >> focalX;
          iss >> centerX;
          iss >> centerY;
          focalY = focalX;
          break;
        case CameraModel::kRadialFisheye:
          iss >> focalX;
          iss >> centerX;
          iss >> centerY;
          focalY = focalX;
          break;
        default: std::cerr << "Error in readCameraTxt: unsupported camera model" << std::endl; continue;
      }
      if ( !iss ) {
        std::cerr << "Error in readCameraTxt: can't read line" << std::endl;
        continue;
      }
      CameraData el;
      el.cameraId_ = cameraId;
      el.focal_    = { (float)focalX, (float)focalY };
      el.size_     = { width, height };
      el.center_   = { (float)centerX, (float)centerY };
      cameraData_.push_back( el );
    }
  }

  //============================================================================

  void readCameraBin( const std::string& filename ) {
    std::ifstream file( filename, std::ios::binary );
    if ( !file ) {
      std::cerr << "Error : Can't open " << filename << std::endl;
      return;
    }
    uint64_t numCameras = read<uint64_t>( file );
    for ( uint64_t i = 0; i < numCameras; ++i ) {
      double      focalX = 0, focalY = 0, centerX = 0, centerY = 0;
      int         cameraId = read<int>( file );
      CameraModel modelId  = static_cast<CameraModel>( read<int>( file ) );
      uint64_t    width    = read<uint64_t>( file );
      uint64_t    height   = read<uint64_t>( file );
      switch ( modelId ) {
        case CameraModel::kSimplePinhole:
          focalX  = read<double>( file );
          centerX = read<double>( file );
          centerY = read<double>( file );
          focalY  = focalX;
          break;
        case CameraModel::kPinhole:
          focalX  = read<double>( file );
          focalY  = read<double>( file );
          centerX = read<double>( file );
          centerY = read<double>( file );
          break;
        case CameraModel::kSimpleRadial:
          focalX  = read<double>( file );
          centerX = read<double>( file );
          centerY = read<double>( file );
          file.seekg( sizeof( double ), std::ios::cur );
          focalY = focalX;
          break;
        case CameraModel::kRadial:
          focalX  = read<double>( file );
          centerX = read<double>( file );
          centerY = read<double>( file );
          file.seekg( sizeof( double ) * 2, std::ios::cur );
          focalY = focalX;
          break;
        case CameraModel::kOpenCV:
          focalX  = read<double>( file );
          focalY  = read<double>( file );
          centerX = read<double>( file );
          centerY = read<double>( file );
          file.seekg( sizeof( double ) * 4, std::ios::cur );
          break;
        case CameraModel::kOpenCVFisheye:
          focalX  = read<double>( file );
          focalY  = read<double>( file );
          centerX = read<double>( file );
          centerY = read<double>( file );
          file.seekg( sizeof( double ) * 4, std::ios::cur );
          break;
        case CameraModel::kFullOpenCV:
          focalX  = read<double>( file );
          focalY  = read<double>( file );
          centerX = read<double>( file );
          centerY = read<double>( file );
          file.seekg( sizeof( double ) * 8, std::ios::cur );
          break;
        case CameraModel::kFOV:
          focalX  = read<double>( file );
          focalY  = read<double>( file );
          centerX = read<double>( file );
          centerY = read<double>( file );
          file.seekg( sizeof( double ), std::ios::cur );
          break;
        case CameraModel::kSimpleRadialFisheye:
          focalX  = read<double>( file );
          centerX = read<double>( file );
          centerY = read<double>( file );
          file.seekg( sizeof( double ), std::ios::cur );
          focalY = focalX;
          break;
        case CameraModel::kRadialFisheye:
          focalX  = read<double>( file );
          centerX = read<double>( file );
          centerY = read<double>( file );
          file.seekg( sizeof( double ) * 2, std::ios::cur );
          focalY = focalX;
          break;
        default: std::cerr << "Error in readCameraBin: unsupported camera model" << std::endl; return;
      }
      if ( !file ) {
        std::cerr << "Error in readCameraBin: can't read all camera parameters" << std::endl;
        return;
      }
      CameraData el;
      el.cameraId_ = cameraId;
      el.focal_    = { (float)focalX, (float)focalY };
      el.size_     = { width, height };
      el.center_   = { (float)centerX, (float)centerY };
      cameraData_.push_back( el );
    }
  }

  //============================================================================

  void readImageTxt( const std::string& filename ) {
    std::ifstream file( filename );
    if ( !file.is_open() ) {
      std::cerr << "Error : Can't open " << filename << std::endl;
      return;
    }
    std::string line;
    while ( std::getline( file, line ) ) {
      if ( line.empty() || line[0] == '#' ) { continue; }
      std::istringstream iss( line );
      ImageData          el;
      double             tx, ty, tz, qw, qx, qy, qz;
      if ( !( iss >> el.imageId_ >> qw >> qx >> qy >> qz >> tx >> ty >> tz >> el.cameraId_ >> el.name_ ) ) {
        std::cerr << "Error in readImageTxt: can't read line" << std::endl;
        continue;
      }
      // Inverse the transformation as in Python (t = -R.T * t )
      el.quaternion_ = glm::quat( (float)qw, (float)qx, (float)qy, (float)qz );
      el.position_   = -( glm::transpose( glm::mat3_cast( el.quaternion_ ) ) * glm::vec3( tx, ty, tz ) );

      auto it = std::find_if( cameraData_.begin(), cameraData_.end(),
                              [&]( const CameraData& camera ) { return camera.cameraId_ == el.cameraId_; } );
      if ( it != cameraData_.end() ) {
        el.focal_ = it->focal_;
      } else {
        std::cerr << "Warning: Camera ID " << el.cameraId_ << " not found in cameraData_" << std::endl;
      }
      imageData_.push_back( el );
      std::getline( file, line );
    }
  }

  //============================================================================

  void readImageBin( const std::string& filename ) {
    std::ifstream file( filename, std::ios::binary );
    if ( !file ) {
      std::cerr << "Error : Can't open " << filename << std::endl;
      return;
    }
    uint64_t numImages = read<uint64_t>( file );
    for ( uint64_t i = 0; i < numImages; ++i ) {
      ImageData el;
      el.imageId_        = read<int>( file );
      double qw          = read<double>( file );
      double qx          = read<double>( file );
      double qy          = read<double>( file );
      double qz          = read<double>( file );
      double tx          = read<double>( file );
      double ty          = read<double>( file );
      double tz          = read<double>( file );
      el.cameraId_       = read<int>( file );
      el.name_           = read<std::string>( file );
      uint64_t numPoints = read<uint64_t>( file );
      for ( uint64_t j = 0; j < numPoints; ++j ) {
        read<double>( file );
        read<double>( file );
        read<int64_t>( file );
      }
      // Inverse transformation as in Colmap: t = -R.T * t
      el.quaternion_ = glm::quat( (float)qw, (float)qx, (float)qy, (float)qz );
      el.position_   = -( glm::transpose( glm::mat3_cast( el.quaternion_ ) ) * glm::vec3( tx, ty, tz ) );

      auto it = std::find_if( cameraData_.begin(), cameraData_.end(),
                              [&]( const CameraData& camera ) { return camera.cameraId_ == el.cameraId_; } );
      if ( it != cameraData_.end() ) {
        el.focal_ = it->focal_;
      } else {
        std::cerr << "Warning: Camera ID " << el.cameraId_ << " not found in cameraData_" << std::endl;
      }
      imageData_.push_back( el );
    }
  }

  //============================================================================

  template <typename T>
  T read( std::ifstream& file ) {
    if constexpr ( std::is_same_v<T, std::string> ) {
      std::string str;
      char        c;
      while ( file.read( &c, 1 ) && c != '\0' ) { str += c; }
      return str;
    } else {
      T value;
      file.read( reinterpret_cast<char*>( &value ), sizeof( T ) );
      return value;
    }
  }

  std::vector<CameraData> cameraData_;
  std::vector<ImageData>  imageData_;
};

//============================================================================
