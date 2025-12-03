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

#include "TComPointCloud.h"
#include "HighLevelSyntax.h"
#include "TComKdTree.h"
#include "contributors.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <unordered_map>

///< \in TLibCommon \{

/**
 * Class TComPointCloud
 * point cloud class
 */

//////////////////////////////////////////////////////////////////////////
// Public class functions
//////////////////////////////////////////////////////////////////////////

Void TComPointCloud::clear() {
  m_pos.clear();
  m_color.clear();
  m_refl.clear();
  m_numPoint = 0;
  m_numMulti_color = 0;
  m_numMulti_refl = 0;
}

Void TComPointCloud::swapPoints(const TSize& idx1, const TSize& idx2) {
  assert(idx1 < m_numPoint && idx2 < m_numPoint);
  if (idx1 == idx2)
    return;
  swap(m_pos[idx1], m_pos[idx2]);
  if (hasColors())
    swap(m_color[idx1], m_color[idx2]);
  if (hasReflectances())
    swap(m_refl[idx1], m_refl[idx2]);
}

Bool TComPointCloud::readFromFile(const string& fileName, const Bool geomOnly) {
  this->clear();  ///< clear the point cloud data

  ifstream plyFile(fileName, ios::in | ios::binary);

  if (!checkCond(plyFile.is_open(), "Error: failed to read ply file!"))
    return false;

  const unordered_map<string, int> colorToIdx = {{"red", 0}, {"r", 0},    {"green", 1},
                                                 {"g", 1},   {"blue", 2}, {"b", 2}};
  string line;
  vector<string> tokens;
  Bool isStart = false;
  Bool isVertex = false;
  Bool isAscii = true;
  vector<AttributeReader> attributes;
  Int attrType = 0;
  unordered_map<string, int> multiAttrDataSize;  ///<This is used for multi attribute dataset
  int attrIdx = 0;
  bool first = false;
  ///< read ply header
  while (true) {
    if (!checkCond(!plyFile.eof(), "Error: invalid ply header!"))
      return false;

    getline(plyFile, line);
    getTokens(line, tokens);

    if (tokens.empty() || tokens[0] == "comment")  ///< empty line or comment line
      continue;
    if (!isStart) {
      if (!checkCond(tokens[0] == "ply", "Error: invalid ply header!"))
        return false;
      isStart = true;
    } else if (tokens[0] == "format") {
      if (!checkCond(tokens.size() == 3, "Error: invalid ply format!"))
        return false;
      if (tokens[1] == "ascii")
        isAscii = true;
      else if (tokens[1].find("binary") != string::npos)
        isAscii = false;
      else
        return checkCond(false, "Error: invalid ply format!");
    } else if (tokens[0] == "element") {
      if (!checkCond(tokens.size() == 3, "Error: invalid ply element!"))
        return false;
      if (tokens[1] == "vertex") {
        m_numPoint = atoi(tokens[2].data());
        assert(m_numPoint > 0);
        isVertex = true;
      } else if (tokens[1] == "face")
        isVertex = false;
      else
        return checkCond(false, "Error: invalid ply element!");
    } else if (tokens[0] == "property" && isVertex) {
      if (!checkCond(tokens.size() == 3, "Error: invalid ply property!"))
        return false;
      string propType = tokens[1];
      string propName = tokens[2];

      ///< in there maybe exit multi dataset
      int lastIdx = propName.size() - 1;
      while (lastIdx >= 0 && (propName[lastIdx] >= '0' && propName[lastIdx] <= '9')) {
        --lastIdx;
      }
      propName = propName.substr(0, lastIdx + 1);

      AttributeReader attr(this);
      attr.setSourceDataType(propType);
      attr.setTargetDataType(propName);

      if (attr.getTargetDataType() == sizeof(PC_COL)) {
        ++multiAttrDataSize[propName];

      } else if (attr.getTargetDataType() == sizeof(PC_REFL)) {
        ++multiAttrDataSize[propName];
      }
      if (multiAttrDataSize[propName] == 1 && !first) {
        first = true;
      }
      if (!first)
        ++attrIdx;

      attrType |= attr.setAttrType(propName);
      attributes.push_back(attr);
    } else if (tokens[0] == "end_header") {
      break;  ///< finish reading ply header
    }
  }  ///< end while

  ///< prepare attribute memories
  if ((attrType & AttributeReader::AttributeType::AT_POS) ==
      AttributeReader::AttributeType::AT_POS) {
    m_pos.resize(m_numPoint);
  } else {
    return checkCond(false, "Error: position is incomplete!");
  }
  if (geomOnly)
    attrType = AttributeReader::AttributeType::AT_POS;
  if ((attrType & AttributeReader::AttributeType::AT_COL) ==
      AttributeReader::AttributeType::AT_COL) {
    assert(multiAttrDataSize["red"] == multiAttrDataSize["green"]);
    assert(multiAttrDataSize["green"] == multiAttrDataSize["blue"]);
    assert(multiAttrDataSize["r"] == multiAttrDataSize["g"]);
    assert(multiAttrDataSize["g"] == multiAttrDataSize["b"]);
    this->addColors();
  }
  if ((attrType & AttributeReader::AttributeType::AT_REFLE) ==
      AttributeReader::AttributeType::AT_REFLE) {
    this->addReflectances();
  }
  if (!isAscii) {
    if ((attrType & AttributeReader::AttributeType::AT_COL)) {
      m_numMulti_color = std::max(multiAttrDataSize["red"], multiAttrDataSize["r"]);
      m_color.resize(m_numPoint * m_numMulti_color);
    }
    if ((attrType & AttributeReader::AttributeType::AT_REFLE)) {
      m_numMulti_refl =
        std::max(std::max(multiAttrDataSize["reflectance"], multiAttrDataSize["refl"]),
                 multiAttrDataSize["refc"]);
      m_refl.resize(m_numPoint * m_numMulti_refl);
    }
    for (auto& attr : attributes) {
      attr.setMultiAttrTarget();
    }
  }
    
  ///< read ply data
  TSize numPointFromeFile = 0;
  TSize numAttributes = attributes.size();
  while (!plyFile.eof() && numPointFromeFile < m_numPoint) {
    if (isAscii) {
      getline(plyFile, line);
      getTokens(line, tokens);
      if (tokens.empty())  ///< empty line
        continue;
      if (!checkCond(tokens.size() >= numAttributes, "Error: insufficient ply data!"))
        return false;
      ///<At first geometry information
      int beginIdx = attrIdx;
      for (int idx = 0; idx < 3; ++idx) {
        Double data = atof(tokens[idx].c_str());
        m_pos[numPointFromeFile][idx] = data;
      }
      //<color
      if ((attrType & AttributeReader::AttributeType::AT_COL)) {
        m_numMulti_color = std::max(multiAttrDataSize["red"], multiAttrDataSize["r"]);
        m_color.resize(m_numMulti_color * m_numPoint);
        for (int multilIdx = 0; multilIdx < m_numMulti_color; ++multilIdx)
          for (int idx = 0; idx < 3; ++idx) {
            UInt8 data = atoi(tokens[beginIdx].c_str());
            int attrIdx = colorToIdx.at(attributes[beginIdx].propName);
            m_color[numPointFromeFile * m_numMulti_color + multilIdx][attrIdx] = data;
            ++beginIdx;
          }
      }
      //<reflectances
      if ((attrType & AttributeReader::AttributeType::AT_REFLE)) {
        m_numMulti_refl =
          std::max(std::max(multiAttrDataSize["reflectance"], multiAttrDataSize["refl"]),
                   multiAttrDataSize["refc"]);
        m_refl.resize(m_numMulti_refl * m_numPoint);
        for (int multilIdx = 0; multilIdx < m_numMulti_refl; ++multilIdx) {
          PC_REFL data = atoi(tokens[beginIdx++].c_str());
          m_refl[numPointFromeFile * m_numMulti_refl + multilIdx] = data;
        }
      }
    } else {
      for (Int i = 0; i < numAttributes; i++) {
        attributes[i].readMultiAttrFromBinary(plyFile,numPointFromeFile,m_color,m_refl);
      }
        
    }

    numPointFromeFile++;
  }
  setNumPoint(numPointFromeFile);

  plyFile.close();
  return true;
}


