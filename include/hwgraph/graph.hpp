#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <variant>
#include <vector>

#include "config.hpp"
#include "dot_label.hpp"
#include "edge_data.hpp"
#include "pci_address.hpp"
#include "vertex_data.hpp"

namespace hwgraph {

struct Edge;
typedef std::shared_ptr<Edge> Edge_t;

struct Vertex;
typedef std::shared_ptr<Vertex> Vertex_t;

struct Vertex {

  // enum should match index in Data, so that data.index corresponds to this
  // type
  enum Type {
    Unknown = 0,
    Bridge = 1, // PCI Bridge
    Gpu = 2,
    Intel = 3,
    NvLinkBridge = 4,
    NvSwitch = 5,
    PciDev = 6,
    Ppc = 7,
  };

  typedef std::variant<UnknownData, BridgeData, GpuData, IntelData,
                       NvLinkBridgeData, NvSwitchData, PciDeviceData, PpcData>
      Data;

  std::set<Edge_t> edges_;

  std::string name_;

  Data data;

  Vertex() {}

  static Vertex_t new_bridge(const char *name, const PciAddress &addr,
                             unsigned short dnDom, unsigned char dnSecBus,
                             unsigned char dnSubBus) {
    auto v = std::make_shared<Vertex>();

    if (name) {
      v->name_ = name;
    } else {
      v->name_ = "anonymous bridge";
    }

    BridgeData data;
    data.addr = addr;
    data.domain = {dnDom, 0, 0, 0};
    data.secondaryBus = {0, dnSecBus, 0, 0};
    data.subordinateBus = {0, dnSubBus, 0, 0};

    v->data = data;
    return v;
  }

  static Vertex_t new_pci_device(const char *name, const PciAddress &addr,
                                 float linkSpeed) {
    auto v = std::make_shared<Vertex>(Vertex::Type::PciDev);

    if (name) {
      v->name_ = name;
    } else {
      v->name_ = "anonymous pcidev";
    }
    PciDeviceData data;
    data.addr = addr;
    data.linkSpeed = linkSpeed;

    v->data = data;
    return v;
  }

  static Vertex_t new_gpu(const char *name) {
    Vertex_t v = std::make_shared<Vertex>(Vertex::Type::Gpu);
    if (name) {
      v->name_ = name;
    } else {
      v->name_ = "anonymous gpu";
    }
    return v;
  }

  static Vertex_t new_gpu(const char *name, const PciDeviceData &pciDev) {
    auto v = new_gpu(name);

    GpuData data;

    data.pciDev = pciDev;
    v->data = data;
    return v;
  }

  static Vertex_t new_nvlink_bridge(const char *name,
                                    const PciDeviceData &pciDev) {
    auto v = std::make_shared<Vertex>(Vertex::Type::NvLinkBridge);
    if (name) {
      v->name_ = name;
    } else {
      v->name_ = "anonymous nvlink bridge";
    }

    NvLinkBridgeData data;
    data.pciDev = pciDev;
    v->data = data;
    return v;
  }

  static bool is_pci_device(Vertex_t v) noexcept {
    assert(v);
    return std::holds_alternative<PciDeviceData>(v->data) ||
           std::holds_alternative<GpuData>(v->data) ||
           std::holds_alternative<NvLinkBridgeData>(v->data) ||
           std::holds_alternative<NvSwitchData>(v->data);
  }

  static bool is_package(Vertex_t v) noexcept {
    assert(v);
    return std::holds_alternative<PpcData>(v->data) ||
           std::holds_alternative<IntelData>(v->data);
  }

  std::string str() const {
    std::string s = "{";
    s += "name: " + name_;

    if (auto p = std::get_if<BridgeData>(&data)) {
      s += ", type: bridge, ";
      s += "addr=" + p->addr.str();
      s += ",dom=" + p->domain.str();
      s += ",sec=" + p->secondaryBus.str();
      s += ",sub=" + p->subordinateBus.str();
    } else if (auto p = std::get_if<PciDeviceData>(&data)) {
      s += ", type: pcidev, ";
      s += "pcidev: " + p->str();
    } else if (auto p = std::get_if<GpuData>(&data)) {
      s += ", type: gpu, ";
      s += "gpu: " + p->str();
    } else if (auto p = std::get_if<NvLinkBridgeData>(&data)) {
      s += ", type: nvlinkbridge, ";
      s += "pcidev: " + p->pciDev.str();
    } else if (auto p = std::get_if<IntelData>(&data)) {
      s += ", type: intel";
      s += p->str();
    } else if (auto p = std::get_if<PpcData>(&data)) {
      s += ", type: ppc";
      s += p->str();
    } else if (auto p = std::get_if<UnknownData>(&data)) {
      s += ", type: unknown";
    } else {
      s += ", type: <unhandled in Vertex::str()>";
    }


    s += "}";
    return s;
  }

