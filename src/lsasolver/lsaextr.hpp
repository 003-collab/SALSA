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
/// @file lsaextr.hpp  Extraction for lsasolver, include file

#ifndef LSA_EXTRACTION_INCLUDE
#define LSA_EXTRACTION_INCLUDE

// system
#include <string>
#include <vector>
#include <map>
// gnsstk
#include "Exception.hpp"
#include "StringUtils.hpp"
#include "Namelist.hpp"
#include "ref_ptr.hpp"
// lsa
#include "MatrixVector.hpp"
#include "Point.hpp"
#include "MeasBase.hpp"

//------------------------------------------------------------------------------------
/// Class to implement extraction of solution and covariance from the final NLLS
/// solution given a list of extraction (DAT) records belonging to the same block.
class Extraction
{
private:
   std::string block;                           ///< block string common to all
   std::vector<Point> ExtrPoints;               ///< Points to extract
   std::vector< ref_ptr<MeasBase> > ExtrMeas;   ///< measurements to extract

   std::vector<std::string> labels;    ///< labels, parallel to the following
   std::vector<int> isangle;           ///< 1 if angle, else 0
   gnsstk::FlexMatrix<double> Partials; ///< Partials matrix for this set
   gnsstk::Vector<double> NominalData;  ///< Nominal data for this set
   gnsstk::Matrix<double> Covariance;   ///< Solution Covariance for this set

   // TD keep this? problem is 2D semimajor of covariance for PPM output
   /// need to access the 3x3 covariance of each Delta, for sigma(PPM); save indexes
   //std::map< std::string, gnsstk::Vector<int> > DeltaIndexesMap;

public:
   /// empty constructor
   Extraction(void) : block(std::string()) { }

   /// Constructor from block string
   Extraction(std::string& t) : block(t) { }

   /// Return block string for this object
   /// @return block string
   std::string getBlock(void) const { return block; }

   /// Parse a string just to get the block string
   /// @param extrstr input extraction record
   /// @param msg output message if parsing fails
   /// @return block string or blank if failure
   static std::string getBlockFromString(const std::string& extrstr, std::string& msg)
      throw(gnsstk::Exception)
   {
      std::string block, line(extrstr);
      // start parsing the string
      gnsstk::StringUtils::stripTrailing(line,"\n");
      gnsstk::StringUtils::stripTrailing(line,"\r");
      gnsstk::StringUtils::stripTrailing(line," ");
      gnsstk::StringUtils::stripLeading(line," ");
      if(line.empty()) {
         msg = "Record is blank";
         return block;
      }
      block = gnsstk::StringUtils::stripFirstWord(line, ' ');
      if(block != std::string("EXTR")) {
         msg = "Missing first word 'EXTR'";
         return block;
      }
      block = gnsstk::StringUtils::firstWord(line, ' ');
      if(block.empty()) msg = "No block string";

      return block;
   }

   /// Add an extraction record, of the form EXTR block TYP label label [label]
   /// and convert to measurement
   /// Block must match this->block, unless it is undefined, then block is set.
   /// @param extrstr string to add
   /// @param msg return msg explaining why record could not be stored.
   /// @param StateNames Namelist for LS state, for checking
   /// @return true if the block matches and the record parses and is stored.
   bool Add(const std::string& extrstr, std::string& msg,
                     const gnsstk::Namelist& StateNames) throw(gnsstk::Exception);

   /// Perform the extraction; that is compute Partials, NominalData and Covariance.
   /// @param StateNames Namelist for LS state, identifies Cov matrix
   /// @param AllPoints map of all Points (GD.Points)
   /// @param Cov Matrix<double> covariance matrix for StateNames
   /// @param APV double APV for solution
   /// @param is2D if true this problem is 2-dimensional
   /// @param noAPV if true do not scale covariance with APV
   void Compute(const gnsstk::Namelist& StateNames,
                std::map<std::string,Point>& AllPoints,
                const gnsstk::Matrix<double>& Cov,
                const double& APV,
                const bool& is2D,
                const bool& noAPV)
      throw(gnsstk::Exception);

   /// Output the results
   /// @param os the stream to write
   /// @param prec digits of precision
   /// @param is2D if true this problem is 2-dimensional
   /// @param doSOA if true output in seconds-of-arc
   void Output(std::ostream& os, const int& prec, const bool& is2D, const bool& doSOA)
      throw(gnsstk::Exception);

}; // end class Extraction

#endif   // define LSA_EXTRACTION_INCLUDE
