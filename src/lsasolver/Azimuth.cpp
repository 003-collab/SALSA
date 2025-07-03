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
/// @file Azimuth.cpp  Class Azimuth, azimuth angle with sigma for LSA solver

#include <string>

#include "stl_helpers.hpp"
#include "lsaobseqn.hpp"         // for use by ContributeObsEquation
//#include "logstream.hpp"

#include "Azimuth.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
// Compute the partial derivatives of this data w.r.t. the components of the
// "From" and "To" Point objects. If the Point is fixed, the partial is zero.
// Also compute the measured and nominal data values.
//
// param pts  map with key=label, value=Point which must contain "From" and "To"
// param Partials Vector<double> the 4-vector partials for this object.
// param MeasData double the measurment for this object.
// param NomData  double the nominal datum for this object.
// param doSOA    bool if true, express data in seconds-of-arc (else radians)
// param is2D     bool if true, problem is truly 2-dimensional (all Z==0)
// return the 4-vector partials and 2 data values.
// throw if the "From" or "To" Points are not found in the input.
void Azimuth::ObservationEquation(const map<string,Point>& pts,
      Vector<double>& Partials, double& MeasuredData, double& NominalData,
      bool doSOA, bool is2D) throw(Exception)
{
try {
   // are the points there?
   map<string,Point>::const_iterator it;
   if((it=pts.find(From)) == pts.end())
      GNSSTK_THROW(Exception(string("From Point ") + From + string(" is not found")));
   const Point& F(it->second);
   if((it=pts.find(To)) == pts.end())
      GNSSTK_THROW(Exception(string("To Point ") + To + string(" is not found")));
   const Point& T(it->second);

   // compute difference vector T-F in XYZ (vector from F to T, instrument to target)
   Vector<double> TmF(3);
   TmF(0) = T.x-F.x; TmF(1) = T.y-F.y; TmF(2) = T.z-F.z;

   // compute slant length and squared length - independent of coordinate frame
   double TFsq = TmF(0)*TmF(0) + TmF(1)*TmF(1) + TmF(2)*TmF(2);
   double TF(::sqrt(TFsq));

   Partials = Vector<double>(6,0.0);

   // 2D problem ---------------------------------------------------------------
   if(is2D) {
      // build the partials vector in XYZ coordinates.  See Leick table 2.4
      Partials(0) = -TmF(1)/TFsq;
      Partials(1) =  TmF(0)/TFsq;
      Partials(2) = 0.0;
      Partials(3) =  TmF(1)/TFsq;
      Partials(4) = -TmF(0)/TFsq;
      Partials(5) = 0.0;

      NominalData = azimuth(TmF(0),TmF(1));           // azimuth(e,n);

      MeasuredData = value;
   }

   // 3D problem ---------------------------------------------------------------
   else {
      // rotate into local (NEU) geodetic coordinates at From = instrument
      Matrix<double> Rot = F.getRotation();        // get rotation matrix XYZ->NEU
      Vector<double> neuTmF = Rot * TmF;

      // get geodetic latitude and longitude in radians at From == instrument
      double latF,lonF,htF,latT,lonT,htT;
      F.getLatLonHeight(latF, lonF, htF);
      T.getLatLonHeight(latT, lonT, htT);

      // compute horizontal (NE) distance, plus sin and cos of nominal azimuth
      double TFhor(::sqrt(neuTmF(0)*neuTmF(0) + neuTmF(1)*neuTmF(1)));
      double sinazm(neuTmF(1)/TFhor);              // east/hor-dist
      double cosazm(neuTmF(0)/TFhor);              // north/hor-dist

      // build the partials vector in XYZ coordinates.  See Leick table 2.4
      Partials(0) = -(::sin(latF)*::cos(lonF)*sinazm - ::sin(lonF)*cosazm)/TFhor;//dxF
      Partials(1) = -(::sin(latF)*::sin(lonF)*sinazm + ::cos(lonF)*cosazm)/TFhor;//dyF
      Partials(2) =  ::cos(latF)*sinazm/TFhor;                                   //dzF

      Partials(3) = -Partials(0);                  // dxT
      Partials(4) = -Partials(1);                  // dyT
      Partials(5) = -Partials(2);                  // dzT

      // data
      NominalData = azimuth(neuTmF(1),neuTmF(0));

      MeasuredData = value;

      // corrections --  Leick, Table 9.1(a), p. 323
      // NB use NominalData,sinazm,cosazm, not measured, b/c this may be a Dir
      double corr(0.0);
      corr += ::SOA_TO_RAD * 0.108*cos(latF)*cos(latF)*sin(2.0*NominalData)
                                * (htT+HtTo)/1000.0;

      // Corrections DoV
      if(isGeoid) {           // Hofmann-Wellenhof (5-98)
         double zang = Pi/2.0 - F.getVerticalAngleTo(T)
            + ::SOA_TO_RAD*(F.dovN*cosazm + F.dovE*sinazm);
         double tanzang=tan(zang);
         corr +=SOA_TO_RAD*(-F.dovE*tan(latF)-(F.dovN*sinazm-F.dovE*cosazm)/tanzang);
         //Commented below is the original code (before July 28th, 2016)
         //corr += SOA_TO_RAD*(F.dovN*sinazm-F.dovE*(cosazm+::tan(latF)))/::tan(zang);
      }
      MeasuredData += corr;

      //LOG(INFO) << "Azimuth " << label() << " corrections "
      //   << fixed << setprecision(9) << corr << " rad"
      //   << " nomin data " << NominalData
      //   << " meas data " << MeasuredData;
   }

   // correct for 2pi - must correct Nominal here
   // (data will be = measurement minus nominal)
   while(MeasuredData - NominalData > Pi) NominalData += TwoPi;
   while(MeasuredData - NominalData < -Pi) NominalData -= TwoPi;

   //LOG(INFO) << "Azimuth " << label() << " measured " << fixed << setprecision(3)
   //   << MeasuredData*RAD_TO_DEG << " nominal " << NominalData*RAD_TO_DEG
   //   << " diff " << (MeasuredData-NominalData)*RAD_TO_SOA << " SOA";

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
void Azimuth::FillObservationEquation(const map<string,Point>& pts,
      const Namelist& Names, FlexMatrix<double>& Partials, Vector<double>& MeasData,
      Vector<double>& NomData, const int rowindex, bool doSOA, bool is2D)
   throw(Exception)
{
try {
   int i,k;
   string label;

   // get the observation equations (partials , meas data, nominal data)
   Vector<double> Parts;
   double MData, NData;
   ObservationEquation(pts, Parts, MData, NData, doSOA, is2D);

   // loop over x,y,z
   // Partials are ordered { xFrom, yFrom, zFrom, xTo, yTo, zTo }
   for(i=0; i<3; i++) {
      // find the index in the state for this component of "From"
      label = From + XYZ[i];
      k = Names.index(label);
      if(k > -1) Partials(rowindex,k) = Parts(i);

      // find the index in the state for this component of "To"
      label = To + XYZ[i];
      k = Names.index(label);
      if(k > -1) Partials(rowindex,k) = Parts(i+3);
   }

   // data
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
// return measurement variance in radians or SOA
double Azimuth::fullMeasurementVariance(const map<string,Point>& pts, bool doSOA)
   throw(Exception)
{
   double var(sigma*sigma), cov(0.0);

   // centering errors
   // Ghilani Ch 7
   if(hasFcsig || hasTcsig)
   {
      // are the points there?
      map<string,Point>::const_iterator it;
      if((it=pts.find(From)) == pts.end())
         GNSSTK_THROW(Exception(string("From Point ")+From+string(" is not found")));
      const Point& F(it->second);
      if((it=pts.find(To)) == pts.end())
         GNSSTK_THROW(Exception(string("To Point ") + To + string(" is not found")));
      const Point& T(it->second);

      // compute horizontal length squared
      Vector<double> deltasXYZ = Vector<double>(3);
      Vector<double> deltasENU = Vector<double>(3);
      deltasXYZ(0) = T.x - F.x;
      deltasXYZ(1) = T.y - F.y;
      deltasXYZ(2) = T.z - F.z;
      Matrix<double> Rot;
      Rot = F.getRotation();
      deltasENU = Rot*deltasXYZ;
      double dSq = deltasENU(0)*deltasENU(0) + deltasENU(1)*deltasENU(1);

      // From and To centering: Ghilani Eqn 7.9 pg 109.
      // TD is this right for From?
      var += (Tcentersig*Tcentersig + Fcentersig*Fcentersig)/dSq;
   }

   // convert to SOA
   if(doSOA) var *= (RAD_TO_SOA*RAD_TO_SOA);

   // scale
   if(datscale != 1.0) var *= datscale;

   return var;
}

//------------------------------------------------------------------------------------
// Fill the given measurement covariance Matrix with covariance for this object.
// Insert covariance elements (~sigma^2) into the Matrix, at row rowindex.
// param pts   map with key=label, value=Point which must contain "From" and "To"
// param MCov Matrix<double> of measurement covariance
// param index (row,col) index in MCov at which to add covariance(s)
// param doSOA    bool if true, express data in seconds-of-arc (else radians)
// throw if the "From" or "To" Points are not found in the input.
// throw if sigma is zero
void Azimuth::FillMeasCovariance(const map<string,Point>& pts,
                              FlexMatrix<double>& MCov, const int index, bool doSOA)
   throw(Exception)
{
try {
   // get the full variance
   double var = fullMeasurementVariance(pts, doSOA);

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
bool Azimuth::setUnused(const vector<string>& labels) throw()
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
int Azimuth::ContributeObsEquation(const map<string,Point>& pts, const int& index)
   throw(Exception)
{
   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation

   if(notUsed) return 0;                              // do nothing if this unused

   // fill the observation equation at index
   FillObservationEquation(pts, EQD.StateNames, EQD.Partials,
                           EQD.MeasData, EQD.NomData, index, EQD.doSOA, EQD.is2D);
   // fill the measurement covariance at index
   FillMeasCovariance(pts, EQD.MCov, index, EQD.doSOA);

   return 1;         // return 1 row defined
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
