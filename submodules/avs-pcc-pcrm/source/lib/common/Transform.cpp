
//#include "PCCMisc.h"
#include "common/Transform.h"
#include "common/transform_core.h"
#include <queue>
#include <stack>
#include <tuple>
#include <utility>
using namespace std;
//============================================================================

template<typename T>
void WaveletCoreTransform(FXPoint* attributes, const int attribCount, const int voxelCount,
                          T* integerizedAttributes, const SequenceParameterSet& sps,
                          const AttributeParameterSet& aps, const AttributeBrickHeader& abh,
                          int* positions, int* RecAttributes, const int disThInit) {
  size_t M, N;
  size_t d, j, i, S;

  int offsetShift = aps.deadZoneLen == 0 ? 1 : aps.deadZoneLen * 0.1 * (1 << encoderShiftBit);
  int axisBias = aps.axisBias;
  int transQuantParam;
  int countPLow = 6, countTLow = 0;
  int countT = 0, countP = 0;

  if (attribCount == 3) {  // color default setting
    offsetShift = 0.2 * (1 << encoderShiftBit);
    transQuantParam = aps.colorQuantParam;
    if (aps.transResLayer)
      transQuantParam += aps.attrTransQpDelta;
    countTLow = std::min(32, 1 << (transQuantParam / 8));
    countTLow = 2 * std::max(8, countTLow);
  } else {
    transQuantParam = aps.reflQuantParam + abh.QpOffset;
    if (aps.transResLayer)
      transQuantParam += aps.attrTransQpDelta;
    countTLow = std::min(32, 1 << (transQuantParam / 8));
    if (abh.reflInitPredTransRatio >= 8) {  // cat1A
      countTLow = 64 * std::max((int)8, countTLow);
    } else {  // cat2 cat1C
      countTLow = 1 * std::max((int)8, countTLow);
    }
  }

  // processing single pts
  if (voxelCount == 1) {
    int predValue = 0;
    if (attribCount == 3) {
      predValue = 128;
    }
    for (size_t k = 0; k < attribCount; k++) {
      int64_t delta = attributes[k].round() - predValue;
      int residualSign = delta < 0 ? -1 : 1;
      uint64_t absResidual = std::abs(delta);
      uint64_t residualQuant;
      residualQuant = QuantizaResidual(absResidual, transQuantParam, offsetShift);
      int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
      integerizedAttributes[k] = signResidualQuant;
      uint64_t inverseResidualQuant = InverseQuantizeResidual(residualQuant, transQuantParam);
      int64_t recResidual = inverseResidualQuant * residualSign;
      RecAttributes[k] = recResidual + predValue;
    }
    return;
  }

  int64_t DistanceTH[32] = {0};
  DistanceTH[0] = disThInit;
  FXPoint* attributeCoeff = new FXPoint[voxelCount * attribCount * 2]();  // transform coefficients
  FXPoint* recAttributesCoeff =
    new FXPoint[voxelCount * attribCount * 2]();           // reconstructed attributes
  bool* nodeModeFlag = new bool[voxelCount * 2]();         // transform/predict node flag
  int* positionsAllNodes = new int[2 * voxelCount * 3]();  // position coordinates
  size_t S_buffer[32] = {};

  int ii = 2 * voxelCount, lowBound = voxelCount, jj = 0;
  while (ii > lowBound) {
    ii--;
    for (int kk = 0; kk < attribCount; kk++)
      attributeCoeff[ii * attribCount + kk] = attributes[jj * attribCount + kk];
    for (int kk = 0; kk < 3; kk++)
      positionsAllNodes[ii * 3 + kk] = positions[jj * 3 + kk];
    jj++;
  }

  d = 0;  // 层数
  int distanceNodes = 0, distanceNodes_prev = 0, distanceNodes_prev_prev = 0, pairMode = 0;
  bool predictFlag = true;
  int idxNodeFlag, idxDCT;
  M = N = voxelCount;
  S_buffer[0] = 2 * N;
  //************************************* compute DC and pred flag bottom->top **************************
  while (N > 1) {
    S_buffer[d + 1] = M;
    idxNodeFlag = S_buffer[d];
    d++;    //当前层序号
    S = N;  //当前层节点数
    i = 0;  //attributes 当前节点
    M = 0;  //attributesTransformed 当前节点
    countP = 0, countT = 0;
    while (i < S) {
      idxDCT = S_buffer[d - 1] - i - 1;
      if (predictFlag) {
        distanceNodes = 0;
        for (int kk = 0; kk < 3; kk++) {
          int64_t tmp =
            (positionsAllNodes[idxDCT * 3 + kk] - positionsAllNodes[(idxDCT - 1) * 3 + kk]);
          distanceNodes += tmp * tmp;
        }
        if (countT > countTLow && DistanceTH[d - 1] != 1) {
          DistanceTH[d - 1] = std::max(1, (int)(DistanceTH[d - 1] / 2));
          countT = 0;
        } else if (countP > countPLow) {
          DistanceTH[d - 1] = DistanceTH[d - 1] * 2;
          countP = 0;
        }
      }
      if ((distanceNodes > DistanceTH[d - 1]) && predictFlag) {  // pred node
        idxNodeFlag--;
        nodeModeFlag[idxNodeFlag] = true;
        i++;
        countP++;
        countT = 0;
      } else {  // if distanceNodes <= th
        if (distanceNodes > 0) {
          countT = countT + 2;
          countP = 0;
        }
        FXPoint attributes222[3][2] = {};
        for (int k = 0; k < attribCount; k++) {
          attributes222[k][0] = attributeCoeff[idxDCT * attribCount + k];
          attributes222[k][1] = attributeCoeff[(idxDCT - 1) * attribCount + k];
        }
        for (int k = 0; k < attribCount; k++) {  // compute DC coefficient
          attributes222[k][0] += attributes222[k][1];
        }
        N--;
        for (int kk = 0; kk < attribCount; kk++) {
          attributeCoeff[N * attribCount + kk] = attributes222[kk][0];
        }
        for (int kk = 0; kk < 3; kk++) {  // compute parent node coordinates
          positionsAllNodes[N * 3 + kk] =
            (positionsAllNodes[idxDCT * 3 + kk] + positionsAllNodes[(idxDCT - 1) * 3 + kk] + 1) / 2;
        }
        M++;
        idxNodeFlag -= 2;
        i += 2;
      }
      if (i == S - 1) {
        idxNodeFlag--;
        nodeModeFlag[idxNodeFlag] = true;
        i++;
      }
    }
    N = M;

    if (
      M < S_buffer[d] / 2 &&
      M >
        128)  // M: 当前层父层的节点数（d+1 层）；如果M大于128或者当前层具有预测节点，则进行距离阈值的更新
      DistanceTH[d] =
        std::max({(int64_t)1, (int64_t)((double)DistanceTH[d - 1] * S_buffer[d] / M)});
    else
      predictFlag = false;  // 当前层父层（d+1 层），不进行距离阈值判断，全部为变换节点
  }
  FXPoint attributesDC[3] = {};
  for (size_t k = 0; k < attribCount; k++) {
    attributesDC[k] = attributeCoeff[(S_buffer[d] - 1) * attribCount + k];
  }
  //************************************* compute AC and residue top->bottom **************************
  int S_top, S_bottom, N_top, N_bottom, N_parent;

  S_buffer[d + 1] = M;
  int MAXd = d;
  V3<FXPoint> pred;
  while (d > 0) {
    FXPoint div_ac, div_dc, div_ac_neg;
    if (d & 1) {
      div_ac = 1 << (d - 1) / 2;
      div_ac *= KfraIndex[FXPoint::kFracBits - 1];
      div_ac_neg = 1 << (d - 1) / 2;
      div_ac_neg *= -KfraIndex[FXPoint::kFracBits - 1];
      div_dc = 1 << (d - 1) / 2;
    } else {
      div_ac = 1 << d / 2;
      div_ac_neg = -(1 << d / 2);
      div_dc = (1 << (d - 2) / 2);
      div_dc *= KfraIndex[FXPoint::kFracBits - 1];
    }
    d--;
    S_top = S_buffer[d + 1];
    S_bottom = S_buffer[d];
    N_top = S_top;
    N_bottom = S_bottom;
    N_parent = S_top;
    while (N_top > S_buffer[d + 2]) {
      if (nodeModeFlag[N_bottom - 1]) {
        N_top--;
        N_bottom--;
      } else {  // process all transform nodes in d-level
        FXPoint attributes222[3][2] = {0};
        N_bottom -= 2;
        N_top--;
        for (int kk = 0; kk < attribCount; kk++) {
          attributes222[kk][0] = attributeCoeff[(N_bottom + 1) * attribCount + kk];
          attributes222[kk][1] = attributeCoeff[N_bottom * attribCount + kk];
        }

        for (int kk = 0; kk < attribCount; kk++) {  // compute AC coefficient
          attributes222[kk][1] -= attributes222[kk][0];
          attributes222[kk][1] /= div_ac_neg;
          integerizedAttributes[N_top * attribCount + kk] =
            fwdQuantValue(attributes222[kk][1].round(), transQuantParam, offsetShift);
        }

        if (d == (MAXd - 1)) {
          for (int kk = 0; kk < attribCount; kk++) {  // process the root node
            attributesDC[kk] /= div_ac;
            integerizedAttributes[kk] = attributesDC[kk].round();
            integerizedAttributes[kk] =
              fwdQuantValue(integerizedAttributes[kk], transQuantParam, offsetShift);

            attributes222[kk][0] =
              invQuantValue(integerizedAttributes[kk], transQuantParam, offsetShift);
            attributes222[kk][0] *= div_ac;
          }
        } else {
          N_parent--;
          for (int kk = 0; kk < attribCount; kk++) {
            attributes222[kk][0] = recAttributesCoeff[N_parent * attribCount + kk];
          }
        }
        for (int kk = 0; kk < attribCount; kk++) {
          attributes222[kk][1] = invQuantValue(integerizedAttributes[N_top * attribCount + kk],
                                               transQuantParam, offsetShift);
          attributes222[kk][1] *= div_ac;
        }

        invTransform2Node(attribCount, attributes222);

        for (int kk = 0; kk < attribCount; kk++) {
          recAttributesCoeff[(N_bottom + 1) * attribCount + kk] = attributes222[kk][0];
          recAttributesCoeff[N_bottom * attribCount + kk] = attributes222[kk][1];
        }
      }
    }
    S_top = S_buffer[d] - S_buffer[d + 1];
    S_bottom = S_buffer[d];
    N_top = S_buffer[d + 1];
    N_bottom = S_bottom;
    while (N_top > S_buffer[d + 2]) {
      if (nodeModeFlag[N_bottom - 1]) {  // process all predict nodes in d-level
        N_top--;
        N_bottom--;
        FXPoint attributesPred[3] = {0};
        for (int kk = 0; kk < attribCount; kk++) {
          attributesPred[kk] = attributeCoeff[N_bottom * attribCount + kk];
        }
        if (attribCount == 1) {
          pred[0] =
            getReflectanceDCPredictor(N_bottom, recAttributesCoeff, positionsAllNodes, nodeModeFlag,
                                      S_bottom - 1, S_top, axisBias, aps.predFixedPointFracBit);
        } else {
          pred = getColorDCPredictor(N_bottom, recAttributesCoeff, positionsAllNodes, nodeModeFlag,
                                     S_bottom - 1, S_top, aps.colorQuantParam);
        }

        for (int kk = 0; kk < attribCount; kk++) {
          attributesPred[kk] -= pred[kk];
          attributesPred[kk] /= div_dc;
          integerizedAttributes[N_top * attribCount + kk] = attributesPred[kk].round();
          integerizedAttributes[N_top * attribCount + kk] = fwdQuantValue(
            integerizedAttributes[N_top * attribCount + kk], transQuantParam, offsetShift);
          recAttributesCoeff[N_bottom * attribCount + kk] = invQuantValue(
            integerizedAttributes[N_top * attribCount + kk], transQuantParam, offsetShift);
          recAttributesCoeff[N_bottom * attribCount + kk] *= div_dc;
          recAttributesCoeff[N_bottom * attribCount + kk] += pred[kk];
        }
      } else {
        N_bottom -= 2;
        N_top--;
      }
    }
  }

  for (int i = 0; i < voxelCount; i++) {
    for (int attri = 0; attri < attribCount; attri++)
      RecAttributes[i * attribCount + attri] =
        recAttributesCoeff[(S_bottom - i - 1) * attribCount + attri].round();
  }

  delete[] attributeCoeff;
  delete[] recAttributesCoeff;
  delete[] positionsAllNodes;
  delete[] nodeModeFlag;
}