Bool TComPointCloud::writeToFile(const string& fileName, const Bool isAscii) const {
  std::ofstream plyFile(fileName, ofstream::out | ofstream::binary);

  if (!checkCond(plyFile.is_open(), "Error: failed to write ply file!"))
    return EXIT_FAILURE;

  ///< write ply header
  plyFile << "ply" << endl;
  if (isAscii)
    plyFile << "format ascii 1.0" << endl;
  else {
    if (isLittleEndian())
      plyFile << "format binary_little_endian 1.0" << endl;
    else
      plyFile << "format binary_big_endian 1.0" << endl;
  }
  plyFile << "element vertex " << m_numPoint << endl;
  assert(m_pos.size() == m_numPoint);
  plyFile << "property float64 x" << endl;
  plyFile << "property float64 y" << endl;
  plyFile << "property float64 z" << endl;

  if (m_color.size() > 0) {
    plyFile << "property uchar red" << endl;
    plyFile << "property uchar green" << endl;
    plyFile << "property uchar blue" << endl;
    for (int multiIdx = 0; multiIdx < m_numMulti_color - 1; ++multiIdx) {
      plyFile << "property uchar red" << multiIdx << endl;
      plyFile << "property uchar green" << multiIdx << endl;
      plyFile << "property uchar blue" << multiIdx << endl;
    }
  }
  if (m_refl.size() > 0) {
    plyFile << "property int32 reflectance" << endl;
    for (int multiIdx = 0; multiIdx < m_numMulti_refl - 1; ++multiIdx) {
      plyFile << "property int32 reflectance" << multiIdx + 1 << endl;
    }
  }
  plyFile << "end_header" << endl;

  ///< write ply data
  if (isAscii) {
    plyFile << fixed << setprecision(6);
    for (TSize i = 0; i < m_numPoint; i++) {
      plyFile << m_pos[i];
      if (m_color.size() > 0){
        for (int multiIdx = 0; multiIdx < m_numMulti_color; ++multiIdx)
          plyFile << ' ' << static_cast<int>(m_color[i * m_numMulti_color + multiIdx][0]) << ' '
                  << static_cast<int>(m_color[i * m_numMulti_color + multiIdx][1]) << ' '
                  << static_cast<int>(m_color[i * m_numMulti_color + multiIdx][2]);
      }
        
      if (m_refl.size() > 0){
        for (int multiIdx = 0; multiIdx < m_numMulti_refl; ++multiIdx)
          plyFile << ' ' << m_refl[i * m_numMulti_refl + multiIdx];
      }
      plyFile << endl;
    }
  } else {
    for (TSize i = 0; i < m_numPoint; i++) {
      plyFile.write((TChar*)&(m_pos[i][0]), sizeof(PC_POS));
      if (m_color.size() > 0)
        plyFile.write((TChar*)&(m_color[i][0]), sizeof(PC_COL));
      if (m_refl.size() > 0)
        plyFile.write((TChar*)&(m_refl[i]), sizeof(PC_REFL));
    }
  }

  plyFile.close();
  return EXIT_SUCCESS;
}

