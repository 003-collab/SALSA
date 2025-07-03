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

#ifndef LSAHEIGHTMODIFIER_INCLUDE
#define LSAHEIGHTMODIFIER_INCLUDE

#include <string>
#include <deque>
#include <LSARecord.hpp>
#include <LSAType.hpp>
#include <StringUtils.hpp>


// Enum that mirrors modifier keywords found in LSARecords.
// Used primarily for enabling Record Editor controls when multiple records are selected.
// The primary difference between ModifierKeyword and LSAType is separate values for HEIGHTFROM and HEIGHTTO
enum ModifierKeyword
{
    MODKEY_HEIGHTFROM,
    MODKEY_HEIGHTTO,
    MODKEY_UNCR,
    MODKEY_REFRACT,
    MODKEY_REDUCED,
    MODKEY_VSCA,
    MODKEY_CURV,
    MODKEY_OHC,
};

/// Class LSAHeightModifier encapsulates all of the data to parse and store
/// height modifiers for a LSARecord.  This class is included as a member variable
/// in LSARecord subclasses that use height modifiers.
class LSAModifier
{
public:

    /// Constructor
    /// Constructor parms are used to enable/disable individual modifier types.  For
    /// example, HAngles have no refraction correction, so the LSAHAngle constructor
    /// will set useRefract=false;
    /// @param useHeights enable height corrections
    /// @param useSigma   enable UNCR corrections
    /// @param useRefract enable refraction corrections
    /// @param useGeoid   enable geoid corrections
    /// @param useVSCA    enable variance scaling modifiers
    /// @param useCurv    enable curvature corrections
    /// @param useOHC     enable orthometric height correction
    LSAModifier( LSAType lsaType) :
                 unitsFrom("M"), unitsTo("M"),
                 supportsHeightFrom(supportsModifierControl(lsaType, MODKEY_HEIGHTFROM)),
                 supportsHeightTo(  supportsModifierControl(lsaType, MODKEY_HEIGHTTO)),
                 supportsUNCR(     supportsModifierControl(lsaType, MODKEY_UNCR)),
                 supportsVSCA(      supportsModifierControl(lsaType, MODKEY_VSCA)),
                 supportsRefract(   supportsModifierControl(lsaType, MODKEY_REFRACT)),
                 supportsGeoid(     supportsModifierControl(lsaType, MODKEY_REDUCED)),
                 supportsCurv(      supportsModifierControl(lsaType, MODKEY_CURV)),
                 supportsOHC(      supportsModifierControl(lsaType, MODKEY_OHC)),
                 hasHeightFrom(false), hasHeightTo(false), hasUNCR(false), hasVSCA(false),
                 hasRefract(false), isReduced(false), curvCorrectionNeeded(false),
                 ohcNeeded(false), vscaIsNumeric(false), refractCorr(0.0)
                 {}



    // LSAString
    std::string getLSAString() const; 

    // From Height
    bool        hasFromHeight() const {return supportsHeightFrom && hasHeightFrom; }
    std::string getHeightFromLabel() const { return labelFrom; }
    bool        getHeightFromData(std::string &label, double & value, std::string & units, bool & isNumeric) const;
    double      getHeightFromInMeters();
    bool        fromHeightIsNumeric() const { return heightFromIsNumeric; }
    double      getNumericHeightFrom() const {return numericFrom; }
    std::string getHeightFromUnits() const {return unitsFrom; }
    double      getHeightFromSigma() const {return sigmaFrom; }
    std::string getHeightFromSigmaUnits() const {return sigmaUnitsFrom; }

    void        clearHeightFrom();
    void        setHeightFromData(std::string label, double value, std::string units, double sigma, std::string sigmaUnits);
    void        setHeightFromData(std::string label, double value, std::string units);
    void        setHeightFromLabel(std::string label);
    void        setHeightFromValue(double value, std::string units);
    void        setHeightFromSigma(double sigma, std::string sigmaUnits);


