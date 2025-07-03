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
/// @file Dir.cpp  Class Dir, azimuth angle with sigma for LSA solver

#include <string>

#include "stl_helpers.hpp"
#include "lsaobseqn.hpp"         // for use by ContributeObsEquation
#include "logstream.hpp"

#include "Dir.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
// NB. DO NOT DO THIS - causes strange segfault in static initialization
//const string Dir::DocString = DATDir::DocString;
const string Dir::DocString = string("DIR group labTo angle [min sec DMS|AUNIT] sig [asig] AUNIT [tc LUNIT] [CORR htt [LUNIT]]");

//------------------------------------------------------------------------------------
// Fill the Partials Matrix and Data Vector for the data in this object.
// Call FillObservationEquation() and FillMeasurementCovariance() to get the
// partials, data values, and covariance, and use the state Namelist to insert
// them into the LS quantities, starting at row index.
// param pts   a map with key=label, value=Point of known Points
// param index index in Data, row in Partials and row,col in MCov at which to add
// return the number of rows contributed in the equation
int Dir::ContributeObsEquation(const map<string,Point>& pts, const int& index)
   throw(Exception)
{
   ObsEqnData& EQD=ObsEqnData::Instance();            // access observation equation

   if(notUsed) return 0;                              // do nothing if this unused

   // fill the observation equation at index
   Azimuth::FillObservationEquation(pts, EQD.StateNames, EQD.Partials,
                           EQD.MeasData, EQD.NomData, index, EQD.doSOA, EQD.is2D);
   // fill the measurement covariance at index
   Azimuth::FillMeasCovariance(pts, EQD.MCov, index, EQD.doSOA);

   // now add the bias term to the partials
   string label("bias");
   label += group;
   int k = EQD.StateNames.index(label);
   if(k < 0) {
      LOG(ERROR) << " Error - lost DirSet bias state: " << label;
      GNSSTK_THROW(Exception("Invalid bias state"));
   }
   EQD.Partials(index,k) = 1.0;

   // remove the bias from the data
   // Azm = Dir - bias
   // Azm + bias = Dir        this is the equation
   // Meas = Dir.value   Nom = Azm(Points)+bias   Data = M-N = Dir.value-Azm(Pts)-bias
   double bias(EQD.DSbiases[label]), nomvalue;
   if(EQD.doSOA) bias *= ::RAD_TO_SOA;
   nomvalue = EQD.NomData(index) + bias;

   // NB this has been done in Azimuth::FillObs() but must be re-done here b/c of bias
   // correct for 2pi - must correct Nominal here (data = measurement minus nominal)
   double value(EQD.MeasData(index));
   while(value - nomvalue > Pi) nomvalue += TwoPi;
   while(value - nomvalue < -Pi) nomvalue -= TwoPi;

   EQD.NomData(index) = nomvalue;

   return 1;         // return 1 row defined
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------

