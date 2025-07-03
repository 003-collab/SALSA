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
/// @file Dist.cpp  Class Dist, distance measurement for LSA solver

#include <string>

#include "stl_helpers.hpp"
#include "lsaobseqn.hpp"         // for use by ContributeObsEquation
//#include "logstream.hpp"

#include "Dist.hpp"

using namespace std;
using namespace gnsstk;

//------------------------------------------------------------------------------------
// Compute the partial derivatives of this data w.r.t. the components of the
// "From" and "To" Point objects. If the Point is fixed, the partial is zero.
// Also compute the measured and nominal data values.
//
// Observation equation for triangle; Ghilani Sec 17.8, pg 346
//
// ^ y
// |         T (To)
// |        /
// |       /
// |      /
// |     /
// |    /
// |   /
// |  F (From)
// |
// 0---------------> x
//
// AT = T - A = sqrt[(xb-xa)*(xb-xa)+(yb-ya)*(yb-ya)]
// Partial derivatives
// dAT = - [(xb-xa)/AT]dxb + [(xb-xa)/AT]dxa
//       - [(yb-ya)/AT]dyb + [(yb-ya)/AT]dya 
//       - [(zb-za)/AT]dzb + [(zb-za)/AT]dza 
//
// return 6-vector Partials and two data, where partials are ordered as below.
// Partial = { -[(xb-xa)/AT], [(xb-xa)/AT],
//             -[(yb-ya)/AT], [(yb-ya)/AT],
//             -[(zb-za)/AT], [(zb-za)/AT]}
//
// param pts  map with key=label, value=Point which must contain "From" and "To"
// param Partials Vector<double> the 6-vector partials for this object.
// param MeasData double the measurment for this object.
// param NomData  double the nominal datum for this object.
// return 6-vector partials Vector and two data.
// throw if the "From" or "To" Points are not found in the input.
void Dist::ObservationEquation(const map<string,Point>& pts,
      Vector<double>& Partials, double& MeasuredData, double& NominalData)
   throw(Exception)
{
try {
   // are the points there?
   map<string,Point>::const_iterator it;
   if((it=pts.find(From)) == pts.end())
      GNSSTK_THROW(Exception(string("From Point ")
         + From + string(" is not found")));
   const Point& F(it->second);

   if((it=pts.find(To)) == pts.end())
      GNSSTK_THROW(Exception(string("To Point ")
         + To + string(" is not found")));
   const Point& T(it->second);

   // compute apriori deltas and length
   double dx(T.x-F.x), dy(T.y-F.y), dz(T.z-F.z);
   double FT(::sqrt(dx*dx+dy*dy+dz*dz));

   // build the vector
   Partials = Vector<double>(6,0.0);
   Partials(0) = -dx/FT;  // dxb
   Partials(1) =  dx/FT;  // dxa
   Partials(2) = -dy/FT;  // dyb
   Partials(3) =  dy/FT;  // dya
   Partials(4) = -dz/FT;  // dzb
   Partials(5) =  dz/FT;  // dza

   // data
   NominalData = FT;

   // Correct distance for target,instrument heights (reduction to mark-to-mark dist)
   // Ref: See User Manual and "Types of Distances for Geodetic Observations", Ghilani, xyHt
   double EHtTo(T.getHeight()),EHtFrom(F.getHeight()); // ellipsoid height of target,instrument
   double EHtTotTo(EHtTo+HtTo),EHtTotFrom(EHtFrom+HtFrom); // total ellipsoid height of target,instrument
   double numerator(pow(value,2)-pow(EHtTotTo-EHtTotFrom,2)); // measured slant dist sq minus total hght diff sq
   double Rfactor((Rearth+EHtTo)*(Rearth+EHtFrom)/((Rearth+EHtTotTo)*(Rearth+EHtTotFrom))); //Rearth ratio
   double Dm(::sqrt(numerator*Rfactor+pow(EHtTo-EHtFrom,2))); //final mark-to-mark dist
   MeasuredData = Dm;

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
void Dist::FillObservationEquation(const map<string,Point>& pts,
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
   //[ (xb-xa)/FT, -(xb-xa)/FT, (yb-ya)/FT,
   //                 -(yb-ya)/FT, (zb-za)/FT, -(zb-za)/FT] *  [ Fx ] = FT
   //                                                          [ Tx ]
   //                                                          [ Fy ]
   //                                                          [ Ty ]
   //                                                          [ Fz ]
   //                                                          [ Tz ]
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
double Dist::fullMeasurementVariance(const std::map<std::string,Point>& pts) throw(Exception)
{
   double var(sigma*sigma);
   double cov = 0.0;

   // Note: UNCR GUI Record Additional Sigma is added to nominal sigma by preprocessor prior to writing .dat file.
   // The hasAddsig/addsig will only get set by SALSA GUI if PPM is set
   // In principle, addsig could be set in .dat file without being a PPM, but a .dat file will never be produced by the SALSA GUI in which that would be the case.
   // additional sigma
   if(hasAddsig)
   {
      var += addsig*addsig*(isPPM ? 1.0e-12*value*value : 1.0);
   }

   // instrument height uncertainty & centering errors
   if(hasTcsig || hasFcsig || hasHtFrom || hasHtTo)
   {
       // are the points there?
       map<string,Point>::const_iterator it;
       if((it=pts.find(From)) == pts.end())
          GNSSTK_THROW(Exception(string("From Point ")
             + From + string(" is not found")));
       const Point& F(it->second);

       if((it=pts.find(To)) == pts.end())
          GNSSTK_THROW(Exception(string("To Point ")
             + To + string(" is not found")));
       const Point& T(it->second);

       Vector<double> deltasXYZ = Vector<double>(3);
       Vector<double> deltasENU = Vector<double>(3);
       deltasXYZ(0) = T.x - F.x;
       deltasXYZ(1) = T.y - F.y;
       deltasXYZ(2) = T.z - F.z;
       Matrix<double> Rot;
       Rot = F.getRotation();
       deltasENU = Rot*deltasXYZ;

       double alpha = atan(abs(deltasENU(2))/sqrt(deltasENU(0)*deltasENU(0) + deltasENU(1)*deltasENU(1)));

       // instrument height errors
       if(hasHtFrom || hasHtTo)
       {
          cov = (HtFromSigma*HtFromSigma*sin(alpha)*sin(alpha) + HtToSigma*HtToSigma*sin(alpha)*sin(alpha));
          var += cov;
       }

       // centering errors
       if(hasFcsig || hasTcsig)
       {
          cov = (Fcentersig*Fcentersig*cos(alpha)*cos(alpha) + Tcentersig*Tcentersig*cos(alpha)*cos(alpha));
          var += cov;
       }
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
void Dist::FillMeasCovariance(const std::map<std::string,Point>& pts, FlexMatrix<double>& MCov, const int index)
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
bool Dist::setUnused(const vector<string>& labels) throw()
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
int Dist::ContributeObsEquation(const map<string,Point>& pts, const int& index)
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
