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
/// @file geoid.cpp Implementation of the geoid class

// system includes
#include <string>
#include <utility>
#include <cmath>
#include <ostream>

// GNSSTk
#include "Position.hpp"
#include "WGS84Ellipsoid.hpp"
#include "Matrix.hpp"
#include "Vector.hpp"

// geomatics
#include "logstream.hpp"

//lsa
#include "lsaUtils.hpp"
#include "geoid.hpp"
#include "qdebug.h"

using namespace std;
using namespace gnsstk;

// Constructors

Geoid::Geoid(): 
   geoidfile(string()), gridSpacing(-1),
   minLat(-90), maxLat(90),
   minLon(0), maxLon(0),
   interp(UNKNOWN)
{
    setGeoidModel(geoidfile);
}

Geoid::Geoid(std::string fn):
   geoidfile(fn), gridSpacing(-1),
   minLat(-90), maxLat(90),
   minLon(0), maxLon(0),
   interp(UNKNOWN)
{
    setGeoidModel(geoidfile);
}

Geoid::Geoid(std::string fn, double grid):
   geoidfile(fn), gridSpacing(grid),
   minLat(-90), maxLat(90),
   minLon(0), maxLon(0),
   interp(UNKNOWN)

{
    setGeoidModel(geoidfile);
}

Geoid::Geoid(std::string fn, double grid, Geoid::Interpolator i):
   geoidfile(fn), gridSpacing(grid),
   minLat(-90), maxLat(90),
   minLon(0), maxLon(0),
   interp(i)
{
    setGeoidModel(geoidfile);
}


// Public Functions

double Geoid::getGridSpacing(void)
{
   return gridSpacing;
}

string Geoid::getGeoidFilename(void)
{
   return geoidfile;
}

string Geoid::getGeoidFilenameH5(void)
{
   return geoidfileH5;
}

void Geoid::setGeoidFileH5(std::string filename)
{
    /// This should only be set if we want to write the geoid to
    /// a patch in the project directory, otherwise leave empty
    this->geoidfileH5 = filename;
}

Matrix<double> Geoid::setBicubicCoefMatrix(void) throw(Exception)
{
   Matrix<double> a(16,16,0.);
   a( 0, 0)= 1;
   a( 1, 4)= 1;
   a( 2, 0)=-3; a( 2, 1)= 3; a( 2, 4)=-2; a( 2, 5)=-1;
   a( 3, 0)= 2; a( 3, 1)=-2; a( 3, 4)= 1; a( 3, 5)= 1;
   a( 4, 8)= 1;
   a( 5,12)= 1;
   a( 6, 8)=-3; a( 6, 9)= 3; a( 6,12)=-2; a( 6,13)=-1;
   a( 7, 8)= 2; a( 7, 9)=-2; a( 7,12)= 1; a( 7,13)= 1;
   a( 8, 0)=-3; a( 8, 2)= 3; a( 8, 8)=-2; a( 8,10)=-1;
   a( 9, 4)=-3; a( 9, 6)= 3; a( 9,12)=-2; a( 9,14)=-1;
   a(10, 0)= 9; a(10, 1)=-9; a(10, 2)=-9; a(10, 3)= 9; a(10, 4)= 6; a(10, 5)= 3;
   a(10, 6)=-6; a(10, 7)=-3; a(10, 8)= 6; a(10, 9)=-6; a(10,10)= 3; a(10,11)=-3;
   a(10,12)= 4; a(10,13)= 2; a(10,14)= 2; a(10,15)= 1;
   a(11, 0)=-6; a(11, 1)= 6; a(11, 2)= 6; a(11, 3)=-6; a(11, 4)=-3; a(11, 5)=-3;
   a(11, 6)= 3; a(11, 7)= 3; a(11, 8)=-4; a(11, 9)= 4; a(11,10)=-2; a(11,11)= 2;
   a(11,12)=-2; a(11,13)=-2; a(11,14)=-1; a(11,15)=-1;
   a(12, 0)= 2; a(12, 2)=-2; a(12, 8)= 1; a(12,10)= 1; 
   a(13, 4)= 2; a(13, 6)=-2; a(13,12)= 1; a(13,14)= 1; 
   a(14, 0)=-6; a(14, 1)= 6; a(14, 2)= 6; a(14, 3)=-6; a(14, 4)=-4; a(14, 5)=-2;
   a(14, 6)= 4; a(14, 7)= 2; a(14, 8)=-3; a(14, 9)= 3; a(14,10)=-3; a(14,11)= 3;
   a(14,12)=-2; a(14,13)=-1; a(14,14)=-2; a(14,15)=-1;
   a(15, 0)= 4; a(15, 1)=-4; a(15, 2)=-4; a(15, 3)= 4; a(15, 4)= 2; a(15, 5)= 2;
   a(15, 6)=-2; a(15, 7)=-2; a(15, 8)= 2; a(15, 9)=-2; a(15,10)= 2; a(15,11)=-2;
   a(15,12)= 1; a(15,13)= 1; a(15,14)= 1; a(15,15)= 1;
   
   return a;
}
// Set static variable
Matrix<double> Geoid::BicubicCoefMatrix = Geoid::setBicubicCoefMatrix();


void Geoid::setInterpolation(Interpolator i) throw(Exception)
{
   interp = i;
}

void Geoid::setInterpolation(std::string istr) throw(Exception)
{
   if(istr == "bicubic")
      interp = BICUBIC;
   else if(istr == "bilinear")
      interp = BILINEAR;
   else
      GNSSTK_THROW(Exception(string("Invalid interpolation type specified: ") + istr));
}

