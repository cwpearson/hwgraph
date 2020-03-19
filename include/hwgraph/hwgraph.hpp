#pragma once

#include "graph.hpp"
#include "hwloc.hpp"
#if HWGRAPH_USE_NVML == 1
#include "nvml.hpp"
#endif

namespace hwgraph {

/* Methods usable to discover topology
 */
enum class DiscoveryMethod {
  None = 0,
#if HWGRAPH_USE_NVML == 1
  Nvml = 1,
#endif
  Hwloc = 2,
};
static_assert(sizeof(DiscoveryMethod) == sizeof(int), "int");

inline DiscoveryMethod operator|(DiscoveryMethod a, DiscoveryMethod b) {
  return static_cast<DiscoveryMethod>(static_cast<int>(a) | static_cast<int>(b));
}

inline DiscoveryMethod &operator|=(DiscoveryMethod &a, DiscoveryMethod b) {
  a = a | b;
  return a;
}

inline DiscoveryMethod operator&(DiscoveryMethod a, DiscoveryMethod b) {
  return static_cast<DiscoveryMethod>(static_cast<int>(a) & static_cast<int>(b));
}

inline bool operator&&(DiscoveryMethod a, DiscoveryMethod b) {
  return (a & b) != DiscoveryMethod::None;
}

Graph make_graph(const DiscoveryMethod &method) {
    Graph g;

    if (method && DiscoveryMethod::Hwloc) {
        hwloc::add_packages(g);
    }
    if (method && DiscoveryMethod::Hwloc) {
        hwloc::add_pci(g);
    }

#if HWGRAPH_USE_NVML == 1
    if (method && DiscoveryMethod::Nvml) {
        nvml::add_gpus(g);
        nvml::add_nvlinks(g);
    }
#endif

    return g;
}

}
