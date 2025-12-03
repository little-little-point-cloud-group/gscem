#pragma once
#include "TEncBacTop.h"
#include "TEncTree.h"
#include "common/HighLevelSyntax.h"
#include "common/TComPointCloud.h"

class TComTspEncoder {
public:
  TComTspEncoder(TComTspEncoder&) = delete;
  TComTspEncoder& operator=(TComTspEncoder&) = delete;
  TComTspEncoder(HighLevelSyntax* hls, TEncBacTop* encBac);

  void encodeTsp(int numPoints, TComPointCloud& pointCloudOrg, int32_t* pointIdx,
                 int32_t* codedOrder, PC_POS tspStartPoint, UInt* isDuplicatePointTsp,
                 TComOctreePartitionParams& partitionParams);
  Int getNumBits(UInt num);


private:
  HighLevelSyntax* m_hls;  ///< pointer to high-level syntax parameters
  TEncBacTop* m_encBac;    ///< pointer to bac
  std::vector<int32_t> m_stack;
};

void encodeTspLcu(TComPointCloud* pointCloudOrg, TComPointCloud* pointCloudRec,
                  UInt& numReconPoints, TComOctreeNode& lcuNode, HighLevelSyntax* hls,
                  TEncBacTop* encBac, TComOctreePartitionParams& partitionParams,
                  TreeEncoderParams& encParams);