  std::string dot_id() const { return std::to_string(uintptr_t(this)); }

  std::string dot_label() const {
    if (auto p = std::get_if<NvLinkBridgeData>(&data)) {
      return DotLabel("NvLink Bridge").str();
    } else {
      return DotLabel(name_).str();
    }
  }

  std::string dot_shape() const { return "record"; }
  std::string dot_str() const {
    std::stringstream ss;

    ss << dot_id() << " ";
    ss << "[ ";
    if (!dot_shape().empty()) {
      ss << "shape=" << dot_shape() << ",";
    }
    ss << "label=\"";
    ss << dot_label();
    ss << "\" ];";

    return ss.str();
  }
};

struct Edge {

  static const int64_t QPI_GT = 1e9;
  static const int64_t XBUS_GIB = int64_t(1) << 30;

  // should be the same order as the variant types so data.index() returns this
  // value
  enum class Type {
    Unknown = 0,
    Nvlink = 1,
    Pci = 2,
    Qpi = 3,
    Xbus = 4,
  };
  typedef std::variant<UnknownData, NvlinkData, PciData, QpiData, XbusData>
      Data;

  Vertex_t u_;
  Vertex_t v_;

  Data data;

  Edge() {}

  static Edge_t new_pci(float linkSpeed) {
    auto e = std::make_shared<Edge>();
    PciData data;
    data.linkSpeed = linkSpeed;
    data.lanes = 0;
    e->data = data;
    return e;
  }

  static Edge_t new_nvlink(unsigned int version, int64_t lanes) {
    auto e = std::make_shared<Edge>();
    NvlinkData data;
    data.version = version;
    data.lanes = lanes;
    e->data = data;
    return e;
  }

  static Edge_t new_xbus(int64_t bw) {
    auto e = std::make_shared<Edge>();
    XbusData data;
    data.bw_ = bw;
    e->data = data;
    return e;
  }

  static Edge_t make_qpi(QpiData &data) {
    auto e = std::make_shared<Edge>();
    e->data = data;
    return e;
  }

  bool has_vertex(const Vertex_t v) const noexcept {
    return (u_ == v || v_ == v);
  }

  /* retrieve the vertex on this edge that is not `v`
   */
  Vertex_t other_vertex(const Vertex_t v) const noexcept {
    if (u_ == v && v_ == v) {
      assert(0 && "self-edge?");
    } else if (u_ == v) {
      return v_;
    } else if (v_ == v) {
      return u_;
    } else {
      assert(0 && "vertex is not in edge");
      return nullptr;
    }
  }

  bool same_vertices(const Edge_t &other) const noexcept {
    assert(u_);
    assert(v_);
    assert(other);
    assert(other->u_);
    assert(other->v_);
    if (u_ == other->v_ && v_ == other->u_) {
      return true;
    } else if (u_ == other->u_ && v_ == other->v_) {
      return true;
    }
    return false;
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnoexcept-type"
  int64_t bandwidth() {
    if (auto p = std::get_if<QpiData>(&data)) {
      return p->links_ * p->speed_;
    } else if (auto p = std::get_if<XbusData>(&data)) {
      return p->bw_;
    } else if (auto p = std::get_if<PciData>(&data)) {
      return p->linkSpeed;
    } else if (auto p = std::get_if<UnknownData>(&data)) {
      assert(0 && "bandwidth() called on unknown edge");
      return -1;
    } else {
      assert(0 && "unhandled edge Type");
      return -1;
    }
  }
#pragma GCC diagnostic pop

  std::string str() const {
    std::string s = "{";

    if (auto p = std::get_if<NvlinkData>(&data)) {
      s += "type: nvlink, ";
      s += "lanes: " + std::to_string(p->lanes) + ",";
      s += "version: " + std::to_string(p->version);
    } else if (auto p = std::get_if<PciData>(&data)) {
      s += "type: pci, ";
      s += "linkSpeed: " + std::to_string(p->linkSpeed);
    } else if (auto p = std::get_if<UnknownData>(&data)) {
      s += "type: unknown";
    } else {
      s += "type: <unhandled in Edge::str()>";
    }

    s += "}";
    return s;
  }

  std::string dot_rank() const { return ""; }

  std::string dot_label() const {
    if (auto p = std::get_if<NvlinkData>(&data)) {
      return DotLabel("nvlink")
          .with_field(std::to_string(p->lanes))
          .with_field(std::to_string(p->version))
          .str();
    } else if (auto p = std::get_if<PciData>(&data)) {
      return DotLabel("pci")
          .with_field(std::to_string(p->linkSpeed))
          .str();
    } else if (auto p = std::get_if<XbusData>(&data)) {
      return DotLabel("xbus").str();
    } else if (auto p = std::get_if<UnknownData>(&data)) {
      return DotLabel("unknown").str();
    } else {
      return DotLabel("edge").str();
    }
  }

  std::string dot_attrs() const {
    std::stringstream ret;
    ret << " [ ";
    ret << "label=\"" << dot_label() << "\"";
    ret << " ];";

    return ret.str();
  }
};

struct EdgeEq {
  Edge *e_;
  EdgeEq(Edge *e) : e_(e) {}