Void TComPointCloud::computeBoundingBox(PC_POS& boxMin, PC_POS& boxMax) const {
  boxMin = std::numeric_limits<Double>::max();
  boxMax = std::numeric_limits<Double>::min();
  for (const auto& pos : m_pos) {
    for (Int i = 0; i < 3; i++) {
      if (boxMin[i] > pos[i])
        boxMin[i] = pos[i];
      if (boxMax[i] < pos[i])
        boxMax[i] = pos[i];
    }
  }
}

Void TComPointCloud::computeLcuBoundingBox(PC_POS& boxMin, PC_POS& boxMax, UInt pointIdxBegin,
                                           UInt pointIdxEnd) const {
  boxMin = std::numeric_limits<Double>::max();
  boxMax = std::numeric_limits<Double>::min();
  for (UInt idx = pointIdxBegin; idx < pointIdxEnd; idx++) {
    for (Int i = 0; i < 3; i++) {
      if (boxMin[i] > m_pos[idx][i])
        boxMin[i] = m_pos[idx][i];
      if (boxMax[i] < m_pos[idx][i])
        boxMax[i] = m_pos[idx][i];
    }
  }
}

Void TComPointCloud::computeColorRes(UInt& colorGolombNum, UInt outputDepth,
                                     UInt& attrQuantParam) const {
  int index = outputDepth - (attrQuantParam / 8);
  index = std::max(index, 1);
  colorGolombNum = kthIndex[index - 1];
}

Void TComPointCloud::computeReflRes(UInt& refGolombNum, UInt outputDepth,
                                    UInt& attrQuantParam) const {
  int index = outputDepth - (attrQuantParam / 8);
  index = std::max(index, 1);
  if (index > 13) {  
    refGolombNum = 7;
  } else {
    refGolombNum = kthIndex[index - 1];
  }
}

PC_POS& TComPointCloud::operator[](const TSize i) {
  assert(i < m_numPoint);
  return m_pos[i];
}

