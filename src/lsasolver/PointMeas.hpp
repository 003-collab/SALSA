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
/// @file PointMeas.hpp  Include file for class PointMeas, point pseudo-measurements
///        used when control points are to be adjusted (Point with ADJ, fixtype==3).
/// NB. Class Point cannot be made to inherit class MeasBase b/c MeasBase uses the
/// list of Points, GD.Points, and so MeasBase.hpp must include Point.hpp

#ifndef LSA_POINT_MEAS_INCLUDE
#define LSA_POINT_MEAS_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <string>
#include <map>
// geomatics
#include "Namelist.hpp"
// lsa
#include "MatrixVector.hpp"
#include "MeasBase.hpp"

/// class PointMeas encapsulates a psuedo measurement derived from a Point that is to
/// be adjusted (ADJ in the DATPoint). It includes the fixed coordinates and
/// measurement covariance matrix, and the same label as the Point; it contains a
/// DATPoint object (rather than inheriting class DATPoint).
/// The measurement is defined as the coordinate difference (X,Y,Z) of the Point with
/// the given label and the fixed coordinates.
/// (NB. Class Point cannot be made to inherit class MeasBase b/c MeasBase uses the
/// list of Points, GD.Points, and so MeasBase.hpp must include Point.hpp)
class PointMeas : public MeasBase {
public:

// member data
   std::string label;   ///< Point from which this pseudo-measurement is derived
   std::string dattag;  ///< Optional tag from DAT file
   double datscale;     ///< Optional scale from DAT file

   double x;            ///< X-coordinate
   double y;            ///< Y-coordinate
   double z;            ///< Z-coordinate
   // components of the UT portion of the covariances matrix.
   double covxx;        ///< X,X component of covariance matrix
   double covxy;        ///< X,Y component of covariance matrix
   double covxz;        ///< X,Z component of covariance matrix
   double covyy;        ///< Y,Y component of covariance matrix
   double covyz;        ///< Y,Z component of covariance matrix
   double covzz;        ///< Z,Z component of covariance matrix

// member functions
   /// Empty constructor
   /// empty constructor. label is set to "Undef"
   /// coordinates = 0,0,0 and the covariance matrix to 0.0
   PointMeas() : label(std::string("Undef")),x(0.0),y(0.0),z(0.0),
                 covxx(0.0),covxy(0.0),covxz(0.0),covyy(0.0),covyz(0.0),covzz(0.0),
                 dattag(std::string()), datscale(1.0)
      { }

   /// destructor
   virtual ~PointMeas() { }

   /// Constructor from Point
   PointMeas(Point& p) : label(p.label),x(p.x),y(p.y),z(p.z),
                         covxx(p.covxx),covxy(p.covxy),covxz(p.covxz),
                         covyy(p.covyy),covyz(p.covyz),covzz(p.covzz)
      { dattag = p.getTag();  datscale = p.getScale(); }

   /// Return DATtype
   virtual DATtype type(void) const { return DATtype::POS; }

   /// Return tag (get from DATrecord)
   virtual std::string tag(void) const { return dattag; }

   /// Return scale (get from DATrecord)
   virtual double scale(void) const { return datscale; }

   /// Return true if angular, false if linear
   bool isAngleMeas(void) const { return false; }

   /// Allow clone
   PointMeas* clone(void) const { return new PointMeas(*this); }

   /// Return short name
   std::string name(void) const throw() { return std::string("PsPos"); }

   /// Return long name = label() in DAT class
   std::string dlabel(void) const throw() { return label; }

   /// Return all labels (From, To) in a vector
   virtual std::vector<std::string> allLabels(void) const throw()
   {
      std::vector<std::string> labs;
      labs.push_back(label);
      return labs;
   }

   /// Write data as a simple string
   std::string asString(const int prec, const int width) const;

   /// Write data as a 3-line string
   std::string asString3(const std::string msg,
               const int prec, const int width) const;

   /// Write measurement as a string using asString3().
   std::string asDataString(const int prec=3, const int width=8) const
      { return asString3(name(),prec,width); }

   /// Fill the Partials Matrix and Data Vector for the data in this object.
   /// Call FillObservationEquation() and FillMeasurementCovariance() to get the
   /// partials, data values, and covariance, and use the state Namelist to insert
   /// them into the LS quantities, starting at row index.
   /// @param pts   a map with key=label, value=Point of known Points
   /// @param index index in Data, row in Partials and row,col in MCov at which to add
   int ContributeObsEquation(const std::map<std::string,Point>& pts, const int& index)
      throw(gnsstk::Exception);

   /// Compute the partial derivatives of this data w.r.t. the components of the
   /// Point object.
   /// Also compute the measured and nominal data vectors.
   /// NB measurement covariance is returned by a separate member routine.
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
   gnsstk::FlexMatrix<double> fullMeasurementCovariance(void) throw(gnsstk::Exception);

   /// Fill the given measurement covariance Matrix with covariance for this object.
   /// Insert covariance elements (~sigma^2) into the Matrix, at row rowindex.
   /// @param MCov Matrix<double> of measurement covariance
   /// @param index (row,col) index in MCov at which to add covariance(s)
   /// @throw if sigma is zero
   void FillMeasCovariance(gnsstk::FlexMatrix<double>& MCov, const int index)
      throw(gnsstk::Exception);

   /// set notUsed=T if From and To are not found in the given list of labels
   /// @return the new value of notUsed
   bool setUnused(const std::vector<std::string>& labels) throw();

   /// define names for the Data vector for this object, adding it(them) to the
   /// given DataNamelist. NB. this overloads the version in MeasBase
   /// @param Names vector<string> to which new names are added
   /// @param str string to incorporate into the name
   /// @return the number of names added
   int addDataNames(const std::string& str, std::vector<std::string>& Names) throw();

}; // end class PointMeas

#endif   // LSA_POINT_MEAS_INCLUDE
