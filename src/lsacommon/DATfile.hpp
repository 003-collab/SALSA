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
/// @file DATfile.hpp  Include file for *.dat file: read and parse, write, and fill
///                    DAT* objects.

#ifndef LSA_DAT_FILE_PARSER_WRITER_INCLUDE
#define LSA_DAT_FILE_PARSER_WRITER_INCLUDE

#include <string>
#include <vector>
#include "Exception.hpp"

#include "DATAzimuth.hpp"
#include "DATConfig.hpp"
#include "DATDelta.hpp"
#include "DATDir.hpp"
#include "DATDirSet.hpp"
#include "DATDist.hpp"
#include "DATHAngle.hpp"
#include "DATHeight.hpp"
#include "DATPoint.hpp"
#include "DATVAngle.hpp"
#include "DATZAngle.hpp"

/**
\verbatim
 *** DAT file format ***

# DAT files are ASCII and whitespace-delimited with one record per line
# Labels (From, At, To) denote positions (Points or POS records)
# Each POS record (Point) must have a unique label;
#    but not all labels need have a POS record.
# keywords are in ALL CAPS
# POS ... CONS c[c'] constrain the position solution to a plane[line]
#    using c,c' = one of{XYZ} OR {NEU} and c != c'
# Note that covariance is required for POS with ADJ
# FIX : Point is constant (not adjusted);
# ADJ : use position as a priori information and adjust control point;
# EST or blank: use position only as a priori.
# LUNIT means linear unit MM|M|CM|KM|FT
# AUNIT means angular unit RAD|DEG|SOA but not DMS
# DMS denotes 'deg/min/sec' and follows an angle specified
#    by deg(int) min(int) sec(float)
# NB in the angle records "angle sig AUNIT" means angle and sigma both have unit AUNIT
#    (consider "angle AUNIT sig AUNIT")
# fcsig acsig and tcsig are centering errors on From, At and To stations in LUNITs
# CORR is optional but must be followed by corrections in the order shown;
#    use zero placeholders.
# ht[f|t] is height at From|To; target height difference is implemented using From+To
# refract is a float refraction, and CURV and GEOID are keywords meaning
#    apply curvature and (DOV, undulation) corrections, respectively.
# Title is for output only
# comment lines begin with '#' and are ignored; also #-to-EOL is ignored
#
# Tags: each measurement record (all measurements+POS ADJ -- NOT DIRSET) can be given
#   an optional tag with the field tag=TAG at the end of the DAT line, 
#   where TAG is the an arbitrary string (no whitespace).
#   This tag will be used in generating "data names" in the solver, rather than
#   simply numbering them; the number will be used when a tag is not found.
#   The user MUST use unique tags; failure to do so will cause the solver to throw
#   an exception.
#    
# Scaling: each record can be given an optional scale (to be applied to the
#   measurement covariance) using the field scale=S (no whitespace) at the end of
#   the DAT line, where S is a double to multiply the final measurement covariance.
#   NB. tag on a DIRSET record is ignored.
#   NB. scale on DIRSET applies to all DIRs in it; and individual DIR may be scaled;
#          scale(Dir) = scale(DATDIRSET) * scale(DATDIR) (default scale = 1).
# NB tag=TAG and scale=S must be at the end of the line but may be in any order.

# the Records (each a single line):

# Configuration: output precision; problem dimension (2D is XY); convergence criteria
TITLE title # quotes optional
PREC  eps UNIT [eps UNIT] # output precision; linear and/or angular (meas or pos) unit
   e.g.  PREC 0.001 M 0.0001 SOA|RAD  1.e-9 LLHSOA|LLHRAD (deduce RAD <=> SOA)
DIM 2|3  # dimension of the problem; 2D is XY only, i.e. Z's ignored
CONV [n ITER] [d CONV] # convergence criteria; either or both
CONFIDENCE alpha       # confidence for Chi squ test (0 < alpha < 1)
OUT [NOAPV] [APQUIT]   # output NOAPV = do not scale covariance with APV
                       # output APQUIT = quit after ComputeAPriori()
EXTRELVECT [YES] [NO]  # do/do not not compute external reliability
GEOIDFILE filename     # Filename for the gridded EGM08 geoid file
INTERPOLATION method   # Interpolation method for calculating geoid values
                       #   bicubic or bilinear
HASH string            # GUI may want to pass hash through solver to binary file
COMMENT <any string>   # comment that is echoed in output file

# geoid file
GEOIDFILE <file>

# position (Point)
# no corrections; constraints c one[two] of XYZ|NEU
POS label X Y Z [covxx xy xz yy yz zz] LUNIT [FIX|ADJ|EST] [CONS c[c]]
# undocumented
POS label D M S N|S D M S E|W Ht LUNIT [covxx xy xz yy yz zz [NEU] LUNIT] [FIX|ADJ|EST] [CONS c[c]]

POS label X Y Z covxx xy xz yy yz zz LUNIT FIX|ADJ|EST CONS c[c]                               15
POS label X Y Z covxx xy xz yy yz zz LUNIT FIX|ADJ|EST                                         13
POS label X Y Z covxx xy xz yy yz zz LUNIT             CONS c[c]                               14
POS label X Y Z LUNIT                      FIX|EST     CONS c[c]                                9
POS label X Y Z LUNIT                      FIX|EST                                              7
POS label X Y Z LUNIT                                  CONS c[c]                                8
POS label D M S N|S D M S E|W Ht LUNIT covxx xy xz yy yz zz NEU LUNIT FIX|ADJ|EST CONS c[c]    23
POS label D M S N|S D M S E|W Ht LUNIT covxx xy xz yy yz zz NEU LUNIT FIX|ADJ|EST              21
POS label D M S N|S D M S E|W Ht LUNIT covxx xy xz yy yz zz NEU LUNIT             CONS c[c]    22
POS label D M S N|S D M S E|W Ht LUNIT covxx xy xz yy yz zz     LUNIT FIX|ADJ|EST CONS c[c]    22
POS label D M S N|S D M S E|W Ht LUNIT covxx xy xz yy yz zz     LUNIT FIX|ADJ|EST              20
POS label D M S N|S D M S E|W Ht LUNIT covxx xy xz yy yz zz     LUNIT             CONS c[c]    21
POS label D M S N|S D M S E|W Ht LUNIT                                FIX|EST     CONS c[c]    15
POS label D M S N|S D M S E|W Ht LUNIT                                FIX|EST                  13
POS label D M S N|S D M S E|W Ht LUNIT                                            CONS c[c]    14

pull off CONS
pull off FIX|ADJ|EST - if last is LUNIT, EST
use N|S and E|W to separate xyz from neu
get covariance with unit
get position
POS label X Y Z covxx xy xz yy yz zz LUNIT                             12
POS label X Y Z LUNIT                                                   6
POS label D M S N|S D M S E|W Ht LUNIT covxx xy xz yy yz zz NEU LUNIT  20
POS label D M S N|S D M S E|W Ht LUNIT covxx xy xz yy yz zz     LUNIT  19
POS label D M S N|S D M S E|W Ht LUNIT                                 12

# 3-D delta XYZ (Delta)
DEL labFr labTo dX dY dZ covxx xy xz yy yz zz [asig [PPM]] [fcsig tcsig] LUNIT [CORR htf htt htfs htts [LUNIT]]

# Distance (Dist)
DIS labFr labTo distance sig [asig [PPM]] [fcsig tcsig] LUNIT [CORR htf htt htfs htts [LUNIT] [refract]]

# height (Height) GEOID if orthometric and undulation correction must be applied
HGT labFr labTo height sig [asig [PPM]] [fcsig tcsig] LUNIT [CORR [refract] [CURV] [OHC] [GEOID]]

# Azimuth (Azimuth)
AZM labFr labTo angle [min sec DMS|AUNIT] sig [asig] AUNIT [fcsig tcsig LUNIT] [CORR htf htt htfs htts [LUNIT] [GEOID]]

# Horizontal angle (HAngle)
HAN labFr labAt labTo angle [min sec DMS|AUNIT] sig [asig] AUNIT [fc ac tc LUNIT] [CORR htf htt htfs htts [LUNIT] [GEOID]]

# Direction set and Directions. Directions are biased azimuths, and one DIRSET yields
# a set of directions with a common bias.
# Set = { One DIRSET + >1 DIR with one "group" string};
#     group for each set must be unique and identical throughout set
# Zero or one DIR in a set -> set is ignored - no data can be constructed
# DIRs where group does not appear in a DIRSET are ignored
DIRSET group labFrom [asig AUNIT] [fc LUNIT] [CORR [htf [LUNIT]] [GEOID]]
DIR group labTo angle [min sec DMS|AUNIT] sig [asig] AUNIT [tcsig LUNIT] [CORR htt htts [LUNIT]]

# Vertical angle (VAngle)
VAN labFr labTo angle [min sec DMS|AUNIT] sig [asig] AUNIT [fc tc LUNIT] [CORR htf htt htfs htts [LUNIT] [refract] [GEOID]]

# Zenith angle (ZAngle)
ZAN labFr labTo angle [min sec DMS|AUNIT] sig [asig] AUNIT [ac tc LUNIT] [CORR htf htt htfs htts [LUNIT] [refract] [GEOID]]


 *** Table of members ***
      centering    --sigs--   -------corrections---------  notes
      FC  AC  TC   sig  PPM   HI/HT DOV Undul Refrac Curv

POS    -   -   -    y    -      -    -    -     -     -    also constraints

DEL    y   -   y    y    y      y    -    -     -     -    GNSS relative position

DIS    y   -   y    y    y      y    -    -     y     -

HGT    -   -   -    y    y      -    y    y     y     y    Orthometric/ellipsoid
                                                           as GEOID is present or not

AZM    y   -   y    y    -      y    y    -     -     -

HAN    y   y   y    y    -      y    y    -     -     -

VAN    y   -   y    y    -      y    y    y     y     -    GEOID: geodetic or not

ZAN    y   -   y    y    -      y    y    y     y     -    GEOID: geodetic or not

# example - Test25
# do HI/HT apply to DIRs? In Test25.dat it seems confused
DIRSET A00 OSS_TP1 0.001 M
DIR A00 OSS_TP2  0  0   0.0 DMS 15 SOA 0.001 M      # no heights
DIR A00 OSS_TP3 62 41  38.2 DMS 15 SOA 0.001 M      # no ht
DIR A00 OSS1    95 57  16.0 DMS 15 SOA 0.001 M      # no ht

DIRSET A01 OSS_TP2 0.001 M
DIR A01 OSS_TP1   0  0  0.0 DMS 15 SOA 0.001 M CORR 1.508 M
DIR A01 OSS_TP3 311 46 17.9 DMS 15 SOA 0.001 M CORR 1.407 M
DIR A01 OSS1    314 29 32.2 DMS 15 SOA 0.001 M CORR 0.000 M

DIRSET A02 OSS_TP3 0.001 M
DIR A02 OSS_TP1   0  0  0.0 DMS 15 SOA 0.001 M CORR 1.508 M
DIR A02 OSS_TP2  69  4 34.5 DMS 15 SOA 0.001 M CORR 1.449 M
DIR A02 OSS1    255 47 15.4 DMS 15 SOA 0.001 M CORR 0.000 M
DIR A02 OSSB    255 59 27.0 DMS 60 SOA 0.001 M CORR 0.000 M

# Extraction. block string creates groups, groups are extracted with
#    final nominal value and full covariance.
# Option to store solution in binary file, reload just for extraction.
# Does not apply to data, as such, b/c measurements of same thing can be repeated.
EXTR blk POS label (3)
EXTR blk DEL labFr labTo (3)
EXTR blk DIS labFr labTo
EXTR blk HGT labFr labTo
EXTR blk AZM labFr labTo
EXTR blk HAN labFr labAt labTo
EXTR blk VAN labFr labTo
EXTR blk ZAN labFr labTo
EXTR blk DIRSET group
\endverbatim
*/

