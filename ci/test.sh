set -x -e

source ci/env.sh

cd build

if [[ $USE_NVML == "OFF" ]]; then
    make test
fi