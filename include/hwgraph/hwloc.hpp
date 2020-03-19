#pragma once

#include <hwloc.h>

#include "graph.hpp"

namespace hwgraph {
namespace hwloc {

inline void add_packages(hwgraph::Graph &graph) {
  hwloc_topology_t topology;

  hwloc_topology_init(&topology);
  hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM);
  //hwloc_topology_set_all_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_ALL);
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
      v->data_.intel_.idx = i;
#elif __PPC__
      v->type_ = Vertex::Type::Ppc;
      v->data_.ppc_.idx = i;
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
          std::strncpy(v->data_.ppc_.model, obj->infos[j].value,
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

    for (auto i : graph.vertices<Vertex::Type::Ppc>()) {
      for (auto j : graph.vertices<Vertex::Type::Ppc>()) {
        if (i != j) {
          graph.join(i, j, Edge::new_xbus(64 * Edge::XBUS_GIB));
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


inline bool is_hostbridge(const hwloc_obj_t obj) {
  auto result = hwloc_compare_types(obj->type, HWLOC_OBJ_BRIDGE);
  assert(result != HWLOC_TYPE_UNORDERED);
  if (result == 0) { // is a bridge
    auto upstream = obj->attr->bridge.upstream_type;
    auto downstream = obj->attr->bridge.downstream_type;
    return (upstream == HWLOC_OBJ_BRIDGE_HOST) &&
           (downstream == HWLOC_OBJ_BRIDGE_PCI); // is a host->pci bridge
  } else {
    return false;
  }
}

inline bool is_pcibridge(const hwloc_obj_t obj) {
  auto result = hwloc_compare_types(obj->type, HWLOC_OBJ_BRIDGE);
  assert(result != HWLOC_TYPE_UNORDERED);
  if (result == 0) { // is a bridge
    auto upstream = obj->attr->bridge.upstream_type;
    return (upstream == HWLOC_OBJ_BRIDGE_PCI); // is a pci->something bridge
  } else {
    return false;
  }
}

inline void visit_pci_bridge(hwgraph::Graph &graph, const hwloc_obj_t obj) {
  // Insert this object
  const auto up_pci = obj->attr->bridge.upstream.pci;
  const auto dn_pci = obj->attr->bridge.downstream.pci;
  PciAddress bridgeAddress = {up_pci.domain, up_pci.bus, up_pci.dev, up_pci.func};
  auto bridge = Vertex::new_bridge(bridgeAddress,
                                            dn_pci.domain, dn_pci.secondary_bus,
                                            dn_pci.subordinate_bus);
  graph.insert_vertex(bridge);
  std::cerr << "visit_pci_bridge(): Added " << bridge->str() << "\n";

  const auto &parent = graph.get_bridge_for_address(bridgeAddress);
  std::cerr << "visit_pci_bridge(): Parent bridge is " << parent->str() << "\n";

  // Link to parent
  auto link = Edge::new_pci(up_pci.linkspeed);
  std::cerr << "Link " << parent << " and " << bridge << " @ " << up_pci.linkspeed << "\n";
  graph.join(parent, bridge, link);
}


inline void visit_pci_device(hwgraph::Graph &graph, const hwloc_obj_t obj) {

  // get name
  char *name;
  if (obj->name) {
    name = obj->name;
  } else {
    name = "anon_pci_dev";
  }

  // get address
  const auto pci = obj->attr->pcidev;
  PciAddress objAddress = {pci.domain, pci.bus, pci.dev, pci.func};

  auto newDevice = Vertex_t(nullptr);

  // Try to use OS Device children to create a more refined PCI device
  // https://github.com/cwpearson/hwcomm/blob/8b7213da5addbf7852ce72c1a09ec489d14d4419/src/backend_hwloc.cpp#L79
  if (!newDevice) {
    newDevice = Vertex::new_pci_device(name, objAddress, pci.linkspeed);
  }

  // Add device
  graph.insert_vertex(newDevice);
  std::cerr << "visit_pci_device(): Added " << newDevice->str() << "\n";

  // Link to parent
  const auto parent = obj->parent;
  assert(parent);
  assert(0 == hwloc_compare_types(parent->type, HWLOC_OBJ_BRIDGE) &&
         "Pci parent should be a bridge");

  const auto &parentBridge = graph.get_bridge_for_address(objAddress);
  std::cerr << "visit_pci_device(): Parent is " << parentBridge->str() << "\n";
  auto link = Edge::new_pci(pci.linkspeed);

  std::cerr << "visit_pci_device(): speed = " << link->data_.pci_.linkSpeed_ << "\n";

  graph.join(parentBridge, newDevice, link);
}

inline void descend_pci_tree(hwloc_topology_t topology, hwgraph::Graph &graph,
                             hwloc_obj_t obj, std::set<hwloc_obj_t> &visited,
                             int depth = 0) {
  // may enter the same tree at different points, so skip objects we've seen
  if (visited.count(obj)) {
    return;
  }

  if (is_hostbridge(obj)) {

    std::cerr << "descend_pci_tree(): hostbridge!\n";

    // Add us to the machine
    // https://www.open-mpi.org/projects/hwloc/doc/hwloc-v1.11.7-letter.pdf#section.20.2
    const auto up_pci = obj->attr->bridge.upstream.pci;
    const auto dn_pci = obj->attr->bridge.downstream.pci;
    const PciAddress bridgeAddr = {up_pci.domain, up_pci.bus, up_pci.dev,
                                   up_pci.func};
    auto hub = Vertex::new_bridge(bridgeAddr, dn_pci.domain,
                                           dn_pci.secondary_bus,
                                           dn_pci.subordinate_bus);

    std::cerr << "descend_pci_tree(): " << hub->str() << "\n";
      
    graph.insert_vertex(hub);

    for (unsigned i = 0; i < obj->infos_count; ++i) {
      std::cerr << obj->infos[i].name << "::" << obj->infos[i].value;
    }

    // Find the NUMA node this package lives in
    auto nonIoAncestor = hwloc_get_non_io_ancestor_obj(topology, obj);
    assert(nonIoAncestor);

    // find the upstream packages that occuy the same numa nodes
    const int numPackages =
        hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PACKAGE);
    assert(numPackages);
    std::cerr << "descend_pci_tree(): Searching " << numPackages
                           << " packages for corresponding NUMA nodes\n";
    for (int pkgIdx = 0; pkgIdx < numPackages; ++pkgIdx) {
      hwloc_obj_t upstreamObj =
          hwloc_get_obj_by_type(topology, HWLOC_OBJ_PACKAGE, pkgIdx);
      assert(upstreamObj);
      const auto &upstreamCompute = graph.get_package(pkgIdx);

      // nonio included in pkg nodeset
      if (hwloc_bitmap_isincluded(nonIoAncestor->nodeset,
                                  upstreamObj->nodeset)) {
        // Link between this host bridge and the upstream soc

        std::cerr << "descend_pci_tree(): linking with package " << pkgIdx << " @ " << up_pci.linkspeed << "\n";

        auto link = Edge::new_pci(up_pci.linkspeed);
        graph.join(upstreamCompute, hub, link);
        break;
      }
    }


  } else if (is_pcibridge(obj)) {
    std::cerr << "descend_pci_tree(): PCI bridge!\n";
    visit_pci_bridge(graph, obj);
  } else if (obj->type == HWLOC_OBJ_PCI_DEVICE) {
    std::cerr << "descend_pci_tree(): PCI device!\n";
    visit_pci_device(graph, obj);
  } else if (obj->type == HWLOC_OBJ_GROUP) {
    std::cerr << "Group device " << obj->name << "\n";
  } else if (obj->type == HWLOC_OBJ_MISC) {
    std::cerr << "MISC device " << obj->name << "\n";
  } else if (obj->type == HWLOC_OBJ_OS_DEVICE) {
    std::cerr << "OS Device " << obj->name << "\n";
    //visit_os_device(graph, obj, visited);
  } else {
    std::cerr << "Other kind of device " << obj->name << "\n";
    for (unsigned i = 0; i < obj->infos_count; ++i) {
	    std::cerr << obj->infos[i].name << "::" << obj->infos[i].value << "\n";
    }
    assert(obj->type != HWLOC_OBJ_BRIDGE); // should have caught these
    assert(0 && "How did we get here?");
  }


  visited.insert(obj);
  for (unsigned i = 0; i < obj->arity; ++i) {
    auto child = obj->children[i];
    descend_pci_tree(topology, graph, child, visited, depth + 1);
  }
}


inline void add_pci(hwgraph::Graph &graph) {
    hwloc_topology_t topology;

  hwloc_topology_init(&topology);
  hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM |
                                         HWLOC_TOPOLOGY_FLAG_IO_BRIDGES |
					 HWLOC_TOPOLOGY_FLAG_WHOLE_IO);
  hwloc_topology_load(topology);

  // Descend down into each PCI bridge
  const int bridgeDepth = hwloc_get_type_depth(topology, HWLOC_OBJ_BRIDGE);
  if (HWLOC_TYPE_DEPTH_UNKNOWN == bridgeDepth) {
    std::cerr << "add_pci(): Couldn't find any PCI bridges\n";
  } else if (HWLOC_TYPE_DEPTH_MULTIPLE == bridgeDepth) {
    std::cerr << "add_pci(): Bridges exist at multiple depths\n";
    assert(0);
  } else {
    const int numBridges = hwloc_get_nbobjs_by_depth(topology, bridgeDepth);
    std::cerr << "add_pci(): Found " << numBridges << " bridges\n";
    std::set<hwloc_obj_t> visited;
    for (int i = 0; i < numBridges; ++i) {
      auto bridge = hwloc_get_obj_by_depth(topology, bridgeDepth, i);
      descend_pci_tree(topology, graph, bridge, visited);
    }
  }

  hwloc_topology_destroy(topology);
}

} // namespace hwloc
} // namespace hwgraph
