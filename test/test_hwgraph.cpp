#include "catch2/catch.hpp"

#include "hwgraph/hwgraph.hpp"

TEST_CASE("hwgraph", "[hwloc][nvml]") {

  using namespace hwgraph;

  DiscoveryMethod method = DiscoveryMethod::None;
#if HWGRAPH_USE_NVML == 1
  method |= DiscoveryMethod::Nvml;
#endif

#if HWGRAPH_USE_HWLOC == 1
  method |= DiscoveryMethod::Hwloc;
#endif

  Graph g = make_graph(method);

  int64_t numPackages = 0;
  for (auto e : g.vertices()) {
    if (e->type_ == Vertex::Type::Intel || e->type_ == Vertex::Type::Ppc) {
      ++numPackages;
    }
  }
  REQUIRE(1 <= numPackages);
  REQUIRE(8 >= numPackages);

  std::cerr << g.dot_str() << "\n";
}