const PC_POS& TComPointCloud::operator[](const TSize i) const {
  assert(i < m_numPoint);
  return m_pos[i];
}

const TSize& TComPointCloud::getNumPoint() const {
  return m_numPoint;
}

const vector<PC_POS>& TComPointCloud::positions() const {
  return m_pos;
}

vector<PC_POS>& TComPointCloud::positions() {
  return m_pos;
}

Void TComPointCloud::setNumPoint(TSize numPoint) {
  m_numPoint = numPoint;
  m_pos.resize(numPoint);
  if (m_color.size() > 0)
    m_color.resize(numPoint * m_numMulti_color);
  if (m_refl.size() > 0)
    m_refl.resize(numPoint * m_numMulti_refl);
}
Void TComPointCloud::setPos(const TSize index, const V3<Double> pos) {
  assert(index < m_pos.size());
  m_pos[index] = pos;
}
Void TComPointCloud::setPoses(vector<PC_POS> poses) {
  m_pos = poses;
}

PC_COL TComPointCloud::getColor(const TSize index, const TSize& multilIdx) const {
  return m_color[index * m_numMulti_color + multilIdx];
}
PC_COL& TComPointCloud::getColor(const TSize index, const TSize& multilIdx) {
  return m_color[index * m_numMulti_color + multilIdx];
}
const vector<PC_COL>& TComPointCloud::getColors() const {
  return m_color;
}
vector<PC_COL>& TComPointCloud::getColors() {
  return m_color;
}
Void TComPointCloud::getMultiColors(const TSize index, vector<PC_COL>& colors) {
  for (int i = 0; i < m_numMulti_color; i++) {
    colors[i] = m_color[index * m_numMulti_color + i];
  }
}
Void TComPointCloud::setColor(const TSize index, const V3<UInt8> color, const TSize& multilIdx) {
  m_color[index * m_numMulti_color + multilIdx] = color;
}
Void TComPointCloud::setMultiColors(const TSize index, const vector<PC_COL> colors) {
  for (int i = 0; i < m_numMulti_color; i++) {
    m_color[index * m_numMulti_color + i] = colors[i];
  }
}
Void TComPointCloud::setColors(vector<PC_COL> colors) {
  m_color = colors;
}

PC_REFL TComPointCloud::getReflectance(const TSize index, const TSize& multilIdx) const {
  return m_refl[index * m_numMulti_refl + multilIdx];
}
PC_REFL& TComPointCloud::getReflectance(const TSize index, const TSize& multilIdx) {
  return m_refl[index * m_numMulti_refl + multilIdx];
}
const vector<PC_REFL>& TComPointCloud::getReflectances() const {
  return m_refl;
}
vector <PC_REFL>& TComPointCloud::getReflectances() {
  return m_refl;
}
Void TComPointCloud::getMultiReflectances(const TSize index, vector<PC_REFL>& reflectances) {
  for (int i = 0; i < m_numMulti_refl; i++) {
    reflectances[i] = m_refl[index * m_numMulti_refl + i];
  }
}

Void TComPointCloud::setReflectance(const TSize index, const PC_REFL reflectance,
                                    const TSize multilIdx) {
  m_refl[index * m_numMulti_refl + multilIdx] = reflectance;
}
Void TComPointCloud::setMultiReflectances(const TSize index, const vector<PC_REFL> reflectances) {
  for (int i = 0; i < m_numMulti_refl; i++) {
    m_refl[index * m_numMulti_refl + i] = reflectances[i];
  }
}
Void TComPointCloud::setReflectances(vector<PC_REFL> reflectances) {
  m_refl = reflectances;
}

int TComPointCloud::getNumMultilColor() {
  return m_numMulti_color;
}
int TComPointCloud::getNumMultilRefl() {
  return m_numMulti_refl;
}
int TComPointCloud::getNumMultilColor() const {
  return m_numMulti_color;
}
int TComPointCloud::getNumMultilRefl() const {
  return m_numMulti_refl;
}
Void TComPointCloud::setNumMultilColor(TSize numMultiColor) {
  m_numMulti_color = numMultiColor;
  m_color.resize(m_numPoint * m_numMulti_color);
}
Void TComPointCloud::setNumMultilRefl(TSize numMultiRefl) {
  m_numMulti_refl = numMultiRefl;
  m_refl.resize(m_numPoint * m_numMulti_refl);
}

