# 3.8+ for project(LANGUAGES CUDA)
# 3.9+ for OpenMP::OpenMP_CXX
# 3.10+ findopenmp gained support for language-specific components
# 3.11+ for CMake not to add -fopenmp to the nvcc flags
# 3.13+ for target_link_directories
# 3.17+ for FindCudaToolkit
cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(hwgraph LANGUAGES CXX VERSION 0.1.0.0)

include(CheckLanguage)
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

option(USE_NVML "compile with NVML support" ON)
option(USE_HWLOC "compile with hwloc support" ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include("${CMAKE_CURRENT_LIST_DIR}/cmake/GetGitRevisionDescription.cmake")

get_git_head_revision(GIT_REFSPEC GIT_HASH)
git_local_changes(GIT_LOCAL_CHANGES)
message(STATUS GIT_REFSPEC=${GIT_REFSPEC})
message(STATUS GIT_HASH=${GIT_HASH})
message(STATUS GIT_LOCAL_CHANGES=${GIT_LOCAL_CHANGES})

add_library(hwgraph INTERFACE)
add_library(hwgraph::hwgraph ALIAS hwgraph)

target_compile_features(hwgraph INTERFACE cxx_std_11)

if (USE_NVML)
  find_package(CUDAToolkit REQUIRED)
  if (CUDAToolkit_FOUND)
    message(STATUS "found CUDAToolkit " ${CUDAToolkit_LIBRARY_DIR})
    message(STATUS "CUDA_nvml_LIBRARY " ${CUDA_nvml_LIBRARY})
    message(STATUS "CUDAToolkit_LIBRARY_DIR " ${CUDAToolkit_LIBRARY_DIR})
    message(STATUS "CUDAToolkit_INCLUDE_DIRS " ${CUDAToolkit_INCLUDE_DIRS})
  endif()

  target_link_libraries(hwgraph INTERFACE CUDA::nvml)
  target_compile_definitions(hwgraph INTERFACE -DHWGRAPH_USE_NVML=1)
else()
  target_compile_definitions(hwgraph INTERFACE -DHWGRAPH_USE_NVML=0)
endif()

if (USE_HWLOC)
  find_package(Hwloc)
  if (Hwloc_FOUND)
    message(STATUS "Hwloc found: " ${HWLOC_INCLUDE_DIR} ", " ${HWLOC_LIBRARY})
    target_link_libraries(hwgraph INTERFACE Hwloc::Hwloc)
    target_compile_definitions(hwgraph INTERFACE -DHWGRAPH_USE_HWLOC=1)
  else()
    message(STATUS "Hwloc NOT found")
    target_compile_definitions(hwgraph INTERFACE -DHWGRAPH_USE_HWLOC=0)
  endif()
else()
  message(STATUS "Hwloc not requested")
  target_compile_definitions(hwgraph INTERFACE -DHWGRAPH_USE_HWLOC=0)
endif()


## Add include directories
target_include_directories(hwgraph INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_subdirectory(src)

# "this command should be in the source directory root for CTest to find the test file"
enable_testing() 
add_subdirectory(test)

