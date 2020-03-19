#include "catch2/catch.hpp"

#include "hwgraph/graph.hpp"

using namespace hwgraph;

TEST_CASE("graph", "") {

  Graph g;

  SECTION("ctor") {
    auto paths = g.paths(nullptr, nullptr);
    REQUIRE(paths.empty());
  }

  SECTION("insert_vertex") {
    auto v0 = std::make_shared<Vertex>();
    auto v1 = std::make_shared<Vertex>();

    g.insert_vertex(v0);
    REQUIRE(1 == g.vertices().count(v0));
    g.insert_vertex(v0);
    REQUIRE(1 == g.vertices().count(v0));
    g.insert_vertex(v1);
    REQUIRE(1 == g.vertices().count(v0));
    REQUIRE(1 == g.vertices().count(v1));

  }

    SECTION("insert_edge") {
    auto e0 = std::make_shared<Edge>();
    auto e1 = std::make_shared<Edge>();

    g.insert_edge(e0);
    REQUIRE(1 == g.edges().count(e0));
    g.insert_edge(e0);
    REQUIRE(1 == g.edges().count(e0));
    g.insert_edge(e1);
    REQUIRE(1 == g.edges().count(e0));
    REQUIRE(1 == g.edges().count(e1));
  }

  SECTION("join") {
    auto src = std::make_shared<Vertex>();
    auto dst = std::make_shared<Vertex>();
    auto e = std::make_shared<Edge>();

    g.join(src, dst, e);

    REQUIRE(1 == g.edges().count(e));
    REQUIRE(1 == g.vertices().count(src));
    REQUIRE(1 == g.vertices().count(dst));
  }

  SECTION("single path") {
    auto src = std::make_shared<Vertex>();
    auto dst = std::make_shared<Vertex>();
    auto e = std::make_shared<Edge>();

    g.join(src, dst, e);

    std::vector<Graph::Path> paths;
    Graph::Path path;

    // forward path
    paths = g.paths(src, dst);
    REQUIRE(1 == paths.size());
    path = paths.back();
    REQUIRE(1 == path.size());
    REQUIRE(e == path.back());

    // backwards path
    paths = g.paths(dst, src);
    REQUIRE(1 == paths.size());
    path = paths.back();
    REQUIRE(1 == path.size());
    REQUIRE(e == path.back());
  }

  SECTION("multiple_paths") {
    auto src = std::make_shared<Vertex>();
    auto dst = std::make_shared<Vertex>();
    auto e1 = std::make_shared<Edge>();
    auto e2 = std::make_shared<Edge>();

    g.join(src, dst, e1);
    g.join(dst, src, e2);

    REQUIRE(1 == g.edges().count(e1));
    REQUIRE(1 == g.edges().count(e2));
    REQUIRE(1 == g.vertices().count(src));
    REQUIRE(1 == g.vertices().count(dst));
    REQUIRE(2 == src->edges_.size());
    REQUIRE(2 == dst->edges_.size());

    std::vector<Graph::Path> paths;
    Graph::Path path;

    // forward paths
    paths = g.paths(src, dst);
    REQUIRE(2 == paths.size());

    // backwards paths
    paths = g.paths(dst, src);
    REQUIRE(2 == paths.size());
  }
}