template<typename T>
void WaveletCoreInverseTransform(FXPoint* attributes, const int attribCount, const int voxelCount,
                                 T* integerizedAttributes, const SequenceParameterSet& sps,
                                 const AttributeParameterSet& aps, const AttributeBrickHeader& abh,
                                 int* positions, const int disThInit) {
  size_t M, N;
  size_t d, j, i, S;

  int axisBias = aps.axisBias;
  int transQuantParam;

  int countPLow = 6, countTLow = 0;
  int countT = 0, countP = 0;

  if (attribCount == 3) {  // color default setting
    transQuantParam = aps.colorQuantParam;
    if (aps.transResLayer)
      transQuantParam += aps.attrTransQpDelta;
    countTLow = std::min(32, 1 << (transQuantParam / 8));
    countTLow = 2 * std::max(8, countTLow);
  } else {
    transQuantParam = aps.reflQuantParam + abh.QpOffset;
    if (aps.transResLayer)
      transQuantParam += aps.attrTransQpDelta;
    countTLow = std::min(32, 1 << (transQuantParam / 8));
    if (abh.reflInitPredTransRatio >= 8) {  // cat1A
      countTLow = 64 * std::max((int)8, countTLow);
    } else {  // cat2 cat1C
      countTLow = 1 * std::max((int)8, countTLow);
    }
  }

  // processing single pts
  if (voxelCount == 1) {
    int predValue = 0;
    ;
    if (attribCount == 3) {
      predValue = 128;
    }
    for (size_t k = 0; k < attribCount; k++) {
      attributes[k] = FXPoint(integerizedAttributes[k] + predValue);
    }
    return;
  }

  int64_t DistanceTH[32] = {0};
  DistanceTH[0] = disThInit;

  bool* nodeModeFlag = new bool[voxelCount * 2]();
  int* positionsAllNodes = new int[2 * voxelCount * 3]();
  FXPoint* recAttributesCoeff = new FXPoint[voxelCount * attribCount * 2]();

  int ii = 2 * voxelCount, lowBound = voxelCount, jj = 0;
  while (ii > lowBound) {
    ii--;
    for (int kk = 0; kk < 3; kk++) {
      positionsAllNodes[ii * 3 + kk] = positions[jj * 3 + kk];
    }
    jj++;
  }

  d = 0;  // 层数
  int distanceNodes = 0, distanceNodes_prev = 0, distanceNodes_prev_prev = 0, pairMode = 0;
  bool predictFlag = true;
  int idxNodeFlag, idxDCT;
  M = N = voxelCount;
  size_t S_buffer[32] = {};
  S_buffer[0] = 2 * voxelCount;
  //************************************* compute DC and pred flag bottom->top **************************
  while (N > 1) {
    S_buffer[d + 1] = M;
    idxNodeFlag = S_buffer[d];
    d++;    //当前层序号
    S = N;  //当前层节点数
    i = 0;  //attributes 当前节点
    M = 0;  //attributesTransformed 当前节点
    countP = 0, countT = 0;
    while (i < S) {
      idxDCT = S_buffer[d - 1] - i - 1;
      if (predictFlag) {
        distanceNodes = 0;
        for (int kk = 0; kk < 3; kk++) {
          int64_t tmp =
            (positionsAllNodes[idxDCT * 3 + kk] - positionsAllNodes[(idxDCT - 1) * 3 + kk]);
          distanceNodes += tmp * tmp;
        }
        if (countT > countTLow && DistanceTH[d - 1] != 1) {
          DistanceTH[d - 1] = std::max(1, (int)(DistanceTH[d - 1] / 2));
          countT = 0;
        } else if (countP > countPLow) {
          DistanceTH[d - 1] = DistanceTH[d - 1] * 2;
          countP = 0;
        }
      }
      if (distanceNodes > DistanceTH[d - 1] && predictFlag) {
        idxNodeFlag--;
        nodeModeFlag[idxNodeFlag] = true;
        i++;
        countP++;
        countT = 0;
      } else {
        if (distanceNodes > 0) {
          countT = countT + 2;
          countP = 0;
        }
        N--;
        for (int kk = 0; kk < 3; kk++) {  // compute parent node coordinates
          positionsAllNodes[N * 3 + kk] =
            (positionsAllNodes[idxDCT * 3 + kk] + positionsAllNodes[(idxDCT - 1) * 3 + kk] + 1) / 2;
        }
        M++;
        idxNodeFlag -= 2;
        i += 2;
      }
      if (i == S - 1) {
        idxNodeFlag--;
        nodeModeFlag[idxNodeFlag] = true;
        i++;
      }
    }
    N = M;

    if (M < S_buffer[d] / 2 && M > 128)
      DistanceTH[d] =
        std::max({(int64_t)1, (int64_t)((double)DistanceTH[d - 1] * S_buffer[d] / M)});
    else
      predictFlag = false;
  }

  //************************************* compute AC and residue top->bottom **************************
  int S_top, S_bottom, N_top_AC, N_bottom, N_top_DC;

  for (int kk = 0; kk < attribCount; kk++)
    recAttributesCoeff[attribCount * (S_buffer[d] - 1) + kk] = integerizedAttributes[kk];
  S_buffer[d + 1] = M;
  int MAXd = d;
  V3<FXPoint> pred;
  while (d > 0) {
    FXPoint div_ac, div_dc;
    if (d & 1) {
      div_ac = (1 << (d - 1) / 2);
      div_ac *= KfraIndex[FXPoint::kFracBits - 1];
      div_dc = 1 << (d - 1) / 2;
    } else {
      div_ac = 1 << d / 2;
      div_dc = (1 << (d - 2) / 2);
      div_dc *= KfraIndex[FXPoint::kFracBits - 1];
    }
    d--;
    S_top = S_buffer[d + 1];
    S_bottom = S_buffer[d];
    N_top_AC = S_top;
    N_bottom = S_bottom;
    N_top_DC = S_top;
    //FLAG = (S_bottom - S_top) % 2;
    while (N_top_AC > S_buffer[d + 2]) {
      if (nodeModeFlag[N_bottom - 1]) {
        N_top_AC--;
        N_bottom--;
      } else {
        FXPoint attributes222[3][2] = {0};
        N_bottom -= 2;
        N_top_AC--;
        N_top_DC--;

        for (int kk = 0; kk < attribCount; kk++) {
          attributes222[kk][0] = recAttributesCoeff[N_top_DC * attribCount + kk];
          attributes222[kk][1] = integerizedAttributes[N_top_AC * attribCount + kk];
          attributes222[kk][1] *= div_ac;
        }

        if (d == (MAXd - 1)) {
          for (int kk = 0; kk < attribCount; kk++) {
            attributes222[kk][0] *= div_ac;
          }
        }

        invTransform2Node(attribCount, attributes222);
        for (int kk = 0; kk < attribCount; kk++) {
          recAttributesCoeff[(N_bottom + 1) * attribCount + kk] = attributes222[kk][0];
          recAttributesCoeff[N_bottom * attribCount + kk] = attributes222[kk][1];
        }
      }
    }

    S_top = S_buffer[d] - S_buffer[d + 1];
    S_bottom = S_buffer[d];
    N_top_AC = S_buffer[d + 1];
    N_bottom = S_bottom;
    while (N_top_AC > S_buffer[d + 2]) {
      if (nodeModeFlag[N_bottom - 1]) {
        N_top_AC--;
        N_bottom--;

        if (attribCount == 1) {
          pred[0] =
            getReflectanceDCPredictor(N_bottom, recAttributesCoeff, positionsAllNodes, nodeModeFlag,
                                      S_bottom - 1, S_top, axisBias, aps.predFixedPointFracBit);
        } else {
          pred = getColorDCPredictor(N_bottom, recAttributesCoeff, positionsAllNodes, nodeModeFlag,
                                     S_bottom - 1, S_top, aps.colorQuantParam);
        }

        for (int kk = 0; kk < attribCount; kk++) {
          recAttributesCoeff[N_bottom * attribCount + kk] =
            integerizedAttributes[N_top_AC * attribCount + kk];
          recAttributesCoeff[N_bottom * attribCount + kk] *= div_dc;
          recAttributesCoeff[N_bottom * attribCount + kk] += pred[kk];
        }
      } else {
        N_bottom -= 2;
        N_top_AC--;
      }
    }
  }

  ii = 2 * voxelCount, lowBound = voxelCount, jj = 0;
  while (ii > lowBound) {
    ii--;
    for (int kk = 0; kk < attribCount; kk++)
      attributes[jj * attribCount + kk] = recAttributesCoeff[ii * attribCount + kk];
    jj++;
  }

  delete[] nodeModeFlag;
  delete[] recAttributesCoeff;
  delete[] positionsAllNodes;
}

