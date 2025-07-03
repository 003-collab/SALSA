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
/// @file MeasBase.hpp Pure virtual base class encapsulating data used as
///                    measurements in lsasolver.

#ifndef LSA_SOLVER_MEASUREMENT_BASE_INCLUDE
#define LSA_SOLVER_MEASUREMENT_BASE_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <string>
#include <vector>
// lsa
#include "MatrixVector.hpp"
#include "DATtype.hpp"
#include "Point.hpp"

/// Virtual base class encapsulating records used as measurement data in solver.
class MeasBase {
protected:
   bool notUsed;        ///< if true, this data is not used in LS equation

public:
   /// Constructor
   MeasBase(void) : notUsed(false) { }

   /// Destructor
   virtual ~MeasBase() { }

   /// Return DATtype (get from DAT<class>)
   virtual DATtype type(void) const = 0;

   /// Return tag (get from DAT<class>, special provision for PointMeas)
   virtual std::string tag(void) const = 0;

   /// Return scale (get from DAT<class>, special provision for PointMeas)
   virtual double scale(void) const = 0;

   /// Return true if angular, false if linear (all are measurements of one or other)
   virtual bool isAngleMeas(void) const = 0;

   /// Return short name
   virtual std::string name(void) const throw() = 0;

   /// Return long name = DAT<meas>::label()
   virtual std::string dlabel(void) const throw() = 0;

   /// Return all labels (From, At, To) in a vector
   virtual std::vector<std::string> allLabels(void) const throw() = 0;

   /// Force clone() to be defined for all derived classes - for ref_ptr
   virtual MeasBase* clone(void) const = 0;

   /// rename the DATrecord::asString()
   virtual std::string asDataString(const int prec=3, const int width=8) const = 0;

   /// Fill the Partials Matrix, Data Vectors and MCov Matrix for this data.
   /// These quantities are found in the ObsEqnData singleton.
   /// Call FillObservationEquation() and FillMeasurementCovariance() to get the
   /// partials, data values, and covariance, and use the state Namelist to insert
   /// them into the LS quantities, starting at row index.
   /// @param pts   a map with key=label, value=Point of known Points
   /// @param index index in Data, row in Partials and row,col in MCov at which to add
   virtual int ContributeObsEquation(const std::map<std::string,Point>& pts,
                                     const int& index) throw(gnsstk::Exception) = 0;

   /// Fill the given Partials Matrix and Data Vector for the data in this object.
   /// Call ObservationEquation() to get the partials (a vector) and data values,
   /// then use the given Namelist to insert them into the LS quantities,
   /// starting at row rowindex.
   /// param pts      a map with key=label, value=Point of known Points
   /// param Names    Namelist for the state vector
   /// param Partials Matrix<double> of partials
   /// param MeasData Vector<double> of measurements
   /// param NomData  Vector<double> of nominal data values
   /// param rowindex index (in Data, and row in Partials) at which to begin adding
   /// param doSOA    bool if true, express data in seconds-of-arc (else radians)
   /// param is2D     bool if true, problem is truly 2-dimensional (all Z==0)
   virtual void FillObservationEquation(const std::map<std::string,Point>& pts,
                                        const gnsstk::Namelist& Names,
                                        gnsstk::FlexMatrix<double>& Partials,
                                        gnsstk::Vector<double>& MeasData,
                                        gnsstk::Vector<double>& NomData,
                                        const int rowindex,
                                        const bool doSOA=false, const bool is2D=false)
   throw(gnsstk::Exception) = 0;

   // get the observation equations (partials, meas data, nominal data)
   /// set notUsed=T if From, At and To are not found in the given list of labels
   /// @return the new value of notUsed
   virtual bool setUnused(const std::vector<std::string>& labels) throw() = 0;

   /// Return true if this data is not used (setUnused() returned true)
   virtual bool isUnused(void) { return notUsed; }

   /// define names for the Data vector for this object, adding it(them) to the
   /// given list of names.  NB. Delta and PointMeas will overload this
   /// @param tag string to incorporate into the name
   /// @param Names vector<string> to which new names are added
   /// @return the number of names added
   virtual int addDataNames(const std::string& tag, std::vector<std::string>& Names)
      throw()
   {
      if(notUsed) return 0;
      Names.push_back(name() + "." + tag + "(" + dlabel() + ")");
      return 1;
   }

}; // end class MeasBase

#endif   // LSA_SOLVER_MEASUREMENT_BASE_INCLUDE
