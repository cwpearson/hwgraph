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

    std::vector<Path> paths;
    Path path;

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

    std::vector<Path> paths;
    Path path;

    // forward paths
    paths = g.paths(src, dst);
    REQUIRE(2 == paths.size());

    // backwards paths
    paths = g.paths(dst, src);
    REQUIRE(2 == paths.size());
  }

  SECTION("triangle") {
    auto a = std::make_shared<Vertex>();
    auto b = std::make_shared<Vertex>();
    auto c = std::make_shared<Vertex>();
    auto eab = std::make_shared<Edge>();
    auto eac = std::make_shared<Edge>();
    auto ebc = std::make_shared<Edge>();

    g.join(a, b, eab);
    g.join(a, c, eac);
    g.join(b, c, ebc);

    std::vector<Path> paths;
    Path path;

    paths = g.paths(a, b);
    REQUIRE(2 == paths.size());
    paths = g.paths(a, c);
    REQUIRE(2 == paths.size());
    paths = g.paths(b, c);
    REQUIRE(2 == paths.size());

    auto f = [b](Vertex_t v){return v == b;};
    auto p = g.shortest_path(a, f);
    REQUIRE(p.first.size() == 1);
    REQUIRE(p.second == b);

  }

}
