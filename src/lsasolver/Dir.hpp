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
/// @file Dir.hpp  Include file for class Dir, direction, which is a biased azimuth
///                   angle, with sigma for LSA solver

#ifndef LSA_DIR_DATA_INCLUDE
#define LSA_DIR_DATA_INCLUDE

#include <map>
#include <string>
// geomatics
#include "Namelist.hpp"
// lsa
#include "MatrixVector.hpp"
#include "Azimuth.hpp"
#include "DATDir.hpp"
#include "DATDirSet.hpp"
#include "logstream.hpp"

/// Class Dir encapsulates a direction measurement, which is just a biased azimuth
/// angle measurement. It belongs to a DirSet, including the labels
/// "From" and "To" which define the sense and orientation of the angle,
/// the measurement itself and its uncrtainty sigma, plus a second sigma
/// value which may be additive or multiplicative (PPM), plus corrections.
/// NB the angle is defined as the angle from North to the line "From"->"To"
class Dir : public Azimuth {
public:
   /// replacement for DATAzimuth version
   static const std::string DocString;

   /// DirSet group name to which this belongs.
   std::string group;

   /// empty constructor
   Dir(void) { }

   /// Constructor from DATDir and DATDirSet
   Dir(DATDir& dat, DATDirSet& dirset)
   {
      // use dat and dirset to fill DATAzimuth info
      From = dirset.From;
      To = dat.To;
      value = dat.value;
      sigma = dat.sigma;
      addsig = dat.addsig;
      hasAddsig = dat.hasAddsig;
      Fcentersig = dirset.Fcentersig;
      hasFcsig = dirset.hasFcsig;
      Tcentersig = dat.Tcentersig;
      hasTcsig = dat.hasTcsig;
      HtFrom = dirset.HtFrom;
      HtFromSigma = dirset.HtFromSigma;
      hasHtFrom = dirset.hasHtFrom;
      HtTo = dat.HtTo;
      hasHtTo = dat.hasHtTo;
      isGeoid = dirset.isGeoid;
      // also include dattag and datscale
      dattag = dat.getTag();                 // this means tags on DIRSET are ignored.
      // either DIRSET or DIR or both (use the product) may be scaled.
      datscale = dirset.getScale() * dat.getScale();
      // must give Dir the group name
      group = dirset.DSGroup;
   }

   /// Constructor from DATAzimuth - overload and prohibit: do not implement
   Dir(DATAzimuth& dat);

   /// Return DATtype (get from DATrecord)
   virtual DATtype type(void) const { return DATtype::DIR; }

   /// Return tag (get from DATrecord)
   virtual std::string tag(void) const { return dattag; }

   /// Return scale (get from DATrecord)
   virtual double scale(void) const { return datscale; }

   /// Return true if angular, false if linear
   bool isAngleMeas(void) const { return true; }

   /// Allow clone
   Dir* clone(void) const { return new Dir(*this); }

   /// Return short name
   std::string name(void) const throw() { return std::string("Dir"); }

   /// Return long name = label() in DAT class (this is just From-To)
   virtual std::string dlabel(void) const throw() { return label(); }

   /// Return all labels (From, To) in a vector
   virtual std::vector<std::string> allLabels(void) const throw()
   { return Azimuth::allLabels(); }

   /// rename the DATrecord::asString()
   std::string asDataString(const int prec=3, const int width=8) const
      { return (name() + " (group=" + group + ") " + asString(prec,width)); }

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
   /// \verbatim
   /// Observation equation for azimuth; Ghilani Eqn 16.3 pg 299
   ///
   /// ^  N
   /// |  ^         /B (xb,yb) (To)
   /// |  |        /
   /// |  |       /                        N = local "north"
   /// |  |      /                         AzIB = azimuth NIB
   /// |  |     /
   /// |  |AzIB/
   /// |  |   /
   /// |  |  /
   /// |  | /
   /// |  |/
   /// |  I(xi,yi) (From)
   /// |
   /// 0------------------------> x
   ///
   /// equation is (NB sign/ordering convention is not consistent!)
   ///   [(yi-yb)/IB^2] dxi + [(xb-xi)/IB^2] dyi
   /// + [(yb-yi)/IB^2] dxb + [(xi-xb)/IB^2] dyb
   ///       = AzIB - AzIB(0)
   ///
   /// I = this->From, B = this->To
   ///
   /// return 4-vector equation of Partials, ordered as here, and data values.
   /// { xi yi xb yb } = { xFrom, yFrom, xTo, yTo }
   /// \endverbatim
   ///
   /// @param pts  map with key=label, value=Point which must contain "From" and "To"
   /// @param Partials Vector<double> the 4-vector partials for this object.
   /// @param MeasData double the measurment for this object.
   /// @param NomData  double the nominal datum for this object.
   /// @param doSOA    bool if true, express data in seconds-of-arc (else radians)
   /// @param is2D     bool if true, problem is truly 2-dimensional (all Z==0)
   /// @return the 4-vector Partials and 2 data.
   /// @throw if the "From" or "To" Points are not found in the input.
   void ObservationEquation(const std::map<std::string,Point>& pts,
                            gnsstk::Vector<double>& Partials,
                            double& MeasData, double& NomData,
                            const bool doSOA=false, bool is2D=false)
      throw(gnsstk::Exception)
   {
      Azimuth::ObservationEquation(pts, Partials, MeasData, NomData, doSOA, is2D);
   }

   void FillObservationEquation(const std::map<std::string,Point>& pts,
                                const gnsstk::Namelist& Names,
                                gnsstk::FlexMatrix<double>& Partials,
                                gnsstk::Vector<double>& MeasData,
                                gnsstk::Vector<double>& NomData,
                                const int rowindex,
                                const bool doSOA=false, const bool is2D=false)
      throw(gnsstk::Exception)
   {
      Azimuth::FillObservationEquation(pts, Names, Partials, MeasData, NomData,
                                          rowindex, doSOA, is2D);
   }

   /// Fill the given measurement covariance Matrix with covariance for this object.
   /// Insert covariance elements (~sigma^2) into the Matrix, at row rowindex.
   /// @param pts  map with key=label, value=Point which must contain "From" and "To"
   /// @param MCov Matrix<double> of measurement covariance
   /// @param index (row,col) index in MCov at which to add covariance(s)
   /// @param doSOA    bool if true, express data in seconds-of-arc (else radians)
   /// @throw if the "From" or "To" Points are not found in the input.
   /// @throw if sigma is zero
   void FillMeasCovariance(const std::map<std::string,Point>& pts,
            gnsstk::FlexMatrix<double>& MCov, const int index, const bool doSOA=false)
      throw(gnsstk::Exception)
   { Azimuth::FillMeasCovariance(pts, MCov, index, doSOA); }

   /// set notUsed=T if From and To are not found in the given list of labels
   /// For class Dir, the data is always used b/c it can help determine the bias.
   /// @return the new value of notUsed (always false for Dir)
   bool setUnused(const std::vector<std::string>& labels) throw()
   { return false; }

   bool isUnused(void) { return false; }

}; // end class Dir

#endif   // LSA_DIR_DATA_INCLUDE

