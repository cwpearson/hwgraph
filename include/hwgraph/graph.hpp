#pragma once

#include <cstring>
#include <deque>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

#include "dot_label.hpp"
#include "pci_address.hpp"
#include "vertex_data.hpp"

namespace hwgraph {

struct Edge;
typedef std::shared_ptr<Edge> Edge_t;

struct Vertex;
typedef std::shared_ptr<Vertex> Vertex_t;

struct Vertex {

  static constexpr size_t MAX_STR = 64;

  enum class Type {
    Unknown,
    Ppc,
    Intel,
    Bridge, // PCI host bridge
    PciDev,
    Gpu,
    NvLinkBridge,
    NvSwitch,
  } type_;

  std::set<Edge_t> edges_;

  std::string name_;

  union Data {
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
    } intel;
    struct BridgeData {
      PciAddress addr;
      PciAddress domain;
      PciAddress secondaryBus;
      PciAddress subordinateBus;
    } bridge_;
    PciDeviceData pciDev;
    GpuData gpu;
    NvLinkBridgeData nvLinkBridge;
    NvSwitchData nvSwitch;
  } data_;

  Vertex(Type type) : type_(type) { std::memset(&data_, 0, sizeof(data_)); }
  Vertex() : Vertex(Type::Unknown) {}

  static Vertex_t new_bridge(const char *name, const PciAddress &addr,
                             unsigned short dnDom, unsigned char dnSecBus,
                             unsigned char dnSubBus) {
    auto v = std::make_shared<Vertex>(Vertex::Type::Bridge);
    if (name) {
      v->name_ = name;
    } else {
      v->name_ = "anonymous bridge";
    }

    v->data_.bridge_.addr = addr;
    v->data_.bridge_.domain = {dnDom, 0, 0, 0};
    v->data_.bridge_.secondaryBus = {0, dnSecBus, 0, 0};
    v->data_.bridge_.subordinateBus = {0, dnSubBus, 0, 0};
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
    v->data_.pciDev.addr = addr;
    v->data_.pciDev.linkSpeed = linkSpeed;
    return v;
  }

  static Vertex_t new_gpu(const char *name, const PciDeviceData &pciDev) {
    auto v = std::make_shared<Vertex>(Vertex::Type::Gpu);
    if (name) {
      v->name_ = name;
    } else {
      v->name_ = "anonymous gpu";
    }
    v->data_.gpu.pciDev = pciDev;
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
    v->data_.nvLinkBridge.pciDev = pciDev;
    return v;
  }

  bool is_pci_device() const noexcept {
    return type_ == Type::PciDev || type_ == Type::Gpu ||
           type_ == Type::NvLinkBridge || type_ == Type::NvSwitch;
  }

  std::string str() const {
    std::string s = "{";
    s += "name: " + name_;

    switch (type_) {
    case Type::Bridge: {
      s += ", type: bridge, ";
      s += "addr=" + data_.bridge_.addr.str();
      s += ",dom=" + data_.bridge_.domain.str();
      s += ",sec=" + data_.bridge_.secondaryBus.str();
      s += ",sub=" + data_.bridge_.subordinateBus.str();
      break;
    }
    case Type::PciDev:
      s += ", type: pcidev, ";
      s += "pcidev: " + data_.pciDev.str();
      break;
    case Type::Gpu:
      s += ", type: gpu, ";
      s += "pcidev: " + data_.gpu.pciDev.str();
      break;
    case Type::NvLinkBridge:
      s += ", type: nvlinkbridge, ";
      s += "pcidev: " + data_.pciDev.str();
      break;
    case Type::Unknown: {
      s += ", type: unknown";
      break;
    }
    default:
      s += ", type: <unhandled in Vertex::str()>";
      break;
    }

    s += "}";
    return s;
  }

  std::string dot_id() const { return std::to_string(uintptr_t(this)); }

