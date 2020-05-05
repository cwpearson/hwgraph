#include "hwgraph/hwgraph.hpp"

using namespace hwgraph;

int main(int argc, char **argv) {

  (void)argc;
  (void)argv;

  DiscoveryMethod methods = available_methods();
  Graph g = make_graph(methods);

  auto is_intel = [](Vertex_t v) {
    return v->type_ == Vertex::Type::Intel;
  };

  std::cout << "Intel CPUs:\n";
  for (auto v : g.get_vertices(is_intel)) {
    std::cout << v->str() << "\n";
  }

  auto is_gpu = [](Vertex_t v) {
    return v->type_ == Vertex::Type::Gpu;
  };

  std::cout << "GPUs:\n";
  for (auto v : g.get_vertices(is_gpu)) {
    std::cout << v->str() << "\n";
  }

}