Bool TComPointCloud::hasReflectances() const {
  return m_refl.size() > 0;
}
Void TComPointCloud::addReflectances() {
  m_refl.resize(m_numPoint * m_numMulti_refl);
}
Void TComPointCloud::removeReflectances() {
  m_refl.resize(0);
}

Bool TComPointCloud::hasColors() const {
  return m_color.size() > 0;
}
Void TComPointCloud::addColors() {
  m_color.resize(m_numPoint * m_numMulti_color);
}
Void TComPointCloud::removeColors() {
  m_color.resize(0);
}
Void TComPointCloud::init(const size_t size, bool WithColor, bool WithRef) {
  setNumPoint(size);
  if (WithColor == true) {
    addColors();
  } else {
    removeColors();
  }
  if (WithRef == true) {
    addReflectances();
  } else {
    removeReflectances();
  }
}

Void TComPointCloud::convertRGBToYUV() {
  for (auto& color : m_color) {
      const uint8_t r = color[0];
      const uint8_t g = color[1];
      const uint8_t b = color[2];
      const double y = TComClip(0., 255., std::round(0.212600 * r + 0.715200 * g + 0.072200 * b));
      const double u =
        TComClip(0., 255., std::round(-0.114572 * r - 0.385428 * g + 0.5 * b + 128.0));
      const double v =
        TComClip(0., 255., std::round(0.5 * r - 0.454153 * g - 0.045847 * b + 128.0));
      color[0] = static_cast<uint8_t>(y);
      color[1] = static_cast<uint8_t>(u);
      color[2] = static_cast<uint8_t>(v);
  }
}

Void TComPointCloud::convertYUVToRGB() {
  for (auto& color : m_color) {
      const double y1 = color[0];
      const double u1 = color[1] - 128.0;
      const double v1 = color[2] - 128.0;
      const double r = TComClip(0.0, 255.0, round(y1 + 1.57480 * v1));
      const double g = TComClip(0.0, 255.0, round(y1 - 0.18733 * u1 - 0.46813 * v1));
      const double b = TComClip(0.0, 255.0, round(y1 + 1.85563 * u1));
      color[0] = static_cast<uint8_t>(r);
      color[1] = static_cast<uint8_t>(g);
      color[2] = static_cast<uint8_t>(b);

  }
}

