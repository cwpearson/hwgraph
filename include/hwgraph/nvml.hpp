#pragma once

#include <cstdio>
#include <iostream>

#include <nvml.h>

#include "graph.hpp"

namespace hwgraph {
namespace nvml {

inline void checkNvml(nvmlReturn_t result, const char *file, const int line) {
  if (result != NVML_SUCCESS) {
    fprintf(stderr, "nvml Error: %s in %s : %d\n", nvmlErrorString(result),
            file, line);
    exit(-1);
  }
}

#define NVML(stmt) checkNvml(stmt, __FILE__, __LINE__);

static bool once = false;
class Initer {
private:
public:
  Initer() {
    if (!once) {
      NVML(nvmlInit());
      once = true;
    }
  }
};

Initer initer;

// https://github.com/NVIDIA/nccl/blob/6c61492eba5c25ac6ed1bf57de23c6a689aa75cc/src/graph/topo.cc#L222
inline void add_nvlinks(hwgraph::Graph &graph) {

  unsigned int deviceCount;
  NVML(nvmlDeviceGetCount(&deviceCount));
  for (unsigned int devIdx = 0; devIdx < deviceCount; ++devIdx) {

    std::cerr << "Querying NVML device " << devIdx << "\n";
    nvmlDevice_t dev;
    NVML(nvmlDeviceGetHandleByIndex(devIdx, &dev));

    int cudaMajor, cudaMinor;
    NVML(nvmlDeviceGetCudaComputeCapability(dev, &cudaMajor, &cudaMinor));
    int maxNvLinks, width;
    if (cudaMajor < 6) {
      maxNvLinks = 0;
      width = 0;
    } else if (cudaMajor == 6) {
      maxNvLinks = 4;
      width = 12;
    } else {
      maxNvLinks = 6;
      width = 16;
    }

    for (int l = 0; l < maxNvLinks; ++l) {

      nvmlEnableState_t isActive;
      const auto ret = nvmlDeviceGetNvLinkState(dev, l, &isActive);
      if (NVML_ERROR_NOT_SUPPORTED == ret) { // GPU does not support NVLink
        std::cerr << "GPU does not support NVLink\n";
        break; // no need to check all links
      } else if (NVML_FEATURE_ENABLED != ret) {
        std::cerr << "link not active on GPU\n";
        continue;
      }

      // Get the PCI address of this device
      nvmlPciInfo_t pciInfo;
      NVML(nvmlDeviceGetPciInfo(dev, &pciInfo));
      PciAddress addr = {pciInfo.domain, pciInfo.bus, pciInfo.device, 0};
      auto local = graph.get_pci(addr);

      // figure out what's on the other side
      NVML(nvmlDeviceGetNvLinkRemotePciInfo(dev, l, &pciInfo));
      addr = {pciInfo.domain, pciInfo.bus, pciInfo.device, 0};
      auto remote = graph.get_pci(addr);

      if (remote->type_ == Vertex::Type::Gpu) {
        std::cerr << "connect to remote GPU\n";

        // nvlink will be visible from both sides, so only connect one way
        if (local->data_.pciDev_.addr < remote->data_.pciDev_.addr) {
          auto link = Edge::new_nvlink();
          graph.join(local, remote, link);
        }

      } else if (remote->type_ == Vertex::Type::Bridge) {
        auto link = Edge::new_nvlink();
        graph.join(local, remote, link);
      } else {
        std::cerr << "nvswitch?\n";
        assert(0);
      }
    }
  }
}

} // namespace nvml
} // namespace hwgraph
