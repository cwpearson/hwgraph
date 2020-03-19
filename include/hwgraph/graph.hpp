#pragma once

#include <cstring>
#include <deque>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

namespace hwgraph {

struct PciAddress {
  typedef unsigned short domain_type;
  typedef unsigned char bus_type;
  typedef unsigned char dev_type;
  typedef unsigned char func_type;

  domain_type domain_;
  bus_type bus_;
  dev_type dev_;
  func_type func_;

  std::string domain_str() const {
    std::stringstream ret;
    ret << std::setfill('0') << std::setw(4) << std::hex << (size_t)domain_;
    return ret.str();
  }
  std::string bus_str() const {
    std::stringstream ret;
    ret << std::setfill('0') << std::setw(2) << std::hex << (size_t)bus_;
    return ret.str();
  }
  std::string dev_str() const {
    std::stringstream ret;
    ret << std::setfill('0') << std::setw(2) << std::hex << (size_t)dev_;
    return ret.str();
  }
  std::string func_str() const {
    std::stringstream ret;
    ret << std::setfill('0') << std::setw(1) << std::hex << (size_t)func_;
    return ret.str();
  }

  std::string str() const {
    std::stringstream ret;
    ret << domain_str() << ":" << bus_str() << ":" << dev_str() << "."
        << func_str();
    return ret.str();
  }
};

struct Edge;
typedef std::shared_ptr<Edge> Edge_t;

struct Vertex;
typedef std::shared_ptr<Vertex> Vertex_t;

struct Vertex {

  static constexpr size_t MAX_STR = 64;

  enum class Type {
    Unknown,
    Gpu,
    Ppc,
    Intel,
    Bridge, // PCI host bridge
    PciDev,
  } type_;

  std::set<Edge_t> edges_;

  union Data {
    struct GpuData {
    } gpu_;
    struct PpcData {
      unsigned idx; // hwloc index
      char model[MAX_STR];
      int revision;
    } ppc_;
    struct IntelData {
      unsigned idx; // hwloc index
      char model[MAX_STR];
      char vendor[MAX_STR];
      int modelNumber;
      int familyNumber;
      int stepping;
    } intel_;
    struct BridgeData {
      PciAddress addr;
      PciAddress domain;
      PciAddress secondaryBus;
      PciAddress subordinateBus;
    } bridge_;
    struct PciDevData {
      char name[MAX_STR];
      PciAddress addr;
      float linkSpeed;
    } pciDev_;
  } data_;

  Vertex(Type type) : type_(type) { std::memset(&data_, 0, sizeof(data_)); }
  Vertex() : Vertex(Type::Unknown) {}

  static Vertex_t new_bridge(const PciAddress &addr, unsigned short dnDom,
                             unsigned char dnSecBus, unsigned char dnSubBus) {
    auto v = std::make_shared<Vertex>(Vertex::Type::Bridge);
    v->data_.bridge_.addr = addr;
    v->data_.bridge_.domain = {dnDom, 0, 0, 0};
    v->data_.bridge_.secondaryBus = {0, dnSecBus, 0, 0};
    v->data_.bridge_.subordinateBus = {0, dnSubBus, 0, 0};
    return v;
  }

  static Vertex_t new_pci_device(const char *name, const PciAddress &addr, float linkSpeed) {
    auto v = std::make_shared<Vertex>(Vertex::Type::PciDev);
    std::strncpy(v->data_.pciDev_.name, name, MAX_STR);
    v->data_.pciDev_.addr = addr;
    v->data_.pciDev_.linkSpeed = linkSpeed;
    return v;
  }

  std::string str() const {
    switch (type_) {
    case Type::Bridge: {
      std::string s = "bridge ";
      s += data_.bridge_.addr.str();
      return s;
    }
    case Type::PciDev: {
      std::string s = data_.pciDev_.name;
      s += " @ " + data_.pciDev_.addr.str();
      return s;
    }
    default:
      return "vertex";
    }
  }
};

struct Edge {

  static const int64_t QPI_GT = 1e9;

  enum class Type {
    Unknown,
    Qpi,
    Xbus,
    Pci,
  } type_;

  Vertex_t u_;
  Vertex_t v_;
  union Data {
    struct QpiData {
      int64_t links_;
      int64_t speed_;
    } qpi_;
    struct XbusData {
      int64_t bw_;
    } xbus_;
    struct PciData {
      float linkSpeed_;
    } pci_;
  } data_;

  Edge(Type type) : type_(type), u_(nullptr), v_(nullptr) {
    std::memset(&data_, 0, sizeof(data_));
  }
  Edge() : Edge(Type::Unknown) {}

  static Edge_t new_pci(float linkSpeed) {
    auto e = std::make_shared<Edge>(Edge::Type::Pci);
    e->data_.pci_.linkSpeed_ = linkSpeed;
    return e;
  }

  int64_t bandwidth() {
    switch (type_) {
    case Type::Qpi:
      return data_.qpi_.links_ * data_.qpi_.speed_;
    case Type::Xbus:
      return data_.xbus_.bw_;
    case Type::Pci:
      return data_.pci_.linkSpeed_;
    case Type::Unknown:
      assert(0 && "bandwidth() called on unknown edge");
      return -1;
    default:
      assert(0 && "unexpected edge Type");
      return -1;
    }
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

class Graph {

public:
  typedef std::vector<Edge_t> Path;

  /*! Build a new graph
   */
  Graph() {}

  const std::set<Edge_t> &edges() const { return edges_; }
  std::set<Edge_t> &edges() { return edges_; }

  const std::set<Vertex_t> &vertices() const { return vertices_; }

  template <Vertex::Type T> std::set<Vertex_t> vertices() {
    std::set<Vertex_t> ret;
    for (auto &v : vertices_) {
      if (v->type_ == T) {
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

  std::shared_ptr<Edge> replace(Edge_t orig, Edge_t next) {
    // replace vertices edges with next
    orig->u_->edges_.erase(orig);
    orig->u_->edges_.insert(next);
    orig->v_->edges_.erase(orig);
    orig->v_->edges_.insert(next);

    // attach next to vertices
    next->u_ = orig->u_;
    next->v_ = orig->v_;

    // delete original edge
    auto it = std::find(edges_.begin(), edges_.end(), orig);
    auto ret = *it;
    edges_.erase(it);
    return ret;
  }

  Vertex_t get_package(unsigned idx) {
    for (auto v : vertices<Vertex::Type::Intel>()) {
      if (v->data_.intel_.idx == idx) {
        return v;
      }
    }
    for (auto v : vertices<Vertex::Type::Ppc>()) {
      if (v->data_.ppc_.idx == idx) {
        return v;
      }
    }

    return nullptr;
  }

Vertex_t get_bridge_for_address(const PciAddress &address) {
  for (const auto &br : vertices<Vertex::Type::Bridge>()) {
    if (br->data_.bridge_.domain.domain_ == address.domain_ &&
        br->data_.bridge_.secondaryBus.bus_ == address.bus_) {
      return br;
    }
  }

  return nullptr;
}

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

private:
  std::set<std::shared_ptr<Vertex>> vertices_;
  std::set<std::shared_ptr<Edge>> edges_;
};

} // namespace hwgraph
