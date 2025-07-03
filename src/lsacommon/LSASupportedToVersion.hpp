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
#ifndef LSASUPPORTEDTOVERSION_HPP
#define LSASUPPORTEDTOVERSION_HPP

#include "LSAVersion.hpp"

const std::string LSASUPPORTEDTOVERSION_MAJOR = "1";
const std::string LSASUPPORTEDTOVERSION_MINOR = "16";
const std::string LSASUPPORTEDTOVERSION_PATCH = "0";
const std::string LSASUPPORTEDTOVERSION_MAJOR_MINOR_PATCH = LSASUPPORTEDTOVERSION_MAJOR + "." + LSASUPPORTEDTOVERSION_MINOR + "." + LSASUPPORTEDTOVERSION_PATCH;
const std::string LSAVERSION_FILE_CREATED_STRING = "Created by SALSA version";
const std::string LSAVERSION_FILE_STRING = LSAVERSION_FILE_CREATED_STRING + " " + LSAVERSION_MAJOR_MINOR_PATCH + " -- Backward compatible to " + LSASUPPORTEDTOVERSION_MAJOR_MINOR_PATCH;

#endif // LSASUPPORTEDTOVERSION_HPP
