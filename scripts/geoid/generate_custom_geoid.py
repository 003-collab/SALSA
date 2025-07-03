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
#File: generate_custom_geoid.py
#Purpose: Create an hdf geoid file from a .geoid or .iob file.
#Structure:
#   Global
#   Classes
#   ParseFile
#   ParseArgs
#   SiteUnds
#   GridUnds
#   MakePlots

import os,sys,h5py,re,subprocess,math as m,numpy as np,argparse as arg
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.font_manager
matplotlib.font_manager.findSystemFonts(fontpaths=None, fontext='ttf')
plt.rcParams.update({'font.size': 12,'font.weight': 'bold'})

##############################
###         Global         ###
##############################

#Array to hold Site objects and GridPts
global Sites,GridPts,proDir,egmFile,cgeoidInputFileName,cgeoidOutputFileName,precision
Sites,GridPts = ([] for ii in range(2))
precision=6

#Convert dms to decimal degrees, also support GeoLab string format
def dms2decGeoLab(line,degMin,degMax,minMin,minMax,secMin,secMax):
  try: 
    tmpDeg=float(line[degMin:degMax])
    tmpMin=float(line[minMin:minMax])/60.
    tmpSec=float(line[secMin:secMax])/3600.
  except TypeError: 
    sys.stderr.write('Encountered a DMS value which is non-numerical.\n'+line+'\n')
    exit(1)
  if ((tmpMin<0.) or (tmpSec<0.)):
   sys.stderr.write('Negative signs on minutes and seconds (dms) are not supported. Please place the negative sign on the degrees.\n'+line+'\n')
   exit(1)
  if (tmpDeg>=0.): tmpDec = tmpDeg+tmpMin+tmpSec
  else: tmpDec = -(-tmpDeg+tmpMin+tmpSec)
  return float(np.sign(tmpDec)*(abs(tmpDec)%360.))
  
#Class for parameters
class Parameters(object):
  def __init__(self):
    self.aWGS84 = 6378137.0
    self.fWGS84 = 1./298.257223563
    self.originN = "" #Name of origin site (first site encountered)
    self.originLLH = (0.,0.,0.) #LLH of origin site
    self.polyDeg = 2 #degree of poly
    self.poly = [] #coeffs of best fit poly
    self.polyStr = "" #poly as a string
    self.sresid2 = 0.0 #sum of resid squared of fit
    self.spacingDegrees = 0.05/60.
    # add padding that is needed by the solver, there are two contributions: bicubicPadding and adjustmentPadding
    self.adjustmentPadding = 1./60. #one arcminute added by lsageoid.cpp to allow for adjustment
  @property
  def roriginLLH(self):
    return (m.radians(self.originLLH[0]),m.radians(self.originLLH[1]),self.originLLH[2])
  @property
  def bicubicPadding(self):
    return 5.*self.spacingDegrees #corresponds to geoid.cpp padding
  @property
  def e2(self):
    return (2.-self.fWGS84)*self.fWGS84
  @property
  def findCoeffs(self):
    if len(Sites) > 0:
      self.originN = Sites[0].name
      self.originLLH = Sites[0].LLH
    else:
      sys.stderr.write("Unable to retreive site data.\n")
      exit(1)

    if len(Sites) == 0:
      self.poly.append(0)
      self.polyStr = "0"
      self.polyDeg = 0
    #point case
    elif len(Sites) == 1:
      self.poly.append(Sites[0].undInput)
      self.polyStr = str(self.poly[0])
      self.polyDeg = 0
    #More than one specified geoid height
    elif len(Sites) > 1:
      self.polyDeg = 2
      offsetVectors,geoidHeights,polyStrTmp = ([] for ii in range(3))
      for ii in range(len(Sites)):
        offsetVectors.append([])
        offsetVectors[ii] = Sites[ii].offsetVector
        geoidHeights.append(Sites[ii].undInput)
      self.poly,self.sresid2,_,_ = np.linalg.lstsq(offsetVectors,geoidHeights,rcond=-1)
      ii = 0
      for var1pow in range(self.polyDeg+1):
        for var2pow in range(self.polyDeg+1-var1pow):
          polyStrTmp.append('('+'{:.6f}'.format(self.poly[ii])+') DLat^'+str(var1pow)+' DLon^'+str(var2pow))
          ii+=1
      self.polyStr = ' + '.join(polyStrTmp)
  @property
  def degreesPadding(self):
    return self.bicubicPadding + self.adjustmentPadding + 5./3600. #5 extra arcseconds added
  @property
  def LatLonMinMax(self):
    lats,lons,lowLons,highLons = ([] for ii in range(4))
    for ii in range(len(Sites)):
      lats.append(Sites[ii].LLH[0])
      lons.append(Sites[ii].LLH[1])
    latmin,latmax = min(lats),max(lats)
    tmp = max(lons)-min(lons)
    #correct for wrapping
    if tmp<180.: lonmin,lonmax = min(lons),max(lons)
    else:
      for Lon in lons:
        if (0<=Lon<180): lowLons.append(Lon)
        elif(180<Lon): highLons.append(Lon)
      lonmin,lonmax = min(highLons),max(lowLons)+360.
    latmax += self.degreesPadding
    lonmax += self.degreesPadding
    latmin -= self.degreesPadding
    lonmin -= self.degreesPadding
    return latmin,lonmin,latmax,lonmax

global params
params = Parameters()

