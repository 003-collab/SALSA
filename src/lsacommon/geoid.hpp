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
/// @file geoid.hpp  Include file for class geoid, data and methods for
///                  importing the geoid and calculating DoV, undulation

// system includes
#include <string>
#include <utility>
#include <vector>
#include "lsah5.hpp"

// GNSSTk
#include "Exception.hpp"
#include "Position.hpp"
#include "Matrix.hpp"

#include "Point.hpp"
#include "LSAPosG.hpp"

#ifndef GEOID_DATA_INCLUDE
#define GEOID_DATA_INCLUDE

//------------------------------------------------------------------------------------
/// Find bounding box of Points, for use with geoid
/// Originally resided in the now-removed lsageoid.*pp files
/// @param Points map<string,Point> of Points to be considered
/// @param latmin on output, minimum in latitude, in degrees
/// @param latmax on output, maximum in latitude, in degrees
/// @param lonmin on output, minimum in longitude, in degrees
/// @param lonmax on output, maximum in longitude, in degrees
void FindBoundingBox(std::map<std::string,Point>& Points,
                   double& latmin, double& latmax,
                   double& lonmin, double& lonmax);

void FindBoundingBox(const std::vector<LSAPosG *> &passedPoints,
                     double &latmin, double &latmax,
                     double &lonmin, double &lonmax);
// First element of pair is latitude, second is longitude
void performComparisons(const std::vector<std::pair<double, double>> &latLonPairs,
                        double &latmin, double &latmax,
                        double &lonmin, double &lonmax);

/// Struct containing data that can be read from the header row
/// of an NGS geoid file
/// defaults are the values for the Geoid12b/Geoid18 model
struct NGSHeaderData{
    double cornerLatNorth = 24.;
    double cornerLonEast = 230.;
    double latStepDeg = 1./60.; // 1-min spacing
    double lonStepDeg = 1./60.; // 1-min spacing
    int gridNumRows = 2401;
    int gridNumCols = 4201;

};

/// Format of the data in the Geoid18 model file, g2018u0.asc
/// These values also work for Geoid12b, filename g2012bu0.asc
namespace Geoid18FileFormat{
    const int HEADER_BYTES = 103;
    const int GRID_ROW_BYTES = 82*525+12;
    const int FILE_ROW_BYTES = 82;
    const int COL_BYTES = 10;
    const int NUM_GRID_ROWS = 526;
};

/// Class Geoid encapsulates the data and methods needed to import a gridded
/// geoid file, and calculate undulation and deflection at a point using a
/// specified interpolation scheme.
class Geoid {
public:
    // member data --------------------------------------

    typedef std::pair<double, double> latLonPair;
    enum Interpolator {UNKNOWN=0,     ///< enum of different interpolation methods
                       BICUBIC,              
                       BILINEAR};

    enum GeoidModel {NONE=-1,
                     EGM96,
                     EGM08,
                     GEOID12b,
                     GEOID18,
                     CUSTOM};

    static gnsstk::Matrix<double> BicubicCoefMatrix; 

    /// empty constructor
    Geoid(void);
    
    /// destructor
    virtual ~Geoid() { }

    /// constructor from filename
    /// @param fn string containing filename for the gridded geoid file to be loaded
    Geoid(std::string fn);
   
    /// constructor from filename, grid spacing
    /// @param fn string containing filename for the gridded geoid file to be loaded
    /// @param gs double grid spacing of the file to be loaded
    Geoid(std::string fn, double gs);

    /// constructor from filename, grid spacing, and interpolator to be used
    /// @param fn string containing filename for the gridded geoid file to be loaded
    /// @param gs double grid spacing of the file to be loaded
    /// @param i Interpolator to be used in calculations
    Geoid(std::string fn, double gs, Interpolator i);

    // Get private file data
    double getGridSpacing(void);
    std::string getGeoidFilename(void);
    std::string getGeoidFilenameH5(void);
   
