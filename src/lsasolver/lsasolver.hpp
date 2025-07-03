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
/// @file lsasolver.hpp  Global include file for ARL:UT LSA least squares solver.

#ifndef LSA_SOLVER_INCLUDE
#define LSA_SOLVER_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

// system includes
#include <ostream>
#include <string>
#include <vector>
#include <map>

// GNSSTk
#include "Exception.hpp"
#include "StringUtils.hpp"
#include "Epoch.hpp"
#include "stl_helpers.hpp"
#include "Stats.hpp"
#include "singleton.hpp"
#include "ref_ptr.hpp"
#include "ENUUtil.hpp"
#include "SunEarthSatGeometry.hpp"

// geomatics
#include "expandtilde.hpp"
#include "logstream.hpp"
#include "CommandLine.hpp"
#include "Namelist.hpp"
#include "SRI.hpp"

// lsa
#include "MatrixVector.hpp"      // keep this the first lsa include

#include "Point.hpp"
#include "HAngle.hpp"
#include "VAngle.hpp"
#include "Azimuth.hpp"
#include "Delta.hpp"
#include "Dir.hpp"
#include "Dist.hpp"
#include "Height.hpp"
#include "ZAngle.hpp"

#include "DATfile.hpp"
#include "DirSet.hpp"
#include "PointMeas.hpp"

#include "lsaUtils.hpp"
#include "lsabinary.hpp"
#include "geoid.hpp"

//------------------------------------------------------------------------------------
using namespace gnsstk;
using namespace gnsstk::StringUtils;
//------------------------------------------------------------------------------------
/// Class GlobalData encapsulates global static data for program lsasolver.
class GlobalData : public gnsstk::Singleton<GlobalData> {
public:
   /// Default and only constructor, sets defaults.
   GlobalData() throw() { SetDefaults(); }

   // prgm housekeeping
   static const std::string Version;///< program version; see lsasolver.cpp
   std::string PrgmName;            ///< name of this program
   std::string RunStr;              ///< name, version and runtime
   clock_t totaltime;
   gnsstk::Epoch wallbegin,wallend;

   // strings filled by command line parser
   std::string cmdlineErrors;       ///< command line errors
   std::string cmdlineDump;         ///< command line summary
   std::string cmdlineUsage;        ///< command line usage
   /// vector of command line unrecognized arguments
   std::vector<std::string> cmdlineUnrecognized;

   // begin command line input ---------------------------
   std::string logfile;             ///< name of log file
   std::ofstream oflog;             ///< output log file stream

   bool inputIsValid;               ///< true if input is valid
   bool apquit;                     ///< if true, quit after computing a prioris
   bool doTiming;                   ///< if true, output timing information
   bool useSRIF;                    ///< if true, use SRIF solver
   bool forcefast;                  ///< placeholder for config file
   bool forcestable;                ///< if true, set useSRIF = true
   bool allowAPcentroid;            ///< if true, allow ComputeAPriori to do C-O-M
   bool statTests;                  ///< if true, do ChiSq and snooping every iter.
   bool noExtRelVect;               ///< if true do not calculate external reliability metrics

   std::string inpath;              ///< input path for input file
   std::string infile;              ///< input file name (.dat file)
   std::string logpath;             ///< input path for logfile
   std::string obseqnfile;          ///< output file name for obs.eqns.
   std::string binfile;             ///< file name for GUI output
   std::string compfile;            ///< compare with results in this file
   std::string outdatfile;          ///< write a complete DAT file after adjustment

   // use to overwrite DAT file input only
   bool cmd_noAPV;                  ///< if true, do not apply APV to covariance
   bool cmd_doAPV;                  ///< if true, do not apply APV to covariance
   int cmd_nitermax;                ///< cmdline limit on iteration number
   double cmd_converge;             ///< cmdline convergence criterion (unitless)

   // output details
   int linprecM;                     ///< output precision in measured linear quanitites
   int linprecP;                     ///< output precision in geocentric position
   int linwidth;                    ///< output width in linear quanitites
   // measurements and positions
   int angprecM;                    ///< output precision in measured angles (RAD)
   int angwidthM;                   ///< output width in measured angles
   int angprecP;                    ///< output precision in position angles (LLH-SOA)
   bool doWest;                     ///< output west longitude

