#pragma once

#include <cstdio>
#include <iostream>

#include <nvml.h>

#include "graph.hpp"
#include "narrow.hpp"

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

    // Get the PCI address of this device
    std::cerr << "Get PCI Info for device " << devIdx << "\n";
    nvmlPciInfo_t pciInfo;
    NVML(nvmlDeviceGetPciInfo(dev, &pciInfo));
    PciAddress addr = {safe_narrow<short unsigned int>(pciInfo.domain),
                       safe_narrow<unsigned char>(pciInfo.bus),
                       safe_narrow<unsigned char>(pciInfo.device), 0};
    auto local = graph.get_pci(addr);

    std::cerr << "matching device in graph: " << devIdx << "\n";
    std::cerr << local->str() << "\n";

    // get the name of this device
    std::cerr << "Get name for device " << devIdx << "\n";
    char name[64]; // nvml says 64 is the max size
    NVML(nvmlDeviceGetName(dev, name, sizeof(name)));

    std::cerr << "make new GPU\n";
    Vertex_t gpu = Vertex::new_gpu(name);
    std::cerr << gpu->str() << "\n";

    std::cerr << "take pci info\n";
    // update it with existing PCI info, if found
    if (local) {
      if (local->data.index() == Vertex::Type::PciDev) {
        std::get<GpuData>(gpu->data).pciDev = std::get<PciDeviceData>(local->data);
      } else {
        assert(0);
      }
    }

    std::cerr << "get CUDA CC\n";
    int cudaMajor, cudaMinor;
    NVML(nvmlDeviceGetCudaComputeCapability(dev, &cudaMajor, &cudaMinor));
    std::get<GpuData>(gpu->data).ccMajor = cudaMajor;
    std::get<GpuData>(gpu->data).ccMinor = cudaMinor;

    if (local) {
      std::cerr << "add_gpus(): replace\n";

      graph.replace(local, gpu);

    } else {
      std::cerr << "add_gpus(): new\n";
      graph.insert_vertex(gpu);
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
      PciAddress addr = {safe_narrow<short unsigned int>(pciInfo.domain),
                         safe_narrow<unsigned char>(pciInfo.bus),
                         safe_narrow<unsigned char>(pciInfo.device), 0};
      std::cerr << "add_nvlinks(): local " << addr.str() << "\n";
      auto local = graph.get_pci(addr);
      assert(local->data.index() == Vertex::Type::Gpu);

      // figure out what's on the other side
      NVML(nvmlDeviceGetNvLinkRemotePciInfo(dev, l, &pciInfo));
      addr = {safe_narrow<short unsigned int>(pciInfo.domain),
              safe_narrow<unsigned char>(pciInfo.bus),
              safe_narrow<unsigned char>(pciInfo.device), 0};
      auto remote = graph.get_pci(addr);

      /* the NvLink Bridges on the CPUs are emulated PCI device that we did not
      add during PCI discovery just directly connect to whatever CPU is closest.
      */
      if (!remote) {
        std::cerr << "searching for closest package\n";
        auto p = graph.shortest_path(local, Vertex::is_package);
        remote = p.second;
      }

      if (!remote)
        std::cerr << "add_nvlinks(): couldn't connect nvlink to anything\n";

      unsigned int version;
      NVML(nvmlDeviceGetNvLinkVersion(dev, l, &version));

      if (remote->data.index() == Vertex::Type::Gpu) {
        std::cerr << "remote is " << remote->str() << "\n";

        // nvlink will be visible from both sides, so only connect one way
        if (std::get<GpuData>(local->data).pciDev.addr < std::get<GpuData>(remote->data).pciDev.addr) {
          auto link = Edge::new_nvlink(version, 1);
          graph.join(local, remote, link);
        }

      } else if (remote->data.index() == Vertex::Type::Ppc) {
        std::cerr << "remote is " << remote->str() << "\n";
        auto link = Edge::new_nvlink(version, 1);
        graph.join(local, remote, link);
      } else if (remote->data.index() == Vertex::Type::NvSwitch) {
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
          if (i != j && i->same_vertices(j)) {
            std::cerr << "add_nvlinks(): combining " << i->str() << " and "
                      << j->str() << "\n";
            assert(i->data_.nvlink.version == j->data_.nvlink.version);
            i->data_.nvlink.lanes += j->data_.nvlink.lanes;
            std::cerr << "add_nvlinks(): into " << i->str() << "\n";
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
  GPU-CPU NvLinks are connected to an NvLinkBridge, which is connected to the
  hostbridge we can treat these connections as infinite bandwidth when computing
  bandwidth, so this is fine for now
  */
}

} // namespace nvml
} // namespace hwgraph
