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
/// @file VAngle.hpp  Include file for class VAngle, vertical angle for LSA solver

#ifndef LSA_VERTICAL_ANGLE_INCLUDE
#define LSA_VERTICAL_ANGLE_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <string>
#include <map>
// geomatics
#include "Namelist.hpp"
// lsa
#include "MatrixVector.hpp"
#include "DATVAngle.hpp"
#include "MeasBase.hpp"

/// Class VAngle encapsulates a vertical angle measurement, including the labels
/// "From" and "To" which define the sense and orientation of the angle,
/// the measurement itself and its uncrtainty sigma, plus a second sigma
/// value which may be additive or multiplicative (PPM), plus corrections
class VAngle : public DATVAngle, public MeasBase {
public:
   /// Empty constructor
   VAngle(void) { }

   /// Constructor from DATVAngle
   VAngle(DATVAngle& dat) : DATVAngle(dat) { }

   /// Return DATtype (get from DATrecord)
   virtual DATtype type(void) const { return DATVAngle::type; }

   /// Return tag (get from DATrecord)
   virtual std::string tag(void) const { return dattag; }

   /// Return scale (get from DATrecord)
   virtual double scale(void) const { return datscale; }

   /// Return true if angular, false if linear
   bool isAngleMeas(void) const { return true; }

   /// Allow clone
   VAngle* clone(void) const { return new VAngle(*this); }

   /// Return short name
   std::string name(void) const throw() { return std::string("Van"); }

   /// Return long name = label() in DAT class
   std::string dlabel(void) const throw() { return DATVAngle::label(); }

   /// Return all labels (From, To) in a vector
   virtual std::vector<std::string> allLabels(void) const throw()
   {
      std::vector<std::string> labs;
      labs.push_back(From);
      labs.push_back(To);
      return labs;
   }

   /// rename the DATrecord::asString()
   std::string asDataString(const int prec=3, const int width=8) const
      { return (name() + " " + DATVAngle::asString(prec,width)); }

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
   /// Also compute the measured and nominal data values.
   /// NB measurement covariance is returned by a separate member routine.
   ///
// TD replace this
   /// \verbatim
   /// Observation equation for triangle; Ghilani Eqn 15.13 pg 270
   ///
   /// ^  N
   /// |  ^         /B (xb,yb)
   /// |  |        /
   /// |  |       /                        N = local "north"
   /// |  |      /                         AzIB = azimuth NIB
   /// |  |     /                          AzIF = azimuth NIF
   /// |  |AzIB/        __/ F(xf,yf)       ABIF = measurement angle at I from B to F
   /// |  |   / ABIF __/
   /// |  |  /    __/
   /// |  | /  __/
   /// |  |/__/
   /// |  I(xi,yi)
   /// |
   /// 0------------------------> x
   ///
   /// equation is (NB sign/ordering convention is not consistent!)
   ///   [(yi-yb)/IB^2] dxb + [(xb-xi)/IB^2] dyb
   /// + [(yb-yi)/IB^2 - (yf-yi)/IF^2] dxi + [(xi-xb)/IB^2 - (xi-xf)/IF^2] dyi
   /// + [(yf-yi)/IF^2] dxf + [(xi-xf)/IF^2] dyf
   ///       = ABIF - ABIF(0)
   ///
   /// where I = this->At, B = this->From and F = this->To
   ///
   /// return 6-vector equation of Partials, ordered as here, and data values.
   /// { xb yb xi yi xf yf } = { xFrom, yFrom, xAt, yAt, xTo, yTo }
   /// \endverbatim
// end TD
   /// @param pts   map with key=label, value=Point which must contain "From" and "To"
   /// @param Partials Vector<double> the 6-vector partials for this object.
   /// @param MeasData double the measurment for this object.
   /// @param NomData  double the nominal datum for this object.
   /// @param doSOA    bool if true, express data in seconds-of-arc (else radians)
   /// @throw if the "From" or "To" Points are not found in the input.
   void ObservationEquation(const std::map<std::string,Point>& pts,
                            gnsstk::Vector<double>& Partials,
                            double& MeasData, double& NomData, const bool doSOA=false)
      throw(gnsstk::Exception);

   /// Fill the given Partials Matrix and Data Vector for the data in this object.
   /// Call ObservationEquation() to get the partials (a vector) and data values,
   /// then use the given Namelist to insert them into the LS quantities,
   /// starting at row rowindex.
   /// @param pts      a map with key=label, value=Point of known Points
   /// @param Names    Namelist for the state vector
   /// @param Partials Matrix<double> of partials
   /// @param MeasData Vector<double> of measurements
   /// @param NomData  Vector<double> of nominal data values
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

   /// Return the full measurement variance (sigma squared), which includes all
   /// contributions such as additional sigma and centering errors.
   /// @param pts  map with key=label, value=Point which must contain "From" and "To"
   /// @param doSOA    bool if true, express in seconds-of-arc (else radians)
   /// @return measurement variance in meters
   double fullMeasurementVariance(const std::map<std::string,Point>& pts, bool doSOA)
      throw(gnsstk::Exception);

   /// Fill the given measurement covariance Matrix with covariance for this object.
   /// Insert covariance elements (~sigma^2) into the Matrix, at row rowindex.
   /// @param pts   map with key=label, value=Point which must contain "From" and "To"
   /// @param MCov Matrix<double> of measurement covariance
   /// @param index (row,col) index in MCov at which to add covariance(s)
   /// @param doSOA  bool if true, express data in seconds-of-arc (else radians)
   /// @throw if sigma is zero
   void FillMeasCovariance(const std::map<std::string,Point>& pts,
                           gnsstk::FlexMatrix<double>& MCov, const int index,
                           const bool doSOA=false)
      throw(gnsstk::Exception);

   /// set notUsed=T if From and To are not found in the given list of labels
   /// @return the new value of notUsed
   bool setUnused(const std::vector<std::string>& labels) throw();

}; // end class VAngle

#endif   // LSA_VERTICAL_ANGLE_INCLUDE