template<typename T>
void WaveletCoreTransform(FXPoint* attributes, const int attribCount, const int voxelCount,
                          T* integerizedAttributes, const SequenceParameterSet& sps,
                          const AttributeParameterSet& aps, const AttributeBrickHeader& abh,
                          int* positions, int* RecAttributes, const int disThInit,
                          int* refAttributes, const int refAttribCount) {
  size_t M, N;
  size_t d, j, i, S;

  int offsetShift = aps.deadZoneLen == 0 ? 1 : aps.deadZoneLen * 0.1 * (1 << encoderShiftBit);
  int axisBias = aps.axisBias;
  int transQuantParam;

  int countPLow = 6, countTLow = 0;
  int countT = 0, countP = 0;

  if (attribCount == 3) {  // color default setting
    offsetShift = 0.2 * (1 << encoderShiftBit);
    transQuantParam = aps.colorQuantParam;
    if (aps.transResLayer)
      transQuantParam += aps.attrTransQpDelta;
    countTLow = std::min(32, 1 << (transQuantParam / 8));
    countTLow = 2 * std::max(8, countTLow);
  } else {
    transQuantParam = aps.reflQuantParam + abh.QpOffset;
    if (aps.transResLayer)
      transQuantParam += aps.attrTransQpDelta;
    countTLow = std::min(32, 1 << (transQuantParam / 8));
    if (abh.reflInitPredTransRatio >= 8) {  // cat1A
      countTLow = 64 * std::max((int)8, countTLow);
    } else {  // cat2 cat1C
      countTLow = 1 * std::max((int)8, countTLow);
    }
  }

  // processing single pts
  if (voxelCount == 1) {
    int predValue = 0;
    if (attribCount == 3) {
      predValue = 128;
    }
    for (size_t k = 0; k < attribCount; k++) {
      int64_t delta = attributes[k].round() - predValue;
      int residualSign = delta < 0 ? -1 : 1;
      uint64_t absResidual = std::abs(delta);
      uint64_t residualQuant;
      residualQuant = QuantizaResidual(absResidual, transQuantParam, offsetShift);
      int64_t signResidualQuant = residualSign * (int64_t)residualQuant;
      integerizedAttributes[k] = signResidualQuant;
      uint64_t inverseResidualQuant = InverseQuantizeResidual(residualQuant, transQuantParam);
      int64_t recResidual = inverseResidualQuant * residualSign;
      RecAttributes[k] = recResidual + predValue;
    }
    return;
  }

  int64_t DistanceTH[32] = {0};
  DistanceTH[0] = disThInit;
  FXPoint* attributeCoeff = new FXPoint[voxelCount * attribCount * 2]();  // transform coefficients
  FXPoint* recAttributesCoeff =
    new FXPoint[voxelCount * attribCount * 2]();           // reconstructed attributes
  bool* nodeModeFlag = new bool[voxelCount * 2]();         // transform/predict node flag
  int* positionsAllNodes = new int[2 * voxelCount * 3]();  // position coordinates
  size_t S_buffer[32] = {};
  int* refAttributesAllNodes = new int[2 * voxelCount * refAttribCount]();

  int ii = 2 * voxelCount, lowBound = voxelCount, jj = 0;
  while (ii > lowBound) {
    ii--;
    for (int kk = 0; kk < attribCount; kk++)
      attributeCoeff[ii * attribCount + kk] = attributes[jj * attribCount + kk];
    for (int kk = 0; kk < 3; kk++)
      positionsAllNodes[ii * 3 + kk] = positions[jj * 3 + kk];
    for (int kk = 0; kk < refAttribCount; kk++)
      refAttributesAllNodes[ii * refAttribCount + kk] = refAttributes[jj * refAttribCount + kk];
    jj++;
  }

  d = 0;  // 层数
  int distanceNodes = 0, distanceNodes_prev = 0, distanceNodes_prev_prev = 0, pairMode = 0;
  bool predictFlag = true;
  int idxNodeFlag, idxDCT;
  M = N = voxelCount;
  S_buffer[0] = 2 * N;
  //************************************* compute DC and pred flag bottom->top **************************
  while (N > 1) {
    S_buffer[d + 1] = M;
    idxNodeFlag = S_buffer[d];
    d++;    //当前层序号
    S = N;  //当前层节点数
    i = 0;  //attributes 当前节点
    M = 0;  //attributesTransformed 当前节点
    countP = 0, countT = 0;
    while (i < S) {
      idxDCT = S_buffer[d - 1] - i - 1;
      if (predictFlag) {
        distanceNodes = 0;
        for (int kk = 0; kk < refAttribCount; kk++) {
          int64_t tmp = abs(refAttributesAllNodes[idxDCT * refAttribCount + kk] -
                            refAttributesAllNodes[(idxDCT - 1) * refAttribCount + kk]) >>
            10;
          distanceNodes += tmp * tmp;
        }
        for (int kk = 0; kk < 3; kk++) {
          int64_t tmp =
            (positionsAllNodes[idxDCT * 3 + kk] - positionsAllNodes[(idxDCT - 1) * 3 + kk]);
          distanceNodes += tmp * tmp;
        }
        if (countT > countTLow && DistanceTH[d - 1] != 1) {
          DistanceTH[d - 1] = std::max(1, (int)(DistanceTH[d - 1] / 2));
          countT = 0;
        } else if (countP > countPLow) {
          DistanceTH[d - 1] = DistanceTH[d - 1] * 2;
          countP = 0;
        }
      }
      if ((distanceNodes > DistanceTH[d - 1]) && predictFlag) {  // pred node
        idxNodeFlag--;
        nodeModeFlag[idxNodeFlag] = true;
        i++;
        countP++;
        countT = 0;
      } else {  // if distanceNodes <= th
        if (distanceNodes > 0) {
          countT = countT + 2;
          countP = 0;
        }
        FXPoint attributes222[3][2] = {};
        for (int k = 0; k < attribCount; k++) {
          attributes222[k][0] = attributeCoeff[idxDCT * attribCount + k];
          attributes222[k][1] = attributeCoeff[(idxDCT - 1) * attribCount + k];
        }
        for (int k = 0; k < attribCount; k++) {  // compute DC coefficient
          attributes222[k][0] += attributes222[k][1];
        }
        N--;
        for (int kk = 0; kk < attribCount; kk++) {
          attributeCoeff[N * attribCount + kk] = attributes222[kk][0];
        }
        for (int kk = 0; kk < 3; kk++) {  // compute parent node coordinates
          positionsAllNodes[N * 3 + kk] =
            (positionsAllNodes[idxDCT * 3 + kk] + positionsAllNodes[(idxDCT - 1) * 3 + kk] + 1) / 2;
        }
        for (int kk = 0; kk < refAttribCount; kk++) {  // compute parent node reference attributes
          refAttributesAllNodes[N * refAttribCount + kk] =
            (refAttributesAllNodes[idxDCT * refAttribCount + kk] +
             refAttributesAllNodes[(idxDCT - 1) * refAttribCount + kk] + 1) /
            2;
        }
        M++;
        idxNodeFlag -= 2;
        i += 2;
      }
      if (i == S - 1) {
        idxNodeFlag--;
        nodeModeFlag[idxNodeFlag] = true;
        i++;
      }
    }
    N = M;

    if (
      M < S_buffer[d] / 2 &&
      M >
        128)  // M: 当前层父层的节点数（d+1 层）；如果M大于128或者当前层具有预测节点，则进行距离阈值的更新
      DistanceTH[d] =
        std::max({(int64_t)1, (int64_t)((double)DistanceTH[d - 1] * S_buffer[d] / M)});
    else
      predictFlag = false;  // 当前层父层（d+1 层），不进行距离阈值判断，全部为变换节点
  }
  FXPoint attributesDC[3] = {};
  for (size_t k = 0; k < attribCount; k++) {
    attributesDC[k] = attributeCoeff[(S_buffer[d] - 1) * attribCount + k];
  }
  //************************************* compute AC and residue top->bottom **************************
  int S_top, S_bottom, N_top, N_bottom, N_parent;

  S_buffer[d + 1] = M;
  int MAXd = d;
  V3<FXPoint> pred;
  while (d > 0) {
    FXPoint div_ac, div_dc, div_ac_neg;
    if (d & 1) {
      div_ac = 1 << (d - 1) / 2;
      div_ac *= KfraIndex[FXPoint::kFracBits - 1];
      div_ac_neg = 1 << (d - 1) / 2;
      div_ac_neg *= -KfraIndex[FXPoint::kFracBits - 1];
      div_dc = 1 << (d - 1) / 2;
    } else {
      div_ac = 1 << d / 2;
      div_ac_neg = -(1 << d / 2);
      div_dc = (1 << (d - 2) / 2);
      div_dc *= KfraIndex[FXPoint::kFracBits - 1];
    }
    d--;
    S_top = S_buffer[d + 1];
    S_bottom = S_buffer[d];
    N_top = S_top;
    N_bottom = S_bottom;
    N_parent = S_top;
    while (N_top > S_buffer[d + 2]) {
      if (nodeModeFlag[N_bottom - 1]) {
        N_top--;
        N_bottom--;
      } else {  // process all transform nodes in d-level
        FXPoint attributes222[3][2] = {0};
        N_bottom -= 2;
        N_top--;
        for (int kk = 0; kk < attribCount; kk++) {
          attributes222[kk][0] = attributeCoeff[(N_bottom + 1) * attribCount + kk];
          attributes222[kk][1] = attributeCoeff[N_bottom * attribCount + kk];
        }

        for (int kk = 0; kk < attribCount; kk++) {  // compute AC coefficient
          attributes222[kk][1] -= attributes222[kk][0];
          attributes222[kk][1] /= div_ac_neg;
          integerizedAttributes[N_top * attribCount + kk] =
            fwdQuantValue(attributes222[kk][1].round(), transQuantParam, offsetShift);
        }

        if (d == (MAXd - 1)) {
          for (int kk = 0; kk < attribCount; kk++) {  // process the root node
            attributesDC[kk] /= div_ac;
            integerizedAttributes[kk] = attributesDC[kk].round();
            integerizedAttributes[kk] =
              fwdQuantValue(integerizedAttributes[kk], transQuantParam, offsetShift);
            attributes222[kk][0] = invQuantValue(integerizedAttributes[kk], transQuantParam, 0);
            attributes222[kk][0] *= div_ac;
          }
        } else {
          N_parent--;
          for (int kk = 0; kk < attribCount; kk++) {
            attributes222[kk][0] = recAttributesCoeff[N_parent * attribCount + kk];
          }
        }
        for (int kk = 0; kk < attribCount; kk++) {
          attributes222[kk][1] = invQuantValue(integerizedAttributes[N_top * attribCount + kk],
                                               transQuantParam, offsetShift);
          attributes222[kk][1] *= div_ac;
        }

        invTransform2Node(attribCount, attributes222);

        for (int kk = 0; kk < attribCount; kk++) {
          recAttributesCoeff[(N_bottom + 1) * attribCount + kk] = attributes222[kk][0];
          recAttributesCoeff[N_bottom * attribCount + kk] = attributes222[kk][1];
        }
      }
    }
    S_top = S_buffer[d] - S_buffer[d + 1];
    S_bottom = S_buffer[d];
    N_top = S_buffer[d + 1];
    N_bottom = S_bottom;
    while (N_top > S_buffer[d + 2]) {
      if (nodeModeFlag[N_bottom - 1]) {  // process all predict nodes in d-level
        N_top--;
        N_bottom--;
        FXPoint attributesPred[3] = {0};
        for (int kk = 0; kk < attribCount; kk++) {
          attributesPred[kk] = attributeCoeff[N_bottom * attribCount + kk];
        }
        if (attribCount == 1) {
          pred[0] = getReflectanceDCPredictorFromColor(
            N_bottom, recAttributesCoeff, positionsAllNodes, refAttributesAllNodes, nodeModeFlag,
            S_bottom - 1, S_top, axisBias, aps.predFixedPointFracBit);

        } else {
          pred = getColorDCPredictorFromRefl(N_bottom, recAttributesCoeff, positionsAllNodes,
                                             refAttributesAllNodes, nodeModeFlag, S_bottom - 1,
                                             S_top, aps.colorQuantParam);
        }

        for (int kk = 0; kk < attribCount; kk++) {
          attributesPred[kk] -= pred[kk];
          attributesPred[kk] /= div_dc;
          integerizedAttributes[N_top * attribCount + kk] = attributesPred[kk].round();
          integerizedAttributes[N_top * attribCount + kk] = fwdQuantValue(
            integerizedAttributes[N_top * attribCount + kk], transQuantParam, offsetShift);
          recAttributesCoeff[N_bottom * attribCount + kk] = invQuantValue(
            integerizedAttributes[N_top * attribCount + kk], transQuantParam, offsetShift);
          recAttributesCoeff[N_bottom * attribCount + kk] *= div_dc;
          recAttributesCoeff[N_bottom * attribCount + kk] += pred[kk];
        }
      } else {
        N_bottom -= 2;
        N_top--;
      }
    }
  }

  for (int i = 0; i < voxelCount; i++) {
    for (int attri = 0; attri < attribCount; attri++)
      RecAttributes[i * attribCount + attri] =
        recAttributesCoeff[(S_bottom - i - 1) * attribCount + attri].round();
  }

  delete[] attributeCoeff;
  delete[] recAttributesCoeff;
  delete[] positionsAllNodes;
  delete[] nodeModeFlag;
  delete[] refAttributesAllNodes;
}