  std::string dot_label() const {
    switch (type_) {
    case Type::NvLinkBridge:
      return DotLabel("NvLink Bridge").str();
    default:
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

  enum class Type {
    Unknown,
    Qpi,
    Xbus,
    Pci,
    Nvlink,
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
      float linkSpeed;
    } pci;
    struct NvlinkData {
      unsigned int version;
      int64_t lanes;
    } nvlink;
  } data_;

  Edge(Type type) : type_(type), u_(nullptr), v_(nullptr) {
    std::memset(&data_, 0, sizeof(data_));
  }
  Edge() : Edge(Type::Unknown) {}

  static Edge_t new_pci(float linkSpeed) {
    auto e = std::make_shared<Edge>(Edge::Type::Pci);
    e->data_.pci.linkSpeed = linkSpeed;
    return e;
  }

  static Edge_t new_nvlink(unsigned int version, int64_t lanes) {
    auto e = std::make_shared<Edge>(Edge::Type::Nvlink);
    e->data_.nvlink.version = version;
    e->data_.nvlink.lanes = lanes;
    return e;
  }

  static Edge_t new_xbus(int64_t bw) {
    auto e = std::make_shared<Edge>(Edge::Type::Xbus);
    e->data_.xbus_.bw_ = bw;
    return e;
  }

  Vertex_t other_vertex(const Vertex_t v) const noexcept {
    if (u_ == v) {
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

  int64_t bandwidth() {
    switch (type_) {
    case Type::Qpi:
      return data_.qpi_.links_ * data_.qpi_.speed_;
    case Type::Xbus:
      return data_.xbus_.bw_;
    case Type::Pci:
      return data_.pci.linkSpeed;
    case Type::Unknown:
      assert(0 && "bandwidth() called on unknown edge");
      return -1;
    default:
      assert(0 && "unhandled edge Type");
      return -1;
    }
  }

  std::string str() const {
    std::string s = "{";

    switch (type_) {
    case Type::Nvlink: {
      s += "type: nvlink, ";
      s += "lanes: " + std::to_string(data_.nvlink.lanes) + ",";
      s += "version: " + std::to_string(data_.nvlink.version);
      break;
    }
    default:
      s += "type: <unhandled in Edge::str()>";
      break;
    }

    s += "}";
    return s;
  }

  std::string dot_rank() const { return ""; }

  std::string dot_label() const {
    switch (type_) {
    case Type::Nvlink:
      return DotLabel("nvlink")
          .with_field(std::to_string(data_.nvlink.lanes))
          .with_field(std::to_string(data_.nvlink.version))
          .str();
    case Type::Pci:
      return DotLabel("pci")
          .with_field(std::to_string(data_.pci.linkSpeed))
          .str();
    case Type::Xbus:
      return DotLabel("xbus").str();
    default:
      return "edge";
    }

    return "edge";
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
    return erase(orig);
  }

  Vertex_t replace(Vertex_t orig, Vertex_t next) {
    assert(vertices_.count(orig));

    // replace all edge terminators
    for (auto &e : orig->edges_) {
      if (e->u_ == orig) {
        e->u_ = next;
      }
      if (e->v_ == orig) {
        e->v_ = next;
      }
    }

    // add new vertex
    vertices_.insert(next);

    // delete original vertex
    auto it = std::find(vertices_.begin(), vertices_.end(), orig);
    auto ret = *it;
    vertices_.erase(it);

    return ret;
  }

  Vertex_t get_package(unsigned idx) {
    for (auto v : vertices<Vertex::Type::Intel>()) {
      if (v->data_.intel.idx == idx) {
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

  Vertex_t get_pci(const PciAddress &address) {
    for (const auto &v : vertices_) {
      if (v->type_ == Vertex::Type::Bridge) {
        PciAddress br = v->data_.bridge_.addr;
        if (br.domain_ == address.domain_ && br.bus_ == address.bus_) {
          return v;
        }

      } else if (v->type_ == Vertex::Type::PciDev) {
        if (v->data_.pciDev.addr == address) {
          return v;
        }
      } else if (v->type_ == Vertex::Type::Gpu) {
        if (v->data_.gpu.pciDev.addr == address) {
          return v;
        }
      } else if (v->type_ == Vertex::Type::NvLinkBridge) {
        if (v->data_.nvLinkBridge.pciDev.addr == address) {
          return v;
        }
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
