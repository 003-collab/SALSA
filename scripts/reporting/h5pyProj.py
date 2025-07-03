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
#Author: Brad Beal
#This script depends requires:
#   1. site package pyproj v2.6.1 or later, available: https://github.com/pyproj4/pyproj
#   2. ARL library utmLib.py 
#Converts SALSA WGS 84 (lat,lon,elHt) coordinates to
#   1. Transform to 3D Geographic Coordinates (lat,lon,elHt) of different reference system (e.g. ITRF08)
#   2. Transform to 2D Geographic Coordinates (lat,lon) of different reference system (e.g. ITRF08)
#   3. Transform to 3D Geocentric Coordinates (X,Y,Z) of different reference system (e.g. ITRF08)
#   4. Projection to 2D+Ht Mapping Coordinates (East,North,OrHeight) of different map system (e.g. Calif State Plane) 
#   5. Vertical datum shift (not supported)
#input:    
#   1. SALSA *.h5 output file
#   2. EPSG code of output coordinate system (library available here: available: https://www.spatialreference.org)
#output files:
#   1. projectName_projection_detailed.csv  : detailed csv file showing all input and output coordinates, 
#       for projections, scale factors and convergence angles are shown for each point, as well as mean values for the project
#   2. projectName_projection_detailed_info.txt : metadata text file showing the details of "from" and "to" coordinate syatems
#   3. projectName_XXX_outputCoordinateSystem.csv : basic CSV file showing only the point names and output "to" coordinates

#python imports
import pyproj
from pyproj import CRS
from pyproj.exceptions import CRSError
from pyproj import Transformer
from datetime import datetime
import argparse as arg
import sys
import os
import h5py
import utmLib as utm
import csv

#The name of this file:
this_file = os.path.basename(__file__)

#define the expected command-line argument flags, corresponding variable names, and default values (if any)
#Running 'python h5SolnCov.py --help' outputs the help text
parser = arg.ArgumentParser(usage="")
parser.add_argument('--in', action='store', dest='h5file', required=True, help=".h5 file name with full path (REQUIRED)")
parser.add_argument('--epsg', action='store', dest='epsg_code', required=True, help="EPSG Code (REQUIRED)")
#parser.add_argument('--out', action='store', dest='outfile', required=False, help="Full path to CSV output file (OPTIONAL)")

#parse the command-line to the script
args = parser.parse_args()

#alert the user if the provided command-line is invalid
if args.h5file == None:
    sys.stderr.write(this_file +': No input h5 file specified.\n')
    parser.print_help()
    exit(1)
if args.epsg_code == None:
    sys.stderr.write(this_file + ': No EPSG code specified.\n')
    parser.print_help()
    exit(1)
if not os.path.exists(args.h5file):
    sys.stderr.write(this_file + ': Could not locate input h5 file %s. Exiting. ' % args.h5file)
    exit(1)

#if no output path is specified then place outfile in same directory as .h5 file with the .cov extension
outfile_name = "output_detailed.csv" #place holder name
outfile_CRD = "output_CRD.csv" #place holder name
pre, ext = os.path.splitext(args.h5file)
outfile_name = pre
outfile_CRD = pre

#Error trap for unrecognized EPSG:
try:
    CRS.from_epsg(str(args.epsg_code))
    valid = True
except CRSError:
    valid = False
if valid == False:
    sys.stderr.write('No valid EPSG code specified.\n')
    exit(1)

#epsg code
epsg = args.epsg_code 
    
#Zone metadata
from_epsg = '4979' #4979 is WGS84 lat,lon,ht SALSA is always this CRS for this script 
to_epsg = epsg
from_CRS = CRS.from_epsg(from_epsg)
to_CRS = CRS.from_epsg(to_epsg) 
from_Name = from_CRS.name
to_Name = to_CRS.name
from_ellipsoid = str(from_CRS.ellipsoid)
to_ellipsoid = str(to_CRS.ellipsoid)
to_operation = str(to_CRS.coordinate_operation)
to_type = str(to_CRS.type_name)
#print(to_type)
headerStr_PyProj=""     #header file for pyProj output file

#Read axis labels and units:

if len(to_CRS.axis_info)==3:
    #3D-transformation:
    axis0_name = to_CRS.axis_info[0].name  #0 axis name
    axis0_unit = to_CRS.axis_info[0].unit_name  #0 axis unit
    axis0_str = str(axis0_name).replace(" ","_") + "_" + str(axis0_unit)
    axis1_name = to_CRS.axis_info[1].name  #1 axis name
    axis1_unit = to_CRS.axis_info[1].unit_name  #1 axis unit
    axis1_str = str(axis1_name).replace(" ","_") + "_" + str(axis1_unit)
    axis2_name = to_CRS.axis_info[2].name  #1 axis name
    axis2_unit = to_CRS.axis_info[2].unit_name  #1 axis unit
    axis2_str = str(axis2_name).replace(" ","_") + "_" + str(axis2_unit)
elif len(to_CRS.axis_info)==2:
    #2D-projection
    #projecteded systems do not have a Z component
    #thus, use just the input or "from" ellipsoid height
    #make label units match the x and y axis units
    axis0_name = to_CRS.axis_info[0].name  #0 axis name
    axis0_unit = to_CRS.axis_info[0].unit_name  #0 axis unit
    axis0_str = str(axis0_name).replace(" ","_") + "_" + str(axis0_unit)
    axis1_name = to_CRS.axis_info[1].name  #1 axis name
    axis1_unit = to_CRS.axis_info[1].unit_name  #1 axis unit
    axis1_str = str(axis1_name).replace(" ","_") + "_" + str(axis1_unit)
    axis2_str = "ElHt_m"
    axis_elHt_str = "ElHt_" + str(axis1_unit)
    axis_orHt_str = "OrHt_" + str(axis1_unit)   
elif len(to_CRS.axis_info)==1:
    #1D transform is a vertcal datum shift
    axis0_name = to_CRS.axis_info[0].name  #0 axis name
    axis0_unit = to_CRS.axis_info[0].unit_name  #0 axis unit
    axis0_str = str(axis0_name).replace(" ","_") + "_" + str(axis0_unit)
else:
    sys.stderr.write('EPSG not supported.\n')
    exit(1)

#Setup the output file:
outfile_name = outfile_name + "_projection_detailed.csv"

#Setup the output coordinate file:
#This file will be used as input for CAD and survey software
if to_type == "Geographic 3D CRS" or to_type == "Geographic 2D CRS":
    outfile_CRD = outfile_CRD + "_LLH_" + to_Name.replace(" ", "") + ".csv"
    headerStr_PyProj = ["From_EPSG","CRS","Station","Lat_DD","Lon_DD","ElHt_m","OrHt_m","","To_EPSG","CRS","ellipsoid",axis0_str,axis1_str,axis2_str]
    headerStr_CRD = ["Station",axis0_str,axis1_str,axis2_str] 
elif to_type == "Geocentric CRS":
    outfile_CRD = outfile_CRD + "_XYZ_" + to_Name.replace(" ", "") + ".csv"
    headerStr_PyProj = ["From_EPSG","CRS","Station","Lat_DD","Lon_DD","ElHt_m","OrHt_m","","To_EPSG","CRS","ellipsoid",axis0_str,axis1_str,axis2_str]
    headerStr_CRD = ["Station",axis0_str,axis1_str,axis2_str]
elif to_type == "Projected CRS":
    outfile_CRD = outfile_CRD + "_ENH_" + to_operation.replace(" ", "_") + ".csv"
    headerStr_PyProj = ["From_EPSG","CRS","Station","Lat_DD","Lon_DD","ElHt_m","OrHt_m","","To_EPSG","Zone","ellipsoid",axis0_str,axis1_str,axis_elHt_str,axis_orHt_str,"conv_DD","conv_d","conv_m","conv_s","gridSF","elevSF","combSF"]
    headerStr_CRD = ["Station",axis0_str,axis1_str,axis_orHt_str]
    #Note: projected coordinate users are almost always interested in orthometric heights or elevations, while
    # Ellipsoid heights are rarely needed.
    # If needed, the ellipsoid heights can be obtained from the "_projection_detailed.csv" file. 
elif to_type == "Vertical CRS":
    print("Vertical transformation not available: " + to_type)
    exit(1)
else:
    print("Unrecognized coordinate transformation: " + to_type)
    exit(1)

