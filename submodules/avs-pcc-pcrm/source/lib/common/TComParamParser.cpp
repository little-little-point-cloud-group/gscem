/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2019-2033, Audio Video coding Standard Workgroup of China
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of Audio Video coding Standard Workgroup of China
*    nor the names of its contributors maybe used to endorse or promote products
*    derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "TComParamParser.h"
#include "contributors.h"
#include <iostream>

///< \in TLibCommon \{

//////////////////////////////////////////////////////////////////////////
// Public class functions
//////////////////////////////////////////////////////////////////////////

TComParamParser::~TComParamParser() {
  for (auto paraGroup : m_paramGroups) {
    for (auto para : paraGroup.params) {
      if (para)
        delete para;
      para = nullptr;
    }
  }
}

void TComParamParser::parseConfigureFile(const string& configFileName, AttributeParameterSet& aps) {
  vector<string> keys;
  vector<string> vals;

  m_configParam.configParser(configFileName, keys, vals);

  for (int i = 0; i < keys.size(); i++) {
    bool findKey = setLongParameter(keys[i], vals[i], aps);
    if (keys[i] == "parse_multi_attr_params" || keys[i] == "pmap") {
      stringstream ss(vals[i], istringstream::in);
      ss >> m_reserveMultiAttrParams;
    }
    else if (keys[i] == "num_multi_attr_params_set" || keys[i] == "nmaps") {
      stringstream ss(vals[i], istringstream::in);
      ss >> m_numAttrParamSet;
    }
    else if (keys[i] == "multi_attr_params_set_idx" || keys[i] == "mapsi") {
      stringstream ss(vals[i], istringstream::in);
      ss >> m_multiAttrIdx;
      checkCond(m_multiAttrIdx < m_numAttrParamSet, "The multil attribute index must less than the number of multil attribute parameters set");

    }
    else if (keys[i] == "attribute" || keys[i] == "attr") {
      if (vals[i] == "color")
        attrIdx = 0;
      else if (vals[i] == "refl")
        attrIdx = 1;
      else {
        std::cout << "======================<The new attribute type!>=====================" << std::endl;
      }

    }
    if (!findKey)
      findKey = setShortParameter(keys[i], vals[i], aps);
    if (!findKey)
      m_invalidParams.push_back(keys[i]);
  }
}

void TComParamParser::initParameters() {
  for (const auto& paraGroup : m_paramGroups) {
    for (const auto& para : paraGroup.params)
      para->init();
  }
}

void TComParamParser::printHelp(ostream& out) {
  ///< get the max length of long parameters and short parameters
  int maxLengthLong = 0;
  int maxLengthShort = 0;
  for (const auto& paraGroup : m_paramGroups) {
    for (const auto& para : paraGroup.params) {
      if (para->nameLong.length() > maxLengthLong)
        maxLengthLong = (int)para->nameLong.length();
      if (para->nameShort.length() > maxLengthShort)
        maxLengthShort = (int)para->nameShort.length();
    }
  }

  ///< print help infor
  for (auto& paraGroup : m_paramGroups) {
    paraGroup.printGroupName(out);
    for (const auto& para : paraGroup.params)
      para->printHelp(out, maxLengthLong, maxLengthShort);
  }
  out << endl;
}

void TComParamParser::printParameters(ostream& out) {
  ///< get the max length of long parameters
  int maxLengthLong = 0;
  for (const auto& paraGroup : m_paramGroups) {
    for (const auto& para : paraGroup.params)
      if (para->nameLong.length() > maxLengthLong)
        maxLengthLong = (int)para->nameLong.length();
  }
  for (auto& paraGroup : m_paramGroups) {
    paraGroup.printGroupName(out);
    for (const auto& para : paraGroup.params)
      para->printValue(out, maxLengthLong);
  }
  out << endl;
}

void TComParamParser::printInvalidParameters(ostream& out) {
  for (const auto& para : m_invalidParams) {
    out << "Warning: invalid parameter --" << para << endl;
  }
}

