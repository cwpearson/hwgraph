set -x -e

source ci/env.sh

# Install Cmake if it doesn't exist
mkdir -p $CMAKE_PREFIX
if [[ ! -f $CMAKE_PREFIX/bin/cmake ]]; then
    if [[ $TRAVIS_CPU_ARCH == "ppc64le" ]]; then
        wget -qSL https://github.com/Kitware/CMake/releases/download/v3.17.0-rc3/cmake-3.17.0-rc3.tar.gz -O cmake.tar.gz
        tar -xf cmake.tar.gz --strip-components=1 -C $CMAKE_PREFIX
        rm cmake.tar.gz
        cd $CMAKE_PREFIX
        ./bootstrap --prefix=$CMAKE_PREFIX
        make -j `nproc` install
    elif [[ $TRAVIS_CPU_ARCH == "amd64" ]]; then
        wget -qSL https://github.com/Kitware/CMake/releases/download/v3.17.0-rc3/cmake-3.17.0-rc3-Linux-x86_64.tar.gz -O cmake.tar.gz
        tar -xf cmake.tar.gz --strip-components=1 -C $CMAKE_PREFIX
        rm cmake.tar.gz
    fi
fi
cd $HOME

## install hwloc
sudo apt-get update 
sudo apt-get install -y --no-install-recommends \
  libhwloc-dev

## install CUDA
if [[ $USE_NVML == "ON" ]]; then
    sudo apt-key adv --fetch-keys http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1604/x86_64/7fa2af80.pub

    if [[ $TRAVIS_CPU_ARCH == "ppc64le" ]]; then
        CUDA102="https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/ppc64el/cuda-repo-ubuntu1804_10.2.89-1_ppc64el.deb"
    elif [[ $TRAVIS_CPU_ARCH == "amd64" ]]; then
        CUDA102="http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-repo-ubuntu1804_10.2.89-1_amd64.deb"
    fi

    wget -SL $CUDA102 -O cuda.deb
    sudo dpkg -i cuda.deb
    sudo apt-get update 
    sudo apt-get install -y --no-install-recommends \
      cuda-toolkit-10-2
    find /usr/local/cuda-10.2 -name "libnvidia-ml.so"

    ls -l /usr/local/cuda-10.2/lib64/stubs

    ## FindCUDAToolkit doesn't seem to look for libraries in the targets folder
    # if [[ $TRAVIS_CPU_ARCH == "ppc64le" ]]; then
    #     sudo ln -s /usr/local/cuda-10.2/targets/ppc64le-linux/lib/stubs/libnvidia-ml.so /usr/local/cuda-10.2/lib64/stubs/libnvidia-ml.so
    # elif [[ $TRAVIS_CPU_ARCH == "amd64" ]]; then
    #     sudo ln -s /usr/local/cuda-10.2/targets/x86_64-linux/lib/stubs/libnvidia-ml.so /usr/local/cuda-10.2/lib64/stubs/libnvidia-ml.so
    # fi
fi