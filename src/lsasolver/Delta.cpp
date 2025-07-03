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
/// @file Delta.cpp  Class Delta, 3-D XYZ coordinate difference measurement for
///                  LSA solver.

#include <string>

#include "stl_helpers.hpp"
#include "lsaobseqn.hpp"         // for use by ContributeObsEquation
//#include "logstream.hpp"

#include "Delta.hpp"

using namespace std;
using namespace gnsstk;

//------------------------------------------------------------------------------------
// Compute the partial derivatives of this data w.r.t. the components of the
// "From" and "To" Point objects. If the Point is fixed, the partial is zero.
// Also compute the measured and nominal data vectors.
// NB measurement covariance is returned by a separate member routine.
// Observation equations for delta; Ghilani Sec 17.8, pg 346
// ^ y
// |         B (To)
// |        /
// |       /
// |      /
// |     /
// |    /
// |   /
// |  A (From)
// |
// 0---------------> x
//
// Bx = Ax + dxAB
// By = Ay + dyAB
// Bz = Az + dzAB
// Partial derivatives * State = Data(Measured-Nominal);   Meas.Cov (symmetric)
// [-1  1  0  0  0  0 ] [Ax]     [ dx ]                   [ covxx covxy covxz ]
// [ 0  0 -1  1  0  0 ] [Bx]  =  [ dy ]                   [   .   covyy covyz ]
// [ 0  0  0  0 -1  1 ] [Ay]     [ dz ]                   [   .     .   covzz ]
//                      [By]
//                      [Az]
//                      [Bz]
//
// return 3x6 matrix of partials, and measured and nominal data vectors
//          [-1  1  0  0  0  0]  [ dx ]  [Bx-Ax]
//          [ 0  0 -1  1  0  0]  [ dy ]  [By-Ay]
//          [ 0  0  0  0 -1  1]  [ dz ]  [Bz-Az]
//
// param pts  map with key=label, value=Point which must contain "From" and "To"
// param Partials Matrix<double> the 3x6 partials matrix for this object.
// param MeasuredData Vector<double> the 3-vector of measurments for this object.
// param NominalData  Vector<double> the 3-vector of nominal data for this object.
// throw  if the "From" or "To" Points are not found in the input.
void Delta::ObservationEquation(const map<string,Point>& pts,
                            Matrix<double>& Partials,
                            Vector<double>& MeasuredData,
                            Vector<double>& NominalData)
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

   // build the partials matrix
   Partials = Matrix<double>(3,6,0.0);
   Partials(0,0) = Partials(1,2) = Partials(2,4) = -1.0;
   Partials(0,1) = Partials(1,3) = Partials(2,5) = 1.0;

   // build the data vectors
   MeasuredData = Vector<double>(3);
   NominalData = Vector<double>(3);
   MeasuredData(0) = dx;
   MeasuredData(1) = dy;
   MeasuredData(2) = dz;
   NominalData(0) = B.x - A.x;
   NominalData(1) = B.y - A.y;
   NominalData(2) = B.z - A.z;

   // compute corrections and apply to MeasuredData
   if(hasHtFrom || hasHtTo) {
      // compute vector offsets of From and To in ECEF XYZ
      Vector<double> FromOff(3,0.0),ToOff(3,0.0),Ht(3);
      Matrix<double> Rot;
      if(hasHtFrom) {
         Ht(0) = Ht(1) = 0.0;
         Ht(2) = HtFrom;
         Rot = A.getRotation();
         FromOff = inverse(Rot)*Ht;
      }
      if(hasHtTo) {
         Ht(0) = Ht(1) = 0.0;
         Ht(2) = HtTo;
         Rot = A.getRotation();
         ToOff = inverse(Rot)*Ht;
      }
      MeasuredData -= ToOff - FromOff;

      //LOG(INFO) << "Correction (XYZ,m) to DEL is "
      //      << scientific << setprecision(3) << FromOff-ToOff;

      // TD refraction?
   }

   //LOG(INFO) << "Delta " << label() << " measured " << fixed << setprecision(3)
   //   << MeasuredData << " nominal " << NominalData
   //   << " diff " << (MeasuredData-NominalData);
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Fill the given Partials Matrix and Data Vector for the data in this object.
// Call ObservationEquation() to get the partials and data vectors, then use
// the given Namelist to insert them into the LS quantities,
// starting at row rowindex.
// param pts      a map with key=label, value=Point of known Points
// param Names    Namelist for the state vector
// param Partials Partials matrix of the full LS equation
// param MeasData Measurement data vector
// param NomData  Nominal values for the data vector
// param rowindex index (in Data, and row in Partials) at which to begin adding
void Delta::FillObservationEquation(const map<string,Point>& pts,
      const Namelist& Names, FlexMatrix<double>& Partials,
      Vector<double>& MeasData, Vector<double>& NomData, const int rowindex,
      const bool doSOA, const bool is2D)
   throw(Exception)
{
try {
   int i,j,k;
   string label;

   // get the observation equations (partials , meas data, nominal data)
   Matrix<double> Parts;
   Vector<double> MData, NData;
   ObservationEquation(pts, Parts, MData, NData);

   // Partial derivatives * State = Data(Meas-Nominal);   Meas.Cov (symmetric)
   // [-1  1  0  0  0  0 ] [Ax]     [ dx ]               [ covxx covxy covxz ]
   // [ 0  0 -1  1  0  0 ] [Bx]  =  [ dy ]               [   .   covyy covyz ]
   // [ 0  0  0  0 -1  1 ] [Ay]     [ dz ]               [   .     .   covzz ]
   //                      [By]
   //                      [Az]
   //                      [Bz]

   // loop over x,y,z
   for(j=0; j<3; j++) {
      // find the index in the state for this component of "From"
      label = From + XYZ[j];
      k = Names.index(label);
      if(k > -1) Partials(rowindex+j,k) = Parts(j,2*j);

      // find the index in the state for this component of "To"
      label = To + XYZ[j];
      k = Names.index(label);
      if(k > -1) Partials(rowindex+j,k) = Parts(j,2*j+1);

      // data
      MeasData(rowindex+j) = MData(j);
      NomData(rowindex+j) = NData(j);
   }
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Return the full measurement covariance matrix, that is the part defined
// by the UT components in the constructor plus all the additions due to additional
// sigma and centering errors.
// return 3x3 measurement covariance matrix
// throw if sigma is zero
FlexMatrix<double> Delta::fullMeasurementCovariance(const map<string,Point>& pts) throw(Exception)
{
try {
   int j,k;

   // get the nominal measurement covariance
   Matrix<double> MCov = measurementCovariance();
   double cov = 0.0;

   // Note: UNCR GUI Record Additional Sigma is added to nominal sigma by preprocessor prior to writing .dat file.
   // The hasAddsig/addsig will only get set by SALSA GUI if PPM is set
   // In principle, addsig could be set in .dat file without being a PPM, but a .dat file will never be produced by the SALSA GUI in which that would be the case.
   // additional sigma
   if(hasAddsig)
   {
      double dist2((dx*dx+dy*dy+dz*dz)*1.e-12);    // ?? /addsig ?
      MCov(0,0) += addsig*addsig*(isPPM ? dist2 : 1.0);
      MCov(1,1) += addsig*addsig*(isPPM ? dist2 : 1.0);
      MCov(2,2) += addsig*addsig*(isPPM ? dist2 : 1.0);
   }

   // centering & instrument height uncertainty errors
   if(hasHtFrom || hasHtTo || hasFcsig || hasTcsig) {

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

      gnsstk::Vector<double> HtFromVecXYZ(3,0.0), HtToVecXYZ(3,0.0), HtFromVecENU(3,0.0), HtToVecENU(3,0.0); // Need to transform to XYZ

      // compute vector offsets of From and To in ECEF XYZ
      Matrix<double> RotFrom, RotTo;
      Matrix<double> CenteringErrorMatrix(3,3,0.0), HeightErrorMatrix(3,3,0.0), CenteringBase(3,3,0.0), HeightBase(3,3,0.0);
      RotFrom = F.getRotation(); //xyz2enu
      RotTo = T.getRotation();
      RotFrom = inverse(RotFrom); //enu2xyz
      RotTo = inverse(RotTo);

      HeightBase(2,2) = 1;
      CenteringBase(0,0) = 1;
      CenteringBase(1,1) = 1;

      if(hasHtFrom)
      {
          HeightErrorMatrix = HtFromSigma*HtFromSigma*RotFrom*HeightBase*transpose(RotFrom);
          MCov += HeightErrorMatrix;
      }

      if(hasHtTo)
      {
          HeightErrorMatrix = HtToSigma*HtToSigma*RotTo*HeightBase*transpose(RotTo);
          MCov += HeightErrorMatrix;
      }

      if(hasFcsig)
      {
          CenteringErrorMatrix = Fcentersig*Fcentersig*RotFrom*CenteringBase*transpose(RotFrom);
          MCov += CenteringErrorMatrix;
      }

      if(hasTcsig)
      {
          CenteringErrorMatrix = Tcentersig*Tcentersig*RotTo*CenteringBase*transpose(RotTo);
          MCov += CenteringErrorMatrix;
      }
   }

   // scale
   if(datscale != 1.0)
   {
      MCov *= datscale;
   }

   return MCov;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Fill the given measurement covariance Matrix with covariance for this object.
// Insert covariance elements (~sigma^2) into the Matrix, at row rowindex.
// param MCov Matrix<double> of measurement covariance
// param index (row,col) index in MCov at which to add covariance(s)
// throw if sigma is zero
void Delta::FillMeasCovariance(const map<string,Point>& pts,FlexMatrix<double>& MCov, const int index)
   throw(Exception)
{
try {
   // get the full measurement covariance
   Matrix<double> mcov = fullMeasurementCovariance(pts);
   
   // TD? test for singular mcov
   //try {
   //   Matrix<double> dummy(inverseLUD(mcov));
   //}
   //catch(Exception& e) { GNSSTK_RETHROW(e); }

   for(unsigned int j=0; j<3; j++)
      for(unsigned int k=0; k<3; k++)
         MCov(index+j,index+k) = mcov(j,k);
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// get the slant range (distance) defined by this Delta
// return the slant range defined by this object
double Delta::distance(void) const throw(Exception)
{
   return (::sqrt(dx*dx + dy*dy + dz*dz));
}

//------------------------------------------------------------------------------------
// set notUsed=T if From and To are not found in the given list of labels
// return the new value of notUsed
bool Delta::setUnused(const vector<string>& labels) throw()
{
   notUsed = true;
   if(vectorindex(labels,From) != -1) notUsed = false;
   if(vectorindex(labels,To) != -1) notUsed = false;
   return notUsed;
}

//------------------------------------------------------------------------------------
// define names for the Data vector for this object, adding it(them) to the
// given DataNamelist.
// parameter Names vector<string> to which new names are added
// parameter tag string to incorporate into the name
// return the number of names added
int Delta::addDataNames(const string& tag, vector<string>& Names) throw()
{
   if(notUsed) return 0;
   Names.push_back(name() + "." + tag + "("+label()+")X");
   Names.push_back(name() + "." + tag + "("+label()+")Y");
   Names.push_back(name() + "." + tag + "("+label()+")Z");
   return 3;
}

//------------------------------------------------------------------------------------
// Fill the Partials Matrix and Data Vector for the data in this object.
// Call FillObservationEquation() and FillMeasurementCovariance() to get the
// partials, data values, and covariance, and use the state Namelist to insert
// them into the LS quantities, starting at row index.
// param pts   a map with key=label, value=Point of known Points
// param index index in Data, row in Partials and row,col in MCov at which to add
// return the number of rows contributed in the equation
int Delta::ContributeObsEquation(const map<string,Point>& pts, const int& index)
   throw(Exception)
{
   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation

   if(notUsed) return 0;                              // do nothing if this unused

   // fill the observation equation at index
   FillObservationEquation(pts, EQD.StateNames, EQD.Partials,
                           EQD.MeasData, EQD.NomData, index);

   // fill the measurement covariance at index
   FillMeasCovariance(pts, EQD.MCov, index);

   return 3;                                          // return 3 rows defined
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