void TComParamParser::parseParameters(const int argc, const char* argv[], AttributeParameterSet* aps,const bool& isEncoder) {
  int idx = 1;
  string key, val;
  while (idx < argc && argv[idx] != nullptr) {
    switch (getKey(idx, argv, key)) {
    case KT_INVALID:  ///< invalid option
      m_invalidParams.push_back(key);
    case KT_LONG:  ///< double dash parameter, such as --help
      getValue(idx, argv, val);
      if (!setLongParameter(key, val,*aps))
        m_invalidParams.push_back(key);
      break;
    case KT_SHORT:  ///< single dash parameter, such as -h
      getValue(idx, argv, val);
      if (!setShortParameter(key, val, *aps))
        m_invalidParams.push_back(key);
      break;
    case KT_FILE:  ///< parse from configure file
      getValue(idx, argv, val);
      parseConfigureFile(val,*aps);
      break;
    default:
      break;
    }
  }
  if (isEncoder)
    aps->updateMultilAttrParams = m_reserveMultiAttrParams;
}

TComParamParser& TComParamParser::addParameter() {
  return *this;
}

//////////////////////////////////////////////////////////////////////////
// Private class functions
//////////////////////////////////////////////////////////////////////////

int TComParamParser::getKey(int& argi, const char* argv[], string& key) {
  string str(argv[argi++]);

  if (str.length() <= 1 || str[0] != '-')  ///< invalid key
  {
    key = str;
    return KT_INVALID;
  }

  ///< obtain key from the following pattern
  ///< --key
  ///< -key
  ///< --key=v
  ///< -key=v
  size_t startPos = str.find_first_not_of('-');
  size_t endPos = str.find_first_of('=');
  key = str.substr(startPos, endPos - startPos);

  if (key == m_configParam.nameLong || key == m_configParam.nameShort)
    return KT_FILE;

  return (str[1] == '-') ? KT_LONG : KT_SHORT;
}

