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
/// @file LSAPoint.cpp  Class LSAPoint, data for the *.LSA file record POS
///                     3-D XYZ coordinate position

#include <string>
#include <ostream>
#include <set>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"

#include "lsaUtils.hpp"
#include "LSAPosC.hpp"

// gnsstk
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Position.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAPosC::NUM_UI_DISPLAY_COLUMNS = 5;

void LSAPosC::setCovariance(double cxx, double cxy, double cxz, double cyy, double cyz, double czz)
{
    covxx = cxx;
    covxy = cxy;
    covxz = cxz;
    covyy = cyy;
    covyz = cyz;
    covzz = czz;

    hasCovariance = true;

    fixedState = LSAFixedState::CONSTRAINED;

}


//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAPosC::fromString(const string& line)
{
    if (line.empty()) return;

    modifiers.clearModifiers();

    int n, index;
    vector<string> F;
    std::deque<std::string> optionalParms;

    string str(line);                // copy const line
    stripTrailing(str,"\n");
    stripTrailing(str,"\r");
    stripTrailing(str," ");
    stripLeading(str," ");

    // Steal the General Notes before continuing.
    textNotes = QString::fromStdString(extractXMLFromString(str, RecordTags::TextNotes, true));
    // split into fields
    F = splitWithDoubleQuotes(str,' ');
    n = F.size();

    //search for modifier tags and remove them from F
    vector<string>::iterator vscaKeywordIndex = std::find( F.begin(), F.end(), string("VSCA") );
    if(vscaKeywordIndex!=F.end())
    {
        index = vscaKeywordIndex-F.begin();

        if (index == n -1)
        {
            std::string message = "Parse Warning - " + getRecType().asString() + " " + label + " has invalid VSCA.";
            parseWarnings.push_back(message);
            F.pop_back();
        }
        else
        {
            std::string secondParm = F.at(index+1); // make a copy so upperCase won't change the value in F
            bool vscaValueUsed = (gnsstk::StringUtils::upperCase(secondParm) == "VALUE");

            if ( !vscaValueUsed )
            {
                optionalParms.push_back(F.at(index));
                optionalParms.push_back(F.at(index+1));

                F.pop_back();
                F.pop_back();
            }
            else
            {
                optionalParms.push_back(F.at(index));
                optionalParms.push_back(F.at(index+1));
                optionalParms.push_back(F.at(index+2));

                F.pop_back();
                F.pop_back();
                F.pop_back();
            }
        }



        n = F.size();
    }
    if(optionalParms.size()>0)
    {
        modifiers.parseOptionalParms(optionalParms, LSAType::POSC, label, &parseWarnings);
    }

    if(n < 4 )
    {
        addNumFieldsWarning(LSAType::POSC, label);
        return;
    }

    // Search for optional constraint modifiers and remove them from F
    std::string valueFltConFix;
    std::string valueNEU;

    if (isFltConFix(F[2]) && isNEU(F[3]) )
    {
        valueNEU = F.at(3);
        F.erase(F.begin() + 3);

        valueFltConFix = F.at(2);
        F.erase(F.begin() + 2);
    }
    else if (isFltConFix(F[2]))
    {
        valueFltConFix = F.at(2);
        F.erase(F.begin() + 2);
    }
    else if (isNEU(F[2]))
    {
        valueNEU = F.at(2);
        F.erase(F.begin() + 2);
    }
    n = F.size();
    // Note values for fixConFlt are set below after hasCovariance is parsed

    if (!valueNEU.empty())
    {
        validateString(valueNEU, LSAFieldType::FIXED_STATE, "n,e,u,ne,nu,eu,en,ue,un,neu,nue,enu,eun,uen,une");
    }
    if (!valueFltConFix.empty())
    {
        validateString(valueFltConFix, LSAFieldType::FIXED_STATE, "flt,con,fix");
    }
    setFixedConstraints(valueNEU);

    //POSC OSS_TP1 -2678772.71814 -4524254.27275 3598645.53633 m
    if(n == 6 && isLinearUnit(F[5]))
    {

        label = F[1];

        x = validateDouble(F[2], LSAFieldType::X);
        y = validateDouble(F[3], LSAFieldType::Y);
        z = validateDouble(F[4], LSAFieldType::Z);

        posUnits = F[5];
        hasCovariance = false;

    }
    //POSC VNDP -2678090.37178 -4525437.15272 3597431.88202 m 6.5228955464e-006 0.0 0.0 9.4820697996e-006 0.0 4.2811843741e-004
    else if(n == 12 && isLinearUnit(F[5]))
    {
        label = F[1];

        x = validateDouble(F[2], LSAFieldType::X);
        y = validateDouble(F[3], LSAFieldType::Y);
        z = validateDouble(F[4], LSAFieldType::Z);

        covxx = validateDouble(F[6], LSAFieldType::COVARIANCE);
        covxy = validateDouble(F[7], LSAFieldType::COVARIANCE);
        covxz = validateDouble(F[8], LSAFieldType::COVARIANCE);
        covyy = validateDouble(F[9], LSAFieldType::COVARIANCE);
        covyz = validateDouble(F[10], LSAFieldType::COVARIANCE);
        covzz = validateDouble(F[11], LSAFieldType::COVARIANCE);

        posUnits = F[5];
        hasCovariance = true;

    }
    else
    {
        addNumFieldsWarning(LSAType::POSC, label);
    }

    // set fixedState
    valueFltConFix = gnsstk::StringUtils::upperCase(valueFltConFix);
    if (valueFltConFix.empty() && hasCovariance)
    {
        fixedState = LSAFixedState::CONSTRAINED;
    }
    else if (valueFltConFix == "FLT")
    {
        fixedState = LSAFixedState::FLOATING;
    }
    else if (valueFltConFix == "CON")
    {
        fixedState = LSAFixedState::CONSTRAINED;
    }
    else if (valueFltConFix == "FIX")
    {
        fixedState = LSAFixedState::FIXED;
    }

    return;
}