def fillUndEGM():
  egmund = []
  try:
    with open(cgeoidInputFileName, 'w') as cgeoidInput:
      cgeoidInput.write("--latmin {0}\n".format(str(params.LatLonMinMax[0])))
      cgeoidInput.write("--latmax {0}\n".format(str(params.LatLonMinMax[2])))
      cgeoidInput.write("--lonmin {0}\n".format(str(params.LatLonMinMax[1])))
      cgeoidInput.write("--lonmax {0}\n".format(str(params.LatLonMinMax[3])))
      cgeoidInput.write("--gfile \"{0}\"\n".format(egmFile))
      for ii in range(len(Sites)):
        cgeoidInput.write("--pos {0}:{1}\n".format(str(Sites[ii].LLH[0]),str(Sites[ii].LLH[1])))
  except FileNotFoundError:
    sys.stderr.write("An error setting up the cgeoid input file and EGM data will not be used for geoid height comparison figures.\n")
  try:
    cmd = []
    cmd.append(cgeoid)
    cmd.append("--file")
    cmd.append(cgeoidInputFileName)
    cmd.append("--log")
    cmd.append(cgeoidOutputFileName)
    process = subprocess.Popen(cmd,stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
  except IOError:
    sys.stderr.write("An error occurred executing cgeoid and EGM data will not be used for geoid height comparison figures.\n")
  try:
    with open(cgeoidOutputFileName, 'r') as cgeoidData:
      output=cgeoidData.readlines()
      cnt = 0
      for line in output:
        linesplt=line.split()
        pos=0
        try: pos=linesplt.index('DOV')
        except ValueError: pass
        if pos>1: 
          Sites[cnt].undEGM = float(linesplt[pos-1])
          cnt += 1
  except IOError:
    sys.stderr.write("An error occurred with reading cgeoid output and EGM data will not be used for geoid height comparison figures.\n")
  #Clean up temporary files
  try: 
    os.path.isfile(cgeoidInputFileName)
    try: os.remove(cgeoidInputFileName)
    except FileNotFoundError: pass
  except NameError: pass
  try:
    os.path.isfile(cgeoidOutputFileName)
    try: os.remove(cgeoidOutputFileName)
    except FileNotFoundError: pass
  except NameError: pass

def fillUndEGMGrid():
  try:
    cmd = []
    cmd.append(cgeoid)
    cmd.append("--file")
    cmd.append(cgeoidInputFileName)
    cmd.append("--log")
    cmd.append(cgeoidOutputFileName)
    process = subprocess.Popen(cmd,stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
  except IOError:
    sys.stderr.write("An error occurred executing cgeoid and EGM data will not be used for geoid height comparison figures.\n")
  try:
    with open(cgeoidOutputFileName, 'r') as cgeoidData:
      output=cgeoidData.readlines()
      cnt = 0
      for line in output:
        linesplt=line.split()
        pos=0
        try: pos=linesplt.index('DOV')
        except ValueError: pass
        if pos>1: 
          GridPts[cnt].undEGM = float(linesplt[pos-1])
          cnt += 1
  except IOError:
    sys.stderr.write("An error occurred with reading cgeoid output and EGM data will not be used for geoid height comparison figures.\n")

  #Clean up temporary files
  try:
    os.path.isfile(cgeoidInputFileName)
    try: os.remove(cgeoidInputFileName)
    except FileNotFoundError: pass
  except NameError: pass
  try:
    os.path.isfile(cgeoidOutputFileName)
    try: os.remove(cgeoidOutputFileName)
    except FileNotFoundError: pass
  except NameError: pass

##############################
###         Classes        ###
##############################

#Class for sites
class Site(object):
  def __init__(self):
    self.name = ""
    self.LLH = (0.,0.,0.)
    self.undEGM = 0.
    self.undInput = 0.
  @property
  def rLLH(self):
    return (m.radians(self.LLH[0]),m.radians(self.LLH[1]),self.LLH[2])
  @property
  def dist(self):
    var1 = self.LLH[0] - params.originLLH[0]
    #check for lon wrapping
    tmp = self.LLH[1] - params.originLLH[1]
    #Origin on other side of wrap
    if (tmp>180.): var2=self.LLH[1]-(360.+params.originLLH[1])
    #Point on other side of wrap
    elif (tmp<-180.): var2=(360.+self.LLH[1])-params.originLLH[1]
    else: var2=tmp 
    return (var1,var2)
  @property
  def undPoly(self):
    (var1,var2) = self.dist
    und, ii = 0,0
    for var1pow in range(params.polyDeg+1):
      for var2pow in range(params.polyDeg+1-var1pow):
        und += params.poly[ii]*pow(var1,var1pow)*pow(var2,var2pow)
        ii += 1
    return und
  @property
  def offsetVector(self):
    var1,var2 = self.dist
    offsetVector = []
    for var1pow in range(params.polyDeg+1):
      for var2pow in range(params.polyDeg+1-var1pow):
        offsetVector.append((var1**var1pow)*(var2**var2pow))
    return offsetVector

#Class for grid points
class GridPoint(object):
  def __init__(self):
    self.LLH = (0.,0.,0.)
    self.undEGM = 0.
  @property
  def dist(self):
    var1 = self.LLH[0] - params.originLLH[0]
    #check for lon wrapping
    tmp = self.LLH[1] - params.originLLH[1]
    #Origin on other side of wrap
    if (tmp>180.): var2=self.LLH[1]-(360.+params.originLLH[1])
    #Point on other side of wrap
    elif (tmp<-180.): var2=(360.+self.LLH[1])-params.originLLH[1]
    else: var2=tmp 
    return (var1,var2)
  @property
  def undPoly(self):
    (var1,var2) = self.dist
    und, ii = 0,0
    for var1pow in range(params.polyDeg+1):
      for var2pow in range(params.polyDeg+1-var1pow):
        und += params.poly[ii]*pow(var1,var1pow)*pow(var2,var2pow)
        ii += 1
    return und

def xyz2llh(X):
  lons = m.atan2(X[1],X[0])
  D = (X[0]**2+X[1]**2)**(0.5)
  for ii in range(4):
    lats = m.atan2(X[2],D*(1-params.e2))
    N = params.aWGS84/((1.-params.e2*(m.sin(lats))**2.)**(0.5))
    lat2 = m.atan2(X[2]+params.e2*N*m.sin(lats),D)
    lats = lat2
  h = (D/m.cos(lats))-N
  return m.degrees(lats),m.degrees(lons),h

##############################
###       Parse File       ###
##############################  

def parseFile(fname):
  Sites.append(Site())
  ftype = (os.path.splitext(fname)[-1]).replace('.','')
  # IOB input files
  if ftype == 'iob':
    try:
      with open(fname,'r') as fin:
        flines = fin.readlines()
        cnt = 0 #site number
        undRec = 1 #placeholder for ``siteName'' for UND records
        for line in flines:
          if re.match('GEOI',line.lstrip()):
            try:
              for ii in range(len(Sites)):
                if Sites[ii].name == line[10:21].strip():
                  cnt = ii
                  break
                elif ii == (len(Sites) - 1):
                  Sites.append(Site())
                  cnt = len(Sites) - 1
              Sites[cnt].undInput = float(line[59:71].strip())
              Sites[cnt].name = line[10:22].strip()
              cnt = len(Sites)
            except TypeError: 
              sys.stderr.write('Encountered a Geoid Height which is non-numerical.\n'+line+'\n')
              exit(1)
          elif re.match('UND',line.lstrip()):
            try:
              Sites.append(Site())
              cnt = len(Sites)-1
              Sites[cnt].undInput = float(line[10:22].strip())
              Sites[cnt].name = "undRec "+str(undRec)
              undRec +=1
            except TypeError: 
              sys.stderr.write('Encountered a Geoid Height which is non-numerical.\n'+line+'\n')
              exit(1)
            tmpLat=dms2decGeoLab(line,25,27,28,30,31,40)
            if(line[23].lower() == 's'): tmpLat*=-1.
            tmpLon=dms2decGeoLab(line,42,45,46,48,49,60)
            if not (line[41].lower() == 'e'):tmpLon=360.-tmpLon
            while (tmpLon<0.): tmpLon+=360.
            Sites[cnt].LLH = (tmpLat,tmpLon%360.,0.)
            cnt = len(Sites)
          elif re.match('PL',line.lstrip()) or re.match('PLH',line.lstrip()) or re.match('PLO',line.lstrip()):
            for ii in range(len(Sites)):
              if Sites[ii].name == line[10:21].strip():
                cnt = ii
                break
              elif ii == (len(Sites) - 1):
                Sites.append(Site())
                cnt = len(Sites) - 1
            Sites[cnt].name = line[10:21].strip()
            tmpLat=dms2decGeoLab(line,25,27,28,30,31,40)
            if(line[23].lower() == 's'): tmpLat*=-1.
            tmpLon=dms2decGeoLab(line,42,45,46,48,49,60)
            if not (line[41].lower() == 'e'):tmpLon=360.-tmpLon
            while (tmpLon<0.): tmpLon+=360.
            Sites[cnt].LLH = (tmpLat,tmpLon%360.,0.)
          elif re.match('XYZ',line.lstrip()):
            for ii in range(len(Sites)):
              if Sites[ii].name == line[10:21].strip():
                cnt = ii
                break
              elif ii == (len(Sites) - 1):
                Sites.append(Site())
                cnt = len(Sites) - 1
            Sites[cnt].name = line[10:21].strip()
            if Sites[cnt].name=='':
              sys.stderr.write('Encountered an XYZ record without a name.\n'+line+'\n')
              exit(1)
            try:tmplat,tmplon,tmph = xyz2llh((float(line[23:41]),float(line[42:60]),float(line[61:79])))
            except TypeError: 
              sys.stderr.write('Encountered a XYZ which is non-numerical.\n'+line+'\n')
              exit(1)
            if (tmplon < 0): tmplon += 360.
            Sites[cnt].LLH = (tmplat,tmplon,tmph)
            cnt = len(Sites)
    except IOError:
      sys.stderr.write('Could not open input file\n')
      exit(1)

  # GEOID input files
  elif ftype == 'geoid':
    try: 
      cnt = 1
      with open(fname,'r') as fin:
        flines = fin.readlines()
        for line in flines:
          if (re.match('//',line.lstrip())) or (re.match('\#',line.lstrip())) or (re.match('\*',line.lstrip())) or not (line.strip()):
            pass # skip over line with comment or empty line
          else:
            linesplt0 = re.split('\,',line.strip())
            linesplt = []
            for ele in linesplt0: linesplt.append(ele.strip())
            flagPosLat,flagN,flagW = 1,1,1 #Positive lat, Northern lat, Eastern lon given
            if ('S' in linesplt) or ('s' in linesplt): flagN = -1 #Southern Lat specified
            if ('-' in linesplt): flagPosLat = -1 #Negative Lat specified
            if(3<len(linesplt)<8): #Valid format
              Sites.append(Site())
              Sites[cnt].name = linesplt[0]
              try:Sites[cnt].undInput = float(linesplt[-1])
              except TypeError: 
                sys.stderr.write('Encountered a Geoid Height which is non-numerical.\n'+line+'\n')
                exit(1)
            if(3<len(linesplt)<8): #decimal degrees specification
              indexLat = -3 #start by assuming E/W not specified
              try: tmpLon=float(linesplt[-2])
              except TypeError: 
                sys.stderr.write('Encountered a Longitude which is non-numerical.\n'+line+'\n')
                exit(1)
              if ('E' in linesplt) or ('e' in linesplt):indexLat -= 1 #E lon
              elif ('W' in linesplt) or ('w' in linesplt): #W lon
                tmpLon=(360. - tmpLon)
                indexLat -= 1
              while (tmpLon<0.): tmpLon+=360.
              try: tmpLat=float(linesplt[indexLat])
              except TypeError: 
                sys.stderr.write('Encountered a Latitude which is non-numerical.\n'+line+'\n')
                exit(1)
              tmpLat = float(flagPosLat*flagN*tmpLat) #-3 if no E/W specification, else -4
              Sites[cnt].LLH = (tmpLat,tmpLon%360.,0) 
              cnt+=1
            elif(7<len(linesplt)<12): #decimal degrees specification
              Sites.append(Site())
              Sites[cnt].name = linesplt[0]
              try:Sites[cnt].undInput = float(linesplt[-1])
              except (TypeError, ValueError):
                sys.stderr.write('Encountered a Geoid Height which is non-numerical.\n'+line+'\n')
                exit(1)
              indexLat = -7 #start by assuming E/W not specified
              try:tmpLonDeg,tmpLonMin,tmpLonSec=float(linesplt[-4]),float(linesplt[-3])/60.,float(linesplt[-2])/3600.
              except (TypeError, ValueError):
                sys.stderr.write('Encountered a Longitude which is non-numerical.\n'+line+'\n')
                exit(1)
              if ((tmpLonMin<0.) or (tmpLonSec<0.)):
                sys.stderr.write('Negative signs on minutes and seconds (dms) are not supported. Please place the negative sign on the degrees.\n'+line+'\n')
                exit(1)
              if (tmpLonDeg>=0.):tmpLon = tmpLonDeg+tmpLonMin+tmpLonSec
              else:tmpLon = -(-tmpLonDeg+tmpLonMin+tmpLonSec)
              if ('E' in linesplt) or ('e' in linesplt):indexLat -= 1 #E lon
              elif ('W' in linesplt) or ('w' in linesplt): #W lon
                tmpLon=360. - float(tmpLon)
                indexLat -= 1
              while (tmpLon<0.): tmpLon+=360.
              try:tmpLatDeg,tmpLatMin,tmpLatSec=float(linesplt[indexLat]),float(linesplt[indexLat+1])/60.,float(linesplt[indexLat+2])/3600.
              except TypeError: 
                sys.stderr.write('Encountered a Latitude which is non-numerical.\n'+line+'\n')
                exit(1)
              if ((tmpLatMin<0.) or (tmpLatSec<0.)):
                sys.stderr.write('Negative signs on minutes and seconds (dms) are not supported. Please place the negative sign on the degrees.\n'+line+'\n')
                exit(1)
              if (tmpLatDeg>=0.):tmpLat = tmpLatDeg+tmpLatMin+tmpLatSec
              else:tmpLat = -(-tmpLatDeg+tmpLatMin+tmpLatSec)
              tmpLat = float(flagPosLat*flagN*np.sign(tmpLat)*(abs(tmpLat)%360.)) #-7 if no E/W specification, else -8
              Sites[cnt].LLH = (tmpLat,tmpLon%360.,0)
              cnt+=1
            else:
              sys.stderr.write('Invalid number of columns in line.\n'+str(cnt)+'\n')
              exit(1)
    except IOError:
      sys.stderr.write('Could not open input file\n')
      exit(1)
  else: 
    sys.stderr.write('Could not read input file\n')
    exit(1)


##############################
###         Parse          ###
##############################  

parser=arg.ArgumentParser(description="Create Custom Geoid File.")
parser.add_argument("--data",action='store',dest='fileName',required=False)
parser.add_argument("--outDir",action='store',dest='destDir',required=False)
parser.add_argument("--cgeoid",action='store',dest='cgeoid',required=False)
parser.add_argument("--egmFile",action='store',dest='egmFile',required=False)
args = parser.parse_args()

if args.fileName==None:
  sys.stderr.write('No input data given.')
  exit(1)
else: fileName=args.fileName
projName =re.split('\.',os.path.split(fileName)[-1])[0]

#Attempt to parse file
parseFile(fileName)
Sites.pop(0)
#for ii in range(len(Sites)):
#  sys.stderr.write(Sites[ii].name+' '+str(Sites[ii].LLH[0])+' '+str(Sites[ii].LLH[1])+' '+str(Sites[ii].undInput)+'\n')
#exit(1)

if args.egmFile==None:
  sys.stderr.write('No input EGM geoid file given.')
  exit(1)
else:egmFile=args.egmFile

if args.cgeoid==None:
  sys.stderr.write('No cgeoid path given. EGM data will not be used for geoid height comparison figures.\n')
else:cgeoid=args.cgeoid

if args.destDir==None:
  sys.stderr.write('File will be saved in same directory as input data file\n')
  destDir = os.path.splitext(fileName)[0]  #projName included
  projName = re.split('\/',destDir)[-1]
  projDir = re.split('\/',destDir)[:-1]
  projDir = "/".join(projDir)+"/"
  destDir = projDir
else:
  destDir=args.destDir
  if(re.split('/',destDir)[-1]=="geoid"): projDir = destDir[:-5]
  else: projDir = destDir
  if not (projDir[-1]=="/"): projDir+="/"

cgeoidInputFileName = projDir+'tmp_cgeoid_cmd.txt'
cgeoidOutputFileName = projDir+'tmp_cgeoid_data.txt'

#Make destDir if doesn't exist
if not os.path.exists(destDir): os.makedirs(destDir)

# H5 file will have essentially the same name as the data file
h5File= destDir + '/' + projName + '_custom_geoid.h5'

##############################
###       Site Unds        ###
##############################  

# Get polynomial coefficients
params.findCoeffs
sys.stdout.write(str(len(Sites)) + ' records were used to create the region polynomial\n')

# Get EGM values for each site
fillUndEGM()

#Residuals information
gResidual,rawHgts = ([] for ii in range(2))
for ii in range(len(Sites)):
  rawHgts.append((Sites[ii].undPoly,Sites[ii].undInput,Sites[ii].undEGM))
  gResidual.append(Sites[ii].undPoly - Sites[ii].undInput)

##############################
###       Grid Unds        ###
##############################  

# set number of rows and columns
numrows = int(m.ceil(abs(params.LatLonMinMax[2] - params.LatLonMinMax[0]) / params.spacingDegrees))
numcols = int(m.ceil(abs(params.LatLonMinMax[3] - params.LatLonMinMax[1]) / params.spacingDegrees))

# create the custom grid file using the HDF format
f = h5py.File(h5File,'w')

try:
  lat,lon = params.LatLonMinMax[2],params.LatLonMinMax[1]
  newGrid,rowGrid = ([] for ii in range(2))

  # actual grid population in row major order starting at the top left corner
  with open(cgeoidInputFileName, 'w') as cgeoidInput:
    cgeoidInput.write("--latmin {0}\n".format(str(params.LatLonMinMax[0])))
    cgeoidInput.write("--latmax {0}\n".format(str(params.LatLonMinMax[2])))
    cgeoidInput.write("--lonmin {0}\n".format(str(params.LatLonMinMax[1])))
    cgeoidInput.write("--lonmax {0}\n".format(str(params.LatLonMinMax[3])))
    cgeoidInput.write("--gfile \"{0}\"\n".format(egmFile))
    cnt = 0
    for r in range(numrows):
      for c in range(numcols):
        GridPts.append(GridPoint())
        GridPts[cnt].LLH = (lat,lon,0)
        cgeoidInput.write("--pos {0}:{1}\n".format(str(lat),str(lon)))
        rowGrid.append(round(GridPts[cnt].undPoly,6))
        lon += params.spacingDegrees
        cnt+=1
      lat -= params.spacingDegrees
      lon -= params.spacingDegrees*numcols
      newGrid.append(rowGrid)
      rowGrid = []

  # create a dataset within our HDF5 file to store our data
  custom = f.create_dataset('Custom Geoid Heights',(numrows,numcols),dtype = 'f', data = newGrid)
  custom.attrs['Description'] = np.string_("These are the geoid heights derived from the fitting polynomial.")
  custom.attrs['Format'] = np.string_("Rows correspond to different latitudes, from North to South.  Columns correspond to different longitudes, from West to East.  The spacing between rows is given in minutes by gridSpacingMinutes.")
  custom.attrs['Geoid Height Units'] = np.string_("m")
  custom.attrs['Geoid Height Polynomial: Geoid Height'] = np.string_(params.polyStr)
  strresid2='N/A'
  try:
    if len(params.sresid2)==0: params.strresid2="N/A"
    else: strresid2='{:.6f}'.format(0.0+np.round(params.sresid2[0],precision))
  except TypeError: strresid2='{:.6f}'.format(0.0+np.round(params.sresid2,precision))
  custom.attrs['Polynomial Origin Point Used and Sum of Residuals Squared'] = np.string_('('+params.originN+',   '+strresid2+')')
  custom.attrs['Number of (Rows,Columns)'] = np.string_('('+str(numrows)+',   '+str(numcols)+')')
  custom.attrs['Latitude N (Minimum,Maximum) in Decimal Degrees'] = np.string_('('+'{:.6f}'.format(0.0+np.round(params.LatLonMinMax[0],precision))+',   '+'{:.6f}'.format(0.0+np.round(params.LatLonMinMax[2],precision))+')')
  custom.attrs['Longitude E (Minimum,Maximum) in Decimal Degrees'] = np.string_('('+'{:.6f}'.format(0.0+np.round(params.LatLonMinMax[1],precision))+',   '+'{:.6f}'.format(0.0+np.round(params.LatLonMinMax[3],precision))+')')
  custom.attrs['Grid Spacing (Minutes)'] = round(params.spacingDegrees * 60.,6)
  custom.attrs['Data Type (4 byte float)'] = np.string_('f')
  
  fillUndEGMGrid()

##############################
###        MakePlots       ###
##############################

  # make a list plot
  try:
    fig = plt.figure()
    fig.patch.set_facecolor('white')
    ax = fig.add_axes([0.14, 0.1, 0.82, 0.82])
    ax.grid(True)
    ax.get_yaxis().get_major_formatter().set_useOffset(False)
    ax.set_ylabel("Geoid Height (m)", fontweight="bold")
    ax.set_xlabel("Each point is a different input site", fontweight="bold")
    rawHgtsSorted = np.array(rawHgts)
    rawHgtsSorted = rawHgtsSorted[rawHgtsSorted[:,1].argsort()[::-1]]
    ax.plot(rawHgtsSorted[:,0],marker='o', linestyle='--',label='Polynomial',color='b')
    ax.plot(rawHgtsSorted[:,1],marker='o', linestyle='--',label='Input',color='r')
    ax.plot(rawHgtsSorted[:,2],marker='o', linestyle='--',label='EGM08',color='m')
    ax.legend(bbox_to_anchor=(0.5, 1.1), loc='upper center', ncol=3)
    fig.canvas.draw()
	
    # make an image dataset
    data = np.frombuffer(fig.canvas.tostring_rgb(), dtype=np.uint8)
    image = data.reshape(fig.canvas.get_width_height()[::-1] + (3,))

    # convert image to dataset with corresponding lookup table
    width,height,colors = image.shape[0],image.shape[1],image.shape[2]
    img = f.create_dataset('Geoid Heights Comparison',(width,height,colors), dtype = np.uint8, data = image)
    img.attrs['Description'] = np.string_("This list plot displays the polynomial derived height, spot-specified input height and the EGM08 derived height for each of the input sites.")
    img.attrs["CLASS"] = np.string_("IMAGE")
    img.attrs["IMAGE_SUBCLASS"] = np.string_("IMAGE_TRUECOLOR")
    img.attrs["IMAGE_VERSION"] = np.string_("1.2")
    img.attrs["IMAGE_COLORMODEL"] = np.string_("RGB")
    img.attrs["INTERLACE_MODE"] = np.string_("INTERLACE_PIXEL")
    plt.close()
  except ValueError: sys.stderr.write('An error occurred when creating the geoid height comparison plots (with EGM08).\n')
  
  try:
    diffPolyEGM = []
    for ii in range(len(newGrid)):
      newRow = []
      for jj in range(len(newGrid[0])):
        newRow.append(newGrid[ii][jj]-GridPts[len(newGrid[0])*ii+jj].undEGM)
      diffPolyEGM.append(newRow)
    fig = plt.figure(tight_layout=True)
    fig.patch.set_facecolor('white')
    ax1 = fig.add_subplot(111)
    levels = np.linspace(np.amin(diffPolyEGM), np.amax(diffPolyEGM), num = 20)
    CS = ax1.contour(diffPolyEGM, levels, origin='upper')
    ax1.clabel(CS, levels, inline=1, fmt='%1.3f')
    ax1.set_ylabel("Latitude N (Decimal Degrees)", fontweight="bold")
    ax1.set_xlabel("Longitude E (Decimal Degrees)", fontweight="bold")
    y = np.round(np.linspace(params.LatLonMinMax[0], params.LatLonMinMax[2], 5),3)
    ypos = np.round(np.linspace(1,numrows,5),0)
    x = np.round(np.linspace(params.LatLonMinMax[1], params.LatLonMinMax[3], 5),3)
    x = [num%360. for num in x]
    xpos = np.round(np.linspace(1,numcols,5),0)
    ax1.set_yticks(ypos)
    ax1.set_yticklabels(y)
    ax1.set_xticks(xpos)
    ax1.set_xticklabels(x)
    ax2=ax1.twiny()
    ax2.set_xlim(ax1.get_xlim())
    xTOP = [(num-360.) for num in x]
    ax2.set_xticks(xpos)
    ax2.set_xticklabels(xTOP)
    ax1.autoscale(False)
    ax1.set_title("Polynomial Derived Heights - EGM08\n", fontweight="bold")
    (inputRows,inputCols) = ([] for ii in range(2))
    for ii in range(len(Sites)):
      col = (Sites[ii].LLH[0]-params.LatLonMinMax[0])/params.spacingDegrees
      row = min(abs(Sites[ii].LLH[1]-params.LatLonMinMax[1]),(Sites[ii].LLH[1]-params.LatLonMinMax[1]+360.))/params.spacingDegrees
      inputCols.append(col)
      inputRows.append(row)
	
    # add input points on top	
    ax1.scatter(inputRows,inputCols,zorder = 1)
    fig.canvas.draw()
  
    # make an image dataset
    data = np.frombuffer(fig.canvas.tostring_rgb(), dtype=np.uint8)
    image = data.reshape(fig.canvas.get_width_height()[::-1] + (3,))

    # convert image to dataset with corresponding lookup table
    width,height,colors = image.shape[0],image.shape[1],image.shape[2]
    img = f.create_dataset('Geoid EGM Offset',(width,height,colors), dtype = np.uint8, data = image)
    img.attrs['Description'] = np.string_("This contour plot displays the geoid height difference (m) between the local patch and EGM.")
    img.attrs["CLASS"] = np.string_("IMAGE")
    img.attrs["IMAGE_SUBCLASS"] = np.string_("IMAGE_TRUECOLOR")
    img.attrs["IMAGE_VERSION"] = np.string_("1.2")
    img.attrs["IMAGE_COLORMODEL"] = np.string_("RGB")
    img.attrs["INTERLACE_MODE"] = np.string_("INTERLACE_PIXEL")
    plt.close()
  except ValueError: sys.stderr.write('An error occurred when creating the geoid contour plot.\n')
  resGrp = f.create_group("Input Data Residuals")
  
  # make a spreadsheet of geoid heights for each site
  newGridTable = []
  numrowsTable=len(Sites)
  rowGrid=[]
  rowGrid.append(np.string_('Site Table'.encode("ascii","ignore")))
  rowGrid.append(np.string_('Input'.encode("ascii","ignore")))
  rowGrid.append(np.string_('Custom'.encode("ascii","ignore")))
  dtype0=np.dtype([('Site Table','S10'),('Latitude','S10'),('Longitude','S10'),('Input','S10'),('Custom','S10'),('EGM','S10'),('Custom-Input','S10'),('Custom-EGM','S10')])
  for ii in range(len(rawHgts)):
    rowGrid=[]
    rowGrid.append(np.string_(Sites[ii].name.encode("ascii","ignore")))
    rowGrid.append(np.string_('{:.6f}'.format(0.0+np.round(Sites[ii].LLH[0],precision)).encode("ascii","ignore")))
    rowGrid.append(np.string_('{:.6f}'.format(0.0+np.round(Sites[ii].LLH[1],precision)).encode("ascii","ignore")))
    rowGrid.append(np.string_('{:.6f}'.format(0.0+np.round(rawHgts[ii][1],precision)).encode("ascii","ignore")))
    rowGrid.append(np.string_('{:.6f}'.format(0.0+np.round(rawHgts[ii][0],precision)).encode("ascii","ignore")))
    rowGrid.append(np.string_('{:.6f}'.format(0.0+np.round(rawHgts[ii][2],precision)).encode("ascii","ignore")))
    rowGrid.append(np.string_('{:.6f}'.format(0.0+np.round(gResidual[ii],precision)).encode("ascii","ignore")))
    rowGrid.append(np.string_('{:.6f}'.format(0.0+np.round(rawHgts[ii][0]-rawHgts[ii][2],precision)).encode("ascii","ignore")))
    newGridTable.append(rowGrid)
  data = np.array(newGridTable)
  table = resGrp.create_dataset('Site Table', (numrowsTable,1),dtype=dtype0)
  for i in range(0,numrowsTable):
    table[i]=tuple(newGridTable[i])
  table.attrs['Description'] = np.string_("The Input Geoid Height, Custom Geoid Height, EGM Geoid Height (if available), Difference between Custom Geoid Height and Input Geoid Height, Difference between Custom Geoid Height and EGM Geoid Height (if available).")
 
  # make a histogram
  try:
    fig = plt.figure(tight_layout=True)
    fig.patch.set_facecolor('white')
    plt.grid(True)
    plt.ylabel("Number of Sites", fontweight="bold")
    plt.xlabel("Polynomial derived height (m) - Input geoid height (m)", fontweight="bold")
    hist, bins = np.histogram(gResidual, bins=50)
    width = 0.7 * (bins[1] - bins[0])
    center = (bins[:-1] + bins[1:]) / 2
    plt.bar(center, hist, align='center', width=width)
    fig.canvas.draw()
	
    # make an image dataset
    data = np.frombuffer(fig.canvas.tostring_rgb(), dtype=np.uint8)
    image = data.reshape(fig.canvas.get_width_height()[::-1] + (3,))

    # convert image to dataset with corresponding lookup table
    width,height,colors = image.shape[0],image.shape[1],image.shape[2]
    img = resGrp.create_dataset('Histogram',(width,height,colors), dtype = np.uint8, data = image)
    img.attrs['Description'] = np.string_("This histogram displays the difference between the polynomial derived height for the input sites and the spot-specified geoid heights for the input sites.")
    img.attrs["CLASS"] = np.string_("IMAGE")
    img.attrs["IMAGE_SUBCLASS"] = np.string_("IMAGE_TRUECOLOR")
    img.attrs["IMAGE_VERSION"] = np.string_("1.2")
    img.attrs["IMAGE_COLORMODEL"] = np.string_("RGB")
    img.attrs["INTERLACE_MODE"] = np.string_("INTERLACE_PIXEL")
    plt.close()
  except ValueError: sys.stderr.write('An error occurred when creating the histogram of residuals.\n')

  # make a residuals list plot
  try:
    fig = plt.figure()
    fig.patch.set_facecolor('white')
    ax = fig.add_axes([0.14, 0.1, 0.82, 0.82])
    ax.grid(True)
    ax.get_yaxis().get_major_formatter().set_useOffset(False)
    gResidual.sort(reverse=True)
    ax.set_ylabel("Polynomial derived height (m) - Input geoid height (m)", fontweight="bold")
    ax.set_xlabel("Each point is a different input site", fontweight="bold")
    ax.plot(gResidual,marker='o', linestyle='--',label='Geoid Height Residuals (m)',color='b')
    ax.legend(bbox_to_anchor=(0.5, 1.1), loc='upper center', ncol=1)
    fig.canvas.draw()

    # make an image dataset
    data = np.frombuffer(fig.canvas.tostring_rgb(), dtype=np.uint8)
    image = data.reshape(fig.canvas.get_width_height()[::-1] + (3,))

    # convert image to dataset with corresponding lookup table
    width,height,colors = image.shape[0],image.shape[1],image.shape[2]
    img = resGrp.create_dataset('List Plot',(width,height,colors), dtype = np.uint8, data = image)
    img.attrs['Description'] = np.string_("This list plot displays the difference between the polynomial derived height for the input sites and the spot-specified geoid heights for the input sites (residuals).")
    img.attrs["CLASS"] = np.string_("IMAGE")
    img.attrs["IMAGE_SUBCLASS"] = np.string_("IMAGE_TRUECOLOR")
    img.attrs["IMAGE_VERSION"] = np.string_("1.2")
    img.attrs["IMAGE_COLORMODEL"] = np.string_("RGB")
    img.attrs["INTERLACE_MODE"] = np.string_("INTERLACE_PIXEL")
    plt.close()
  except ValueError: sys.stderr.write('An error occurred when creating the residuals list plot.\n')

  # make a list plot
  try:
    fig = plt.figure()
    fig.patch.set_facecolor('white')
    ax = fig.add_axes([0.14, 0.1, 0.82, 0.82])
    ax.grid(True)
    ax.get_yaxis().get_major_formatter().set_useOffset(False)
    ax.set_ylabel("Geoid Height (m)", fontweight="bold")
    ax.set_xlabel("Each point is a different input site", fontweight="bold")
    rawHgtsSorted = np.array(rawHgts)
    rawHgtsSorted = rawHgtsSorted[rawHgtsSorted[:,1].argsort()[::-1]]
    ax.plot(rawHgtsSorted[:,0],marker='o', linestyle='--',label='Polynomial',color='b')
    ax.plot(rawHgtsSorted[:,1],marker='o', linestyle='--',label='Input',color='r')
    ax.legend(bbox_to_anchor=(0.5, 1.1), loc='upper center', ncol=2)
    fig.canvas.draw()

    # make an image dataset
    data = np.frombuffer(fig.canvas.tostring_rgb(), dtype=np.uint8)
    image = data.reshape(fig.canvas.get_width_height()[::-1] + (3,))

    # convert image to dataset with corresponding lookup table
    width,height,colors = image.shape[0],image.shape[1],image.shape[2]
    img = resGrp.create_dataset('Input Geoid Heights',(width,height,colors), dtype = np.uint8, data = image)
    img.attrs['Description'] = np.string_("This list plot displays the polynomial derived height and spot-specified input height for each of the input sites.")
    img.attrs["CLASS"] = np.string_("IMAGE")
    img.attrs["IMAGE_SUBCLASS"] = np.string_("IMAGE_TRUECOLOR")
    img.attrs["IMAGE_VERSION"] = np.string_("1.2")
    img.attrs["IMAGE_COLORMODEL"] = np.string_("RGB")
    img.attrs["INTERLACE_MODE"] = np.string_("INTERLACE_PIXEL")
    plt.close()
  except ValueError: sys.stderr.write('An error occurred when creating the geoid height comparison list plot (without EGM08).\n')

  if len(Sites)>1:
    # make a contour plot
    try:
      fig = plt.figure(tight_layout=True)
      fig.patch.set_facecolor('white')
      ax1 = fig.add_subplot(111)
      levels = np.linspace(np.amin(newGrid), np.amax(newGrid), num = 20)
      CS = ax1.contour(newGrid, levels, origin='upper')
      ax1.clabel(CS, levels, inline=1, fmt='%1.3f')
      ax1.set_ylabel("Latitude N (Decimal Degrees)", fontweight="bold")
      ax1.set_xlabel("Longitude E (Decimal Degrees)", fontweight="bold")
      y = np.round(np.linspace(params.LatLonMinMax[0], params.LatLonMinMax[2], 5),3)
      ypos = np.round(np.linspace(1,numrows,5),0)
      x = np.round(np.linspace(params.LatLonMinMax[1], params.LatLonMinMax[3], 5),3)
      x = [num%360. for num in x]
      xpos = np.round(np.linspace(1,numcols,5),0)
      ax1.set_yticks(ypos)
      ax1.set_yticklabels(y)
      ax1.set_xticks(xpos)
      ax1.set_xticklabels(x)
      ax2=ax1.twiny()
      ax2.set_xlim(ax1.get_xlim())
      xTOP = [(num-360.) for num in x]
      ax2.set_xticks(xpos)
      ax2.set_xticklabels(xTOP)
      ax1.autoscale(False)
      ax1.set_title("Geoid Heights (m) for the Local Patch\n", fontweight="bold")
      (inputRows,inputCols) = ([] for ii in range(2))
      for ii in range(len(Sites)):
        col = (Sites[ii].LLH[0]-params.LatLonMinMax[0])/params.spacingDegrees
        row = min(abs(Sites[ii].LLH[1]-params.LatLonMinMax[1]),(Sites[ii].LLH[1]-params.LatLonMinMax[1]+360.))/params.spacingDegrees
        inputCols.append(col)
        inputRows.append(row)
	
      # add input points on top	
      ax1.scatter(inputRows,inputCols,zorder = 1)
      fig.canvas.draw()
  
      # make an image dataset
      data = np.frombuffer(fig.canvas.tostring_rgb(), dtype=np.uint8)
      image = data.reshape(fig.canvas.get_width_height()[::-1] + (3,))

      # convert image to dataset with corresponding lookup table
      width,height,colors = image.shape[0],image.shape[1],image.shape[2]
      img = f.create_dataset('Geoid Contour Plot',(width,height,colors), dtype = np.uint8, data = image)
      img.attrs['Description'] = np.string_("This contour plot displays the geoid height (m) within the local patch.")
      img.attrs["CLASS"] = np.string_("IMAGE")
      img.attrs["IMAGE_SUBCLASS"] = np.string_("IMAGE_TRUECOLOR")
      img.attrs["IMAGE_VERSION"] = np.string_("1.2")
      img.attrs["IMAGE_COLORMODEL"] = np.string_("RGB")
      img.attrs["INTERLACE_MODE"] = np.string_("INTERLACE_PIXEL")
      plt.close()
    except ValueError: sys.stderr.write('An error occurred when creating the geoid contour plot.\n')

    # make a labeled contour plot
    try:
      fig = plt.figure(tight_layout=True)
      fig.patch.set_facecolor('white')
      ax1 = fig.add_subplot(111)
      levels = np.linspace(np.amin(newGrid), np.amax(newGrid), num = 20)
      CS = ax1.contour(newGrid, levels, origin='upper')
      ax1.clabel(CS, levels, inline=1, fmt='%1.3f')
      ax1.set_ylabel("Latitude N (Decimal Degrees)", fontweight="bold")
      ax1.set_xlabel("Longitude E (Decimal Degrees)", fontweight="bold")
      y = np.round(np.linspace(params.LatLonMinMax[0], params.LatLonMinMax[2], 5),3)
      ypos = np.round(np.linspace(1,numrows,5),0)
      x = np.round(np.linspace(params.LatLonMinMax[1], params.LatLonMinMax[3], 5),3)
      x = [num%360. for num in x]
      xpos = np.round(np.linspace(1,numcols,5),0)
      ax1.set_yticks(ypos)
      ax1.set_yticklabels(y)
      ax1.set_xticks(xpos)
      ax1.set_xticklabels(x)
      ax2=ax1.twiny()
      ax2.set_xlim(ax1.get_xlim())
      xTOP = [(num-360.) for num in x]
      ax2.set_xticks(xpos)
      ax2.set_xticklabels(xTOP)
      ax1.autoscale(False)
      ax1.set_title("Geoid Heights (m) for the Local Patch (Labeled)\n", fontweight="bold")
      (inputRows,inputCols) = ([] for ii in range(2))
      for ii in range(len(Sites)):
        col = (Sites[ii].LLH[0]-params.LatLonMinMax[0])/params.spacingDegrees
        row = min(abs(Sites[ii].LLH[1]-params.LatLonMinMax[1]),(Sites[ii].LLH[1]-params.LatLonMinMax[1]+360.))/params.spacingDegrees
        inputCols.append(col)
        inputRows.append(row)
	
      # add input points on top	
      ax1.scatter(inputRows,inputCols,zorder = 1)
      siteNames = []
      for ii in range(len(Sites)): siteNames.append(Sites[ii].name)
      for label,x,y in zip(siteNames,inputRows,inputCols):
        ax1.annotate(label,xy=(x,y),xytext=(0,1.2),textcoords = 'offset points', ha = 'right', va = 'bottom',size='6')
      fig.canvas.draw()
  
      # make an image dataset
      data = np.frombuffer(fig.canvas.tostring_rgb(), dtype=np.uint8)
      image = data.reshape(fig.canvas.get_width_height()[::-1] + (3,))

      # convert image to dataset with corresponding lookup table
      width,height,colors = image.shape[0],image.shape[1],image.shape[2]
      img = f.create_dataset('Geoid Contour Plot (Labeled)',(width,height,colors), dtype = np.uint8, data = image)
      img.attrs['Description'] = np.string_("This labeled contour plot displays the geoid height (m) within the local patch.")
      img.attrs["CLASS"] = np.string_("IMAGE")
      img.attrs["IMAGE_SUBCLASS"] = np.string_("IMAGE_TRUECOLOR")
      img.attrs["IMAGE_VERSION"] = np.string_("1.2")
      img.attrs["IMAGE_COLORMODEL"] = np.string_("RGB")
      img.attrs["INTERLACE_MODE"] = np.string_("INTERLACE_PIXEL")
      plt.close()
    except ValueError: sys.stderr.write('An error occurred when creating the labeled geoid contour plot.\n')
  
    sys.stdout.write('Custom geoid file created for the region \n')
except IOError:
  sys.stderr.write('A problem occurred when creating the custom grid file\n')
  if os.path.isfile(h5File): os.remove(h5File)

finally:
  if os.path.isfile(h5File):f.close()