template<typename T>
void WaveletCoreInverseTransform(FXPoint* attributes, const int attribCount, const int voxelCount,
                                 T* integerizedAttributes, const SequenceParameterSet& sps,
                                 const AttributeParameterSet& aps, const AttributeBrickHeader& abh,
                                 int* positions, const int disThInit, int* refAttributes,
                                 const int refAttribCount) {
  size_t M, N;
  size_t d, j, i, S;

  int axisBias = aps.axisBias;
  int transQuantParam;

  int countPLow = 6, countTLow = 0;
  int countT = 0, countP = 0;

  if (attribCount == 3) {  // color default setting
    transQuantParam = aps.colorQuantParam;
    if (aps.transResLayer)
      transQuantParam += aps.attrTransQpDelta;
    countTLow = std::min(32, 1 << (transQuantParam / 8));
    countTLow = 2 * std::max(8, countTLow);
  } else {
    transQuantParam = aps.reflQuantParam + abh.QpOffset;
    if (aps.transResLayer)
      transQuantParam += aps.attrTransQpDelta;
    countTLow = std::min(32, 1 << (transQuantParam / 8));
    if (abh.reflInitPredTransRatio >= 8) {  // cat1A
      countTLow = 64 * std::max((int)8, countTLow);
    } else {  // cat2 cat1C
      countTLow = 1 * std::max((int)8, countTLow);
    }
  }

  // processing single pts
  if (voxelCount == 1) {
    int predValue = 0;
    ;
    if (attribCount == 3) {
      predValue = 128;
    }
    for (size_t k = 0; k < attribCount; k++) {
      attributes[k] = FXPoint(integerizedAttributes[k] + predValue);
    }
    return;
  }

  int64_t DistanceTH[32] = {0};
  DistanceTH[0] = disThInit;

  bool* nodeModeFlag = new bool[voxelCount * 2]();
  int* positionsAllNodes = new int[2 * voxelCount * 3]();
  FXPoint* recAttributesCoeff = new FXPoint[voxelCount * attribCount * 2]();
  int* refAttributesAllNodes = new int[2 * voxelCount * refAttribCount]();

  int ii = 2 * voxelCount, lowBound = voxelCount, jj = 0;
  while (ii > lowBound) {
    ii--;
    for (int kk = 0; kk < 3; kk++)
      positionsAllNodes[ii * 3 + kk] = positions[jj * 3 + kk];
    for (int kk = 0; kk < refAttribCount; kk++)
      refAttributesAllNodes[ii * refAttribCount + kk] = refAttributes[jj * refAttribCount + kk];
    jj++;
  }

  d = 0;  // 层数
  int distanceNodes = 0, distanceNodes_prev = 0, distanceNodes_prev_prev = 0, pairMode = 0;
  bool predictFlag = true;
  int idxNodeFlag, idxDCT;
  M = N = voxelCount;
  size_t S_buffer[32] = {};
  S_buffer[0] = 2 * voxelCount;
  //************************************* compute DC and pred flag bottom->top **************************
  while (N > 1) {
    S_buffer[d + 1] = M;
    idxNodeFlag = S_buffer[d];
    d++;    //当前层序号
    S = N;  //当前层节点数
    i = 0;  //attributes 当前节点
    M = 0;  //attributesTransformed 当前节点
    countP = 0, countT = 0;
    while (i < S) {
      idxDCT = S_buffer[d - 1] - i - 1;
      if (predictFlag) {
        distanceNodes = 0;
        for (int kk = 0; kk < refAttribCount; kk++) {
          int64_t tmp = abs(refAttributesAllNodes[idxDCT * refAttribCount + kk] -
                            refAttributesAllNodes[(idxDCT - 1) * refAttribCount + kk]) >>
            10;
          distanceNodes += tmp * tmp;
        }
        for (int kk = 0; kk < 3; kk++) {
          int64_t tmp =
            (positionsAllNodes[idxDCT * 3 + kk] - positionsAllNodes[(idxDCT - 1) * 3 + kk]);
          distanceNodes += tmp * tmp;
        }
        if (countT > countTLow && DistanceTH[d - 1] != 1) {
          DistanceTH[d - 1] = std::max(1, (int)(DistanceTH[d - 1] / 2));
          countT = 0;
        } else if (countP > countPLow) {
          DistanceTH[d - 1] = DistanceTH[d - 1] * 2;
          countP = 0;
        }
      }
      if (distanceNodes > DistanceTH[d - 1] && predictFlag) {
        idxNodeFlag--;
        nodeModeFlag[idxNodeFlag] = true;
        i++;
        countP++;
        countT = 0;
      } else {
        if (distanceNodes > 0) {
          countT = countT + 2;
          countP = 0;
        }
        N--;
        for (int kk = 0; kk < 3; kk++) {  // compute parent node coordinates
          positionsAllNodes[N * 3 + kk] =
            (positionsAllNodes[idxDCT * 3 + kk] + positionsAllNodes[(idxDCT - 1) * 3 + kk] + 1) / 2;
        }
        for (int kk = 0; kk < refAttribCount; kk++) {  // compute parent node reference attributes
          refAttributesAllNodes[N * refAttribCount + kk] =
            (refAttributesAllNodes[idxDCT * refAttribCount + kk] +
             refAttributesAllNodes[(idxDCT - 1) * refAttribCount + kk] + 1) /
            2;
        }
        M++;
        idxNodeFlag -= 2;
        i += 2;
      }
      if (i == S - 1) {
        idxNodeFlag--;
        nodeModeFlag[idxNodeFlag] = true;
        i++;
      }
    }
    N = M;

    if (M < S_buffer[d] / 2 && M > 128)
      DistanceTH[d] =
        std::max({(int64_t)1, (int64_t)((double)DistanceTH[d - 1] * S_buffer[d] / M)});
    else
      predictFlag = false;
  }

  //************************************* compute AC and residue top->bottom **************************
  int S_top, S_bottom, N_top_AC, N_bottom, N_top_DC;

  for (int kk = 0; kk < attribCount; kk++)
    recAttributesCoeff[attribCount * (S_buffer[d] - 1) + kk] = integerizedAttributes[kk];
  S_buffer[d + 1] = M;
  int MAXd = d;
  V3<FXPoint> pred;
  while (d > 0) {
    FXPoint div_ac, div_dc;
    if (d & 1) {
      div_ac = (1 << (d - 1) / 2);
      div_ac *= KfraIndex[FXPoint::kFracBits - 1];
      div_dc = 1 << (d - 1) / 2;
    } else {
      div_ac = 1 << d / 2;
      div_dc = (1 << (d - 2) / 2);
      div_dc *= KfraIndex[FXPoint::kFracBits - 1];
    }
    d--;
    S_top = S_buffer[d + 1];
    S_bottom = S_buffer[d];
    N_top_AC = S_top;
    N_bottom = S_bottom;
    N_top_DC = S_top;
    //FLAG = (S_bottom - S_top) % 2;
    while (N_top_AC > S_buffer[d + 2]) {
      if (nodeModeFlag[N_bottom - 1]) {
        N_top_AC--;
        N_bottom--;
      } else {
        FXPoint attributes222[3][2] = {0};
        N_bottom -= 2;
        N_top_AC--;
        N_top_DC--;

        for (int kk = 0; kk < attribCount; kk++) {
          attributes222[kk][0] = recAttributesCoeff[N_top_DC * attribCount + kk];
          attributes222[kk][1] = integerizedAttributes[N_top_AC * attribCount + kk];
          attributes222[kk][1] *= div_ac;
        }
        if (d == (MAXd - 1)) {
          for (int kk = 0; kk < attribCount; kk++) {
            attributes222[kk][0] *= div_ac;
          }
        }

        invTransform2Node(attribCount, attributes222);
        for (int kk = 0; kk < attribCount; kk++) {
          recAttributesCoeff[(N_bottom + 1) * attribCount + kk] = attributes222[kk][0];
          recAttributesCoeff[N_bottom * attribCount + kk] = attributes222[kk][1];
        }
      }
    }

    S_top = S_buffer[d] - S_buffer[d + 1];
    S_bottom = S_buffer[d];
    N_top_AC = S_buffer[d + 1];
    N_bottom = S_bottom;
    while (N_top_AC > S_buffer[d + 2]) {
      if (nodeModeFlag[N_bottom - 1]) {
        N_top_AC--;
        N_bottom--;

        if (attribCount == 1) {
          pred[0] = getReflectanceDCPredictorFromColor(
            N_bottom, recAttributesCoeff, positionsAllNodes, refAttributesAllNodes, nodeModeFlag,
            S_bottom - 1, S_top, axisBias, aps.predFixedPointFracBit);
        } else {
          pred = getColorDCPredictorFromRefl(N_bottom, recAttributesCoeff, positionsAllNodes,
                                             refAttributesAllNodes, nodeModeFlag, S_bottom - 1,
                                             S_top, aps.colorQuantParam);
        }

        for (int kk = 0; kk < attribCount; kk++) {
          recAttributesCoeff[N_bottom * attribCount + kk] =
            integerizedAttributes[N_top_AC * attribCount + kk];
          recAttributesCoeff[N_bottom * attribCount + kk] *= div_dc;
          recAttributesCoeff[N_bottom * attribCount + kk] += pred[kk];
        }
      } else {
        N_bottom -= 2;
        N_top_AC--;
      }
    }
  }

  ii = 2 * voxelCount, lowBound = voxelCount, jj = 0;
  while (ii > lowBound) {
    ii--;
    for (int kk = 0; kk < attribCount; kk++)
      attributes[jj * attribCount + kk] = recAttributesCoeff[ii * attribCount + kk];
    jj++;
  }

  delete[] nodeModeFlag;
  delete[] recAttributesCoeff;
  delete[] positionsAllNodes;
}
//============================================================================================
Void getLength(std::vector<pointCodeWithIndex>& pointCloudHilbert, vector<int>& length,
               vector<int>& numofGroupCount, int maxNumofCoeff, int& ShiftBits, UInt MaxTransNum) {
  auto voxelCount = pointCloudHilbert.size();
  if (MaxTransNum > 1) {
    int64_t cur_ordercode = 0;
    int countL = 0;
    queue<int> numPts;
    int maxQueue = 3, checkLen = 32;  /// adaptive adjust Pos_Shift
    int sumNum = 0;
    int groupIndex = 0;
    int64_t lastcode = -1;
    bool isduplicate = false;
    int totalNum = 0;
    for (int curIndex = 0; curIndex < voxelCount;) {
      if (groupIndex % checkLen == 0 && groupIndex > 0) {
        auto aveNum = sumNum >> maxQueue;
        if (aveNum < 2)
          ShiftBits += 1;
        else if (aveNum > 8)
          ShiftBits -= 1;
      }
      ShiftBits = std::max(3, ShiftBits);

      cur_ordercode = pointCloudHilbert[curIndex].code >> ShiftBits;
      countL = 0;

      //处理重复点
      if (((curIndex + countL) < (voxelCount)) &&
          pointCloudHilbert[curIndex + countL].code == lastcode) {
        lastcode = pointCloudHilbert[curIndex + countL].code;
        countL++;
        isduplicate = true;
        while (((curIndex + countL) < (voxelCount) &&
                lastcode == pointCloudHilbert[curIndex + countL].code)) {
          countL++;
        }
      } else {
        lastcode = pointCloudHilbert[curIndex + countL].code;
        countL++;
        while (((curIndex + countL) < (voxelCount) &&
                cur_ordercode == pointCloudHilbert[curIndex + countL].code >> ShiftBits)) {
          //重复点
          isduplicate = false;
          if (pointCloudHilbert[curIndex + countL].code == lastcode) {
            break;
          } else {
            lastcode = pointCloudHilbert[curIndex + countL].code;
            countL++;
          }
        }
      }
      lastcode = pointCloudHilbert[curIndex + countL - 1].code;
      lengthDivide(pointCloudHilbert, length, countL, MaxTransNum, maxNumofCoeff, numofGroupCount,
                   totalNum, curIndex, ShiftBits, isduplicate);
      isduplicate = false;
      numPts.push(countL);
      sumNum += countL;
      if (numPts.size() > maxQueue) {
        sumNum -= numPts.front();
        numPts.pop();
      }
      groupIndex++;
      curIndex += countL;
    }
    if (maxNumofCoeff > MaxTransNum)
      numofGroupCount.push_back(length.size());
  } else {
    int num = voxelCount / maxNumofCoeff;
    length.resize(voxelCount, 1);
    numofGroupCount.resize(num + 1);
    if (maxNumofCoeff < voxelCount) {
      numofGroupCount[0] = maxNumofCoeff;
      for (int i = 1; i < num; i++) {
        numofGroupCount[i] = maxNumofCoeff + numofGroupCount[i - 1];
      }
      numofGroupCount[num] = voxelCount;
    } else {
      numofGroupCount[0] = voxelCount;
    }
  }
}

