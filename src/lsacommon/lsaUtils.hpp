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
/// @file lsaUtils.hpp  Include file for utilities for LSA.

#ifndef LSA_UTILITIES_INCLUDE
#define LSA_UTILITIES_INCLUDE

#include <string>
#include "Exception.hpp"
#include "Matrix.hpp"
#include "LSAConstants.hpp"
#include <QFileInfo>
#include <QString>
#include <LSAType.hpp>
#include <QSettings>
#include "Triple.hpp"
#include "ENUUtil.hpp"
#include "Matrix.hpp"

//------------------------------------------------------------------------------------
///< static const converts degrees to radians
//static const double DEG_TO_RAD = 1.7453292519943e-2;
static const double DEG_TO_RAD(atan(1) / 45);

///< static const converts radians to degrees
//static const double RAD_TO_DEG = 57.295779513082;
static const double RAD_TO_DEG(1/DEG_TO_RAD);

///< static const converts radians to seconds-of-arc
static const double RAD_TO_SOA = 206264.8;

///< static const converts seconds-of-arc to radians
static const double SOA_TO_RAD = 1.0/RAD_TO_SOA;

///< static const converts seconds-of-arc to degrees
static const double SOA_TO_DEG = RAD_TO_DEG / RAD_TO_SOA;

///< static const converts meters to feet
static const double M_TO_FT = 3.2808;

///< static const value of pi
static const double Pi = 3.14159265358979323846264;

///< static const value of 2*pi
static const double TwoPi = 6.283185307179586476925;

///< static const value of radius of earth == semi-major axis (WGS84)
static const double Rearth = 6378137.0;

///< static const value of eccentricity (WGS84)
static const double eccent = 8.1819190842622e-2;

///< static string labels X,Y,Z
static const std::string XYZ[3] = { "X", "Y", "Z" };


//------------------------------------------------------------------------------------
double roundDoubleToPrecision(double number, int precision);

//------------------------------------------------------------------------------------
/// small routine to manage confidence levels for ellipse
double getConfidenceFactor(std::string level, int dimensionality, int dof) throw(gnsstk::Exception);

//------------------------------------------------------------------------------------
/// Convert double degrees to int degrees, int minutes, double seconds of arc
/// @param deg          double degrees input (in degrees)
/// @param is_neg       reference to boolean indicating whether or not the returned DMS value is negative
/// @param D            reference to integer degrees output
/// @param M            reference to integer minutes-of-arc output
/// @param sec          reference to double seconds-of-arc output
/// @param precision    integer desired decimals of precision on seconds, unused if negative input
void degToDMS(double deg,  bool &is_neg, int& D, int& M, double& sec, int precision = -1) throw();

//------------------------------------------------------------------------------------
/// Convert int degrees, int minutes, double seconds of arc to double decimal degrees
/// @param is_neg boolean whether DMS input is negative
/// @param D      integer degrees input
/// @param M      integer minutes-of-arc input
/// @param sec    double seconds-of-arc input
/// @return full angle in degrees
double DMSToDeg(bool is_neg, int D, int M, double sec) throw();

//------------------------------------------------------------------------------------
/// Convert int degrees, int minutes, double seconds of arc to double decimal degrees
/// where the input is three strings. Calls (int&, int&, double&) version, but
/// this handles the sign of the angle more rigorously.
/// @param D    string (containing int) degrees input
/// @param M    string (containing int) minutes-of-arc input
/// @param sec  string (containing double) seconds-of-arc input
/// @return full angle in degrees
double DMSToDeg(std::string D, std::string M, std::string sec) throw();

//------------------------------------------------------------------------------------
/// given a horizontal vector in components dx, dy, compute the azimuth, correcting
/// for the quadrant in which the vector lies. The zero of azimuth is the +y axis.
/// The input components must have the same units; output is in radians
/// @param dx  double X-vector component of the vector
/// @param dy  double Y-vector component of the vector
/// @return the azimuth of the vector in radians.
double azimuth(double dx, double dy) throw(gnsstk::Exception);

//------------------------------------------------------------------------------------
/// determine if the given string is a valid linear unit; for use in DAT parser + ??
/// @param str   string of the form "FT" "M" "CM" "KM" etc., denoting linear unit.
/// @return bool true if this is a valid linear unit
bool isLinearUnit(std::string str) throw();

//------------------------------------------------------------------------------------
/// determine if the given string is a valid token to indicate a ENU geodetic position
/// constraint for use in LSA parser
/// @param str   string of the form "FIX" "E" "EN" "ENU" etc., denoting a constraint
/// @return bool true if this is a valid constraint token
bool isNEU(std::string str) throw();