//------------------------------------------------------------------------------------
// Set the constraints
// @param input string combo of E | N | U to indicate cpnstrained axes
void LSAPosC::setFixedConstraints(string input)
{
    input = gnsstk::StringUtils::upperCase(input);

    // Reset constraints
    isNorthFixed = isEastFixed = isUpFixed = false;

    if ( input.find("N") !=std::string::npos )
    {
        isNorthFixed = true;
    }
    if ( input.find("E") !=std::string::npos )
    {
        isEastFixed = true;
    }
    if ( input.find("U") !=std::string::npos )
    {
        isUpFixed = true;
    }

    return;
}


//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSAPosC::writeLSAString(std::ostream & out) const
{
    std::string comment = isCommented ? "#" : "";

    // FLT-CON-FIX
    out << comment << "POSC " << addQuotes(label);

    if (isNorthFixed && isEastFixed && isUpFixed)
    {
        out << " Fix";
    }
    else if (fixedState == LSAFixedState::FLOATING)
    {
        out << " Flt";
    }
    else if (fixedState == LSAFixedState::CONSTRAINED)
    {
        out << " Con";
    }
    else if (fixedState == LSAFixedState::FIXED)
    {
        out << " Fix";
    }

    // NEU
    std::ostringstream neuOSS;
    if (isNorthFixed)
    {
        neuOSS << "N";
    }
    if (isEastFixed)
    {
        neuOSS << "E";
    }
    if (isUpFixed)
    {
        neuOSS << "U";
    }
    if (!neuOSS.str().empty())
    {
        out << " " << neuOSS.str();
    }

    // x y z units
    int xyzCompPrecision = grabNumDigits(getRecType(), posUnits);
    out << std::fixed << std::setprecision(xyzCompPrecision);
    out << " " << x << " " << y << " " << z << " " << posUnits;

    // covariance
    if (hasCovariance)
    {
        out << " ...\n";
        int w = 18;
        out << std::scientific << std::setprecision(lsa::NUM_DECIMALS_COVARIANCE);
        out << comment << setw(w) << covxx << " " << setw(w) << covxy << " " << setw(w) << covxz << " ...\n";
        out << comment << setw(w) << ""    << " " << setw(w) << covyy << " " << setw(w) << covyz << " ...\n";
        out << comment << setw(w) << ""    << " " << setw(w) << ""    << " " << setw(w) << covzz;
    }

    std::string modString = modifiers.getLSAString();
    if (!modString.empty())
    {
        out << " ...\n";
        out << comment << modString;
        if(isAutogenerated)
        {
            out << " AUTOGEN";
        }
    }
    else 
    {
        if(isAutogenerated)
        {
            out << " ...\n" << "AUTOGEN";
        }
    }
    
    if (!textNotes.isEmpty())
    {
        out << " ...\n";
        // put the text notes at the end of the record.
        out << RecordTags::TextNotes.first + textNotes.toStdString() + RecordTags::TextNotes.second;
    }
    
    out << "\n";

    return;
}