   bool help;                       ///< if true, output syntax page and quit
   bool validate;                   ///< if true, check validity of input only
   bool verbose;                    ///< verbose output, if true
   int debug;                       ///< level of debug output, > -1 (the default)
   bool doProgress;                 ///< if true, write assurance output to stdout

   // input data (DAT) file
   bool noAPV;                      ///< if true, do not apply APV to covariance
   DATfile datfile;                 ///< all data from input .dat file
   std::string Title;               ///< optional title found in input .dat file
   int nitermax;                    ///< limit on iteration number
   double converge;                 ///< convergence criterion (unitless)
   int niter;                       ///< current iteration number
   std::string hashstr;             ///< hash string

   // end command line input

   // input data - points and measurements
   std::map<std::string,Point> Points;       ///< all Points, including unused ones
   std::vector<std::string> usedLabels;      ///< all labels (Points) used in NLLS prob.
   std::vector<std::string> unUsedLabels;    ///< all labels (Points) NOT used in NLLS


   /// all measurements, including usused ones
   std::vector< ref_ptr<MeasBase> > Measurements;

   // other input data
   std::vector<DirSet> DirSets;           ///< all direction sets
   std::vector<std::string> Extracts;     ///< Extraction records (strings)
   std::vector<std::string> comments;     ///< Comments from DAT file

   /// data computed and used by ComputeAPriori()
   std::vector<std::string> computeAPlabels; ///< labels (Points) computed in APriori
   std::vector<HAngle> computeAPHANs;     ///< All HANs, to be used in APriori only

   // geoid input
   std::string geoidpath;           ///< input path for geoid file
   std::string geoidfile;           ///< geoid file name
   std::string geoidfile0;          ///< geoid file name without path
   std::string geoidinterp;         ///< interpolation method for geoid
   Geoid geoid;                     ///< geoid instance to calculate undulation,DoV
   bool applyGeoid;                 ///< true if geoid file loaded
   bool overwriteGeoidPatch=false;  ///< true if project geoid .h5 file will be overwritten
                                    ///< .h5 geoid patch is also written if it doesn't exist

   // Observation equations - see ObsEqnData in lsaobseqn.hpp

   // NLLS problem
   gnsstk::SRI sri;                  ///< Square root information object for sol
   gnsstk::Vector<double> NomState0; ///< a priori state vector before any iteration
   gnsstk::Vector<double> NomState;  ///< a priori state vector before each iteration

   // Solution - see SolutionData below

   // misc for output
   int plwidth;                     ///< length of the longest Point label
   int stwidth;                     ///< length of the longest State name
   int dnwidth;                     ///< length of the longest Data name
   std::string stateMsg;            ///< state vector names as readable string
   std::string dataMsg;             ///< data vector names as readable string
   std::string unusedMsg;           ///< point and data names that are not used

   // binary file - optional
   LSABinaryFile LSABin;

   // print timing information
   void timing(std::string msg)
   {
      if(!doTiming) return;
      wallend.setLocalTime();
      LOG(INFO) << "Timing " << msg << " net " << std::fixed << std::setprecision(6)
               << double(clock()-totaltime)/double(CLOCKS_PER_SEC)
               << " tot " << int(wallend - wallbegin);
      totaltime = clock();
   }

private:
   /// Set default values in the configuration, called by constructor
   void SetDefaults(void) throw()
   {
      wallbegin.setLocalTime();              // begin counting time Timing
      totaltime = clock();

      PrgmName = std::string("lsasolver");
      logfile = std::string("");
      infile = obseqnfile = binfile = std::string();

      // command line input -------------------------------------------
      // input control
      apquit = doTiming = allowAPcentroid = statTests = useSRIF = forcefast = forcestable = false;

      geoidfile = std::string();
      geoidinterp = std::string("bicubic");
      applyGeoid = false;
      hashstr = std::string();

      // overwrite DAT file input
      cmd_noAPV = cmd_doAPV = false;
      cmd_nitermax = -1;
      cmd_converge = 0.0;

      // solution
      nitermax = 15;
      converge = 1.e-8;

      // output
      doWest = false;
      noAPV = false;
      noExtRelVect = false;
      linprecP = 4;   // 1/10 MM
      linprecM = 5;   // 1/100 MM
      linwidth = 10;
      angprecM = 3;     // SOA
      angwidthM = 7;    // SOA
      angprecP = 5;     // SOA
      doProgress = false;
      debug = -1;
      help = verbose = validate = false;
      // end command line input ---------------------------------------
   }

}; // end class GlobalData

