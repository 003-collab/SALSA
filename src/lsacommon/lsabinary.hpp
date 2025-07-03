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
/// @file lsabinary.hpp
/// Implement binary output file for lsasolver, including input output and dump.

#ifndef LSA_BINARY_FILE_INCLUDE
#define LSA_BINARY_FILE_INCLUDE

// system includes
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <map>
// GNSSTk
#include "Exception.hpp"
#include "StringUtils.hpp"
// geomatics
#include "Namelist.hpp"
// lsa
#include "MatrixVector.hpp"

/// Class to hold a priori position information in the binary file
/// Use in a map with key == label, define this object for each position
class apriori_pos {
public:
    enum ComputedState
    {
      Provided = 0,
        ComputedFloating,
        ComputedLatLonFixed
    };

   //std::string label;         ///< Point label
   double X,Y,Z;              ///< a priori position
   int fixtype;               ///< flag 0-3 = unknown,estimate,fix,adjust
   int computed;              ///< if non-zero, this Point was computed by APriori()

   /// constructor
   apriori_pos(void) { }

}; // end class apriori_pos

/// Class to hold data after final iteration in the binary file
/// Use in a map with key == label, define this object for each measurement
class data_result {
public:
   //std::string label;         ///< measurement name : EQD.DataNames
   double meas;                 ///< measurement : EQD.MeasData
   double nomin;                ///< nominal measurement : EQD.NomData
   double rawresid;             ///< raw residual : SD.RawResid
   double resid;                ///< residual : SD.Resid
   double redundancy;           ///< redundancy : SD.Redund
   double stdresid;             ///< standard residual : SD.StdResid
   double minDectBias;          ///< Internal reliability value
   double extVectMag;           ///< External reliability magnitude
   bool PossibleOutlier;        ///< flag indicating whether std res possible outlier
   bool RedundancyZero;         ///< flag to indicate whether a redudnacy of zero is present

   /// constructor
   data_result(void) { }

}; // end class data_result

/// Class to hold directions after final iteration in the binary file.
/// Use with dirset_result. Use in a map with key == label
class dir_result {
public:
   //std::string label;         ///< "To" site name + ":" + tag
   double direction;          ///< adjusted direction (radians)
   double resid;              ///< raw residual (SOA)
   double stdresid;           ///< std residual ()
   double sigma;              ///< std deviation (SOA)

   /// constructor
   dir_result(void) { }

}; // end class dir_result

/// Class to hold adjusted Points after final iteration in the binary file.
/// Use in a map with key == label, define this object for each non-fixed Point
class posXYZ_result {
public:
   //std::string label;                ///< Point label
   int computed;                       ///< if non-zero, this Point was computed by APriori()
   int constraint[3];                  ///< 1 if constrained, else 0, for N,E,U
   double X,Y,Z;                       ///< coordinates ECEF XYZ m
   double adjX,adjY,adjZ;              ///< adjustments XYZ m
   double sigX,sigY,sigZ;              ///< sigmas XYZ m
   double Cxx,Cxy,Cxz,Cyy,Cyz,Czz;     ///< covariance matrix m*m

   /// constructor
   posXYZ_result(void) { }

}; // end class posXYZ_result

/// Class to hold adjusted Points after final iteration in the binary file.
/// Use in a map with key == label, define this object for each non-fixed Point
class posLLH_result {
public:
   //std::string label;                ///< Point label
   int computed;                       ///< if non-zero, this Point was computed by APriori()
   double lat,lon,ht;                  ///< coordinates lat, lon E, ht
   int latd,latm;                      ///< latitude degrees, minutes
   double lats;                        ///< latitude seconds of arc
   int lond,lonm;                      ///< longitude degrees, minutes
   double lons;                        ///< longitude seconds of arc
   double adjN,adjE,adjU;              ///< adjustments NEU
   double sigN,sigE,sigU;              ///< sigmas NEU
   double Cnn,Cne,Cnu,Cee,Ceu,Cuu;     ///< covariance matrix
   double und,dovN,dovE;               ///< undulation(m) DoV(SOA) for this point, if available
   double maj2Drr;                     ///< 2D major (NA) axis of reliability rectangle
   double min2Drr;                     ///< 2D minor (NB) axis of reliability rectangle
   double maj3Drr;                     ///< 3D major axis of reliability rectangle
   double vertrr;                      ///< Vertical component of reliability rectangle
   double azrr;                        ///< azimuth of reliability rectangle

   /// constructor
   posLLH_result(void) { }

}; // end class posLLH_result

/// Class to hold data and implement read write and dump of binary file.
class LSABinaryFile
{
public:
// member data
   /// Version number for this class; change only when the binary file format changes.
   /// If ReadBinaryFile() finds an early version, it fails with message to rerun the
   /// solver to regenerate the binary file.
   /// 8: Added reliability metrics to llh and measurement data classes
   static const int Version=8;