    // To Height
    bool        hasToHeight() const {return supportsHeightTo && hasHeightTo; }
    std::string getHeightToLabel() const { return labelTo; }
    bool        getHeightToData(std::string &label, double & value, std::string & units, double & sigma, std::string & sigmaUnits, bool & isNumeric) const;
    double      getHeightToInMeters();
    bool        toHeightIsNumeric() const { return heightToIsNumeric; }
    double      getNumericHeightTo() const {return numericTo; }
    std::string getHeightToUnits() const {return unitsTo; }
    double      getHeightToSigma() const {return sigmaTo; }
    std::string getHeightToSigmaUnits() const {return sigmaUnitsTo; }

    void        setHeightToData(std::string label, double value, std::string units, double sigma, std::string sigmaUnits);
    void        setHeightToData(std::string label, double value, std::string units);
    void        clearHeightTo();
    void        setHeightToLabel(std::string label);
    void        setHeightToValue(double value, std::string units);
    void        setHeightToSigma(double sigma, std::string sigmaUnits);

    std::string getUncrLabel() const { return uncrLabel; }
    void        setUncrLabel(std::string label);
    void        clearUncrLabel();

    bool        hasUNCRCorrection() const   { return supportsUNCR && hasUNCR; }

    /// Get for geoid corrections
    /// @return true if this object has already been reduced to the geoid
    bool isReducedToEllipsoid() const   { return supportsGeoid && isReduced; }
    void setIsReduced(bool value)   { isReduced = value; }

    /// Accessor for refraction corrections
    /// @return refraction correction to apply (default value = 0)
    bool   hasRefractionCorrection() const { return supportsRefract && hasRefract; }
    double getRefract() const         { return hasRefract ? refractCorr : 0;}
    double getRawRefract() const      { return refractCorr; }
    void   setRefract(double value)   { hasRefract = true; refractCorr = value; }
    void   enableRefract(bool enable) { hasRefract = enable; }

    /// Accessor for curvature corrections
    /// @return true if this object has non-default curvature corrections to apply
    bool getCurvCorr() const     { return curvCorrectionNeeded;}
    void setCurvCorr(bool value) { curvCorrectionNeeded = value;}

    /// Accessor for orthometric height corrections
    /// @return true if this object has orthometric height corrections to apply
    bool getOHC() const     { return ohcNeeded;}
    void setOHC(bool value) { ohcNeeded = value;}

    /// Accessor for variance scaling modifier
    /// @return true if this object has non-default variance scaling modifier to apply
    bool        hasVSCACorrection() const   { return supportsVSCA && hasVSCA; }
    std::string getVSCALabel() const  { return vscaLabel; }
    bool        hasNumericVSCA() const { return supportsVSCA && vscaIsNumeric; }
    double      getNumericVSCA() const {return numericVSCA; }

    void        setVSCAData(std::string label, double value = 1.0);
    void        setHasVSCACorrection(bool useVSCA) { hasVSCA = useVSCA; }
    void        setVSCAValue(double value);
    void        setVSCALabel(std::string label);
    void        clearVSCA();
    bool        getVSCASupport() const { return supportsVSCA; }

    /// Parse modifiers from a string
    /// @param optionalParms the deque of optional parms to parse
    /// @return true on a succesful parse
    bool parseOptionalParms(std::deque<std::string> inputParms, LSAType recordType, std::string recordLabel, std::vector<std::string> *warningsVector);

    /// Output as a string (one line) to display in the UI
    /// @return a string to display in the UI
    std::string getUIDetails() const;

    std::string getModifierLabel(ModifierKeyword keyword) const;
    void modifyLabel(LSAType lsaType, std::string oldLabel, std::string newLabel);

    static bool supportsModifierControl(LSAType lsaType, ModifierKeyword keyword);

    bool sigmIsMissing();  ///< Returns true if 1) modifier supports UNCR, 2) modifier has a non-empty UNCR label 3) no UNCR record with that label exists in the project
    bool vscaIsMissing();  ///< Returns true if 1) modifier supports VSCA, 2) modifier has a non-empty VSCA label 3) no VSCA record with that label exists in the project
    bool htoIsMissing();   ///< Returns true if 1) modifier supports HGHT, 2) modifier has a non-empty HGHT label 3) no HGHT record with that label exists in the project
    bool hfromIsMissing(); ///< Returns true if 1) modifier supports HGHT, 2) modifier has a non-empty HGHT label 3) no HGHT record with that label exists in the project

    void clearModifiers();

private:
    std::deque<std::string> optionalParms; ///< deque of parms used by Parse methods  

