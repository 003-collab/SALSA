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
/// @file DATHeight.hpp  Include file for class DATHeight, data for the *.dat file
///                    HGT record, orthometric or geodetic height measurement

#ifndef LSA_DAT_HEIGHT_DATA_INCLUDE
#define LSA_DAT_HEIGHT_DATA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
// lsa
#include "MatrixVector.hpp"
#include "DATrecord.hpp"

/// Class DATHeight encapsulates the data from a height or HGT record
/// in the *.dat file, including the height, measurement sigma and additional sigma
/// (either additive or multiplicative - PPM) and refracation.
class DATHeight : public DATrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving DAT file specification for this record
   static const std::string DocString;

   // labels
   std::string From; ///< string label for the "From" Point
   std::string To;   ///< string label for the "To" Point

   // height and sigma
   double value;     ///< coordinate height (To - From) in meters
   double sigma;     ///< uncrtainty on the height in meters

   // additional sigma
   double addsig;    ///< additional sigma in linear units, unless isPPM, then PPM
   bool isPPM;       ///< if true, addsig has units parts-per-million (PPM)

   // corrections
   double refract;   ///< refraction
   bool isCurvature; ///< if true, apply curvature corrections
   bool isOHC; ///< if true, apply orthometric height correction
   bool isGeoid;     ///< if true, apply geoid corrections

   // flags
   bool hasAddsig;   ///< true if addsig has been set
   bool hasRefract;  ///< true if refract has been set

   // member functions -------------------------------------------------

   /// empty constructor
   DATHeight(void) { }

   /// destructor
   virtual ~DATHeight() { }

   /// constructor, given the From and To labels, the height and sigma
   /// @param f string label of the "From" point
   /// @param t string label of the "To" point
   /// @param ht double height measurement in meters
   /// @param sig sigma in meters
   DATHeight(std::string f, std::string t, double ht, double sig)
      : From(f), To(t), value(ht), sigma(sig),
        addsig(0.0), hasAddsig(false),
        refract(0.0), isCurvature(false), isOHC(false), isGeoid(false), hasRefract(false)
      { type = DATtype::HGT; }

   /// Parse a string from a single line in the DAT file.
   /// @param line single line read from DAT file
   /// @return true if successful
   virtual bool fromString(const std::string& line);

   /// Output as a string (one line) for the DAT file.
   /// @return string, a single line for a DAT file
   virtual std::string asDATString(void) const;

   /// write the object as a 1-line string
   /// @param prec integer number of digits precision (default 3)
   /// @param width integer width (default 8)
   /// @return string version of the object
   virtual std::string asString(const int prec=3, const int width=9) const;

   /// Consistent interface for all DAT records - not a config record
   virtual bool isConfig(void) const { return false; }

   /// Consistent interface for all DAT records - not a position record
   virtual bool isPosition(void) const { return false; }

   /// Consistent interface for all DAT records - is a measurement record
   virtual bool isMeasurement(void) const { return true; }

   /// Consistent interface for all DAT records - is not an angle measurement
   virtual bool isAngle(void) const { return false; }

   /// Consistent interface for all DAT records - is a linear measurement
   virtual bool isLength(void) const { return true; }

   // this ends the base class interface

   /// @return a simple label = 'From-To'
   std::string label(void) const { return (From+"-"+To); }

   /// Define the "additional sigma", include the isPPM flag.
   /// @param asig double uncrtainty of measurment in meters, unless isppm==T
   /// @param isppm if true, addsig has units part-per-million (PPM)
   void setAddSigma(double asig, bool isppm=false) throw()
   {
      addsig = asig;
      isPPM = isppm;
      if(::fabs(addsig) > 1.e-15) hasAddsig = true;
   }

   /// Define the correction refraction
   /// @param ref refraction
   /// @param iscurve if true apply curvature corrections
   /// @param isohc if true apply orthometric height correction
   /// @param isgeoid if true apply geoid corrections
   void setCorrections(double ref, bool iscurve, bool isohc, bool isgeoid) throw()
   {
      refract = ref;
      if(::fabs(refract) > 1.e-15) hasRefract = true;
      isCurvature = iscurve;
      isOHC = isohc;
      isGeoid = isgeoid;
   }

}; // end class DATHeight

#endif   // LSA_DAT_HEIGHT_DATA_INCLUDE