Void getLengthRef(const Int& shift, std::vector<pointCodeWithIndex>& pointCloudHilbert,
                  vector<int>& length, vector<int>& numofGroupCount, int maxNumofCoeff,
                  int& ShiftBits, const UInt& MaxTransNum, const bool isMemControl) {
  auto voxelCount = pointCloudHilbert.size();
  if (MaxTransNum > 1) {
    int64_t cur_ordercode = 0;
    int countL = 0;
    queue<int> numPts;
    int maxQueue = 8, checkLen = 8;  /// adaptive adjust Pos_Shift
    int sumNum = 0;
    int groupIndex = 0;
    int64_t lastcode = -1;
    bool isduplicate = false;
    int totalNum = 0;
    for (int curIndex = 0; curIndex < voxelCount;) {
      if (MaxTransNum > 2 && shift > 0 && groupIndex % checkLen == 0 && groupIndex > 0) {
        auto aveNum = sumNum / maxQueue;
        if (aveNum < 2)
          ShiftBits += 1;
        else if (aveNum > MaxTransNum)
          ShiftBits -= 1;
      }
      ShiftBits = std::max(1, ShiftBits);

      cur_ordercode = pointCloudHilbert[curIndex].code >> ShiftBits;
      countL = 0;

      //处理重复点
      if (((curIndex + countL) < (voxelCount)) &&
          pointCloudHilbert[curIndex + countL].code == lastcode) {
        lastcode = pointCloudHilbert[curIndex + countL].code;
        countL++;
        isduplicate = true;
        while (((curIndex + countL) < (voxelCount) &&
                lastcode == pointCloudHilbert[curIndex + countL].code)) {
          countL++;
        }
      } else {
        lastcode = pointCloudHilbert[curIndex + countL].code;
        countL++;
        while (((curIndex + countL) < (voxelCount) &&
                cur_ordercode == pointCloudHilbert[curIndex + countL].code >> ShiftBits)) {
          //重复点
          isduplicate = false;
          if (pointCloudHilbert[curIndex + countL].code == lastcode) {
            break;
          } else {
            lastcode = pointCloudHilbert[curIndex + countL].code;
            countL++;
          }
        }
      }
      lastcode = pointCloudHilbert[curIndex + countL - 1].code;
      lengthDivide(pointCloudHilbert, length, countL, MaxTransNum, maxNumofCoeff, numofGroupCount,
                   totalNum, curIndex, ShiftBits, isduplicate);
      isduplicate = false;
      if (MaxTransNum > 2) {
        numPts.push(countL);
        sumNum += countL;
        if (numPts.size() > maxQueue) {
          sumNum -= numPts.front();
          numPts.pop();
        }
      }
      groupIndex++;
      curIndex += countL;
    }
    if (maxNumofCoeff > MaxTransNum)
      numofGroupCount.push_back(length.size());
  } else {
    int num = voxelCount / maxNumofCoeff;
    length.resize(voxelCount, 1);
    numofGroupCount.resize(num + 1);
    if (maxNumofCoeff < voxelCount) {
      numofGroupCount[0] = maxNumofCoeff;
      for (int i = 1; i < num; i++) {
        numofGroupCount[i] = maxNumofCoeff + numofGroupCount[i - 1];
      }
      numofGroupCount[num] = voxelCount;
    } else {
      numofGroupCount[0] = voxelCount;
    }
  }
}

