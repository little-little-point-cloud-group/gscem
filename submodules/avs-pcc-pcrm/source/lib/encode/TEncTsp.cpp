#include "TEncTsp.h"
#include "TEncAttribute.h"
#include "common/nanoflann.hpp"
#include <time.h>

TComTspEncoder::TComTspEncoder(HighLevelSyntax* hls, TEncBacTop* encBac)
  : m_hls(hls)
  , m_encBac(encBac) {
  m_stack.reserve(1024);
}
Int TComTspEncoder::getNumBits(UInt num) {
  int numBits = 0;
  while (num) {
    num = num >> 1;
    numBits++;
  }

  return numBits;
}
void TComTspEncoder::encodeTsp(int numPoints, TComPointCloud& pointCloudOrg, int32_t* pointIdx,
                               int32_t* codedOrder, PC_POS tspStartPoint, UInt* isDuplicatePointTsp,
                               TComOctreePartitionParams& partitionParams) {
  using NanoflannKdTreeT = nanoflann::KDTreeSingleIndexDynamicAdaptor<
    nanoflann::L1_Adaptor<double, NanoflannCloud, double>, NanoflannCloud, 3, int32_t>;
  using NanoflannResultT = nanoflann::KNNResultSet<double, int32_t>;

  NanoflannCloud predictionPoints;
  NanoflannKdTreeT predictionPointsTree(3, predictionPoints);
  predictionPoints.pts.reserve(numPoints);

  // add points in KDTREE
  vector<UInt> searchPointIdxSet;
  for (int i = 0; i < numPoints; i++) {
    if ((isDuplicatePointTsp[i] != 0) && (i != 0)) {
      continue;
    } else {
      const auto size_begin = predictionPoints.pts.size();
      searchPointIdxSet.push_back(i);
      predictionPoints.pts.push_back(pointCloudOrg[pointIdx[i]]);
      const auto size_end = predictionPoints.pts.size();
      predictionPointsTree.addPoints(size_begin, size_end - 1);
    }
  }

  // initialization
  const UInt numResults = 1;
  auto curPoint = tspStartPoint;

  for (int nodeIdx = 0; nodeIdx < numPoints;) {
    if (nodeIdx > 0) {
      curPoint = pointCloudOrg[codedOrder[nodeIdx - 1]];
    }

    // search next point using KDTREE
    int searchPointIdx;
    double searchDistance;
    NanoflannResultT resultSet(numResults);
    resultSet.init(&searchPointIdx, &searchDistance);
    predictionPointsTree.findNeighbors(resultSet, &curPoint[0], {});

    // remove the visited node in the KDTREE
    UInt searchPointIdxOrigin = searchPointIdxSet[searchPointIdx];
    predictionPointsTree.removePoint(searchPointIdx);
    codedOrder[nodeIdx] = pointIdx[searchPointIdxOrigin];
    nodeIdx++;

    // duplicate point order
    if (!m_hls->sps.geomRemoveDuplicateFlag) {
      while (((++searchPointIdxOrigin) < numPoints) &&
             (isDuplicatePointTsp[searchPointIdxOrigin] != 0)) {
        codedOrder[nodeIdx] = pointIdx[searchPointIdxOrigin];
        nodeIdx++;
      }
    }
  }

  V3<int32_t> numbits_Lcusize_log2;
  for (int k = 0; k < 3; k++) {
    numbits_Lcusize_log2[k] = getNumBits(uint32_t(partitionParams.nodeSizeLog2[k]));
  }

  auto curPos = tspStartPoint;
  auto predPos = pointCloudOrg[codedOrder[0]];
  V3<int32_t> prePosV3;
  V3<int32_t> curPosV3;
  for (int i = 0; i < numPoints; i++) {
    if (i > 0) {
      curPos = pointCloudOrg[codedOrder[i - 1]];
      predPos = pointCloudOrg[codedOrder[i]];
    }
    auto diffPos = predPos - curPos;

    V3<int32_t> residual;
    residual[0] = diffPos[0];
    residual[1] = diffPos[1];
    residual[2] = diffPos[2];
    curPosV3[0] = (int)curPos[0];
    curPosV3[1] = (int)curPos[1];
    curPosV3[2] = (int)curPos[2];
    if (i > 1) {
      prePosV3[0] = (int)pointCloudOrg[codedOrder[i - 2]][0];
      prePosV3[1] = (int)pointCloudOrg[codedOrder[i - 2]][1];
      prePosV3[2] = (int)pointCloudOrg[codedOrder[i - 2]][2];

    } else
      prePosV3 = curPosV3;
    m_encBac->encodePredTreeResidual(residual, curPosV3, prePosV3, numbits_Lcusize_log2);
  }
}

