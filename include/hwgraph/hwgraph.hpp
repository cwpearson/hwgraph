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
  Nvml = 1,
  Hwloc = 2,
};
static_assert(sizeof(DiscoveryMethod) == sizeof(int), "int");

inline DiscoveryMethod operator|(DiscoveryMethod a, DiscoveryMethod b) {
  return static_cast<DiscoveryMethod>(static_cast<int>(a) |
                                      static_cast<int>(b));
}

inline DiscoveryMethod &operator|=(DiscoveryMethod &a, DiscoveryMethod b) {
  a = a | b;
  return a;
}

inline DiscoveryMethod operator&(DiscoveryMethod a, DiscoveryMethod b) {
  return static_cast<DiscoveryMethod>(static_cast<int>(a) &
                                      static_cast<int>(b));
}

inline bool operator&&(DiscoveryMethod a, DiscoveryMethod b) {
  return (a & b) != DiscoveryMethod::None;
}

/* Return available methods (to be passed to make_graph)
*/
DiscoveryMethod available_methods() {
  DiscoveryMethod ret = DiscoveryMethod::None;
#if HWGRAPH_USE_NVML == 1
  ret |= DiscoveryMethod::Nvml;
#endif
#if HWGRAPH_USE_HWLOC == 1
  ret |= DiscoveryMethod::Hwloc;
#endif
  return ret;
}

/* Use the provided methods to build a hardware graph
*/
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

} // namespace hwgraph
