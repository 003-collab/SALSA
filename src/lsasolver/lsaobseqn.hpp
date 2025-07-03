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
/// @file lsaobseqn.hpp  Global include file for observation equation in
///                      ARL:UT LSA least squares solver.

#ifndef LSA_SOLVER_OBS_EQN_INCLUDE
#define LSA_SOLVER_OBS_EQN_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

// system includes
#include <ostream>
#include <string>
#include <vector>
#include <map>
// gnsstk
#include "singleton.hpp"
// geomatics
#include "Namelist.hpp"
// lsa
#include "MatrixVector.hpp"

/// Class ObsEqnData encapsulates data for observation equation in lsasolver
class ObsEqnData : public gnsstk::Singleton<ObsEqnData> {
public:
   /// Default and only constructor, sets defaults.
   ObsEqnData() throw() { SetDefaults(); }

   bool is2D;                                           ///< true (default false) 2-D XY problem
   bool doSOA;                                          ///< if T, write angle equations in SOA
   gnsstk::Namelist StateNames;                          ///< Namelist of state vector  N
   gnsstk::Namelist DataNames;                           ///< Namelist of Data vector  M
   gnsstk::FlexMatrix<double> Partials;                  ///< Partials matrix  MxN
   gnsstk::Vector<double> MeasData;                      ///< Measurement data vector M
   gnsstk::Vector<double> NomData;                       ///< Nominal data vector M
   gnsstk::Vector<double> Data;                          ///< Data vector = MeasData - NomData M
   gnsstk::FlexMatrix<double> MCov;                      ///< Measurement covariance matrix MxM
   gnsstk::FlexMatrix<double> White;                     ///< inverse Cholesky of MCov MxM
   gnsstk::Vector<double> MinDetectableBias;             ///< minimum detectable bias array
   gnsstk::Vector<double> BiasArray;                     ///< bias array of zeros except for one value used in ext rel calculations
   gnsstk::Matrix<double> ExternalReliabilityVectors;    ///< External Reliability vectors for reliability metrics
   gnsstk::Vector<double> ExternalReliabilityMags;       ///< Magnitudes of the external reliability vectors
   gnsstk::Matrix<double> ExternalReliabilityVectorsENU; ///< External Reliability vectors in ENU frame
   gnsstk::Matrix<double> ExternalReliabilityVectorsRot; ///< External Reliability vectors rotated so NA is X-axis
   unsigned int NMeas;                                  ///< Number of Measurement excluding cov specification
   gnsstk::Vector<bool> RedundancyZero;                  ///< Array of flags that determine if redundancy for that meas equals zero


   /// DirSet bias estimates, key=DirSet group name, value = apriori/estimated bias
   std::map<std::string, double> DSbiases;

   // constraints
   std::vector<std::string> Pts;       ///< Constraint: Point.label, parallel to axes
   std::vector<char> axes;             ///< Constraint: char N,E,U,X,Y,Z
   unsigned int Ncon;                  ///< Number of constraints
   gnsstk::Matrix<double> F;            ///< Constraints partials
   // don't need these b/c h is always zero
   //gnsstk::Vector<double> h;         ///< Constraints data
   //gnsstk::Matrix<double> Up,Vp,Wp;  ///< Components of the SVD of constraint eqn
   gnsstk::FlexMatrix<double> V0;       ///< Projection into null space of constraints

private:
   /// Set default values in the configuration, called by constructor
   void SetDefaults(void) throw()
   {
      is2D = false;
      doSOA = false;
      Ncon = 0;

   }

}; // end class ObsEqnData

//------------------------------------------------------------------------------------
#endif   // LSA_SOLVER_OBS_EQN_INCLUDE
