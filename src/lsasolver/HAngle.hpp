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
/// @file HAngle.hpp  Include file for class HAngle, horizontal angle for LSA solver

#ifndef LSA_DAT_HANGLE_INCLUDE
#define LSA_DAT_HANGLE_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <string>
#include <vector>
#include <map>
// geomatics
#include "Namelist.hpp"
// lsa
#include "MatrixVector.hpp"
#include "DATHAngle.hpp"
#include "MeasBase.hpp"

/// Class HAngle encapsulates horizontal angle measurement, including the labels
/// "From" "At" and "To" which define the sense and orientation of the angle,
/// the measurement itself and its uncrtainty sigma, plus a second sigma
/// value which may be additive or multiplicative (PPM), and corrections.
/// NB the angle is defined at the "At" Point, measured from "From" to "To".
class HAngle : public DATHAngle, public MeasBase {
public:
// member data

   /// Tag of DirSet from which this was constructed, empty if not from a DirSet
   std::string DSGroup;

   /// Index of this object in the DirSet, IF it came from a DirSet, else -1
   int dsindex;

   /// Row(dsindex) of the full measurement covariance of this DirSet.
   /// This is used by FillMeasCovariance() to define (one row of) the
   /// full DirSet measurement covariance.
   gnsstk::Vector<double> MCovRow;

// member functions
   /// Empty constructor
   HAngle(void) : DATHAngle(), dsindex(-1) { }

   /// Constructor from DATHAngle
   HAngle(DATHAngle& dat) : DATHAngle(dat), dsindex(-1) { }

   /// constructor given labels and angle in radians - matching DATHAngle c'tor
   /// @param f string label of the "From" Point
   /// @param a string label of the "At" Point
   /// @param t string label of the "To" Point
   /// @param angle double value of the angle in radians
   /// @param sig double uncrtainty of the measurment in radians
   HAngle(std::string f, std::string a, std::string t, double angle, double sig)
      : DATHAngle(f,a,t,angle,sig), dsindex(-1)
      { }

   /// Return DATtype (get from DATrecord)
   virtual DATtype type(void) const { return DATHAngle::type; }

   /// Return tag (get from DATrecord)
   virtual std::string tag(void) const { return dattag; }

   /// Return scale (get from DATrecord)
   virtual double scale(void) const { return datscale; }

   /// Return true if angular, false if linear
   bool isAngleMeas(void) const { return true; }

   /// Allow clone
   HAngle* clone(void) const { return new HAngle(*this); }

   /// Return short name
   std::string name(void) const throw() { return std::string("Han"); }

   /// Return long name = label() in DAT class
   std::string dlabel(void) const throw() { return DATHAngle::label(); }

   /// Return all labels (From, At, To) in a vector
   virtual std::vector<std::string> allLabels(void) const throw()
   {
      std::vector<std::string> labs;
      labs.push_back(From);
      labs.push_back(At);
      labs.push_back(To);
      return labs;
   }

   /// write as a string
   /// @param prec integer number of digits precision (default 3)
   /// @param width integer width (default 8)
   std::string asDataString(const int prec=3, const int width=8) const;

   /// Fill the Partials Matrix and Data Vector for the data in this object.
   /// Call FillObservationEquation() and FillMeasurementCovariance() to get the
   /// partials, data values, and covariance, and use the state Namelist to insert
   /// them into the LS quantities, starting at row index.
   /// @param pts   a map with key=label, value=Point of known Points
   /// @param index index in Data, row in Partials and row,col in MCov at which to add
   int ContributeObsEquation(const std::map<std::string,Point>& pts, const int& index)
      throw(gnsstk::Exception);

