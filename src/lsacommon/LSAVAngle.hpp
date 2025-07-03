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
/// @file LSAVAngle.hpp  Include file for class LSAVAngle, data for the *.LSA file
///                      record VAN vertical angle measurement

#ifndef LSA_LSA_VERTICAL_ANGLE_DATA_INCLUDE
#define LSA_LSA_VERTICAL_ANGLE_DATA_INCLUDE

#include <string>
#include "LSARecord.hpp"
#include "LSAModifier.hpp"
#include <DATVAngle.hpp>

/// Class LSAVAngle encapsulates the data for a vertical angle measurement,
/// including the labels "From" and "To" which define the sense and orientation of
/// the angle, the measurement itself and its uncrtainty sigma, plus an optional
/// second sigma (additive), and corrections.
class LSAVAngle : public LSARecord {
public:
   // member data ------------------------------------------------------
   /// static string giving LSA file specification for this record
   static const std::string DocString;

   // labels
   std::string From; ///< string label for the "From" Point
   std::string To;   ///< string label for the "To" Point

   // angle and sigma
   double value;           ///< angle measurement in radians
   double angleDecDeg;     ///< angle measurement in decimal degrees
   bool usesDecDeg;        ///< true when record stores angle as dec degrees

   bool	angleDMSIsNeg;     ///< is_negative component of DMS angle
   int angleDeg;           ///< DMS decimal component of angle measurement
   int angleMin;           ///< DMS minute component of angle measurement
   double angleSec;        ///< DMS seconds component of angle measurement
   double sigma;           ///< angle sigma in radians
   std::string sigmaUnits; ///< units for angle sigma

   // corrections
   LSAModifier modifiers;

   // UI display
   static const int NUM_UI_DISPLAY_COLUMNS;

   // member functions -------------------------------------------------

   /// constructor
   LSAVAngle()
          : LSARecord(LSAType::VANG), From("from"), To("to"), value(0.0), angleDMSIsNeg(false), angleDecDeg(0.0), usesDecDeg(true), sigma(0.0), sigmaUnits("soa"),
            modifiers(LSAType::VANG) {}

   /// constructor given labels and angle in radians
   /// @param f string label of the "From" Point
   /// @param t string label of the "To" Point
   /// @param angle double value of the angle in radians
   /// @param sig double uncrtainty of the measurment in radians
   LSAVAngle(std::string f, std::string t, double angle, double sig, std::string sigmaUnits)
      : LSARecord( LSAType::VANG), From(f), To(t), value(angle), sigma(sig), sigmaUnits(sigmaUnits),
        usesDecDeg(false), modifiers(LSAType::VANG)
        { }

   /// destructor
   virtual ~LSAVAngle() { }

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
   virtual bool referencesPosition(const std::string& label) const { return From == label || To == label; }
   virtual bool referencesModifier(LSAType modifierType, std::string label) const;
   virtual bool supportsModifier(ModifierKeyword keyword) const { return modifiers.supportsModifierControl(type, keyword); }
   virtual std::string getModifierLabel(ModifierKeyword keyword) const { return modifiers.getModifierLabel(keyword); }
   virtual bool vscaIsNumeric() const { return modifiers.hasNumericVSCA(); }

   virtual bool hasValidCovariance() const { return true; } // measurement covariance not supported
   virtual LSAModifier* getModifiers() { return &modifiers; }

   // Modifiers
   void setHeightFromLabel(std::string label)               { modifiers.setHeightFromLabel(label); }
   void setHeightFromValue(double value, std::string units) { modifiers.setHeightFromValue(value, units); }
   void setHeightFromSigma(double value, std::string units)   { modifiers.setHeightFromSigma(value, units); }

   void setHeightToLabel(std::string label)                 { modifiers.setHeightToLabel(label); }
   void setHeightToValue(double value, std::string units)   { modifiers.setHeightToValue(value, units); }
   void setHeightToSigma(double value, std::string units)   { modifiers.setHeightToSigma(value, units); }

   void setUncrLabel(std::string label)                     { modifiers.setUncrLabel(label); }

   void setRefract(double value)                            { modifiers.setRefract(value); }

   void setIsReduced(bool isReduced)                        { modifiers.setIsReduced(isReduced); }
   void setVSCALabel(std::string label)                     { modifiers.setVSCALabel(label); }
   void setVSCAValue(double value)                          { modifiers.setVSCAValue(value); }

private:
   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIName() const;

   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIDetails() const;

}; // end class LSAVAngle

#endif   // LSA_LSA_VERTICAL_ANGLE_DATA_INCLUDE
