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
#ifndef LSAFIELDTYPE_HPP
#define LSAFIELDTYPE_HPP

#include <string>

class LSAFieldType
{

public:
    enum Types
    {
        UNKNOWN,
        ADD_SIGMA,
        ANGLE_DEC_DEG,
        ANGLE_DEG,
        ANGLE_MIN,
        ANGLE_SEC,
        CENTER_FROM,
        CENTER_AT,
        CENTER_TO,
        COVARIANCE,
        DISTANCE,
        DELTA_X,
        DELTA_Y,
        DELTA_Z,
        FIXED_STATE,
        HEIGHT,
        HEIGHT_DIFF,
        HEIGHT_UNITS,
        INPUT_POINT,
        LAT_DEC_DEG,
        LAT_DEG,
        LAT_MIN,
        LAT_SEC,
        LAT_DIR,
        LAT_UNITS,
        LON_DEC_DEG,
        LON_DEG,
        LON_MIN,
        LON_SEC,
        LON_DIR,
        LON_UNITS,
        PPM,
        REFRACT_CORR,
        SIGMA,
        SIGMA_UNITS,
        UNITS,
        VALUE,
        VAR_FACTOR,
        X,
        Y,
        Z,
        DELTA_E,
        DELTA_N,
        DELTA_U,
        DMS,
        COUNT
    };

    /// Constructor, including empty constructor
    LSAFieldType(Types t = UNKNOWN)
    {
       if(t < 0 || t >= COUNT)
          type = UNKNOWN;
       else
          type = t;
    }

    /// constructor from int
    LSAFieldType(int i)
    {
       if(i < 0 || i >= COUNT)
          type = UNKNOWN;
       else
          type = static_cast<Types>(i);
    }

    std::string getLSAFieldName(Types fieldType);
    std::string asString() const { return Strings[type]; }

private:
    Types type;

    static const std::string Strings[];
};


#endif // LSAFIELDTYPE_HPP
