#pragma once

#include <vector

template <typename T> class Mat2D {

  std::vector<T> elems_;
  int64_t r_;
  int64_t c_;

public:
    Mat2D(int64_t r, int64_t c) : elems_(r * c), r_(r), c_(c) {

    }
    Mat2D() : r_(0), c_(0) {}
    Mat2D(int64_t r, int64_t c, T& val) : elems(r * c, val), r_(r), c_(c) {

    }

    Mat2D(Mat2D &&other) = default;
    Mat2D(const Mat2D &other) = default;
    ~Mat2D() {
        r_ = 0;
        c_ = 0;
    }

  void resize(int64_t r, int64_t c) {
    std::vector<T> newElems_(r * c);
    for (int64_t i = 0; i < std::min(r, r_); ++i) {
      for (int64_t j = 0; j < std::min(c, c_); ++j) {
        newElems_[i * c + j] = std::move(i * c_ + j);
      }
    }
    elems_ = std::move(newElems_);
  }

  T *&operator[](size_t i) { return &vector[i * c_]; }
  const T *&operator[](size_t i) const { return &vector[i * c_]; }
};