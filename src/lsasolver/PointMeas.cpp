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
/// @file PointMeas.cpp  Class PointMeas, point pseudo-measurements
///        used when control points are to be adjusted (Point with ADJ, fixtype==3).

#include <string>
#include <sstream>

#include "stl_helpers.hpp"
#include "lsaobseqn.hpp"         // for use by ContributeObsEquation
#include "logstream.hpp"

#include "PointMeas.hpp"

using namespace std;
using namespace gnsstk;

//------------------------------------------------------------------------------------
// Write the object as a 3-line string of the form
//
// \verbatim
//     PsPos label  X-comp     covXX  covXY  covXZ
//                  Y-comp            covYY  covYZ
//                  Z-comp                   covZZ
// \endverbatim
// param msg string to place at first of string
// param prec integer number of digits precision (default 2)
// param width integer width (default 7)
// return string version of the object
string PointMeas::asString3(const string msg, const int prec, const int width) const
{
   int ncov;
   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation
   string lab,spacer;
   ostringstream oss;
   oss << msg << " " << label;
   lab = oss.str(); oss.str("");
   oss << scientific << setprecision(3) << -::fabs(covxx);
   spacer = oss.str(); oss.str("");
   ncov = spacer.size();
   spacer = StringUtils::rightJustify(" ",ncov);
   oss << lab << " X" << fixed << setprecision(prec)
      << " " << setw(width) << x << scientific << setprecision(3)
      << "  " << setw(ncov) << covxx << " " << setw(ncov) << covxy;
   if(!EQD.is2D) oss << " " << setw(ncov) << covxz;
   if(!dattag.empty()) oss << " tag=" << dattag;
   oss << "\n";

   lab = StringUtils::rightJustify(" ",lab.size()+1);
   oss << lab << " Y" << fixed << setprecision(prec)
      << " " << setw(width) << y << scientific << setprecision(3)
      << "  " << spacer << " " << setw(ncov) << covyy;
   if(!EQD.is2D) {
      oss << " " << setw(ncov) << covyz << "\n";

      oss << lab << " Z" << fixed << setprecision(prec)
         << " " << setw(width) << z << scientific << setprecision(3)
         << "  " << spacer << " " << spacer << " " << setw(ncov) << covzz;
   }

   return oss.str();
}

//------------------------------------------------------------------------------------
// Write data as a string.
string PointMeas::asString(const int prec, const int width) const
{
   ostringstream oss;
   oss << name() << " " << label << " is" << fixed << setprecision(prec)
      << " " << x << " " << y << " " << z
      << "  with cov(XYZ,UT):" << scientific
      << setprecision(2) << " " << covxx << " " << covxy << " " << covxz
      << " " << covyy << " " << covyz << " " << covzz << " M";
   if(!dattag.empty()) oss << " tag=" << dattag;
   
   return oss.str();
}