void lengthDivide(std::vector<pointCodeWithIndex>& pointCloudHilbert, vector<int>& length, int num,
                  int MaxTransNum, int maxNumofCoeff, vector<int>& numofGroupCount, int& totalNum,
                  int curIdx, int shiftBits, bool isduplicate) {
  int count = 0, countL = 0;
  int NumOfSubGroup = 0;
  int shiftBits2 = shiftBits;
  int64_t cur_ordercode = 0;
  std::stack<std::tuple<int, int, int>> numLength;
  numLength.push(std::make_tuple(curIdx, num, shiftBits));

  while (!numLength.empty()) {
    auto cur_length = numLength.top();
    int idx = get<0>(cur_length);
    int len = get<1>(cur_length);
    int shiftBits2 = get<2>(cur_length);

    numLength.pop();

    if (!isduplicate) {
      if (len <= MaxTransNum) {
        length.push_back(len);
        if (maxNumofCoeff > MaxTransNum) {
          totalNum += len;
          if (totalNum > maxNumofCoeff) {
            numofGroupCount.push_back(length.size() - 1);
            totalNum = len;
          }
        } else {
          numofGroupCount.push_back(length.size());
        }
      } else if (len >= MaxTransNum) {
        shiftBits2 = max(1, shiftBits2 - 1);

        int idx1 = idx + len;
        while (len > 0) {
          cur_ordercode = pointCloudHilbert[idx1 - 1].code >> shiftBits2;
          countL = 1;
          while (idx1 - 2 >= curIdx &&
                 cur_ordercode == pointCloudHilbert[idx1 - 2].code >> shiftBits2) {
            countL++;
            idx1--;
          }
          numLength.push(std::make_tuple(--idx1, countL, shiftBits2));
          len -= countL;
        }
      }
    } else {
      while (len > 0) {
        length.push_back(1);
        len--;
        if (maxNumofCoeff > MaxTransNum) {
          totalNum++;
          if (totalNum > maxNumofCoeff) {
            numofGroupCount.push_back(length.size() - 1);
            totalNum = 1;
          }
        } else {
          numofGroupCount.push_back(length.size());
        }
      }
    }
  }
}
//============================================================================
// clang-format off
int matrixColor_B1[1][1] = {512};

