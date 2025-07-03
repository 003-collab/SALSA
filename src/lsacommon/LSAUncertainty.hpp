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
/// @file LSAUncr.hpp  Include file for class LSAUncr, uncrtainty values applie
/// to points and measurements.

#ifndef LSA_UNCERRECORD_INCLUDE
#define LSA_UNCERRECORD_INCLUDE

#include <string>
#include <StringUtils.hpp>
#include "LSARecord.hpp"
#include <map>

/// Class LSAUncr encapsulates uncrtainty data to be applied to other records
/// Other position and measurement records can apply these values by including
/// the "label" tag in their descriptions.
///
///
///

class LSAUncertainty : public LSARecord {
public:
   // member data ------------------------------------------------------
   /// static string giving LSA file specification for this record
   static const std::string DocString;

   // values
   std::string label;       ///< unique label for this uncr record
   double      Sigma;       ///< sigma to apply
   double      PPM;         ///< ppm to apply
   double      AtCenter;    ///< centering error to apply to AT point
   double      FromCenter;  ///< centering error to apply to FROM point
   double      ToCenter;    ///< centering error to apply to TO point
   std::string linUnits;    ///< units for linear measurements
   std::string sigmaUnits;  ///< angle or linear unit for sigma

   // flags
   bool hasAddSigma;             ///< flag indicating record contains sigma value
   bool hasPPM;             ///< flag indicating record contains PPM value
   bool hasAtCenter;        ///< flag indicating record contains AtCenter value
   bool hasFromCenter;      ///< flag indicating record contains FromCenter value
   bool hasToCenter;        ///< flag indicating record contains ToCenter value

   // UI display
   static const int NUM_UI_DISPLAY_COLUMNS;

   // member functions -------------------------------------------------

   /// constructor
   LSAUncertainty() : LSARecord(LSAType::UNCR),
       label("newUNCR"), Sigma(0.0), PPM(0.0), AtCenter(0.0), FromCenter(0.0), ToCenter(0.0),
       linUnits("M"), sigmaUnits("M"), hasAddSigma(false),  hasPPM(false), hasAtCenter(false), hasFromCenter(false), hasToCenter(false){}


   LSAUncertainty(std::string label) : LSARecord(LSAType::UNCR),
       label(label), Sigma(0.0), PPM(0.0), AtCenter(0.0), FromCenter(0.0), ToCenter(0.0),
       linUnits("M"), sigmaUnits("M"), hasAddSigma(false),  hasPPM(false), hasAtCenter(false), hasFromCenter(false), hasToCenter(false){}

   /// destructor
   virtual ~LSAUncertainty() { }

   double getAddSigmaValue()   const { return (hasAddSigma   ? Sigma      : 0.0); }
   double getPPMValue()        const { return (hasPPM        ? PPM        : 0.0); }
   double getAtCenterValue()   const { return (hasAtCenter   ? AtCenter   : 0.0); }
   double getFromCenterValue() const { return (hasFromCenter ? FromCenter : 0.0); }
   double getToCenterValue()   const { return (hasToCenter   ? ToCenter   : 0.0); }

   // Will check a given record type to see if the Uncertainty component is applicable to it or not.
   // Based off Table 10.1 in the Salsa User Manual
   bool isSigmaApplicable(const LSAType &) const;
   bool isPPMApplicable(const LSAType &) const;
   bool isAtCenterApplicable(const LSAType &) const;
   bool isFromCenterApplicable(const LSAType &) const;
   bool isToCenterApplicable(const LSAType &) const;

   /// Parse a string from a single line in the LSA file.
   /// @param line single line read from LSA file
   /// @return true if successful
   virtual void fromString(const std::string& line);

   /// write the object as a string for the LSA file.
   /// param out, an ostream reference to the output filestream or stringstream
   virtual void writeLSAString(std::ostream & out) const;

   /// Get the number of columns used to display this record type in the Project Navigator
   /// @return the number of columns to display
   virtual int getNumDisplayColumns() const { return NUM_UI_DISPLAY_COLUMNS; }

   /// Get a string to display in a given column in the Project Navigator
   /// @param the column number to display
   /// @return the string to display in the UI
   virtual std::string getUIColumnData(int columnNumber) const;

   /// If this record refers to a height modifier with label oldLabel, change the reference to refer to newLabel instead
   /// @param oldLabel the old label
   /// @param newLabel the new label
   virtual void renameModifierLabel(LSAType type, std::string oldLabel, std::string newLabel);
   virtual void renamePosition(std::string oldLabel, std::string newLabel) { return; }
   virtual std::vector<std::string> getReferencedPositions() const { return std::vector<std::string>(); } // empty vector

   virtual double getVSCAFactor() { return 1.0; }

   std::string  getLabel() const { return label; }
   virtual bool referencesPosition(const std::string& label) const { return false; }
   virtual bool referencesModifier(LSAType modifierType, std::string label) const { return false; }
   virtual bool supportsModifier(ModifierKeyword keyword) const { return false; }
   virtual std::string getModifierLabel(ModifierKeyword keyword) const { return ""; }
   virtual bool vscaIsNumeric() const { return false; }

   virtual bool hasValidCovariance() const { return true; } // measurement covariance not supported
   virtual LSAModifier* getModifiers() { return NULL; }

   std::string getUIDetailsBrief() const;

private:
   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIName() const;

   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIDetails() const;

}; // end class LSASample

#endif   // LSA_UNCERRECORD_INCLUDE