//------------------------------------------------------------------------------------
// Compute the partial derivatives of this data w.r.t. the components of the
// Point object.
// Also compute the measured and nominal data vectors.
// NB measurement covariance is returned by a separate member routine.
// param pts  map with key=label, value=Point which must contain label.
// param Partials Matrix<double> the 3x3 partials matrix for this object.
// param MeasuredData Vector<double> the 3-vector of measurments for this object.
// param NominalData  Vector<double> the 3-vector of nominal data for this object.
// throw  if the Point's label is not found in the input.
void PointMeas::ObservationEquation(const map<string,Point>& pts,
                            Matrix<double>& Partials,
                            Vector<double>& MeasuredData,
                            Vector<double>& NominalData)
      throw(Exception)
{
try {
   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation
   // is the point there?
   map<string,Point>::const_iterator it;
   if((it=pts.find(label)) == pts.end())
      GNSSTK_THROW(Exception(string("Label ") + label + string(" is not found")));
   const Point& A(it->second);

   // build the partials matrix
   Partials = Matrix<double>(3,3,0.0);
   Partials(0,0) = Partials(1,1) = Partials(2,2) = 1.0;

   // build the data vectors
   MeasuredData = Vector<double>(3);
   NominalData = Vector<double>(3);
   MeasuredData(0) = A.x - x;
   MeasuredData(1) = A.y - y;
   MeasuredData(2) = A.z - z;
   NominalData(0) = 0.0;
   NominalData(1) = 0.0;
   NominalData(2) = 0.0;

   //LOG(INFO) << "PointMeas " << label() << " measured " << fixed << setprecision(3)
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
void PointMeas::FillObservationEquation(const map<string,Point>& pts,
      const Namelist& Names, FlexMatrix<double>& Partials,
      Vector<double>& MeasData, Vector<double>& NomData, const int rowindex,
      const bool doSOA, const bool is2D)
   throw(Exception)
{
try {
   int i,j,k;
   string lab;

   // get the observation equations (partials , meas data, nominal data)
   Matrix<double> Parts;
   Vector<double> MData, NData;
   ObservationEquation(pts, Parts, MData, NData);

   // Partial derivatives * State = Data(Meas-Nominal);   Meas.Cov (symmetric)
   // [ 1  0  0 ] [Ax]     [ A.x-x ]               [ covxx covxy covxz ]
   // [ 0  1  0 ] [Ay]  =  [ A.y-y ]               [   .   covyy covyz ]
   // [ 0  0  1 ] [Az]     [ A.z-z ]               [   .     .   covzz ]

   // loop over x,y,z
   for(j=0; j<3; j++) {
      // find the index in the state for this component of "label"
      lab = label + XYZ[j];
      k = Names.index(lab);
      if(k > -1) Partials(rowindex+j,k) = Parts(j,j);

      // data
      MeasData(rowindex+j) = MData(j);
      NomData(rowindex+j) = NData(j);

      if(is2D && j==1) break;
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
FlexMatrix<double> PointMeas::fullMeasurementCovariance(void) throw(Exception)
{
try {
   // get the nominal measurement covariance
   Matrix<double> MCov(3,3,0.0);
   MCov(0,0) = covxx;
   MCov(0,1) = MCov(1,0) = covxy;
   MCov(0,2) = MCov(2,0) = covxz;
   MCov(1,1) = covyy;
   MCov(1,2) = MCov(2,1) = covyz;
   MCov(2,2) = covzz;

   // scale
   if(datscale != 1.0) MCov *= datscale;

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
void PointMeas::FillMeasCovariance(FlexMatrix<double>& MCov, const int index)
   throw(Exception)
{
try {
   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation
   Matrix<double> mcov = fullMeasurementCovariance();

   // TD? test for singular mcov

   unsigned int j,k,n(EQD.is2D ? 2 : 3);
   for(j=0; j<n; j++)
      for(k=0; k<n; k++)
         MCov(index+j,index+k) = mcov(j,k);

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// set notUsed=T if From and To are not found in the given list of labels
// return the new value of notUsed
bool PointMeas::setUnused(const vector<string>& labels) throw()
{
   notUsed = true;
   if(vectorindex(labels,label) != -1) notUsed = false;
   return notUsed;
}

//------------------------------------------------------------------------------------
// define names for the Data vector for this object, adding it(them) to the
// given DataNamelist.
// param Names vector<string> to which new names are added
// param tag string to incorporate into the name
// return the number of names added
int PointMeas::addDataNames(const string& tag, vector<string>& Names) throw()
{
   if(notUsed) return 0;

   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation
   Names.push_back(name() + "." + tag + "(" + label + ")X");
   Names.push_back(name() + "." + tag + "(" + label + ")Y");
   if(!EQD.is2D)
      Names.push_back(name() + "." + tag + "(" + label + ")Z");

   return (EQD.is2D ? 2:3);
}

//------------------------------------------------------------------------------------
// Fill the Partials Matrix and Data Vector for the data in this object.
// Call FillObservationEquation() and FillMeasurementCovariance() to get the
// partials, data values, and covariance, and use the state Namelist to insert
// them into the LS quantities, starting at row index.
// param pts   a map with key=label, value=Point of known Points
// param index index in Data, row in Partials and row,col in MCov at which to add
// return the number of rows contributed in the equation
int PointMeas::ContributeObsEquation(const map<string,Point>& pts, const int& index)
   throw(Exception)
{
   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation

   if(notUsed) return 0;                              // do nothing if this unused

   // fill the observation equation at index
   FillObservationEquation(pts, EQD.StateNames, EQD.Partials,
                           EQD.MeasData, EQD.NomData, index, false, EQD.is2D);
   // fill the measurement covariance at index
   FillMeasCovariance(EQD.MCov, index);

   return (EQD.is2D ? 2:3);
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
