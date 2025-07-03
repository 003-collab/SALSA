/*
	Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
	at Austin (ARL:UT).
	
	SALSA is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License version 3 (GPL-3.0-only) as published
	by the Free Software Foundation.
	
	SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along with
	SALSA.  If not, see https://www.gnu.org/licenses/.
*/
#ifndef LSAINVERSE_HPP
#define LSAINVERSE_HPP

// system includes
#include <ctime>
#include <set>
#include <string>
#include <vector>
// gnsstk
#include "Epoch.hpp"
//#include "StringUtils.hpp"
#include <Point.hpp>
#include "singleton.hpp"
#include "expandtilde.hpp"
#include "logstream.hpp"
#include "CommandLine.hpp"
//LSA
#include <lsah5.hpp>
#include "lsaextr.hpp"
#include <lsaUtils.hpp>
#include <LSAConstants.hpp>
//Qt
#include <QFileInfo>
// WGS84 ellipsoid
#include <WGS84Ellipsoid.hpp>
// Eigen
// pragma commands used to squelch useless warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <Eigen/Dense>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

//------------------------------------------------------------------------------------
using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
// disable some MSVC compiler warnings
#pragma warning(disable:4290)

//------------------------------------------------------------------------------------
struct lsainverse_output
{
    std::string fromStationName;
    std::string fromGeodeticLatitudeDMS;
    std::string fromGeodeticLongitudeDMS;
    std::string fromEllipsoidalHeight;
    std::string fromOrthometricHeight;
    std::string fromDoVN;
    std::string fromDoVE;
    std::string fromUndulation;
    std::string toStationName;
    std::string toGeodeticLatitudeDMS;
    std::string toGeodeticLongitudeDMS;
    std::string toEllipsoidalHeight;
    std::string toOrthometricHeight;
    std::string toDoVN;
    std::string toDoVE;
    std::string toUndulation;
    std::string slantDistance;
    std::string geodesicDistance;
    std::string horizontalDistance;
    std::string azimuth;
    std::string verticalAngle;
    std::string heightDifference;
    std::string dXdYdZ;
    std::string deltaCovariance;
};

// Class GlobalDataLsapost encapsulates global static data.
class GlobalDataLsainverse : public gnsstk::Singleton<GlobalDataLsainverse> {
public:
   // Default and only constructor, sets defaults.
   GlobalDataLsainverse() throw() { SetDefaults(); }

   // prgm housekeeping
   static const std::string Version;   // see below
   std::string PrgmName;               // this program (must match Jamfile)
   std::string Title;                  ///< name, version and runtime
   std::ofstream oflog;                ///< output log file stream

   // command line input ----------------------------------------------------------
   bool inputIsValid;

   std::string cmdlineErrors,cmdlineDump,cmdlineUsage;  // strings filled by parser
   std::vector<std::string> cmdlineUnrecognized;

   //input file
   std::string hdf5path,hdf5FileName;///< paths for input files

   //relevant stations
   std::string fromStation;
   std::string toStation;

   //optional output file
   std::string logpath, logfile;///< name of log file

   bool scaleByAPV;
   bool westLon;
   bool GUIOutput;
   double APV;

   bool verbose,help;///< output switches (don't currently do anything)
   // end command line input ------------------------------------------------------

   bool stationIsFloatUnused;
   bool invalidCov;//Bug #1364

   // object used to hold and write data to the HDF5 file
   LSAH5File hdf5File;

   llhExternal fromLLH, toLLH;

   //containers to hold .h5 content to pass to Extraction class
   vector<Extraction> Extrs;
   std::map<std::string,Point> Points;
   gnsstk::Namelist StateNames;
   gnsstk::Matrix<double> Covariance;

   //extracted quantities
   bool azimuthIsNeg;
   int azimuthDeg;
   int azimuthMin;
   double azimuthSec;
   double azimuthSigma;//soa
   double deltaX, deltaY, deltaZ;//m
   double deltaXSigma, deltaYSigma, deltaZSigma;//m
   double deltaCovXX, deltaCovXY, deltaCovXZ, deltaCovYY, deltaCovYZ, deltaCovZZ;
   double slantDistance;//m
   double slantDistanceSigma;//m
   double horzDistance;//m
   double horzDistanceSigma;//m
   double geodesicDistance;//m
   double geodesicDistanceSigma;//m
   double heightDiff;//m
   double heightDiffSigma;//m
   bool verticalAngleIsNeg;
   int verticalAngleDeg;
   int verticalAngleMin;
   double verticalAngleSec;
   double verticalAngleSigma;//soa