void TComParamParser::getValue(int& argi, const char* argv[], string& val) {
  if (argv[argi] == nullptr)  ///< reach the last parameter
  {
    val = "1";  ///< infer default value is 1 (caution: only works for boolean parameters)
    return;
  }
  string str(argv[argi++]);

  if (str.length() == 0 || str[0] == '-')  ///< invalid value
  {
    val = "1";  ///< infer default value is 1 (caution: only works for boolean parameters)
    argi--;
    return;
  }
  val = str;
}
void TComParamParser::setMultiAttrLongParameter(const string& key, const string& val, AttributeParameterSet& aps) {

  if (key == "cross_comp_pred")
  {
    stringToValue(key, val, aps.crossMultiComponentPred[m_multiAttrIdx]);
  }
  else if (key == "order_switch")
  {
    stringToValue(key, val, aps.orderMultiSwitch[m_multiAttrIdx]);
  }
  else if (key == "color_output_depth")
  {
    stringToValue(key, val, aps.colorMultiOutputDepth[m_multiAttrIdx]);
    stringToValue(key, val, aps.outputMultiBitDepthMinus1[0][m_multiAttrIdx]);
  }
  else if (key == "refl_output_depth")
  {
    stringToValue(key, val, aps.reflMultiOutputDepth[m_multiAttrIdx]);
    stringToValue(key, val, aps.outputMultiBitDepthMinus1[1][m_multiAttrIdx]);
  }
  else if (key == "color_quant_param")
  {
    stringToValue(key, val, aps.colorMultiQuantParam[m_multiAttrIdx]);
    stringToValue(key, val, aps.attrMultiQuantParam[0][m_multiAttrIdx]);
  }
  else if (key == "refl_quant_param")
  {
    stringToValue(key, val, aps.reflMultiQuantParam[m_multiAttrIdx]);
    stringToValue(key, val, aps.attrMultiQuantParam[1][m_multiAttrIdx]);
  }
  else if (key == "chroma_qp_offset_cb")
  {
    stringToValue(key, val, aps.chromaMultiQpOffsetCb[m_multiAttrIdx]);
  }
  else if (key == "chroma_qp_offset_cr")
  {
    stringToValue(key, val, aps.chromaMultiQpOffsetCr[m_multiAttrIdx]);
  }
  else if (key == "nearest_pred_param1")
  {
    stringToValue(key, val, aps.nearestMultiPredParam1[m_multiAttrIdx]);
  }
  else if (key == "nearest_pred_param2")
  {
    stringToValue(key, val, aps.nearestMultiPredParam2[m_multiAttrIdx]);
  }
  else if (key == "chroma_Qp_Offset_DC")
  {
    stringToValue(key, val, aps.chromaMultiQpOffsetDC[m_multiAttrIdx]);
  }
  else if (key == "chroma_Qp_Offset_AC")
  {
    stringToValue(key, val, aps.chromaMultiQpOffsetAC[m_multiAttrIdx]);
  }
  else if (key == "refl_max_transform_number")
  {
    stringToValue(key, val, aps.reflMaxMultiTransNum[m_multiAttrIdx]);
  }
  else if (key == "color_max_transform_number")
  {
    stringToValue(key, val, aps.colorMaxMultiTransNum[m_multiAttrIdx]);
  }
  else if (key == "color_reorder_mode")
  {
    stringToValue(key, val, aps.colorMultiReordermode[m_multiAttrIdx]);
  }
  else if (key == "refl_reorder_mode")
  {
    stringToValue(key, val, aps.reflMultiReordermode[m_multiAttrIdx]);
  }
  else if (key == "refl_group_predict")
  {
    stringToValue(key, val, aps.reflMultiGroupPredict[m_multiAttrIdx]);
  }
  else if (key == "golomb_group_size_log2")
  {
    stringToValue(key, val, aps.golombMultiGroupSizeLog2[m_multiAttrIdx]);
  }
  else if (key == "color_QP_Adjust_Flag")
  {
    stringToValue(key, val, aps.colorMultiQPAdjustFlag[m_multiAttrIdx]);
  }
  else if (key == "pred_fixed_point_frac_bit")
  {
    stringToValue(key, val, aps.predMultiFixedPointFracBit[m_multiAttrIdx]);
  }
  else if (key == "pred_dist_weight_group_size_log2")
  {
    stringToValue(key, val, aps.predDistWeightMultiGroupSizeLog2[m_multiAttrIdx]);
  }
  else if (key == "multi_attr_group_id") {
    stringToArray(val, &aps.multiAttrGroupID[0]);
  }
  else if (key == "transform") {
    stringToValue(key, val, aps.transformMulti[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "transform_segment_size") {
    stringToValue(key, val, aps.transformMultiSegmentSize[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "Qp_Offset_DC") {
    stringToValue(key, val, aps.QpMultiOffsetDC[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "Qp_Offset_AC") {
    stringToValue(key, val, aps.QpMultiOffsetAC[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "log2_max_number_coefficients_minus8") {
    stringToValue(key, val,
      aps.MultimaxNumofCoeffLog2Minus8[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "coeff_length_control_log2_minus8") {
    stringToValue(key, val,
      aps.coeffMultiLengthControlLog2Minus8[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "trans_residue_layer") {
    stringToValue(key, val, aps.transMultiResLayer[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "k_Frac_Bit") {
    stringToValue(key, val, aps.kMultiFracBits[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "max_number_neighbours_log2_minus7") {
  stringToValue(key, val, aps.maxMultiNumOfNeighboursLog2Minus7[attrIdx][m_multiAttrIdx]);
  }
}
void TComParamParser::setMultiAttrShortParameter(const string& key, const string& val, AttributeParameterSet& aps) {

  if (key == "ccp")
  {
    stringToValue(key, val, aps.crossMultiComponentPred[m_multiAttrIdx]);
  }
  else if (key == "os")
  {
    stringToValue(key, val, aps.orderMultiSwitch[m_multiAttrIdx]);
  }
  else if (key == "cod")
  {
    stringToValue(key, val, aps.colorMultiOutputDepth[m_multiAttrIdx]);
  }
  else if (key == "rod")
  {
    stringToValue(key, val, aps.reflMultiOutputDepth[m_multiAttrIdx]);
  }
  else if (key == "cqp")
  {
    stringToValue(key, val, aps.colorMultiQuantParam[m_multiAttrIdx]);
  }
  else if (key == "rqp")
  {
    stringToValue(key, val, aps.reflMultiQuantParam[m_multiAttrIdx]);
  }
  else if (key == "qocb")
  {
    stringToValue(key, val, aps.chromaMultiQpOffsetCb[m_multiAttrIdx]);
  }
  else if (key == "qocr")
  {
    stringToValue(key, val, aps.chromaMultiQpOffsetCr[m_multiAttrIdx]);
  }
  else if (key == "npp1")
  {
    stringToValue(key, val, aps.nearestMultiPredParam1[m_multiAttrIdx]);
  }
  else if (key == "npp2")
  {
    stringToValue(key, val, aps.nearestMultiPredParam2[m_multiAttrIdx]);
  }
  else if (key == "cqodc")
  {
    stringToValue(key, val, aps.chromaMultiQpOffsetDC[m_multiAttrIdx]);
  }
  else if (key == "cqoac")
  {
    stringToValue(key, val, aps.chromaMultiQpOffsetAC[m_multiAttrIdx]);
  }
  else if (key == "rmtn")
  {
    stringToValue(key, val, aps.reflMaxMultiTransNum[m_multiAttrIdx]);
  }
  else if (key == "cmtn")
  {
    stringToValue(key, val, aps.colorMaxMultiTransNum[m_multiAttrIdx]);
  }
  else if (key == "crom")
  {
    stringToValue(key, val, aps.colorMultiReordermode[m_multiAttrIdx]);
  }
  else if (key == "rrom")
  {
    stringToValue(key, val, aps.reflMultiReordermode[m_multiAttrIdx]);
  }
  else if (key == "rgp")
  {
    stringToValue(key, val, aps.reflMultiGroupPredict[m_multiAttrIdx]);
  }
  else if (key == "ggsl")
  {
    stringToValue(key, val, aps.golombMultiGroupSizeLog2[m_multiAttrIdx]);
  }
  else if (key == "cqpaf")
  {
    stringToValue(key, val, aps.colorMultiQPAdjustFlag[m_multiAttrIdx]);
  }
  else if (key == "pfpfb")
  {
    stringToValue(key, val, aps.predMultiFixedPointFracBit[m_multiAttrIdx]);
  }
  else if (key == "pdwgsl")
  {
    stringToValue(key, val, aps.predDistWeightMultiGroupSizeLog2[m_multiAttrIdx]);
  }
  else if (key == "magid")
  {
    stringToArray(val, &aps.multiAttrGroupID[0]);
  }
  else if (key == "trans") {
    stringToValue(key, val, aps.transformMulti[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "tss") {
    stringToValue(key, val, aps.transformMultiSegmentSize[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "qodc") {
    stringToValue(key, val, aps.QpMultiOffsetDC[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "qoac") {
    stringToValue(key, val, aps.QpMultiOffsetAC[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "mncmlm8") {
    stringToValue(key, val,
      aps.MultimaxNumofCoeffLog2Minus8[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "clclm8") {
    stringToValue(key, val,
      aps.coeffMultiLengthControlLog2Minus8[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "trl") {
    stringToValue(key, val, aps.transMultiResLayer[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "kfb") {
    stringToValue(key, val, aps.kMultiFracBits[attrIdx][m_multiAttrIdx]);
  }
  else if (key == "cqp") {
  stringToValue(key, val, aps.attrMultiQuantParam[0][m_multiAttrIdx]);
  }
  else if (key == "rqp") {
  stringToValue(key, val, aps.attrMultiQuantParam[1][m_multiAttrIdx]);
  }
  else if (key == "mnnlm7") {
  stringToValue(key, val, aps.maxMultiNumOfNeighboursLog2Minus7[attrIdx][m_multiAttrIdx]);
  }
}
bool TComParamParser::isMultiAttrLongParameter(const string& key) {

  return (m_multiAttrIdx >= 0) && ((key == "cross_comp_pred") || (key == "order_switch") || (key == "color_output_depth") || (key == "refl_output_depth")
    || (key == "color_quant_param") || (key == "refl_quant_param") || (key == "chroma_qp_offset_cb") || (key == "chroma_qp_offset_cr")
    || (key == "nearest_pred_param1") || (key == "nearest_pred_param2") || (key == "chroma_Qp_Offset_DC") || (key == "chroma_Qp_Offset_AC")
    || (key == "refl_max_transform_number") || (key == "color_max_transform_number")|| (key == "color_reorder_mode")  || (key == "refl_reorder_mode")
    || (key == "refl_group_predict") || (key == "golomb_group_size_log2") || (key == "color_QP_Adjust_Flag") || (key == "pred_fixed_point_frac_bit")|| (key == "pred_dist_weight_group_size_log2") 
    || (key == "multi_attr_group_id") || (key == "transform") || (key == "transform_segment_size") || (key == "Qp_Offset_DC") || (key == "Qp_Offset_AC")|| (key == "max_number_coefficients_log2_minus8")
    || (key == "coeff_length_control_log2_minus8") || (key == "trans_residue_layer") || (key == "k_Frac_Bit")
    || (key == "color_quant_param") || (key == "refl_quant_param") || (key == "max_number_neighbours_log2_minus7"));

}

bool TComParamParser::isMultiAttrShortParameter(const string& key) {

  return  (m_multiAttrIdx >= 0) && ((key == "ccp") || (key == "os") || (key == "cod") || (key == "rod")
    || (key == "cqm") || (key == "rqm") || (key == "qocb") || (key == "qocr")
    || (key == "npp1") || (key == "npp2") || (key == "cqodc") || (key == "cqoac")
    || (key == "rmtn") || (key == "cmtn") || (key == "crm") || (key == "rrm")
    || (key == "rgp") || (key == "ggsl") || (key == "cqpaf") || (key == "pfpfb") || (key == "pdwgsl") 
    || (key == "magid") || (key == "trans") || (key == "tss") || (key == "qodc") || (key == "qoac") || (key == "mncmlm8")
    || (key == "clclm8") || (key == "trl") || (key == "kfb") || (key == "cqp") || (key == "rqp") || (key == "mnnlm7"));
}
bool TComParamParser::setLongParameter(const string& key, const string& val, AttributeParameterSet& aps) {
  bool findKey = false;
  for (auto paraGroup : m_paramGroups) {
    for (auto para : paraGroup.params)
      if (para->nameLong == key) {
        if (m_reserveMultiAttrParams && isMultiAttrLongParameter(key)) {
          setMultiAttrLongParameter(key, val,aps);
          findKey = true;
        }
        else {
          if (key == "multi_attr_group_id")
            stringToArray(val, &aps.multiAttrGroupID[0]);
          else
            para->set(val);
          findKey = true;
        }
        break;
      }
  }
  return findKey;
}

bool TComParamParser::setShortParameter(const string& key, const string& val, AttributeParameterSet& aps) {
  bool findKey = false;
  for (auto paraGroup : m_paramGroups) {
    for (auto para : paraGroup.params)
      if (para->nameShort == key) {
        if (m_reserveMultiAttrParams && isMultiAttrShortParameter(key)) {
          setMultiAttrShortParameter(key, val, aps);
          findKey = true;
        }
          
        else {
          if (key == "magid")
            stringToArray(val, &aps.multiAttrGroupID[0]);
          para->set(val);
          findKey = true;
        }
        break;
      }
  }
  return findKey;
}

///< \}