void encodeTspLcu(TComPointCloud* pointCloudOrg, TComPointCloud* pointCloudRec,
                  UInt& numReconPoints, TComOctreeNode& lcuNode, HighLevelSyntax* hls,
                  TEncBacTop* encBac, TComOctreePartitionParams& partitionParams,
                  TreeEncoderParams& encParams) {
  Int numPointsInLcu = lcuNode.childIdxEnd - lcuNode.childIdxBegin;
  encBac->encodePredTreeNumPtsInLcu(numPointsInLcu);

  vector<int32_t> lcuNodePointIdx(numPointsInLcu, -1);
  // src indexes in coded order
  vector<int32_t> codedOrder(numPointsInLcu, -1);
  vector<UInt> isDuplicatePointSet(numPointsInLcu, 0);

  // sort the points in the Lcu node
  if (encParams.sortMode == TreeEncoderParams::NoSort && !hls->aps.eligibleDupPointPred) {
    noSortLcu(pointCloudOrg, lcuNodePointIdx, lcuNode, isDuplicatePointSet,
              hls->sps.geomRemoveDuplicateFlag);
  } else if (encParams.sortMode == TreeEncoderParams::MortonSort || hls->aps.eligibleDupPointPred) {
    mortonSortLcu(pointCloudOrg, lcuNodePointIdx, lcuNode, isDuplicatePointSet,
                  hls->sps.geomRemoveDuplicateFlag);
  } else {
    cout << "Not supported sort order at LCU level right now" << endl;
    exit(1);
  }

  // create TSP encoder
  TComTspEncoder encoder(hls, encBac);
  int maxPointsPerTree = std::min(encParams.maxPtsPerTree, numPointsInLcu);

  // Initialization
  PC_POS tspStartPoint;
  auto tmpStartPoint = lcuNode.pos;
  tspStartPoint[0] = tmpStartPoint[0];
  tspStartPoint[1] = tmpStartPoint[1];
  tspStartPoint[2] = tmpStartPoint[2];

  for (int i = 0; i < numPointsInLcu;) {
    int iBegin = i;
    int iEnd = std::min(i + maxPointsPerTree, numPointsInLcu);
    // decide max partition
    if ((numPointsInLcu - iEnd) < maxPointsPerTree) {
      iEnd = numPointsInLcu;
    }
    int numPoints = iEnd - iBegin;

    int32_t* pointIdx = lcuNodePointIdx.data() + iBegin;
    encoder.encodeTsp(numPoints, *pointCloudOrg, pointIdx, codedOrder.data() + iBegin,
                      tspStartPoint, isDuplicatePointSet.data() + iBegin, partitionParams);
    tspStartPoint = (*pointCloudOrg)[codedOrder[iEnd - 1]];

    for (; i < iEnd; i++) {
      auto srcIdx = codedOrder[i];
      (*pointCloudRec)[numReconPoints] = (*pointCloudOrg)[srcIdx];
      if (pointCloudRec->hasColors()) {
        for (int multiIdx = 0; multiIdx < pointCloudOrg->getNumMultilColor(); ++multiIdx)
          pointCloudRec->setColor(numReconPoints, pointCloudOrg->getColor(srcIdx, multiIdx),
                                  multiIdx);
      }
      if (pointCloudRec->hasReflectances()) {
        for (int multiIdx = 0; multiIdx < pointCloudOrg->getNumMultilRefl(); ++multiIdx)
          pointCloudRec->setReflectance(numReconPoints,
                                        pointCloudOrg->getReflectance(srcIdx, multiIdx), multiIdx);
      }
      numReconPoints++;
    }
  }
}

static Void mortonSortLcu(TComPointCloud* pointCloud, vector<int32_t>& lcuNodePointIdx,
                          TComOctreeNode& lcuNode, vector<UInt>& isDuplicatePointSet,
                          Bool geomRemoveDuplicateFlag) {
  Int numPointsInLcu = lcuNodePointIdx.size();
  std::vector<pointCodeWithAttr> mortonOrder(numPointsInLcu);

  for (UInt idx = 0; idx < numPointsInLcu; idx++) {
    auto index = lcuNode.childIdxBegin + idx;
    mortonOrder[idx].index = index;
    const PC_POS& point = (*pointCloud)[index];
    mortonOrder[idx].code = mortonAddr((Int32)point[0], (Int32)point[1], (Int32)point[2]);
    if (pointCloud->hasColors()) {
      auto attr = pointCloud->getColor(index, 0);
      mortonOrder[idx].attr = attr[0];
    }
    if (pointCloud->hasReflectances()) {
      auto attr = pointCloud->getReflectance(index, 0);
      mortonOrder[idx].attr = attr;
    }
  }
  std::sort(mortonOrder.begin(), mortonOrder.end());

  if (geomRemoveDuplicateFlag) {
    for (UInt idx = 0; idx < numPointsInLcu; idx++) {
      lcuNodePointIdx[idx] = mortonOrder[idx].index;
    }
  } else {
    lcuNodePointIdx[0] = mortonOrder[0].index;
    for (UInt idx = 1; idx < numPointsInLcu; idx++) {
      lcuNodePointIdx[idx] = mortonOrder[idx].index;
      if (mortonOrder[idx].code == mortonOrder[idx - 1].code) {
        isDuplicatePointSet[idx] = 1;
      }
    }
  }
}

static Void noSortLcu(TComPointCloud* pointCloud, vector<int32_t>& lcuNodePointIdx,
                      TComOctreeNode& lcuNode, vector<UInt>& isDuplicatePointSet,
                      Bool geomRemoveDuplicateFlag) {
  Int numPointsInLcu = lcuNodePointIdx.size();
  for (UInt idx = 0; idx < numPointsInLcu; idx++) {
    lcuNodePointIdx[idx] = lcuNode.childIdxBegin + idx;
  }
}