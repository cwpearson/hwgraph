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
    if (dynamic_cast<const Package *>(e)) {
      ++numPackages;
    }
  }
  REQUIRE(1 == numPackages);
}