   /// Compute the partial derivatives of this data with respect to components of the
   /// "From" and "To" Point objects. If the Point is fixed, the partial is zero.
   /// Also compute the measured and nominal data values.
   /// NB measurement covariance is returned by a separate member routine.
   ///
   /// \verbatim
   /// Observation equation for triangle in the plane of the angle;
   /// Ghilani Eqn 15.13 pg 270
   ///
   /// ^  N
   /// |  ^         /F (xf,yf)
   /// |  |        /
   /// |  |       /                        N = local "north"
   /// |  |      /                         AzAT = azimuth NAT (not shown)
   /// |  |     /                          AzAF = azimuth NAF (not shown)
   /// |  |AzAF/        __/ T(xt,yt)       AFAT = measurement angle at A from F to T
   /// |  |   / AFAT __/
   /// |  |  /    __/
   /// |  | /  __/
   /// |  |/__/
   /// |  A(xa,ya)
   /// |
   /// 0------------------------> x
   ///
   /// equation is (NB sign/ordering convention is not consistent!)
   ///                  [(ya-yf)/AF^2] dxf +                [(xf-xa)/AF^2] dyf
   /// + [(yf-ya)/AF^2 - (yt-ya)/AT^2] dxa + [(xa-xf)/AF^2 - (xa-xt)/AT^2] dya
   ///                + [(yt-ya)/AT^2] dxt +                [(xa-xt)/AT^2] dyt
   ///    = AFAT - AFAT(0)
   /// where A = this->At, F = this->From and T = this->To
   ///
   /// Return 6-vector equation of Partials, ordered as shown below, and data values.
   /// First construct the 6-vector in the local horizontal plane:
   ///     Vh = { xf yf xa ya xt yt }
   ///        = { xFrom, yFrom, xAt, yAt, xTo, yTo },
   /// and add zeros for the z components to make it a 9-vector
   ///     Vh' = { xFrom, yFrom, 0, xAt, yAt, 0 xTo, yTo, 0 }.
   ///
   /// Then rotate this, using the rotation matrix XYZ->NEU at "At", into the
   /// XYZ frame, yielding the result: the 9-vector in the XYZ coordinate frame
   /// V3 = { Xf Yf Zf Xa Ya Za Xt Yt Zt }
   ///    = { XFrom, YFrom, ZFrom, XAt, YAt, ZAt, XTo, YTo, Zto }
   ///    = transpose(Rot)*Vh';
   /// \endverbatim
   ///
   /// @param pts  map key=label, value=Point which must contain "From" "At" and "To"
   /// @param Partials Vector<double> the 9-vector partials for this object.
   /// @param MeasData double the measurment for this object.
   /// @param NomData  double the nominal datum for this object.
   /// @param doSOA    bool if true, express data in seconds-of-arc (else radians)
   /// @param is2D     bool if true, problem is truly 2-dimensional (all Z==0)
   /// @throw if the "From" "At" or "To" Points are not found in the input.
   void ObservationEquation(const std::map<std::string,Point>& pts,
                            gnsstk::Vector<double>& Partials,
                            double& MeasData, double& NomData,
                            bool doSOA=false, bool is2D=false)
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
   /// @param veccov on output, Vector<double>(3) row of covariance matrix (DirSet)
   /// @return measurement variance in radians or SOA
   double fullMeasurementVariance(const std::map<std::string,Point>& pts, bool doSOA,
                                    gnsstk::Vector<double>& veccov)
      throw(gnsstk::Exception);

   /// Fill the given measurement covariance Matrix with covariance for this object.
   /// Insert covariance elements (~sigma^2) into the Matrix, at row rowindex.
   /// @param pts   map with key=label, value=Point which must contain "From" and "To"
   /// @param MCov Matrix<double> of measurement covariance
   /// @param index (row,col) index in MCov at which to add covariance(s)
   /// @param doSOA    bool if true, express data in seconds-of-arc (else radians)
   /// @throw if sigma is zero
   void FillMeasCovariance(const std::map<std::string,Point>& pts,
                           gnsstk::FlexMatrix<double>& MCov, const int index,
                           const bool doSOA=false)
      throw(gnsstk::Exception);

   /// set notUsed=T if From and To are not found in the given list of labels
   /// @return the new value of notUsed
   bool setUnused(const std::vector<std::string>& labels) throw();

}; // end class HAngle

#endif   // LSA_DAT_HANGLE_INCLUDE
