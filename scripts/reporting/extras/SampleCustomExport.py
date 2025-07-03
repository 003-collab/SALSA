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
#File: SampleCustomExport.py
#Purpose: Transforms positions provided in geodetic Lat, Lon, and Height above the WGS-84 ellipsoid to a Line frame
#         defined by an origin and an initial azimuth that runs parallel to the WGS-84 ellipsoid.
#Structure:
#   Global - Defines WGS84 parameters and utilities to rotate to a different azimuth
#   Classes - Defines the Site class
#   Parse - Reads in the user-supplied values for the .hdf file name and output directory
#   ReadHDF - Creates an array of Sites from the .hdf file's LLH Adjustment Results table
#   Line Frame - Transforms lat,lon delta from a reference into an along Line and across Line delta
#   Write Out - Converts the array of Sites into rows in a tab-delimited publication.txt output file

import h5py,os,sys,argparse as arg,math as m,numpy as np,re

##############################
###         Global         ###
##############################

#Convert dms to decimal degrees
def dms2dec(degrees,minutes,seconds):
    return float(degrees)+(float(minutes)/60.)+(float(seconds)/3600.)
  
#Class for parameters
class Parameters(object):
    def __init__(self):
        #Define the LLH offset from reference site (decimal deg,decimal deg,m) and azimuth (decimal deg) of the origin point
        self.originDLLH = (-dms2dec(0,10,30.54303), #Origin Latitude offset from reference
                           -dms2dec(0,0,10.12305), #Origin Longitude offset from reference
                           -22.32196,              #Origin Height offset from reference
                          )
        self.aWGS84 = 6378137.0
        self.fWGS84 = 1./298.257223563
        self.azLine = dms2dec(355,58,49.630)
        self.originLLH = (0.,0.,0.)
    @property
    def razLine(self):
        return m.radians(self.azLine)
    @property
    def roriginLLH(self):
        return (m.radians(self.originLLH[0]),m.radians(self.originLLH[1]),self.originLLH[2])
    @property
    def ellipseParams(self):
        aEllipse = self.aWGS84+self.originLLH[2]
        bEllipse = aEllipse*(1.-self.fWGS84)
        epsilon = (pow(aEllipse,2)-pow(bEllipse,2))/pow(bEllipse,2)
        return aEllipse,bEllipse,epsilon
    @property
    def Renu2Line(self):
        matrix = np.array([[m.cos(self.razLine),m.sin(self.razLine),0.],
                           [-m.sin(self.razLine),m.cos(self.razLine),0.],
                           [0.,0.,1.]])
        return matrix,np.transpose(matrix)

global params
params = Parameters()

##############################
###         Classes        ###
##############################

#Array to hold Site objects
global Sites
Sites = []

#Class for sites
class Site(object):
    def __init__(self):
        self.name = ""
        self.LLH = (0.,0.,0.)
        self.XYZ = (0.,0.,0.)
        self.OH = 0.
        #Cee,Cen,Ceu,Cnn,Cnu,Cuu
        self.covENU = (0.,0.,0.,0.,0.,0.)

    def sqrmat_from_uppertri_list(self, a):
        a = np.array(a)
        n = int(len(a)/2)
        idx = np.triu_indices(n, k=0, m=n)
        upr_tri = np.zeros([n,n])
        upr_tri[idx] = a
        lwr_tri = upr_tri.copy().T
        np.fill_diagonal(lwr_tri,0)
        sqr_mat = upr_tri+lwr_tri
        return sqr_mat

    def uppertri_list_from_sqrmat(m):
        a = np.triu(m)
        a = list(a[np.triu_indices(3)])
        return a

    @property
    def und(self):
        return (self.LLH[2] - self.OH)
    @property
    def frameLine(self):
        deltaLat, deltaLon = (self.LLH[0]-params.originLLH[0]),(self.LLH[1] - params.originLLH[1])
        return Bowring(m.radians(deltaLat),m.radians(deltaLon)) #lateral,downLine
    @property
    def covLine(self):
        covENUMatrix = self.sqrmat_from_uppertri_list(self.covENU)
        covLineMatrix = np.dot(params.Renu2Line[0],np.dot(covENUMatrix,params.Renu2Line[1]))
        return self.uppertri_list_from_sqrmat(covLineMatrix)

##############################
###         Parse          ###
##############################

#Parse the input file argument
parser=arg.ArgumentParser(description="Custom Reference Frame")
parser.add_argument("--in",action='store',dest='inFile',required=True,
                    help="The h5 file resulting from the network adjustment in SALSA.")
parser.add_argument("--out",action='store',dest='outDir',required=False,
                    help="Output directory where data is written to.")
args,unknown = parser.parse_known_args()

global inFile
inFile=args.inFile

#Validate input file
try:
    os.path.isfile(inFile)
except NameError:
    quit()

if args.outDir is None:
    outDir = os.path.dirname(inFile)
else:
    outDir = args.outDir

##############################
###        ReadHDF         ###
##############################