// Get a string to display in a given column in the Project Navigator
// @param the column number to display
// @return the string to display in the UI
std::string LSAPosC::getUIColumnData(int columnNumber) const
{
    std::string retVal = "";

    switch (columnNumber)
    {
    case 0:
        retVal = asTypeString();
        break;
    case 1:
        retVal = getUIName();
        break;
    case 2:
        // SCJ TODO: vsca refactoring needed
        // retVal = gnsstk::StringUtils::asString(groupVSCA * modifiers.getVSCAValue(), 3);
        break;
    case 3:
        retVal = getUIDetails();
        break;
    case 4:
        retVal = modifiers.getUIDetails();
        break;
    }

    return retVal;
}

void LSAPosC::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    modifiers.modifyLabel(lsaType, oldLabel, newLabel);

    return;
}

std::vector<std::string> LSAPosC::getReferencedPositions() const
{
    std::vector<std::string> returnValues;
    returnValues.push_back(label);

    return returnValues;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAPosC::getUIName() const
{
    return label;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAPosC::getUIDetails() const
{
    std::ostringstream oss;
    // fixed
    if ( (isNorthFixed && isEastFixed && isUpFixed) || fixedState == LSAFixedState::FIXED )
    {
        oss << "Fix";
    }
    else
    {
        if (isNorthFixed)
        {
            oss << "N";
        }
        if (isEastFixed)
        {
            oss << "E";
        }
        if (isUpFixed)
        {
            oss << "U";
        }
    }

    // x y z units
    oss << std::fixed << std::setprecision(3);
    oss << " " << x << " " << y << " " << z << " " << posUnits;

    return oss.str();
}

void LSAPosC::getLatLonHeight(double &lon, double &lat, double &height) const
{
    gnsstk::Position position(x,y,z);

    lat = position.getGeodeticLatitude();
    lon = position.getLongitude();
    height = position.getHeight();

    return;
}

bool LSAPosC::referencesModifier(LSAType modifierType, std::string label) const
{
    if      (modifierType == LSAType::VSCA)  return (modifiers.getVSCALabel() == label);
    else if (modifierType == LSAType::UNCR) return (modifiers.getUncrLabel() == label);
    else if (modifierType == LSAType::HGHT)
    {
       return ( modifiers.getHeightFromLabel() == label || modifiers.getHeightToLabel() == label);
    }

    return false;
}

bool LSAPosC::hasValidCovariance() const
{
    if (fixedState == LSAFixedState::FLOATING)
    {
        return true;
    }
    else if( fixedState == LSAFixedState::FIXED)
    {
        return true;
    }
    else if (fixedState == LSAFixedState::CONSTRAINED && hasCovariance &&
        !(isNorthFixed && isEastFixed && isUpFixed) )
    {
        bool covIsPositiveSemiDefinite = isPositiveSemiDefinite(covxx, covxy, covxz, covyy, covyz, covzz);

        if (covIsPositiveSemiDefinite)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}


//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