bool isFltConFix(std::string str);

//------------------------------------------------------------------------------------
/// determine if the given string is N or S.  Used in LSA parser.
/// param str   string of the form "N" or "S"
/// return bool true if this is N or S
bool isNorS(std::string str) throw();

//------------------------------------------------------------------------------------
/// determine if the given string is N or S.  Used in LSA parser.
/// param str   string of the form "E" or "W"
/// return bool true if this is E or W
bool isEorW(std::string str) throw();

//------------------------------------------------------------------------------------
/// determine if the given string is lsa modifier keyword
/// @param str   str parsed from lsafile
/// @return bool true if this is a lsa modifier keyword
bool isLSAModifier(std::string str) throw();

//------------------------------------------------------------------------------------
/// determine if the given string is a valid angular unit; for use in DAT parser + ??
/// @param str   string of the form "RAD" "DEG" "SOA" etc., denoting angular unit.
/// @return bool true if this is a valid angular unit
bool isAngularUnit(std::string str) throw();

//------------------------------------------------------------------------------------
/// determine if the given string is a valid coordinate.
/// @param str   string of the form "X" "Y" "Z" "N" "E" or "U"
/// @return bool true if this is a valid coordinate
bool isCoordinate(std::string str) throw();

//------------------------------------------------------------------------------------
/// determine if a string represents an integer
/// @param s  string to test
/// @return bool true if input string is an integer
bool isInteger(const std::string & s);

//------------------------------------------------------------------------------------
/// determine if a string is a number
/// @param s  string to test
/// @return bool true if input string is a number
bool isNumber(const std::string& s);

//------------------------------------------------------------------------------------
/// convert units of value from inp to out, using strings:
///  linear: MM M CM KM FT  and angular: DEG RAD SOA
/// @param out   string denoting linear or angular unit for output
/// @param value the double to convert
/// @param inp   string denoting linear or angular unit of input; inp may == out
/// @return value converted from inp units to out units
/// @throw on invalid input
double convertTo(std::string out, double value, std::string inp)
   throw(gnsstk::Exception);

//------------------------------------------------------------------------------------
/// print the matrix with elements <~ 10^-15 printed as "0"
/// @param M Matrix<double> to print
/// @param p precision of output
/// @param w width of output
/// @param sci if true use scientific, else fixed, format
/// @param isSYM if true print only the lower triangular portion; assumes M symmetric
std::string cleanMatrixString(const gnsstk::Matrix<double>& M, int p, int w,
                                bool sci=false, bool isSYM=false);

//------------------------------------------------------------------------------------
/// Compute the eigenvalues of a 3x3 symmetric (covariance) matrix.
/// @param Cov Matrix<double> 3x3 symmetric covariance matrix
/// @param Eval Vector<double> on output, 3-vector containing the three eigenvalues.
/// @return a code giving the nature of the 3 eigenvalues:
///    1 one real value plus a complex conjugate pair (not a valid covariance),
///       NB. the eigenvalues are Eval(0), Eval(1)+i*Eval(2), Eval(1)-i*Eval(2)
///    2 three real values, but 2 are equal
///    3 three distinct real values.
/// Reference: Abramowitz and Stegun 3.8.2
int EigenCovariance(const gnsstk::Matrix<double>& Cov, gnsstk::Vector<double>& Eval)
   throw(gnsstk::Exception);

//------------------------------------------------------------------------------------
/// Compute all eigenvalues and eigenvectors of a square symmetric matrix A; return
/// the eigenvalues in vector D, the eigenvectors are the corresponding columns of V.
/// Only the elements of A at and above the diagonal are used.
/// This routine is not efficient; best to use only for dimension < ~10.
/// @param A covariance matrix, a symmetric Matrix<double> of dimension < ~10
/// @param D on output a Vector<double> containing eigenvalues of A
/// @param V on output a Matrix<double> containing eigenvectors of A in columns;
///          element n of D corresponds with column n of V.
/// @throw if input is invalid or if the algorithm fails.
void EigenDecomp(gnsstk::Matrix<double> A,
                 gnsstk::Vector<double>& D, gnsstk::Matrix<double>& V)
   throw(gnsstk::Exception);

/// Verify that the eigenvectors of a matrix are all positive
/// @param aa-cc, elements of a covariance matrix
/// @return true if the matrix is positive semi-definite
bool isPositiveSemiDefinite(double aa, double ab, double ac, double bb, double bc, double cc);