#Read the HDF
def readLLHData():
    try:
        with h5py.File(inFile) as h5file: #open file
            pointsGroup = h5file["Adjustment Results/Points"] #open group
            dset = pointsGroup["LLH"] #open dataset
            rdata = dset[...]
            for i in range(len(dset)):
                Sites.append(Site())
                Sites[i].name = rdata[i]["Point Label"][0]
                Sites[i].LLH = (float(rdata[i]["Latitude"]),float(rdata[i]["Longitude"]),float(rdata[i]["Height"]))
                Sites[i].OH = float(rdata[i]["Oht"])
                Sites[i].covENU = (float(rdata[i]["Cee"]),float(rdata[i]["Cne"]),float(rdata[i]["Ceu"]),float(rdata[i]["Cnn"]),float(rdata[i]["Cnu"]),float(rdata[i]["Cuu"]))
    except KeyError:
        sys.stdout.write("Unable to open: "+inFile+"\n")
        exit(1)

readLLHData()

def readXYZData():
    try:
        with h5py.File(inFile) as h5file: #open file
            pointsGroup = h5file["Adjustment Results/Points"] #open group
            dset = pointsGroup["XYZ"] #open dataset
            rdata = dset[...]
            for i in range(len(dset)):
                Sites[i].XYZ = (float(rdata[i]["X"]),float(rdata[i]["Y"]),float(rdata[i]["Z"]))
    except KeyError:
        sys.stdout.write("Unable to open: "+inFile+"\n")
        exit(1)

readXYZData()

##############################
###      Line Frame       ###
##############################

for i in range(len(Sites)):
    if Sites[i].name.decode() == "REF":
        params.originLLH = ((Sites[i].LLH[0]+params.originDLLH[0]),(Sites[i].LLH[1]+params.originDLLH[1]),(Sites[i].LLH[2]+params.originDLLH[2]))
        break

#Compute downline distance (Bowrings formula)
def Bowring(deltaLat,deltaLon):
    rlat0 = m.radians(params.originLLH[0])
    A = m.sqrt(1.+params.ellipseParams[2]*pow(m.cos(rlat0),4))
    B = m.sqrt(1.+params.ellipseParams[2]*pow(m.cos(rlat0),2))
    C = m.sqrt(1.+params.ellipseParams[2])
    w = A*deltaLon/2.
    d = ((deltaLat/(2.*B)),(3.*params.ellipseParams[2]*deltaLat)/(4.*pow(B,2)),(2.*rlat0+(2.*deltaLat)/3.))
    D = d[0]*(1.+d[1]*m.sin(d[2]))
    E = m.sin(D)*m.cos(w)
    F = (m.sin(w)/A)*(B*m.cos(rlat0)*m.cos(D)-m.sin(rlat0)*m.sin(D))
    G = m.atan2(F,E)
    sigma = 2.*m.asin(m.sqrt(pow(E,2)+pow(F,2)))
    H = m.atan2(m.tan(w)*(m.sin(rlat0)+B*m.cos(rlat0)*m.tan(D)),A)
    alpha = (G-H) % (2.*m.pi) #make positive if negative
    s = params.ellipseParams[0]*C*sigma/pow(B,2)
    lateral,downLine = s*m.sin(alpha-m.radians(params.azLine)),s*m.cos(alpha-m.radians(params.azLine))
    return lateral,downLine

##############################
###       Write Out        ###
##############################
def format_row(site):
    precision = 6
    strSite=(site.name.decode(),site.frameLine[0],site.frameLine[1],site.OH,
             site.covLine[0],site.covLine[1],site.covLine[2],site.covLine[3],site.covLine[4],site.covLine[5],
             site.LLH[0],-(360.-site.LLH[1]),site.LLH[2],
             site.covENU[0],site.covENU[1],site.covENU[2],site.covENU[3],site.covENU[4],site.covENU[5])
    strSite = [str(np.round(item,precision)) if item.isnumeric() else item for item in strSite]
    return '\t'.join(strSite)+"\n"
  
def compose_blocked_sites():
    blocked=[]
    for i in range(51):
        blocked.append("BM0"+str(i).rjust(2,'0')+"A")
        blocked.append("BM0"+str(i).rjust(2,'0')+"B")
    blocked.append('REF')
    blocked.append('STA00')
    return blocked

outFile = os.path.join(outDir, "publication.txt")
colHeaders = ("SiteID","LateralOffset(m)","DownLineOffset(m)","OrthometricHeight(m)",
              "C11(m^2)","C12(m^2)","C13(m^2)","C22(m^2)","C23(m^2)","C33(m^2)",
              "Latitude(deg.)","Longitude(deg.)","EllipsoidalHeight(m)",
              "ENU11(m^2)","ENU12(m^2)","ENU13(m^2)","ENU22(m^2)","ENU23(m^2)","ENU33(m^2)")

blocked = compose_blocked_sites()
with open(outFile,'w') as out:
    out.write('\t'.join(colHeaders)+"\n")
    for site in Sites:
        if site.name.decode() in blocked:
            #don't write out the blocked sites to the publication
            continue
        else:
            out.write(format_row(site))