/// Class DATfile encapsulates reading, parsing, writing and data storage for all
/// records in *.dat files.
class DATfile {
public:
   
   // member data
   DATConfig Config;                   ///< (one) DATConfig object for all config recs
   std::vector<DATAzimuth> Azimuths;   ///< vector of DATAzimuth objects
   std::vector<DATDelta> Deltas;       ///< vector of DATDelta objects
   std::vector<DATDir> Dirs;           ///< vector of DATDist objects
   std::vector<DATDirSet> DirSets;     ///< vector of DATDist objects
   std::vector<DATDist> Dists;         ///< vector of DATDist objects
   std::vector<DATHAngle> HAngles;     ///< vector of DATHAngle objects
   std::vector<DATHeight> Heights;     ///< vector of DATHeight objects
   std::vector<DATPoint> Points;       ///< vector of DATPoint objects
   std::vector<DATVAngle> VAngles;     ///< vector of DATVAngle objects
   std::vector<DATZAngle> ZAngles;     ///< vector of DATZAngle objects
   std::vector<std::string> Extract;   ///< vector of EXTR strings
   std::vector<std::string> DerivedPoints;  ///< vector of PostProc strings (MEAN, ENUO, etc)

   // member functions - c'tor and d'tor are defaults

   /// destroy all the arrays
   void clear(void)
   {
      Azimuths.clear();
      Deltas.clear();
      Dirs.clear();
      DirSets.clear();
      Dists.clear();
      HAngles.clear();
      Heights.clear();
      Points.clear();
      VAngles.clear();
      ZAngles.clear();
      Extract.clear();
      DerivedPoints.clear();
   }