//------------------------------------------------------------------------------------
/// Compute the 32-bit Cyclic Redudancy Check of a string
/// @param line - the string to be hashed
/// @param hash - the value to use as the starting value for the hash
/// @return the hash of the line
unsigned int hashLine(std::string line, unsigned int hash);

//------------------------------------------------------------------------------------
// TODO document these
std::string getExecutablePath();
void gen_random(char *s, const int len);
std::string quoteIfSpaces(std::string path);

std::string addQuotes(std::string input);
QString addQuotes(QString input);

//------------------------------------------------------------------------------------
/// Compute confidence ellipse for the given covariance matrix (NEU).
/// Scaling with confidence factors is done by the caller.
/// @param CovNEU Matrix<double> 3x3 covariance matrix in North-East-Up
/// @param maj3 double output 3D-semi-major axis of ellipse
/// @param min3 double output 3D-semi-minor axis of ellipse
/// @param maj2 double output 2D(horizontal)-semi-major axis of ellipse
/// @param min2 double output 2D(horizontal)-semi-minor axis of ellipse
/// @param vert double output vertical (Up) axis of ellipse
/// @param azm double output azimuth of maj2
void getEllipse(const gnsstk::Matrix<double>& CovNEU,
                  double& maj3, double& min3,
                  double& maj2, double& min2,
                  double& vert, double& azm)
   throw(gnsstk::Exception);

//------------------------------------------------------------------------------------
/// Returns desired number of digits to print after the decimal place
/// @param units - refers to standard units string in LSAConstants
/// @param isPosition - whether precison for angles are for geodetic position or measurement
/// @return number of digits of precision to print/display
int getPrecisionForUnits(const std::string &units, bool isPosition=false);
//------------------------------------------------------------------------------------

int grabNumDigits(LSAType lsaType, QString units);
int grabNumDigits(LSAType lsaType, std::string units);

//------------------------------------------------------------------------------------
/// Write a floating point angle in a string as degrees, minutes, seconds.
/// @param rad  double floating-point degrees input (in radians)
/// @param prec integer number of digits precision for sec-of-arc (default 2)
/// @return string of the form "<int degrees> <int minutes> <float seconds>"
std::string angleAsDMSstring(double rad, int prec = getPrecisionForUnits(lsa::UNITS_SOA));

//------------------------------------------------------------------------------------
/// Returns text contained by XML tags, and optionally cuts it from the source string.
/// @param inString - original string
/// @param xmlTag - Tag of the xml to find (e.g. RecordTags::TextNotes)
/// @param bCut - whether the tags and everything between should be removed from the original string
/// @return a string containing everything in between tags
std::string extractXMLFromString(std::string& inString, const std::pair<std::string, std::string>& xmlTag, bool bCut = false);

//------------------------------------------------------------------------------------
/// Tries to determine what type of file the fileInfo is refering to, and then gives
/// a converter type based on that.
/// @param filePath - path to the file
/// @return the converter_type which is likely appropriate for the given file
lsa::CONVERTER_TYPE getConverterTypeForFile(const QFileInfo& fileInfo);

//------------------------------------------------------------------------------------
/// Returns the precision for the specified parameter
/// @return an int of angular measurement in SOA precision
int getAngularMeasurementPrecisionSOA();

//------------------------------------------------------------------------------------
/// Returns the precision for the specified parameter
/// @return an int of angular positions in SOA precision
int getAngularPositionPrecisionSOA();

//------------------------------------------------------------------------------------
/// Returns the precision for the specified parameter
/// @return an int of linear measurements in meters precision
int getLinearMeasurementPrecisionMeters();

//------------------------------------------------------------------------------------
/// Returns the precision for the specified parameter
/// @return an int of linear positions in meters precision
int getLinearPositionPrecisionMeters();

//------------------------------------------------------------------------------------
/// Converts the given XYZ vector to ENU, with respect to the refPos
/// @param refLonLat - the reference position longitude latitude (radians)
/// @param ECEFmatrix - 3x3 matrix to be rotated
/// @return the vector in ENU
gnsstk::Matrix<double> convertECEFtoENU(std::pair<double, double> refLonLat, gnsstk::Matrix<double> ECEFmatrix);

// It's the gnsstk::ENUUtil class, but we wanted to be able to use the rotation matrix
class LSAENUUtil : public gnsstk::ENUUtil
{
public:
    LSAENUUtil(double lon, double geodeticLat):gnsstk::ENUUtil(geodeticLat, lon){};
    gnsstk::Matrix<double> getRotMatrix(){return rotMat;};
};

#endif   // LSA_UTILITIES_INCLUDE