    /// Compute grid spacing from file utilizing the known row endcaps, assuming equal
    /// row and column spacing.
    /// Sets the gridSpacing member variable
    /// @return grid spacing
    double calculateGridSpacing(void) throw(gnsstk::Exception);
    
    /// Check if file is ready to be loaded
    /// @return bool of whether the file can be loaded into memory
    bool fileReady(void) throw(gnsstk::Exception);

    /// Load file (or portions) into memory
    void loadGeoid(double mnLt, double mxLt, double mnLn, double mxLn)
      throw(gnsstk::Exception);

    /// Load geoids from specific file types
    void loadEGM(double mnLt, double mxLt, double mnLn, double mxLn)
      throw(gnsstk::Exception);
    void loadNGSGeoid(double mnLt, double mxLt, double mnLn, double mxLn)
      throw(gnsstk::Exception);
    void loadHDF(double mnLt, double mxLt, double mnLn, double mxLn)
      throw(gnsstk::Exception);

    /// Write geoid patch to an HDF file
    void writeHDF()
      throw(gnsstk::Exception);
    void setWriteH5Patch();

    /// Pass in a project directory file for writing geoid data
    /// This should only be set if we want to write the geoid to
    /// a patch in the project directory, otherwise leave empty
    void setGeoidFileH5(std::string filename);

    /// Set interpolator to be used
    void setInterpolation(Interpolator i) throw(gnsstk::Exception);

    /// Set interpolator to be used
    void setInterpolation(std::string istr) throw(gnsstk::Exception);

    /// Test whether values may be calculated for a given position and interpolation
    /// @param p Position to be tested
    /// @return bool of whether the values may be calculated there
    bool validPosition(const gnsstk::Position& p) throw(gnsstk::Exception);
    bool validPosition(const LSAPosG *posgRecord);
    bool validPosition(double lat, double lon);

    /// Return min and max elements of undulation matrix
    /// useful for error/range checking
    double getUndMin();
    double getUndMax();
    
    /// Calculate the undulation value at a point
    /// @param p gnsstk::Position for location
    /// @param und reference to be updated with undulation value
    /// @return und double of the interpolated undulation value at point p
    void calculateUndulation(const gnsstk::Position& p, double& und)
      throw(gnsstk::Exception);
    void calculateUndulation(const LSAPosG *posgRecord, double& und);
    
    /// Calculate the deflection value (in radians) at a point
    /// @param p gnsstk::Position for the location
    /// @param dovN reference to be updated with north (meridian) comp. of deflection
    /// @param dovE reference to be updated with east component of deflection
    /// @return def deflection parameters - <North, East>
    void calculateDeflection(const gnsstk::Position& p, double& dovN, double& dovE) 
        throw(gnsstk::Exception);

    void calculateAll(const gnsstk::Position& p, 
                      double &und, double& dovN, double& dovE) 
        throw(gnsstk::Exception);
    void calculateAll(const LSAPosG *posgRecord, double &und, double &dovN, double &dovE);

    /// Set the bicubic coeffecient matrix used for bicubic interpolation
    static gnsstk::Matrix<double> setBicubicCoefMatrix() throw(gnsstk::Exception);

    /// Update undulation and DoV values for all Points.
    /// Originally resided in the now-removed lsageoid.*pp files
    /// @param Points map<string,Point> of Points to be updated
    void UpdateGeoidValues(std::map<std::string,Point>& Points) throw(gnsstk::Exception);


    std::map<std::string, double> UpdateGeoidValues(std::vector<LSAPosG *> &geodeticPos);
    //void UpdateGeoidValues(std::vector<latLonPair> &coordList);

private:
    gnsstk::Matrix<float> und;          ///< matrix of undulation values imported

    double gridSpacing;                ///< Grid size in minutes of the file
    std::string geoidfile;             ///< input path for geoid file
    std::string geoidfileH5;           ///< project directory H5 file for geoid data
    GeoidModel geoidModel;             ///< Geoid model being used
    Interpolator interp;               ///< Interpolator selected for use
    bool writeH5Patch = false;         ///< Flag to determine if .h5 patch of und values will be written

