set( DIR ${CMAKE_SOURCE_DIR}/dependencies/glm )
if( NOT EXISTS ${DIR}/CMakeLists.txt )
  include(FetchContent)
  FetchContent_Declare( glm
                        GIT_REPOSITORY  https://github.com/g-truc/glm.git
                        GIT_TAG         1.0.0
                        SOURCE_DIR      ${DIR} )
                        FetchContent_MakeAvailable(glm)
endif()
