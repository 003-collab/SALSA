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
#reads a SALSA .h5 output file and generates a CSV file for UTM/UPS coordinates, zone, latitude bands
#along with convergence angle as D,M,S, grid scale factor, elevation factor,
#combined scale factor, and MGRS value

#python imports
import argparse as arg
import os
import sys
import h5py
import utmLib as utm
#import utmPyproj as utmP
import ctypes
import csv

#ellipsoid used for all calculations
ellip="WGS 84"

#define the expected command-line argument flags, corresponding variable names, and default values (if any)
parser = arg.ArgumentParser(usage="")
parser.add_argument('--in', action='store', dest='h5file', required=True, help=".h5 file name with full path (REQUIRED)")
parser.add_argument('--out', action='store', dest='outfile', required=False, help="Full path to CSV output file (OPTIONAL)")

#parse the command-line to the script
args = parser.parse_args()

#alert the user if the provided command-line is invalid
if args.h5file == None:
    sys.stderr.write('h5UTM.py: No input h5 file specified.\n')
    parser.print_help()
    exit(1)
if not os.path.exists(args.h5file):
    sys.stderr.write('h5UTM.py: Could not locate input h5 file %s. Exiting. ' % args.h5file)
    exit(1)
#if no output path is specified then place outfile in same directory as .h5 file with the .cov extension
if args.outfile == None:
    pre, ext = os.path.splitext(args.h5file)
    args.outfile = pre + "_UTM.csv"
    

#read the h5 file
with h5py.File(args.h5file,'r') as h5file:#open h5 file
    #open group
    pointsGroup = h5file["Adjustment Results/Points"]
    dset = pointsGroup["LLH"]
    numOfEntries = len(dset)
    # numOfLabels = len(dset[0][0])
    rdata = dset[...] #need this to take raw h5 data and convert into an array
    
#Calculate UTM values for geodetic coordinates (lat,lon,elht)
#create a string of all values (comma separated)
def calcUTM(LLH):
    #Assign Lat, Lon
    LonDD = LLH[0]
    LatDD = LLH[1]
    #Find the zone number
    utmZone = utm.zone(LLH)
    utmZone_str = utm.zoneLet(LatDD)
        
    #Latitude band
    latBand_str = str(utmZone).zfill(2) + utm.letter(LLH) #zfill(2)=pad left with 2 zeros
    #Grid Coordinates
    utmXY = utm.LLHtoUTM(LLH,utmZone,ellip)
    x_utm = utmXY[0]
    y_utm = utmXY[1]
    x_utm_str = "{:.3f}".format(float(x_utm))    #x coordinate (m)
    y_utm_str = "{:.3f}".format(float(y_utm))    #y coordinate (m)
    gridSF= utmXY[2]
    gridSF_str = "{:.8f}".format(float(gridSF))    #grid scale factor
    com = utmXY[3]
    comD, comM, comS = utm.DDtoDMS(com)     #convert to DMS values
    com_str = "{:.8f}".format(float(com))    #conv of merid (deg)
    comD_str = "{:.0f}".format(float(comD))
    comM_str = "{:.0f}".format(float(comM))
    comS_str = "{:.1f}".format(float(comS))
    if comD_str.find("-0")==0:
        quote = "\""
        comD_str = "=" + quote + "-0" + quote   
    elSF = utm.ElevSF(LLH,ellip)
    elSF_str = "{:.8f}".format(float(elSF[0]))    #elevation scale factor
    comSF = elSF*gridSF
    comSF_str = "{:.8f}".format(float(comSF[0]))    #combined scale factor
    #Compute MGRS
    MGRS_str = utm.UTMtoMGRS(utmZone,latBand_str,x_utm,y_utm)
    #Combined string
    dataStr = [pointName,str(utmZone),utm.zoneLet(LatDD),x_utm_str,y_utm_str,latBand_str,com_str,comD_str,comM_str,comS_str,gridSF_str,elSF_str,comSF_str,MGRS_str]
    return dataStr