//------------------------------------------------------------------------------------
/// Class SolutionData encapsulates data for full NLLS solution in lsasolver
class SolutionData : public gnsstk::Singleton<SolutionData> {
public:
   /// Default and only constructor, sets defaults.
   SolutionData() throw() { SetDefaults(); }

   gnsstk::Vector<double> Sol;            ///< LLS Solution vector  N
   gnsstk::Matrix<double> Cov;            ///< LLS Covariance matrix NxN
   gnsstk::Vector<double> Resid;          ///< Data residual vector - relative M
   gnsstk::Vector<double> RawResid;       ///< Data residual vector - physical units M
   double relRMSresid;                   ///< RMS relative (whitened) residual (dimless)
   double angRMSresid;                   ///< RMS raw (colored) residual (radians)
   double lenRMSresid;                   ///< RMS raw (colored) residual (meters)
   int ndof;                             ///< Degrees of freedom = num.data - num.unknowns
   double APV;                           ///< a posteriori variance of unit weight
   double alpha;                         ///< significance level for ChiSq test (0.01) (1-confidence)
   double StdResConfid;                  ///< Confidence level of Standard Residual Threshold (Tau Dist) [0<prob<1]
   double upperChiSq,lowerChiSq;         ///< ChiSq tests, compare APV*(ndof-1)
   gnsstk::Vector<double> maj2Drr;        ///< Major (NA) axis of reliability rctangle
   gnsstk::Vector<double> maj3Drr;        ///< Major (NA) axis including all three dimensions
   gnsstk::Vector<double> min2Drr;        ///< Minor (NB) aasix of reliability rectangle
   gnsstk::Vector<double> vertrr;         ///< Vertical (U of ENU) compoinent of reliability metrics
   gnsstk::Vector<double> azrr;           ///< Azimuth for reliability rectangle
   gnsstk::Vector<double> Redund;         ///< redundancy number = sqrt diagonal of Qvv
   gnsstk::Vector<double> StdResid;       ///< standardized residuals
   gnsstk::Vector<bool> PossibleOutlier;  ///< flag indicating standard residuals are possible outliers


private:
   /// Set default values in the configuration, called by constructor
   void SetDefaults(void) throw()
   {
      alpha = 0.05;
      StdResConfid = 0.95;
      maj2Drr = 0.0;
      maj3Drr = 0.0;
      min2Drr = 0.0;
      vertrr = 0.0;
      azrr = 0.0;
   }

}; // end class SolutionData

//------------------------------------------------------------------------------------
/// Class FinalData encapsulates data for each Point used in FinalOutput()
class FinalData {
public:
   double glat, glon, ght;             ///< geodetic latN lonE (rad), height(m)
   double olat, olon, oht;             ///< orthometric latN lonE (rad), height(m)
   double siglat,siglon,sight;         ///< sigmas to be printed
   double APV, APS;                    ///< apost variance and sigma, 1 if not scaling
   gnsstk::Matrix<double> Rot;          ///< 3x3 rotation matrix XYZ->NEU
   gnsstk::Matrix<double> CovXYZ;       ///< 3x3 covariance matrix (XYZ) (m*m)
   gnsstk::Matrix<double> CovNEU;       ///< 3x3 covariance matrix (NEU) (m*m)
   gnsstk::Vector<double> Nominal0;     ///< 3-vector nominal initial data
   gnsstk::Vector<double> offsetNEU;    ///< 3-vector adjustment NEU
}; // end class FinalData

// lsapriori.cpp
//------------------------------------------------------------------------------------
/// Compute a priori positions for unknown Points. This is the entry point to the
/// a priori algorithms from lsasolver.
/// @param nunknown output the number of unknown Points that remain unknown
/// @param allowDefault if true, allow the default algorithm to provide positions
/// @param is2D if true, problem is 2 dimensional
/// @return 0 for success, 8 if user requested quit, and 11 on failure.
int ComputeAPriori(unsigned int& nunknown, bool allowDefault=false, bool is2D=false)
   throw(gnsstk::Exception);

//------------------------------------------------------------------------------------
#endif   // LSA_SOLVER_INCLUDE
