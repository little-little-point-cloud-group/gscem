#pragma once

#include "TDecBacTop.h"
#include "common/HighLevelSyntax.h"
#include "common/TComPointCloud.h"
//#include "common/TComPredictiveTree.h"

class TComTspDecoder {
public:
  TComTspDecoder(TComTspDecoder&) = delete;
  TComTspDecoder& operator=(TComTspDecoder&) = delete;
  TComTspDecoder(HighLevelSyntax* hls, TDecBacTop* decBac);

  int decodeTsp(int numPoints, TComPointCloud& cloud, UInt startIdx, PC_POS tspStartPoint,
                int geomTreeMaxSize, const V3<int32_t>& m);

private:
  HighLevelSyntax* m_hls;  ///< pointer to high-level syntax parameters
  TDecBacTop* m_decBac;    ///< pointer to bac
  std::vector<int32_t> m_stack;
};

int decodeTspLcu(TComPointCloud* pointCloudRec, UInt& numReconPoints, const TComOctreeNode& lcuNode,
                 HighLevelSyntax* hls, TDecBacTop* decBac, const V3<int32_t>& m);
