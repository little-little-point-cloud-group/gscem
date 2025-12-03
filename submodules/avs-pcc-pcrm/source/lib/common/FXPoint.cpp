#include "common/FXPoint.h"
int FXPoint::kFracBits;
int FXPoint::kOneHalf = 1 << (kFracBits - 1);
void FXPoint::set_kFracBits(int set) {
  FXPoint::kFracBits = set;
  FXPoint::kOneHalf = 1 << (kFracBits - 1);
}