   std::string filename;      ///< name of binary file

   // 'header' data
   std::string Title;         ///< title from DAT file
   double converge;           ///< convergence limit
   int nitermax;              ///< iteration limit
   std::string geoidfile;     ///< geoid file name
   std::string geoint;        ///< geoid interpolation method
   int ndof;                  ///< degrees of freedom
   int nunknown;              ///< number of unknowns
   int ndata;                 ///< number of data     EQD.DataNames
   int nmeas;                 ///< number of data - 3*number of constrained pts
   int nconstr;               ///< number of constraints
   int dim;                   ///< dimension of problem: 2 or 3
   std::string hashstr;       ///< hash string
   bool calcExtRelVect;       ///< flag that points if the external reliability is calculated

   // a priori positions, key == Point label
   std::map<std::string,apriori_pos> apPoints;  ///< data: XYZ fixtype and computed

   // each iteration
   std::vector<double> changes;   ///< converge = RMS Point adjustment in m
   std::vector<double> relresid;  ///< RMS relative residual
   std::vector<int> status;       ///< 0 cont, 1 converge, -1 niter>limit, -2 diverge

   /// after final iteration, key == label in the map
   std::map<std::string, data_result> dataResults;    ///< data: value, residuals

   // stats
   double relRMSresid;  ///< RMS relative residual
   double APV;          ///< APV
   double angRMSresid;  ///< angular RMS residual (radians)
   double lenRMSresid;  ///< linear RMS residual (m)
   double confidence;   ///< confidence (alpha) in chi squared test
   double lowerChiSq;   ///< lower limit in chi squared test
   double upperChiSq;   ///< upper limit in chi squared test


   // final solution and adjustments, key == label
   std::map<std::string, posXYZ_result> posXYZResults; ///< adjusted Point
   std::map<std::string, posLLH_result> posLLHResults; ///< adjusted Point

   // full final Partials, Namelists and Covariance
   gnsstk::Namelist DataNames, StateNames;
   //gnsstk::FlexMatrix<double> Partials;          // TD need Partials?
   gnsstk::Matrix<double> Covariance;

// member functions

   /// empty and only constructor
   LSABinaryFile(void) { clear(); }

   /// clear the object
   inline void clear(void) {
      filename = Title = geoint = geoidfile = std::string();
      hashstr = std::string("(empty)");
      nitermax = ndof = nunknown = ndata = nmeas = nconstr = dim = -1;
      converge = relRMSresid = APV = angRMSresid
         = lenRMSresid = confidence = lowerChiSq = upperChiSq = 0.0;
      apPoints.clear();
      changes.clear();
      relresid.clear();
      status.clear();
      dataResults.clear();
      posXYZResults.clear();
      posLLHResults.clear();
      DataNames.clear(); StateNames.clear();
      //Partials = gnsstk::FlexMatrix<double>();
      Covariance = gnsstk::Matrix<double>();
   }

   /// write a string to a binary strm
   /// @param os output stream to write
   /// @param str string to write
   void writeString(std::ostream& os, std::string str) throw(gnsstk::Exception);

   /// write an integer to a binary stream
   /// @param os output stream to write
   /// @param in integer value to write
   void writeInt(std::ostream& os, int in) throw(gnsstk::Exception);

   /// write a double to a binary stream
   /// @param os output stream to write
   /// @param d double value to write
   void writeDouble(std::ostream& os, double d) throw(gnsstk::Exception);

   /// read a string from a binary file
   /// @param is input stream to read
   /// @return string read
   std::string readString(std::ifstream& is) throw(gnsstk::Exception);

   /// read an int from a binary file
   /// @param is input stream to read
   /// @return double read
   double readDouble(std::ifstream& is) throw(gnsstk::Exception);

   /// read an int from a binary file
   /// @param is input stream to read
   /// @return integer read
   int readInt(std::ifstream& is) throw(gnsstk::Exception);

   /// Write the binary file using this object; must strictly parallel ReadBinaryFile.
   /// @param fn name of binary file to be written
   /// @return 0 ok, 92 if file could not be opened
   /// @throw on stream error
   int WriteBinaryFile(std::string& fn) throw(gnsstk::Exception);

   /// Read the binary file and load the information into this object;
   /// must strictly parallel WriteBinaryFile().
   /// @param fn name of binary file to be read
   /// @return 0 ok, 92 if file could not be opened, 93 if Version is obsolete
   /// @throw on stream error
   int ReadBinaryFile(std::string& fn) throw(gnsstk::Exception);

