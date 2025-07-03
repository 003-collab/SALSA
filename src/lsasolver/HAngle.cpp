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
/// @file HAngle.cpp  Class HAngle, horizontal angle for LSA solver

#include <string>

#include "stl_helpers.hpp"
#include "lsaobseqn.hpp"         // for use by ContributeObsEquation
//#include "logstream.hpp"

#include "HAngle.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
// write as a string
// @param prec integer number of digits precision (default 3)
// @param width integer width (default 8)
string HAngle::asDataString(const int prec, const int width) const
{
   ostringstream oss;
   oss << name();
   if(dsindex > -1) {
      oss << " DS=" << DSGroup << "[" << dsindex << "]";
   }
   oss << " " << DATHAngle::asString(prec,width);
   return oss.str();
}

//------------------------------------------------------------------------------------
// Compute the partial derivatives of this data w.r.t. the components of the
// "From" and "To" Point objects. If the Point is fixed, the partial is zero.
// Also compute the measured and nominal data values.
// NB measurement covariance is returned by a separate member routine.
//
// param pts   map with key=label, value=Point which must contain "At" "From" and "To"
// param Partials Vector<double> the 9-vector partials for this object.
// param MeasData double the measurment for this object.
// param NomData  double the nominal datum for this object.
// param doSOA    bool if true, express data in seconds-of-arc (else radians)
// throw if the "From" "At" or "To" Points are not found in the input.
void HAngle::ObservationEquation(const map<string,Point>& pts,
   Vector<double>& Partials, double& MeasuredData, double& NominalData,
   bool doSOA, bool is2D) throw(Exception)
{
try {
   // are the points there?
   map<string,Point>::const_iterator it;
   if((it=pts.find(From)) == pts.end())
      GNSSTK_THROW(Exception(string("From Point ") + From + string(" is not found")));
   const Point& F(it->second);
   if((it=pts.find(At)) == pts.end())
      GNSSTK_THROW(Exception(string("At Point ") + At + string(" is not found")));
   const Point& A(it->second);
   if((it=pts.find(To)) == pts.end())
      GNSSTK_THROW(Exception(string("To Point ") + To + string(" is not found")));
   const Point& T(it->second);

   // NB partials here are just the difference of two Azimuths.
   Partials = Vector<double>(9,0.0);

   // compute difference vectors from At to both targets: F-A and T-A in XYZ
   Vector<double> FmA(3), TmA(3);
   FmA(0) = F.x-A.x; FmA(1) = F.y-A.y; FmA(2) = F.z-A.z;
   TmA(0) = T.x-A.x; TmA(1) = T.y-A.y; TmA(2) = T.z-A.z;

   // 2D problem ---------------------------------------------------------------
   if(is2D) {
      // compute squared lengths
      double AFsq = FmA(0)*FmA(0)+FmA(1)*FmA(1)+FmA(2)*FmA(2);
      double ATsq = TmA(0)*TmA(0)+TmA(1)*TmA(1)+TmA(2)*TmA(2);

      // build the partials vector
      Partials(0) = -FmA(1)/AFsq;               // dx(F)
      Partials(1) =  FmA(0)/AFsq;               // dy(F)
      Partials(2) = 0.0;                        // dz(F)
      Partials(3) =  FmA(1)/AFsq - TmA(1)/ATsq; // dx(A)
      Partials(4) = -FmA(0)/AFsq + TmA(0)/ATsq; // dy(A)
      Partials(5) = 0.0;                        // dz(A)
      Partials(6) =  TmA(1)/ATsq;               // dx(T)
      Partials(7) = -TmA(0)/ATsq;               // dy(T)
      Partials(8) = 0.0;                        // dz(T)

      // nominal angle - difference of azimuths: atan2(E,N)(T) - atan2(E,N)(F)
      NominalData = ::atan2(FmA(1),FmA(0)) - ::atan2(TmA(1),TmA(0));

      // measured angle
      MeasuredData = value;
   }

   // 3D problem ---------------------------------------------------------------
   else {
      // get geodetic latitude and longitude in radians at From
      double latF,lonF,htF, latT,lonT,htT;
      F.getLatLonHeight(latF, lonF, htF);
      T.getLatLonHeight(latT, lonT, htT);

      // rotate into local (NEU) geodetic coordinates at At = instrument
      Matrix<double> Rot = A.getRotation();        // get rotation matrix XYZ->NEU
      Vector<double> neuFmA = Rot * FmA;
      Vector<double> neuTmA = Rot * TmA;

      // compute horizontal (NE) distance, sin and cos of nominal azimuth for both
      double FAhor(::sqrt(neuFmA(0)*neuFmA(0) + neuFmA(1)*neuFmA(1)));
      double sinazmFA(neuFmA(1)/FAhor);            // east/hor-dist
      double cosazmFA(neuFmA(0)/FAhor);            // north/hor-dist

      double TAhor(::sqrt(neuTmA(0)*neuTmA(0) + neuTmA(1)*neuTmA(1)));
      double sinazmTA(neuTmA(1)/TAhor);            // east/hor-dist
      double cosazmTA(neuTmA(0)/TAhor);            // north/hor-dist

      // build the partials vector in XYZ coordinates.
      // See Ghilani table 23.2 and Leick table 2.4
      // azimuth for A->F XYZ
      Partials(0) = -(::sin(latF)*::cos(lonF)*sinazmFA - ::sin(lonF)*cosazmFA)/FAhor;
      Partials(1) = -(::sin(latF)*::sin(lonF)*sinazmFA + ::cos(lonF)*cosazmFA)/FAhor;
      Partials(2) =  ::cos(latF)*sinazmFA/FAhor;                                    
      // azimuth for A->T XYZ, with opposite sign
      Partials(6) =  (::sin(latT)*::cos(lonT)*sinazmTA - ::sin(lonT)*cosazmTA)/TAhor;
      Partials(7) =  (::sin(latT)*::sin(lonT)*sinazmTA + ::cos(lonT)*cosazmTA)/TAhor;
      Partials(8) = -::cos(latT)*sinazmTA/TAhor;
      // both above terms, with opposite sign
      Partials(3) = -(Partials(0) + Partials(6));
      Partials(4) = -(Partials(1) + Partials(7));
      Partials(5) = -(Partials(2) + Partials(8));

      // nominal angle - defined as difference in azimuths, From to To at At
      double azmTA(::atan2(neuTmA(1),neuTmA(0)));
      double azmFA(::atan2(neuFmA(1),neuFmA(0)));
      NominalData = azmTA - azmFA;

      // measured angle
      MeasuredData = value;

      // compute corrections and apply to MeasuredData
      double corr(0.0), latA,lonA,htA;
      A.getLatLonHeight(latA, lonA, htA);

      // Height correction
      // Ghilani Eqn 23.34 pg 500
      // correct-meas = 0.108(SOA) *cos^2(lat_at)[sin(2az(at-from))*htFromGeodetic/1000
      //                                        - sin(2az(at-to))*htToGeodetic/1000]

      double sin2azmFA(sin(2.0*azmFA));       // east/hor-dist
      double sin2azmTA(sin(2.0*azmTA));       // east/hor-dist
      double coslatA = ::cos(latA);
      corr += ::SOA_TO_RAD*0.108*coslatA*coslatA
                           *(sin2azmTA*(htT+HtTo)-sin2azmFA*(htF+HtFrom))/1000.0;
      // deflection of vertical correction - Ghilani Eqn 23.32
      if(isGeoid) {
        double zangFA = Pi/2.0 - A.getVerticalAngleTo(F)      // nominal
                        + ::SOA_TO_RAD*(A.dovN*cosazmFA + A.dovE*sinazmFA);
        double zangTA = Pi/2.0 - A.getVerticalAngleTo(T)
                        + ::SOA_TO_RAD*(A.dovN*cosazmTA + A.dovE*sinazmTA);

        double tanlatA = ::tan(latA);
        double azmTAc = azmTA + ::SOA_TO_RAD*(
                        -(A.dovN*sinazmTA-A.dovE*cosazmTA)/::tan(zangTA) - A.dovE*tanlatA);
        double azmFAc = azmFA + ::SOA_TO_RAD*(
                        -(A.dovN*sinazmFA-A.dovE*cosazmFA)/::tan(zangFA) - A.dovE*tanlatA);

        corr += ::SOA_TO_RAD*(-(A.dovN*::sin(azmTAc) - A.dovE*::cos(azmTAc))/::tan(zangTA)
                              +(A.dovN*::sin(azmFAc) - A.dovE*::cos(azmTAc))/::tan(zangFA));
        /*Below is the original code: pre-1.5
        double zangFA = Pi/2.0 - A.getVerticalAngleTo(F)      // nominal
                        + ::SOA_TO_RAD*(A.dovN*cosazmFA + A.dovE*sinazmFA);
        double zangTA = Pi/2.0 - A.getVerticalAngleTo(T)
                        + ::SOA_TO_RAD*(A.dovN*cosazmTA + A.dovE*sinazmTA);

         double tanlatA = ::tan(latA);
         double azmTAc = azmTA + ::SOA_TO_RAD*(
               (A.dovN*sinazmTA-A.dovE*cosazmTA)/::tan(zangTA) - A.dovE*tanlatA);
         double azmFAc = azmFA + ::SOA_TO_RAD*(
               (A.dovN*sinazmFA-A.dovE*cosazmFA)/::tan(zangFA) - A.dovE*tanlatA);

         corr += ::SOA_TO_RAD*(
                  (A.dovN*::sin(azmTAc) - A.dovE*::cos(azmTAc))/::tan(zangTA)
                - (A.dovN*::sin(azmFAc) - A.dovE*::cos(azmTAc))/::tan(zangFA));*/
      }

      // correct the measurement
      MeasuredData += corr;
   }

   // correct for 2pi - must correct Nominal here
   // (data will be = measurement minus nominal)
   while(MeasuredData - NominalData > Pi) NominalData += TwoPi;
   while(MeasuredData - NominalData < -Pi) NominalData -= TwoPi;

   //LOG(INFO) << "HAngle " << label() << " measured " << fixed << setprecision(3)
   //   << MeasuredData*RAD_TO_DEG << " nominal " << NominalData*RAD_TO_DEG
   //   << " diff " << (MeasuredData-NominalData)*RAD_TO_DEG
   //   << " diff(SOA) " << (MeasuredData-NominalData)*RAD_TO_SOA;

   // convert to SOA
   if(doSOA) {
      Partials *= RAD_TO_SOA;
      NominalData *= RAD_TO_SOA;
      MeasuredData *= RAD_TO_SOA;
   }
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
// param doSOA    bool if true, express data in seconds-of-arc (else radians)
// param is2D     bool if true, problem is truly 2-dimensional (all Z==0)
void HAngle::FillObservationEquation(const map<string,Point>& pts,
                             const Namelist& Names,
                             FlexMatrix<double>& Partials,
                             Vector<double>& MeasData,
                             Vector<double>& NomData,
                             const int rowindex, bool doSOA, bool is2D)
   throw(Exception)
{
try {
   int i,k;
   string label;

   // get the observation equations (partials, meas data, nominal data)
   Vector<double> Parts;
   double MData, NData;
   ObservationEquation(pts, Parts, MData, NData, doSOA, is2D);

   // Copy partials in Parts into the full Partials matrix at rowindex, using Names
   // Parts is the vector { xFrom, yFrom, zFrom, xAt, yAt, zAt, xTo, yTo, zTo }
   for(i=0; i<3; i++) {                      // loop over x,y,z
      // find the index in the state for this component of "From"
      label = From + XYZ[i];
      k = Names.index(label);
      if(k > -1) Partials(rowindex,k) = Parts(i);

      // find the index in the state for this component of "At"
      label = At + XYZ[i];
      k = Names.index(label);
      if(k > -1) Partials(rowindex,k) = Parts(i+3);

      // find the index in the state for this component of "To"
      label = To + XYZ[i];
      k = Names.index(label);
      if(k > -1) Partials(rowindex,k) = Parts(i+6);
   }

   // Data
   MeasData(rowindex) = MData;
   NomData(rowindex) = NData;

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Return the full measurement variance (sigma squared), which includes all
// contributions such as additional sigma and centering errors.
// param pts  map with key=label, value=Point which must contain "From" and "To"
// param doSOA    bool if true, express in seconds-of-arc (else radians)
// param veccov on output, Vector<double> row of covariance matrix (DirSet)
// return measurement variance in radians or SOA
double HAngle::fullMeasurementVariance(const map<string,Point>& pts, bool doSOA,
                                       Vector<double>& veccov)
   throw(Exception)
{
   double var(0.0), cov(0.0);

   // if this came from a DirSet, there is a full covariance matrix
   // dsindex is the row index in the covariance matrix for this HAN of the DirSet
   if(dsindex > -1) {
      veccov = Vector<double>(MCovRow.size());
      for(unsigned int i=0; i<MCovRow.size(); i++)
         veccov(i) = MCovRow(i);
      var = veccov(dsindex);
   }
   else {
      veccov = Vector<double>(1);
      var = veccov(0) = sigma*sigma;
   }

   // centering errors
   // Ghilani Ch 7
   if(hasFcsig || hasTcsig || hasAcsig)
   {
      // are the points there?
      map<string,Point>::const_iterator it;
      if((it=pts.find(From)) == pts.end())
         GNSSTK_THROW(Exception(string("From Point ") + From+string(" is not found")));
      const Point& F(it->second);
      if((it=pts.find(At)) == pts.end())
         GNSSTK_THROW(Exception(string("At Point ") + At + string(" is not found")));
      const Point& A(it->second);
      if((it=pts.find(To)) == pts.end())
         GNSSTK_THROW(Exception(string("To Point ") + To + string(" is not found")));
      const Point& T(it->second);

      // covariance contribution from all centering
      double centercov(0.0);

      // compute lengths squared
      double FAsq, TAsq, FTsq;
      FAsq = (F.x-A.x)*(F.x-A.x)+(F.y-A.y)*(F.y-A.y)+(F.z-A.z)*(F.z-A.z);
      TAsq = (T.x-A.x)*(T.x-A.x)+(T.y-A.y)*(T.y-A.y)+(T.z-A.z)*(T.z-A.z);
      if(hasAcsig)
         FTsq = (F.x-T.x)*(F.x-T.x)+(F.y-T.y)*(F.y-T.y)+(F.z-T.z)*(F.z-T.z);

      // From and To centering: Ghilani Eqn 7.9 pg 109.
      if(hasFcsig || hasTcsig)
         centercov += Fcentersig*Fcentersig/FAsq + Tcentersig*Tcentersig/TAsq;

      // At centering: Ghilani Eqn 7.21 pg 113.
      if(hasAcsig)
         centercov += Acentersig*Acentersig*FTsq/(2.0*FAsq*TAsq);

      // add to the full measurement covariance
      var += centercov;
      veccov(dsindex > -1 ?  dsindex : 0) += centercov;
   }

   // convert to SOA and scale
   if(doSOA) {
      var *= (RAD_TO_SOA*RAD_TO_SOA);
      for(unsigned int i=0; i<veccov.size(); i++)
         veccov(i) *= (RAD_TO_SOA*RAD_TO_SOA);
   }
   if(datscale != 1.0) {
      var *= datscale;
      for(unsigned int i=0; i<veccov.size(); i++)
         veccov(i) *= datscale;
   }

   return var;
}

//------------------------------------------------------------------------------------
// Fill the given measurement covariance Matrix with covariance for this object.
// Insert covariance elements (~sigma^2) into the Matrix, at row index.
// param pts   map with key=label, value=Point which must contain "From" and "To"
// param MCov Matrix<double> of measurement covariance
// param index (row,col) index in MCov at which to add covariance(s)
// param doSOA    bool if true, express data in seconds-of-arc (else radians)
// throw if the "From" "At" or "To" Points are not found in the input.
// throw if sigma is zero
void HAngle::FillMeasCovariance(const map<string,Point>& pts,
                        FlexMatrix<double>& MCov, const int index, const bool doSOA)
   throw(Exception)
{
try {
   Vector<double> Covrow;
   double var = fullMeasurementVariance(pts, doSOA, Covrow);

   if(var == 0.0)
       GNSSTK_THROW(Exception("HANG Sigma is zero"));

   if(dsindex > -1) {
      // j is index in MCov of first col of the full DirSet MCov matrix
      int i,j(index-dsindex);
      for(i=0; i<Covrow.size(); i++) {
         MCov(index,j+i) = Covrow(i);
      }
   }
   else {
      MCov(index,index) = var;
   }
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// set notUsed=T if From and To are not found in the given list of labels
// return the new value of notUsed
bool HAngle::setUnused(const vector<string>& labels) throw()
{
   notUsed = true;
   if(vectorindex(labels,From) != -1) notUsed = false;
   if(vectorindex(labels,At) != -1) notUsed = false;
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
int HAngle::ContributeObsEquation(const map<string,Point>& pts, const int& index)
   throw(Exception)
{
   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation

   if(notUsed) return 0;                              // do nothing if this unused

   // fill the observation equation at index
   FillObservationEquation(pts, EQD.StateNames, EQD.Partials,
                           EQD.MeasData, EQD.NomData, index, EQD.doSOA, EQD.is2D);
   // fill the measurement covariance at index
   FillMeasCovariance(pts, EQD.MCov, index, EQD.doSOA);

   return 1;                                          // return 1 row defined
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
