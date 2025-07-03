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
/// @file LSAHeight.cpp for class LSAHeight

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "lsaUtils.hpp"
#include "LSAHeight.hpp"
#include "LSAModifier.hpp"
#include "LSAVarianceScaling.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;


// Public Methods


std::string LSAModifier::getLSAString() const
{
    std::ostringstream oss;

    // Height From
    if (hasFromHeight() )
    {
        oss << " HFROM ";

        if (heightFromIsNumeric)
        {
            oss << numericFrom << " " << unitsFrom << " " << sigmaFrom << " " << sigmaUnitsFrom;
        }
        else
        {
            oss << addQuotes(labelFrom);
        }
    }

    // Height To
    if (hasToHeight() )
    {
        oss << " HTO ";

        if (heightToIsNumeric)
        {
            oss << numericTo << " " << unitsTo << " " << sigmaTo << " " << sigmaUnitsTo;
        }
        else
        {
            oss << addQuotes(labelTo);
        }
    }

    // UNCR
    if ( hasUNCRCorrection() )
    {
        oss << " UNCR " << addQuotes(uncrLabel);
    }

    // REFRACT
    if ( hasRefract)
    {
        oss << " REFRACT " << refractCorr;
    }

    // Geoid
    if ( isReducedToEllipsoid() )
    {
        oss << " REDUCED";
    }

    // VSCA
    if ( hasVSCACorrection() )
    {
        if (vscaIsNumeric)
        {
            oss << " VSCA VALUE " << numericVSCA;
        }
        else
        {
            oss << " VSCA " << addQuotes(vscaLabel);
        }

    }

    // CURV
    if (curvCorrectionNeeded)
    {
        oss << " CURV";
    }

    // OHC
    if (ohcNeeded)
    {
        oss << " OHC";
    }

    return oss.str();
}

// Parse modifiers from a string
// @param input ine input vector of strings
// @return true on a succesful parse
bool LSAModifier::parseOptionalParms(std::deque<std::string> inputParms, LSAType recordType, std::string recordLabel, std::vector<std::string> *warningsVector)
{
    optionalParms = inputParms;

    bool goodParse = true;
    size_t oldParmCount = optionalParms.size() + 1; // just an initial value to get into the loop below

    while (goodParse == true && 0 < optionalParms.size() && optionalParms.size() < oldParmCount)
    {
        oldParmCount = optionalParms.size();

        std::string firstWord = gnsstk::StringUtils::upperCase(optionalParms.at(0));

        if ("HFROM" == firstWord && supportsHeightFrom )
        {
            goodParse = ParseHeightModifier(recordType, recordLabel, warningsVector);
        }
        else if ("HTO" == firstWord && supportsHeightTo)
        {
            goodParse = ParseHeightModifier(recordType, recordLabel, warningsVector);
        }
        else if("UNCR" == firstWord && supportsUNCR)
        {
            goodParse = ParseUncrModifier(recordType, recordLabel, warningsVector);
        }
        else if("REFRACT" == firstWord && supportsRefract)
        {
            goodParse = ParseRefractionModifier(recordType, recordLabel, warningsVector);
        }
        else if("REDUCED" == firstWord && supportsGeoid)
        {
            goodParse = ParseReducedModifier();
        }
        else if("VSCA" == firstWord && supportsVSCA)
        {
            goodParse = ParseVSCAModifier(recordType, recordLabel, warningsVector);
        }
        else if("CURV" == firstWord && supportsCurv)
        {
            goodParse = ParseCurvatureModifier();
        }
        else if("OHC" == firstWord && supportsOHC)
        {
            goodParse = ParseOHCModifier();
        }
        else
        {
            std::string message = recordType.asString() + " " + recordLabel + " has invalid keyword: " + firstWord + ".";
            warningsVector->push_back(message);
        }
    }

    return goodParse;
}

/// Sets the height from modifier values
/// @param label The height record name
/// @param value The height value
/// @param units The height units (m,cm,ft)
/// @param sigma The height uncertainty
/// @param sigmaUnits The height uncertainty units (m,cm,ft)
void LSAModifier::setHeightFromData(std::string label, double value, std::string units, double sigma, std::string sigmaUnits)
{
    if      (lsa::CONTROLVALUE_NONE == label)
    {
       clearHeightFrom();
    }
    else if (lsa::CONTROLVALUE_VALUE == label)
    {
       setHeightFromValue(value, units);
       setHeightFromSigma(sigma, sigmaUnits);
    }
    else
    {
       setHeightFromLabel(label);
    }

    return;
}

