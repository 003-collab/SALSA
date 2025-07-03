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
/// @file LSAPoint.hpp  Include file for class LSAPoint, data for the *.LSA file
///                     record POS 3-D XYZ coordinate position

#ifndef LSA_LSA_COMMENT_DATA_INCLUDE
#define LSA_LSA_COMMENT_DATA_INCLUDE

#include <string>
#include "LSARecord.hpp"

/// Class LSAPoint encapsulates data for a 3-D coordinate position and includes
/// a label and whether or not it is considered fixed.
class LSAComment : public LSARecord {
public:
   // member data ------------------------------------------------------
   /// static string giving LSA file specification for this record
   static const std::string DocString;

   std::string label;         ///< unique label for this point
   // TD this should probably be an enum
   std::string lineContents;  ///< contents of line read from lsa file

   // UI display
   static const int NUM_UI_DISPLAY_COLUMNS;

   // member functions -------------------------------------------------

   /// destructor
   virtual ~LSAComment() { }

   // lineContents must be initialized to an empty string for blank lines to parse correctly
   LSAComment() : LSARecord( LSAType::COMMENT), lineContents("") {}

   LSAComment(std::string line);

   /// set the fixtype to "fixed" if input is true, otherwise "estimate"
   /// @param fix true to defined the Point fixtype as "fixed," otherwise "estimate"
   /// @return the fixtype
   int setFixed(bool fix) { return fix; }

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

   virtual double getVSCAFactor() const { return 1.0; }

   std::string  getLabel() const { return ""; }
   virtual bool referencesPosition(const std::string& label) const { return false; }
   virtual bool referencesModifier(LSAType modifierType, std::string label) const { return false; }
   virtual bool supportsModifier(ModifierKeyword keyword) const { return false; }
   virtual std::string getModifierLabel(ModifierKeyword keyword) const { return ""; }
   virtual bool        vscaIsNumeric() const { return false; }

   virtual bool hasValidCovariance() const { return true; } // measurement covariance not supported
   virtual LSAModifier* getModifiers() { return NULL; }

private:
   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIName() const;

   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIDetails() const;

}; // end class LSAPoint

#endif   // LSA_LSA_COMMENT_DATA_INCLUDE

