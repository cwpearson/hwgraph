#include <argparse/argparse.hpp>

#include "hwgraph/hwgraph.hpp"

using namespace hwgraph;

int main(int argc, char **argv) {
  argparse::Parser p;
  bool modeText = true;
  bool modeJson = false;
  bool modeDot = false;
  p.add_flag(modeJson, "--json", "-j")->help("JSON output");
  p.add_flag(modeDot, "--dot", "-d")->help("Graphviz output");
  if (!p.parse(argc, argv)) {
    std::cerr << p.help();
    exit(EXIT_FAILURE);
  }
  if (p.need_help()) {
    std::cerr << p.help();
    return 0;
  }

  DiscoveryMethod methods = available_methods();
  Graph g = make_graph(methods);

  if (modeDot) {
    std::cout << g.dot_str();
  } else if (modeJson) {
    std::cerr << "json output not supported\n";
    exit(EXIT_FAILURE);
  } else if (modeText) {
    std::cout << "Intel CPUs:\n";
    auto is_intel = [](Vertex_t v) { return v->type_ == Vertex::Type::Intel; };
    for (auto v : g.get_vertices(is_intel)) {
      std::cout << v->str() << "\n";
    }
    std::cout << "GPUs:\n";
    auto is_gpu = [](Vertex_t v) { return v->type_ == Vertex::Type::Gpu; };
    for (auto v : g.get_vertices(is_gpu)) {
      std::cout << v->str() << "\n";
    }

    auto is_cpu = [](Vertex_t v) { return v->type_ == Vertex::Type::Intel || v->type_ == Vertex::Type::Ppc; };

    std::cout << "CPU-GPU Paths:\n";
    std::set<Vertex_t> cpus = g.get_vertices(is_cpu);
    std::set<Vertex_t> gpus = g.get_vertices(is_gpu);

    for (auto &src : cpus) {
      for (auto &dst : gpus) {
        Path path = g.max_path(src, dst, path_bandwidth);

        for (const Edge_t &e : path) {
          std::cout << e->str() << "\n";
        }

      }
    }



  }

  return 0;
}