  bool operator()(const std::shared_ptr<Edge> &up) const {
    return up.get() == e_;
  }
};

struct VertexEq {
  Vertex *v_;
  VertexEq(Vertex *v) : v_(v) {}

  bool operator()(const std::shared_ptr<Vertex> &up) const {
    return up.get() == v_;
  }
};

typedef std::vector<Edge_t> Path;

inline double path_bandwidth(const Path &p) {
  if (p.empty()) {
    return -1;
  } else {
    auto cmp_bandwidth = [](const Edge_t &a, const Edge_t &b) {
      return a->bandwidth() < b->bandwidth();
    };
    auto it = std::min_element(p.begin(), p.end(), cmp_bandwidth);
    assert(it != p.end());
    return (*it)->bandwidth();
  }
}

class Graph {

public:
  /*! Build a new graph
   */
  Graph() {}

  const std::set<Edge_t> &edges() const { return edges_; }
  std::set<Edge_t> &edges() { return edges_; }

  const std::set<Vertex_t> &vertices() const { return vertices_; }

  template <Vertex::Type T> std::set<Vertex_t> vertices() {
    std::set<Vertex_t> ret;
    for (auto &v : vertices_) {
      if (v->data.index() == T) {
        ret.insert(v);
      }
    }
    return ret;
  }

  /* take ownership of a vertex if we haven't already
   */
  Vertex_t take_vertex(Vertex *v) {
    auto it = std::find_if(vertices_.begin(), vertices_.end(), VertexEq(v));
    if (it == vertices_.end()) {
      return insert_vertex(std::shared_ptr<Vertex>(v));
    } else {
      return *it;
    }
  }

  Vertex_t insert_vertex(Vertex_t v) {
    auto p = vertices_.insert(v);
    return *(p.first);
  }

  /* take ownership of an edge if we haven't already
   */
  Edge_t take_edge(Edge *e) {
    auto it = std::find_if(edges_.begin(), edges_.end(), EdgeEq(e));
    if (it == edges_.end()) {
      return insert_edge(std::shared_ptr<Edge>(e));
    } else {
      return *it;
    }
  }

  /* take ownership of an edge if we haven't already
   */
  Edge_t insert_edge(Edge_t e) {
    auto p = edges_.insert(e);
    return *(p.first);
  }

  Edge_t join(Vertex *u, Vertex *v, Edge *e) {
    auto us = take_vertex(u);
    auto vs = take_vertex(v);
    auto es = take_edge(e);
    return join(us, vs, es);
  }

  Edge_t join(Vertex_t u, Vertex_t v, Edge *e) {
    auto us = insert_vertex(u);
    auto vs = insert_vertex(v);
    auto es = take_edge(e);
    return join(us, vs, es);
  }

  Edge_t join(Vertex_t u, Vertex_t v, Edge_t e) {
    assert(u);
    assert(v);
    assert(e);
    assert(e->u_ == nullptr && "edge is already connected");
    insert_vertex(u);
    e->u_ = u;
    assert(e->v_ == nullptr && "edge is already connected");
    insert_vertex(v);
    e->v_ = v;

    u->edges_.insert(e);
    v->edges_.insert(e);
    auto ret = insert_edge(e);
    return ret;
  }

