#pragma once

#include <sstream>
#include <iomanip>

#include "pci_address.hpp"

inline std::string hex_str(unsigned short u) {
    std::stringstream ss;
    ss << "0x" << std::hex << u;
    return ss.str();
}

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
};

struct NvLinkBridgeData {
  PciDeviceData pciDev;
};

struct NvSwitchData {
  PciDeviceData pciDev;
};