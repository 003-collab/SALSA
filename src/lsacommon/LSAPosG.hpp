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

#ifndef LSA_LSA_POINT_DATA_INCLUDE
#define LSA_LSA_POINT_DATA_INCLUDE

#include <string>

// gnsstk
#include "Matrix.hpp"
#include "SunEarthSatGeometry.hpp"

// lsa
#include "DATPoint.hpp"
#include "LSARecord.hpp"
#include "LSAModifier.hpp"

/// Class LSAPosG encapsulates data for a geodetic 3-D coordinate position and includes
/// a label and whether or not it is considered fixed.
class LSAPosG : public LSARecord {
public:


    // member data ------------------------------------------------------

    std::string label;              ///< unique label for this point
    bool useDecimalDegrees;         ///< true when record stores lat and lon as dec degrees

    // initial latitude
    bool	    latDMSIsNeg;        ///< sign for DMS latitude
    int         latDeg;             ///< degree component of latitude
    int         latMin;             ///< minute component of latitude
    double      latSec;             ///< second component of latitude
    double      latDecDeg;          ///< latitude expressed as decimal degrees
    std::string latDir;             ///< latitude direction, must be N or S

    // initial longitude
    bool	    lonDMSIsNeg;        ///< sign for DMS longitude
    int         lonDeg;             ///< degree component of longitude
    int         lonMin;             ///< minute component of longitude
    double      lonSec;             ///< second component of longitude
    double      lonDecDeg;          ///< lonitude expressed as decimal degrees
    std::string lonDir;             ///< longitude direction, must be W or E

    // initial height
    double      height;             ///< height
    std::string heightUnits;        ///< units for height in LSA file
    bool heightIsEllipsoidal = true; ///< Determines if height is above ellipsoid or MSL
    double initialUndulation = 0.0; /// Only gets set if the

    // components of the UT portion of the covariances matrix.
    bool  hasCovariance;       ///< covariance exists in LSA file
    double covnn;              ///< the X,X component of the covariance matrix
    double covne;              ///< the X,Y component of the covariance matrix
    double covnu;              ///< the X,Z component of the covariance matrix
    double covee;              ///< the Y,Y component of the covariance matrix
    double coveu;              ///< the Y,Z component of the covariance matrix
    double covuu;              ///< the Z,Z component of the covariance matrix

    // constraints
    LSAFixedState fixedState;     ///< floating, constrained or fixed
    bool          isNorthFixed;   ///< true if the north position component is fixed
    bool          isEastFixed;    ///< true if the east position component is fixed
    bool          isUpFixed;      ///< true if the up position component is fixed

    // corrections
    LSAModifier modifiers;

    // UI display
    static const int NUM_UI_DISPLAY_COLUMNS; 

    // member functions -------------------------------------------------

    /// destructor
    virtual ~LSAPosG() { }

    /// coordinates = 0,0,0 and the covariance matrix to 0.0
    LSAPosG() : LSARecord( LSAType::POSG),
                label(std::string("newPOSG")),
                useDecimalDegrees(false),
                latDeg(0.0),latMin(0.0),latSec(0.0),latDir("N"),latDMSIsNeg(false),
                lonDeg(0.0),lonMin(0.0),lonSec(0.0),lonDir("E"),lonDMSIsNeg(false),
                height(0.0),heightUnits("m"),
                hasCovariance(false),
                fixedState(LSAFixedState::FLOATING),
                covnn(0.0),covne(0.0),covnu(0.0),covee(0.0),coveu(0.0),covuu(0.0),
                isNorthFixed(false), isEastFixed(false), isUpFixed(false),
                modifiers( LSAType::POSG)
      { }

    /// constructor from a label only. fixtype is set to unknown, coordinates = 0,0,0
    /// @param lab  string containing the label for this Point
    LSAPosG(std::string lab) : LSARecord(LSAType::POSG),
                label(lab),
                useDecimalDegrees(false),
                latDeg(0.0),latMin(0.0),latSec(0.0), latDir("N"),latDMSIsNeg(false),
                lonDeg(0.0),lonMin(0.0),lonSec(0.0), lonDir("E"),lonDMSIsNeg(false),
                height(0.0),
                hasCovariance(false),
                fixedState(LSAFixedState::FLOATING),
                covnn(0.0),covne(0.0),covnu(0.0),
                covee(0.0),coveu(0.0),covuu(0.0),
                isNorthFixed(false), isEastFixed(false), isUpFixed(false),
                modifiers( LSAType::POSG)
      { }