  Edge_t erase(Edge_t e) {
    assert(edges_.count(e));
    e->u_->edges_.erase(e);
    e->v_->edges_.erase(e);
    auto it = std::find(edges_.begin(), edges_.end(), e);
    edges_.erase(it);
    return e;
  }

  Edge_t replace(Edge_t orig, Edge_t next) {
    assert(edges_.count(orig));

    // insert new edge into vertices
    orig->u_->edges_.insert(next);
    orig->v_->edges_.insert(next);

    // attach next to vertices
    next->u_ = orig->u_;
    next->v_ = orig->v_;

    // delete original edge
    auto ret = erase(orig);
    assert(0 == edges_.count(orig));
    return ret;
  }

  Vertex_t replace(Vertex_t orig, Vertex_t next) {
    std::cerr << "entry\n";
    std::cerr << orig->str() << "\n";
    std::cerr << next->str() << "\n";
    assert(orig);
    assert(next);
    assert(vertices_.count(orig));

    // replace all edges with orig to be next
    std::cerr << "replace edges\n";
    for (auto &e : orig->edges_) {
      if (e->u_ == orig) {
        e->u_ = next;
      }
      if (e->v_ == orig) {
        e->v_ = next;
      }
    }

    // copy all orig edges to next
    std::cerr << "copy edges\n";
    next->edges_ = orig->edges_;

    // add new vertex
    vertices_.insert(next);

    // delete original vertex
    auto it = std::find(vertices_.begin(), vertices_.end(), orig);
    assert(it != vertices_.end());
    auto ret = *it;
    vertices_.erase(it);

    return ret;
  }

  std::set<Vertex_t> get_vertices(std::function<bool(Vertex_t)> pred) {
    std::set<Vertex_t> ret;
    for (auto &v : vertices_) {
      if (pred(v)) {
        ret.insert(v);
      }
    }
    return ret;
  }

