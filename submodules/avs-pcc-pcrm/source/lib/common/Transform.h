#pragma once

#include "common/FXPoint.h"
#include "common/TComPointCloud.h"

typedef void FwdTrans(int64_t[][8], int, int);

typedef void InvTrans(int64_t[][8], int, int);

Void Transform(int64_t tranformBuf[][8], int count, int num);

Void invTransform(int64_t transformBuf[][8], int count, int num);

// predict and haar transform
template<typename T>
void WaveletCoreTransform(FXPoint* attributes, const int attribCount, const int voxelCount,
                          T* integerizedAttributes, const SequenceParameterSet& sps,
                          const AttributeParameterSet& aps, const AttributeBrickHeader& abh,
                          int* positions, int* RecAttributes, const int disThInit);
template<typename T>
void WaveletCoreInverseTransform(FXPoint* attributes, const int attribCount, const int voxelCount,
                                 T* integerizedAttributes, const SequenceParameterSet& sps,
                                 const AttributeParameterSet& aps, const AttributeBrickHeader& abh,
                                 int* positions, const int disThInit);
// predict and haar transform with cross attribute predict
template<typename T>
void WaveletCoreTransform(FXPoint* attributes, const int attribCount, const int voxelCount,
                          T* integerizedAttributes, const SequenceParameterSet& sps,
                          const AttributeParameterSet& aps, const AttributeBrickHeader& abh,
                          int* positions, int* RecAttributes, const int disThInit,
                          int* refAttributes, const int refAttribCount);
template<typename T>
void WaveletCoreInverseTransform(FXPoint* attributes, const int attribCount, const int voxelCount,
                                 T* integerizedAttributes, const SequenceParameterSet& sps,
                                 const AttributeParameterSet& aps, const AttributeBrickHeader& abh,
                                 int* positions, const int disThInit, int* refAttributes,
                                 const int refAttribCount);

Void getLength(std::vector<pointCodeWithIndex>& pointCloudHilbert, vector<int>& length,
               vector<int>& numofGroupCount, int maxNumofCoeff, int& ShiftBits, UInt MaxNum);

Void lengthDivide(std::vector<pointCodeWithIndex>& pointCloudHilbert, vector<int>& length, int num,
                  int MaxTransNum, int maxNumofCoeff, vector<int>& numofGroupCount, int& totalNum,
                  int curIdx, int shiftBits, bool isduplicate);

Void getLengthRef(const Int& shift, std::vector<pointCodeWithIndex>& pointCloudHilbert,
                  vector<int>& length, vector<int>& numofGroupCount, int maxNumofCoeff,
                  int& ShiftBits, const UInt& MaxNum, const bool isMemControl);