int matrixColor_B2[2][2] = {{362, 362}, {362, -362}};

int matrixColor_B3[3][3] = {{296, 296, 296}, {362, 0, -362}, {209, -418, 209}};

int matrixColor_B4[4][4] = {
  {256, 256, 256, 256}, {256, 256, -256, -256}, {256, -256, -256, 256}, {256, -256, 256, -256}};

int matrixColor_B5[5][5] = {{229, 229, 229, 229, 229},
                            {308, 190, 0, -190, -308},
                            {262, -100, -324, -100, 262},
                            {190, -308, 0, 308, -190},
                            {100, -262, 324, -262, 100}};

int matrixColor_B6[6][6] = {{209, 209, 209, 209, 209, 209},
                            {286, 209, 77, -77, -209,-286,},
                            {256, 0, -256, -256, 0, 256},
                            {209, -209, -209, 209, 209, -209},
                            {148, -296, 148, 148, -296, 148},
                            {77, -209, 286, -286, 209, -77}};

int matrixColor_B7[7][7] = {
  {194, 194, 194, 194, 194, 194, 194},   {267, 214, 119, 0, -119, -214, -267},
  {247, 61, -171, -274, -171, 61, 247},  {214, -119, -267, 0, 267, 119, -214},
  {171, -247, -61, 274, -61, -247, 171}, {119, -267, 214, 0, -214, 267, -119},
  {61, -171, 247, -274, 247, -171, 61}};

int matrixColor_B8[8][8] = 
   {{181, 181, 181, 181, 181, 181, 181, 181,},
    {251, 213, 142, 50, -50, -142, -213,-251,},
    {236, 98, -98, -236, -236,-98, 98, 236,},
    {213, -50, -251, -142, 142,251, 50, -213,},
    {181, -181, -181, 181, 181, -181, -181, 181,},
    {142, -251, 50, 213, -213, -50, 251, -142,},
    {98, -236, 236, -98, -98, 236, -236, 98,},
    {50, -142, 213, -251, 251, -213, 142, -50,}};
// clang-format on
//============================================================================
int* matrix[8] = {
  matrixColor_B1[0], matrixColor_B2[0], matrixColor_B3[0], matrixColor_B4[0],
  matrixColor_B5[0], matrixColor_B6[0], matrixColor_B7[0], matrixColor_B8[0],
};

void fwdTransformColor_B2(int64_t transformBuf[][8], int count, int num) {
  int64_t E, O;
  int64_t coff[3][2] = {};
  int line = 1;
  for (int k = 0; k < num; ++k) {
    E = transformBuf[k][0] + transformBuf[k][1];
    O = transformBuf[k][0] - transformBuf[k][1];
    coff[k][0] = 362 * E;
    coff[k][1] = 362 * O;
  }
  for (int i = 0; i < 2; ++i) {
    for (int k = 0; k < num; ++k)
      transformBuf[k][i] = coff[k][i];
  }
}

void fwdTransformColor_B4(int64_t transformBuf[][8], int count, int num) {
  int64_t E[2], O[2];
  int64_t coff[3][4] = {};
  int line = 1;
  for (int k = 0; k < num; ++k) {
    E[0] = transformBuf[k][0] + transformBuf[k][3];
    O[0] = transformBuf[k][0] - transformBuf[k][3];
    E[1] = transformBuf[k][1] + transformBuf[k][2];
    O[1] = transformBuf[k][1] - transformBuf[k][2];
    coff[k][0] = (matrixColor_B4[0][0] * E[0] + matrixColor_B4[0][1] * E[1]);
    coff[k][2 * line] = (matrixColor_B4[2][0] * E[0] + matrixColor_B4[2][1] * E[1]);
    coff[k][line] = (matrixColor_B4[1][0] * O[0] + matrixColor_B4[1][1] * O[1]);
    coff[k][3 * line] = (matrixColor_B4[3][0] * O[0] + matrixColor_B4[3][1] * O[1]);
  }
  for (int i = 0; i < 4; ++i) {
    for (int k = 0; k < num; ++k)
      transformBuf[k][i] = coff[k][i];
  }
}