    // Nb: Latitudes are in [-90., 90.]
    //     Longitudes in [0., 360.)
    double minLat;                     ///< minimum latitude for matrix in memory
    double maxLat;                     ///< maximum latitude for matrix in memory
    double minLon;                     ///< minimum longitude for matrix in memory
    double maxLon;                     ///< maximum longitude for matrix in memory

    /// Set geoid model type based on input filename
    void setGeoidModel(std::string fn);

    /// Read NGS geoid model header data
    NGSHeaderData readNGSHeader() throw(gnsstk::Exception);

    /// Convert lat,lon to index position value for interpolation
    /// @param p Position to convert
    /// @param r return row index value
    /// @param c return column index value
    void convertToIndex(double lat, double lon, double& r, double&c);

    /// Prepare position for interpolation, converting to WGS84, then to grid coords
    /// @param p Position to prepare
    /// @param r return integer portion of row value
    /// @param c return integer portion of column value
    /// @param dr return fractional portion of row value, in [0,1)
    /// @param dc return fraction portion of column value, in [0,1)
    void preparePosition(const gnsstk::Position& p,
                         int& r, int& c, double& dr, double& dc);
    void preparePosition(const LSAPosG *posgRecord,
                         int& r, int& c, double& dr, double& dc);
    void preparePosition(double lat, double lon,
                         int& r, int& c, double& dr, double& dc);

    /// Calculate interpolation coefficients at r,c
    /// @param r row value of cell for coefficients
    /// @param c column value of cell for coefficients
    /// @return matrix of interpolation coefficients
    gnsstk::Matrix<double> calculateCoefficients(const int& r, const int& c);

    /// Interpolate value at the point (dr,dc) and the coefficient matrix
    /// @param coef Interpolation coefficients
    /// @param dr row value, in interval [0,1)
    /// @param dc column value, in interval [0,1)
    /// @param retval return interpolated value
    void interpolateValue(const gnsstk::Matrix<double>& coef, 
                          const double& dr, const double& dc,
                          double& retval);

    /// Interpolate derivative at the point (dr,dc) given the coefficients matrix
    /// @param coef Interpolation coefficients
    /// @param dr row value, in interval [0,1)
    /// @param dc column value, in interval [0,1)
    /// @param di_dr return derivative with respect to rows
    /// @param di_dc return derivative with respect to cols
    void interpolateDerivative(const gnsstk::Matrix<double>& coef,
                               const double& dr, const double& dc,
                               double& di_dr, double& di_dc);

    /// Convert derivative in grid coordinates to physical coordinates
    /// @param p Position of calculation
    /// @param du_dr derivative of undulation with respect to rows
    /// @param du_dc derivative of undulation with respect to cols
    /// @param dov_n return deflection (in SoA) towards north
    /// @param dov_e return deflection (in SoA) towards east
    void convertDerivativeToDOV(const gnsstk::Position& p, 
                                const double& du_dr, const double& du_dc,
                                double& dov_n, double& dov_e);

    void convertDerivativeToDOV(double lat,
                                const double& du_dr, const double& du_dc,
                                double& dov_n, double& dov_e);

    /// Generate coefficients for bilinear interpolation
    /// @param r row index of the cell for which coefficients are calculated
    /// @param c column index of the cell for which coefficients are calculated
    /// @return 2x2 matrix of coefficients for bilinear interpolation
    gnsstk::Matrix<double> bilinearCoefs(int r, int c);

    /// Generate coefficients for bicubic interpolation
    /// @param r row index of the cell for which coefficients are calculated
    /// @param c column index of the cell for which coefficients are calculated
    /// @return 4x4 matrix of coefficients for bicubic interpolation
    gnsstk::Matrix<double> bicubicCoefs(int r, int c);

    /// Convert a position to the WGS84 Ellipsoid
    gnsstk::Position convertToWGS84(const gnsstk::Position& p);

};   // end class geoid

#endif  // GEOID_DATA_INCLUDE
