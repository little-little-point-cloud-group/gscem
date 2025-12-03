
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

#include "util/pointcloud.hpp"
#include "util/cameraPosition.hpp"
#include "program_options_lite.h"

//============================================================================

struct Parameters {
  // Global
  bool verbose_ = false;

  // Input
  std::string input_  = {};
  std::string image_  = {};
  std::string camera_ = {};
  std::string json_   = {};

  // Output
  std::string output_ = {};
  bool        ascii_  = false;

  void printHelp( df::program_options_lite::Options& eOptions,
                  const std::string /*unused*/,
                  df::program_options_lite::ErrorReporter& errorReporter ) {
    printf( "input parameters must be:\n" );
    doHelp( std::cout, eOptions );
  }

  bool parse( int argc, char* argv[] ) {
    bool                                    ret = true;
    df::program_options_lite::Options       eOptions;
    df::program_options_lite::ErrorReporter err;
    /* clang-format off */
    eOptions.addOptions()
      ( "v,verbose",        verbose_,      false,           "Verbose" )                  
      ( "i,input",          input_,        std::string(""), "Path to input point cloud" ) 
      ( "camera",           camera_,        std::string(""), "Path to camera information:\n"
                                                            " - cameras.txt\n" 
                                                            " - cameras.bin" ) 
      ( "image",            image_,        std::string(""), "Path to image information:\n"
                                                            " - images.txt\n" 
                                                            " - images.bin" ) 
      ( "json",             json_,         std::string(""), "Path to json file" )                                                            
      ( "o,output",         output_,       std::string(""), "Path to output point cloud" ) 
      ( "ascii",            ascii_,        false,           "Create ascii point cloud" ) 
      ;
    // clang-format on
    setDefaults( eOptions );
    try {
      scanArgv( eOptions, argc, (const char**)argv, err );
    } catch ( df::program_options_lite::ParseFailure& e ) {
      printHelp( eOptions, "", err );
      std::cerr << "Parsing error: option: \"" << e.arg << "\" and value: \"" << e.val << "\" are not supported. \n";
      return false;
    }
    if ( argc == 1 || err.is_errored ) {
      printHelp( eOptions, "", err );
      return false;
    }

    // Check
    if ( input_.empty() || !exists( input_ ) ) {
      printf( "Input path is not set or file not exists: %s \n", input_.c_str() );
      ret = false;
    }
    if ( !image_.empty() && !exists( image_ ) ) {
      printf( "Image path not exists: %s \n", image_.c_str() );
      ret = false;
    }
    if ( !camera_.empty() && !exists( camera_ ) ) {
      printf( "Camera path not exists: %s \n", camera_.c_str() );
      ret = false;
    }
    if ( !json_.empty() && !exists( json_ ) ) {
      printf( "Json path not exists: %s \n", json_.c_str() );
      ret = false;
    }
    return ret;
  }
};

//============================================================================

int main( int argc, char* argv[] ) {
  Parameters params;
  if ( !params.parse( argc, argv ) ) { return 1; }

  // Read Pointclouds
  Pointcloud pc;
  pc.load( params.input_, 0 );

  // Read camera position
  auto& cameraPosition = pc.cameraPosition();
  if ( !params.camera_.empty() ) cameraPosition.readCamera( params.camera_ );
  if ( !params.image_.empty() ) cameraPosition.readImage( params.image_ );
  if ( !params.json_.empty() ) cameraPosition.readJson( params.json_ );

  // Log camera position
  if ( params.verbose_ ) cameraPosition.trace();

  // Save output
  if ( !params.output_.empty() ) pc.save( params.output_, 0, params.ascii_ );
  return 0;
}

//============================================================================
