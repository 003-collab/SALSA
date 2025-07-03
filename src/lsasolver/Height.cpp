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
/// @file Height.cpp  Include file for class Height, orthometric or geodetic height
/// measurement for LSA solver.

#include <string>

#include "stl_helpers.hpp"
#include "lsaobseqn.hpp"         // for use by ContributeObsEquation
//#include "logstream.hpp"

#include "Height.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
// Compute the partial derivatives of this data w.r.t. the components of the
// "From" and "To" Point objects. If the Point is fixed, the partial is zero.
// Also compute the measured and nominal data values.
//
// \verbatim
// Observation equation level height differences; Ghilani Sec 12.3, pg 211
//
// ^ U
// |           B (To) Bu
// |          /|
// |         / |
// |        /  |
// |       /   |
// |      /    |-------- EHD = Bu-Au = HB-HA 
// |     /     |
// |    /      |
// |   /       |
// |  A-(From)-|Au
// |
// 0--+--------+---> E,N
//
// This assumes that Au and Bu are both in same direction.
// In fact, due to the curvature of the ellipsoid, they are not.
// TBD: Curvature correction
//
// In NEU frame, EHDneu = {0, 0, HB-HA}
//
// Rotate EHD from NEU to XYZ (assumes Alat ~= Blat, etc.).
//
// EHDxyz = R * EHDneu
// 
// [EHDx]   [cos(lata)cos(lona)(HB-HA)]
// [EHDy] = [cos(lata)sin(lona)(HB-HA)]
// [EHDz]   [     sin(lata)(HB-HA)    ]
//
// Partial derivatives
// [dEHDx]   [cos(lata)cos(lona)(dxb-dxa)]
// [dEHDy] = [cos(lata)sin(lona)(dyb-dya)]
// [dEHDz]   [     sin(lata)(dzb-dza)    ]
//
// return 6-vector Partials and two data, where partials are ordered as below.
// Partial = { -cos(lata)cos(lona), cos(lata)cos(lona),
//             -cos(lata)sin(lona), cos(lata)sin(lona),
//             -sin(lata), sin(lata) }
// \endverbatim
//
// param pts  map with key=label, value=Point which must contain "From" and "To"
// param Partials Vector<double> the 6-vector partials for this object.
// param MeasData double the measurment for this object.
// param NomData  double the nominal datum for this object.
// return 6-vector partials Vector and two data.
// throw if the "From" or "To" Points are not found in the input.
void Height::ObservationEquation(const map<string,Point>& pts,
      Vector<double>& Partials, double& MeasuredData, double& NominalData)
   throw(Exception)
{
try {
   // are the points there?
   map<string,Point>::const_iterator it;
   if((it=pts.find(From)) == pts.end())
      GNSSTK_THROW(Exception(string("From Point ")
         + From + string(" is not found")));
   const Point& A(it->second);

   if((it=pts.find(To)) == pts.end())
      GNSSTK_THROW(Exception(string("To Point ")
         + To + string(" is not found")));
   const Point& B(it->second);

   // get rotation from NEU to XYZ for point A
   gnsstk::Matrix<double> RA(A.getRotation());
   //gnsstk::Matrix<double> RtA(transpose(RA));
   // get rotation from NEU to XYZ for point B
   gnsstk::Matrix<double> RB(B.getRotation());
   //gnsstk::Matrix<double> RtB(transpose(RB));

   // build the partials matrix
   Partials = Vector<double>(6,0.0);
   Partials(0) = -RA(2,0);  // dxa
   Partials(1) = RB(2,0);  // dxb
   Partials(2) = -RA(2,1);  // dya
   Partials(3) = RB(2,1);  // dyb
   Partials(4) = -RA(2,2);  // dza
   Partials(5) = RB(2,2);  // dzb

   //build the data vectors
   NominalData = B.getHeight()-A.getHeight();
   MeasuredData = value;

   // Corrections
   // Geoid -- orthometric height difference
   if(isGeoid){
      MeasuredData += (B.und - A.und);
   }

   // slant distance between From and To
   Vector<double> TmF(3);
   TmF(0) = B.x-A.x;
   TmF(1) = B.y-A.y;
   TmF(2) = B.z-A.z;
   double slant = ::sqrt( TmF(0)*TmF(0) + TmF(1)*TmF(1) + TmF(2)*TmF(2) );

   // Refraction
   if(hasRefract) {
       double refractCorr = refract*slant*slant/(2.0*Rearth);
       MeasuredData += refractCorr;
   }

   //Curvature
   if(isCurvature) {
       MeasuredData -= slant*slant/(2.0*Rearth);
   }

   //OHC
   if(isOHC) {
       double Alat = A.getLatitude(), Blat = B.getLatitude(), Ah = A.getHeight(), Bh = B.getHeight();
       double AOH = Ah - A.und, BOH = Bh - B.und;
       double sA=sin(Alat),sA2=pow(sA,2.),sB=sin(Blat),sB2 = pow(sB,2.);
       double s2A=sin(2.*Alat),s2A2=pow(s2A,2.),s2B=sin(2.*Blat),s2B2=pow(s2B,2.);
       double gamma0A = 9.780327*(1+5.3024e-03*sA2-(5.8e-06)*s2A2);
       double gamma0B = 9.780327*(1+5.3024e-03*sB2-(5.8e-06)*s2B2);
       double gamma045 = 9.806199;
       double gammaA = gamma0A-(3.0877e-01-(4.3e-04)*sA2)*(Ah/1000.)*(1.e-02)+(7.2e-05)*pow(Ah/1000.,2.)*(1.e-02);
       double gammaB = gamma0B-(3.0877e-01-(4.3e-04)*sB2)*(Bh/1000.)*(1.e-02)+(7.2e-05)*pow(Bh/1000.,2.)*(1.e-02);
       double rA = (gammaA - gamma045)/gamma045,rB = (gammaB - gamma045)/gamma045;
       double OHC = (rB+rA)*(BOH-AOH)/2. - (rB*BOH) + (rA*AOH) - (4.317e-08)*(BOH-AOH);

       MeasuredData += OHC;
   }

   //LOG(INFO) << "Height " << label() << " measured " << fixed << setprecision(3)
   //   << MeasuredData << " nominal " << NominalData
   //   << " diff " << (MeasuredData-NominalData);
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Fill the given Partials Matrix and Data Vector for the data in this object.
// Call ObservationEquation() to get the partials (a vector) and data values,
// then use the given Namelist to insert them into the LS quantities,
// starting at row rowindex.
// param pts      a map with key=label, value=Point of known Points
// param Names    Namelist for the state vector
// param Partials Matrix<double> of partials
// param MeasData Vector<double> of measurements
// param NomData  Vector<double> of nominal data values
// param rowindex index (in Data, and row in Partials) at which to begin adding
void Height::FillObservationEquation(const map<string,Point>& pts,
      const Namelist& Names, FlexMatrix<double>& Partials,
      Vector<double>& MeasData, Vector<double>& NomData, const int rowindex,
      const bool doSOA, const bool is2D)
   throw(Exception)
{
try {
   int i,j,k;
   string label;

   // get the observation equations (partials , meas data, nominal data)
   Vector<double> Parts;
   double MData, NData;
   ObservationEquation(pts, Parts, MData, NData);

   // Partial derivatives * State = Data
   //[ -Rt13, Rt13,               [(HB-HA)x,
   //  -Rt23, Rt23,   *  [ HA ] =  (HB-HA)y,
   //  -Rt33, Rt33]      [ HB ]    (HB-HA)z]
   //                     
   // loop over x,y,z
   for(j=0; j<3; j++) {
      // find the index in the state for this component of "From"
      label = From + XYZ[j];
      k = Names.index(label);
      if(k > -1) Partials(rowindex,k) = Parts(2*j);

      // find the index in the state for this component of "To"
      label = To + XYZ[j];
      k = Names.index(label);
      if(k > -1) Partials(rowindex,k) = Parts(2*j+1);
   }

   NomData(rowindex) = NData;
   MeasData(rowindex) = MData;

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Return the full measurement variance (sigma squared), which includes all
// contributions such as additional sigma and centering errors.
// param pts  map with key=label, value=Point which must contain "From" and "To"
// return measurement variance in meters
double Height::fullMeasurementVariance(const map<string,Point>& pts)
   throw(Exception)
{
   double var(sigma*sigma);

   // Note: UNCR GUI Record Additional Sigma is added to nominal sigma by preprocessor prior to writing .dat file.
   // The hasAddsig/addsig will only get set by SALSA GUI if PPM is set
   // In principle, addsig could be set in .dat file without being a PPM, but a .dat file will never be produced by the SALSA GUI in which that would be the case.
   // additional sigma; PPM referenced to the baseline, not the height measurement
   if(hasAddsig) {
      if(isPPM) {
         // get the baseline - are the points there?
         map<string,Point>::const_iterator it,jt;
         if((it=pts.find(From)) == pts.end() || (jt=pts.find(To)) == pts.end())
            GNSSTK_THROW(Exception(string("Points not found.")));
         // dist is the baseline in m
         double dist = it->second.distanceTo(jt->second);
         dist *= addsig;
         var += 1.0e-12 * dist * dist;
      }
      else
         var += addsig*addsig;
   }

   // scale
   if(datscale != 1.0) var *= datscale;

   return var;
}

//------------------------------------------------------------------------------------
// Fill the given measurement covariance Matrix with covariance for this object.
// Insert covariance elements (~sigma^2) into the Matrix, at row rowindex.
// param MCov Matrix<double> of measurement covariance
// param index (row,col) index in MCov at which to add covariance(s)
// throw if sigma is zero
void Height::FillMeasCovariance(const std::map<std::string,Point>& pts,
                                FlexMatrix<double>& MCov, const int index)
   throw(Exception)
{
try {
   // get the full variance
   double var = fullMeasurementVariance(pts);

   if(var == 0.0)
      GNSSTK_THROW(Exception("Sigma is zero"));

   // copy into matrix
   MCov(index,index) = var;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// set notUsed=T if From and To are not found in the given list of labels
// return the new value of notUsed
bool Height::setUnused(const vector<string>& labels) throw()
{
   notUsed = true;
   if(vectorindex(labels,From) != -1) notUsed = false;
   if(vectorindex(labels,To) != -1) notUsed = false;
   return notUsed;
}

//------------------------------------------------------------------------------------
// Fill the Partials Matrix and Data Vector for the data in this object.
// Call FillObservationEquation() and FillMeasurementCovariance() to get the
// partials, data values, and covariance, and use the state Namelist to insert
// them into the LS quantities, starting at row index.
// param pts   a map with key=label, value=Point of known Points
// param index index in Data, row in Partials and row,col in MCov at which to add
// return the number of rows contributed in the equation
int Height::ContributeObsEquation(const map<string,Point>& pts, const int& index)
   throw(Exception)
{
   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation

   if(notUsed) return 0;                              // do nothing if this unused

   // fill the observation equation at index
   FillObservationEquation(pts, EQD.StateNames, EQD.Partials,
                           EQD.MeasData, EQD.NomData, index);
   // fill the measurement covariance at index
   FillMeasCovariance(pts, EQD.MCov, index);

   return 1;         // return 1 row defined
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