#Notes:
#to_CRS.operation #can be: "None" for CRS transformations or name of to zone if a map projection
#to_CRS.type_name #can be: "Geographic 3D CRS" for LLH, "Geocentric CRS" for XYZ, "Projected CRS" for xyz grid,

#read the h5 file
with h5py.File(args.h5file,'r') as h5file:  #open h5 file in read only mode 'r'
    #open group
    pointsGroup = h5file["Adjustment Results/Points"]
    dset = pointsGroup["LLH"]
    numOfEntries = len(dset)
    rdata = dset[...] #need this to take raw h5 data and convert into an array

def calc_results_transform(LLH,from_epsg,to_epsg):
    """ Function to transform coordinates from one CRS to another

    :param LLH: (longitude, latitude, ellipsoidal height) in decimal degrees
    :param from_epsg: from CRS code
    :param to_epsg: to CRS code
    :return: x,y,z in units of the "to CRS" system (either DD or meters)
    """
    #Reset all output values:
    x=0
    y=0
    z=0
    
    #Assign transformation to use based on EPSG user selected:
    from_epsg_str = 'EPSG:'+ from_epsg
    to_epsg_str = 'EPSG:'+ to_epsg

    #Transform coordinates
    transformer = Transformer.from_crs(from_epsg_str, to_epsg_str, always_xy=True)

    #Convert lon,lat(DD),h_m into x,y,z:
    lam = LLH[0]
    phi = LLH[1]
    h = LLH[2]
    x,y,z = transformer.transform(lam, phi,h)
    return x,y,z

def calc_results_projection(LLH,to_epsg):
    """ Function to transform coordinates from one CRS to map projection grid.
    gridSF as meridian grid scale factor (extracted from pyproj)
    elevSF as elevation scale factor (computed)
    comDD as convergence of meridians in DD (extracted from pyproj)

    :param LLH: (lon, lat, ellipsoid height) in Dec Deg
    :param to_epsg: to CRS code
    :return: x,y,z of the "to CRS" system (meters or feet)
    """
    #Reset all output values:
    x=0
    y=0
    gridSF=0 
    elevSF=0
    comDD=0
    
    #Assign map projection to use based on EPSG user selected:
    epsg_str = 'EPSG:'+ to_epsg
    proj_used = pyproj.Proj(epsg_str) 
    
    #Convert lon,lat(DD) into x,y:
    lam = LLH[0]
    phi = LLH[1]
    x,y = proj_used(lam, phi)
    
    #Extract coordinate factors:
    #def get_factors(longitude: Any, latitude: Any, radians: bool=False, errcheck: bool=False)
    #Calculate various cartographic properties, such as scale factors, angular distortion and meridian convergence. Depending on the underlying projection values will be calculated either numerically (default) or analytically.
    #The function also calculates the partial derivatives of the given coordinate.
    factors = proj_used.get_factors(lam,phi,False,True)
    
    #Scale factors:
    mScale = factors.meridional_scale
    gridSF = mScale

    #convergence angle (in DD):
    comDD = factors.meridian_convergence

    #Compute elevation scale factor:
    crs_data = CRS.from_epsg(epsg)
    invf = crs_data.ellipsoid.inverse_flattening
    a = crs_data.ellipsoid.semi_major_metre
    elevSF = utm.ElevSF2(LLH,a,invf)
    return x,y,gridSF,elevSF,comDD

   
#Setup variables to be used:
#String and List Variables:
dataStr=""
numRows = len(rdata)
dataList_detail = [] #initialize the list of data
dataList_CRD = [] #initialize the list of data

#Statistics variables:
count = 0
sum_combSF = 0
sum_comDD = 0
sum_elevSF = 0
sum_gridSF = 0

