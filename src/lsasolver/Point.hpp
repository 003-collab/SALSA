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
/// @file Point.hpp  Include file for class Point,
///                  3-D XYZ coordinate position for LSA

#ifndef LSA_POINT_DATA_INCLUDE
#define LSA_POINT_DATA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <string>
#include "Position.hpp"
#include "SunEarthSatGeometry.hpp"
// lsa
#include "MatrixVector.hpp"
#include "DATPoint.hpp"
#include "lsaUtils.hpp"

/// Class Point encapsulates data for a 3-D coordinate position and includes
/// a label and whether or not it is considered fixed, and constraints.
class Point : public DATPoint {
public:
// member data

   // parameters of local gravity
   double und;                ///< the undulation at the position (m)
   double dovN;               ///< deflection of the vertical towards north (SoA)
   double dovE;               ///< deflection of the vertical towards east (SoA)

// member functions
   /// empty constructor
   Point(void) { }

   /// Constructor from DATPoint
   Point(DATPoint& dat) : DATPoint(dat), und(0.0),dovN(0.0),dovE(0.0)
      { }

   /// destructor
   virtual ~Point() { }

   /// constructor from a label only. fixtype is set to unknown, coordinates = 0,0,0
   /// @param lab  string containing the label for this Point
   Point(std::string lab) : DATPoint(lab), und(0.0),dovN(0.0),dovE(0.0)
      { }

   /// constructor from label xyz coordinates and optional covariance (default 0);
   /// fixtype is set to 1 (estimate)
   /// @param lab  string containing the label for this Point
   /// @param xin  double X-coordinate for this Point
   /// @param yin  double Y-coordinate for this Point
   /// @param zin  double Z-coordinate for this Point
   /// @param sxx double the X,X component of the covariance matrix
   /// @param sxy double the X,Y component of the covariance matrix
   /// @param sxz double the X,Z component of the covariance matrix
   /// @param syy double the Y,Y component of the covariance matrix
   /// @param syz double the Y,Z component of the covariance matrix
   /// @param szz double the Z,Z component of the covariance matrix
   Point(std::string lab, double xin, double yin, double zin,
                          double sxx, double sxy, double sxz,
                          double syy, double syz, double szz)
      : DATPoint(lab, xin, yin, zin, sxx, sxy, sxz, syy, syz, szz),
                 und(0.0),dovN(0.0),dovE(0.0)
      { }

   /// Compute geodetic latitude, longitude and ellipsoid height at this Point
   /// @param lat return North geodetic latitude of this position in degrees
   /// @param lon return East longitude of this position in degrees
   /// @param hgt return ellipsoid height of this position in meters
   /// @param inDeg if true return angles in degrees, else radians (default)
   /// @throw any gnsstk::Position exception
   void getLatLonHeight(double& lat, double& lon, double& hgt, bool inDeg=false) const
      throw(gnsstk::Exception)
   {
      try {
         gnsstk::Position p;
         p.setECEF(x,y,z);
         lat = p.getGeodeticLatitude();
         lon = p.getLongitude();
         if(!inDeg) {
            lat *= DEG_TO_RAD;
            lon *= DEG_TO_RAD;
         }
         hgt = p.getHeight();
      }
      catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
   }

   /// Compute astronomic latitude, longitude and orthometric height at this Point
   /// @param lat return North astronomic latitude of this position in degrees
   /// @param lon return East astronomic longitude of this position in degrees
   /// @param hgt return orthometric height of this position in meters
   /// @param inDeg if true return angles in degrees, else radians (default)
   /// @throw any gnsstk::Position exception
   void getAstroLatLonHeight(double& lat, double& lon, double& hgt, bool inDeg=false) 
      const throw(gnsstk::Exception)
   {
      try {
         gnsstk::Position p;
         p.setECEF(x,y,z);
         lat = p.getGeodeticLatitude();
         lon = p.getLongitude();
         // Astro lon = lon + dovE/cos(lat)
         lon += (dovE/3600/::cos(lat*DEG_TO_RAD));
         // Astro lat = lat + dovN
         lat += (dovN/3600);
         // Remove ambiguity
         if(lat>90) {
            lat = 180 - lat;
            lon += 180;
         } else if(lat<-90) {
            lat = -90 - lat;
            lon += 180;
         }

         if(lon>360) lon-=360;
         else if(lon<0) lon+=360;

         if(!inDeg) {
            lat *= DEG_TO_RAD;
            lon *= DEG_TO_RAD;
         }
         hgt = p.getHeight();
         hgt -= und;
      }
      catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
   }

