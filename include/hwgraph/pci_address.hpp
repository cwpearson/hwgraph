#pragma once

#include <sstream>
#include <iomanip>

struct PciAddress {
  typedef unsigned short domain_type;
  typedef unsigned char bus_type;
  typedef unsigned char dev_type;
  typedef unsigned char func_type;

  domain_type domain_;
  bus_type bus_;
  dev_type dev_;
  func_type func_;

  bool operator==(const PciAddress &rhs) const noexcept {
    return rhs.domain_ == domain_ && rhs.bus_ == bus_ && rhs.dev_ == dev_ &&
           rhs.func_ == func_;
  }

  bool operator<(const PciAddress &rhs) const noexcept {
    if (rhs.domain_ < domain_) {
      return true;
    }
    if (rhs.domain_ > domain_) {
      return false;
    }
    if (rhs.bus_ < bus_) {
      return true;
    }
    if (rhs.bus_ > bus_) {
      return false;
    }
    if (rhs.dev_ < dev_) {
      return true;
    }
    if (rhs.dev_ > dev_) {
      return false;
    }
    return rhs.func_ < func_;
  }

  std::string domain_str() const {
    std::stringstream ret;
    ret << std::setfill('0') << std::setw(4) << std::hex << (size_t)domain_;
    return ret.str();
  }
  std::string bus_str() const {
    std::stringstream ret;
    ret << std::setfill('0') << std::setw(2) << std::hex << (size_t)bus_;
    return ret.str();
  }
  std::string dev_str() const {
    std::stringstream ret;
    ret << std::setfill('0') << std::setw(2) << std::hex << (size_t)dev_;
    return ret.str();
  }
  std::string func_str() const {
    std::stringstream ret;
    ret << std::setfill('0') << std::setw(1) << std::hex << (size_t)func_;
    return ret.str();
  }

  std::string str() const {
    std::stringstream ret;
    ret << domain_str() << ":" << bus_str() << ":" << dev_str() << "."
        << func_str();
    return ret.str();
  }
};