#Read lines from the .h5 file,
#Transform or project each station based on EPSG,
#Add projected/transformed station to dataLists
for i in range(numRows):
    # Station
    postemp = rdata[i]["Point Label"] #This gets the Point Label attribute (ex ['VNDP'])
    pos = []
    for j in postemp:
        pos.append(j.decode()) #convert byte data from the h5 file into a string
    pointName = " ".join(pos)   #use this to strip out [] and '' from the point label (ex. VNDP) 
    # Latitude DD
    LatDD = rdata[i]["Latitude"]
    LatDDstr = "{:.9f}".format(float(LatDD[0])) #limit number of fixed digits after decimal point
    # Longitude DD
    LonDD = rdata[i]["Longitude"]
    if LonDD > 180:
        LonDD = -1*(360-LonDD)
    LonDDstr = "{:.9f}".format(float(LonDD[0])) #limit number of fixed digits after decimal point
    # Ellipsoid height
    elHt = rdata[i]["Height"]
    elHtstr = "{:.3f}".format(float(elHt[0])) #limit number of fixed digits after decimal point
    # Orthometric height
    orHt = rdata[i]["Oht"]
    orHtstr = "{:.3f}".format(float(orHt[0])) #limit number of fixed digits after decimal point
    LLH = (LonDD, LatDD, elHt)
    
    if to_type == "Geographic 3D CRS" or to_type == "Geographic 2D CRS":
        #Convert lon/lat/elHt values into XYZ cartesian
        x,y,z= calc_results_transform(LLH,from_epsg,to_epsg)
        to_lonStr =  "{:.9f}".format(float(x))
        to_latStr =  "{:.9f}".format(float(y))
        to_elHtStr =  "{:.3f}".format(float(z))
        
        #Append all values to dataStr and add row to dataList   
        dataStr = [from_epsg,from_Name,pointName,LatDDstr,LonDDstr,elHtstr,orHtstr,"",to_epsg,to_Name,to_ellipsoid,to_latStr,to_lonStr,to_elHtStr]
        dataList_detail.append(dataStr)
        dataStr = [pointName,to_latStr,to_lonStr,to_elHtStr]
        dataList_CRD.append(dataStr)  

    elif to_type == "Geocentric CRS":
        #Convert lon/lat/elHt values into XYZ cartesian
        x,y,z= calc_results_transform(LLH,from_epsg,to_epsg)
        xStr =  "{:.3f}".format(float(x))
        yStr =  "{:.3f}".format(float(y))
        zStr =  "{:.3f}".format(float(z))
        #Append all values to dataStr and add row to dataList     
        dataStr = [from_epsg,from_Name,pointName,LatDDstr,LonDDstr,elHtstr,orHtstr,"",to_epsg,to_Name,to_ellipsoid,xStr,yStr,zStr]
        dataList_detail.append(dataStr)
        dataStr = [pointName,xStr,yStr,zStr]
        dataList_CRD.append(dataStr) 

    elif to_type == "Projected CRS":
        count +=1
        #Convert lon/lat/elHt values into plane coordinates, scale factors, and convergence angles
        x,y,gridSF,elevSF,comDD = calc_results_projection(LLH,to_epsg)
        #Issue with comDD in southern hemisphere:
        #proj returns vale with opposite sign as is expected
        #See open issue reported: https://github.com/OSGeo/PROJ/issues/2253
        #To fix, multiply comDD by -1
        if LatDD < 0:
            comDD = -1 * comDD

        xStr =  "{:.3f}".format(float(x[0]))
        yStr =  "{:.3f}".format(float(y[0]))
        gridSF_str = "{:.9f}".format(float(gridSF[0]))    #grid scale factor
        elevSF_str = "{:.9f}".format(float(elevSF[0]))    #elevation scale factor
        combSF = elevSF*gridSF
        sum_gridSF += gridSF
        sum_elevSF += elevSF
        sum_combSF += combSF
        combSF_str = "{:.9f}".format(float(combSF[0]))    #combined scale factor
        sum_comDD += comDD
        comD, comM, comS = utm.DDtoDMS(comDD)     #convert to DMS values
        comDD_str = "{:.9f}".format(float(comDD[0]))    #conv of merid (deg)
        comD_str = "{:.0f}".format(float(comD[0]))
        comM_str = "{:.0f}".format(float(comM[0]))
        comS_str = "{:.2f}".format(float(comS[0]))
        #This forces Excel to show "-0" not as just "0"
        if comD_str.find("-0")==0: 
            quote = "\""
            comD_str = "=" + quote + "-0" + quote
        to_elHt = 0
        to_orHt = 0
        if axis1_unit == "US survey foot":
            #US Survey feet
            to_elHt = elHt * 3937/1200
            to_orHt = orHt * 3937/1200
        elif axis1_unit == "foot":
            #Int feet
            to_elHt = elHt/0.3048
            to_orHt = orHt/0.3048
        else:
            #metric
            to_elHt = elHt
            to_orHt = orHt

        to_elHt_str =  "{:.3f}".format(float(to_elHt[0]))
        to_orHt_str =  "{:.3f}".format(float(to_orHt[0]))

        #Append all values to dataStr and add row to dataList    
        dataStr = [from_epsg,from_Name,pointName,LatDDstr, LonDDstr,elHtstr,orHtstr,"",to_epsg,to_Name,to_ellipsoid,xStr,yStr,to_elHt_str,to_orHt_str,comDD_str,comD_str,comM_str,comS_str,gridSF_str,elevSF_str,combSF_str]
        dataList_detail.append(dataStr)
        dataStr = [pointName,xStr,yStr,to_orHt_str]
        dataList_CRD.append(dataStr)
    elif to_type == "Vertical CRS":
        pass
        #Compute new vertical ortho height based on lat/lon of station only
        #on 7/13/20 this transformer method was tried, but the output height=input height
        #disable this thread for now 
        # lam = LLH[0]
        # phi = LLH[1]
        # from_epsg_str = 'EPSG:'+ from_epsg
        # to_epsg_str = 'EPSG:'+ to_epsg
        # lam = LLH[0]
        # phi = LLH[1]
        # h = LLH[2]
                
    else:
        print("Unrecognized coordinate transformation: " + to_type)
    