   /// Dump the header information
   /// @param os output stream
   void DumpHeader(std::ostream& os) throw(gnsstk::Exception);

   /// Dump the initial position information
   /// @param os output stream
   /// @param doWest if true, output west longitude
   /// @param linprecM int precision for linear measurements (m)
   /// @param linprecP int precision for geocentric positions (m)
   /// @param angprecM int precision for angular measurements (SOA)
   /// @param angprecP int precision for angular positions (SOA)
   void DumpInitial(std::ostream& os, bool doWest=false, const int linprecM=5,
                    const int linprecP=4, const int angprecM=3, const int angprecP=5)
      throw(gnsstk::Exception);

   /// Dump the iteration information
   /// @param os output stream
   /// @param linprec int precision for residuals
   void DumpIterations(std::ostream& os, const int prec=3)
      throw(gnsstk::Exception);

   /// Dump the data and residuals
   /// @param os output stream
   /// @param linprec int precision for linear (m)
   /// @param angprecM int precision for angular measurements (SOA)
   /// @param angprecP int precision for angular positions (SOA)
   void DumpData(std::ostream& os,
         const int linprec=3, const int angprecM=3, const int angprecP=5)
      throw(gnsstk::Exception);

   /// Dump the final statistics
   /// @param os output stream
   /// @param linprec int precision for linear (m)
   /// @param angprecM int precision for angular measurements (SOA)
   /// @param angprecP int precision for angular positions (SOA)
   void DumpFinalStats(std::ostream& os,
         const int linprec=3, const int angprecM=3, const int angprecP=5)
      throw(gnsstk::Exception);

   /// Dump the final positions in XYZ
   /// @param os output stream
   /// @param noAPV if true, do not scale with APV
   /// @param linprecP int precision for geocentric positions (m)
   /// @param angprecM int precision for angular measurements (SOA)
   /// @param angprecP int precision for angular positions (SOA)
   void DumpFinalXYZ(std::ostream& os, bool noAPV=false,
         const int linprecP=4, const int angprecM=3, const int angprecP=5)
      throw(gnsstk::Exception);

   /// Dump the final positions in LLH
   /// @param os output stream
   /// @param doWest if true, output west longitude
   /// @param noAPV if true, do not scale with APV
   /// @param linprec int precision for linear (m)
   /// @param angprecM int precision for angular measurements (SOA)
   /// @param angprecP int precision for angular positions (SOA)
   void DumpFinalLLH(std::ostream& os, bool doWest=false, bool noAPV=false,
         const int linprec=3, const int angprecM=3, const int angprecP=5)
      throw(gnsstk::Exception);

   /// Dump the covariance matrix
   /// @param os output stream
   /// @param noAPV if true, do not scale with APV
   /// @param linprec int precision for linear (m)
   /// @param angprecM int precision for angular measurements (SOA)
   /// @param angprecP int precision for angular positions (SOA)
   void DumpCovariance(std::ostream& os, bool noAPV=false,
         const int linprec=3, const int angprecM=3, const int angprecP=5)
      throw(gnsstk::Exception);

   /// Dump the contents of this object.
   /// @param os output stream
   /// @param doWest if true, output west longitude
   /// @param noAPV if true, do not scale with APV
   /// @param linprecM int precision for linear measurements (m)
   /// @param linprecP int precision for geocentric positions (m)
   /// @param angprecM int precision for angular measurements (SOA)
   /// @param angprecP int precision for angular positions (SOA)
   void Dump(std::ostream& os, bool doWest=false, bool noAPV=false,
         const int linprecM=5, const int linprecP=4, const int angprecM=3, const int angprecP=5)
      throw(gnsstk::Exception);

   /// Write the CSV file
   /// @param os output stream
   /// @param prec int precision for M and SOA
   void writeCSVFile(std::ostream& os, const int prec=3)
      throw(gnsstk::Exception);

   /// Write the contents of the points file
   /// @param os output stream
   /// @param doWest if true, output west longitude
   /// @param noAPV if true, do not scale with APV
   /// @param linprec int precision for linear (m)
   /// @param angprec int precision for angular positions (SOA)
   /// @param conf string confidence level for ellipse (one of 1sig, 90, 95)
   void DumpPoints(std::ostream& os, bool doWest=false, bool noAPV=false,
         const int linprec=3, const int angprecP=5, const std::string conf="1sig")
      throw(gnsstk::Exception);

   /// Write the contents of the CSV file
   /// @param os output stream
   /// @param linprec int precision for linear (m)
   /// @param angprec int precision for angular positions (SOA)
   void DumpCSV(std::ostream& os, const int linprec=3, const int angprecP=5)
      throw(gnsstk::Exception);

}; // end class LSABinaryFile

#endif   // define LSA_BINARY_FILE_INCLUDE