/// Sets the height from modifier values
/// @param label The height record name
/// @param value The height value
/// @param units The height units (m,cm,ft)
void LSAModifier::setHeightFromData(std::string label, double value, std::string units)
{
    if      (lsa::CONTROLVALUE_NONE == label)
    {
       clearHeightFrom();
    }
    else if (lsa::CONTROLVALUE_VALUE == label)
    {
       setHeightFromValue(value, units);
       setHeightFromSigma(0.0,"m");
    }
    else
    {
       setHeightFromLabel(label);
    }

    return;
}

void LSAModifier::clearHeightFrom()
{
    hasHeightFrom = false;
    heightFromIsNumeric = false;
    labelFrom = "";

    return;
}

void LSAModifier::setHeightFromLabel(std::string label)
{
    hasHeightFrom = true;
    labelFrom = label;
    heightFromIsNumeric = false;

    return;
}

void LSAModifier::setHeightFromValue(double value, std::string units)
{
    hasHeightFrom = true;
    numericFrom = value;
    unitsFrom = units;
    heightFromIsNumeric = true;

    return;
}

void LSAModifier::setHeightFromSigma(double value, std::string units)
{
    hasHeightFrom = true;
    sigmaFrom = value;
    sigmaUnitsFrom = units;
    heightFromIsNumeric = true;

    return;
}

/// Sets the height to modifier values
/// @param label The height record name
/// @param value The height value
/// @param units The height units (m,cm,ft)
/// @param sigma The height uncertainty
/// @param sigmaUnits The height uncertainty units (m,cm,ft)
void LSAModifier::setHeightToData(std::string label, double value, std::string units, double sigma, std::string sigmaUnits)
{
    if      (lsa::CONTROLVALUE_NONE == label)
    {
       clearHeightTo();
    }
    else if (lsa::CONTROLVALUE_VALUE == label)
    {
       setHeightToValue(value, units);
       setHeightToSigma(sigma, sigmaUnits);
    }
    else
    {
       setHeightToLabel(label);
    }
    return;
}

/// Sets the height to modifier values
/// @param label The height record name
/// @param value The height value
/// @param units The height units (m,cm,ft)
void LSAModifier::setHeightToData(std::string label, double value, std::string units)
{
    if      (lsa::CONTROLVALUE_NONE == label)
    {
       clearHeightTo();
    }
    else if (lsa::CONTROLVALUE_VALUE == label)
    {
       setHeightToValue(value, units);
       setHeightToSigma(0.0, "m");
    }
    else
    {
       setHeightToLabel(label);
    }
    return;
}

void LSAModifier::clearHeightTo()
{
    hasHeightTo = false;
    heightToIsNumeric = false;
    labelTo = "";

    return;
}

void LSAModifier::setHeightToLabel(std::string label)
{
    hasHeightTo = true;
    labelTo = label;
    heightToIsNumeric = false;
    return;
}

void LSAModifier::setHeightToValue(double value, std::string units)
{
    hasHeightTo = true;
    numericTo = value;
    unitsTo = units;
    heightToIsNumeric = true;

    return;
}

void LSAModifier::setHeightToSigma(double value, std::string units)
{
    hasHeightTo = true;
    sigmaTo = value;
    sigmaUnitsTo = units;
    heightToIsNumeric = true;

    return;
}

void LSAModifier::modifyLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    if (oldLabel.empty())
    {
        return;
    }

    if (newLabel.empty())
    {
        removeLabel(lsaType, oldLabel);
    }
    else
    {
        renameLabel(lsaType, oldLabel, newLabel);
    }

    return;
}

void LSAModifier::removeLabel(LSAType lsaType, std::string oldLabel)
{
    if (lsaType == LSAType::HGHT)
    {
        if (labelFrom == oldLabel)
        {
            labelFrom = "";
            hasHeightFrom = false;
        }

        if (labelTo == oldLabel)
        {
            labelTo   = "";
            hasHeightTo = false;
        }
    }
    else if (lsaType == LSAType::UNCR)
    {
        if (uncrLabel == oldLabel)
        {
            uncrLabel = "";
            hasUNCR = false;
        }
    }
    else if (lsaType == LSAType::VSCA)
    {
        if (vscaLabel == oldLabel)
        {
            vscaLabel = "";
            hasVSCA = false;
        }
    }
    else
    {
        GNSSTK_THROW(Exception("Call to LSAModifier::renameLabel with invalid LSAType."))
    }

    return;
}

