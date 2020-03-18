set -x -e

source ci/env.sh

which g++
which nvcc
which cmake

g++ --version
nvcc --version
cmake --version

mkdir build
cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DUSE_NVML=$USE_NVML
make VERBOSE=1 