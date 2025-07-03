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
/// @file Delta.hpp  Include file for class Delta, 3-D XYZ coordinate
///                  difference measurement for LSA solver

#ifndef LSA_DELTA_INCLUDE
#define LSA_DELTA_INCLUDE

#include <string>
#include <map>
// geomatics
#include "Namelist.hpp"
// lsa
#include "MatrixVector.hpp"
#include "DATDelta.hpp"
#include "MeasBase.hpp"

/// class Delta encapsulates a delta-XYZ measurement, including the coordinates and
/// measurement covariance matrix, and the labels "From" and "To". The measurement is
/// defined as the coordinate difference Point("To") minus Point("From").
class Delta : public DATDelta, public MeasBase {
public:
   /// Empty constructor
   Delta(void) { }

   /// Constructor from DATDelta
   Delta(DATDelta& dat) : DATDelta(dat) { }

   /// Constructor from two Points
   /// @param A Point to be the "From" position in the Delta
   /// @param B Point to be the "To" position in the Delta
   Delta(const Point& A, const Point& B) throw(gnsstk::Exception)
   {
      DATDelta d(A.label, B.label, B.x-A.x, B.y-A.y, B.z-A.z,
                                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
      *this = *(static_cast<Delta *>(&d));
   }

   /// Return DATtype (get from DATrecord)
   virtual DATtype type(void) const { return DATDelta::type; }

   /// Return tag (get from DATrecord)
   virtual std::string tag(void) const { return dattag; }

   /// Return scale (get from DATrecord)
   virtual double scale(void) const { return datscale; }

   /// Return true if angular, false if linear
   bool isAngleMeas(void) const { return false; }

   /// Allow clone
   Delta* clone(void) const { return new Delta(*this); }

   /// Return short name
   std::string name(void) const throw() { return std::string("Del"); }

   /// Return long name = label() in DAT class
   std::string dlabel(void) const throw() { return DATDelta::label(); }

   /// Return all labels (From, To) in a vector
   virtual std::vector<std::string> allLabels(void) const throw()
   {
      std::vector<std::string> labs;
      labs.push_back(From);
      labs.push_back(To);
      return labs;
   }

   /// rename the DATrecord::asString()  NB use asString3
   std::string asDataString(const int prec=3, const int width=8) const
      { return DATDelta::asString3(name(),prec,width); }

   /// Fill the Partials Matrix and Data Vector for the data in this object.
   /// Call FillObservationEquation() and FillMeasurementCovariance() to get the
   /// partials, data values, and covariance, and use the state Namelist to insert
   /// them into the LS quantities, starting at row index.
   /// @param pts   a map with key=label, value=Point of known Points
   /// @param index index in Data, row in Partials and row,col in MCov at which to add
   int ContributeObsEquation(const std::map<std::string,Point>& pts, const int& index)
      throw(gnsstk::Exception);

   /// Compute the partial derivatives of this data w.r.t. the components of the
   /// "From" and "To" Point objects. If the Point is fixed, the partial is zero.
   /// Also compute the measured and nominal data vectors.
   /// NB measurement covariance is returned by a separate member routine.
   ///
   /// \verbatim
   /// Observation equations for delta; Ghilani Sec 17.8, pg 346
   /// ^ y
   /// |         B (To)
   /// |        /
   /// |       /
   /// |      /
   /// |     /
   /// |    /
   /// |   /
   /// |  A (From)
   /// |
   /// 0---------------> x
   ///
   /// Bx = Ax + dxAB
   /// By = Ay + dyAB
   /// Bz = Az + dzAB
   /// Partial derivatives * State = Data(Measured-Nominal);   Meas.Cov (symmetric)
   /// [-1  1  0  0  0  0 ] [Ax]     [ dx ]                   [ covxx covxy covxz ]
   /// [ 0  0 -1  1  0  0 ] [Bx]  =  [ dy ]                   [   .   covyy covyz ]
   /// [ 0  0  0  0 -1  1 ] [Ay]     [ dz ]                   [   .     .   covzz ]
   ///                      [By]
   ///                      [Az]
   ///                      [Bz]
   ///
   /// return 3x6 matrix of partials, and measured and nominal data vectors
   ///          [-1  1  0  0  0  0]  [ dx ]  [Bx-Ax]
   ///          [ 0  0 -1  1  0  0]  [ dy ]  [By-Ay]
   ///          [ 0  0  0  0 -1  1]  [ dz ]  [Bz-Az]
   /// \endverbatim
   ///
   /// @param pts  map with key=label, value=Point which must contain "From" and "To"
   /// @param Partials Matrix<double> the 3x6 partials matrix for this object.
   /// @param MeasData Vector<double> the 3-vector of measurments for this object.
   /// @param NomData  Vector<double> the 3-vector of nominal data for this object.
   /// @throw  if the "From" or "To" Points are not found in the input.
   void ObservationEquation(const std::map<std::string,Point>& pts,
                            gnsstk::Matrix<double>& Partials,
                            gnsstk::Vector<double>& MeasData,
                            gnsstk::Vector<double>& NomData)
      throw(gnsstk::Exception);

   /// Fill the given Partials Matrix and Data Vectors for the data in this object.
   /// Call ObservationEquation() to get the partials and data vectors, then use
   /// the given Namelist to insert them into the LS quantities,
   /// starting at row rowindex.
   /// @param pts      a map with key=label, value=Point of known Points
   /// @param Names    Namelist for the state vector
   /// @param Partials Partials matrix of the full LS equation
   /// @param MeasData Measurement data vector
   /// @param NomData  Nominal values for the data vector
   /// @param rowindex index (in Data, and row in Partials) at which to begin adding
   /// @param doSOA    bool if true, express data in seconds-of-arc (else radians)
   /// @param is2D     bool if true, problem is truly 2-dimensional (all Z==0)
   void FillObservationEquation(const std::map<std::string,Point>& pts,
                                const gnsstk::Namelist& Names,
                                gnsstk::FlexMatrix<double>& Partials,
                                gnsstk::Vector<double>& MeasData,
                                gnsstk::Vector<double>& NomData,
                                const int rowindex,
                                const bool doSOA=false, const bool is2D=false)
      throw(gnsstk::Exception);

   /// Return the full measurement covariance matrix, that is the part defined
   /// by the UT components in the constructor plus all the additions due to
   /// additional sigma and centering errors.
   /// @return 3x3 measurement covariance matrix
   /// @throw if sigma is zero
   gnsstk::FlexMatrix<double> fullMeasurementCovariance(const std::map<std::string,Point>& pts) throw(gnsstk::Exception);

   /// Fill the given measurement covariance Matrix with covariance for this object.
   /// Insert covariance elements (~sigma^2) into the Matrix, at row rowindex.
   /// @param MCov Matrix<double> of measurement covariance
   /// @param index (row,col) index in MCov at which to add covariance(s)
   /// @throw if sigma is zero
   void FillMeasCovariance(const std::map<std::string,Point>& pts, gnsstk::FlexMatrix<double>& MCov, const int index)
      throw(gnsstk::Exception);

   /// get the slant range (distance) defined by this Delta
   /// @return the slant range defined by this object
   double distance(void) const throw(gnsstk::Exception);

   /// set notUsed=T if From and To are not found in the given list of labels
   /// @return the new value of notUsed
   bool setUnused(const std::vector<std::string>& labels) throw();

   /// define names for the Data vector for this object, adding it(them) to the
   /// given DataNamelist. NB. this overloads the version in MeasBase
   /// @param Names vector<string> to which new names are added
   /// @param tag string to incorporate into the name
   /// @return the number of names added
   int addDataNames(const std::string& tag, std::vector<std::string>& Names) throw();

}; // end class Delta

#endif   // LSA_DELTA_INCLUDE
