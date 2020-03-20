#pragma once

#include <string>
#include <vector>
#include <set>

class DotField {
private:
  std::string str_;

  void reflow() {
    remove_newlines();
    add_newlines();
  }

  void remove_newlines() { std::set<std::string> substrs = {"\\n"}; }

  void add_newlines() {
    const size_t minLineLength = 20;
    std::set<char> delims = {' '};

    size_t currentLineLength = 0;
    for (auto iter = str_.begin(); iter != str_.end(); ++iter) {
      if ((currentLineLength > minLineLength) && delims.count(*iter)) {
        iter = str_.erase(iter);
        iter = str_.insert(iter, 'n');
        iter = str_.insert(iter, '\\');
        currentLineLength = 0;
      } else {
        ++currentLineLength;
      }
    }
  }

public:
  DotField(const std::string &str) : str_(str) { reflow(); }
  const std::string &str() const { return str_; }
};

class DotLabel {
  std::vector<DotField> fields_;

public:
  explicit DotLabel(const std::string &str) {
    fields_.push_back(DotField(str));
  }
  DotLabel &with_field(const std::string &str) {
    fields_.push_back(DotField(str));
    return *this;
  }

  std::string str() const {
    std::string ret;

    if (fields_.size() > 1) {
      ret += "{ ";
    }

    for (size_t i = 0; i < fields_.size(); ++i) {
      ret += fields_[i].str();
      if (i + 1 < fields_.size()) {
        ret += " | ";
      }
    }

    if (fields_.size() > 1) {
      ret += " }";
    }

    return ret;
  }
};