void fwdTransformColor_B8(int64_t transformBuf[][8], int count, int num) {
  int64_t E[4], O[4];
  int64_t EE[2], EO[2];
  int64_t coff[3][10] = {};

  for (int j = 0; j < num; j++) {
    /* E and O*/
    for (int k = 0; k < 4; k++) {
      E[k] = transformBuf[j][k] + transformBuf[j][7 - k];
      O[k] = transformBuf[j][k] - transformBuf[j][7 - k];
    }
    /* EE and EO */
    EE[0] = E[0] + E[3];
    EO[0] = E[0] - E[3];
    EE[1] = E[1] + E[2];
    EO[1] = E[1] - E[2];

    coff[j][0] = (matrixColor_B8[0][0] * EE[0] + matrixColor_B8[0][1] * EE[1]);
    coff[j][4] = (matrixColor_B8[4][0] * EE[0] + matrixColor_B8[4][1] * EE[1]);
    coff[j][2] = (matrixColor_B8[2][0] * EO[0] + matrixColor_B8[2][1] * EO[1]);
    coff[j][6] = (matrixColor_B8[6][0] * EO[0] + matrixColor_B8[6][1] * EO[1]);

    coff[j][1] = (matrixColor_B8[1][0] * O[0] + matrixColor_B8[1][1] * O[1] +
                  matrixColor_B8[1][2] * O[2] + matrixColor_B8[1][3] * O[3]);
    coff[j][3] = (matrixColor_B8[3][0] * O[0] + matrixColor_B8[3][1] * O[1] +
                  matrixColor_B8[3][2] * O[2] + matrixColor_B8[3][3] * O[3]);
    coff[j][5] = (matrixColor_B8[5][0] * O[0] + matrixColor_B8[5][1] * O[1] +
                  matrixColor_B8[5][2] * O[2] + matrixColor_B8[5][3] * O[3]);
    coff[j][7] = (matrixColor_B8[7][0] * O[0] + matrixColor_B8[7][1] * O[1] +
                  matrixColor_B8[7][2] * O[2] + matrixColor_B8[7][3] * O[3]);
  }
  for (int i = 0; i < 8; ++i) {
    for (int k = 0; k < num; ++k)
      transformBuf[k][i] = coff[k][i];
  }
}

void fwdTransformColor_Bk(int64_t transformBuf[][8], int count, int num) {
  int64_t* coff = new int64_t[count * 3]();
  int* matrixTrans = matrix[count - 1];
  for (int k = 0; k < num; k++) {
    for (int m = 0; m < count; m++) {
      for (int n = 0; n < count; n++)
        coff[count * k + m] += transformBuf[k][n] * matrixTrans[m * count + n];
    }
  }
  for (int k = 0; k < num; k++) {
    for (int m = 0; m < count; m++) {
      transformBuf[k][m] = coff[count * k + m];
    }
  }
  delete[] coff;
}

void invTransformColor_B2(int64_t transformBuf[][8], int count, int num) {
  int64_t E, O;
  int64_t coff[3][2] = {};
  int line = 1;
  for (int k = 0; k < num; ++k) {
    E = transformBuf[k][0] + transformBuf[k][1];
    O = transformBuf[k][0] - transformBuf[k][1];
    coff[k][0] = 362 * E;
    coff[k][1] = 362 * O;
  }
  for (int i = 0; i < 2; ++i) {
    for (int k = 0; k < num; ++k)
      transformBuf[k][i] = coff[k][i];
  }
}

void invTransformColor_B4(int64_t transformBuf[][8], int count, int num) {
  int64_t E[2], O[2];
  int64_t coff[3][4] = {};
  int line = 1;
  for (int k = 0; k < num; ++k) {
    O[0] = matrixColor_B4[1][0] * transformBuf[k][line] +
      matrixColor_B4[3][0] * transformBuf[k][3 * line];
    O[1] = matrixColor_B4[1][1] * transformBuf[k][line] +
      matrixColor_B4[3][1] * transformBuf[k][3 * line];
    E[0] =
      matrixColor_B4[0][0] * transformBuf[k][0] + matrixColor_B4[2][0] * transformBuf[k][2 * line];
    E[1] =
      matrixColor_B4[0][1] * transformBuf[k][0] + matrixColor_B4[2][1] * transformBuf[k][2 * line];
    coff[k][0] = E[0] + O[0];
    coff[k][line] = E[1] + O[1];
    coff[k][2 * line] = E[1] - O[1];
    coff[k][3 * line] = E[0] - O[0];
  }

  for (int i = 0; i < 4; ++i) {
    for (int k = 0; k < num; ++k)
      transformBuf[k][i] = coff[k][i];
  }
}

void invTransformColor_B8(int64_t transformBuf[][8], int count, int num) {
  int64_t E[4], O[4];
  int64_t EE[2], EO[2];
  int64_t coff[3][10] = {};
  for (int j = 0; j < num; j++) {
    /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
    for (int k = 0; k < 4; k++) {
      O[k] = matrixColor_B8[1][k] * transformBuf[j][1] + matrixColor_B8[3][k] * transformBuf[j][3] +
        matrixColor_B8[5][k] * transformBuf[j][5] + matrixColor_B8[7][k] * transformBuf[j][7];
    }

    EO[0] = matrixColor_B8[2][0] * transformBuf[j][2] + matrixColor_B8[6][0] * transformBuf[j][6];
    EO[1] = matrixColor_B8[2][1] * transformBuf[j][2] + matrixColor_B8[6][1] * transformBuf[j][6];
    EE[0] = matrixColor_B8[0][0] * transformBuf[j][0] + matrixColor_B8[4][0] * transformBuf[j][4];
    EE[1] = matrixColor_B8[0][1] * transformBuf[j][0] + matrixColor_B8[4][1] * transformBuf[j][4];

    /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
    E[0] = EE[0] + EO[0];
    E[3] = EE[0] - EO[0];
    E[1] = EE[1] + EO[1];
    E[2] = EE[1] - EO[1];

    for (int k = 0; k < 4; k++) {
      coff[j][k] = E[k] + O[k];
      coff[j][k + 4] = E[3 - k] - O[3 - k];
    }
  }
  for (int i = 0; i < 8; ++i) {
    for (int k = 0; k < num; ++k)
      transformBuf[k][i] = coff[k][i];
  }
}

void invTransformColor_Bk(int64_t transformBuf[][8], int count, int num) {
  int64_t* coff = new int64_t[count * 3]();
  int* matrixTrans = matrix[count - 1];
  for (int k = 0; k < num; k++) {
    for (int m = 0; m < count; m++) {
      for (int n = 0; n < count; n++)
        coff[k * count + m] += transformBuf[k][n] * matrixTrans[n * count + m];
    }
  }
  for (int k = 0; k < num; k++)
    for (int m = 0; m < count; m++)
      transformBuf[k][m] = coff[k * count + m];
  delete[] coff;
}
//=========================================================================================
FwdTrans* fwdTrans[8] = {fwdTransformColor_Bk, fwdTransformColor_B2, fwdTransformColor_Bk,
                         fwdTransformColor_B4, fwdTransformColor_Bk, fwdTransformColor_Bk,
                         fwdTransformColor_Bk, fwdTransformColor_B8};

InvTrans* invTrans[8] = {invTransformColor_Bk, invTransformColor_B2, invTransformColor_Bk,
                         invTransformColor_B4, invTransformColor_Bk, invTransformColor_Bk,
                         invTransformColor_Bk, invTransformColor_B8};

void Transform(int64_t tranformBuf[][8], int count, int num) {
  fwdTrans[count - 1](tranformBuf, count, num);
}

void invTransform(int64_t tranformBuf[][8], int count, int num) {
  invTrans[count - 1](tranformBuf, count, num);
}
//============================================================================