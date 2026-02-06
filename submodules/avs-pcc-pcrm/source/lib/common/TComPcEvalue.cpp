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

#include "TComPcEvalue.h"

namespace pc_evalue {
TMetricCfg::TMetricCfg() {
  m_srcFile = "";
  m_disFile = "";
  m_calLossless = false;
  m_calColor = false;
  m_calReflectance = false;
  m_peakValue = 0.0;
  m_duplicateMode = 1;
  m_multiNeighbourMode = 1;
  m_symmetry = true;
  m_showHausdorff = true;
}
bool TMetricCfg::parseCfg(Int argc, TChar* argv[]) {
  Bool isHelp = false;
  Bool ret = 0;

  // clang-format off
  TComParamParser parser;
  parser.addParameter() ///< data, default value, short option, long option, description
    (isHelp,               false,      "h",   "help",                "help")
    (ConfigParser,                     "c",   "config",              "config file")
    (m_srcFile,            string(""), "f1",  "file1",               "file name of file1")
    (m_disFile,            string(""), "f2",  "file2",               "file name of file2")
    (m_numOfFrames,        (UInt)1,    "ftbc", "frames_to_be_coded",  "number of frames to be coded. Default: 1")
    (m_symmetry,           true,       "sy",  "symmetry",            "if calculate symmetry metrics")
    (m_calLossless,        false,      "cll", "cal_lossless",        "if calculate lossless metrics. 1: on, 0 : off")
    (m_calColor,           false,      "cc",  "cal_color",           "if calculate color metrics. 1: on, 0 : off")
    (m_calReflectance,     false,      "cr",  "cal_reflectance",     "if calculate reflectance metrics. 1: on, 0 : off")
    (m_peakValue,          0.0f,       "pk",  "peakvalue",           "peak value of Geometry PSNR(default = 0)")
    (m_reflOutputDepth,    (UInt)16,   "rod", "refl_output_depth",   "reflectance output attribute bit depth (default = 16)")
    (m_duplicateMode,      true,       "dp",  "duplicate_mode",      "process duplicated points. 1: average 0 : no process")
    (m_multiNeighbourMode, true,       "ne",  "multineighbour_mode", "process same distance neighbours. 1: average 0 : no process")
    (m_showHausdorff,      true,       "hau", "show_hausdorff",      "if show hausdorff and hausdorffPSNR. 1: on, 0: off")
  ;
  // clang-format on
  //AttributeParameterSet aps;
  parser.initParameters();                            ///< set the default parameters
  parser.parseParameters(argc, (const TChar**)argv, nullptr);  ///< parsing
  parser.printInvalidParameters(cout);                ///< print warnings

  if (isHelp || argc == 1) {  ///< print help info
    parser.printHelp(cout);
    return false;
  }

  ret = checkParameter();  ///< check the validity of parameters

  parser.printParameters(cout);  ///< print parameter values

  return ret;
}

bool TMetricCfg::checkParameter() {
  bool isFailed = false;

  isFailed |= checkCond(m_srcFile.length() > 0, "Error: file1 must be specified.");
  isFailed |= checkCond(m_disFile.length() > 0, "Error: file2 must be specified.");

  return isFailed;
}

TMetricRes::TMetricRes() {
  m_resultLossless = true;
  m_geometryMSE = 0.0;
  m_geometryHausdorff = 0.0;
  m_geometryPSNR = 0.0;
  m_geometryHausdorffPSNR = 0.0;

  for (int idx = 0; idx < NUM_MULTIATTRIBUTE; ++idx) {
    m_colorMSE[idx][0] = m_colorMSE[idx][1] = m_colorMSE[idx][2] = 0.0;
    m_colorHausdorff[idx][0] = m_colorHausdorff[idx][1] = m_colorHausdorff[idx][2] = 0.0;
    m_colorPSNR[idx][0] = m_colorPSNR[idx][1] = m_colorPSNR[idx][2] = 0.0;
    m_colorHausdorffPSNR[idx][0] = m_colorHausdorffPSNR[idx][1] = m_colorHausdorffPSNR[idx][2] =
      0.0;

    m_reflectanceMSE[idx] = 0.0;
    m_reflectanceHausdorff[idx] = 0.0;
    m_reflectancePSNR[idx] = 0.0;
    m_reflectanceHausdorffPSNR[idx] = 0.0;
  }
  m_peakValue = 0.0;
}

void TMetricRes::print(TMetricCfg para, int colorMultiDataSize, int refMultiDataSize)
{
  cout.setf(ios::fixed);
  cout.precision(6);
  std::cout << "All frames metric result" << std::endl;
  std::cout << "   D1_PSNR_Ave          : " << m_geometryPSNR << std::endl;
  if (para.m_showHausdorff) {
    std::cout << "   D1_HausdorffPSNR : " << m_geometryHausdorffPSNR << std::endl;
  }
  if (para.m_calColor) {
    for (int mutilIdx = 0; mutilIdx < colorMultiDataSize; ++mutilIdx) {
      std::cout << "MultiData " << mutilIdx
                << "   c[0]_PSNR_Ave           : " << m_colorPSNR[mutilIdx][0] << std::endl;
      std::cout << "MultiData " << mutilIdx
                << "   c[1]_PSNR_Ave           : " << m_colorPSNR[mutilIdx][1] << std::endl;
      std::cout << "MultiData " << mutilIdx
                << "   c[2]_PSNR_Ave           : " << m_colorPSNR[mutilIdx][2] << std::endl;
      if (para.m_showHausdorff) {
        std::cout << "MultiData " << mutilIdx
                  << "   c[0]_HausdorffPSNR_Ave  : " << m_colorHausdorffPSNR[mutilIdx][0]
                  << std::endl;
        std::cout << "MultiData " << mutilIdx
                  << "   c[1]_HausdorffPSNR_Ave  : " << m_colorHausdorffPSNR[mutilIdx][1]
                  << std::endl;
        std::cout << "MultiData " << mutilIdx
                  << "   c[2]_HausdorffPSNR_Ave  : " << m_colorHausdorffPSNR[mutilIdx][2]
                  << std::endl;
      }
    }
  }
  if (para.m_calReflectance) {
    for (int mutilIdx = 0; mutilIdx < refMultiDataSize; ++mutilIdx) {
      std::cout << "MultiData " << mutilIdx
                << "   rel_PSNR_Ave            : " << m_reflectancePSNR[mutilIdx]
                << std::endl;
      if (para.m_showHausdorff) {
        std::cout << "MultiData " << mutilIdx
                  << "   rel_HausdroffPSNR_Ave   : " << m_reflectanceHausdorffPSNR[mutilIdx] << std::endl;
      }
    }
  }
  if (para.m_calLossless) {
    std::cout << "   All frames lossless compression check status: " << m_resultLossless
              << std::endl;
  }
  std::cout << std::endl;
  cout.unsetf(ios::fixed);
}
TMetricRes TMetricRes::operator+(TMetricRes r) {
  TMetricRes res;
  res.m_resultLossless = m_resultLossless & r.m_resultLossless;
  res.m_geometryPSNR = m_geometryPSNR + r.m_geometryPSNR;
  res.m_geometryHausdorffPSNR = m_geometryHausdorffPSNR + r.m_geometryHausdorffPSNR;
  for (int idx = 0; idx < NUM_MULTIATTRIBUTE; ++idx) {
    res.m_colorPSNR[idx][0] = m_colorPSNR[idx][0] + r.m_colorPSNR[idx][0];
    res.m_colorPSNR[idx][1] = m_colorPSNR[idx][1] + r.m_colorPSNR[idx][1];
    res.m_colorPSNR[idx][2] = m_colorPSNR[idx][2] + r.m_colorPSNR[idx][2];
    res.m_colorHausdorffPSNR[idx][0] =
      m_colorHausdorffPSNR[idx][0] + r.m_colorHausdorffPSNR[idx][0];
    res.m_colorHausdorffPSNR[idx][1] =
      m_colorHausdorffPSNR[idx][1] + r.m_colorHausdorffPSNR[idx][1];
    res.m_colorHausdorffPSNR[idx][2] =
      m_colorHausdorffPSNR[idx][2] + r.m_colorHausdorffPSNR[idx][2];
    res.m_reflectancePSNR[idx] = m_reflectancePSNR[idx] + r.m_reflectancePSNR[idx];
    res.m_reflectanceHausdorffPSNR[idx] =
      m_reflectanceHausdorffPSNR[idx] + r.m_reflectanceHausdorffPSNR[idx];
  }
  return res;
}

TMetricRes TMetricRes::operator/(unsigned int numOfFrames) {
  TMetricRes res;
  res.m_resultLossless = m_resultLossless;
  res.m_geometryPSNR = m_geometryPSNR / numOfFrames;
  res.m_geometryHausdorffPSNR = m_geometryHausdorffPSNR / numOfFrames;
  for (int idx = 0; idx < NUM_MULTIATTRIBUTE; ++idx) {
    res.m_colorPSNR[idx][0] = m_colorPSNR[idx][0] / numOfFrames;
    res.m_colorPSNR[idx][1] = m_colorPSNR[idx][1] / numOfFrames;
    res.m_colorPSNR[idx][2] = m_colorPSNR[idx][2] / numOfFrames;
    res.m_colorHausdorffPSNR[idx][0] = m_colorHausdorffPSNR[idx][0] / numOfFrames;
    res.m_colorHausdorffPSNR[idx][1] = m_colorHausdorffPSNR[idx][1] / numOfFrames;
    res.m_colorHausdorffPSNR[idx][2] = m_colorHausdorffPSNR[idx][2] / numOfFrames;
    res.m_reflectancePSNR[idx] = m_reflectancePSNR[idx] / numOfFrames;
    res.m_reflectanceHausdorffPSNR[idx] = m_reflectanceHausdorffPSNR[idx] / numOfFrames;
  }
  return res;
}

struct hash_name {
  size_t operator()(const PC_POS& p) const {
    return hash<Double>()(p[0]) ^ hash<Double>()(p[1]) ^ hash<Double>()(p[2]);
  }
};
struct equalTo {
  bool operator()(const PC_POS& a, const PC_POS& b) const {
    return abs(a[0] - b[0]) < 1e-8 && abs(a[1] - b[1]) < 1e-8 && abs(a[2] - b[2]) < 1e-8;
  }
};

void RGB2YUV_BT709(const PC_COL& rgb, PC_COL& yuv) {
  //! convert RGB to YUV by BT709 (HDTV)
  /* yuv[0] = float((0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2]) / 255.0);
  yuv[1] = float((-0.1146 * rgb[0] - 0.3854 * rgb[1] + 0.5000 * rgb[2]) / 255.0 + 0.5000);
  yuv[2] = float((0.5000 * rgb[0] - 0.4542 * rgb[1] - 0.0458 * rgb[2]) / 255.0 + 0.5000);*/
  const double y =
    TComClip(0., 255., std::round(0.212600 * rgb[0] + 0.715200 * rgb[1] + 0.072200 * rgb[2]));
  const double u =
    TComClip(0., 255., std::round(-0.114572 * rgb[0] - 0.385428 * rgb[1] + 0.5 * rgb[2] + 128.0));
  const double v =
    TComClip(0., 255., std::round(0.5 * rgb[0] - 0.454153 * rgb[1] - 0.045847 * rgb[2] + 128.0));
  yuv[0] = static_cast<uint8_t>(y);
  yuv[1] = static_cast<uint8_t>(u);
  yuv[2] = static_cast<uint8_t>(v);
}

float getPSNR(float MSE, float peakValue, float factor) {
  double maxEnergy = double(peakValue) * double(peakValue);
  float PSNR = float(10.0 * std::log10((double(factor) * maxEnergy) / double(MSE)));
  return PSNR;
}

double dist2(float dist) {
  return dist * dist;
}

bool checkPointCloud(const TComPointCloud& in, TComPointCloud& out, TMetricCfg& para) {
  if (!in.getNumPoint()) {
    cout << "ERROR: point cloud is empty!" << endl;
    return false;
  }
  if (!in.hasColors() && para.m_calColor) {
    cout << "WARNING: point cloud has no color!" << endl;
    para.m_calColor = false;
  }

  if (!in.hasReflectances() && para.m_calReflectance) {
    cout << "WARNING: point cloud has no reflectance!" << endl;
    para.m_calReflectance = false;
  }

  out = in;
  if (para.m_duplicateMode) {
    std::unordered_map<PC_POS, std::vector<size_t>, hash_name, equalTo> dic;
    for (size_t i = 0; i < in.getNumPoint(); i++) {
      if (dic.find(in[i]) != dic.end())
        dic[in[i]].push_back(i);
      else
        dic.insert(pair<PC_POS, vector<size_t>>(in[i], {i}));
    }

    out.setNumPoint(dic.size());
    size_t count = 0;
    for (auto ele : dic) {
      assert(count < dic.size());
      out[count] = ele.first;
      if (para.m_calColor) {
        for (int multiIdx = 0; multiIdx < in.getNumMultilColor(); ++multiIdx) {
          double colorSum[3] = {0, 0, 0};
          for (size_t i = 0; i < ele.second.size(); i++) {
            colorSum[0] += in.getColor(ele.second[i], multiIdx)[0];
            colorSum[1] += in.getColor(ele.second[i], multiIdx)[1];
            colorSum[2] += in.getColor(ele.second[i], multiIdx)[2];
          }
          out.getColor(count, multiIdx)[0] = UInt8(colorSum[0] / ele.second.size());
          out.getColor(count, multiIdx)[1] = UInt8(colorSum[1] / ele.second.size());
          out.getColor(count, multiIdx)[2] = UInt8(colorSum[2] / ele.second.size());
        }
      }

      if (para.m_calReflectance) {
        for (int multiIdx = 0; multiIdx < in.getNumMultilRefl(); ++multiIdx) {
          double reflSum = 0;
          for (size_t i = 0; i < ele.second.size(); i++) {
            reflSum += in.getReflectance(ele.second[i], multiIdx);
          }
          out.getReflectance(count, multiIdx) = PC_REFL(reflSum / ele.second.size());
        }
      }
      count++;
    }
  }
  size_t numin = in.getNumPoint();
  size_t numout = out.getNumPoint();
  if (numin - numout > 0) {
    if (!para.m_duplicateMode)
      cout << " Same coordinates points size: " << numin - numout << endl;
    else
      cout << " Same coordinates points size (averaged): " << numin - numout << endl;
    cout << " points remained:  " << numout << endl;
  } else {
    cout << " point cloud size: " << numin << endl;
  }
  return true;
}

float computePeakValue(const TComPointCloud& src) {
  float maxDistance = 0;
  KdTree kdtree = KdTree(src.positions(), 10);
  NNResult result;
  for (int i = 0; i < src.getNumPoint(); i++) {
    kdtree.search(src[i], 3, result);
    if (result.indices(0) != i || result.dist(1) < 1e-10) {
      //cout << "WARNING:  Some points are duplicated!" << endl;
    } else {
      if (result.dist(1) > maxDistance)
        maxDistance = float(result.dist(1));
    }
  }
  return sqrt(maxDistance);
}

void computeEValue(const TComPointCloud& src, const TComPointCloud& dis, TMetricCfg& para,
                   TMetricRes& res) {
  double geoMaxSquareErr = 0;
  double geoSumSquareErr = 0;
  double colorMaxSquareErr[NUM_MULTIATTRIBUTE][3] = {{0}};
  double colorSumSquareErr[NUM_MULTIATTRIBUTE][3] = {{0}};
  double reflMaxSquareErr[NUM_MULTIATTRIBUTE] = {0};
  double reflSumSquareErr[NUM_MULTIATTRIBUTE] = {0};
  int multilColorSize = para.m_calColor ? src.getNumMultilColor() : 0;
  int multilReflSize = para.m_calReflectance ? src.getNumMultilRefl() : 0;
  size_t resultmax = 30 > src.getNumPoint() ? src.getNumPoint() : 30;
  KdTree kdtree = KdTree(dis.positions(), 10);
  NNResult result;
  for (int i = 0; i < src.getNumPoint(); i++) {
    int resultnum = 0;
    do {
      resultnum += 5;
      resultnum = resultnum > resultmax ? resultmax : resultnum;
      if (!kdtree.search(src[i], resultnum, result)) {
        cout << " WARNING: requested neighbors could not be found " << endl;
      }
    } while (result.dist(0) == result.dist(resultnum - 1) && (resultnum + 1) <= resultmax);

    int idx = result.indices(0);
    assert(idx >= 0);

    //! same distance points
    std::vector<size_t> multineighbour;
    multineighbour.push_back(idx);
    for (int j = 1; j < resultnum; j++) {
      if (abs(result.dist(j) - result.dist(0)) < 1e-8) {
        multineighbour.push_back(result.indices(j));
      } else
        break;
    }

    //! compute geometry EValue
    double geoSquareErr = result.dist(0);
    geoSumSquareErr += geoSquareErr;
    geoMaxSquareErr = std::max(geoMaxSquareErr, geoSquareErr);

    //! compute color EValue
    double colorSquareErr[3] = {0, 0, 0};
    if (para.m_calColor) {
      for (int multilIdx = 0; multilIdx < multilColorSize; ++multilIdx) {
        PC_COL src_yuv, dis_yuv;
        double colorSquareErr[3] = {0, 0, 0};
        V3<int> originColor;
        originColor[0] = src.getColor(i, multilIdx)[0];
        originColor[1] = src.getColor(i, multilIdx)[1];
        originColor[2] = src.getColor(i, multilIdx)[2];

        RGB2YUV_BT709(src.getColor(i, multilIdx), src_yuv);
        if (para.m_multiNeighbourMode) {
          //! average
          PC_COL colorTemp;
          V3<int> dtsColor;
          double temp[3] = {0, 0, 0};
          for (auto& pnt : multineighbour) {
            temp[0] += dis.getColor(pnt, multilIdx)[0];
            temp[1] += dis.getColor(pnt, multilIdx)[1];
            temp[2] += dis.getColor(pnt, multilIdx)[2];
          }
          colorTemp[0] = (unsigned char)round(temp[0] / multineighbour.size());
          colorTemp[1] = (unsigned char)round(temp[1] / multineighbour.size());
          colorTemp[2] = (unsigned char)round(temp[2] / multineighbour.size());
          dtsColor[0] = colorTemp[0];
          dtsColor[1] = colorTemp[1];
          dtsColor[2] = colorTemp[2];
          RGB2YUV_BT709(colorTemp, dis_yuv);
          int dist = (originColor - dtsColor).getNorm1();
          colorMaxSquareErr[multilIdx][0] =
            std::max(colorMaxSquareErr[multilIdx][0],
                     dist2(float(src.getColor(i, multilIdx)[0] - colorTemp[0])));
          colorMaxSquareErr[multilIdx][1] =
            std::max(colorMaxSquareErr[multilIdx][1],
                     dist2(float(src.getColor(i, multilIdx)[1] - colorTemp[1])));
          colorMaxSquareErr[multilIdx][2] =
            std::max(colorMaxSquareErr[multilIdx][2],
                     dist2(float(src.getColor(i, multilIdx)[2] - colorTemp[2])));
        } else {
          RGB2YUV_BT709(dis.getColor(idx, multilIdx), dis_yuv);
          colorMaxSquareErr[multilIdx][0] =
            std::max(colorMaxSquareErr[multilIdx][0],
                     dist2(float(src.getColor(i, multilIdx)[0] - dis.getColor(idx, multilIdx)[0])));
          colorMaxSquareErr[multilIdx][1] =
            std::max(colorMaxSquareErr[multilIdx][1],
                     dist2(float(src.getColor(i, multilIdx)[1] - dis.getColor(idx, multilIdx)[1])));
          colorMaxSquareErr[multilIdx][2] =
            std::max(colorMaxSquareErr[multilIdx][2],
                     dist2(float(src.getColor(i, multilIdx)[2] - dis.getColor(idx, multilIdx)[2])));
        }
        colorSquareErr[0] = dist2(float(src_yuv[0] - dis_yuv[0]));
        colorSquareErr[1] = dist2(float(src_yuv[1] - dis_yuv[1]));
        colorSquareErr[2] = dist2(float(src_yuv[2] - dis_yuv[2]));
        colorSumSquareErr[multilIdx][0] += colorSquareErr[0];
        colorSumSquareErr[multilIdx][1] += colorSquareErr[1];
        colorSumSquareErr[multilIdx][2] += colorSquareErr[2];
      }
    }
    //! compute reflectance EValue
    double reflSquareErr = 0.0;
    if (para.m_calReflectance) {
      double reflTemp = 0;
      for (int multilIdx = 0; multilIdx < multilReflSize; ++multilIdx) {
        double reflTemp = 0;

        if (para.m_multiNeighbourMode) {
          for (auto& pnt : multineighbour) {
            reflTemp += dis.getReflectance(pnt, multilIdx);
          }
        }
        reflSquareErr =
          dist2(float(src.getReflectance(i, multilIdx) - round(reflTemp / multineighbour.size())));
        //reflSquareErr = dist2(src.refl(i) - dis.refl(idx));
        reflSumSquareErr[multilIdx] += reflSquareErr;
        reflMaxSquareErr[multilIdx] = std::max(reflMaxSquareErr[multilIdx], reflSquareErr);
      }
    }
  }
  //! compute geometry MSE and PSNR
  res.m_geometryMSE = float(geoSumSquareErr / src.getNumPoint());
  res.m_geometryHausdorff = float(geoMaxSquareErr);
  res.m_geometryPSNR = getPSNR(res.m_geometryMSE, res.m_peakValue, 3.0);
  res.m_geometryHausdorffPSNR = getPSNR(res.m_geometryHausdorff, res.m_peakValue, 3.0);
  //! compute colors MSE and PSNR
  if (para.m_calColor) {
    for (int multilIdx = 0; multilIdx < multilColorSize; ++multilIdx) {
      res.m_colorMSE[multilIdx][0] = float(colorSumSquareErr[multilIdx][0] / src.getNumPoint());
      res.m_colorMSE[multilIdx][1] = float(colorSumSquareErr[multilIdx][1] / src.getNumPoint());
      res.m_colorMSE[multilIdx][2] = float(colorSumSquareErr[multilIdx][2] / src.getNumPoint());
      res.m_colorPSNR[multilIdx][0] =
        getPSNR(res.m_colorMSE[multilIdx][0], float(std::numeric_limits<UInt8>::max()), 1.0);
      res.m_colorPSNR[multilIdx][1] =
        getPSNR(res.m_colorMSE[multilIdx][1], float(std::numeric_limits<UInt8>::max()), 1.0);
      res.m_colorPSNR[multilIdx][2] =
        getPSNR(res.m_colorMSE[multilIdx][2], float(std::numeric_limits<UInt8>::max()), 1.0);
      res.m_colorHausdorff[multilIdx][0] = float(colorMaxSquareErr[multilIdx][0]);
      res.m_colorHausdorff[multilIdx][1] = float(colorMaxSquareErr[multilIdx][1]);
      res.m_colorHausdorff[multilIdx][2] = float(colorMaxSquareErr[multilIdx][2]);
      res.m_colorHausdorffPSNR[multilIdx][0] =
        getPSNR(res.m_colorHausdorff[multilIdx][0], float(std::numeric_limits<UInt8>::max()), 1.0);
      res.m_colorHausdorffPSNR[multilIdx][1] =
        getPSNR(res.m_colorHausdorff[multilIdx][1], float(std::numeric_limits<UInt8>::max()), 1.0);
      res.m_colorHausdorffPSNR[multilIdx][2] =
        getPSNR(res.m_colorHausdorff[multilIdx][2], float(std::numeric_limits<UInt8>::max()), 1.0);
    }
  }
  //! compute reflectance MSE and PSNR
  if (para.m_calReflectance) {
    for (int multilIdx = 0; multilIdx < multilReflSize; ++multilIdx) {
      res.m_reflectanceMSE[multilIdx] = float(reflSumSquareErr[multilIdx] / src.getNumPoint());
      res.m_reflectancePSNR[multilIdx] =
        getPSNR(res.m_reflectanceMSE[multilIdx], float((1ll << para.m_reflOutputDepth) - 1), 1.0);
      res.m_reflectanceHausdorff[multilIdx] = float(reflMaxSquareErr[multilIdx]);
      res.m_reflectanceHausdorffPSNR[multilIdx] =
        getPSNR(res.m_reflectanceHausdorff[multilIdx], float(1ll << para.m_reflOutputDepth) - 1, 1.0);
    }
  }
}

bool compareLossless(const TComPointCloud& srcout, TComPointCloud& disout) {
  TSize pointNum = srcout.getNumPoint();
  std::vector<int32_t> OrderSrc(pointNum);
  std::vector<int32_t> OrderDis(pointNum);
  sortPointCloud(srcout, OrderSrc);
  sortPointCloud(disout, OrderDis);
  bool result = true;
  for (UInt count = 0; count < pointNum; count++) {
    if (disout[OrderDis[count]] != srcout[OrderSrc[count]]) {
      result = false;
      break;
    }
    if (disout.hasColors()) {
      for (int multiIdx = 0; multiIdx < disout.getNumMultilColor(); ++multiIdx)
        if (disout.getColor(OrderDis[count], multiIdx) !=
            srcout.getColor(OrderSrc[count], multiIdx)) {
          result = false;
          break;
        }
    }
    if (disout.hasReflectances()) {
      for (int multiIdx = 0; multiIdx < disout.getNumMultilRefl(); ++multiIdx)
        if (disout.getReflectance(OrderDis[count], multiIdx) !=
            srcout.getReflectance(OrderSrc[count], multiIdx)) {
          result = false;
          break;
        }
    }  
  }
  return result;
}

void sortPointCloud(const TComPointCloud& pc, std::vector<int32_t>& Order) {
  for (UInt idx = 0; idx < pc.getNumPoint(); idx++) {
    Order[idx] = idx;
  }
  if (!pc.hasColors() && !pc.hasReflectances()) {
    std::sort(Order.begin(), Order.end(), [&](int32_t& left, int32_t& right) {
      return pc[left][0] != pc[right][0]
        ? pc[left][0] < pc[right][0]
        : (pc[left][1] != pc[right][1] ? pc[left][1] < pc[right][1] : pc[left][2] < pc[right][2]);
    });
  }
  if (pc.hasColors()) {
    std::sort(Order.begin(), Order.end(), [&](int32_t& left, int32_t& right) {
      return pc[left][0] != pc[right][0]
        ? pc[left][0] < pc[right][0]
        : (pc[left][1] != pc[right][1]
             ? pc[left][1] < pc[right][1]
             : (pc[left][2] != pc[right][2]
                  ? pc[left][2] < pc[right][2]
                  : mortonAddr((Int32)pc.getColor(left, 0)[0], (Int32)pc.getColor(left, 0)[1],
                               (Int32)pc.getColor(left, 0)[2]) <
                    mortonAddr((Int32)pc.getColor(right, 0)[0], (Int32)pc.getColor(right, 0)[1],
                               (Int32)pc.getColor(right, 0)[2])));
    });
  }
  if (pc.hasReflectances()) {
    std::sort(Order.begin(), Order.end(), [&](int32_t& left, int32_t& right) {
      return pc[left][0] != pc[right][0]
        ? pc[left][0] < pc[right][0]
        : (pc[left][1] != pc[right][1] ? pc[left][1] < pc[right][1] :
                                       (pc[left][2] != pc[right][2] ? pc[left][2] < pc[right][2]
                                                                    : pc.getReflectance(left, 0) <
                                            pc.getReflectance(right, 0)));
    });
  }
}

bool computeMetric(const TComPointCloud& src, TComPointCloud& dis, TMetricCfg& para,
                   TMetricRes& res) {
  clock_t pc_evalue_TimeBegin = clock();
  cout << "Checking original point cloud..." << endl;
  TComPointCloud srcout;
  if (!checkPointCloud(src, srcout, para)) {
    cout << "ERROR: Load reference point cloud failed!" << endl;
    return false;
  }
  cout << "Checking reconstruct point cloud..." << endl;
  TComPointCloud disout;
  if (!checkPointCloud(dis, disout, para)) {
    cout << "ERROR: Load distortion point cloud failed!" << endl;
    return false;
  }

  //! compute geometry and attribute metrics
  TMetricRes resA;
  if (!para.m_peakValue) {
    resA.m_peakValue = computePeakValue(src);
    cout << "peakValue is set " << resA.m_peakValue << endl;
  } else {
    resA.m_peakValue = para.m_peakValue;
    cout << "peakValue: " << resA.m_peakValue << endl;
  }
  computeEValue(srcout, disout, para, resA);

  cout.setf(ios::fixed);
  cout.precision(6);
  cout << endl << "1. Take original point cloud as reference:" << endl;
  cout << "   D1_MSE_1           : " << resA.m_geometryMSE << endl;
  cout << "   D1_PSNR_1          : " << resA.m_geometryPSNR << endl;
  if (para.m_showHausdorff) {
    cout << "   D1_Hausdorff_1     : " << resA.m_geometryHausdorff << endl;
    cout << "   D1_HausdorffPSNR_1 : " << resA.m_geometryHausdorffPSNR << endl;
  }
  if (para.m_calColor) {
    for (int multilIdx = 0; multilIdx < src.getNumMultilColor(); ++multilIdx) {
      cout << "MultiData " << multilIdx
           << "   c[0]_MSE_1            : " << resA.m_colorMSE[multilIdx][0] << endl;
      cout << "MultiData " << multilIdx
           << "   c[1]_MSE_1            : " << resA.m_colorMSE[multilIdx][1] << endl;
      cout << "MultiData " << multilIdx
           << "   c[2]_MSE_1            : " << resA.m_colorMSE[multilIdx][2] << endl;
      cout << "MultiData " << multilIdx
           << "   c[0]_PSNR_1           : " << resA.m_colorPSNR[multilIdx][0] << endl;
      cout << "MultiData " << multilIdx
           << "   c[1]_PSNR_1           : " << resA.m_colorPSNR[multilIdx][1] << endl;
      cout << "MultiData " << multilIdx
           << "   c[2]_PSNR_1           : " << resA.m_colorPSNR[multilIdx][2] << endl;
      if (para.m_showHausdorff) {
        cout << "MultiData " << multilIdx
             << "   c[0]_Hausdorff_1      : " << resA.m_colorHausdorff[multilIdx][0] << endl;
        cout << "MultiData " << multilIdx
             << "   c[1]_Hausdorff_1      : " << resA.m_colorHausdorff[multilIdx][1] << endl;
        cout << "MultiData " << multilIdx
             << "   c[2]_Hausdorff_1      : " << resA.m_colorHausdorff[multilIdx][2] << endl;
        cout << "MultiData " << multilIdx
             << "   c[0]_HausdorffPSNR_1  : " << resA.m_colorHausdorffPSNR[multilIdx][0] << endl;
        cout << "MultiData " << multilIdx
             << "   c[1]_HausdorffPSNR_1  : " << resA.m_colorHausdorffPSNR[multilIdx][1] << endl;
        cout << "MultiData " << multilIdx
             << "   c[2]_HausdorffPSNR_1  : " << resA.m_colorHausdorffPSNR[multilIdx][2] << endl;
      }
    }
  }
  if (para.m_calReflectance) {
    for (int multilIdx = 0; multilIdx < src.getNumMultilRefl(); ++multilIdx) {
      cout << "MultiData " << multilIdx
           << "   rel_MSE_1             : " << resA.m_reflectanceMSE[multilIdx] << endl;
      cout << "MultiData " << multilIdx
           << "   rel_PSNR_1            : " << resA.m_reflectancePSNR[multilIdx] << endl;
      if (para.m_showHausdorff) {
        cout << "MultiData " << multilIdx
             << "   rel_Hausdroff_1       : " << resA.m_reflectanceHausdorff[multilIdx] << endl;
        cout << "MultiData " << multilIdx
             << "   rel_HausdroffPSNR_1   : " << resA.m_reflectanceHausdorffPSNR[multilIdx] << endl;
      }
    }
  }

  if (para.m_symmetry) {
    TMetricRes resB;
    if (!para.m_peakValue)
      resB.m_peakValue = computePeakValue(src);
    else
      resB.m_peakValue = para.m_peakValue;
    computeEValue(disout, srcout, para, resB);
    cout << endl << "2. Take reconstruct point cloud as reference:" << endl;
    cout << "   D1_MSE_2           : " << resB.m_geometryMSE << endl;
    cout << "   D1_PSNR_2          : " << resB.m_geometryPSNR << endl;
    if (para.m_showHausdorff) {
      cout << "   D1_Hausdorff_2     : " << resB.m_geometryHausdorff << endl;
      cout << "   D1_HausdorffPSNR_2 : " << resB.m_geometryHausdorffPSNR << endl;
    }

    if (para.m_calColor) {
      for (int multilIdx = 0; multilIdx < src.getNumMultilColor(); ++multilIdx) {
        cout << "MultiData " << multilIdx
             << "   c[0]_MSE_2            : " << resB.m_colorMSE[multilIdx][0] << endl;
        cout << "MultiData " << multilIdx
             << "   c[1]_MSE_2            : " << resB.m_colorMSE[multilIdx][1] << endl;
        cout << "MultiData " << multilIdx
             << "   c[2]_MSE_2            : " << resB.m_colorMSE[multilIdx][2] << endl;
        cout << "MultiData " << multilIdx
             << "   c[0]_PSNR_2           : " << resB.m_colorPSNR[multilIdx][0] << endl;
        cout << "MultiData " << multilIdx
             << "   c[1]_PSNR_2           : " << resB.m_colorPSNR[multilIdx][1] << endl;
        cout << "MultiData " << multilIdx
             << "   c[2]_PSNR_2           : " << resB.m_colorPSNR[multilIdx][2] << endl;
        if (para.m_showHausdorff) {
          cout << "MultiData " << multilIdx
               << "   c[0]_Hausdorff_2      : " << resB.m_colorHausdorff[multilIdx][0] << endl;
          cout << "MultiData " << multilIdx
               << "   c[1]_Hausdorff_2      : " << resB.m_colorHausdorff[multilIdx][1] << endl;
          cout << "MultiData " << multilIdx
               << "   c[2]_Hausdorff_2      : " << resB.m_colorHausdorff[multilIdx][2] << endl;
          cout << "MultiData " << multilIdx
               << "   c[0]_HausdorffPSNR_2  : " << resB.m_colorHausdorffPSNR[multilIdx][0] << endl;
          cout << "MultiData " << multilIdx
               << "   c[1]_HausdorffPSNR_2  : " << resB.m_colorHausdorffPSNR[multilIdx][1] << endl;
          cout << "MultiData " << multilIdx
               << "   c[2]_HausdorffPSNR_2  : " << resB.m_colorHausdorffPSNR[multilIdx][2] << endl;
        }
      }
    }
    if (para.m_calReflectance) {
      for (int multilIdx = 0; multilIdx < src.getNumMultilRefl(); ++multilIdx) {
        cout << "MultiData " << multilIdx
             << "   rel_MSE_2             : " << resB.m_reflectanceMSE[multilIdx] << endl;
        cout << "MultiData " << multilIdx
             << "   rel_PSNR_2            : " << resB.m_reflectancePSNR[multilIdx] << endl;
        if (para.m_showHausdorff) {
          cout << "MultiData " << multilIdx
               << "   rel_Hausdroff_2       : " << resB.m_reflectanceHausdorff[multilIdx] << endl;
          cout << "MultiData " << multilIdx
               << "   rel_HausdroffPSNR_2   : " << resB.m_reflectanceHausdorffPSNR[multilIdx]
               << endl;
        }
      }
    }
    res.m_geometryMSE = max(resA.m_geometryMSE, resB.m_geometryMSE);
    res.m_geometryPSNR = min(resA.m_geometryPSNR, resB.m_geometryPSNR);
    res.m_geometryHausdorff = max(resA.m_geometryHausdorff, resB.m_geometryHausdorff);
    res.m_geometryHausdorffPSNR = min(resA.m_geometryHausdorffPSNR, resB.m_geometryHausdorffPSNR);
    if (para.m_calColor) {
      for (int multilIdx = 0; multilIdx < src.getNumMultilColor(); ++multilIdx) {
        res.m_colorMSE[multilIdx][0] =
          max(resA.m_colorMSE[multilIdx][0], resB.m_colorMSE[multilIdx][0]);
        res.m_colorMSE[multilIdx][1] =
          max(resA.m_colorMSE[multilIdx][1], resB.m_colorMSE[multilIdx][1]);
        res.m_colorMSE[multilIdx][2] =
          max(resA.m_colorMSE[multilIdx][2], resB.m_colorMSE[multilIdx][2]);
        res.m_colorPSNR[multilIdx][0] =
          min(resA.m_colorPSNR[multilIdx][0], resB.m_colorPSNR[multilIdx][0]);
        res.m_colorPSNR[multilIdx][1] =
          min(resA.m_colorPSNR[multilIdx][1], resB.m_colorPSNR[multilIdx][1]);
        res.m_colorPSNR[multilIdx][2] =
          min(resA.m_colorPSNR[multilIdx][2], resB.m_colorPSNR[multilIdx][2]);
        res.m_colorHausdorff[multilIdx][0] =
          max(resA.m_colorHausdorff[multilIdx][0], resB.m_colorHausdorff[multilIdx][0]);
        res.m_colorHausdorff[multilIdx][1] =
          max(resA.m_colorHausdorff[multilIdx][1], resB.m_colorHausdorff[multilIdx][1]);
        res.m_colorHausdorff[multilIdx][2] =
          max(resA.m_colorHausdorff[multilIdx][2], resB.m_colorHausdorff[multilIdx][2]);
        res.m_colorHausdorffPSNR[multilIdx][0] =
          min(resA.m_colorHausdorffPSNR[multilIdx][0], resB.m_colorHausdorffPSNR[multilIdx][0]);
        res.m_colorHausdorffPSNR[multilIdx][1] =
          min(resA.m_colorHausdorffPSNR[multilIdx][1], resB.m_colorHausdorffPSNR[multilIdx][1]);
        res.m_colorHausdorffPSNR[multilIdx][2] =
          min(resA.m_colorHausdorffPSNR[multilIdx][2], resB.m_colorHausdorffPSNR[multilIdx][2]);
      }
    }
    if (para.m_calReflectance) {
      for (int multilIdx = 0; multilIdx < src.getNumMultilRefl(); ++multilIdx) {
        res.m_reflectanceMSE[multilIdx] =
          max(resA.m_reflectanceMSE[multilIdx], resB.m_reflectanceMSE[multilIdx]);
        res.m_reflectancePSNR[multilIdx] =
          min(resA.m_reflectancePSNR[multilIdx], resB.m_reflectancePSNR[multilIdx]);
        res.m_reflectanceHausdorff[multilIdx] =
          max(resA.m_reflectanceHausdorff[multilIdx], resB.m_reflectanceHausdorff[multilIdx]);
        res.m_reflectanceHausdorffPSNR[multilIdx] = min(resA.m_reflectanceHausdorffPSNR[multilIdx],
                                                        resB.m_reflectanceHausdorffPSNR[multilIdx]);
      }
    }

    cout << endl << "3. Symmetric result:" << endl;
    cout << "   D1_MSE_F           : " << res.m_geometryMSE << endl;
    cout << "   D1_PSNR_F          : " << res.m_geometryPSNR << endl;
    if (para.m_showHausdorff) {
      cout << "   D1_Hausdorff_F     : " << res.m_geometryHausdorff << endl;
      cout << "   D1_HausdorffPSNR_F : " << res.m_geometryHausdorffPSNR << endl;
    }
    if (para.m_calColor) {
      for (int multilIdx = 0; multilIdx < src.getNumMultilColor(); ++multilIdx) {
        cout << "MultiData " << multilIdx
             << "   c[0]_MSE_F            : " << res.m_colorMSE[multilIdx][0] << endl;
        cout << "MultiData " << multilIdx
             << "   c[1]_MSE_F            : " << res.m_colorMSE[multilIdx][1] << endl;
        cout << "MultiData " << multilIdx
             << "   c[2]_MSE_F            : " << res.m_colorMSE[multilIdx][2] << endl;
        cout << "MultiData " << multilIdx
             << "   c[0]_PSNR_F           : " << res.m_colorPSNR[multilIdx][0] << endl;
        cout << "MultiData " << multilIdx
             << "   c[1]_PSNR_F           : " << res.m_colorPSNR[multilIdx][1] << endl;
        cout << "MultiData " << multilIdx
             << "   c[2]_PSNR_F           : " << res.m_colorPSNR[multilIdx][2] << endl;
        if (para.m_showHausdorff) {
          cout << "MultiData " << multilIdx
               << "   c[0]_Hausdorff_F      : " << res.m_colorHausdorff[multilIdx][0] << endl;
          cout << "MultiData " << multilIdx
               << "   c[1]_Hausdorff_F      : " << res.m_colorHausdorff[multilIdx][1] << endl;
          cout << "MultiData " << multilIdx
               << "   c[2]_Hausdorff_F      : " << res.m_colorHausdorff[multilIdx][2] << endl;
          cout << "MultiData " << multilIdx
               << "   c[0]_HausdorffPSNR_F  : " << res.m_colorHausdorffPSNR[multilIdx][0] << endl;
          cout << "MultiData " << multilIdx
               << "   c[1]_HausdorffPSNR_F  : " << res.m_colorHausdorffPSNR[multilIdx][1] << endl;
          cout << "MultiData " << multilIdx
               << "   c[2]_HausdorffPSNR_F  : " << res.m_colorHausdorffPSNR[multilIdx][2] << endl;
        }
      }
    }
    if (para.m_calReflectance) {
      for (int multilIdx = 0; multilIdx < src.getNumMultilRefl(); ++multilIdx) {
        cout << "MultiData " << multilIdx
             << "   rel_MSE_F             : " << res.m_reflectanceMSE[multilIdx] << endl;
        cout << "MultiData " << multilIdx
             << "   rel_PSNR_F            : " << res.m_reflectancePSNR[multilIdx] << endl;
        if (para.m_showHausdorff) {
          cout << "MultiData " << multilIdx
               << "   rel_Hausdroff_F       : " << res.m_reflectanceHausdorff[multilIdx] << endl;
          cout << "MultiData " << multilIdx
               << "   rel_HausdroffPSNR_F   : " << res.m_reflectanceHausdorffPSNR[multilIdx]
               << endl;
        }
      }
    }
  }

  //! compute losssless metrics
  if (para.m_calLossless) {
    res.m_resultLossless = false;
    if (src.getNumPoint() == dis.getNumPoint()) {
      res.m_resultLossless = compareLossless(src, dis);
    }
    cout << endl << "Lossless compression check status: " << res.m_resultLossless << endl;
  }

  cout.unsetf(ios::fixed);
  Double userTime = (Double)(clock() - pc_evalue_TimeBegin) / CLOCKS_PER_SEC;
  cout << "Point cloud evalue processing time (user): " << userTime << " sec." << endl << endl;
  return true;
}
}  // namespace pc_evalue
