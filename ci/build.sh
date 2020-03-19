set -x -e

source ci/env.sh

which g++
which cmake

g++ --version
cmake --version

if [[ $USE_NVML == "ON" ]]; then
  find /usr/local/cuda-10.2 -name "libnvidia-ml.so"
  which nvcc
  nvcc --version
fi

mkdir build
cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DUSE_NVML=$USE_NVML \
  -DCUDA_nvml_LIBRARY=/usr/local/cuda-10.2/lib64/stubs/libnvidia-ml.so
make VERBOSE=1 