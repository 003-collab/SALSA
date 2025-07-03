'''
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
'''
import numpy as np
from enum import Enum
import math

class FixedType(Enum):
    Unknown = 0
    FLOATING = 1
    CONSTRAINED = 2
    FIXED = 3
    count = 4

def decDegToDMS(degrees, precision):
    if degrees < 0:
        sign = -1
    else:
        sign = 1

    D = int(abs(degrees))
    M = int((abs(degrees) - abs(D))*60)
    S = float(abs(degrees)-float(abs(D))-(float(M)/60.0))*3600.0

    if("{0:.{1}f}".format(S, precision) == "{0:.{1}f}".format(float(60), precision)):
        S = 0
        M+=1
    if(M == 60):
        M = 0
        D+=1

    if sign == -1:
        if D==0:
            D="-0"
        else:
            D=-1*D

    return [D,M,S]


def eastLongitudeToWest(eastLongitude):
    if math.fabs(float(eastLongitude)) < 1.0e-9:
        return 0.0
    else:
        return 360 - eastLongitude


def northLatitudeToSouth(northLatitude):
    value = -northLatitude
    return value


def under180(degrees):
    if degrees <= 180:
        return degrees
    else:
        return degrees - 360

def rmNegZero(number, precision):
    return round(float(number), precision) + 0

def formatInt(numpyData):
    if numpyData == "-0":
        return numpyData
    else:
        return "{0:1d}".format(numpyData)


def formatFloat(numpyData, precision):
    num = float(numpyData)
    num = round(num, precision) + 0 #Add zero to change any -0.0 to 0.0
    return "{0:.{1}f}".format(num, precision)