    // Height FROM
    const bool  supportsHeightFrom;  ///< true if the object that contains this object supports height FROM correction
    bool        hasHeightFrom;       ///< true if record contains a height FROM correction
    bool        heightFromIsNumeric; ///< true when heightFrom is stored as a numeric value
    double      numericFrom;         ///< height of instrument (at From)
    std::string unitsFrom;           ///< units for the FROM modifier
    std::string labelFrom;           ///< label for height applied to FROM point
    double      sigmaFrom;               ///< instrument height correction
    std::string sigmaUnitsFrom;          ///< units for instrument height uncertainty

    // Height TO
    const bool  supportsHeightTo;    ///< true if the object that contains this object supports height FROM correction
    bool        hasHeightTo;         ///< true if record contains a height TO correction
    bool        heightToIsNumeric;   ///< true when heightTo is stored as a numeric value
    double      numericTo;           ///< height of target (at To)
    std::string unitsTo;             ///< units for the TO modifier
    std::string labelTo;             ///< label for height applied to TO point
    double      sigmaTo;               ///< instrument height correction
    std::string sigmaUnitsTo;          ///< units for instrument height uncertainty

    // Height labels (may be used instead of numeric values)

    // Additional Sigma
    const bool  supportsUNCR;  ///< true if the object that contains this object supports uncr modifiers
    bool        hasUNCR;       ///< true if record contains additional sigma adjustments
    std::string uncrLabel;     ///< label for the uncr modifier for this DXYZ record

    // Variance scaling
    const bool  supportsVSCA;  ///< true if the object that contains this object supports VSCA modifiers
    bool        hasVSCA;       ///< true if record contains VSCA adjustments
    bool        vscaIsNumeric; ///< true when vsca is stored as a numeric value
    double      numericVSCA;   ///< scaling applied directly to record without use of VSCA record
    std::string vscaLabel;      ///< label for the VSCA modifier record

    // Index of refraction
    const bool  supportsRefract; ///< true if the object that contains this object supports refract modifiers
    bool        hasRefract;      ///< true if refract has been set
    double      refractCorr;     ///< refractive index correction

    // Geoid correction
    const bool  supportsGeoid; ///< true if the object that contains this object supports geoid modifiers
    bool        isReduced;     ///< true if measurement has been reduced to geoid

    // Curvature correction
    const bool  supportsCurv;         ///< true if the object that contains this object supports curv modifiers
    bool        curvCorrectionNeeded; ///< true when curvature correction needs to be applied

    // Orthometric height correction
    const bool  supportsOHC; ///< true if the object that contains this object supports ohc modifiers
    bool        ohcNeeded;   ///< true when orthometric height correction needs to be applied

    /// Parse height modifiers applied to LSARecord subclasses.
    /// @return true on successful parse
    bool ParseHeightModifier    (LSAType recordType, std::string recordLabel, std::vector<std::string> *warningsVector);

    /// Parse sigmas applied to LSARecord subclasses.
    /// @return true on successful parse
    bool ParseUncrModifier     (LSAType recordType, std::string recordLabel, std::vector<std::string> *warningsVector);

    /// Parse refraction modifier applied to LSARecord subclasses.
    /// @return true on successful parse
    bool ParseRefractionModifier(LSAType recordType, std::string recordLabel, std::vector<std::string> *warningsVector);

    /// Parse geoid reduction modifier applied to LSARecord subclasses.
    /// @return true on successful parse
    bool ParseReducedModifier();

    /// Parse VSCA modifier applied to LSARecord subclasses.
    /// @return true on successful parse
    bool ParseVSCAModifier      (LSAType recordType, std::string recordLabel, std::vector<std::string> *warningsVector);

    /// Parse geoid curvature modifier applied to LSARecord subclasses.
    /// @return true on successful parse
    bool ParseCurvatureModifier ();

    /// Parse ohc modifier applied to LSARecord subclasses.
    /// @return true on successful parse
    bool ParseOHCModifier ();

    void renameLabel(LSAType type, std::string oldLabel, std::string newLabel);
    void removeLabel(LSAType lsaType, std::string oldLabel);
    bool isModifierKeyword(std::string input);
}; // end class LSASample

#endif   // LSAHEIGHTMODIFIER_INCLUDE

