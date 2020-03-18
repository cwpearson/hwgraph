#pragma once

#include <deque>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

namespace hwgraph {

class Edge;

class Vertex {
public:
  std::set<Edge *> edges_;
  virtual ~Vertex() = default;
};

class Edge {
public:
  Vertex *u_;
  Vertex *v_;
  Edge() : u_(nullptr), v_(nullptr) {}
  virtual ~Edge() = default;
};

class Interconnect : public Edge {
public:
  virtual int64_t bandwidth() const = 0;
};

class Xbus : public Interconnect {
public:
  int64_t bw_;
  Xbus(int64_t bw) : bw_(bw) {}

  int64_t bandwidth() const override { return bw_; }
};

class Qpi : public Interconnect {
  public:
  static constexpr int64_t GT = 1e9;

  int64_t links_;
  int64_t speed_;

  Qpi(int links, int64_t speed) : links_(links), speed_(speed) {}

  int64_t bandwidth() const override { return links_ * speed_; }

};

class Unknown : public Interconnect {
  public:
int64_t bandwidth() const override { return -1; }
};

class Gpu : public Vertex {};

class Package : public Vertex {};

class Intel : public Package {
public:
  std::string model;
  std::string vendor;
  int modelNumber;
  int familyNumber;
  int stepping;
};

class Ppc : public Package {
public:
  std::string model;
  int revision;
};

struct EdgeCmp {
  Edge *e_;
  EdgeCmp(Edge *e) : e_(e) {}

  bool operator()(const std::unique_ptr<Edge> &up) const {
    return up.get() == e_;
  }
};

struct VertexCmp {
  Vertex *v_;
  VertexCmp(Vertex *v) : v_(v) {}

  bool operator()(const std::unique_ptr<Vertex> &up) const {
    return up.get() == v_;
  }
};

class Graph {

public:
  typedef std::vector<const Edge *> Path;

  /*! Build a new graph
   */
  Graph() {}

  std::set<const Edge *> edges() const {
    std::set<const Edge *> ret;
    for (auto &e : edges_) {
      ret.insert(e.get());
    }
    return ret;
  }

  template<typename T = Vertex>
  std::set<T *> vertices() {
    std::set<T *> ret;
    for (auto &v : vertices_) {
      if (auto t = dynamic_cast<T*>(v.get())) {
        ret.insert(t);
      }
    }
    return ret;
  }

  /* take ownership of a vertex if we haven't already
   */
  Vertex *take_vertex(Vertex *v) {
    assert(v && "tried to take vertex nullptr");
    auto it = std::find_if(vertices_.begin(), vertices_.end(), VertexCmp(v));
    if (it == vertices_.end()) {
      vertices_.insert(std::unique_ptr<Vertex>(v));
    }
    return v;
  }

  /* take ownership of an edge if we haven't already
   */
  Edge *take_edge(Edge *e) {
    assert(e && "tried to take edge nullptr");
    auto it = std::find_if(edges_.begin(), edges_.end(), EdgeCmp(e));
    if (it == edges_.end()) {
      edges_.insert(std::unique_ptr<Edge>(e));
    }
    return e;
  }

  Edge *join(Vertex *u, Vertex *v, Edge *e) {
    assert(u);
    assert(v);
    assert(e);
    assert(e->u_ == nullptr && "edge is already connected");
    take_vertex(u);
    e->u_ = u;
    assert(e->v_ == nullptr && "edge is already connected");
    take_vertex(v);
    e->v_ = v;

    take_edge(e);
    u->edges_.insert(e);
    v->edges_.insert(e);
    return e;
  }

  /* it's hard to get a unique_ptr out of a set
     to return the original, might want to use a shared_ptr
  */
  void replace(Edge *orig, Edge *next) {
    // replace vertices edges with next
    orig->u_->edges_.erase(orig);
    orig->u_->edges_.insert(next);
    orig->v_->edges_.erase(orig);
    orig->v_->edges_.insert(next);

    // attach next to vertices
    next->u_ = orig->u_;
    next->v_ = orig->v_;

    // delete original edge
    auto it = std::find_if(edges_.begin(), edges_.end(), EdgeCmp(orig));
    edges_.erase(it);
  }

  std::vector<Path> paths(const Vertex *src, const Vertex *dst) {

    std::vector<Path> ret; // the paths from src to dst

    if (vertices_.empty()) {
      assert(edges_.empty());
      return ret;
    }

    std::set<const Edge *> visited; // the edges we have traversed
    std::deque<Path> worklist;

    // std::cerr << "init worklist " << src->edges_.size() << "\n";

    // initialize worklist
    for (const auto e : src->edges_) {
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
        const Vertex *u = next.back()->u_;
        for (const auto e : u->edges_) {
          Path path = next;
          if (0 == visited.count(e)) {
            path.push_back(e);
            visited.insert(e);
            worklist.push_back(path);
          }
        }

        const Vertex *v = next.back()->v_;
        for (const auto e : v->edges_) {
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
  std::set<std::unique_ptr<Vertex>> vertices_;
  std::set<std::unique_ptr<Edge>> edges_;
};

} // namespace hwgraph
