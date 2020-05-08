#pragma once

#include <iomanip>
#include <sstream>

#include "config.hpp"
#include "pci_address.hpp"

inline std::string hex_str(unsigned short u) {
  std::stringstream ss;
  ss << "0x" << std::hex << u;
  return ss.str();
}

struct UnknownData {

};

struct BridgeData {
  PciAddress addr;
  PciAddress domain;
  PciAddress secondaryBus;
  PciAddress subordinateBus;
};

struct PciDeviceData {
  PciAddress addr;
  unsigned short classId;
  unsigned short vendorId;
  unsigned short deviceId;
  unsigned short subvendorId;
  unsigned short subdeviceId;
  unsigned char revision;
  float linkSpeed;

  std::string str() const {
    std::string s = "{";
    s += "addr: " + addr.str() + ", ";
    s += "classId: " + hex_str(classId) + ", ";
    s += "vendorId: " + hex_str(vendorId) + ", ";
    s += "deviceId: " + hex_str(deviceId) + ", ";
    s += "subvendorId: " + hex_str(subvendorId) + ", ";
    s += "subdeviceId: " + hex_str(subdeviceId) + ", ";
    s += "revision: " + hex_str(revision) + ", ";
    s += "linkSpeed: " + std::to_string(linkSpeed);
    s += "}";
    return s;
  }
};

struct GpuData {
  PciDeviceData pciDev;
  int ccMajor; // CUDA compute capability major
  int ccMinor; // CUDA compute capability minor

  std::string str() const {
    std::string s = "{";
    s += "pci_dev: " + pciDev.str() + ", ";
    s += "cc: " + std::to_string(ccMajor) + "." + std::to_string(ccMinor);
    s += "}";
    return s;
  }
};

struct NvLinkBridgeData {
  PciDeviceData pciDev;
};

struct NvSwitchData {
  PciDeviceData pciDev;
};

struct IntelData {
  unsigned idx; // hwloc index
  std::string model;
  std::string vendor;
  int modelNumber;
  int familyNumber;
  int stepping;

  std::string str() const {
    std::string s = "{";
    s += "idx: " + std::to_string(idx) + ", ";
    s += "model: " + std::string(model) + ", ";
    s += "vendor: " + std::string(vendor) + ", ";
    s += "modelNumber: " + std::to_string(modelNumber) + ", ";
    s += "familyNumber: " + std::to_string(familyNumber) + ", ";
    s += "stepping: " + std::to_string(stepping);
    s += "}";
    return s;
  }
};

struct PpcData {
  unsigned idx; // hwloc index
  std::string model;
  int revision;

  std::string str() const {
    std::string s = "{";
    s += "idx: " + std::to_string(idx) + ", ";
    s += "model: " + std::string(model) + ", ";
    s += "revision: " + std::to_string(revision);
    s += "}";
    return s;
  }
};