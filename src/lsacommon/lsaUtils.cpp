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
/// @file lsaUtils.cpp  Utilities for LSA.

#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <limits.h>
#ifdef _WIN32
    #include <windows.h>//exe path
    #include <tchar.h>
#else
    #include <unistd.h>//exe path
#endif
#include <ENUUtil.hpp>
#include "StringUtils.hpp"
#include "lsaUtils.hpp"
#include <QTextStream>

#include "SpecialFuncs.hpp"

using namespace std;
using namespace gnsstk;

double roundDoubleToPrecision(double number, int precision)
{
    double factor = std::pow(10,precision);
    double boosted = number*factor;
    double rounded, dummy;

    if(std::modf(boosted,&dummy)>=0.5)
        rounded = boosted >= 0 ? std::ceil(boosted) : std::floor(boosted);
    else
        rounded = boosted < 0 ? std::ceil(boosted) : std::floor(boosted);
    return rounded/factor;
}

//------------------------------------------------------------------------------------
// Convert double degrees to int degrees, int minutes, double seconds of arc
// param double deg  floating-point degrees input (in degrees)
// param bool is_neg             boolean indicating whether or not the returned DMS values is negative
// param int D                   integer degrees output
// param int M                   integer minutes-of-arc output
// param double sec              floating seconds-of-arc output
// optional param int precision  integer specifying the number of digits of precision in the seconds (default=-1)
void degToDMS(double deg_in, bool &is_neg, int& D, int& M, double& sec, int precision) throw()
{
//   const double ROUND_THRESHOLD = lsa::ZERO_BOUND;

   if(deg_in < 0.0)
       is_neg = true;
   else
       is_neg = false;
   double deg = ::fabs(deg_in);
/*
   double ceil_deg = std::ceil(deg);
   double floor_deg = std::floor(deg);
   if(ceil_deg - deg < ROUND_THRESHOLD)// want 111.99999999 ---> 112, but 111.54944 to be treated normally
       D = int(ceil_deg);
   else if(deg - floor_deg < ROUND_THRESHOLD)
       D = int(floor_deg);
   else
*/
       D = int(deg);
   deg = 60. * ::fabs((deg - double(D)));   // added fabs in case D > deg (e.g. 111.9999999 - 112)
   M = int(deg);                    // int minutes
   sec = 60. * (deg - double(M));   // remove int min and convert to seconds

   //If precision is specified
   if(precision > -1)
   {
       if(int(std::floor(sec*pow(10,precision)+0.5)/pow(10,precision)) >= 60)
       {
           sec -= 60.0;
           if(sec < 0.0)
           {
               sec = 0.0;
           }
           M += 1;
           if(M>=60)
           {
               M -= 60;
               D += 1;
               if(D>=360)
               {
                   D -= 360;
               }
           }
       }
   }
}

//------------------------------------------------------------------------------------
// Convert int degrees, int minutes, double seconds of arc to double decimal degrees
// param is_neg boolean whether DMS input is negative
// param D    integer degrees input
// param M    integer minutes-of-arc input
// param sec  double seconds-of-arc input
// return full angle in degrees
double DMSToDeg(bool is_neg, int D, int M, double sec) throw()
{
   D = abs(D);
   double deg = double(D) + (abs(M) + ::fabs(sec)/60.0)/60.0;
   if(is_neg)
   {
       if(deg < lsa::ZERO_BOUND)//turn -0.000000 into 0.000000
           deg = 0.0;
       else
           deg = -deg;
   }

   return deg;
}

//------------------------------------------------------------------------------------
// Convert int degrees, int minutes, double seconds of arc to double decimal degrees
// where the input is three strings. Calls (int&, int&, double&) version, but
// this handles the sign of the angle more rigorously.
// param Dstr    string (containing int) degrees input
// param Mstr    string (containing int) minutes-of-arc input
// param secstr  string (containing double) seconds-of-arc input
// return full angle in degrees
double DMSToDeg(string Dstr, string Mstr, string secstr) throw()
{
    // find the sign
    bool is_neg;
    StringUtils::stripLeading(Dstr," ");
    StringUtils::stripLeading(Mstr," ");
    StringUtils::stripLeading(secstr," ");
    if(Dstr.at(0) == '-')
        is_neg = true;
    else
        is_neg = false;

   int D(abs(StringUtils::asInt(Dstr))), M(abs(StringUtils::asInt(Mstr)));
   double sec(::fabs(StringUtils::asDouble(secstr)));

   double deg = DMSToDeg(is_neg,D,M,sec);
   return deg;
}

