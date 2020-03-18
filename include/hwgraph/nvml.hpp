#pragma once

#include <iostream>
#include <cstdio>

#include <nvml.h>

#include "graph.hpp"

namespace hwgraph {
namespace nvml {

inline void checkNvml(nvmlReturn_t result, const char *file, const int line) {
  if (result != NVML_SUCCESS) {
    fprintf(stderr, "nvml Error: %s in %s : %d\n", nvmlErrorString(result),
            file, line);
    exit(-1);
  }
}

#define NVML(stmt) checkNvml(stmt, __FILE__, __LINE__);

static bool once = false;
class Initer {
  private:

public:
  Initer() {
    if (!once) {
      NVML(nvmlInit());
      once = true;
    }
  }
};

Initer initer;


inline void add_gpus(hwgraph::Graph &graph) {
  unsigned int deviceCount;
  NVML(nvmlDeviceGetCount(&deviceCount));

  (void) graph;
}

inline void add_nvlinks(hwgraph::Graph &graph){
  (void) graph;
}

} // namespace nvml
} // namespace hwgraph
