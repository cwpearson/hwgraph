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

      Vertex *v = new Vertex();

#ifdef __x86_64__
      v->type_ = Vertex::Type::Intel;
#elif __PPC__
      v->type_ = Vertex::Type::Ppc;
#endif

      // section 23.13 p. 266
      for (unsigned j = 0; j < obj->infos_count; ++j) {

        // fprintf(stderr, "%s %s %x\n", obj->infos[j].name,
        // obj->infos[j].value,
        //         std::atoi(obj->infos[j].value));

// Section 9.2 (p.37)
#ifdef __x86_64__
        if (std::string("CPUModel") == obj->infos[j].name) {
          std::strncpy(v->data_.intel_.model, obj->infos[j].value,
                       Vertex::MAX_STR);
        } else if (std::string("CPUVendor") == obj->infos[j].name) {
          std::strncpy(v->data_.intel_.vendor, obj->infos[j].value,
                       Vertex::MAX_STR);
        } else if (std::string("CPUModelNumber") == obj->infos[j].name) {
          v->data_.intel_.modelNumber = std::atoi(obj->infos[j].value);
        } else if (std::string("CPUFamilyNumber") == obj->infos[j].name) {
          v->data_.intel_.familyNumber = std::atoi(obj->infos[j].value);
        } else if (std::string("CPUStepping") == obj->infos[j].name) {
          v->data_.intel_.stepping = std::atoi(obj->infos[j].value);
        }
#endif

#ifdef __PPC__
        if (std::string("CPUModel") == obj->infos[j].name) {
          std::strncpy(v->data_.ppc_.model, = obj->infos[j].value,
                       Vertex::MAX_STR);
        } else if (std::string("CPURevision") == obj->infos[j].name) {
          v->data_.ppc_.revision = std::atoi(obj->infos[j].value);
        }
#endif
      }
      graph.take_vertex(v);
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

    for (auto i : graph.vertices<Vertex::Type::Intel>()) {
      for (auto j : graph.vertices<Vertex::Type::Intel>()) {
        if (i != j) {
          if (i->data_.intel_.familyNumber == 6 && i->data_.intel_.modelNumber == 0x4f) {
            Edge *edge = new Edge(Edge::Type::Qpi);
            edge->data_.qpi_.links_ = 2;
            edge->data_.qpi_.speed_ = 8 * Edge::QPI_GT;
            graph.join(i, j, edge);
          } else {
            Edge *edge = new Edge(Edge::Type::Unknown);
            graph.join(i, j, edge);
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