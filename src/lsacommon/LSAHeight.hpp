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
/// @file LSAHeight.hpp  Include file for class LSAHeight, uncrtainty values applie
/// to points and measurements.

#ifndef LSAHEIGHTRECORD_INCLUDE
#define LSAHEIGHTRECORD_INCLUDE

#include <map>
#include <string>
#include <StringUtils.hpp>
#include "LSARecord.hpp"

/// Class LSAHeight encapsulates instrument and target height data to be applied to other records
/// Other position and measurement records can apply these values by including
/// the "label" tag in their descriptions.
class LSAHeight : public LSARecord {
public:
   // member data ------------------------------------------------------
   /// static string giving LSA file specification for this record
   static const std::string DocString;

   // values
   std::string      label;  ///< unique label for this hght record
   double           value;  ///< height value
   std::string      units;  ///< units for linear measurements
   double      sigmaValue;  ///< sigma to apply
   std::string sigmaUnits;  ///< units for height uncertainty

   // UI display
   static const int NUM_UI_DISPLAY_COLUMNS;

   /// constructor
   LSAHeight() : LSARecord(LSAType::HGHT),
       label("newHGHT"), value(0.0), units("m"), sigmaValue(0.0), sigmaUnits("m") {}

   LSAHeight(std::string label, double value, std::string units, double sigmaValue, std::string sigmaUnits): LSARecord(LSAType::HGHT),
       label(label), value(value), units(units), sigmaValue(sigmaValue), sigmaUnits(sigmaUnits) {}

   /// destructor
   virtual ~LSAHeight() { }

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

   std::string  getLabel() const { return label; }
   virtual bool referencesPosition(const std::string& label) const { return false; }
   virtual bool referencesModifier(LSAType modifierType, std::string label) const { return false; }
   virtual bool supportsModifier(ModifierKeyword keyword) const { return false; }
   virtual std::string getModifierLabel(ModifierKeyword keyword) const { return ""; }
   virtual bool vscaIsNumeric() const { return false; }

   virtual bool hasValidCovariance() const { return true; } // measurement covariance not supported
   virtual LSAModifier* getModifiers() { return NULL; }

private:
   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIName() const;

   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIDetails() const;

}; // end class LSASample

#endif   // LSAHEIGHTRECORD_INCLUDE