   /// Open and read a .dat file, parse the contents and fill the data vectors.
   /// @param filename input file name
   /// @param badrecs return vector<string> of records that could not be parsed
   /// @return the number of records successfully read and parsed.
   /// @throw if file cannot be opened
   int Read(const std::string& filename, std::vector<std::string>& badrecs)
      throw(gnsstk::Exception);

   /// Open and read a .dat file, parse the contents and fill the data vectors.
   /// @param filename input file name
   /// @param Records vector of all dat records from the .dat file
   /// @param badrecs return vector<string> of records that could not be parsed
   /// @return the number of records successfully read and parsed.
   /// @throw if file cannot be opened
   int ReadAndKeepRecords(const std::string& filename, std::vector<DATrecord *>& Records, std::vector<std::string>& badrecs)
      throw(gnsstk::Exception);

   /// Open and write a .dat file, writing DAT records for all the data vectors.
   /// @param filename file name
   /// @return the number of records successfully written.
   /// @throw if file cannot be opened
   int Write(const std::string& filename) throw(gnsstk::Exception);

   /// Write all the DocStrings from all the DAT record types, and return as a string
   /// @return the entire documentation for the DAT records
   std::string asString(void);

   /// Write an example DAT file, including all optional fields
   /// @return a string containing the sample .dat file
   std::string exampleOutput(void);

private:
   /// Open, read and parse a .dat file, filling a vector of pointers to DATrecord.
   /// @param filename file name
   /// @param recPtrs a std::vector of pointers of 'DATrecord *' containing records
   ///           read.
   /// @param badrecs return vector<string> of records that could not be parsed
   /// @return number of records stored, or an error code
   /// @throw if file cannot be opened
   int ReadAndParseDATfile(const std::string& filename,
                           std::vector<DATrecord *>& recPtrs,
                           std::vector<std::string>& badrecs)
      throw(gnsstk::Exception);

}; // end class DATfile

#endif // LSA_DAT_FILE_PARSER_WRITER_INCLUDE
