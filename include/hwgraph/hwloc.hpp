#pragma once

#include <hwloc.h>

#include "graph.hpp"

namespace hwgraph {
namespace hwloc {

void add_packages(hwgraph::Graph &graph) {
  hwloc_topology_t topology;

  hwloc_topology_init(&topology);
  hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM);
  hwloc_topology_load(topology);

  // Add packages to system

  // p266 of https://www.open-mpi.org/projects/hwloc/doc/hwloc-v2.1.0-letter.pdf
  const int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_PACKAGE);
  if (depth == HWLOC_TYPE_DEPTH_UNKNOWN) {
    assert(0 && "Can't determine number of packages");
  } else {
    unsigned numPackages = hwloc_get_nbobjs_by_depth(topology, depth);

    std::cerr << numPackages << " packages\n";

    for (unsigned i = 0; i < numPackages; ++i) {
      hwloc_obj_t obj = hwloc_get_obj_by_depth(topology, depth, i);

#ifdef __x86_64__
      Intel *p = new Intel();
#elif __PPC__
      PPC *p = new Ppc();
#endif

      // section 23.13 p. 266
      for (unsigned j = 0; j < obj->infos_count; ++j) {

        // fprintf(stderr, "%s %s %x\n", obj->infos[j].name,
        // obj->infos[j].value,
        //         std::atoi(obj->infos[j].value));

// Section 9.2 (p.37)
#ifdef __x86_64__
        if (std::string("CPUModel") == obj->infos[j].name) {
          p->model = obj->infos[j].value;
        } else if (std::string("CPUVendor") == obj->infos[j].name) {
          p->vendor = obj->infos[j].value;
        } else if (std::string("CPUModelNumber") == obj->infos[j].name) {
          p->modelNumber = std::atoi(obj->infos[j].value);
        } else if (std::string("CPUFamilyNumber") == obj->infos[j].name) {
          p->familyNumber = std::atoi(obj->infos[j].value);
        } else if (std::string("CPUStepping") == obj->infos[j].name) {
          p->stepping = std::atoi(obj->infos[j].value);
        }
#endif

#ifdef __PPC__
        if (std::string("CPUModel") == obj->infos[j].name) {
          p->model = obj->infos[j].value;
        } else if (std::string("CPURevision") == obj->infos[j].name) {
          p->revision = std::atoi(obj->infos[j].value);
        }
#endif
      }
      graph.take_vertex(p);
    }

// https://en.wikichip.org/wiki/intel/cpuid
#ifdef __PPC__

    for (auto i : graph.vertices<Ppc>()) {
      for (auto j : graph.vertices<Ppc>()) {
        if (i != j) {
          graph.join(i, j, new Xbus(64 * GiB));
        }
      }
    }

#endif

#ifdef __x86_64__
    // consider https://github.com/NVIDIA/nccl/blob/master/src/graph/topo.cc

    for (auto i : graph.vertices<Intel>()) {
      for (auto j : graph.vertices<Intel>()) {
        if (i != j) {
          if (i->familyNumber == 6 && i->modelNumber == 0x4f) {
            graph.join(i, j, new Qpi(2, 8 * Qpi::GT));
          } else {
            graph.join(i, j, new Unknown());
          }
        }
      }
    }
#endif

    hwloc_topology_destroy(topology);
  }
}
} // namespace hwloc
} // namespace hwgraph