#Only for Projected systems, show average values:
if to_type == "Projected CRS" and count != 0:
    #Averages:
    mean_gridSF = sum_gridSF/count
    mean_elevSF = sum_elevSF/count
    mean_combSF = sum_combSF/count
    mean_comDD = sum_comDD/count
    comD, comM, comS = utm.DDtoDMS(mean_comDD)     #convert to DMS values
    #Format strings:
    mean_comDD_str = "{:.9f}".format(float(mean_comDD[0]))
    mean_comD_str = "{:.0f}".format(float(comD[0]))
    mean_comM_str = "{:.0f}".format(float(comM[0]))
    mean_comS_str = "{:.2f}".format(float(comS[0]))
    #This forces Excel to show "-0" not as just "0"
    if mean_comD_str.find("-0")==0: 
        quote = "\""
        mean_comD_str = "=" + quote + "-0" + quote
    mean_combSF_str = "{:.9f}".format(float(mean_combSF[0]))
    mean_gridSF_str = "{:.9f}".format(float(mean_gridSF[0]))
    mean_elevSF_str = "{:.9f}".format(float(mean_elevSF[0]))
    #Output line:
    dataList_detail.append('')
    dataStr = ["","","","","","","","","","","","","","","mean:",mean_comDD_str,mean_comD_str,mean_comM_str,mean_comS_str,mean_gridSF_str,mean_elevSF_str,mean_combSF_str]
    dataList_detail.append(dataStr)

      
#Open file and write DETAILED transformed/projected coordinates:
with open(outfile_name,'w',newline='') as outfile:
    writer = csv.writer(outfile)
    writer.writerow(headerStr_PyProj)
    writer.writerows(dataList_detail)  #write all the rows at once to .csv

#Open file and write simple CRD transformed/projected coordinates:
with open(outfile_CRD,'w',newline='') as outfile:
    writer = csv.writer(outfile)
    writer.writerow(headerStr_CRD)
    writer.writerows(dataList_CRD)  #write all the rows at once to .csv

#Open file and write _info.txt file:
pre, ext = os.path.splitext(outfile_name)
outfile_name_info = pre + "_info.txt"
with open(outfile_name_info,'w',newline='') as outfile:
    from_info = str(from_CRS.get_geod)  #details about the CRS for the epsg number
    to_info = str(to_CRS.get_geod)  #details about the CRS for the epsg number 
    vers_pyproj = pyproj.__version__
    vers_proj =  pyproj.__proj_version__
    thisFile = os.path.basename(__file__)
    
    outfile.write(str(thisFile)+"\n")
    outfile.write(str(datetime.now())+"\n")
    outfile.write("pyproj version: "+ vers_pyproj + "\n")
    outfile.write("proj version: "+ vers_proj + "\n")
    outfile.write("-"*75+"\n")
    outfile.write("From System:"+"\n"+from_info+"\n")
    outfile.write("-"*75+"\n")
    outfile.write("To System:"+"\n"+to_info+"\n")
    
print("\nSolution CSV file written to {0}.".format(outfile_name))