void LSAModifier::renameLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    if ( oldLabel.empty() || newLabel.empty() )
    {
        return;
    }

    if (lsaType == LSAType::HGHT)
    {
        if (labelFrom == oldLabel) labelFrom = newLabel;
        if (labelTo == oldLabel)   labelTo   = newLabel;
    }
    else if (lsaType == LSAType::UNCR)
    {
        if (uncrLabel == oldLabel) uncrLabel = newLabel;
    }
    else if (lsaType == LSAType::VSCA)
    {
        if (vscaLabel == oldLabel) vscaLabel = newLabel;
    }
    else
    {
        GNSSTK_THROW(Exception("Call to LSAModifier::renameLabel with invalid LSAType."))
    }

    return;
}

void  LSAModifier::setUncrLabel(std::string label)
{
    if (lsa::CONTROLVALUE_NONE == label)
    {
        clearUncrLabel();
    }
    else
    {
        hasUNCR = true;
        uncrLabel = label;
    }
}

void LSAModifier::clearUncrLabel()
{
    hasUNCR = false;
    uncrLabel = "";

    return;
}

void LSAModifier::setVSCAData(std::string label, double value)
{

    if      (lsa::CONTROLVALUE_NONE == label)  clearVSCA();
    else if (lsa::CONTROLVALUE_VALUE == label) setVSCAValue(value);
    else                                  setVSCALabel(label);

    return;
}

void LSAModifier::setVSCAValue(double value)
{
    hasVSCA = true;
    numericVSCA = value;
    vscaIsNumeric = true;
    vscaLabel.clear();

    return;
}

void LSAModifier::setVSCALabel(string label)
{
    hasVSCA = true;
    vscaLabel = label;
    vscaIsNumeric = false;

    return;
}

void LSAModifier::clearVSCA()
{
    hasVSCA = false;
    vscaIsNumeric = false;
    vscaLabel.clear();

    return;
}

// Output as a string (one line) to display in the UI
// @return a string to display in the UI
std::string LSAModifier::getUIDetails() const
{
    std::ostringstream oss;

    if (hasVSCACorrection())
    {
        if (vscaIsNumeric)
        {
            oss << "VSCA:" << numericVSCA << " ";
        }
        else
        {
            oss << "VSCA:" << vscaLabel << " ";
        }
    }

    if (hasHeightFrom)
    {
        oss << "HFROM:";

        if (heightFromIsNumeric)
        {
            oss << std::setprecision(3) << numericFrom << " " << unitsFrom << " " << sigmaFrom << " " << sigmaUnitsFrom;
        }
        else
        {
            oss << labelFrom << " ";
        }
    }

    if (hasHeightTo)
    {
        oss << "HTO:";

       if (heightToIsNumeric)
       {
           oss << std::setprecision(3) << numericTo << " " << unitsTo << " " << sigmaTo << " " << sigmaUnitsTo;
       }
       else
       {
           oss << labelTo << " ";
       }
    }

    if (hasUNCRCorrection())
    {
        oss << "UNCR:" << uncrLabel << " ";
    }

    return oss.str();
}

// Private Methods


