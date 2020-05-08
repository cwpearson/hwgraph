#pragma once

#include <cstdint>

struct QpiData {
  int64_t links_;
  int64_t speed_;
};
struct XbusData {
  int64_t bw_;
};
struct PciData {
  float linkSpeed;
  int64_t lanes;
};
struct NvlinkData {
  unsigned int version;
  int64_t lanes;
};