void Geoid::loadHDF(double mnLt, double mxLt, double mnLn, double mxLn)
    throw(gnsstk::Exception) {
    try {
        // Set limits from input and validate
        static const double tolerance = 1.E-10;
        minLat = mnLt,maxLat = mxLt,minLon = mnLn,maxLon = mxLn;
        if (minLat>maxLat || minLat< lsa::MIN_LATITUDE_BOUNDARY || maxLat > lsa::MAX_LATITUDE_BOUNDARY)
        {
            GNSSTK_THROW(gnsstk::Exception(string("Invalid latitude limits - latitude must be in range [" + to_string(lsa::MIN_LATITUDE_BOUNDARY) +
                                                ", " + to_string(lsa::MAX_LATITUDE_BOUNDARY) + "]")));
        }
        // Set longitude limits in [0,360] (within tolerance lsa::EPSILON)
        while(minLon<-lsa::EPSILON)
            minLon += 360.;
        while(minLon>360.+lsa::EPSILON)
            minLon -= 360.;
        while(maxLon<-lsa::EPSILON)
            maxLon += 360.;
        while(maxLon>360.+lsa::EPSILON)
            maxLon -= 360.;
         // Validate longitude limits
        if(minLon<-lsa::EPSILON || minLon>360+lsa::EPSILON || maxLon<-lsa::EPSILON || maxLon>=360+lsa::EPSILON)
            GNSSTK_THROW(gnsstk::Exception(string("Invalid longitude limits")));

        // open custom geoid file and read in attributes
        H5::H5File file(geoidfile.c_str(), H5F_ACC_RDONLY);
        H5::DataSet dataset = file.openDataSet("Custom Geoid Heights");
        // helpers for string attributes
        H5::StrType strdatatype(H5::PredType::C_S1, 50);
        char buffer[1][50]; //one row, up to 50 chars
        // rows in the custom geoid file
        H5::Attribute numRowsColsAttr = dataset.openAttribute("Number of (Rows,Columns)");
        numRowsColsAttr.read(strdatatype,buffer);
        string numRowsCols(*buffer);
        numRowsCols = numRowsCols.substr(1,numRowsCols.size()-2);// rm ()
        int tmp = numRowsCols.find(",");
        string numRows = numRowsCols.substr(0,tmp);
        string numCols = numRowsCols.substr(1+tmp);
        int fileRowMax = (int)atoi(numRows.c_str());
        int fileColMax = (int)atoi(numCols.c_str());
        // max lat covered by the custom geoid file
        H5::Attribute latMinMaxAttr = dataset.openAttribute("Latitude N (Minimum,Maximum) in Decimal Degrees");
        latMinMaxAttr.read(strdatatype,buffer);
        string latMinMax(*buffer);
        latMinMax = latMinMax.substr(1,latMinMax.size()-2);// rm ()
        tmp = latMinMax.find(",");
        string latMin = latMinMax.substr(0,tmp);
        string latMax = latMinMax.substr(1+tmp);
        double fileLatMin = (double)atof(latMin.c_str());
        double fileLatMax = (double)atof(latMax.c_str());
        // min lon covered by the custom geoid file
        H5::Attribute lonMinMaxAttr = dataset.openAttribute("Longitude E (Minimum,Maximum) in Decimal Degrees");
        lonMinMaxAttr.read(strdatatype,buffer);
        string lonMinMax(*buffer);
        lonMinMax = lonMinMax.substr(1,lonMinMax.size()-2);// rm ()
        tmp = lonMinMax.find(",");
        string lonMin = lonMinMax.substr(0,tmp);
        string lonMax = lonMinMax.substr(1+tmp);
        double fileLonMin = (double)atof(lonMin.c_str());;
        double fileLonMax = (double)atof(lonMax.c_str());;
        // gridspacing (in minutes dms)
        H5::Attribute gridAttr = dataset.openAttribute("Grid Spacing (Minutes)");
        gridAttr.read(H5::PredType::NATIVE_DOUBLE,&gridSpacing);
        double gridSpacingDeg = gridSpacing/60.; // (in degrees)
        // conversion factor needed (to meters)
        H5::Attribute convAttr = dataset.openAttribute("Geoid Height Units");
        convAttr.read(strdatatype,buffer);
        string conversionUnits(gnsstk::StringUtils::lowerCase(*buffer));
        double conversion=1.; //default assume meters
        if (conversionUnits=="cm") conversion=100.;
        else if (conversionUnits=="km") conversion=0.001;
        else if (conversionUnits=="ft") conversion=3.28084;

        // floor min/max values respectively to nearest grid values
        // Makes for cleaner computation if values are on grid.
        minLat = floor(minLat/gridSpacingDeg)*gridSpacingDeg;
        maxLat = floor(maxLat/gridSpacingDeg)*gridSpacingDeg;
        if(maxLon<minLon)//Wrapping
            maxLon+=360.;
        minLon = floor(minLon/gridSpacingDeg)*gridSpacingDeg;
        maxLon = floor(maxLon/gridSpacingDeg)*gridSpacingDeg;

        // Add/subtract padding size
        // Catch all padding - allows calculations using both bilinear and bicubic interpolation methods
        int bicubicPadding = 5.;
        double paddingfactor = (gridSpacingDeg*bicubicPadding) + 0.5*1./3600.;
        minLat -= paddingfactor;
        maxLat += paddingfactor;
        minLon -= paddingfactor;
        maxLon += paddingfactor;

        // rows and columns in matrix in file index coordinates
        if (fileLonMin < 0)
            fileLonMin += 360.;
        int minRow = floor(0.5 + (fileLatMax - maxLat)/gridSpacingDeg);
        int maxRow = floor(0.5 + (fileLatMax - minLat)/gridSpacingDeg);
        int minCol = floor(0.5 + (minLon - fileLonMin)/gridSpacingDeg);
        int maxCol = floor(0.5 + (maxLon - fileLonMin)/gridSpacingDeg);

        // custom geoid files should not extend beyond the bounds of the file itself
        if ((minRow < 0) || (maxRow >= fileRowMax))
        {
            cout<<"Attempted to access Latitudes ("<<minLat<<" to "<<maxLat<<") and Longitudes ("<<minLon<<" to "<<maxLon<<"); the geoid surface covers Latitudes ("<< fileLatMin<<" to "<<fileLatMax<<") and Longitudes ("<<fileLonMin<<" to "<<fileLonMax<<")\n";
            ostringstream errorMsg;
            errorMsg<<"Error - invalid latitude limits for custom geoid file. Survey size with padding covers ("<<minLat<<","<<maxLat<<"); Geoid covers ("<<fileLatMin<<","<<fileLatMax<<").";
            GNSSTK_THROW(gnsstk::Exception(errorMsg.str()));
        }
        if ((minCol < 0) || (maxCol >= fileColMax))
        {
            cout<<"Attempted to access Latitudes ("<<minLat<<" to "<<maxLat<<") and Longitudes ("<<minLon<<" to "<<maxLon<<"); the geoid surface covers Latitudes ("<< fileLatMin<<" to "<<fileLatMax<<") and Longitudes ("<<fileLonMin<<" to "<<fileLonMax<<")\n";
            ostringstream errorMsg;
            errorMsg<<"Error - invalid longitude limits for custom geoid file. Survey size with padding covers ("<<minLon<<","<<maxLon<<"); Geoid covers ("<<fileLonMin<<","<<fileLonMax<<").";
            GNSSTK_THROW(gnsstk::Exception(errorMsg.str()));
        }

        und.resize(maxRow-minRow, maxCol-minCol,0.);

        // Define Memory Dataspace. Get file dataspace and select
        // a subset from the file dataspace.
        H5::DataSpace filespace = dataset.getSpace();
        int rank = filespace.getSimpleExtentNdims();
        // define memory space to read dataset, select data of interest
        hsize_t startingPoint[2] = {static_cast<hsize_t>(minRow),static_cast<hsize_t>(minCol)};
        hsize_t matrixDims[2] = {static_cast<hsize_t>(und.rows()),static_cast<hsize_t>(und.cols())};
        H5::DataSpace memspace(rank, matrixDims);
        filespace.selectHyperslab(H5S_SELECT_SET,matrixDims,startingPoint);
        // one dimensional temporary buffer-open it up and stick it in the undulation matrix
        float *undulationsBuffer = new float[und.size()];
        dataset.read(undulationsBuffer,H5::PredType::NATIVE_FLOAT,memspace,filespace);
        und.assignFrom(undulationsBuffer);// assign to the undulation matrix
        delete[] undulationsBuffer;// delete our temp buffer
        memspace.close();

        //clean up everything we opened in HDF
        filespace.close();
        dataset.close();
        file.close();
        // adjust undulations to be in meters if they aren't already (EGM96 uses cm)
        if(conversion != 1)
            und /= conversion;
        if (maxLon>360.) maxLon-=360.;
    }
  catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
}

