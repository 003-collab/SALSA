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
/// @file LSAEHeight.hpp  Include file for class LSAEHeight, data for the *.LSA file
///                    OHT record, orthometric height measurement

#ifndef LSA_HDIR_INCLUDE
#define LSA_HDIR_INCLUDE

#include <string>
#include "LSARecord.hpp"
#include "LSADirGroup.hpp"
#include "DATDir.hpp"

/// Class LSAEHeight encapsulates the data from an ellipsoid height or EHT record
/// in the *.LSA file, including the height, measurement sigma and additional sigma
/// (either additive or multiplicative - PPM) centering errors and refracation.
class LSAHDir : public LSARecord {
public:
   // member data ------------------------------------------------------
   /// static string giving LSA file specification for this record
   static const std::string DocString;

   // labels
   std::string dirGroupLabel; ///< string label for the direction group for this direction
   std::string toLabel;       ///< string label for the "To" Point

   // angle and sigma
   bool	angleDMSIsNeg;  ///< is_negative component of DMS angle
   int angleDeg;        ///< Degree component of DMS angle
   int angleMin;        ///< Minute component of DMS angle
   double angleSec;     ///< Second component of DMS angle

   bool usesDecDeg;     ///< true when record stores angle as dec degrees
   double angleDecDeg;  ///< angle value in decimal degrees

   double sigma;           ///< angle sigma in radians
   std::string sigmaUnits; ///< units for measurement sigma (rad, deg, or soa)

   // corrections
   LSAModifier modifiers;

   // UI display
   static const int NUM_UI_DISPLAY_COLUMNS;

   // member functions -------------------------------------------------

   /// constructor
   LSAHDir() : LSARecord(LSAType::HDIR), dirGroupLabel("DGRPlabel"), toLabel("to"), usesDecDeg(true), angleDecDeg(0.0), sigma(0.0), sigmaUnits("soa"),
         modifiers(LSAType::HDIR) {angleDMSIsNeg = false;}

   /// destructor
   virtual ~LSAHDir() { }

   /// constructor, given the From and To labels, the height and sigma
   /// @param f string label of the "From" point
   /// @param t string label of the "To" point
   /// @param ht double height measurement in meters
   /// @param sig sigma in meters
   LSAHDir(std::string dirGroup, std::string to)
      : LSARecord(type = LSAType::HDIR), dirGroupLabel(dirGroup), toLabel(to),
        modifiers(LSAType::HDIR)
      { angleDMSIsNeg = false; }

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
   virtual void renamePosition(std::string oldLabel, std::string newLabel);
   virtual std::vector<std::string> getReferencedPositions() const;

   std::string  getLabel() const { return ""; }
   virtual bool referencesPosition(const std::string& label) const { return toLabel == label; }
   virtual bool referencesModifier(LSAType modifierType, std::string label) const;
   virtual bool supportsModifier(ModifierKeyword keyword) const { return modifiers.supportsModifierControl(type, keyword); }
   virtual std::string getModifierLabel(ModifierKeyword keyword) const { return modifiers.getModifierLabel(keyword); }
   virtual bool vscaIsNumeric() const { return modifiers.hasNumericVSCA(); }

   virtual bool hasValidCovariance() const { return true; } // measurement covariance not supported
   virtual LSAModifier* getModifiers() { return &modifiers; }

   void setHeightToLabel(std::string label)                 { modifiers.setHeightToLabel(label); }
   void setHeightToValue(double value, std::string units)   { modifiers.setHeightToValue(value, units); }
   void setHeightToSigma(double value, std::string units)   { modifiers.setHeightToSigma(value, units); }

private:
   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIName() const;

   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIDetails() const;

};

#endif   // LSA_HDIR_INCLUDE

