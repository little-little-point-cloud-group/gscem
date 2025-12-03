#include "TDecTsp.h"
#include "TDecAttribute.h"

TComTspDecoder::TComTspDecoder(HighLevelSyntax* hls, TDecBacTop* decBac)
  : m_hls(hls)
  , m_decBac(decBac) {
  m_stack.reserve(1024);
}

int TComTspDecoder::decodeTsp(int numPoints, TComPointCloud& cloud, UInt startIdx, PC_POS curPos,
                              int geomTreeMaxSize, const V3<int32_t>& numbits_Lcusize_log2) {
  int nodeIdx = 0;
  PC_POS* decodedPoints = &(cloud[startIdx]);

  V3<int32_t> prePosV3;
  V3<int32_t> curPosV3;
  for (int k = 0; k < numPoints;) {
    int iBegin = k;
    int iEnd = std::min(k + geomTreeMaxSize, numPoints);
    // decide max partition
    if ((numPoints - iEnd) < geomTreeMaxSize) {
      iEnd = numPoints;
    }
    for (int i = iBegin; i < iEnd; i++) {
      if (i > 0) {
        curPos = decodedPoints[i - 1];
      }
      curPosV3[0] = curPos[0];
      curPosV3[1] = curPos[1];
      curPosV3[2] = curPos[2];
      if (i > iBegin + 1) {
        prePosV3[0] = decodedPoints[i - 2][0];
        prePosV3[1] = decodedPoints[i - 2][1];
        prePosV3[2] = decodedPoints[i - 2][2];
      } else {
        prePosV3 = curPosV3;
      }
      V3<int32_t> residual =
        m_decBac->decodePredTreeResidual(curPosV3, prePosV3, numbits_Lcusize_log2);

      decodedPoints[i][0] = curPos[0] + residual[0];
      decodedPoints[i][1] = curPos[1] + residual[1];
      decodedPoints[i][2] = curPos[2] + residual[2];
    }
    k = iEnd;
  }
  return numPoints;
}

int decodeTspLcu(TComPointCloud* pointCloudRec, UInt& numReconPoints, const TComOctreeNode& lcuNode,
                 HighLevelSyntax* hls, TDecBacTop* decBac,
                 const V3<int32_t>& numbits_Lcusize_log2) {
  TComTspDecoder decoder(hls, decBac);

  int numPoints = decBac->decodePredTreeNumPtsInLcu();
  UInt log2geomTreeMaxSizeMinus8 = hls->gps.log2geomTreeMaxSizeMinus8;
  UInt geomTreeMaxSize = 1 << (log2geomTreeMaxSizeMinus8 + 8);
  if (hls->aps.attributePresentFlag[1]) {
    pointCloudRec->addReflectances();
  }

  if (hls->aps.attributePresentFlag[0]) {
    pointCloudRec->addColors();
  }

  PC_POS tspStartPoint;
  auto tmpStartPoint = lcuNode.pos;
  tspStartPoint[0] = tmpStartPoint[0];
  tspStartPoint[1] = tmpStartPoint[1];
  tspStartPoint[2] = tmpStartPoint[2];
  int decodedPoints = decoder.decodeTsp(numPoints, *pointCloudRec, numReconPoints, tspStartPoint,
                                        geomTreeMaxSize, numbits_Lcusize_log2);
  numReconPoints += decodedPoints;
  return decodedPoints;
}