// Parse height modifiers applied to LSARecord subclasses.
// @return true on successful parse
bool LSAModifier::ParseHeightModifier(LSAType recordType, std::string recordLabel, std::vector<std::string> *warningsVector)
{
    bool goodParse = false;

    if (optionalParms.size() == 1 && optionalParms.at(0) == "HFROM")
    {
        // HFROM keyword without any actual data
        warningsVector->push_back("Invalid HFROM.");
    }
    else if (optionalParms.size() == 1 && optionalParms.at(0) == "HTO")
    {
        // HTO keyword without any actual data
        warningsVector->push_back("Invalid HTO.");
    }
    // numeric value
    else if ( optionalParms.size() == 2 || isModifierKeyword(optionalParms.at(2)))
    {
        if ( optionalParms.at(0) == "HFROM" )
        {
            hasHeightFrom = true;
            heightFromIsNumeric = false;
            labelFrom = optionalParms.at(1);
        }
        else if ( optionalParms.at(0) == "HTO" )
        {
            hasHeightTo = true;
            heightToIsNumeric = false;
            labelTo = optionalParms.at(1);
        }
        optionalParms.pop_front();
        optionalParms.pop_front();

        goodParse = true;
    }
    // label that refers to height record
    else
    {
        if (optionalParms.size() >= 5 && isLinearUnit(optionalParms.at(2)) && isLinearUnit(optionalParms.at(4)))
        {
            if ( optionalParms.at(0) == "HFROM" )
            {
                hasHeightFrom = true;
                heightFromIsNumeric = true;
                numericFrom = LSARecord::validateHFROM(optionalParms.at(1), recordType, recordLabel, warningsVector);
                unitsFrom = optionalParms.at(2);
                sigmaFrom = LSARecord::validateHFROM(optionalParms.at(3), recordType, recordLabel, warningsVector);
                sigmaUnitsFrom = optionalParms.at(4);
            }
            else if ( optionalParms.at(0) == "HTO" )
            {
                hasHeightTo = true;
                heightToIsNumeric = true;
                numericTo = LSARecord::validateHTO(optionalParms.at(1), recordType, recordLabel, warningsVector);
                unitsTo = optionalParms.at(2);
                sigmaTo = LSARecord::validateHTO(optionalParms.at(3), recordType, recordLabel, warningsVector);
                sigmaUnitsTo = optionalParms.at(4);
            }
            optionalParms.pop_front();
            optionalParms.pop_front();
            optionalParms.pop_front();
            optionalParms.pop_front();
            optionalParms.pop_front();
            goodParse = true;
        }
        else if (optionalParms.size() >= 3 && isLinearUnit(optionalParms.at(2)))
        {
            if ( optionalParms.at(0) == "HFROM" )
            {
                hasHeightFrom = true;
                heightFromIsNumeric = true;
                numericFrom = LSARecord::validateHFROM(optionalParms.at(1), recordType, recordLabel, warningsVector);
                unitsFrom = optionalParms.at(2);
                sigmaFrom = 0.0;
                sigmaUnitsFrom = "m";
            }
            else if ( optionalParms.at(0) == "HTO" )
            {
                hasHeightTo = true;
                heightToIsNumeric = true;
                numericTo = LSARecord::validateHTO(optionalParms.at(1), recordType, recordLabel, warningsVector);
                unitsTo = optionalParms.at(2);
                sigmaTo = 0.0;
                sigmaUnitsTo = "m";
            }
            optionalParms.pop_front();
            optionalParms.pop_front();
            optionalParms.pop_front();

            goodParse = true;
        }
        else
        {
            std::string message = recordType.asString() + " " + recordLabel + " has invalid HFROM or HTO.";
            warningsVector->push_back(message);
        }
    }

    return goodParse;
}

// Parse sigmas applied to LSARecord subclasses.
// @return true on successful parse
bool LSAModifier::ParseUncrModifier(LSAType recordType, std::string recordLabel, std::vector<std::string> *warningsVector)
{
    bool goodParse = true;

    if (hasUNCR)
    {
        // can't have multiple sigmas for one record
        goodParse = false;
    }

    if (optionalParms.size() > 1)
    {
        hasUNCR = true;
        uncrLabel = optionalParms.at(1);
        optionalParms.pop_front();
        optionalParms.pop_front();
    }
    else
    {
        // UNCR keyword with no label
        std::string message = recordType.asString() + " " + recordLabel + " has invalid UNCR.";
        warningsVector->push_back(message);
    }

    return goodParse;
}

// Parse refraction modifier applied to LSARecord subclasses.
// @return true on successful parse
bool LSAModifier::ParseRefractionModifier(LSAType recordType, std::string recordLabel, std::vector<std::string> *warningsVector)
{
    bool goodParse = true;

    if (hasRefract)
    {
        // can't have multiple REFRACT records
        goodParse = false;
    }

    hasRefract = true;
    if (optionalParms.size() < 2 )
    {
        std::string message = recordType.asString() + " " + recordLabel + " has invalid REFRACT.";
        warningsVector->push_back(message);
    }
    else
    {
        refractCorr = LSARecord::validateRefract(optionalParms.at(1), recordType, recordLabel, warningsVector );
        optionalParms.pop_front();
        optionalParms.pop_front();
    }

    return goodParse;
}