   /// Compute geodetic latitude North for this point.
   /// NB. Prefer getLatLonHeight for longitude and/or height also.
   /// @return North geodetic latitude of this position in degrees
   /// @param inDeg if true return angles in degrees, else radians (default)
   /// @throw any gnsstk::Position exception
   double getLatitude(bool inDeg=false) const throw(gnsstk::Exception)
   {
      try {
         gnsstk::Position p;
         p.setECEF(x,y,z);
         double lat = p.getGeodeticLatitude();
         if(!inDeg) lat *= DEG_TO_RAD;
         return lat;
      }
      catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
   }

   /// Compute longitude East for this point.
   /// NB. Prefer getLatLonHeight for latitude and/or height also.
   /// @return East longitude of this position in degrees
   /// @param inDeg if true return angles in degrees, else radians (default)
   /// @throw any gnsstk::Position exception
   double getLongitude(bool inDeg=false) const throw(gnsstk::Exception)
   {
      try {
         gnsstk::Position p;
         p.setECEF(x,y,z);
         double lon = p.getLongitude();
         if(!inDeg) lon *= DEG_TO_RAD;
         return lon;
      }
      catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
   }

   /// Compute ellipsoid height for this point.
   /// NB. Prefer getLatLonHeight for longitude and/or latitude also.
   /// @return ellipsoid height of this position in meters
   /// @throw any gnsstk::Position exception
   double getHeight(void) const throw(gnsstk::Exception)
   {
      try {
         gnsstk::Position p;
         p.setECEF(x,y,z);
         return (p.getHeight());
      }
      catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
   }

   gnsstk::Position getPosition(void) const throw(gnsstk::Exception)
   {
      try {
         gnsstk::Position p;
         p.setECEF(x,y,z);
         return p;
      }
      catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
   }

   /// Compute rotation XYZ->NEU (geodetic) at this Point
   /// @return rotation matrix XYZ->NEU at this Point
   /// @throw any gnsstk::Position exception
   gnsstk::Matrix<double> getRotation(void) const throw(gnsstk::Exception)
   {
      try {
         gnsstk::Position p;
         p.setECEF(x,y,z);
         return northEastUp(p);
      }
      catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
   }

   /// Compute the radius of curvature of the meridian at this Point
   /// @return M the radius of curvature of the Meridian
   /// @throw any gnsstk::Position exception
   double getMeridianRadius(void) const throw(gnsstk::Exception)
   {
      try {
         double lat(getLatitude(false));  // latitude in radians
         double fact(eccent * ::sin(lat));
         fact = ::sqrt(1.0 - fact*fact);
         return ( Rearth*(1.0-eccent*eccent) / (fact*fact*fact) );
      }
      catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
   }

   /// Compute the radius of curvature of the normal at this Point
   /// @return N the radius of curvature of the Normal
   /// @throw any gnsstk::Position exception
   double getNormalRadius(void) const throw(gnsstk::Exception)
   {
      try {
         double lat(getLatitude(false));  // latitude in radians
         double fact(eccent * ::sin(lat));
         fact = ::sqrt(1.0 - fact*fact);
         return ( Rearth / fact );
      }
      catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
   }

   /// Compute the azimuth of the input Point (B) relative to this Point (A),
   /// computed at the Point A.
   /// @return the azimuth in radians of B relative to A.
   /// @throw any gnsstk::Position exception
   double getAzimuth(const Point& B) const throw(gnsstk::Exception);

   /// Compute the vertical angle of the input Point (B) relative to this Point (A),
   /// computed at the Point A.
   /// @return the vertical angle in radians of B relative to A.
   /// @throw any gnsstk::Position exception
   double getVerticalAngleTo(const Point& B) const throw(gnsstk::Exception);

   /// Compute the radius of curvature of the ellipsoid from this Point
   /// to the input Point; computed at this Point
   /// Ref. Ghilani paper
   /// @return R the radius of curvature of the line along the ellipsoid this->B
   /// @throw any gnsstk::Position exception
   double getRadius(const Point& B) const throw(gnsstk::Exception);

   /// Compute the distance between this Point and the input Point.
   /// @return the distance
   double distanceTo(const Point& B) const throw(gnsstk::Exception)
   {
      double d = (x-B.x)*(x-B.x) + (y-B.y)*(y-B.y) + (z-B.z)*(z-B.z);
      return ::sqrt(d);
   }

   /// Get orthometric height at this point
   /// @return orthometric height
   double getOrthometricHeight(void) const throw(gnsstk::Exception)
   {
      try {
         return ( getHeight() + und );   
      }
      catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
   }

}; // end class Point

#endif   // LSA_POINT_DATA_INCLUDE