//------------------------------------------------------------------------------------
// Write a floating point angle in a string as degrees, minutes, seconds.
// param double rad  floating-point degrees input (in radians)
// param int prec    integer number of digits precision for sec-of-arc (default 2)
// return string of the form "<int degrees> <int minutes> <float seconds>"
string angleAsDMSstring(double rad, int prec)
{
   int d,m;
   double sec;
   bool is_neg;
   degToDMS(rad*::RAD_TO_DEG, is_neg, d, m, sec);
   ostringstream oss;
   oss << d;
   string dstr((is_neg ? "-" : "")+oss.str());
   oss.str("");
   oss << dstr << " " << setw(2) << m
            << " " << fixed << setprecision(prec) << setw(prec+3) << sec;
   string debug_str(oss.str());
   return oss.str();
}

//------------------------------------------------------------------------------------
// given a horizontal vector in components dn, de, compute the azimuth, correcting
// for the quadrant in which the vector lies. The zero of azimuth is the +y axis.
// The input components must have the same units; output is in radians
// param double de  East component of the vector
// param double dn  North component of the vector
// return the azimuth of the vector in radians.
double azimuth(double de, double dn) throw(Exception)
{
try {
   double scale(::sqrt(dn*dn+de*de));
   double az = Pi/2.0 - ::atan2(dn/scale,de/scale);

   while(az > TwoPi) az -= TwoPi;
   while(az < 0.0) az += TwoPi;
   return az;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// determine if the given string is a valid coordinate.
/// @param str   string one of "X" "Y" "Z" "N" "E" or "U"
/// @return bool true if this is a valid coordinate
bool isCoordinate(string str) throw()
{
   if(str == string("X")) return true;
   else if(str == string("Y")) return true;
   else if(str == string("Z")) return true;
   else if(str == string("N")) return true;
   else if(str == string("E")) return true;
   else if(str == string("U")) return true;
   return false;
}

//------------------------------------------------------------------------------------
// determine if a string represents an integer
// @param s  string to test
// @return bool true if input string is an integer
bool isInteger(const string & s)
{
try
{
   if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

   char * p ;
   strtol(s.c_str(), &p, 10) ;

   return (*p == 0) ;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// determine if a string is a number
// @param s  string to test
// @return bool true if input string is a number
bool isNumber(const string& s)
{
    return !s.empty() && s.find_first_not_of("0123456789.-+") == string::npos;
}

//------------------------------------------------------------------------------------
// determine if the given string is a valid linear unit; for use in DAT parser + ??
// param str   string of the form "FT" "M" "CM" "KM" etc., denoting linear unit.
// return bool true if this is a valid linear unit
bool isLinearUnit(string str) throw()
{
    str = gnsstk::StringUtils::upperCase(str);

    if(str == string("FT")) return true;
    else if(str == string("MM")) return true;
    else if(str == string("M")) return true;
    else if(str == string("CM")) return true;
    else if(str == string("KM")) return true;

    return false;
}

//------------------------------------------------------------------------------------
// determine if the given string is N or S.  Used in LSA parser.
// param str   string of the form "N" or "S"
// return bool true if this is N or S
bool isNorS(string str) throw()
{
    str = gnsstk::StringUtils::upperCase(str);

    if     (str == string("N")) return true;
    else if(str == string("S")) return true;

    return false;
}

//------------------------------------------------------------------------------------
// determine if the given string is N or S.  Used in LSA parser.
// param str   string of the form "E" or "W"
// return bool true if this is E or W
bool isEorW(string str) throw()
{
    str = gnsstk::StringUtils::upperCase(str);

    if     (str == string("E")) return true;
    else if(str == string("W")) return true;

    return false;
}

//------------------------------------------------------------------------------------
// determine if the given string is a valid token to indicate a ENU geodetic position
// constraint for use in LSA parser
// param str   string of the form "FIX" "E" "EN" "ENU" etc., denoting a constraint
// return bool true if this is a valid constraint token
bool isNEU(string str) throw()
{
   str = gnsstk::StringUtils::upperCase(str);

   if(str == string("FIX")) return true;

   else if(str == string("E")) return true;
   else if(str == string("N")) return true;
   else if(str == string("U")) return true;

   else if(str == string("EN")) return true;
   else if(str == string("EU")) return true;
   else if(str == string("NE")) return true;
   else if(str == string("NU")) return true;
   else if(str == string("UN")) return true;
   else if(str == string("UE")) return true;

   else if(str == string("ENU")) return true;
   else if(str == string("EUN")) return true;
   else if(str == string("NEU")) return true;
   else if(str == string("NUE")) return true;
   else if(str == string("UEN")) return true;
   else if(str == string("UNE")) return true;

   return false;
}

bool isFltConFix(string str)
{
    str = gnsstk::StringUtils::upperCase(str);

    if     (str == string("FLT")) return true;
    else if(str == string("CON")) return true;
    else if(str == string("FIX")) return true;

    return false;
}

//------------------------------------------------------------------------------------
// determine if the given string is lsa modifier keyword
// @param str   str parsed from lsafile
// @return bool true if this is a lsa modifier keyword
bool isLSAModifier(string str) throw()
{
    str = gnsstk::StringUtils::upperCase(str);

    if      (str == string("HT")) return true;
    else if (str == string("UNCR")) return true;
    else if (str == string("REFRACT")) return true;
    else if (str == string("REDUCED")) return true;
    else if (str == string("VSCA")) return true;
    else if (str == string("CURV")) return true;

    return false;
}

//------------------------------------------------------------------------------------
// determine if the given string is a valid angular unit; for use in DAT parser + ??
// param str   string of the form "RAD" "DEG" "SOA" etc., denoting angular unit.
// return bool true if this is a valid angular unit
bool isAngularUnit(string str) throw()
{
   str = gnsstk::StringUtils::upperCase(str);
   if(str == string("DEG")) return true;
   else if(str == string("RAD")) return true;
   else if(str == string("SOA")) return true;
   return false;
}

//------------------------------------------------------------------------------------
// convert units of value from inp to out, using strings:
//  linear: MM M CM KM FT  and angular: DEG RAD SOA
//  throw on invalid input
double convertTo(string out, double value, string inp)
   throw(Exception)
{

   gnsstk::StringUtils::lowerCase(out);
   gnsstk::StringUtils::lowerCase(inp);

   if(inp == out) return value;

   if(!isAngularUnit(inp) && !isLinearUnit(inp))
      GNSSTK_THROW(Exception(string("Not a valid input unit: "+inp),0,gnsstk::Exception::recoverable));
   if(!isAngularUnit(out) && !isLinearUnit(out))
      GNSSTK_THROW(Exception(string("Not a valid output unit: "+out),0,gnsstk::Exception::recoverable));
   if(isAngularUnit(inp) && !isAngularUnit(out))
      GNSSTK_THROW(Exception("cannot convert from angular to linear",0,gnsstk::Exception::recoverable));
   if(isLinearUnit(inp) && !isLinearUnit(out))
      GNSSTK_THROW(Exception("cannot convert from linear to angular",0,gnsstk::Exception::recoverable));

   double ret(value);
   // first convert to meters and radians
   if(inp == string("m")) ;  // do nothing
   else if(inp == string("mm")) ret /= 1000.0;
   else if(inp == string("cm")) ret /= 100.0;
   else if(inp == string("km")) ret *= 1000.0;
   else if(inp == string("ft")) ret /= M_TO_FT;
   else if(inp == string("deg")) ret *= ::DEG_TO_RAD;
   else if(inp == string("rad")) ;   // nothing
   else if(inp == string("soa")) ret /= RAD_TO_SOA;
   // now convert to outp
   if(out == string("m")) ;  // do nothing
   else if(out == string("mm")) ret *= 1000.0;
   else if(out == string("cm")) ret *= 100.0;
   else if(out == string("km")) ret /= 1000.0;
   else if(out == string("ft")) ret *= M_TO_FT;
   else if(out == string("deg")) ret *= ::RAD_TO_DEG;
   else if(out == string("rad")) ;   // nothing
   else if(out == string("soa")) ret *= RAD_TO_SOA;

   return ret;
}

//------------------------------------------------------------------------------------
string cleanMatrixString(const Matrix<double>& M, int p, int w, bool sci, bool isSYM)
{
   ostringstream oss;
   oss << (sci ? std::scientific : std::fixed) << setprecision(p);
   for(int i=0; i<M.rows(); i++) {
      for(int j=0; j<(isSYM ? i+1 : M.cols()); j++) {
         if(M(i,j) == 0)
            oss << " " << StringUtils::rightJustify("0",w);
         else
            oss << " " << setw(w) << M(i,j);
      }
      if(i < M.rows()-1) oss << endl;
   }
   return oss.str();
}

//------------------------------------------------------------------------------------
// Compute the eigenvalues of a 3x3 symmetric (covariance) matrix.
// @param Cov Matrix<double> 3x3 symmetric covariance matrix
// @param Eval Vector<double> on output, 3-vector containing the (real parts of) the
//          three eigenvalues.
// @param imag double imaginary part of the 2nd and 3rd eigenvalues (signs differ).
// @return a code giving the nature of the 3 eigenvalues:
//    1 for one real value plus a complex conjugate pair (not a valid covariance),
//       the eigenvalues are (0), (1)+i*(2), (1)-i*(2)
//    2 for three real values, but 2 are equal
//    3 for three distinct real values.
// Reference: Abramowitz and Stegun 3.8.2
int EigenCovariance(const Matrix<double>& Cov, Vector<double>& Evalues)
   throw(Exception)
{
try {
   // input must be symmetric and 3x3
   if(!Cov.isSymmetric()) GNSSTK_THROW(Exception("Input must be symmetric"));
   if(Cov.rows() != 3 || Cov.cols() != 3) GNSSTK_THROW(Exception("Input must be 3x3"));

   // eigenvalues v are defined by the cubic equation det(Cov - v*Unity)=0.
   // compute the cubic equation coefficients: v^3 + a2*v^2 + a1*v + a0
   double a2 = -(Cov(0,0)+Cov(1,1)+Cov(2,2));
   double a1 = Cov(0,0)*Cov(1,1) + Cov(0,0)*Cov(2,2) + Cov(1,1)*Cov(2,2)
               - Cov(0,1)*Cov(0,1) - Cov(0,2)*Cov(0,2) - Cov(1,2)*Cov(1,2);
   double a0 = Cov(0,0)*Cov(1,2)*Cov(1,2) - Cov(0,0)*Cov(1,1)*Cov(2,2)
             + Cov(1,1)*Cov(0,2)*Cov(0,2) - Cov(0,1)*Cov(1,2)*Cov(0,2)
             + Cov(2,2)*Cov(0,1)*Cov(0,1) - Cov(0,2)*Cov(0,1)*Cov(1,2);

   // solve the cubic equation
   double q = a1/3.0 - a2*a2/9.0;
   double r = (a1*a2 - 3.0*a0)/6.0 - a2*a2*a2/27.0;
   double d = q*q*q + r*r;
   // d is the discriminant, it determines number and type of roots
   //cout << "Discriminant is " << scientific << setprecision(2) << d << endl;
   double s1,s2;
   Evalues = Vector<double>(3);
   if(d > 0.0) {
      // 1 real root, pair of complex
      s1 = std::pow(r + std::sqrt(d), 1./3.);
      s2 = std::pow(r - std::sqrt(d), 1./3.);
      Evalues(0) = s1 + s2 - a2/3.0;
      Evalues(1) = -0.5*(s1+s2) - a2/3.0;          // real part
      Evalues(2) = std::sqrt(3.0)*(s1-s2)/2.0;     // imag part; ev's = re +- im
      return 1;
   }

   else if(d == 0.0) {
      // 3 real root, 2 of them equal
      s1 = std::pow(r, 1./3.);
      Evalues(0) = 2.0*s1 - a2/3.0;
      Evalues(1) = Evalues(2) = -s1 - a2/3.0;
      return 2;
   }

   // 3 distinct real roots
   d = -d;
   double R(std::sqrt(r*r+d));
   d = std::sqrt(d);
   double cth(r/R), sth(d/R);
   R = std::pow(R, 1./3.);
   double theta(std::atan2(d,r));
   s1 = 2.0*R*std::cos(theta/3.);      // actually s1+s2 in A+S
   s2 = 2.0*R*std::sin(theta/3.);      //          s1-s2
   Evalues(0) = s1 - a2/3.0;
   Evalues(1) = -0.5*s1 - a2/3.0 - std::sqrt(3.)*s2/2.0;
   Evalues(2) = -0.5*s1 - a2/3.0 + std::sqrt(3.)*s2/2.0;
   return 3;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Compute all eigenvalues and eigenvectors of a square symmetric matrix A; return
// the eigenvalues in vector D, the eigenvectors are the corresponding columns of V.
// Only the elements of A at and above the diagonal are used.
// This routine is not efficient; best to use only for dimension < ~10.
// param A covariance matrix, a symmetric Matrix<double> of dimension < ~10
// param D on output a Vector<double> containing eigenvalues of A
// param V on output a Matrix<double> containing eigenvectors of A in columns;
//          element n of D corresponds with column n of V.
// throw if input is invalid or if the algorithm fails.
#define JacobiRotate(A,i,j,k,l) { g=A(i,j); h=A(k,l); A(i,j)=g-s*(h+g*tau); A(k,l)=h+s*(g-h*tau); }
void EigenDecomp(Matrix<double> A, Vector<double>& D, Matrix<double>& V)
   throw(Exception)
{
   if(!A.isSymmetric())
      GNSSTK_THROW(Exception("Input must be a square symmetric matrix"));

   const unsigned int N(A.rows());
   unsigned int i,j,k; //,nrot;
   double tol,theta,tau,t,sum,s,h,g,c;
   Vector<double> B(N),Z(N,0.0);

   D = Vector<double>(N);
   V = ident<double>(N);
   for(i=0; i<N; i++) D(i) = B(i) = A(i,i);

   //nrot = 0;
   for(unsigned int iter=0; iter<50; iter++) {     // iterate
      sum = 0.0;
      for(k=0; k<N-1; k++)
         for(j=k+1; j<N; j++)
            sum += ::fabs(A(k,j));                 // sum |off-diag-elements|
      if(sum == 0.0)
         return;                                   // normal return

      if(iter < 4)
         tol = 0.2*sum/(N*N);
      else
         tol = 0.0;

      for(k=0; k<N-1; k++) {
         for(j=k+1; j<N; j++) {
            g = 100.0 * ::fabs(A(k,j));
            if(iter > 4 && double(::fabs(D(k)+g)) == double(::fabs(D(k)))
                        && double(::fabs(D(j)+g)) == double(::fabs(D(j))))
               A(k,j) = 0.0;
            else if(::fabs(A(k,j)) > tol) {
               h = D(j)-D(k);
               if(double(::fabs(h)+g) == double(::fabs(h)))
                  t = A(k,j)/h;
               else {
                  theta = 0.5 * h / A(k,j);
                  t = 1.0 / (::fabs(theta) + ::sqrt(1.0+theta*theta));
                  if(theta < 0.0) t = -t;
               }
               c = 1.0 / ::sqrt(1+t*t);
               s = t*c;
               tau = s/(1.0+c);
               h = t*A(k,j);
               Z(k) -= h;
               Z(j) += h;
               D(k) -= h;
               D(j) += h;
               A(k,j) = 0.0;
               for(i=0; i<k; i++)
                  JacobiRotate(A,i,k,i,j);
               for(i=k+1; i<j; i++)
                  JacobiRotate(A,k,i,i,j);
               for(i=j+1; i<N; i++)
                  JacobiRotate(A,k,i,j,i);
               for(i=0; i<N; i++)
                  JacobiRotate(V,i,k,i,j);
               //nrot++;
            }
         }
      }
      for(k=0; k<N; k++) {
         B(k) += Z(k);
         D(k) = B(k);
         Z(k) = 0.0;
      }
   }                                         // end iteration loop

   GNSSTK_THROW(Exception("Too many iterations"));
}
#undef JacobiRotate


bool isPositiveSemiDefinite(double aa, double ab, double ac, double bb, double bc, double cc)
{
    //get eigenvalues of the covariance matrix
    gnsstk::Matrix<double> covMat(3,3);

    covMat(0,0) = aa;
    covMat(1,1) = bb;
    covMat(2,2) = cc;

    covMat(0,1) = ab;
    covMat(0,2) = ac;
    covMat(1,2) = bc;

    covMat(1,0) = covMat(0,1);
    covMat(2,0) = covMat(0,2);
    covMat(2,1) = covMat(1,2);

    gnsstk::Vector<double> eigenValues(3);
    gnsstk::Matrix<double> eigenVectors(3,3);
    EigenDecomp(covMat,eigenValues,eigenVectors);

    double debug0 = eigenValues(0);
    double debug1 = eigenValues(1);
    double debug2 = eigenValues(2);

    bool isPositiveSemiDefinite = ( (eigenValues(0) > 1e-12) &&
                                    (eigenValues(1) > 1e-12) &&
                                    (eigenValues(2) > 1e-12) );

    return isPositiveSemiDefinite;
}

//------------------------------------------------------------------------------------
// small routine to manage confidence levels for ellipse
// the confidence factor is determined by sqrt(dimensionality * finv(level,dimensionality,dof))
// this is the
// the 95%ile corresponds with the scale factor used by GeoLab and also present
// in Table 19.3 and Table 19.4 in Ghilani
// in the future, this value should be computed instead of performing a lookup
// the indices correspond to the following:

double getConfidenceFactor(string level,int dimensionality, int dof) throw(Exception)
{
    if(dof <= 0) return 0;

    double confidence = 0.0;

    if(level == string("50")) confidence = 0.5;
    else if(level == string("1sig")) confidence = 0.68269;
    else if(level == string("90")) confidence = 0.9;
    else if(level == string("95")) confidence = 0.95;
    else if(level == string("99")) confidence = 0.99;
    else GNSSTK_THROW(Exception("Unknown confidence level: " + level));

   return ::sqrt(dimensionality * invFDistCDF(confidence,dimensionality,dof));

}

//------------------------------------------------------------------------------------
// Compute confidence ellipse for the given covariance matrix (NEU).
// Scaling with confidence factors is done by the caller.
// param CovNEU Matrix<double> 3x3 covariance matrix in North-East-Up
// param maj3 double output 3D-semi-major axis of ellipse
// param min3 double output 3D-semi-minor axis of ellipse
// param maj2 double output 2D(horizontal)-semi-major axis of ellipse
// param min2 double output 2D(horizontal)-semi-minor axis of ellipse
// param vert double output vertical (Up) axis of ellipse
// param azm double output azimuth of maj2
void getEllipse(const Matrix<double>& CovNEU, double& maj3, double& min3,
      double& maj2, double& min2, double& vert, double& azm) throw(Exception)
{
   try {
      unsigned int i;
      Matrix<double> cov(CovNEU),evecs;
      Vector<double> evals;

      // 3D decomposition
      EigenDecomp(cov,evals,evecs);

      maj3 = min3 = evals(0);
      for(i=1; i<3; i++) {
         if(evals(i) > maj3) maj3 = evals(i);
         if(evals(i) < min3) min3 = evals(i);
      }

      Matrix<double> cov2(Matrix<double>(CovNEU,0,0,2,2));
      // 2D major and minor axes - Leick 2.7.5
      //double W(::sqrt((pr.Cnn-pr.Cee)*(pr.Cnn-pr.Cee) + 4.*pr.Cne*pr.Cne));
      //double S((pr.Cnn+pr.Cee)/2.0);
      //double maj2 = S+W/2.0;
      //double min2 = S-W/2.0;
      //double azm(::atan2(2*pr.Cne/W,(pr.Cnn-pr.Cee)/W)*RAD_TO_DEG/2.0);
      // OR
      EigenDecomp(cov2,evals,evecs);
      maj2 = (evals(0) > evals(1) ? evals(0) : evals(1));
      min2 = (evals(0) > evals(1) ? evals(1) : evals(0));
      vert = cov(2,2);
      i = (evals(0) > evals(1) ? 0 : 1);
      azm = azimuth(evecs(1,i),evecs(0,i)) * ::RAD_TO_DEG;

      if(min2 < 0.0) min2 *= -1.0;

      maj3 = ::sqrt(maj3);
      min3 = ::sqrt(min3);
      maj2 = ::sqrt(maj2);
      min2 = ::sqrt(min2);
      vert = ::sqrt(vert);
   }
   catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void getEllipse

//------------------------------------------------------------------------------------
string getExecutablePath()
{
    string exePath=string("");

#ifndef _WIN32//linux only
   char buff[PATH_MAX];
   ssize_t len = ::readlink("/proc/self/exe",buff,sizeof(buff)-1);
   if(len != -1)
   {
       buff[len]='\0';
       exePath = string(buff);
       exePath = exePath.substr(0,exePath.rfind('/'));
   }
   else
       return string("");
#else
    char buff[MAX_PATH];
    GetModuleFileName(NULL,buff,MAX_PATH);
    exePath = string(buff);
    exePath = exePath.substr(0,exePath.rfind('\\'));
#endif
    return exePath;
}

//------------------------------------------------------------------------------------
void gen_random(char *s, const int len)
{
    static const char alphanum[] = 
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    for(int i=0; i<len; ++i)
        s[i] = alphanum[rand() % (sizeof(alphanum)-1)];

    s[len]=0;
}
//-----------------------------------------------------------------------------------

string quoteIfSpaces(string path)
{
    string newPath;

    if(path.find(' ')!=string::npos)
        newPath = string("\"") + path + string("\"");
    else
        newPath = path;

    return newPath;
}

QString addQuotes(QString input)
{
    return QString::fromStdString(quoteIfSpaces(input.toStdString()));
}


std::string addQuotes(std::string input)
{
    return quoteIfSpaces(input);
}

//-----------------------------------------------------------------------------------
unsigned int hashLine(string line, unsigned int hash)
{
    unsigned char *message = (unsigned char*)line.c_str();
    unsigned int byte, mask, crc=hash;
    int i=0, j;
    int buildWarningAvoider=0;

    try
    {
        while (message[i] != 0)
        {
            byte = message[i]; // Get next byte.
            crc = crc ^ byte;
            for (j = 7; j >= 0; j--)
            { // Do eight times.
//                mask = -(crc & 1);
                buildWarningAvoider = -(int)(crc & 1);
                mask = (unsigned int)buildWarningAvoider;
                crc = (crc >> 1) ^ (0xEDB88320 & mask);
            }
            i = i + 1;
        }
        return ~crc;
    }
    catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//-------------------------------------------------------------------------------------
int getPrecisionForUnits(const std::string &units, bool isPosition)
{
    if(std::getenv("salsaAppName") == NULL)
    {
        qputenv("salsaAppName","");
    }

    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    int retVal = 6;

    if ( isPosition )
    {
        if      (lsa::UNITS_KM  == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_KILOMETERS, lsa::DEFAULT_NUM_DECIMALS_LINEAR_POSITION_KM).toInt();
        else if (lsa::UNITS_M   == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_METERS, lsa::DEFAULT_NUM_DECIMALS_LINEAR_POSITION_METERS).toInt();
        else if (lsa::UNITS_CM  == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_CENTIMETERS, lsa::DEFAULT_NUM_DECIMALS_LINEAR_POSITION_CM).toInt();
        else if (lsa::UNITS_FT  == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_FEET, lsa::DEFAULT_NUM_DECIMALS_LINEAR_POSITION_FEET).toInt();
        else if (lsa::UNITS_SOA == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_SOA, lsa::DEFAULT_NUM_DECIMALS_ANGLE_POSITION_SOA).toInt();
        else if (lsa::UNITS_DEG == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_DEGREES, lsa::DEFAULT_NUM_DECIMALS_ANGLE_POSITION_DEGREES).toInt();
        else if (lsa::UNITS_RAD == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_RADIANS, lsa::DEFAULT_NUM_DECIMALS_ANGLE_POSITION_RADIANS).toInt();
    }
    else
    {
        if      (lsa::UNITS_KM  == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_KILOMETERS, lsa::DEFAULT_NUM_DECIMALS_LINEAR_MEASUREMENT_KM).toInt();
        else if (lsa::UNITS_M   == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_METERS, lsa::DEFAULT_NUM_DECIMALS_LINEAR_MEASUREMENT_METERS).toInt();
        else if (lsa::UNITS_CM  == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_CENTIMETERS, lsa::DEFAULT_NUM_DECIMALS_LINEAR_MEASUREMENT_CM).toInt();
        else if (lsa::UNITS_FT  == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_FEET, lsa::DEFAULT_NUM_DECIMALS_LINEAR_MEASUREMENT_FEET).toInt();
        else if (lsa::UNITS_SOA == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_SOA, lsa::DEFAULT_NUM_DECIMALS_ANGLE_MEASUREMENT_SOA).toInt();
        else if (lsa::UNITS_DEG == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_DEGREES, lsa::DEFAULT_NUM_DECIMALS_ANGLE_MEASUREMENT_DEGREES).toInt();
        else if (lsa::UNITS_RAD == units) retVal = qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_RADIANS, lsa::DEFAULT_NUM_DECIMALS_ANGLE_MEASUREMENT_RADIANS).toInt();
    }

    return retVal;
}

/*
 * Logic for this method previously existed in RecordEditor::configureNumDigits
 * Was moved into lsaUtils so that it will be accessible to the child classes
 * of LSARecord.
 */

int grabNumDigits(LSAType lsaType, QString units)
{
    return grabNumDigits(lsaType, units.toStdString());
}

int grabNumDigits(LSAType lsaType, std::string units)
{
    // convert to lower case.
    std::transform(units.begin(), units.end(), units.begin(), ::tolower);
    return getPrecisionForUnits(units, lsaType.isPosition());
}

string extractXMLFromString(string &inString, const std::pair<string, string> &xmlTag, bool bCut /* = false */)
{
    auto openerIdx = inString.find(xmlTag.first);
    if (openerIdx == std::string::npos)
    {
        return "";
    }
    
    std::string xmlCloseTag(xmlTag.second);
    // Need to know the indices for the inside of the tags.
    auto closerIdx = inString.rfind(xmlCloseTag);
    if (closerIdx == std::string::npos)
    {
        return "";
    }
    
    auto endOfOpener = openerIdx + xmlTag.first.size();
    // Get the substring into the member
    auto xmlContents = inString.substr(endOfOpener, closerIdx - endOfOpener);
    // Remove from the original.
    if(bCut)
    {
        auto endOfCloser = closerIdx + xmlCloseTag.size();
        inString.erase(openerIdx, endOfCloser - openerIdx);
    }
    
    return xmlContents;
}

//------------------------------------------------------------------------------------
/// determine if the fileInfo is for a merge file.
/// @param fileInfo   File info for the file in question
/// @return bool      true if this is a merge file
bool isMergeFile(const QFileInfo& fileInfo)
{
    auto filePath = fileInfo.filePath();
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream stream(&file);
    QString line = stream.readLine();
    bool bFoundHeader = false;
    while (!stream.atEnd())
    {
        if(!bFoundHeader)
        {
            if (line.contains("Read ") && line.contains(" files."))
            {
                bFoundHeader = true;
            }
            else
            {
                return bFoundHeader; // It should have been in the first line.
            }
        }

        if(line.contains("MERGEDSOL"))
        {
            return bFoundHeader;
        }
        line = stream.readLine();
    }
    return bFoundHeader;
}

//------------------------------------------------------------------------------------
/// determine if the fileInfo is for a Grape file.
/// @param fileInfo   File info for the file in question
/// @return bool      true if this is a grape file
bool isGrapeFile(const QFileInfo& fileInfo)
{
    auto filePath = fileInfo.filePath();
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream stream(&file);
    QString line = stream.readLine();
    while (!stream.atEnd())
    {
        if (line.contains("grape, the GPS/RINEX ARL:UT PPP estimator"))
        {
            return true;
        }
        line = stream.readLine();
    }
    return false;
}

//------------------------------------------------------------------------------------
/// determine if the fileInfo is for an OPUS file.
/// @param fileInfo   File info for the file in question
/// @return bool      true if this is an OPUS file
bool isOpusFile(const QFileInfo& fileInfo)
{
    auto filePath = fileInfo.filePath();
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    if(QFileInfo(file).suffix() == "opus")
    {
        return true;
    }

    QTextStream stream(&file);
    while (!stream.atEnd())
    {
        QString line = stream.readLine();
        if (line.contains("OPUS solution :") || line.contains("OPUS-RS solution :") ||
            line.contains("OPUS SOLUTION REPORT") || line.contains("OPUS-RS SOLUTION REPORT"))
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------------
/// determine if the fileInfo is for a Sets of Angles file (from a leica total station, probably).
/// @param fileInfo   File info for the file in question
/// @return bool      true if this is an SoA file
bool isSOAFile(const QFileInfo& fileInfo)
{
    auto filePath = fileInfo.filePath();
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    bool isType = false;
    QTextStream stream(&file);
    QString line = stream.readLine();
    while (!stream.atEnd())
    {
        if (line.contains("Leica Logfile - Begin"))
        {
            isType = true;
            break;
        }
        line = stream.readLine();
    }

    file.close();
    return isType;
}

//------------------------------------------------------------------------------------
/// determine if the fileInfo is for a Leica GNSS file (exported from Leica Geo Office).
/// @param fileInfo   File info for the file in question
/// @return bool      true if this is a Leica GNSS file
bool isLeicaGNSSFile(const QFileInfo& fileInfo)
{
    auto filePath = fileInfo.filePath();
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    bool isType = false;
    QTextStream stream(&file);
    QString line = stream.readLine();
    const QString at = "@";
    while (!stream.atEnd())
    {
        if (line.indexOf(at)==0)
        {
            isType = true;
            break;
        }
        line = stream.readLine();
    }

    file.close();
    return isType;
}

lsa::CONVERTER_TYPE getConverterTypeForFile(const QFileInfo &fileInfo)
{
    auto extension = fileInfo.suffix();
    extension = extension.toLower();

    if (extension == "asc")
    {
        if(isLeicaGNSSFile(fileInfo))
        {
            return lsa::CONVERTER_TYPE_LEICAGNSS;
        }
        else
        {
            return lsa::CONVERTER_TYPE_TRIMBLETDEF;
        }
    }
    else if (extension == "iob" || extension == "gps" || extension == "cob" || extension == "apx" || extension == "plh")
    {
        return lsa::CONVERTER_TYPE_IOB;
    }
    else if (extension == "opus")
    {
        return lsa::CONVERTER_TYPE_OPUS;
    }
    else if (extension == "gsi")
    {
        return lsa::CONVERTER_TYPE_GSI;
    }
    else if (extension == "proj" || extension == "lsa")
    {
        return lsa::CONVERTER_TYPE_NONE;
    }
    else                                                   // need to do some investigation to
    {                                                      // figure out what type of converter
        if(isMergeFile(fileInfo) || isGrapeFile(fileInfo)) // to use.
        {
            return lsa::CONVERTER_TYPE_PPP;
        }
        else if(isOpusFile(fileInfo))
        {
            return lsa::CONVERTER_TYPE_OPUS;
        }
        else if(isSOAFile(fileInfo))
        {
            return lsa::CONVERTER_TYPE_SETSOFANGLES;
        }

    }
    return lsa::CONVERTER_TYPE_NULL;
}

int getAngularMeasurementPrecisionSOA()
{
    if(std::getenv("salsaAppName") == NULL)
    {
        qputenv("salsaAppName","");
    }
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    return qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_SOA, lsa::DEFAULT_NUM_DECIMALS_ANGLE_MEASUREMENT_SOA).toInt();
}

int getAngularPositionPrecisionSOA()
{
    if(std::getenv("salsaAppName") == NULL)
    {
        qputenv("salsaAppName","");
    }
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    return qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_SOA, lsa::DEFAULT_NUM_DECIMALS_ANGLE_POSITION_SOA).toInt();
}

int getLinearMeasurementPrecisionMeters()
{
    if(std::getenv("salsaAppName") == NULL)
    {
        qputenv("salsaAppName","");
    }
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    return qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_METERS, lsa::DEFAULT_NUM_DECIMALS_LINEAR_MEASUREMENT_METERS).toInt();
}

int getLinearPositionPrecisionMeters()
{
    if(std::getenv("salsaAppName") == NULL)
    {
        qputenv("salsaAppName","");
    }
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    return qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_METERS, lsa::DEFAULT_NUM_DECIMALS_LINEAR_POSITION_METERS).toInt();
}

gnsstk::Matrix<double> convertECEFtoENU(std::pair<double, double> refLonLat, gnsstk::Matrix<double> ECEFmatrix)
{
    LSAENUUtil enuutil(refLonLat.first, refLonLat.second); // this needs to be in lon. lat. (radians)
    Matrix<double> rot = enuutil.getRotMatrix();
    auto ENUmatrix = rot * ECEFmatrix * transpose(rot);
    return ENUmatrix;
}
