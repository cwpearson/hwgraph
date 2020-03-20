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
inline void add_gpus(hwgraph::Graph &graph) {

  unsigned int deviceCount;
  NVML(nvmlDeviceGetCount(&deviceCount));
  for (unsigned int devIdx = 0; devIdx < deviceCount; ++devIdx) {

    std::cerr << "Querying NVML device " << devIdx << "\n";
    nvmlDevice_t dev;
    NVML(nvmlDeviceGetHandleByIndex(devIdx, &dev));

    int cudaMajor, cudaMinor;
    NVML(nvmlDeviceGetCudaComputeCapability(dev, &cudaMajor, &cudaMinor));

    // get the name of this device
    char name[64];
    NVML(nvmlDeviceGetName(dev, name, sizeof(name)));

    // Get the PCI address of this device
    nvmlPciInfo_t pciInfo;
    NVML(nvmlDeviceGetPciInfo(dev, &pciInfo));
    PciAddress addr = {pciInfo.domain, pciInfo.bus, pciInfo.device, 0};
    auto local = graph.get_pci(addr);
    assert(local);

    if (local) {
      std::cerr << "add_gpus(): replace\n";
      auto gpu = Vertex::new_gpu(name, local->data_.pciDev);
      graph.replace(local, gpu);
    } else {
      std::cerr << "add_gpus(): new\n";
      assert(0);
    }
  }
}
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
    int maxNvLinks;
    if (cudaMajor < 6) {
      maxNvLinks = 0;
    } else if (cudaMajor == 6) {
      maxNvLinks = 4;
    } else {
      maxNvLinks = 6;
    }

    for (int l = 0; l < maxNvLinks; ++l) {

      nvmlEnableState_t isActive;
      const auto ret = nvmlDeviceGetNvLinkState(dev, l, &isActive);
      if (NVML_ERROR_NOT_SUPPORTED == ret) { // GPU does not support NVLink
        std::cerr << "GPU does not support NVLink\n";
        break; // no need to check all links
      } else if (NVML_FEATURE_ENABLED != isActive) {
        std::cerr << "link not active on GPU\n";
        continue;
      }

      // Get the PCI address of this device
      nvmlPciInfo_t pciInfo;
      NVML(nvmlDeviceGetPciInfo(dev, &pciInfo));
      PciAddress addr = {pciInfo.domain, pciInfo.bus, pciInfo.device, 0};
      std::cerr << "add_nvlinks(): local " << addr.str() << "\n";
      auto local = graph.get_pci(addr);
      assert(local->type_ == Vertex::Type::Gpu);

      // figure out what's on the other side
      NVML(nvmlDeviceGetNvLinkRemotePciInfo(dev, l, &pciInfo));
      addr = {pciInfo.domain, pciInfo.bus, pciInfo.device, 0};
      auto remote = graph.get_pci(addr);

      std::cerr << "add_nvlinks(): remote " << addr.str() << "\n";

      unsigned int version;
      NVML(nvmlDeviceGetNvLinkVersion(dev, l, &version));

      if (remote->type_ == Vertex::Type::Gpu) {
        std::cerr << "remote is " << remote->str() << "\n";

        // nvlink will be visible from both sides, so only connect one way
        if (local->data_.gpu.pciDev.addr < remote->data_.gpu.pciDev.addr) {
          auto link = Edge::new_nvlink(version, 1);
          graph.join(local, remote, link);
        }

      } else if (remote->type_ == Vertex::Type::NvLinkBridge) {
        std::cerr << "remote is " << remote->str() << "\n";
        auto link = Edge::new_nvlink(version, 1);
        graph.join(local, remote, link);
      } else if (remote->type_ == Vertex::Type::NvSwitch) {
        std::cerr << "nvswitch?\n";
        std::cerr << "remote is " << remote->str() << "\n";
        assert(0);
      } else {
        std::cerr << "unexpected nvlink endpoint\n";
        assert(0);
      }
    }
  }

  /*
  each NvLink connected component may have multiple nvlinks here.
  We combine them into a single nvlink with a larger lane count
  */

  bool changed = true;
  while (changed) {
    changed = false;

    /*
    look through all edges for an nvlink
    if we find one, look for another nvlink between the same verts
    if we find one, combine them and start over
    */
    for (auto &i : graph.edges()) {
      if (i->type_ == Edge::Type::Nvlink) {
        for (auto &j : graph.edges()) {
          if (i->same_vertices(j)) {

            std::cerr << "add_nvlinks(): combining " << i->str() << " and " << j->str() << "\n"; 

            assert(i->data_.nvlink.version == j->data_.nvlink.version);
            i->data_.nvlink.lanes += j->data_.nvlink.lanes;
            graph.erase(j); // invalidated iterators
            changed = true;
            goto loop_end;
          }
        }
      }
    }
  loop_end:;
  }

  /*
  NvLink lanes have been combined
  GPU-CPU NvLinks are connected to an NvLinkBridge, which is connected to the hostbridge
  we can treat these connections as infinite bandwidth when computing bandwidth, so this is fine for now
  */


}

} // namespace nvml
} // namespace hwgraph