    /// constructor from label xyz coordinates and optional covariance (default 0);
    /// fixtype is set to 1 (estimate)
    /// @param lab  string containing the label for this Point
    /// @param sxx double the X,X component of the covariance matrix
    /// @param sxy double the X,Y component of the covariance matrix
    /// @param sxz double the X,Z component of the covariance matrix
    /// @param syy double the Y,Y component of the covariance matrix
    /// @param syz double the Y,Z component of the covariance matrix
    /// @param szz double the Z,Z component of the covariance matrix
    LSAPosG(std::string lab,
            int latDeg, int latMin, double latSec,
            int lonDeg, int lonMin, double lonSec,
            double sxx, double sxy, double sxz,
            double syy, double syz, double szz)
      : LSARecord(LSAType::POSG),
        label(lab),
        useDecimalDegrees(false),
        latDeg(latDeg),latMin(latMin),latSec(latSec),latDMSIsNeg(false),
        lonDeg(lonDeg),lonMin(lonMin),lonSec(lonSec),lonDMSIsNeg(false),
        height(0.0),
        hasCovariance(false),
        fixedState(LSAFixedState::CONSTRAINED),
        covnn(sxx), covne(sxy), covnu(sxz), covee(syy), coveu(syz), covuu(szz),
        isNorthFixed(false), isEastFixed(false), isUpFixed(false),
        modifiers( LSAType::POSG)
      { }

    void setCovariance(double cnn, double cne, double cnu, double cee, double ceu, double cuu);

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
    virtual std::vector<std::string> getReferencedPositions() const;

    /// set the fixtype to "fixed" if input is true, otherwise "estimate"
    /// @param input FIX, or ENU to indicate fixed,
    /// otherwise any combination of E | N | U to indicate constrained axes
    /// @return the fixtype
    void setFixedConstraints(std::string input);

    void fromECEFXYZCoords(double X, double Y, double Z);

    //------------------------------------------------------------------------------------
    /// Throw an execption if Lat/Lon degrees or minutes are not an int.  Also throws an
    /// execption if Lat/Lon minutes or seconds are negative.
    /// param vector<string> F the vector of strings to parse
    /// param latDegIndex the word index of latDeg in input
    void getLatLonHeight(double &lonOut, double &latOut, double &heightOut) const;

    std::string  getLabel() const { return label; }
    virtual bool referencesPosition(const std::string& label) const { return false; }
    virtual bool referencesModifier(LSAType modifierType, std::string label) const;
    virtual bool supportsModifier(ModifierKeyword keyword) const { return modifiers.supportsModifierControl(type, keyword); }
    virtual std::string getModifierLabel(ModifierKeyword keyword) const { return modifiers.getModifierLabel(keyword); }
    virtual bool vscaIsNumeric() const { return modifiers.hasNumericVSCA(); }
    bool isFixed() const { return fixedState == LSAFixedState::FIXED || ( isNorthFixed && isEastFixed && isUpFixed); }

    bool hasValidCovariance() const;
    virtual LSAModifier* getModifiers() { return &modifiers; }

    // Modifiers
    void setVSCALabel(std::string label) { modifiers.setVSCALabel(label); }
    void setVSCAValue(double value)      { modifiers.setVSCAValue(value); }

    double giveStandardLat() const; // Returns a decimal degree value oriented North
    double giveStandardLon(bool forcePos=false) const; // Returns a decimal degree value oriented East

private:
   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIName() const;

   /// Output as a string (one line) to display in the UI. Pure virtual
   /// @return a string to display in the UI
   virtual std::string getUIDetails() const;

}; // end class LSAPoint

#endif   // LSA_LSA_POINT_DATA_INCLUDE