// Parse geoid reduction modifier applied to LSARecord subclasses.
// @return true on successful parse
bool LSAModifier::ParseReducedModifier()
{
    isReduced = true;
    optionalParms.pop_front();

    return true;
}

// Parse VSCA modifier applied to LSARecord subclasses.
// @return true on successful parse
bool LSAModifier::ParseVSCAModifier(LSAType recordType, std::string recordLabel, std::vector<std::string> *warningsVector)
{
    bool goodParse = true;

    if (hasVSCA)
    {
        // can't have multiple VSCA records
        goodParse = false;
    }

    if (optionalParms.size() > 2 && optionalParms.at(1) == "VALUE")
    {
        hasVSCA = true;
        vscaIsNumeric = true;
        numericVSCA = LSARecord::validateVSCA(optionalParms.at(2), recordType, recordLabel, warningsVector);
        optionalParms.pop_front();
        optionalParms.pop_front();
        optionalParms.pop_front();
    }
    else if(optionalParms.size() > 1)
    {
        if (optionalParms.at(1) == "VALUE")
        {
            // VSCA VALUE with missing numeric term
            std::string message = recordType.asString() + " " + recordLabel + " has missing numeric VSCA.";
            warningsVector->push_back(message);
        }
        hasVSCA = true;
        vscaIsNumeric = false;
        vscaLabel = optionalParms.at(1);
        optionalParms.pop_front();
        optionalParms.pop_front();
    }
    else
    {
        std::string message = recordType.asString() + " " + recordLabel + " has invalid VSCA.";
        warningsVector->push_back(message);
    }

    return goodParse;
}

// Parse geoid curvature modifier applied to LSARecord subclasses.
// @return true on successful parse
bool LSAModifier::ParseCurvatureModifier()
{
    curvCorrectionNeeded = true;
    optionalParms.pop_front();

    return true;
}

// Parse orthometric height correction modifier applied to LSARecord subclasses.
// @return true on successful parse
bool LSAModifier::ParseOHCModifier()
{
    ohcNeeded = true;
    optionalParms.pop_front();

    return true;
}

void LSAModifier::clearModifiers()
{
    clearHeightFrom();
    clearHeightTo();
    hasRefract = false;
    clearUncrLabel();
    clearVSCA();
    isReduced = false;
    ohcNeeded = false;
    curvCorrectionNeeded = false;

    return;
}

bool LSAModifier::isModifierKeyword(std::string input)
{
    bool isModifier = false;

    if      (gnsstk::StringUtils::upperCase(input) == "HTO") isModifier = true;
    else if (gnsstk::StringUtils::upperCase(input) == "HFROM") isModifier = true;
    else if (gnsstk::StringUtils::upperCase(input) == "UNCR") isModifier = true;
    else if (gnsstk::StringUtils::upperCase(input) == "REFRACT") isModifier = true;
    else if (gnsstk::StringUtils::upperCase(input) == "REDUCED") isModifier = true;
    else if (gnsstk::StringUtils::upperCase(input) == "VSCA") isModifier = true;
    else if (gnsstk::StringUtils::upperCase(input) == "CURV") isModifier = true;
    else if (gnsstk::StringUtils::upperCase(input) == "OHC")  isModifier = true;

    return isModifier;
}

std::string LSAModifier::getModifierLabel(ModifierKeyword keyword) const
{
    switch (keyword)
    {
        case MODKEY_HEIGHTFROM: return labelFrom;
        case MODKEY_HEIGHTTO:   return labelTo;
        case MODKEY_UNCR:       return uncrLabel;
        case MODKEY_VSCA:       return vscaLabel;
    }

    return "";
}

