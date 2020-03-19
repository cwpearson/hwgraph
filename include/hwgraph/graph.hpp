#pragma once

#include <deque>
#include <iostream>
#include <memory>
#include <set>
#include <vector>
#include  <cstring>


namespace hwgraph {

struct Edge;

typedef std::shared_ptr<Edge> Edge_t;

struct Vertex {

  static constexpr size_t MAX_STR = 32;

  enum class Type {
    Gpu,
    Ppc,
    Intel,
  } type_;

  std::set<Edge_t> edges_;

  union Data {
    struct GpuData {
    } gpu_;
    struct PpcData {
      char model[MAX_STR];
      int revision;
    } ppc_;
    struct IntelData {
      char model[MAX_STR];
      char vendor[MAX_STR];
      int modelNumber;
      int familyNumber;
      int stepping;
    } intel_;
  } data_;
};

typedef std::shared_ptr<Vertex> Vertex_t;

struct Edge {

  static const int64_t QPI_GT = 1e9;

  enum class Type {
    Qpi,
    Xbus,
    Unknown,
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
  } data_;

  
  Edge(Type type) : type_(type), u_(nullptr), v_(nullptr) {}
  Edge() : Edge(Type::Unknown) {}

  int64_t bandwidth() {
    switch (type_) {
    case Type::Qpi:
      return data_.qpi_.links_ * data_.qpi_.speed_;
    case Type::Xbus:
      return data_.xbus_.bw_;
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

  const std::set<Edge_t> &edges() const {
    return edges_;
  }
  std::set<Edge_t> &edges() {
    return edges_;
  }

  const std::set<Vertex_t> &vertices() const {
    return vertices_;
  }

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
    }
    else {
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
    }
    else {
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
