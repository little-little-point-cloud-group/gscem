#pragma once
#include "TEncBacTop.h"
#include "common/HighLevelSyntax.h"
#include "common/TComPointCloud.h"

struct NanoflannCloud {
  std::vector<PC_POS> pts;

  inline size_t kdtree_get_point_count() const {
    return pts.size();
  }

  double kdtree_get_pt(const size_t idx, const size_t dim) const {
    return pts[idx][dim];
  }

  template<class BBOX> bool kdtree_get_bbox(BBOX& /* bb */) const {
    return false;
  }
};

struct pointSphereWithIndex {
  int64_t radius;
  double phi;
  double theta;
  int32_t index;
  bool operator<(const pointSphereWithIndex& rhs) const {
    // NB: index used to maintain stable sort
    if (radius == rhs.radius) {
      if (phi == rhs.phi) {
        return index < rhs.index;
      }
      return phi < rhs.phi;
    }
    return radius < rhs.radius;
  }
  bool operator==(const int64_t& pointMorton) const {
    // NB: index used to maintain stable sort
    return this->radius == pointMorton;
  }
  bool operator<(const int64_t& pointMorton) const {
    // NB: index used to maintain stable sort
    return this->radius < pointMorton;
  }
};

struct TreeEncoderParams {
  //sort mode: only MortonSort is implemented.
  enum SortMode { NoSort, MortonSort, HilbertSort, RadiusSort, AzimuthSort } sortMode;

  // limit on number of points per tree. Note: not used for now
  int maxPtsPerTree;
};

static Void mortonSortLcu(TComPointCloud* pointCloud, vector<int32_t>& lcuNodePointIdx,
                          TComOctreeNode& lcuNode, vector<UInt>& isDuplicatePointSet,
                          Bool geomRemoveDuplicateFlag);

static Void noSortLcu(TComPointCloud* pointCloud, vector<int32_t>& lcuNodePointIdx,
                      TComOctreeNode& lcuNode, vector<UInt>& isDuplicatePointSet,
                      Bool geomRemoveDuplicateFlag);

static Void hilbertSortLcu(TComPointCloud* pointCloud, vector<int32_t>& lcuNodePointIdx,
                           TComOctreeNode& lcuNode, vector<UInt>& isDuplicatePointSet,
                           Bool geomRemoveDuplicateFlag);