bool TComColorTransfer(const TComPointCloud& pointCloudOrg, double geomQuanStep, PC_VC3 quanOffset,
                       TComPointCloud& pointCloudRec) {
  const size_t orgPointCount = pointCloudOrg.getNumPoint();
  const size_t recPointCount = pointCloudRec.getNumPoint();
  if (!orgPointCount || !recPointCount || !pointCloudOrg.hasColors()) {
    return false;
  }

  pointCloudRec.addColors();
  pc_evalue::KdTree kdtreeOrg(pointCloudOrg.positions(), 10);
  pc_evalue::KdTree kdtreeRec(pointCloudRec.positions(), 10);
  std::vector<std::vector<std::vector<PC_COL>>> referColors1;
  PC_COL col = {0, 0, 0};

  // For each point of the origin point cloud,
  // find its nearest neighbor in the reconstructed cloud
  std::vector<size_t> indices;
  std::vector<double> sqrDist;
  //This will use a original point to reconstruct points
  for (Int i = 0; i < orgPointCount; i++) {
    for (int multiIdx = 0; multiIdx < pointCloudOrg.getNumMultilColor(); ++multiIdx) {
      const PC_COL curColor = pointCloudOrg.getColor(i, multiIdx);
      PC_VC3 QuanPos = (pointCloudOrg[i] - quanOffset) * geomQuanStep;
      int resultMax1 = 30;
      int resultNum1 = 0;
      do {
        resultNum1 += 5;
        indices.resize(resultNum1);
        sqrDist.resize(resultNum1);
        nanoflann::KNNResultSet<double> resultSet1(resultNum1);
        resultSet1.init(&indices[0], &sqrDist[0]);
        kdtreeRec.m_indices->findNeighbors(resultSet1, &QuanPos[0], nanoflann::SearchParams(10));
      } while (sqrDist[0] == sqrDist[resultNum1 - 1] && resultNum1 + 5 <= resultMax1);
      //! same distance points
      referColors1[indices[0]].resize(pointCloudOrg.getNumMultilColor());
      referColors1[indices[0]][multiIdx].push_back(curColor);
      for (int j = 1; j < resultNum1; j++) {
        if (abs(sqrDist[j] - sqrDist[0]) < 1e-8) {
          referColors1[indices[j]].resize(pointCloudOrg.getNumMultilColor());
          referColors1[indices[j]][multiIdx].push_back(curColor);
        } else
          break;
      }
    }
  }
  // -First project the points of the origin cloud to the reconstructed cloud.
  //  In case multiple origin points map to a single reconstructed point, the
  //  mean value is used.
  // -For the remained uncolored points, use their nearest neighbor attribute
  //  found above as their new attribute value
  for (Int i = 0; i < recPointCount; i++) {
    //pointCloudRec.getColors(i).resize(pointCloudOrg.getColorMultilDataSize());
    for (int multiIdx = 0; multiIdx < pointCloudOrg.getNumMultilColor(); ++multiIdx) {
      if (referColors1[i][multiIdx].empty()) {
        // if using Orignal KD-Tree not found nearset,will used Reconstruct KD-Tree
        double InverseQuanStep = 1.0 / geomQuanStep;
        PC_VC3 InverQuanPos = pointCloudRec[i] * InverseQuanStep + quanOffset;

        int resultMax = 30;
        int resultNum2 = 0;
        do {
          resultNum2 += 5;
          indices.resize(resultNum2);
          sqrDist.resize(resultNum2);
          nanoflann::KNNResultSet<double> resultSet2(resultNum2);
          resultSet2.init(&indices[0], &sqrDist[0]);
          kdtreeOrg.m_indices->findNeighbors(resultSet2, &InverQuanPos[0],
                                             nanoflann::SearchParams(10));
        } while (sqrDist[0] == sqrDist[resultNum2 - 1] && resultNum2 + 5 <= resultMax);

        size_t idx = indices[0];
        assert(idx >= 0);

        //! same distance points
        std::vector<size_t> multineighbour;
        multineighbour.push_back(idx);
        for (int j = 1; j < resultNum2; j++) {
          if (abs(sqrDist[j] - sqrDist[0]) < 1e-8) {
            multineighbour.push_back(indices[j]);
          } else
            break;
        }

        PC_VC3 colorAvg(0.0);
        for (const auto idx : multineighbour) {
          const auto color = pointCloudOrg.getColor(idx, multiIdx);
          for (size_t k = 0; k < 3; k++) {
            colorAvg[k] += color[k];
          }
        }
        colorAvg /= (double)multineighbour.size();
        PC_COL refColor;
        for (size_t k = 0; k < 3; k++) {
          refColor[k] = uint8_t(TComClip(0.0, 255.0, std::round(colorAvg[k])));
        }
        pointCloudRec.setColor(i, refColor, multiIdx);
      } else {
        PC_VC3 avgAttr(0.0);
        for (const auto color : referColors1[i][multiIdx]) {
          for (size_t k = 0; k < 3; k++) {
            avgAttr[k] += color[k];
          }
        }
        avgAttr /= double(referColors1[i].size());
        PC_COL avgColor;
        for (size_t k = 0; k < 3; k++) {
          avgColor[k] = uint8_t(TComClip(0.0, 255.0, std::round(avgAttr[k])));
        }
        pointCloudRec.setColor(i, avgColor, multiIdx);
      }
    }
  }
  return true;
}

