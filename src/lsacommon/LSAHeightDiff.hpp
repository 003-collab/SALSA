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

#ifndef LSA_LSA_ELLIPSOID_HEIGHT_DATA_INCLUDE
#define LSA_LSA_ELLIPSOID_HEIGHT_DATA_INCLUDE

#include <string>
#include "LSARecord.hpp"
#include "LSAModifier.hpp"
#include "DATHeight.hpp"

/// Class LSAEHeight encapsulates the data from an ellipsoid height or EHT record
/// in the *.LSA file, including the height, measurement sigma and additional sigma
/// (either additive or multiplicative - PPM) centering errors and refracation.
class LSAHeightDiff : public LSARecord {
public:
   // member data ------------------------------------------------------
   /// static string giving LSA file specification for this record
   static const std::string DocString;

   // labels
   std::string From; ///< string label for the "From" Point
   std::string To;   ///< string label for the "To" Point

   // distance and sigma
   double heightDiff;     ///< Measured height difference
   double sigma;          ///< uncrtainty on the distance in meters
   std::string linUnits;  ///< Units used for linear measurements

   // corrections
   LSAModifier modifiers;

   // UI display
   static const int NUM_UI_DISPLAY_COLUMNS;

   // member functions -------------------------------------------------

   /// constructor
   LSAHeightDiff() : LSARecord(LSAType::HDIF),
       From("from"), To("to"), heightDiff(0.0), sigma(0.0), linUnits("m"),
       modifiers(LSAType::HDIF) {}

   /// destructor
   virtual ~LSAHeightDiff() { }

   /// constructor, given the From and To labels, the height and sigma
   /// @param f string label of the "From" point
   /// @param t string label of the "To" point
   /// @param ht double height measurement in meters
   /// @param sig sigma in meters
   LSAHeightDiff(std::string f, std::string t, double ht, double sig, std::string units)
      : LSARecord(type = LSAType::HDIF), From(f), To(t), heightDiff(ht), sigma(sig), linUnits(units),
        modifiers(LSAType::HDIF)
      {  }

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
   void setUncrLabel(std::string label)                     { modifiers.setUncrLabel(label); }

   void setRefract(double value)                            { modifiers.setRefract(value); }

   void setIsReduced(bool isReduced)                        { modifiers.setIsReduced(isReduced); }
   void setVSCALabel(std::string label)                     { modifiers.setVSCALabel(label); }
   void setVSCAValue(double value)                          { modifiers.setVSCAValue(value); }

   void setCurvCorr(bool curvCorr)                          { modifiers.setCurvCorr(curvCorr); }
   void setOHC(bool ohcCorr)                                { modifiers.setOHC(ohcCorr); }

private:
   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIName() const;

   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIDetails() const;


}; // end class LSAEHeight

#endif   // LSA_LSA_ELLIPSOID_HEIGHT_DATA_INCLUDE

