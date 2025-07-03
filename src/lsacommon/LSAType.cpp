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
/// @file LSAtype.cpp  'Smart enum' class for record types in *.LSA files

#include "LSAType.hpp"

using namespace std;

// -----------------------------------------------------------------------------------
// Static initialization of const std::strings for asString().
// Must parallel enum Types in LSAtype.hpp.
const string LSAType::Strings[count] =
  {
    string("Unknown"),
    string("COMMENT"),
    string("INCLUDE"),
    string("CONFIG"),
    // position
    string("POSC"),                 ///< 3-D XYZ position
    string("POSG"),                 ///< 3-D ENU position
    // angular measurements
    string("AZIM"),                 ///< azimuth angle
    string("HANG"),                 ///< horizontal angle
    string("VANG"),                 ///< vertical angle
    string("ZANG"),                 ///< zenith angle
    // linear measurements
    string("DIST"),                 ///< distance
    string("DXYZ"),                 ///< 3-D delta (dx,dy,dz)
    string("HDIF"),                 ///< height difference
    // direction groups
    string("HDIR"),                 ///< horizontal direction
    string("DGRP"),                 ///< direction group
    // corrections
    string("UNCR"),
    string("HGHT"),                ///< height of instrument or target
    // scaling
    string("VSCA"),                ///< measurement scaling
    // post processed
    string("MEAN"),                ///< mean of positions
    string("ENUO"),                ///< delta enu from position
   };

// -----------------------------------------------------------------------------------
// Static initialization of const std::strings for asString().
// Must parallel enum Types in LSAtype.hpp.
const string LSAType::Descriptions[count] =
  {
    string("Unknown"),
    string("COMMENT - Comment"),
    string("INCLUDE - Additional .lsa file"),
    string("CONFIG - Configuration Option"),
    // position
    string("POSC - Station Position (Cartesian)"),    ///< 3-D XYZ position
    string("POSG - Station Position (Geodetic)"),     ///< 3-D ENU position
    // angular measurements
    string("AZIM - Azimuth Measurement"),             ///< azimuth angle
    string("HANG - Horizontal Angle Measurement"),    ///< horizontal angle
    string("VANG - Vertical Angle Measurement"),      ///< vertical angle
    string("ZANG - Zenith Angle Measurement"),        ///< zenith angle
    // linear measurements
    string("DIST - Distance measurement"),            ///< distance
    string("DXYZ - Delta XYZ measurement"),           ///< 3-D delta (dx,dy,dz)
    string("HDIF - Height Difference Measurement"),   ///< height difference
    // direction groups
    string("HDIR - Horizontal Direction Measurement"),///< horizontal direction
    string("DGRP - Direction Group"),                 ///< direction group
    // corrections
    string("UNCR - Uncertainty Model"),
    string("HGHT - Height of Instrument/Target"),     ///< height of instrument or target
    // scaling
    string("VSCA - Variance Scaling"),                ///< measurement scaling
    // post processed
    string("MEAN - Position Derived from Mean of Positions"), ///< mean of positions
    string("ENUO - Position Derived from ENU Offset"),        ///< enu offset
   };

// -----------------------------------------------------------------------------------
void LSAType::setLSAtype(const Types& sys)
   throw()
{
   if(sys < 0 || sys >= count)
      type = Unknown;
   else
      type = sys;
}

// -----------------------------------------------------------------------------------
void LSAType::fromString(const string str)
   throw()
{
   type = Unknown;
   for(int i=0; i<count; i++) {
      if(Strings[i] == str) {
         type = static_cast<Types>(i);
         break;
      }
   }
}

// -----------------------------------------------------------------------------------
ostream& operator<<(ostream os, const LSAType& t)
{
   return os << t.asString();
}

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