bool TComReflectanceTransfer(const TComPointCloud& pointCloudOrg, double geomQuanStep,
                             PC_VC3 quanOffset, TComPointCloud& pointCloudRec) {
  const size_t orgPointCount = pointCloudOrg.getNumPoint();
  const size_t recPointCount = pointCloudRec.getNumPoint();
  if (!orgPointCount || !recPointCount || !pointCloudOrg.hasReflectances()) {
    return false;
  }

  pointCloudRec.addReflectances();
  pc_evalue::KdTree kdtreeOrg(pointCloudOrg.positions(), 10);
  pc_evalue::KdTree kdtreeRec(pointCloudRec.positions(), 10);
  std::vector < std::vector<std::vector<PC_REFL>>> referReflectances1;

  // For each point of the origin point cloud,
  // find its nearest neighbor in the reconstructed cloud
  std::vector<size_t> indices;
  std::vector<double> sqrDist;
  //This will used a original point to chongjian points
  for (Int i = 0; i < orgPointCount; i++) {
    for (int multiIdx = 0; multiIdx < pointCloudOrg.getNumMultilRefl(); ++multiIdx) {
      const PC_REFL curRefl = pointCloudOrg.getReflectance(i, multiIdx);
      PC_VC3 QuanPos = (pointCloudOrg[i] - quanOffset) * geomQuanStep;
      int resultMax1 = 30;
      int resultNum1 = 0;
      do {
        resultNum1 += 5;
        indices.resize(resultNum1);
        sqrDist.resize(resultNum1);
        nanoflann::KNNResultSet<double> resultSet1(resultNum1);
        resultSet1.init(&indices[0], &sqrDist[0]);
        kdtreeRec.m_indices->findNeighbors(resultSet1, &QuanPos[0], nanoflann::SearchParams(10));
      } while (sqrDist[0] == sqrDist[resultNum1 - 1] && resultNum1 + 5 <= resultMax1);
      //! same distance points
      referReflectances1[indices[0]].resize(pointCloudOrg.getNumMultilRefl());
      referReflectances1[indices[0]][multiIdx].push_back(curRefl);
      for (int j = 1; j < resultNum1; j++) {
        if (abs(sqrDist[j] - sqrDist[0]) < 1e-8) {
          referReflectances1[indices[j]].resize(pointCloudOrg.getNumMultilRefl());
          referReflectances1[indices[j]][multiIdx].push_back(curRefl);
        } else
          break;
      }
    }
  }

  // -First project the points of the origin cloud to the reconstructed cloud.
  //  In case multiple origin points map to a single reconstructed point, the
  //  mean value is used.
  // -For the remained uncolored points, use their nearest neighbor attribute
  //  found above as their new attribute value
  for (Int i = 0; i < recPointCount; i++) {
    for (int multiIdx = 0; multiIdx < pointCloudOrg.getNumMultilRefl(); ++multiIdx) {
      if (referReflectances1[i][multiIdx].empty()) {
        double InverseQuanStep = 1.0 / geomQuanStep;
        PC_VC3 InverQuanPos = pointCloudRec[i] * InverseQuanStep + quanOffset;

        int resultMax = 30;
        int resultNum2 = 0;
        do {
          resultNum2 += 5;
          indices.resize(resultNum2);
          sqrDist.resize(resultNum2);
          nanoflann::KNNResultSet<double> resultSet2(resultNum2);
          resultSet2.init(&indices[0], &sqrDist[0]);
          kdtreeOrg.m_indices->findNeighbors(resultSet2, &InverQuanPos[0],
                                             nanoflann::SearchParams(10));
        } while (sqrDist[0] == sqrDist[resultNum2 - 1] && resultNum2 + 5 <= resultMax);

        size_t idx = indices[0];
        assert(idx >= 0);

        //! same distance points
        std::vector<size_t> multineighbour;
        multineighbour.push_back(idx);
        for (int j = 1; j < resultNum2; j++) {
          if (abs(sqrDist[j] - sqrDist[0]) < 1e-8) {
            multineighbour.push_back(indices[j]);
          } else
            break;
        }

        double reflectanceAvg = 0.0;
        for (int i = 0; i < multineighbour.size(); i++) {
          reflectanceAvg += (double)pointCloudOrg.getReflectance(multineighbour[i], multiIdx);
        }
        reflectanceAvg = TComClip(double(std::numeric_limits<PC_REFL>::min()),
                                  double(std::numeric_limits<PC_REFL>::max()),
                                  std::round(reflectanceAvg / double(multineighbour.size())));
        pointCloudRec.setReflectance(i, reflectanceAvg, multiIdx);
      } else {
        double avgAttr = 0.0;
        for (const auto reflectance :
             referReflectances1[i][multiIdx]) {
          avgAttr += reflectance;
        }
        avgAttr = TComClip(double(std::numeric_limits<PC_REFL>::min()),
                           double(std::numeric_limits<PC_REFL>::max()),
                           std::round(avgAttr / double(referReflectances1[i].size())));
        pointCloudRec.setReflectance(i, uint16_t(avgAttr), multiIdx);
      }
    }
  }
  return true;
}

int recolour(const TComPointCloud& pointCloudOrg, float geomQuanStep, V3<int> quanOffset,
             TComPointCloud* pointCloudRec) {
  PC_VC3 doubleQuanOffset;
  for (int k = 0; k < 3; k++)
    doubleQuanOffset[k] = double(quanOffset[k]);

  if (pointCloudOrg.hasColors()) {
    bool ok = TComColorTransfer(pointCloudOrg, geomQuanStep, doubleQuanOffset, *pointCloudRec);

    if (!ok) {
      std::cout << "Error: can't transfer colors!" << std::endl;
      return -1;
    }
  }

  if (pointCloudOrg.hasReflectances()) {
    bool ok =
      TComReflectanceTransfer(pointCloudOrg, geomQuanStep, doubleQuanOffset, *pointCloudRec);

    if (!ok) {
      std::cout << "Error: can't transfer reflectance!" << std::endl;
      return -1;
    }
  }
  return 0;
}