   lsainverse_output outputLabels;
   lsainverse_output outputValues;
   lsainverse_output outputCombined;

private:
   void SetDefaults(void) throw()
   {
      PrgmName = std::string("lsainverse");

      // command line input -------------------------------------------
      logfile = std::string();
      hdf5FileName = std::string();
      fromStation = std::string();
      toStation = std::string();

      help = verbose = stationIsFloatUnused = scaleByAPV = GUIOutput = false;
      // end command line input ---------------------------------------

      Points.clear();
      StateNames.clear();

      APV = 0.0;
      azimuthDeg = azimuthMin = verticalAngleDeg = verticalAngleMin = 0;
      azimuthSec = azimuthSigma = verticalAngleSec = verticalAngleSigma = 0.0;
      deltaX = deltaY = deltaZ = deltaCovXX = deltaCovXY = deltaCovXZ = deltaCovYY = deltaCovYZ = deltaCovZZ = 0.0;
      deltaXSigma = deltaYSigma = deltaZSigma = 0.0;
      slantDistance = slantDistanceSigma = horzDistance = horzDistanceSigma = geodesicDistance = geodesicDistanceSigma = 0.0;
      heightDiff = heightDiffSigma =0.0;
      verticalAngleIsNeg = azimuthIsNeg = westLon = invalidCov = false;
   }

}; // end class GlobalDataLsainverse

enum INVERSE_RETURNS
{
    DATA_OBJECT_UNCREATED = -5,
    COMMAND_LINE_DEF = -3,
    BAD_ALLOCATION = -2,
    LSAINVERSE_ERROR = -1,
    LSAINVERSE_OK = 0,
    COMMAND_LINE_USAGE,
    COMMAND_LINE_ERRORS,
    USER_VALIDATION,
    INVALID_INPUT,
    LOG_FILE_UNOPENED,
    HDF5_FILE_UNOPENED,
    HDF5_READ_FAILURE,
    HDF5_FILE_VERSION,
    HDF5_NO_DATA,
    INV_FILE_ERROR,
    BAD_CSV_OPEN
};


int getCommandLine(int argc, char **argv) throw(Exception);
int preProcess(void) throw(Exception);
int Process(void) throw(Exception);
int readHDF5File(void) throw(Exception);
int validateInput(void) throw(Exception);
int outputResults(void) throw(Exception);
void formatOutput(void) throw(Exception);
int computeExtraction(void) throw(Exception);
void computeSlantDistanceAndSigma(void) throw(Exception);
void computeHorizontalDistance(void) throw(Exception);
void computeGeodesicDistance(void) throw(Exception);
Eigen::VectorXd computeSlantDistance(Eigen::VectorXd Pos1and2) throw(Exception);
Eigen::VectorXd computeGeodesic(Eigen::VectorXd Pos1and2) throw(Exception);
Eigen::VectorXd computeHorizontal(Eigen::VectorXd Pos1and2) throw(Exception);
gnsstk::Matrix<double> ExtractFromAndToStationCov(void) throw(Exception);
// throw(Exception) not needed here, may not be needed for any of these declarations
double MapECEFcovToGeodesicSigma(gnsstk::Vector<double> Pos1,gnsstk::Vector<double> Pos2,gnsstk::Matrix<double> CovXYZfromAndto); // throw(Exception);

int writeOutputCSV();
bool nonEmpty(const std::string &fileName);
std::string splitAndJoin(const std::string &origString, bool isAngle = false, char delim = ' ', char join = ',');
gnsstk::Vector<double> generateViaPosition(const gnsstk::Position &);
Eigen::MatrixXd castMatrixgnsstktoEigen(const gnsstk::Matrix<double> &gnsstkMatrix);

#endif // LSAINVERSE_HPP