void Geoid::setWriteH5Patch()
{
    this->writeH5Patch = true;
}

void Geoid::writeHDF() throw(gnsstk::Exception)
{
    try {
        // Make sure we have an output file, otherwise do nothing
        if(geoidfileH5.find(".h5") == std::string::npos)
        {
            return;
        }

        // open custom geoid file and set up dataspace
        H5::H5File file(geoidfileH5.c_str(), H5F_ACC_TRUNC);
        hsize_t matrixDims[2] = {static_cast<hsize_t>(und.rows()),static_cast<hsize_t>(und.cols())};
        H5::DataSpace gridspace( 2, matrixDims ); // for heights grid values
        H5::DataSpace att_space(H5S_SCALAR); // for attributes

        // helpers for string attributes
        H5::StrType strdatatype(H5::PredType::C_S1, 250);

        // create dataset
        H5::DataSet dataset = file.createDataSet( "Custom Geoid Heights", H5::PredType::NATIVE_FLOAT, gridspace );

        // write attributes
        dataset.createAttribute( "Geoid Height Units", strdatatype, att_space ).write( strdatatype, "m" );
        dataset.createAttribute( "Description", strdatatype, att_space ).write( strdatatype, "These are the geoid heights derived from the fitting polynomial." );
        dataset.createAttribute( "Format", strdatatype, att_space ).write( strdatatype, "Rows correspond to different latitudes, from North to South.  Columns correspond to different longitudes, from West to East.  The spacing between rows is given in minutes by gridSpacingMinutes." );
        dataset.createAttribute( "Geoid Height Polynomial: Geoid Height", strdatatype, att_space ).write( strdatatype, "" );
        dataset.createAttribute( "Polynomial Origin Point Used and Sum of Residuals Squared", strdatatype, att_space ).write( strdatatype, "" );
        dataset.createAttribute( "Number of (Rows,Columns)", strdatatype, att_space ).write( strdatatype, "(" + to_string(und.rows()) + ", " + to_string(und.cols()) + ")" );
        dataset.createAttribute( "Latitude N (Minimum,Maximum) in Decimal Degrees", strdatatype, att_space ).write( strdatatype, "(" + to_string(minLat) + ", " + to_string(maxLat) + ")" );
        dataset.createAttribute( "Longitude E (Minimum,Maximum) in Decimal Degrees", strdatatype, att_space ).write( strdatatype, "(" + to_string(minLon) + ", " + to_string(maxLon) + ")" );
        dataset.createAttribute( "Grid Spacing (Minutes)", H5::PredType::NATIVE_DOUBLE, att_space ).write( H5::PredType::NATIVE_DOUBLE, &gridSpacing);
        dataset.createAttribute( "Data Type (4 byte float)", strdatatype, att_space ).write( strdatatype, "t" );

        // write heights matrix
        float *undulationsBuffer = new float[und.size()];
        for (int row = 0; row < matrixDims[0]; ++row)
        {
            for (int col = 0; col < matrixDims[1]; ++col)
            {
                undulationsBuffer[row*matrixDims[1] + col] = und(row, col);
            }
        }
        dataset.write( undulationsBuffer, H5::PredType::NATIVE_FLOAT );
        delete[] undulationsBuffer;

        //clean up everything we opened in HDF
        dataset.close();
        gridspace.close();
        att_space.close();
        file.close();

    }
    catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
}

double Geoid::calculateGridSpacing(void) throw(Exception)
{
    // if using NGS geoid, gridSpacing is 1 min
    if(geoidfile.find(".asc") != std::string::npos)
    {
        gridSpacing = 1.0;
        return 1.0;
    }
   // TODO adjust for EGM08 vs EGM96.  Currently hard coded for 08
   int bytesper = 4; // bytes per entry in grid (4 byte float in 08)
   int p = 1;        // units of padding at front and end of each row
   if(geoidfile.empty()) GNSSTK_THROW(Exception(string("No geoid file specified.")));

   streampos begin,end;
   ifstream istrm(geoidfile.c_str(), ios::binary);
   if(!istrm.good()) 
      GNSSTK_THROW(Exception(string("Unable to open geoid file \"")+geoidfile+"\""));
   begin = istrm.tellg();
   istrm.seekg(0,ios::end);
   end = istrm.tellg();

   // entries in file
   int k = (end - begin)/bytesper;

   // Total entries in file (k) = rows * columns
   //                           = (180*60/spacing + 1) * (360*60/spacing + 2*padding)
   // Lots of algebra...
   // spacing = (360 * 60) / (sqrt(2*k + p^2 -2p + 1) - 1 - p)
   double gs = sqrt(2*k+p*p-2*p+1) - 1 - p;
   gs = 360.*60./gs;
   
   gridSpacing = gs;
   // Valid spacing values: 15 min, 2.5 min, 1 min
   double eps = 1E-10;
   if(15. - eps < gridSpacing && gridSpacing < 15. + eps)
      gridSpacing = 15.;
   else if(2.5 - eps < gridSpacing && gridSpacing < 2.5 + eps)
      gridSpacing = 2.5;
   else if(1. - eps < gridSpacing && gridSpacing < 1. + eps)
      gridSpacing = 1.;
   else
      GNSSTK_THROW(Exception(string("Geoid file contains invalid grid spacing")));

   return gridSpacing;
}

bool Geoid::fileReady(void) throw(Exception)
{
   // Verify input parameters
   if(gridSpacing < 0 || geoidfile.empty()) return false;
   // Can open file?
   ifstream istrm(geoidfile.c_str(), ios::in | ios::binary);
   if(!istrm.good())
      return false;
   istrm.close();

   return true;
}

