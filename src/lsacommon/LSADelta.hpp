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
/// @file LSADelta.hpp  Include file for class LSADelta, data for the *.LSA file
///                     record DEL, 3-D XYZ coordinate difference measurement

#ifndef LSA_LSA_DELTA_DATA_INCLUDE
#define LSA_LSA_DELTA_DATA_INCLUDE

#include <string>
#include <map>
#include "Matrix.hpp"

#include "LSARecord.hpp"
#include "LSAModifier.hpp"
#include "DATDelta.hpp"

/// Class LSADelta encapsulates the data from a DEL record in the *.LSA file;
/// a delta-XYZ measurement, including the coordinates and measurement covariance
/// matrix, and the labels "From" and "To", and corrections. The measurement is
/// defined as the coordinate difference Point("To") minus Point("From").
class LSADelta : public LSARecord {
public:
   // member data ------------------------------------------------------
   /// static string giving LSA file specification for this record
   static const std::string DocString;

   // labels
   std::string From; ///< string label for the "From" Point
   std::string To;   ///< string label for the "To" Point

   // coordinate differences
   double dx;             ///< X-coordinate difference (To - From)
   double dy;             ///< Y-coordinate difference (To - From)
   double dz;             ///< Z-coordinate difference (To - From)
   std::string linUnits;  ///< Units used for linear measurements

   // components of the UT portion of the covariances matrix.
   double covxx;     ///< the X,X component of the covariance matrix
   double covxy;     ///< the X,Y component of the covariance matrix
   double covxz;     ///< the X,Z component of the covariance matrix
   double covyy;     ///< the Y,Y component of the covariance matrix
   double covyz;     ///< the Y,Z component of the covariance matrix
   double covzz;     ///< the Z,Z component of the covariance matrix

   // corrections
   LSAModifier modifiers;

   // UI display
   static const int NUM_UI_DISPLAY_COLUMNS;

   // member functions -------------------------------------------------

   /// constructor
   LSADelta() : LSARecord(LSAType::DXYZ),
       From("from"), To("to"), dx(0.0), dy(0.0), dz(0.0),
       covxx(0.0), covxy(0.0), covxz(0.0), covyy(0.0), covyz(0.0), covzz(0.0),linUnits("m"),
       modifiers( LSAType::DXYZ) {}

   /// destructor
   virtual ~LSADelta() { }

   /// constructor, given the From and To labels,
   /// the X,Y,Z components of the delta, and the covariance matrix given
   /// in upper triangular form in the calling arguments, i.e. components
   /// in the order cov(X,X),cov(X,Y),cov(X,Z),cov(Y,Y),cov(Y,Z),cov(Z,Z).
   /// @param f string label of the "From" point
   /// @param t string label of the "To" point
   /// @param dxin double X-coordinate of the delta
   /// @param dyin double Y-coordinate of the delta
   /// @param dzin double Z-coordinate of the delta
   /// @param cxx double the X,X component of the covariance matrix
   /// @param cxy double the X,Y component of the covariance matrix
   /// @param cxz double the X,Z component of the covariance matrix
   /// @param cyy double the Y,Y component of the covariance matrix
   /// @param cyz double the Y,Z component of the covariance matrix
   /// @param czz double the Z,Z component of the covariance matrix
   LSADelta(std::string f, std::string t, double dxin, double dyin, double dzin, std::string units,
                                           double cxx, double cxy, double cxz,
                                           double cyy, double cyz, double czz)
      : LSARecord(LSAType::DXYZ),
        From(f), To(t), dx(dxin), dy(dyin), dz(dzin), linUnits(units),
            covxx(cxx), covxy(cxy), covxz(cxz), covyy(cyy), covyz(cyz), covzz(czz),
            modifiers( LSAType::DXYZ)
      { }

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

   std::string  getLabel() const { return ""; }
   virtual bool referencesPosition(const std::string& label) const { return From == label || To == label; }
   virtual void renamePosition(std::string oldLabel, std::string newLabel);
   virtual std::vector<std::string> getReferencedPositions() const;

   virtual bool referencesModifier(LSAType modifierType, std::string label) const;
   virtual bool supportsModifier(ModifierKeyword keyword) const { return modifiers.supportsModifierControl(type, keyword); }
   virtual std::string getModifierLabel(ModifierKeyword keyword) const { return modifiers.getModifierLabel(keyword); }
   virtual bool vscaIsNumeric() const { return modifiers.hasNumericVSCA(); }

   bool hasValidCovariance() const;
   virtual LSAModifier* getModifiers() { return &modifiers; }

   // Modifiers
   void setHeightFromLabel(std::string label)               { modifiers.setHeightFromLabel(label); }
   void setHeightFromValue(double value, std::string units) { modifiers.setHeightFromValue(value, units); }
   void setHeightFromSigma(double value, std::string units) { modifiers.setHeightFromSigma(value, units); }

   void setHeightToLabel(std::string label)               { modifiers.setHeightToLabel(label); }
   void setHeightToValue(double value, std::string units) { modifiers.setHeightToValue(value, units); }
   void setHeightToSigma(double value, std::string units) { modifiers.setHeightToSigma(value, units); }


   void setUncrLabel(std::string label)                   { modifiers.setUncrLabel(label); }

   void setVSCALabel(std::string label) { modifiers.setVSCALabel(label); }
   void setVSCAValue(double value)      { modifiers.setVSCAValue(value); }

private:
   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIName() const;

   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIDetails() const;

}; // end class LSADelta

#endif   // LSA_LSA_DELTA_DATA_INCLUDE
