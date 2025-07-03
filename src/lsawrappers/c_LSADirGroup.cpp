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
#include <c_LSADirGroup.hpp>
#include <string.h>//why was this needed here, but not for other wrappers?

LSADirGroup* new_LSADirGroup(const char *label, const char *fromStation)
{
    return new LSADirGroup(label, fromStation);
}

void delete_LSADirGroup(LSADirGroup *lsaDirGroup) 
{ 
	delete lsaDirGroup; 
}

void setDirGroupIsReduced(LSADirGroup* lsaDirGroup, bool isReduced)
{
    lsaDirGroup->setIsReduced(isReduced);
}

void setDirGroupUNCRLabel(LSADirGroup* lsaDirGroup, const char *label)
{
    lsaDirGroup->setUncrLabel(label);
}

void setDirGroupVSCALabel(LSADirGroup* lsaDirGroup, const char *label)
{
    lsaDirGroup->setVSCALabel(label);
}

void setDirGroupVSCAValue(LSADirGroup* lsaDirGroup, double value)
{
    lsaDirGroup->setVSCAValue(value);
}

void getLSADirGroupString(LSADirGroup* lsaDirGroup, char *buffer)
{
	strcpy(buffer,lsaDirGroup->getSingleLSAString().c_str()); 
}