void Geoid::loadEGM(double mnLt, double mxLt, double mnLn, double mxLn)
   throw(Exception)
{
try{
   // Set limits from input
   minLat = mnLt;
   maxLat = mxLt;
   minLon = mnLn;
   maxLon = mxLn;

   // Validate latitude limits
   if (minLat>maxLat || minLat< lsa::MIN_LATITUDE_BOUNDARY || maxLat > lsa::MAX_LATITUDE_BOUNDARY)
   {
      GNSSTK_THROW(Exception(string("Invalid latitude limits - latitudes must be in range [" + to_string(lsa::MIN_LATITUDE_BOUNDARY) +
                                    ", " + to_string(lsa::MAX_LATITUDE_BOUNDARY) + "]")));
   }

   // Set longitude limits in [0,360] (within tolerance lsa::EPSILON)
   while(minLon<-lsa::EPSILON)
      minLon += 360;
   while(minLon>360+lsa::EPSILON)
      minLon -= 360;
   while(maxLon<-lsa::EPSILON)
      maxLon += 360;
   while(maxLon>360+lsa::EPSILON)
      maxLon -= 360;

   // Validate longitude limits
   if(minLon<-lsa::EPSILON || minLon>360+lsa::EPSILON || maxLon<-lsa::EPSILON || maxLon>=360+lsa::EPSILON)
      GNSSTK_THROW(Exception(string("Invalid longitude limits")));

   // If grid not set, calculate grid size
   if(gridSpacing < 0)
      calculateGridSpacing();

   // floor/ceil min/max values respectively to nearest grid values
   // Makes for cleaner computation if values are on grid.
   minLat = floor(minLat*60/gridSpacing)*gridSpacing/60.;
   maxLat = ceil(maxLat*60/gridSpacing)*gridSpacing/60.;
   minLon = floor(minLon*60/gridSpacing)*gridSpacing/60.;
   maxLon = ceil(maxLon*60/gridSpacing)*gridSpacing/60.;

   // Add/subtract padding size
   // Catch all padding - allows calculations using all interpolation methods
   // BILINEAR -> 1
   // BICUBIC -> 3
   int padding = 5;
   minLat -= gridSpacing/60.*padding;
   maxLat += gridSpacing/60.*padding;
   minLon -= gridSpacing/60.*padding;
   maxLon += gridSpacing/60.*padding;

   // Verify that file can be opened
   if(!fileReady())
   {
      GNSSTK_THROW(Exception(string("Unable to open geoid file \"")+geoidfile+"\""));
   }

   // rows and columns in matrix in file index coordinates
   // Note - values not in range [0,fr_max], [0,fc_max] require special treatment
   //   (wrapping of values from other parts)
   int r_min = floor((90-maxLat)*60/gridSpacing+0.5);
   int r_max = floor((90-minLat)*60/gridSpacing+0.5);
   int c_min = floor(minLon*60/gridSpacing+0.5);
   int c_max = floor(maxLon*60/gridSpacing+0.5);

   bool meridian = c_min > c_max;

   //rows and columns in file
   // Rows: 180 degrees [-90, 90] converted to grid, plus 1 for including both ends
   //       of interval
   // Cols: 360 degrees [0,360)
   int fr_max = floor(180*60/gridSpacing+1+0.5);
   int fc_max = floor(360*60/gridSpacing+0.5);

   // Resize undulation matrix
   if(!meridian)
      // Not including meridian
      und.resize(r_max-r_min+1, c_max-c_min+1,0.);
   else
      // Including meridian
      und.resize(r_max-r_min+1, c_max+fc_max-c_min+1, 0.);

   ifstream istrm(geoidfile.c_str(), ios::in | ios::binary);
   if(!istrm.good()) {
      GNSSTK_THROW(Exception(string("Unable to open geoid file \"")+geoidfile+"\""));
   }

   float buffer = 0.;
   int i,j,c;

   // TODO Seek to first row in file, start there, avoiding a lot of work

   if (r_min > 0)
   {
      // Seek the to the first row to be loaded.
      // number of bytes in row:(values + buffer) * value size =
      //                        (fc_max + 2) * 4
      // number of rows to skip: r_min
      istrm.seekg((fc_max+2)*r_min*4,ios::beg);
      i = r_min;
   } else {
      i = 0;
      istrm.seekg(0,ios::beg);
   }

   for( ; i<fr_max && i<=r_max; i++)
   {
      // read buffer at start of row (EGM08 Format)
      // TODO Handle EGM96
      istrm.read((char*)&buffer,4);

      for(j=0; j<fc_max; j++)
      {
          // read value
          istrm.read((char*)&buffer,4);

          // fill cells between r_min, r_max
          // Note: r_min <= i <= r_max by initialization, loop condition
          // wrap backwards (subtract off fc_max <-> 359 deg to -1 deg)
          c = j - fc_max - c_min;
          if(c>=0 && c<und.cols())
              und(i-r_min,c) = buffer;

          // wrap around forward once
          // add fc_max
          c += fc_max;
          if(c>=0 && c<und.cols())
              und(i-r_min,c) = buffer;
          // wrap forward once more
          c += fc_max;
          if(c>=0 && c<und.cols())
              und(i-r_min,c) = buffer;

          // pad >90 lat
          // mirror across the pole
          if(-i>=r_min)
          {
              // mirroring forward (10 deg mirrored to 190 deg)
              c = j+fc_max/2 - c_min;
              if(c>=0 && c<und.cols())
                  und(-i-r_min,c) = buffer;
              // mirroring backward (190 deg to 10 deg)
              c -= fc_max;
              if(c>=0 && c<und.cols())
                  und(-i-r_min,c) = buffer;
          }
          // pad <-90 lat
          // mirror across the pole
          // note: 2*fr_max-i-2 >= r_min by limits on for loop
          if(2*fr_max-i-2 <= r_max)
          {
              c = j+fc_max/2 - c_min;
              if(c>=0 && c<und.cols())
                  und(2*fr_max-i-2-r_min,c) = buffer;
              c -= fc_max;
              if(c>=0 && c<und.cols())
                  und(2*fr_max-i-2-r_min,c) = buffer;
          }
      }
      //read delimiter at end of row
      istrm.read((char*)&buffer, 4);
   }

   istrm.close();
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

NGSHeaderData Geoid::readNGSHeader()
throw(Exception)
{
try
{
    // read file header to get grid boundaries and spacing
    // currently Geoid12b and Geoid18 are the only supported models
    NGSHeaderData ngsData;
    const int headerBytes = Geoid18FileFormat::HEADER_BYTES;
    char fileHeader[headerBytes];
    ifstream fileContents(this->geoidfile, ifstream::binary);
    fileContents.read(fileHeader, headerBytes);

    // read past padding of first row
    int headIndex = 0;
    std::vector<double> headerVals;
    while (headIndex < headerBytes)
    {
        if (fileHeader[headIndex] == ' ')
        {
            ++headIndex;
        }
        else
        {
            headerVals.push_back(stod(&fileHeader[headIndex]));
            // find next space
            while (fileHeader[headIndex] != ' ')
            {
                ++headIndex;
            }
        }
    }

    // set values
    // gridSpacing does not currently support different step sizes in Lat and Lon
    ngsData.cornerLatNorth = headerVals[0];
    ngsData.cornerLonEast = headerVals[1];
    ngsData.latStepDeg = headerVals[2];
    ngsData.lonStepDeg = headerVals[3];
    ngsData.gridNumRows = int(headerVals[4]);
    ngsData.gridNumCols = int(headerVals[5]);

    return ngsData;
}
catch(Exception& e)
{
    GNSSTK_RETHROW(e);
}
}

void Geoid::loadNGSGeoid(double mnLt, double mxLt, double mnLn, double mxLn)
   throw(Exception)
{
try{
    // Set limits from input
    minLat = mnLt;
    maxLat = mxLt;
    minLon = mnLn;
    maxLon = mxLn;

    NGSHeaderData headerData = readNGSHeader();

    if ((geoidModel != Geoid::GeoidModel::GEOID18) && (geoidModel != Geoid::GeoidModel::GEOID12b))
    {
        // throwing an exception to remind future developers to check this function for generality with other NGS Geoid files.
        GNSSTK_THROW(Exception(string("Geoid12b and Geoid18 are the only currently-supported NGS Geoids.  Check file byte counts to support other NGS Geoids")));
    }

    // Set gridSpacing
    // TODO: support different grid spacing in Lat and Lon (is this ever done in models?)
    gridSpacing = 60 * headerData.latStepDeg;

    double GEOID_MIN_LAT = headerData.cornerLatNorth;
    double GEOID_MAX_LAT = headerData.cornerLatNorth + headerData.latStepDeg * (headerData.gridNumRows-1);
    double GEOID_MIN_LON = headerData.cornerLonEast;
    double GEOID_MAX_LON = headerData.cornerLonEast + headerData.lonStepDeg * (headerData.gridNumCols-1);

    // Set longitude limits in [0,360] (within tolerance lsa::EPSILON)
    while(minLon<-lsa::EPSILON) minLon += 360;
    while(minLon>360+lsa::EPSILON) minLon -= 360;
    while(maxLon<-lsa::EPSILON) maxLon += 360;
    while(maxLon>360+lsa::EPSILON) maxLon -= 360;


    // floor/ceil min/max values respectively to nearest grid values
    // Makes for cleaner computation if values are on grid.
    minLat = floor(minLat*60/gridSpacing)*gridSpacing/60.;
    maxLat = ceil(maxLat*60/gridSpacing)*gridSpacing/60.;
    minLon = floor(minLon*60/gridSpacing)*gridSpacing/60.;
    maxLon = ceil(maxLon*60/gridSpacing)*gridSpacing/60.;

    // Add/subtract padding size
    // Catch all padding - allows calculations using all interpolation methods
    // BILINEAR -> 1
    // BICUBIC -> 3
    int padding = 5;
    minLat -= gridSpacing/60.*padding;
    maxLat += gridSpacing/60.*padding;
    minLon -= gridSpacing/60.*padding;
    maxLon += gridSpacing/60.*padding;

    // Validate latitude limits
    if (minLat > maxLat || minLat < GEOID_MIN_LAT - lsa::EPSILON || maxLat > GEOID_MAX_LAT + lsa::EPSILON)
    {
        GNSSTK_THROW(Exception(string("Invalid latitude limits - latitudes (minLat, maxLat) = (")
                              + to_string(minLat) + string(", ") + to_string(maxLat) + ") not in Geoid18 limits ["
                              + to_string(GEOID_MIN_LAT) + string(", ") + to_string(GEOID_MAX_LAT) + "]."
                              + " If the queried grid is close to the boundary, the padding "
                              + " of " + to_string(gridSpacing/60.*padding) + " might push it over the edge."
                              + " Proximity to the model boundary may cause inaccurate results."));
    }

    // Validate longitude limits
    if(minLon < GEOID_MIN_LON - lsa::EPSILON || maxLon >= GEOID_MAX_LON + lsa::EPSILON)
    {
        GNSSTK_THROW(Exception(string("Invalid longitude limits - longitudes (minLon, maxLon) = (")
                              + to_string(minLon) + string(", ") + to_string(maxLon) + ") not in Geoid18 limits ["
                              + to_string(GEOID_MIN_LON) + string(", ") + to_string(GEOID_MAX_LON) + "]."
                              + " If the queried grid is close to the boundary, the interpolation padding "
                              + " of " + to_string(gridSpacing/60.*padding) + " might push it over the edge."
                              + " Proximity to the model boundary may cause inaccurate results."));
    }

    // Verify that file can be opened
    if(!fileReady())
    {
      GNSSTK_THROW(Exception(string("Unable to open geoid file \"")+geoidfile+"\""));
    }

    // The Grid has 2041 rows and 4201 columns
    // g2018u0.asc and g2012bu0.asc are wrapped so that each file row only has 8 values
    // So each row of the grid makes up 526 lines of text (8 * 525 + 1 = 4201)
    // (the last line of each grid row only has 1 value in it)
    // There is also a header row.  526 * 2041 + 1 = 1073567 lines in g2018u0.asc
    // When opening in binary format, the byte counts of different lines are:
    // 103 header row
    // 82 8-value row
    // 12 1-value row
    // Each line starts with 2 space chars, each value has 8 chars, and each newline is 2 chars
    // Each char is 1 byte
    // If this is to be generalized, see namespace Geoid18FileFormat for a starting point
    int gridRowMin = int(floor((minLat - GEOID_MIN_LAT) * 60 + 0.5));
    int gridRowMax = int(floor((maxLat - GEOID_MIN_LAT) * 60 + 0.5));

    int gridColMin = int(floor((minLon - GEOID_MIN_LON) * 60 + 0.5));
    int gridColMax = int(floor((maxLon - GEOID_MIN_LON) * 60 + 0.5));

    // get rows and columns in matrix in file coordinates
    // row == line, and column == token in a line
    // use 0-based indexing, and account for skipping 1 header row
    int numBytes;
    int baseRow, rowOffset, fileRow, fileCol;
    double heightsVal = 0.;
    ifstream fileContents(this->geoidfile, ifstream::binary);

    // Set up und matrix and filestream
    int numRows = gridRowMax - gridRowMin + 1;
    int numCols = gridColMax - gridColMin + 1;
    und.resize(numRows, numCols, 0.);
    char readBuffer[7];

    for (int row = gridRowMin; row <= gridRowMax; ++row)
    {
        for (int col = gridColMin; col <= gridColMax; ++col)
        {
            // translate grid coordinates to file coordinates
            baseRow = Geoid18FileFormat::NUM_GRID_ROWS * row;
            rowOffset = col / 8; // integer division
            fileCol = col % 8; // modulo

            // move file pointer and read value
            // 3 leading bytes for 2 spaces and a minus sign
            numBytes = 3 + Geoid18FileFormat::HEADER_BYTES
                    + Geoid18FileFormat::GRID_ROW_BYTES*row
                    + Geoid18FileFormat::FILE_ROW_BYTES*rowOffset
                    + Geoid18FileFormat::COL_BYTES*fileCol;
            fileContents.seekg(numBytes, ios::beg);
            fileContents.read(readBuffer, 7);
            if(readBuffer[0] == '-')
            {// height is a one-digit negative value
                heightsVal = -stof(&readBuffer[1]);
            }
            else if (readBuffer[0] == ' ')
            {// height is a one-digit positive value
                heightsVal = stof(&readBuffer[2]);
            }
            else
            {// height is a two-digit negative value
                heightsVal = -stof(readBuffer);
            }
            und(gridRowMax-row, col-gridColMin) = heightsVal;
        }
    }

    fileContents.close();
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

void Geoid::loadGeoid(double mnLt, double mxLt, double mnLn, double mxLn)
   throw(Exception)
{
try{
    //loadHDF if we have one
    if (geoidModel == Geoid::GeoidModel::CUSTOM)
    {
        loadHDF(mnLt, mxLt, mnLn, mxLn);
        return; // don't write .h5 if we already have one
    }
    //loadEGM if an EGM geoid
    else if (geoidModel == Geoid::GeoidModel::EGM08 || geoidModel == Geoid::GeoidModel::EGM96)
    {
        loadEGM(mnLt, mxLt, mnLn, mxLn);
    }
    //loadNGSGeoid if Geoid12b/Geoid18
    else if ((geoidModel == Geoid::GeoidModel::GEOID18) || (geoidModel == Geoid::GeoidModel::GEOID12b))
    {
        loadNGSGeoid(mnLt, mxLt, mnLn, mxLn);
    }
    else
    {
        GNSSTK_THROW(Exception(string("No valid geoid model selected from file \"")+geoidfile+"\""));
    }

    // write patch to .h5 file if desired
    if (writeH5Patch)
    {
        writeHDF();
    }
    return;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

bool Geoid::validPosition(const Position& p) throw(Exception)
{
   Position pos;
   try { pos = convertToWGS84(p); } catch(Exception& e) { GNSSTK_RETHROW(e) }

   double lat = pos.getGeodeticLatitude(); // [-90,90]
   double lon = pos.getLongitude();        // [0,360.)
   
   return validPosition(lat, lon);
}

bool Geoid::validPosition(const LSAPosG *posgRecord)
{
    return validPosition(posgRecord->giveStandardLat(), posgRecord->giveStandardLon(true));
}

bool Geoid::validPosition(double lat, double lon)
{
    double space;
    switch (interp)
    {
    case BILINEAR:
        space = 2 * gridSpacing / 60;
        break;
    case BICUBIC:
        space = 3 * gridSpacing / 60;
        break;
    default:
        space = 0.0;
        break;
    }

    if((lat > maxLat-space) || (lat < minLat-space)) {
       return false;
    }

    // if range does not include prime meridian
    if(minLon <= maxLon)
       return ((minLon+space < lon) && (lon < maxLon+space));

    // if range does include prime meridian
    return ((minLon+space < lon) || (lon < maxLon+space));
}

double Geoid::getUndMin()
{
    return *(std::min_element(und.begin(), und.end()));
}

double Geoid::getUndMax()
{
    return *(std::max_element(und.begin(), und.end()));
}

void Geoid::calculateUndulation(const Position& p, double& und) throw(Exception)
{
   int r,c;
   double dr,dc;
   Matrix<double> coefs;
   // Convert pos to WGS84, then to grid coords
   preparePosition(p,r,c,dr,dc);
   // Calculate coefficients
   coefs = calculateCoefficients(r,c);
   // Interpolate values
   interpolateValue(coefs,dr,dc,und);
}

void Geoid::calculateUndulation(const LSAPosG *posgRecord, double& und)
{
    int r,c;
    double dr,dc;
    Matrix<double> coefs;
    // Convert pos to grid coords
    preparePosition(posgRecord,r,c,dr,dc);
    // Calculate coefficients
    coefs = calculateCoefficients(r,c);
    // Interpolate values
    interpolateValue(coefs,dr,dc,und);
}

void Geoid::calculateDeflection(const Position& p, double& dov_n, double& dov_e) throw(Exception)
{
   int r,c;
   double dr,dc,du_dr,du_dc;
   Matrix<double>coefs;
   // Convert pos to WGS84, then to grid coords
   preparePosition(p,r,c,dr,dc);
   // Calculate coefficients
   coefs = calculateCoefficients(r,c);
   // Interpolate derivatives
   interpolateDerivative(coefs,dr,dc,du_dr,du_dc);
   // Convert to DOV
   convertDerivativeToDOV(p,du_dr,du_dc,dov_n,dov_e);
}

void Geoid::calculateAll(const Position& p, double& und, double& dov_n, double& dov_e) throw(Exception)
{
   int r,c;
   double dr,dc,du_dr,du_dc;
   Matrix<double>coefs;
   // Convert pos to WGS84, then to grid coords
   preparePosition(p,r,c,dr,dc);
   // Calculate coefficients
   coefs = calculateCoefficients(r,c);
   // Interpolate values
   interpolateValue(coefs,dr,dc,und);
   // Interpolate derivatives
   interpolateDerivative(coefs,dr,dc,du_dr,du_dc);
   // Convert to DOV
   convertDerivativeToDOV(p,du_dr,du_dc,dov_n,dov_e);
}

void Geoid::calculateAll(const LSAPosG *posgRecord, double& und, double& dov_n, double& dov_e)
{
   int r,c;
   double dr,dc,du_dr,du_dc;
   Matrix<double>coefs;
   // Convert pos to WGS84, then to grid coords
   preparePosition(posgRecord->giveStandardLat(), posgRecord->giveStandardLon(true), r,c,dr,dc);
   // Calculate coefficients
   coefs = calculateCoefficients(r,c);
   // Interpolate values
   interpolateValue(coefs,dr,dc,und);
   // Interpolate derivatives
   interpolateDerivative(coefs,dr,dc,du_dr,du_dc);
   // Convert to DOV
   convertDerivativeToDOV(posgRecord->giveStandardLat(), du_dr,du_dc,dov_n,dov_e);
}

// Private functions

void Geoid::setGeoidModel(std::string fn)
{
    // initialize this for the cases not handled under ".und" and ".asc"
    geoidModel = Geoid::GeoidModel::NONE;
    // any .h5 file should be considered custom even if it
    // is just a patch of a previously-specified model
    if (fn.find(".h5") != std::string::npos)
    {
        geoidModel = Geoid::GeoidModel::CUSTOM;
    }
    //EGM if a .und file
    else if (geoidfile.find(".und") != std::string::npos)
    {
        if (geoidfile.find("egm2008") != std::string::npos)
        {
            geoidModel = Geoid::GeoidModel::EGM08;
        }
        else if (geoidfile.find("egm1996") != std::string::npos)
        {
            geoidModel = Geoid::GeoidModel::EGM96;
        }
    }
    //Geoid18 or Geoid12b if a .asc file
    else if (geoidfile.find(".asc") != std::string::npos)
    {
        if (geoidfile.find("g2012b") != std::string::npos)
        {
            geoidModel = Geoid::GeoidModel::GEOID12b;
        }
        else if (geoidfile.find("g2018") != std::string::npos)
        {
            geoidModel = Geoid::GeoidModel::GEOID18;
        }
    }
}

void Geoid::preparePosition(const Position& p, int& r, int& c, double& dr, double& dc)
{
   Position pos = convertToWGS84(p);
   preparePosition(pos.getGeodeticLatitude(), pos.getLongitude(), r, c, dr, dc);
}

void Geoid::preparePosition(const LSAPosG *posgRecord,
                     int& r, int& c, double& dr, double& dc)
{
    preparePosition(posgRecord->giveStandardLat(), posgRecord->giveStandardLon(true), r, c, dr, dc);
}

void Geoid::preparePosition(double lat, double lon, int &r, int &c, double &dr, double &dc)
{
    double row,col;
    convertToIndex(lat, lon, row, col);

    // Separate into int,remainder
    r = floor(row);
    dr = row - r;
    c = floor(col);
    dc = col - c;

    // Sanity check for rounding errors
    while(dr<0.)  { dr += 1.0; r -= 1; }
    while(dr>=1.) { dr -= 1.0; r += 1; }
    while(dc<0.)  { dc += 1.0; c -= 1; }
    while(dc>=1.) { dc -= 1.0; c += 1; }
}

void Geoid::convertToIndex(double lat, double lon, double& r, double& c)
{
   // Matrix begins at maxLat, descends to minLat with each row
   r = (maxLat-lat)*60./gridSpacing;

   // to account for ranges including the meridian (minLon > maxLon)
   if(lon > minLon)
      c = (lon-minLon)*60./gridSpacing;
   else
      c = (lon - minLon + 360.)*60./gridSpacing;
}

Matrix<double> Geoid::calculateCoefficients(const int& r, const int& c)
{
   Matrix<double> coefs;
   switch(interp)
   {
      case BILINEAR:
         coefs = bilinearCoefs(r,c);
         break;
      case BICUBIC:
         coefs = bicubicCoefs(r,c);;
         break;
      default:
         GNSSTK_THROW(Exception(
            string("No interpolation method specified for geoid calculations")));
   }
   return coefs;
}

void Geoid::interpolateValue(const Matrix<double>& coefs,
                           const double& dr, const double& dc,
                           double& val)
{
   int dim(coefs.cols()),i;
   Matrix<double> x(1,dim,1.),y(dim,1,1.),tmp;

   for(i=1; i<dim; i++) {
      x(0,i) = pow(dr,i);
      y(i,0) = pow(dc,i);
   }
   
   tmp = x*coefs*y;
   val = tmp(0,0);
}

void Geoid::interpolateDerivative(const Matrix<double>& coefs,
                               const double& dr, const double& dc,
                               double& di_dr, double& di_dc)
{
   int dim(coefs.cols()),i;
   Matrix<double> dx(1,dim,0.),dy(dim,1,0.),
                 x(1,dim,1.),y(dim,1,1.),
                 tmp;
   
   for(i=1; i<dim; i++) {
      x(0,i) = pow(dr,i);
      dx(0,i) = pow(dr,i-1)*i;
      y(i,0) = pow(dc,i);
      dy(i,0) = pow(dc,i-1)*i;
   }
   tmp = dx*coefs*y;
   di_dr = tmp(0,0);
   tmp = x*coefs*dy;
   di_dc = tmp(0,0);
}

void Geoid::convertDerivativeToDOV(const gnsstk::Position& p,
                                 const double& du_dr, const double& du_dc,
                                 double& dov_n, double& dov_e)
{
   double a,e2,rlat,Rn,Rm,gridSpaceRad;

   WGS84Ellipsoid WGS84;
   Position pos = convertToWGS84(p);
   rlat = pos.getGeodeticLatitude()*::DEG_TO_RAD;
   a = WGS84.a();
   e2 = WGS84.eccSquared();
   gridSpaceRad = ::DEG_TO_RAD*gridSpacing/60.;
   // Compute radii of curvature
   Rn = a/sqrt(1-e2*pow(sin(rlat),2));
   Rm = Rn*(1-e2)/(1-e2*pow(sin(rlat),2));

   //scale to physical from grid (radians - dU/dn, dU/de) and fix sign
   // du/dn = du/dr * dr/d(lat) * d(lat)/dn
   //       = du/dr * -1/gridSpaceRad * 1/Rm
   // dov_n = -atan(du/dn)
   dov_n = atan2(du_dr,gridSpaceRad*Rm);
   // du/de = du/dc * dc/d(lon) * d(lon)/de
   //       = du/dc * 1/gridSpaceRad * 1/(Rn*cos(rlat))
   // dov_e = -atan(du/dc)
   dov_e = -atan2(du_dc,gridSpaceRad*Rn*cos(rlat));
   //convert to seconds
   dov_n *= 3600/::DEG_TO_RAD;
   dov_e *= 3600/::DEG_TO_RAD;
}

void Geoid::convertDerivativeToDOV(double lat,
                                 const double& du_dr, const double& du_dc,
                                 double& dov_n, double& dov_e)
{
   double a,e2,rlat,Rn,Rm,gridSpaceRad;

   WGS84Ellipsoid WGS84;
   rlat = lat*::DEG_TO_RAD;
   a = WGS84.a();
   e2 = WGS84.eccSquared();
   gridSpaceRad = ::DEG_TO_RAD*gridSpacing/60.;
   // Compute radii of curvature
   Rn = a/sqrt(1-e2*pow(sin(rlat),2));
   Rm = Rn*(1-e2)/(1-e2*pow(sin(rlat),2));

   //scale to physical from grid (radians - dU/dn, dU/de) and fix sign
   // du/dn = du/dr * dr/d(lat) * d(lat)/dn
   //       = du/dr * -1/gridSpaceRad * 1/Rm
   // dov_n = -atan(du/dn)
   dov_n = atan2(du_dr,gridSpaceRad*Rm);
   // du/de = du/dc * dc/d(lon) * d(lon)/de
   //       = du/dc * 1/gridSpaceRad * 1/(Rn*cos(rlat))
   // dov_e = -atan(du/dc)
   dov_e = -atan2(du_dc,gridSpaceRad*Rn*cos(rlat));
   //convert to seconds
   dov_n *= 3600/::DEG_TO_RAD;
   dov_e *= 3600/::DEG_TO_RAD;
}

//http://en.wikipedia.org/wiki/Bilinear_interpolation
Matrix<double> Geoid::bilinearCoefs(int r, int c)
{
   Matrix<double> coefs(2,2,0.0);
   coefs(0,0) = und(r,c);
   coefs(1,0) = und(r+1,c) - und(r,c);
   coefs(0,1) = und(r,c+1) - und(r,c);
   coefs(1,1) = (und(r+1,c+1) + und(r,c)) -
                (und(r+1,c) + und(r,c+1));
   return coefs;
}

// http://en.wikipedia.org/wiki/Bicubic_interpolation
Matrix<double> Geoid::bicubicCoefs(int r, int c)
{
   Vector<double> x(16,0.);
   Vector<double> alpha;

   x[0]=und(r,c);
   x[1]=und(r+1,c);
   x[2]=und(r,c+1);
   x[3]=und(r+1,c+1);
   x[4]=(und(r+1,c)-und(r-1,c))/2; // df/dx central diff
   x[5]=(und(r+2,c)-und(r,c))/2; // df/dx central diff
   x[6]=(und(r+1,c+1)-und(r-1,c+1))/2; // df/dx central diff
   x[7]=(und(r+2,c+1)-und(r,c+1))/2; // df/dx central diff
   x[8]=(und(r,c+1)-und(r,c-1))/2; // df/dy central diff
   x[9]=(und(r+1,c+1)-und(r+1,c-1))/2; // df/dy central diff
   x[10]=(und(r,c+2)-und(r,c))/2; // df/dy central diff
   x[11]=(und(r+1,c+2)-und(r+1,c))/2; // df/dy central diff
   x[12]=(und(r+1,c+1)+und(r-1,c-1)-  // d2f/(dxdy) central diff
         und(r+1,c-1)-und(r-1,c+1))/2;
   x[13]=(und(r+2,c+1)+und(r,c-1)-
         und(r+2,c-1)-und(r,c+1))/2;
   x[14]=(und(r+1,c+2)+und(r-1,c)-
         und(r+1,c)-und(r-1,c+2))/2;
   x[15]=(und(r+2,c+2)+und(r,c)-
         und(r+2,c)-und(r,c+2))/2;

   alpha=BicubicCoefMatrix*x;
   Matrix<double> coef(4,4,alpha);
   coef = transpose(coef);
   return coef;
}


Position Geoid::convertToWGS84(const Position& p)
{
   WGS84Ellipsoid WGS84;
   Position retval = p;
   retval.asECEF();
   retval.asGeodetic(&WGS84);

   return retval;
}

void Geoid::UpdateGeoidValues(std::map<string, Point> &Points) throw(gnsstk::Exception)
{
    try
    {
        map<string, Point>::iterator it;
        for(it=Points.begin(); it != Points.end(); it++)
        {
            Position p = it->second.getPosition();
            double lat,lon,ht;
            it->second.getLatLonHeight(lat,lon,ht);
            if(validPosition(p))
            {
                calculateAll(p, it->second.und, it->second.dovN, it->second.dovE);
            }

            else {
               LOG(WARNING) << " Warning - unable to update geoid values for station '"
                  << it->first << "'.";
            }
        }
    }

    catch(Exception &e)
    {
        LOG(ERROR) << " Error - error updating geoid values:\n" << e.what();
        GNSSTK_RETHROW(e);
    }
}

std::map<string, double> Geoid::UpdateGeoidValues(std::vector<LSAPosG *> &geodeticPos)
{
    std::map<std::string, double> stationUndMap;
    double und, dovN, dovE;
    for(std::vector<LSAPosG *>::const_iterator orthoPosIter = geodeticPos.begin();
        orthoPosIter != geodeticPos.end(); ++orthoPosIter)
    {
        calculateAll(*orthoPosIter, und, dovN, dovE);
        stationUndMap[(*orthoPosIter)->getLabel()] = und;
    }

    return stationUndMap;
}

//------------------------------------------------------------------------------------
// find bounding box of Points, for use with geoid
void FindBoundingBox(map<string,Point>& Points,
                     double& latmin, double& latmax,
                     double& lonmin, double& lonmax)
{
   std::vector<std::pair<double, double>> latLonPairs;

   // scan points for min, max values
   map<string,Point>::iterator it;
   for(it=Points.begin(); it != Points.end(); it++) {
      double lat, lon, ht;
      it->second.getLatLonHeight(lat,lon,ht,true);       // true for degrees

      std::pair<double, double> latLonPair(lat, lon);
      latLonPairs.push_back(latLonPair);
   }

   performComparisons(latLonPairs, latmin, latmax, lonmin, lonmax);
}

//------------------------------------------------------------------------------------
// find bounding box for orthometric POSG records, for use with geoid
void FindBoundingBox(const std::vector<LSAPosG *> &passedPoints,
                     double &latmin, double &latmax,
                     double &lonmin, double &lonmax)
{
    std::vector<std::pair<double, double>> latLonPairs;

    for(std::vector<LSAPosG *>::const_iterator it=passedPoints.begin();
        it != passedPoints.end(); it++)
    {
       double lat = (*it)->giveStandardLat();
       double lon = (*it)->giveStandardLon(true);

       std::pair<double, double> latLonPair(lat, lon);
       latLonPairs.push_back(latLonPair);
    }

    performComparisons(latLonPairs, latmin, latmax, lonmin, lonmax);
}

void performComparisons(const std::vector<std::pair<double, double> > &latLonPairs,
                        double &latmin, double &latmax,
                        double &lonmin, double &lonmax)
{
    static const double buffer(1./60.);       // degrees

    latmin = 91.; latmax = -91.; lonmin = 361.; lonmax = -1.;
    double lonmin_rot(361), lonmax_rot(-1.), lon_rot;

    for(std::vector<std::pair<double, double>>::const_iterator latLon_iter = latLonPairs.begin();
        latLon_iter != latLonPairs.end(); ++latLon_iter)
    {
        double lat = latLon_iter->first;
        double lon = latLon_iter->second;

        // rotate 180 degrees (test for meridian)
        // keep in [0,360)
        lon_rot = (lon<180 ? lon+180 : lon-180);
        if(lat < latmin) latmin = lat;
        if(lat > latmax) latmax = lat;
        if(lon < lonmin) lonmin = lon;
        if(lon > lonmax) lonmax = lon;
        if(lon_rot < lonmin_rot) lonmin_rot = lon_rot;
        if(lon_rot > lonmax_rot) lonmax_rot = lon_rot;
    }

    // determine if range should include meridian
    // - choose the shorter range of lon, rotated lon
    if((lonmax - lonmin) > (lonmax_rot - lonmin_rot)) {
       // rotate coords back around, keep in range [0,360)
       lonmin = (lonmin_rot<180) ? lonmin_rot+180 : lonmin_rot-180;
       lonmax = (lonmax_rot<180) ? lonmax_rot+180 : lonmax_rot-180;
    }

    // Add buffer of one minute of arc to allow for site movement
    lonmin -= buffer; lonmax += buffer;
    latmin -= buffer; latmax += buffer;
}