#Calculate UTM values for geodetic coordinates (lat,lon,elht)
#create a string of all values (comma separated)
def calcUPS(LLH,Z):
    #Assign Lat, Lon
    LonDD = LLH[0]
    LatDD = LLH[1]     
    
    #Grid Coordinates
    upsXY = utm.LLHtoUPS(LLH,Z,ellip)
    x_ups = upsXY[0]
    y_ups = upsXY[1]
    x_ups_str = "{:.3f}".format(float(x_ups))    #x coordinate (m)
    y_ups_str = "{:.3f}".format(float(y_ups))    #y coordinate (m)
    gridSF= upsXY[2]
    gridSF_str = "{:.8f}".format(float(gridSF))    #grid scale factor
    com = upsXY[3]
    comD, comM, comS = utm.DDtoDMS(com)     #convert to DMS values
    com_str = "{:.8f}".format(float(com))    #conv of merid (deg)
    comD_str = "{:.0f}".format(float(comD))
    comM_str = "{:.0f}".format(float(comM))
    comS_str = "{:.1f}".format(float(comS))
    #This forces Excel to show "-0" not as just "0"
    if comD_str.find("-0")==0:
        quote = "\""
        comD_str = "=" + quote + "-0" + quote
    Z_str = upsXY[4]
    #Compute MGRS
    MGRS_str = utm.UPStoMGRS(Z,x_ups,y_ups)
    #Compute elevation scale factor
    elSF = utm.ElevSF(LLH,ellip)
    elSF_str = "{:.8f}".format(float(elSF))    #elevation scale factor
    comSF = elSF*gridSF
    comSF_str = "{:.8f}".format(float(comSF))    #combined scale factor
    # Combined string
    dataStr = [pointName,"",Z_str,x_ups_str,y_ups_str,"",com_str,comD_str,comM_str,comS_str,gridSF_str,elSF_str,comSF_str,MGRS_str]
    return dataStr



#Open file and assign LLH values
with open(args.outfile,'w', newline='') as outfile:
    writer = csv.writer(outfile)
    numRows = len(rdata)
    #Column Headers
    writer.writerow(["Station","Zone","Letter","x_m","y_m","LatBand","conv_DD","conv_d","conv_m","conv_s","gridSF","elevSF","combSF","MGRS"])
    #write the rows
    precision = 3 #number of digits after the decimal point
    dataList = [] #initialize the list of data
    for i in range(numRows):
        # Station
        postemp = rdata[i]["Point Label"] #This gets the Point Label attribute (ex ['VNDP'])
        pos = []
        for j in postemp:
            pos.append(j.decode()) #convert byte data from the h5 file into a string
        pointName = " ".join(pos)   #use this to strip out [] and '' from the point label (ex. VNDP)
        #print (pointName)
        # Latitude DD
        LatDD = rdata[i]["Latitude"]
        LatDDstr = "{:.9f}".format(float(LatDD[0])) #limit number of fixed digits after decimal point
        #print ('latDD=',LatDD)
        #print ('latDDstr=',LatDDstr)
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
        
        #Example from Sec. 10.2 (-179deg lon, 89deg lat, 0 elht )
        #LLH = (-90, -81.0106632645, 0) 
        #LatDD = LLH[1]
        #Example from Sec. 3-4 (-10deg lon, 3deg lat, 0 elht )
        #LLH = (-10, 3, 0) 
        #Example from Sec.7.2 
        #LLH = (74, 3, 0)
        
        #Determine region of earth:
        if (LatDD<=90) and (LatDD>=84):
            Z = 1       #North Pole Region
            dataStr = calcUPS(LLH,Z)   #write UPS Coordinates 
        elif (LatDD<-80) and (LatDD>=-90):
            Z = -1      #South Pole Region
            dataStr = calcUPS(LLH,Z)   #write UPS Coordinates
        elif (LatDD<84) and (LatDD>-80):
            Z = 0       #Central Region
            dataStr = calcUTM(LLH) #write UTM Coordinates
        else:
            Z = -999    #invalid region
            dataStr = [pointName,"Invalid Latitude"]
        writer.writerow(dataStr)
             
            
    #MessageBox = ctypes.windll.user32.MessageBoxW
    #MessageBox(None, 'Export Complete', 'UTM Export', 0)


print("\nSolution UTM CSV file written to {0}.".format(args.outfile))
