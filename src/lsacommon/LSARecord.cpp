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
                    #include <LSARecord.hpp>

    std::string LSARecord::getSingleLSAString() const
    {
       std::ostringstream oss;
       writeLSAString(oss);
       std::string lsaString = oss.str();
       std::vector<std::string> lines;

       if(lsaString.find("...") != std::string::npos)
       {
           lines = gnsstk::StringUtils::split(lsaString,"\n");
           lsaString = std::string("");
           for(int i=0;i<lines.size();i++)
           {
               gnsstk::StringUtils::stripLeading(lines[i]," ");     // strip leading spaces
               gnsstk::StringUtils::replaceAll(lines[i],"...","");  // strip ...
               if (i > 0)
                   gnsstk::StringUtils::stripLeading(lines[i],'#'); // strip leading # on all lines except first line
               lsaString += lines[i] + std::string(" ");
           }

           // fix inconsistent spacing between covariance matrix elements
           while (lsaString.find("  ") != std::string::npos)
           {
               gnsstk::StringUtils::replaceAll(lsaString, "  ", " ");
           }

           //remove last added space
           lsaString.erase(lsaString.size()-1,1);
       }
       gnsstk::StringUtils::replaceAll(lsaString, "\n", "");

       return lsaString;
    }

    bool LSARecord::hasRefractionCorrection()
    {
       LSAModifier *modifier = getModifiers();
       if (modifier != NULL)
           return modifier->hasRefractionCorrection();
       return false;
    }

    bool LSARecord::isReducedToEllipsoid()
    {
       LSAModifier *modifier = getModifiers();
       if (modifier != NULL)
           return modifier->isReducedToEllipsoid();
       return false;
    }

    bool LSARecord::getCurvCorr()
    {
       LSAModifier *modifier = getModifiers();
       if (modifier != NULL)
           return modifier->getCurvCorr();
       return false;
    }

    bool LSARecord::getOHC()
    {
       LSAModifier *modifier = getModifiers();
       if (modifier != NULL)
           return modifier->getOHC();
       return false;
    }

    void LSARecord::updateValidationMessage(std::string value, LSAFieldType fieldType)
    {
        validationMessageStart.clear();
        std::string fieldString = fieldType.asString();
        validationMessageStart = "Warning - " + fieldString + " (" + value + ")";
    }

    int LSARecord::validateInt(std::string value, LSAFieldType fieldType)
    {
        updateValidationMessage(value, fieldType);

        // check type
        if (!checkInt(value))
            return 0;

        int intValue = gnsstk::StringUtils::asInt(value);

        return intValue;
    }

    int LSARecord::validateIntMin(std::string value, LSAFieldType fieldType, int minValue, bool minIsExclusive)
    {
        updateValidationMessage(value, fieldType);

        // check type
        if (!checkInt(value))
            return 0;

        // check min
        int intValue = gnsstk::StringUtils::asInt(value);
        checkMin(intValue, minValue, minIsExclusive);

        return intValue;
    }

    int LSARecord::validateIntMax(std::string value, LSAFieldType fieldType, int maxValue, bool maxIsExclusive)
    {
        updateValidationMessage(value, fieldType);

        // check type
        if (!checkInt(value))
            return 0;

        // check max
        int intValue = gnsstk::StringUtils::asInt(value);
        checkMax(intValue, maxValue, maxIsExclusive);

        return intValue;
    }

    int LSARecord::validateIntMinMax(std::string value, LSAFieldType fieldType, int minValue, int maxValue, bool minIsExclusive, bool maxIsExclusive)
    {
        updateValidationMessage(value, fieldType);

        // check type
        if (!checkInt(value))
            return 0;

        // check min and max
        int intValue = gnsstk::StringUtils::asInt(value);
        checkMin(intValue, minValue, minIsExclusive);
        checkMax(intValue, maxValue, maxIsExclusive);

        return intValue;
    }
    /*
    bool LSARecord::validateDMSBool(bool isNegative, int Deg, int Min, double Sec, LSAFieldType fieldType, int minValue, int maxValue, bool minIsExclusive, bool maxIsExclusive)
    {
        std::stringstream value;

        if(isNegative)
            value << "-";
        value << Deg << " " << Min << "' " << std::fixed << std::setprecision(lsa::NUM_DECIMALS_ANGLE_POSITION_SOA) << Sec << "\"";//could be either POSITION or MEASUREMENT, going with most precision

        updateValidationMessage(value.str(), fieldType);
        double decDeg = (double)Deg + (double)Min/60. + Sec/3600.;
        if(isNegative)
            decDeg *= -1.;
        checkMin(decDeg, (double)minValue, minIsExclusive);
        checkMax(decDeg, (double)maxValue, maxIsExclusive);
        if(hasParseWarnings())
            return false;
        else
            return true;
    }
    */
    bool LSARecord::validateDMSBool(bool isNegative, int Deg, int Min, double Sec, double minValue, double maxValue, std::vector<std::string> *warningsVector)
    {
        std::stringstream message;
        std::string sign("");

        if(isNegative)
            sign = "-";
        message << "Parse Warning - " << sign << Deg << " " << Min << "' " << std::fixed << std::setprecision(getAngularPositionPrecisionSOA()) << Sec << "\"";//could be either POSITION or MEASUREMENT, going with most precision
        message << " is out of bounds (" << std::setprecision(getAngularMeasurementPrecisionSOA()) << minValue << ", " << maxValue << ").";

        double decDeg = (double)Deg + (double)Min/60. + Sec/3600.;
        if(isNegative)
            decDeg *= -1.;

        // Check the min value
        if ((decDeg < minValue) || (decDeg > maxValue))
        {
            if(warningsVector != NULL)
                warningsVector->push_back(message.str());
            return false;
        }
        else
            return true;
    }

    int LSARecord::validateIntBool(bool & isNegative, std::string value, LSAFieldType fieldType, int minValue, int maxValue, bool minIsExclusive, bool maxIsExclusive)
    {
        updateValidationMessage(value, fieldType);


        // check type
        if (!checkInt(value))
            return 0;

        // check min and max
        int intValue = gnsstk::StringUtils::asInt(value);
        checkMin(intValue, minValue, minIsExclusive);
        checkMax(intValue, maxValue, maxIsExclusive);

        // If the value is negative do the following:
        // 1) Set isNegative = true;
        // 2) always return a positive value for intValue
        isNegative = (value.at(0) == '-');
        intValue = ::abs(intValue);

        return intValue;
    }

    double LSARecord::validateDouble(std::string value, LSAFieldType fieldType)
    {
        updateValidationMessage(value, fieldType);

        // check type
        if (!checkDouble(value))
            return 0;

        double doubleValue = gnsstk::StringUtils::asDouble(value);

        return doubleValue;
    }

    double LSARecord::validateDoubleMin(std::string value, LSAFieldType fieldType, double minValue, bool minIsExclusive)
    {
        updateValidationMessage(value, fieldType);

        // check type
        if (!checkDouble(value))
            return 0;

        // check min and max
        double doubleValue = gnsstk::StringUtils::asDouble(value);
        checkMin(doubleValue, minValue, minIsExclusive);

        return doubleValue;
    }

    double LSARecord::validateDoubleMax(std::string value, LSAFieldType fieldType, double maxValue, bool maxIsExclusive)
    {
        updateValidationMessage(value, fieldType);

        // check type
        if (!checkDouble(value))
            return 0;

        // check min and max
        double doubleValue = gnsstk::StringUtils::asDouble(value);
        checkMax(doubleValue, maxValue, maxIsExclusive);

        return doubleValue;
    }

    double LSARecord::validateDoubleMinMax(std::string value, LSAFieldType fieldType, double minValue, double maxValue, bool minIsExclusive, bool maxIsExclusive)
    {
        updateValidationMessage(value, fieldType);

        // check type
        if (!checkDouble(value))
            return 0;

        // check min and max
        double doubleValue = gnsstk::StringUtils::asDouble(value);
        checkMin(doubleValue, minValue, minIsExclusive);
        checkMax(doubleValue, maxValue, maxIsExclusive);

        return doubleValue;
    }

    std::string LSARecord::validateString(std::string value, LSAFieldType fieldType, std::string allowedValues)
    {
        updateValidationMessage(value, fieldType);

        std::vector<std::string> values = gnsstk::StringUtils::split(allowedValues,',');

        bool valueIsAllowed = false;
        for (int i = 0; i< values.size(); ++i)
        {
            std::string allowedValue = gnsstk::StringUtils::strip(values[i]," ");
            std::string testValue = value;
            if (gnsstk::StringUtils::upperCase(allowedValue) == gnsstk::StringUtils::upperCase(testValue))
            {
                valueIsAllowed = true;
                break;
            }
        }

        if (!valueIsAllowed)
        {
            std::string possibleValues;

            for (int i = 0; i< values.size(); ++i)
            {
                possibleValues += values[i] + " ";
            }
            possibleValues = possibleValues.substr(0, possibleValues.size()-1); // trim trailing space

            parseWarnings.push_back(validationMessageStart + " must be one of (" + possibleValues + ").");
        }

        return value;
    }


    bool LSARecord::validateCovariance(double aa, double ab, double ac, double bb, double bc, double cc)
    {
        validationMessageStart = "Parse Warning - " + getRecType().asString() + " " + getLabel() + " covariance";

        bool covarianceIsPositiveDeinite = isPositiveSemiDefinite(aa, ab, ac, bb, bc, cc);
        if (!covarianceIsPositiveDeinite)
            parseWarnings.push_back(validationMessageStart + " must be positive semi-definite.");

        return covarianceIsPositiveDeinite;
    }

    void LSARecord::addNumFieldsWarning(LSAType lsaType, std::string label)
    {
        updateValidationMessage(label, LSAFieldType::UNKNOWN);
        parseWarnings.push_back("Parse Warning - " + lsaType.asString() + " record (" + label + ") has missing or extra field.");
    }

    double LSARecord::validateVSCA(std::string input, LSAType lsaType, std::string recordLabel, std::vector<std::string> *warningsVector)
    {
        std::string messageStart = "Parse Warning - " + lsaType.asString() + " " + recordLabel + " VSCA";

        // Check that the value is a number
        bool isDouble;
        QString doubleString = QString::fromStdString(input);
        double value = doubleString.toDouble(&isDouble);
        if (!isDouble)
        {
            warningsVector->push_back(messageStart + " is not a number.");
            return 0.0;
        }

        // Check that the value is positive
        if (value <= 0)
            warningsVector->push_back(messageStart + " must be positive.");

        return value;
    }


    double LSARecord::validateHFROM(std::string input, LSAType lsaType, std::string recordLabel, std::vector<std::string> *warningsVector)
    {
        std::string messageStart = "Parse Warning - " + lsaType.asString() + " " + recordLabel + " HFROM ";

        // Check that the value is a number
        bool isDouble;
        QString doubleString = QString::fromStdString(input);
        double value = doubleString.toDouble(&isDouble);
        if (!isDouble)
        {
            warningsVector->push_back(messageStart + input + " is not a number.");
            return 0.0;
        }

        return value;
    }

    double LSARecord::validateHTO(std::string input, LSAType lsaType, std::string recordLabel, std::vector<std::string> *warningsVector)
    {
        std::string messageStart = "Parse Warning - " + lsaType.asString() + " " + recordLabel + " HTO ";

        // Check that the value is a number
        bool isDouble;
        QString doubleString = QString::fromStdString(input);
        double value = doubleString.toDouble(&isDouble);
        if (!isDouble)
        {
            warningsVector->push_back(messageStart + input + " is not a number.");
            return 0.0;
        }

        return value;
    }


    double LSARecord::validateRefract(std::string input, LSAType lsaType, std::string recordLabel, std::vector<std::string> *warningsVector)
    {
        std::string messageStart = "Parse Warning - " + lsaType.asString() + " " + recordLabel + " REFRACT ";

        // Check that the value is a number
        bool isDouble;
        QString doubleString = QString::fromStdString(input);
        double value = doubleString.toDouble(&isDouble);
        if (!isDouble)
        {
            warningsVector->push_back(messageStart + input + " is not a number.");
            return 0.0;
        }

        // Check the min value
        if (value < -5.0)
            warningsVector->push_back(messageStart + " must be greater than -5.0.");

        // Check the min value
        if (value > 20.0)
            warningsVector->push_back(messageStart + " must be less than 20.0.");

        return value;
    }
