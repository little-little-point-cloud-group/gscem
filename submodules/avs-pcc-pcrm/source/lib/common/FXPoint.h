
#pragma once

#include <cstdint>

//============================================================================

class FXPoint {
public:
  // Number of fractional bits in fixed point representation
  static int kFracBits;
  static int kOneHalf;

  // Fixed point value
  int64_t val;

  FXPoint() = default;
  FXPoint(const FXPoint&) = default;
  FXPoint(FXPoint&&) = default;
  FXPoint& operator=(const FXPoint&) = default;
  FXPoint& operator=(FXPoint&&) = default;

  FXPoint(int val) {
    this->operator=(int64_t(val));
  }
  FXPoint(int64_t val) {
    this->operator=(val);
  }
  FXPoint(double val) {
    this->val = int64_t(val * (1 << kFracBits));
  }

  // return the rounded integer value
  int64_t round();
  static void set_kFracBits(int set);
  void operator=(const int64_t val);
  void operator+=(const FXPoint& that);
  void operator-=(const FXPoint& that);
  void operator*=(const FXPoint& that);
  void operator*=(const int64_t that);
  void operator/=(const FXPoint& that);
  void operator>>=(const int64_t that);
  void operator<<=(const int64_t that);
};

//inline void FXPoint::change_kFracBits(int set) {
//    this->kFracBits = set;
//}

inline int64_t FXPoint::round() {
  if (this->val > 0)
    return (kOneHalf + this->val) >> kFracBits;
  return -((kOneHalf - this->val) >> kFracBits);
}

inline void FXPoint::operator=(int64_t val) {
  if (val > 0)
    this->val = val << kFracBits;
  else
    this->val = -((-val) << kFracBits);
}

inline void FXPoint::operator+=(const FXPoint& that) {
  this->val += that.val;
}

inline void FXPoint::operator-=(const FXPoint& that) {
  this->val -= that.val;
}

inline void FXPoint::operator*=(const FXPoint& that) {
  this->val *= that.val;

  if (this->val < 0)
    this->val = -((kOneHalf - this->val) >> kFracBits);
  else
    this->val = +((kOneHalf + this->val) >> kFracBits);
}

inline void FXPoint::operator*=(const int64_t val) {
  this->val = (this->val * val) >> kFracBits;
}

inline void FXPoint::operator/=(const FXPoint& that) {
  if (this->val < 0) {
    if (that.val < 0)
      this->val = -(((-that.val) >> 1) + ((-this->val) << kFracBits)) / that.val;
    else
      this->val = -(((+that.val) >> 1) + ((-this->val) << kFracBits)) / that.val;
  } else {
    if (that.val < 0)
      this->val = +(((-that.val) >> 1) + ((+this->val) << kFracBits)) / that.val;
    else
      this->val = +(((+that.val) >> 1) + ((+this->val) << kFracBits)) / that.val;
  }
}

inline void FXPoint::operator>>=(int64_t that) {
  int64_t offset = that > 0 ? 1 << (that - 1) : 0;
  if (this->val > 0)
    this->val = (offset + this->val) >> that;
  else
    this->val = -((offset - this->val) >> that);
}

inline void FXPoint::operator<<=(int64_t that) {
  if (this->val < 0)
    this->val = -(-this->val) << that;
  else
    this->val = this->val << that;
}