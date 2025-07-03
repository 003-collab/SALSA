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
/// @file VAngle.cpp  Class VAngle, vertical angle for LSA solver

#include <string>

#include "stl_helpers.hpp"
#include "lsaobseqn.hpp"         // for use by ContributeObsEquation
//#include "logstream.hpp"

#include "VAngle.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
// Compute the partial derivatives of this data w.r.t. the components of the
// "From" and "To" Point objects. If the Point is fixed, the partial is zero.
// Also compute the measured and nominal data values.
// NB measurement covariance is returned by a separate member routine.
// 
// TD need an ascii art picture
//
// param pts   map with key=label, value=Point which must contain "From" and "To"
// param Partials Vector<double> the 6-vector partials for this object.
// param MeasData double the measurment for this object.
// param NomData  double the nominal datum for this object.
// param doSOA    bool if true, express data in seconds-of-arc (else radians)
// throw if the "From" or "To" Points are not found in the input.
void VAngle::ObservationEquation(const map<string,Point>& pts,
   Vector<double>& Partials, double& MeasuredData, double& NominalData, bool doSOA)
   throw(Exception)
{
try {
   // are the points there?
   map<string,Point>::const_iterator it;

   if((it=pts.find(From)) == pts.end())
      GNSSTK_THROW(Exception(string("From Point ") + From + string(" is not found")));
   const Point& fromPoint(it->second);

   if((it=pts.find(To)) == pts.end())
      GNSSTK_THROW(Exception(string("To Point ") + To + string(" is not found")));
   const Point& toPoint(it->second);

   // get geodetic latitude and longitude in radians
   double latFrom,lonFrom,eHtFrom, latTo,lonTo,eHtTo;
   fromPoint.getLatLonHeight(latFrom, lonFrom, eHtFrom);
   toPoint.getLatLonHeight(  latTo,   lonTo,   eHtTo);

   // compute difference vector: To-From in XYZ
   Vector<double> TmF(3);
   TmF(0) = toPoint.x-fromPoint.x;
   TmF(1) = toPoint.y-fromPoint.y;
   TmF(2) = toPoint.z-fromPoint.z;

   // slant distance between From and To
   double slant = ::sqrt( TmF(0)*TmF(0) + TmF(1)*TmF(1) + TmF(2)*TmF(2) );

   // nominal angle
   NominalData = ::asin((TmF(0)*::cos(latFrom)*::cos(lonFrom) + 
                         TmF(1)*::cos(latFrom)*::sin(lonFrom) + 
                         TmF(2)*::sin(latFrom))/slant);

   // sine and cosine of the vertical angle
   double sinVAngle = ::sin(NominalData);
   double cosVAngle = ::cos(NominalData);

   // build the partials vector in XYZ coordinates.
   // See Leick table 2.4
   Partials = Vector<double>(6,0.0);
   Partials(0) = ((-slant*::cos(latFrom)*::cos(lonFrom) + sinVAngle*TmF(0)) / 
                   (slant*slant*cosVAngle));
   Partials(1) = ((-slant*::cos(latFrom)*::sin(lonFrom) + sinVAngle*TmF(1)) / 
                   (slant*slant*cosVAngle));
   Partials(2) = ((-slant*::sin(latFrom) + sinVAngle*TmF(2)) / 
                   (slant*slant*cosVAngle));
   Partials(3) = -Partials(0);
   Partials(4) = -Partials(1);
   Partials(5) = -Partials(2);

   // compute corrections and apply to MeasuredData
   double corr(0.0);

   // apply correction due to height of instrument, target
   double ellipCorr = eHtTo - eHtFrom - slant*slant/(2*Rearth);
   double measuredVAngleCorrected = ::atan( ::tan(value)
             //- (HtTo-HtFrom)
             - (HtTo*::cos(latTo-latFrom)*::cos(lonTo-lonFrom)-HtFrom)
                 / ::sqrt(slant*slant-ellipCorr*ellipCorr) );

   // measured data is vertical angle
   MeasuredData = measuredVAngleCorrected;

   // Apply correction due to deflection of the vertical
   if(isGeoid) {
       // calculate azimuth
       double azm(fromPoint.getAzimuth(toPoint));
       // NB: correction is equal to negative the Zenith correction
       MeasuredData -= ::SOA_TO_RAD*fromPoint.dovN*::cos(azm) + 
                       ::SOA_TO_RAD*fromPoint.dovE*::sin(azm);
   }

   // Refraction - Torge, 3rd Ed, p. 123
   if(hasRefract) {
       double refractCorr = refract*slant/(2.0*Rearth); // unitless
       MeasuredData -= refractCorr;   // added for Zangles, subtracted for Vangles
   }

   //LOG(INFO) << "VAngle " << label() << " measured " << fixed << setprecision(3)
   //   << MeasuredData*::RAD_TO_DEG << " nominal " << NominalData*::RAD_TO_DEG
   //   << " diff " << (MeasuredData-NominalData)*::RAD_TO_DEG;

   // convert to SOA
   if(doSOA)
   {
      Partials *= RAD_TO_SOA;
      NominalData *= RAD_TO_SOA;
      MeasuredData *= RAD_TO_SOA;
   }
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// TD units!
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
void VAngle::FillObservationEquation(const map<string,Point>& pts,
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

   // get the observation equations (partials , meas data, nominal data)
   Vector<double> Parts;
   double MData, NData;
   ObservationEquation(pts, Parts, MData, NData, doSOA);

   // loop over x,y,z
   // Partials are ordered { xFrom, yFrom, zFrom, xTo, yTo, zTo }
   for(i=0; i<3; i++)
   {
      // find the index in the state for this component of "From"
      label = From + XYZ[i];
      k = Names.index(label);
      if(k > -1) Partials(rowindex,k) = Parts(i);

      // find the index in the state for this component of "To"
      label = To + XYZ[i];
      k = Names.index(label);
      if(k > -1) Partials(rowindex,k) = Parts(i+3);
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
// return measurement variance in meters
double VAngle::fullMeasurementVariance(const map<string,Point>& pts, bool doSOA)
   throw(Exception)
{
   double var(sigma*sigma);
   double cov = 0.0;

   // centering errors and instrument height errors
   // Ghilani Ch 7
   if(hasFcsig || hasTcsig || hasHtFrom || hasHtTo)
   {
      // are the points there?
      map<string,Point>::const_iterator it;
      if((it=pts.find(From)) == pts.end())
         GNSSTK_THROW(Exception(string("From Point ") + From
            + string(" is not found")));
      const Point& F(it->second);

      if((it=pts.find(To)) == pts.end())
         GNSSTK_THROW(Exception(string("To Point ") + To
            + string(" is not found")));
      const Point& T(it->second);

      /*
        Analogous to Ghilani 5th Ed., Eqn 7.9, p 109, but for vertical angles

        up
         | Tcsig B
        D|  /---/---/
         |  / t/| /
         | / S/ |/
         | / / /|
         |/ / / |
         |///   |
         |/t____|______horz (neglecting curvature)
         A      C

         <CAB = <DBA = (t) = theta
         S = slope distance from A to B

         sin(deltaTheta)/Tcsig = sin(180-Theta-deltaTheta)/S
                               = [sin(180-Theta)cos(deltaTheta) - cos(180-Theta)sin(deltaTheta)]/S
         S/Tcsig               = sin(180-Theta)cos(deltaTheta)/sin(deltaTheta) - cos(180-Theta)
         S/Tcsig+cos(180-Theta)= sin(180-Theta)cos(deltaTheta)/sin(deltaTheta)
         sin(deltaTheta)/cos(deltaTheta) = sin(180-Theta)/[S/Tcsig+cos(180-Theta)
         deltaTheta should be small, so sin(deltaTheta)/cos(deltaTheta) ~ deltaTheta

         deltaTheta = sin(180-Theta)/[S/Tcsig + cos(180-Theta)]
                    = sin(Theta)/[S/Tcsig - cos(Theta)]

         The analogous follows for the From site.  So,

         cov = [sin(Theta)/(S/Tcsig - cos(Theta))]^2 + [sin(Theta)/[S/Fcsig - cos(Theta))]^2

      */
      double S = sqrt((F.x-T.x)*(F.x-T.x)+(F.y-T.y)*(F.y-T.y)+(F.z-T.z)*(F.z-T.z));
      double measuredVangle = value;
      cov = ((sin(measuredVangle)*Tcentersig)/(S - cos(measuredVangle)*Tcentersig))*
                   ((sin(measuredVangle)*Tcentersig)/(S - cos(measuredVangle)*Tcentersig)) +
                   ((sin(measuredVangle)*Fcentersig)/(S - cos(measuredVangle)*Fcentersig))*
                   ((sin(measuredVangle)*Fcentersig)/(S - cos(measuredVangle)*Fcentersig));

      // add to the full measurement covariance
      var += cov;


      // instrument height errors
      /* Self Derived
         sin(theta - deltaTheta) = (H - deltaH)/sqrt(d^2 + (H-deltaH)^2)
         sin(theta)cos(deltaTheta) - cos(theta)sin(deltaTheta) = (H - deltaH)/sqrt(d^2 + (H-deltaH)^2)

         Assume small angle approx.

         sin(theta) - cos(theta)deltaTheta = (H - deltaH)/sqrt(d^2 + (H-deltaH)^2)
         sin(theta) - cos(theta)deltaTheta = (H - deltaH)/sqrt(d^2 + (H-deltaH)^2)
         cos(theta)deltaTheta              = H/S - (H - deltaH)/sqrt(d^2 + (H-deltaH)^2)
         deltaTheta = [H/S - (H - deltaH)/sqrt(d^2 + (H-deltaH)^2)]/cos(theta)

         It is analagous for both sites. So,

         cov = {[H/S - (H - deltaHFrom)/sqrt(d^2 + (H-deltaHFrom)^2)]/cos(theta)}^2 + {[H/S - (H - deltaHTo)/sqrt(d^2 + (H-deltaHTo)^2)]/cos(theta)}^2
      */
      Vector<double> deltasXYZ = Vector<double>(3);
      Vector<double> deltasENU = Vector<double>(3);
      deltasXYZ(0) = T.x - F.x;
      deltasXYZ(1) = T.y - F.y;
      deltasXYZ(2) = T.z - F.z;
      Matrix<double> Rot;
      Rot = F.getRotation();
      deltasENU = Rot*deltasXYZ;
      double d = sqrt(deltasENU(0)*deltasENU(0) + deltasENU(1)*deltasENU(1));
      double H = deltasENU(2);

      double deltaThetaFrom = ((H/S) - ((H-HtFromSigma)/sqrt(d*d + (H-HtFromSigma)*(H-HtFromSigma))))/cos(measuredVangle);
      double deltaThetaTo = ((H/S) - ((H-HtToSigma)/sqrt(d*d + (H-HtToSigma)*(H-HtToSigma))))/cos(measuredVangle);


      cov = deltaThetaFrom*deltaThetaFrom + deltaThetaTo*deltaThetaTo;

      // Add to full measurement variance
      var += cov;
   }

   // convert to SOA
   if(doSOA) var *= (RAD_TO_SOA*RAD_TO_SOA);

   // scale
   if(datscale != 1.0) var *= datscale;

   return var;
}

//------------------------------------------------------------------------------------
// Fill the given measurement covariance Matrix with covariance for this object.
// Insert covariance elements (~sigma^2) into the Matrix, at row index.
// param pts   map with key=label, value=Point which must contain "From" and "To"
// param MCov Matrix<double> of measurement covariance
// param index (row,col) index in MCov at which to add covariance(s)
// param doSOA    bool if true, express data in seconds-of-arc (else radians)
// throw if the "From" or "To" Points are not found in the input.
// throw if sigma is zero
void VAngle::FillMeasCovariance(const map<string,Point>& pts,
      FlexMatrix<double>& MCov, const int index, const bool doSOA)
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
bool VAngle::setUnused(const vector<string>& labels) throw()
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
int VAngle::ContributeObsEquation(const map<string,Point>& pts, const int& index)
   throw(Exception)
{
   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation

   if(notUsed) return 0;                              // do nothing if this unused

   // fill the observation equation at index
   FillObservationEquation(pts, EQD.StateNames, EQD.Partials,
                           EQD.MeasData, EQD.NomData, index, EQD.doSOA);
   // fill the measurement covariance at index
   FillMeasCovariance(pts, EQD.MCov, index, EQD.doSOA);

   return 1;         // return 1 row defined
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