void sortReconPoints(const TComPointCloud& pointCloudRecon, UInt lastNumReconPoints,
                     UInt currNumReconPoints, std::vector<pointCodeWithIndex>& mortonOrder) {
  Int count = 0;
  for (UInt idx = lastNumReconPoints; idx < currNumReconPoints; idx++) {
    const PC_POS& point = pointCloudRecon[idx];
    mortonOrder[count].index = idx;
    mortonOrder[count].code = mortonAddr((Int32)point[0], (Int32)point[1], (Int32)point[2]);
    count++;
  }
  std::sort(mortonOrder.begin(), mortonOrder.end());
}

void sortReconPointsShift(const TComPointCloud& pointCloudRecon, const Int shift,
                          UInt lastNumReconPoints, UInt currNumReconPoints,
                          std::vector<pointCodeWithIndex>& mortonOrder,
                          std::vector<int>& mortonToindex, std::vector<int>& mortonTomorton2) {
  Int count = 0;
  for (UInt idx = lastNumReconPoints; idx < currNumReconPoints; idx++) {
    auto& index = mortonOrder[count].index;
    mortonToindex[count] = index;
    const PC_POS& point = pointCloudRecon[index];
    mortonOrder[count].code =
      mortonAddr((Int32)point[0] + shift, (Int32)point[1] + shift, (Int32)point[2] + shift);
    mortonOrder[count].index = count;
    count++;
  }
  std::sort(mortonOrder.begin(), mortonOrder.end());
  count = 0;
  for (UInt idx = lastNumReconPoints; idx < currNumReconPoints; idx++) {
    mortonTomorton2[mortonOrder[count].index] = count;
    count++;
  }
}
int32_t determineInitShiftBits(const uint32_t voxelCount,
                               const std::vector<pointCodeWithIndex>& pointCloudMorton) {
  int initShiftBits = -3;
  int recycleTime = 0;
  float Ratio = 0;
  while (voxelCount >= 100 && Ratio < 1) {
    initShiftBits += 3;
    recycleTime = 0;
    int neighborCount = 0;
    for (int i = (voxelCount / 100); i < voxelCount; i += (voxelCount / 100)) {
      ++recycleTime;
      for (int j = i + 1; j < voxelCount; ++j) {
        if (pointCloudMorton[j].code >> (initShiftBits + 3) !=
            pointCloudMorton[i].code >> (initShiftBits + 3))
          break;
        else {
          ++neighborCount;
        }
      }
      for (int j = i - 1; j > 0; --j) {
        if (pointCloudMorton[j].code >> (initShiftBits + 3) !=
            pointCloudMorton[i].code >> (initShiftBits + 3))
          break;
        else {
          ++neighborCount;
        }
      }
    }
    Ratio = (float)neighborCount / recycleTime;
  }
  initShiftBits += 3;

  return initShiftBits;
}
//
//============================================================

Void reOrder(const vector<PC_POS>& pointPos, const UInt& sortMode,
             std::vector<pointCodeWithIndex>& pointCloudCode, const int& voxelCount,
             const UInt& axisBias) {
  switch (sortMode) {
  case 0:
    for (UInt n = 0; n < voxelCount; n++) {
      pointCloudCode[n].code = (int32_t)n;
      pointCloudCode[n].index = n;
    }
    break;
  case 1:
    for (UInt n = 0; n < voxelCount; n++) {
      HilbertAddr(pointPos[n][0], pointPos[n][1], axisBias * pointPos[n][2],
                  pointCloudCode[n].code);
      pointCloudCode[n].index = n;
    }
    std::sort(pointCloudCode.begin(), pointCloudCode.end());
    break;
  case 2:
    for (UInt n = 0; n < voxelCount; n++) {
      const auto& point = pointPos[n];
      pointCloudCode[n].code =
        mortonAddr((int32_t)point[0], (int32_t)point[1], axisBias * (int32_t)point[2]);
      pointCloudCode[n].index = n;
    }
    std::sort(pointCloudCode.begin(), pointCloudCode.end());
    break;
  }
}
///< \}