  Vertex_t get_package(unsigned idx) {
    for (auto v : vertices<Vertex::Type::Intel>()) {
      if (std::get<IntelData>(v->data).idx == idx) {
        return v;
      }
    }
    for (auto v : vertices<Vertex::Type::Ppc>()) {
      if (std::get<PpcData>(v->data).idx == idx) {
        return v;
      }
    }

    return nullptr;
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnoshadow"
  Vertex_t get_pci(const PciAddress &address) {
    for (const auto &v : vertices_) {
      if (auto p = std::get_if<BridgeData>(&v->data)) {
        PciAddress br = p->addr;
        if (br.domain_ == address.domain_ && br.bus_ == address.bus_) {
          return v;
        }
      } else if (auto p = std::get_if<PciDeviceData>(&v->data)) {
        if (p->addr == address) {
          return v;
        }
      } else if (auto p = std::get_if<GpuData>(&v->data)) {
        if (p->pciDev.addr == address) {
          return v;
        }
      } else if (auto p = std::get_if<NvLinkBridgeData>(&v->data)) {
        if (p->pciDev.addr == address) {
          return v;
        }
      }
    }
    return nullptr;
  }
  #pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnoexcept-type"

  Vertex_t get_bridge_for_address(const PciAddress &address) {
    for (const auto &br : vertices<Vertex::Type::Bridge>()) {
      BridgeData &data = std::get<BridgeData>(br->data);
      if (data.domain.domain_ == address.domain_ &&
          data.secondaryBus.bus_ == address.bus_) {
        return br;
      }
    }

    return nullptr;
  }

  template <typename T> void dump(const std::vector<T> v) {
    for (auto &e : v) {
      std::cerr << e->str() << " ";
    }
    std::cerr << "\n";
  }

  // dont care if c++17 mangled name changes
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnoexcept-type"
  /*
    finds the shortest path from src to a vertex for which
    UnaryPredicate(vertex) yields true
  */
  template <typename UnaryPredicate>
  std::pair<Path, Vertex_t> shortest_path(const Vertex_t src,
                                          UnaryPredicate p) {

    std::set<Edge_t> visited; // the edges we have traversed
    std::deque<Path> worklist;

    // initialize worklist
    for (auto e : src->edges_) {
      Path path = {e};
      worklist.push_front(path);
      visited.insert(e);
    }

    while (!worklist.empty()) {

      Path next = worklist.back();
      worklist.pop_back();

      Vertex_t u = next.back()->u_;
      Vertex_t v = next.back()->v_;
      if (p(u)) {
        return std::make_pair(next, u);
      } else if (p(v)) {
        return std::make_pair(next, v);
      } else {
        // make new paths for work list
        for (auto e : u->edges_) {
          Path path = next;
          if (0 == visited.count(e)) {
            path.push_back(e);
            visited.insert(e);
            worklist.push_front(path);
          }
        }
        for (auto e : v->edges_) {
          Path path = next;
          if (0 == visited.count(e)) {
            path.push_back(e);
            visited.insert(e);
            worklist.push_front(path);
          }
        }
      }
    }

    return std::make_pair(Path(), nullptr);
  }
#pragma GCC diagnostic pop

  /*
  return all paths from src to dst
*/
  std::vector<Path> paths(const Vertex_t src, const Vertex_t dst) {

    std::vector<Path> ret; // the paths from src to dst

    if (vertices_.empty()) {
      assert(edges_.empty());
      return ret;
    }

    std::set<Edge_t> visited; // the edges we have traversed
    std::deque<Path> worklist;

    // std::cerr << "init worklist " << src->edges_.size() << "\n";

    // initialize worklist
    for (auto e : src->edges_) {
      Path path = {e};
      worklist.push_front(path);
      visited.insert(e);
    }

    while (!worklist.empty()) {
      // std::cerr << "worklist.size()=" << worklist.size() << "\n";
      // std::cerr << "visited.size()=" << visited.size() << "\n";

      Path next = worklist.back();
      worklist.pop_back();

      if (next.back()->u_ == dst || next.back()->v_ == dst) {
        ret.push_back(next);
      } else {
        // make new paths for work list
        Vertex_t u = next.back()->u_;
        for (auto e : u->edges_) {
          Path path = next;
          if (0 == visited.count(e)) {
            path.push_back(e);
            visited.insert(e);
            worklist.push_back(path);
          }
        }

        Vertex_t v = next.back()->v_;
        for (auto e : v->edges_) {
          Path path = next;
          if (0 == visited.count(e)) {
            path.push_back(e);
            visited.insert(e);
            worklist.push_back(path);
          }
        }
      }
    }

    return ret;
  }

  // return the path from src to dst that has the minimum cost.
  // if no path is found, return an empty path
  Path min_path(const Vertex_t src, const Vertex_t dst,
                std::function<float(const Path &)> cost) {
    std::vector<Path> allPaths = paths(src, dst);
    if (allPaths.empty()) {
      return Path();
    }
    auto path_cmp = [&](const Path &a, const Path &b) {
      return cost(a) < cost(b);
    };
    auto it = std::min_element(allPaths.begin(), allPaths.end(), path_cmp);
    assert(it != allPaths.end());
    return *it;
  }

  // return the path from src to dst that has the max cost.
  // if no path is found, return an empty path
  Path max_path(const Vertex_t src, const Vertex_t dst,
                std::function<float(const Path &)> cost) {
    std::vector<Path> allPaths = paths(src, dst);
    if (allPaths.empty()) {
      return Path();
    }
    auto path_cmp = [&](const Path &a, const Path &b) {
      return cost(a) < cost(b);
    };
    auto it = std::max_element(allPaths.begin(), allPaths.end(), path_cmp);
    assert(it != allPaths.end());
    return *it;
  }

  std::string dot_str() const {
    auto dot_header = [&]() {
      std::string ret;
      ret += "graph {\n";
      return ret;
    };
    auto dot_footer = [&]() {
      std::string ret;
      ret += "}";
      return ret;
    };

    auto dot_nodes = [&]() {
      std::string ret;
      for (const auto &v : vertices_) {
        ret += v->dot_str() + "\n";
      }
      return ret;
    };

    auto dot_edges = [&]() {
      std::string ret;
      for (const auto &e : edges_) {
        ret += e->u_->dot_id() + " -- " + e->v_->dot_id() + " ";
        ret += e->dot_attrs() + "\n";
        if (!e->dot_rank().empty()) {
          ret += "{rank=" + e->dot_rank() + "; " + e->u_->dot_id() + "; " +
                 e->v_->dot_id() + "};\n";
        }
      }
      return ret;
    };

    std::string ret;
    ret += dot_header();
    ret += dot_nodes();
    ret += dot_edges();
    ret += dot_footer();
    return ret;
  }

private:
  std::set<std::shared_ptr<Vertex>> vertices_;
  std::set<std::shared_ptr<Edge>> edges_;
}; // namespace nvml

} // namespace hwgraph
