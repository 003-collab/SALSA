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
#include <LSAFieldType.hpp>

const std::string LSAFieldType::Strings[COUNT] =
    {
        std::string("Unknown"),
        std::string("additional sigma"),
        std::string("angle-decimal-degrees"),
        std::string("angle-degrees"),
        std::string("angle-minutes"),
        std::string("angle-seconds"),
        std::string("from centering error"),
        std::string("at centering error"),
        std::string("to centering error"),
        std::string("covariance"),
        std::string("distance"),
        std::string("dx"),
        std::string("dy"),
        std::string("dz"),
        std::string("fixed state"),
        std::string("height"),
        std::string("height difference"),
        std::string("height units"),
        std::string("input point"),
        std::string("latitude-decimal-degrees"),
        std::string("latitude-degrees"),
        std::string("latitude-minutes"),
        std::string("latitude-seconds"),
        std::string("latitude direction"),
        std::string("latitude units"),
        std::string("longitude-decimal-degrees"),
        std::string("longitude-degrees"),
        std::string("longitude-minutes"),
        std::string("longitude-seconds"),
        std::string("longitude-direction"),
        std::string("longitude-units"),
        std::string("PPM"),
        std::string("refraction correction"),
        std::string("sigma"),
        std::string("sigma units"),
        std::string("units"),
        std::string("value"),
        std::string("variance scale factor"),
        std::string("x"),
        std::string("y"),
        std::string("z"),
        std::string("de"),
        std::string("dn"),
        std::string("du"),
        std::string("degrees minutes seconds")
    };

std::string LSAFieldType::getLSAFieldName(Types fieldType)
{
    return Strings[fieldType];
}
