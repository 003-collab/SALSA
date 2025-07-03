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
/// @file LSAConfig.hpp  Include file for class LSAConfig, configuration input
///                      in *.LSA files, including title, precision, dimension,
///                      and convergence limits

#ifndef LSA_LSA_CONFIG_DATA_INCLUDE
#define LSA_LSA_CONFIG_DATA_INCLUDE

#include <string>
#include <StringUtils.hpp>
#include "lsaUtils.hpp"
#include "LSARecord.hpp"

/// Class LSAConfig encapsulates data for a title, precision of the output for both
/// linear and angular data, the dimension of the problem, and convergence criteria,
/// both a maximum iteration count and a fractional convergence criterion.
class LSAConfig : public LSARecord {
public:
   // member data ------------------------------------------------------

   std::string configType;  ///< the config parameter to be set
   std::string configValue; ///< the value of the setting

   // UI display
   static const int NUM_UI_DISPLAY_COLUMNS;

   // member functions -------------------------------------------------

   /// constructor
   LSAConfig() : LSARecord(LSAType::CONFIG), configType(std::string("--newtype")), configValue(std::string("default value")){}

   /// destructor
   virtual ~LSAConfig() { }

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

   std::string  getLabel() const { return ""; }
   virtual bool referencesPosition(const std::string& label) const { return false; }
   virtual bool referencesModifier(LSAType modifierType, std::string label) const { return false; }
   virtual bool supportsModifier(ModifierKeyword keyword) const { return false; }
   virtual std::string getModifierLabel(ModifierKeyword keyword) const { return ""; }
   virtual bool vscaIsNumeric() const { return false; }

   virtual bool hasValidCovariance() const { return true; } // measurement covariance not supported
   virtual LSAModifier* getModifiers() { return NULL; }

   /// Get a string representation of this object appropriate for writing to a .lsa file
   /// @return string to write to .dat file
   std::string asDATString() const;

private:
   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIName() const;

   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIDetails() const;
}; // end class LSAConfig

#endif   // LSA_LSA_CONFIG_DATA_INCLUDE

