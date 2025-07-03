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
/// @file Point.cpp  Include file for class Point,
///                  3-D XYZ coordinate position for LSA solver

#include <string>
#include <ostream>
#include <math.h>

#include "logstream.hpp"      // TEMP
#include "Vector.hpp"
#include "Point.hpp"

using namespace std;
using namespace gnsstk;

//------------------------------------------------------------------------------------
// Compute the azimuth of the input Point (B) relative to this Point (A),
// computed at the Point A.
// return the azimuth in radians of B relative to A.
// throw any gnsstk::Position exception
double Point::getAzimuth(const Point& B) const throw(Exception)
{
   try {
      // compute difference vector B-A in XYZ
      Vector<double> BmA(3);
      BmA(0) = B.x-x; BmA(1) = B.y-y; BmA(2) = B.z-z;

      // rotate into local (NEU) geodetic coordinates at this Point
      Matrix<double> Rot = getRotation();          // get rotation matrix XYZ->NEU
      Vector<double> neuBmA = Rot * BmA;

      // azimuth is the inverse tangent of (east/north)
      return (::atan2(neuBmA(1),neuBmA(0)));
   }
   catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Compute the radius of curvature of the ellipsoid from this Point
// to the input Point; computed at this Point
// return R the radius of curvature of the line along the ellipsoid this->B
// throw any gnsstk::Position exception
double Point::getRadius(const Point& B) const throw(Exception)
{
   try {
      double azm(getAzimuth(B));          // azimuth of line from here to B
      double M(getMeridianRadius());
      double N(getNormalRadius());
      return ( M*N / (N*::cos(azm)*::cos(azm) + M*::sin(azm)*::sin(azm)));
   }
   catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Compute the vertical angle of the input Point (B) relative to this Point (A),
// computed at the Point A.
// return the vertical angle in radians of B relative to A.
// throw any gnsstk::Position exception
double Point::getVerticalAngleTo(const Point& B) const throw(Exception)
{
    try {
        double d = distanceTo(B);//slant distance
        Vector<double> From(3);
        From(0) = x; From(1) = y; From(2) = z;
        Vector<double> To(3);
        To(0) = B.x; To(1) = B.y; To(2) = B.z;
        Matrix<double> Rot = getRotation(); // get rotation matrix XYZ->NEU
        Vector<double> neuFrom = Rot * From;
        Vector<double> neuTo = Rot * To;
        double D_sqr = (neuTo(0)-neuFrom(0))*(neuTo(0)-neuFrom(0))
               + (neuTo(1)-neuFrom(1))*(neuTo(1)-neuFrom(1));//horz distance squared
        double R = getRadius(B);
        return ::asin((B.getHeight() - getHeight() - D_sqr/(2.0*R))/d);
    }
    catch(Exception& e) { GNSSTK_RETHROW(e); }
}


//------------------------------------------------------------------------------------