bool LSAModifier::supportsModifierControl(LSAType lsaType, ModifierKeyword keyword)
{
    if (lsaType == LSAType::POSC || lsaType == LSAType::POSG)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return false;
            case MODKEY_HEIGHTTO:   return false;
            case MODKEY_UNCR:       return false;
            case MODKEY_REFRACT:    return false;
            case MODKEY_REDUCED:    return false;
            case MODKEY_VSCA:       return true;
            case MODKEY_CURV:       return false;
            case MODKEY_OHC:        return false;
        }
    }
    else if (lsaType == LSAType::DXYZ)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return true;
            case MODKEY_HEIGHTTO:   return true;
            case MODKEY_UNCR:       return true;
            case MODKEY_REFRACT:    return false;
            case MODKEY_REDUCED:    return false;
            case MODKEY_VSCA:       return true;
            case MODKEY_CURV:       return false;
            case MODKEY_OHC:        return false;
        }
    }
    else if (lsaType == LSAType::DIST)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return true;
            case MODKEY_HEIGHTTO:   return true;
            case MODKEY_UNCR:       return true;
            case MODKEY_REFRACT:    return false;
            case MODKEY_REDUCED:    return false;
            case MODKEY_VSCA:       return true;
            case MODKEY_CURV:       return false;
            case MODKEY_OHC:        return false;
        }
    }
    else if (lsaType == LSAType::HANG)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return true;
            case MODKEY_HEIGHTTO:   return true;
            case MODKEY_UNCR:       return true;
            case MODKEY_REFRACT:    return false;
            case MODKEY_REDUCED:    return true;
            case MODKEY_VSCA:       return true;
            case MODKEY_CURV:       return false;
            case MODKEY_OHC:        return false;
        }
    }
    else if (lsaType == LSAType::ZANG)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return true;
            case MODKEY_HEIGHTTO:   return true;
            case MODKEY_UNCR:       return true;
            case MODKEY_REFRACT:    return true;
            case MODKEY_REDUCED:    return true;
            case MODKEY_VSCA:       return true;
            case MODKEY_CURV:       return false;
            case MODKEY_OHC:        return false;
        }
    }
    else if (lsaType == LSAType::VANG)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return true;
            case MODKEY_HEIGHTTO:   return true;
            case MODKEY_UNCR:       return true;
            case MODKEY_REFRACT:    return true;
            case MODKEY_REDUCED:    return true;
            case MODKEY_VSCA:       return true;
            case MODKEY_CURV:       return false;
            case MODKEY_OHC:        return false;
        }
    }
    else if (lsaType == LSAType::AZIM)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return true;
            case MODKEY_HEIGHTTO:   return true;
            case MODKEY_UNCR:       return true;
            case MODKEY_REFRACT:    return false;
            case MODKEY_REDUCED:    return true;
            case MODKEY_VSCA:       return true;
            case MODKEY_CURV:       return false;
            case MODKEY_OHC:        return false;
        }
    }
    else if (lsaType == LSAType::HDIF)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return false;
            case MODKEY_HEIGHTTO:   return false;
            case MODKEY_UNCR:       return true;
            case MODKEY_REFRACT:    return true;
            case MODKEY_REDUCED:    return true;
            case MODKEY_VSCA:       return true;
            case MODKEY_CURV:       return true;
            case MODKEY_OHC:        return true;
        }
    }
    else if (lsaType == LSAType::DGRP)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return false;
            case MODKEY_HEIGHTTO:   return false;
            case MODKEY_UNCR:       return true;
            case MODKEY_REFRACT:    return false;
            case MODKEY_REDUCED:    return true;
            case MODKEY_VSCA:       return true;
            case MODKEY_CURV:       return false;
            case MODKEY_OHC:        return false;
        }
    }
    else if (lsaType == LSAType::HDIR)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return false;
            case MODKEY_HEIGHTTO:   return true;
            case MODKEY_UNCR:       return false;
            case MODKEY_REFRACT:    return false;
            case MODKEY_REDUCED:    return false;
            case MODKEY_VSCA:       return false;
            case MODKEY_CURV:       return false;
            case MODKEY_OHC:        return false;
        }
    }
    else if (lsaType == LSAType::INCLUDE)
    {
        switch (keyword)
        {
            case MODKEY_HEIGHTFROM: return false;
            case MODKEY_HEIGHTTO:   return false;
            case MODKEY_UNCR:       return false;
            case MODKEY_REFRACT:    return false;
            case MODKEY_REDUCED:    return false;
            case MODKEY_VSCA:       return true;
            case MODKEY_CURV:       return false;
            case MODKEY_OHC:        return false;
        }
    }

    return false;
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
