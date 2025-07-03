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
#Library of UTM,UPS, and MGRS coordinate conversions

#python imports
import math
import numpy as np


#Function to calc UTM zone number
#takes in LLH coordinates
#NGA.SIG.0012_2.0.0
#B. Beal 12/31/19
def zone(LLH):
    lonDD = LLH[0]  #lon in decimal degrees
    lonDD = round(float(lonDD),8)   #round to 8 places to match Geotrans
    latDD = LLH[1]  #lat in decimal degrees
    latDD = round(float(latDD),8)   #round to 8 places to match Geotrans
    
    #Sec 7.5
    if lonDD == 180:
        lonDD = -1*180
    if latDD>=84 or latDD<-80:
        print("Invalid Latitude Range")
        print("lon=",lonDD)
        exit(1)
    if lonDD>=180 or lonDD<-180:
        print("Invalid Longitude Range")
        print("lon=",lonDD)
        exit(1)
    Z = math.floor((lonDD+180)/6)+1
    if latDD<0:
        #Z=-1*Z #not needed with python  
        Z=Z    
    if Z==31 and latDD>=56 and latDD<64 and lonDD>=3:
        Z=32
    elif Z==32 and latDD>=72:  
        if lonDD<9:
            Z=31
        else:
            Z=33
    elif Z==34 and latDD>=72:
        if lonDD<21:
            Z=33
        else:
            Z=35
    elif Z==36 and latDD>=72:
        if lonDD<33:
            Z=35
        else:
            Z=37
    else:
        Z=Z
    return Z

#Function to calc Latitude Band Letter
#takes in LLH coordinates, but uses only lat_dd
#NGA.SIG.0012_2.0.0
#B. Beal 12/31/19
def letter(LLH):
    latDD = LLH[1]  #lat in decimal degrees
    latDD = round(float(latDD),8) #round to 8 places to match Geotrans
    i = math.floor(latDD/8)
        
    #Sec 11.7
    switcher={
            -11:'C',
            -10:'C',
            -9:'D',
            -8:'E',
            -7:'F',
            -6:'G',
            -5:'H',
            -4:'J',
            -3:'K',
            -2:'L',
            -1:'M',
            0:'N',
            1:'P',
            2:'Q',
            3:'R',
            4:'S',
            5:'T',
            6:'U',
            7:'V',
            8:'W',
            9:'X',
            10:'X',
            }
    return switcher.get(i,"Invalid Latitude")


#Function to append the N or S to the zone number
#12/21/2019 by B. Beal
#input parameters: zone number, latitude of point
def zoneLet(lat):
    if lat>=0:
        zn_let = 'N'
    else:
        zn_let = 'S'
    return zn_let

#Function to determine central longitude (central meridian) in UTM zone
#12/22/2019 by B. Beal
#input parameters: zone number
def cMeridian(zn):
    cMeridian = (zn - 1) * 6 - 180.0 + 3
    return cMeridian
  


    
#Function to take decimal degrees
#return degrees, minutes, seconds
#12/22/2019 by B. Beal    
def DDtoDMS(DD):
    d = np.trunc(DD)                  #degrees
    m = abs(np.trunc((DD-np.trunc(DD))*60))  #minutes 
    s = abs(((DD-np.trunc(DD))*60-np.trunc((DD-np.trunc(DD))*60)))*60    #seconds 
    return d, m, s

#From NGA.SIG.0012_2.0.0   
#01/04/2020 by B. Beal
def ellip_constants(ellip):
    if str(ellip) == "WGS 84": 
        #4.11 WGS 84 ellipsoid
        twolet = "WE" #NGA two-letter code 
        a = 6378137.0000000000000 #Semi-major axis 
        b = 6356752.3142451794976 #Semi-minor axis 
        finv = 298.25722356300000000 #Semi-minor axis 
        e = 0.081819190842621494335 #first eccentricity
        e2 = 0.0066943799901413169961 #First Eccentricity squared 
        R4 = 6367449.1458234153093 #Meridional isoperimetric radius 
        A2 = 8.3773182062446983032E-04
        A4 = 7.608527773572489156E-07
        A6 = 1.19764550324249210E-09
        A8 = 2.4291706803973131E-12
        A10 = 5.711818369154105E-15
        A12 = 1.47999802705262E-17
        B2 = -8.3773216405794867707E-04
        B4 = -5.905870152220365181E-08
        B6 = -1.67348266534382493E-10
        B8 = -2.1647981104903862E-13
        B10 = -3.787930968839601E-16
        B12 = -7.23676928796690E-19
        return twolet,a,b,finv,e,e2,R4,A2,A4,A6,A8,A10,A12,B2,B4,B6,B8,B10,B12
    elif str(ellip) == "GRS 1980":
        #4.12 GRS 80 ellipsoid
        twolet= "RF"
        a = 6378137.0000000000000
        b = 6356752.3141403558479
        finv = 298.25722210100000000    
        e = 0.081819191042815790146
        e2 = 0.0066943800229007876254
        R4 = 6367449.1457710475269
        A2 = 8.3773182472855134012E-04
        A4 = 7.608527848149655006E-07
        A6 = 1.19764552085530681E-09
        A8 = 2.4291707280369697E-12
        A10 = 5.711818509192422E-15
        A12 = 1.47999807059922E-17
        B2 = -8.3773216816203523672E-04
        B4 = -5.905870210369121594E-08
        B6 = -1.67348268997717031E-10
        B8 = -2.1647981529928124E-13
        B10 = -3.787931061803592E-16
        B12 = -7.23676950110361E-19       
        return twolet,a,b,finv,e,e2,R4,A2,A4,A6,A8,A10,A12,B2,B4,B6,B8,B10,B12
    else:
        return 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

#Compute the intermediate values cosChi and sinChi
#From NGA.SIG.0012_2.0.0   
#12/22/2019 by B. Beal
#input parameters: latitude in radians,eccentricity
def PhiToChi(phi,e):
    #Sec 2.8
    P = math.exp(e*math.atanh(e*math.sin(phi)))
    #print (P)
    t1 = (1+math.sin(phi))/P
    t2 = (1-math.sin(phi))*P
    cosChi = (2*math.cos(phi))/(t1+t2)
    sinChi = (t1-t2)/(t1+t2)
    return cosChi,sinChi


#Compute the transverse mercator coordinates for a point
#From NGA.SIG.0012_2.0.0   
#12/22/2019 by B. Beal
#input parameters: longitude, latitude in radians,ellipsoid
#Contains the f1, f2, f3, f4 functions
def LLHtoTM(lam,phi,ellip):
    #parameters for the ellipsoid
    twolet,a,b,finv,e,e2,R4,a2,a4,a6,a8,a10,a12,b2,b4,b6,b8,b10,b12 = ellip_constants(ellip)
    
    #Sec 3.2, Eq. 3-8
    cosChi, sinChi = PhiToChi(phi,e)
    u = math.atanh(cosChi*math.sin(lam))
    v = math.atan2(sinChi,cosChi*math.cos(lam)) #atan2(y,x)
    #Sec 3.3, Eq. 3-10
    cos2v = math.cos(2*v)
    sin2v = math.sin(2*v)    
    cos4v = (2*cos2v**2) - 1
    sin4v = 2*cos2v*sin2v
    cos6v = cos4v*cos2v - sin4v*sin2v
    sin6v = cos4v*sin2v + cos2v*sin4v
    #Sec 3.3, Eq. 3-11
    cos8v = (2*cos4v**2) - 1
    sin8v = 2*cos4v*sin4v
    cos10v = cos8v*cos2v - sin8v*sin2v
    sin10v = cos8v*sin2v + cos2v*sin8v
    cos12v = (2*cos6v**2) - 1
    sin12v = 2*cos6v*sin6v
    #Sec 3.3, Eq. 3-12
    cosh2u = math.cosh(2*u)
    sinh2u = math.sinh(2*u)
    cosh4u = (2*cosh2u**2) - 1
    sinh4u = 2*cosh2u*sinh2u    
    cosh6u = cosh2u*cosh4u + sinh2u*sinh4u
    sinh6u = cosh4u*sinh2u + cosh2u*sinh4u
    #Sec 3.3, Eq. 3-13
    cosh8u = (2*cosh4u**2) - 1
    sinh8u = 2*cosh4u*sinh4u
    cosh10u = cosh2u*cosh8u + sinh2u*sinh8u
    sinh10u = cosh8u*sinh2u + cosh2u*sinh8u
    cosh12u = (2*cosh6u**2) - 1
    sinh12u = 2*cosh6u*sinh6u
    #Sec 3.2, Eq. 3-7
    #f1 function:
    x = R4 * (u + a2*sinh2u*cos2v + a4*sinh4u*cos4v + a6*sinh6u*cos6v + 
              a8*sinh8u*cos8v + a10*sinh10u*cos10v + a12*sinh12u*cos12v)
    #f2 function
    y = R4 * (v + a2*cosh2u*sin2v + a4*cosh4u*sin4v + a6*cosh6u*sin6v + 
              a8*cosh8u*sin8v + a10*cosh10u*sin10v + a12*cosh12u*sin12v)
    
    #Sec 6.3:
    w = math.sqrt(1 - e**2 * math.sin(phi)**2)
    sig1 = 1 + 2*a2*cosh2u*cos2v + 4*a4*cosh4u*cos4v + 6*a6*cosh6u*cos6v + \
            8*a8*cosh8u*cos8v + 10*a10*cosh10u*cos10v + 12*a12*cosh12u*cos12v
    
    sig2 = 2*a2*sinh2u*sin2v + 4*a4*sinh4u*sin4v + 6*a6*sinh6u*sin6v + \
            8*a8*sinh8u*sin8v + 10*a10*sinh10u*sin10v + 12*a12*sinh12u*sin12v
    P = math.exp(e*math.atanh(e*math.sin(phi)))
    
    #f3 function:
    sigT1 = 2*(R4/a)*w*(math.cosh(u))*math.sqrt(sig1**2+sig2**2)
    sigT2 = (1+math.sin(phi))/P + (1-math.sin(phi))*P
    sig = sigT1/sigT2
    
    #f4 function:
    gam = math.atan2(sinChi*math.sin(lam),math.cos(lam)) + math.atan2(sig2,sig1)
        
    #Test values:
    #lon=-10deg, lat=3deg
    #cosChi = 0.998647785036631316 and sinChi = 0.0519865505821477812
    #u = -0.175183729646051084 and v = 0.0528108539283539197
    #x = -1 117 373.87527102019 and y = 336 868.939627688401    
    #print ('lam=', lam)
    #print ('phi=', phi)
    #print ("cosChi=", cosChi)
    #print ("sinChi=", sinChi)
    #print ("u=", u)
    #print ("v=", v)
    #print ("x=", x)
    #print ("y=", y)
    return x,y,sig,gam


#Compute the polar stereographic coordinates  for a point
#From NGA.SIG.0012_2.0.0   
#12/22/2019 by B. Beal
#input parameters: longitude, latitude in radians,ellipsoid
#Contains the f1, f2, f3, f4 functions
def LLHtoPS(lam,phi,ellip):
    #parameters for the ellipsoid
    twolet,a,b,finv,e,e2,R4,a2,a4,a6,a8,a10,a12,b2,b4,b6,b8,b10,b12 = ellip_constants(ellip)
        
    #Sec 3.2, Eq. 3-8
    cosChi, sinChi = PhiToChi(phi,e)
    
    #Sec 8.1, Eq. 8-19
    k90 = math.sqrt(1-e**2) * math.exp(e*math.atanh(e))
    print ('k90=',k90)
    #Sec 8.1, Eq. 8-18
    #f1 function:
    x = (2*a*math.sin(lam)*cosChi) / (k90*(1+sinChi))
    #f2 function:
    y = (-2*a*math.cos(lam)*cosChi) / (k90*(1+sinChi))
    #f3 function:
    sigT1 = 2*math.sqrt(1-e**2*math.sin(phi)**2) * math.exp(e*math.atanh(e*math.sin(phi)))
    sigT2 = k90*(1+math.sin(phi))
    sig = sigT1/sigT2
    #f4 function:
    gam = lam
    return x,y,sig,gam




#Compute the UTM coordinates for a point
#From NGA.SIG.0012_2.0.0   
#12/22/2019 by B. Beal
#input parameters: lat,lon,elHt as an array [lat,lon,elHt], zone number,ellipsoid
def LLHtoUTM(LLH,z,ellip):
    #define lat, lon
    lam_deg = LLH[0]    #longitude
    phi_deg = LLH[1]    #latitude
    #convert lat, lon to radians:
    phi = phi_deg * math.pi/180
    lam = lam_deg * math.pi/180
    
    #Sec. 7.1, Definition of UTM
    #Longitude of the origin (central meridian) in radians
    lam0 = cMeridian(z) * math.pi/180
    #lam_orig = lam0, =0 for UTM
    
    #central scale factor for UTM
    k0 = 0.9996
    
    #Latitude of the origin in radians, =0 for UTM
    #phi_orig = 0
    
    #X of the origin (false easting) for zone
    x_orig = 500000
   
    
    #Y of the origin (false northing) for zone
    if phi > 0:
        y_orig = 0        #northern hemisphere
    else:
        y_orig = 10000000 #southern hemisphere

    
    #Sec. 5.3, Eq. 17 
    #These equations are for general form.
    #x, y = LLHtoTM(lam_orig - lam0,phi_orig)
    #xCM = x_orig - k0*x
    #yEQ = y_orig - k0*y
    #For UTM, use below:
    xCM = x_orig
    yEQ = y_orig    
    
    #Sec 5.1, Eq. 15 (general for any transverse mercator mapping)
    x, y, sig, gam = LLHtoTM(lam - lam0,phi,ellip) 
    xUTM = k0*x + xCM
    yUTM = k0*y + yEQ
    #Sec 6.4
    sigUTM = k0*sig
    gamUTM = gam*180/math.pi #convert to degrees
    #Sec. 7.2 Example
    # lon=74deg, lat=3deg,zone 43,x= 388870.867643,y= 331643.938073
    # sig=0.999753, gam_deg=-0.052341 
    #print ('lam=',lam_deg)
    #print ('phi=',phi_deg)
    #print ('zone=',zn)
    #print ('lam0=',cMeridian(zn))
    #print ('xUTM=',xUTM)
    #print ('yUTM=',yUTM)
    #print ('sig=',sigUTM)
    #print ('gam=',gamUTM)
    return(xUTM,yUTM,sigUTM,gamUTM)

#Compute the UPS coordinates for a point
#From NGA.SIG.0012_2.0.0   
#12/22/2019 by B. Beal
#input parameters: lat,lon,elHt as an array [lat,lon,elHt], N/S flag (Z),ellipsoid
def LLHtoUPS(LLH,Z,ellip):
    #define lat, lon
    lam_deg = LLH[0]    #longitude
    phi_deg = LLH[1]    #latitude
    #convert lat, lon to radians:
    phi = phi_deg * math.pi/180
    lam = lam_deg * math.pi/180
    
    #Sec 10.1, UPS Constants
    lam0 = 0
    k0 = 0.994
    xPole = 2000000
    yPole = 2000000
    #Sec 9.1, Eq. 9-21
    if Z == 1:
        x, y, sig, gam = LLHtoPS(lam - lam0,phi,ellip) 
        xUPS = k0*x + xPole
        yUPS = k0*y + yPole
        sigUPS = k0*sig
        gamUPS = gam*180/math.pi #convert to degrees
        Z_str = 'N'
    elif Z == -1:
        x, y, sig, gam = LLHtoPS(lam - lam0,-1*phi,ellip) 
        xUPS = k0*x + xPole
        yUPS = -1*k0*y + yPole
        sigUPS = k0*sig
        gamUPS = -1*gam*180/math.pi #convert to degrees
        Z_str = 'S'
    else:
        xUPS, yUPS, sigUPS, gamUPS = (0,0,0,0,0)
    
    #Sec. 7.2 Example
    # lon=-179deg, lat=89deg,zone 1,x=1998062.320046 ,y= 2111009.610243
    # sig=0.994076, gam_deg=-179 
    #print ('Z=',Z)
    #print ('xUPS=',xUPS)
    #print ('yUPS=',yUPS)
    #rint ('sig=',sigUPS)
    #print ('gam=',gamUPS)
    return(xUPS,yUPS,sigUPS,gamUPS,Z_str) 

#Compute the curvature in the prime vertical for a point
#From DMATM 8358.2 "THE UNIVERSAL GRIDS: Universal Tansverse Mercator (UTM) and Universal Polar Stereographic (UPS)"  
#06/01/2020 by B. Beal
#input parameters: a, e2, latitude in radians
def curvMeridian(a,e2,phi):
    t1 = a*(1-e2)
    t2 = (1-e2*math.sin(phi)**2)**(3/2)
    rho = t1/t2    #pg 2-1
    return rho 

#Compute the curvature in the meridian for a point
#From DMATM 8358.2 "THE UNIVERSAL GRIDS: Universal Tansverse Mercator (UTM) and Universal Polar Stereographic (UPS)"  
#12/22/2019 by B. Beal
#input parameters: a, e2, latitude in radians
def curvPVertical(a,e2,phi):
    t1 = a
    t2 = 1 - e2*math.sin(phi)**2
    nu = t1/math.sqrt(t2)    #pg 2-1
    return nu 


#Elevation factor
#Accuracy of Elevation Reduction Factor,Earl F. Burkholder,1 Member, ASCE
#01/03/2020 by B. Beal
#input parameters: lon/lat in DD, height (ellipsoid or ortho), ellipsoid name
def ElevSF(LLH,ellip):
    #parameters for the ellipsoid
    twolet,a,b,finv,e,e2,R4,a2,a4,a6,a8,a10,a12,b2,b4,b6,b8,b10,b12 = ellip_constants(ellip)
    #define lat, lon,oheight
    phi_deg = LLH[1]    #latitude
    height = LLH[2]     #height
    #according to above source, ellipsoid height is acceptable for det. el factor
    #print (height)
    #convert lat, lon to radians:
    phi = phi_deg * math.pi/180
       
    #curvature in prime vertical
    #nu = curvPVertical(a,e2,phi)
    #Change 6/1/20 by Beal to use mean earth radius:
    r = a*math.sqrt(1-e2)/(1-e2*math.sin(phi)**2)
    elSF = r / (r + height)
    return elSF

#Elevation factor version 2
#Accuracy of Elevation Reduction Factor,Earl F. Burkholder,1 Member, ASCE
#05/31/2020 by B. Beal
#input parameters: lon/lat in DD, height (ellipsoid or ortho), a and invf values for any ellipsoid
def ElevSF2(LLH,a,invf):
    #define lat, lon, height
    phi_deg = LLH[1]    #latitude
    height = LLH[2]     #height
    #according to above source, ellipsoid height is acceptable for det. el factor
    #print (height)
    
    #convert lat, lon to radians:
    #phi_deg = 41.98097  #test value, see Meyer pg 67-68
    #M = 6364009, N= 6387710 checks!
    phi = phi_deg * math.pi/180
       
    #curvature in prime vertical
    f = 1/invf      #ellipsoid flattening
    e2 = f*(2-f)    #first eccentrity squared see 2.1 in NGA.SIG.0012_2.0.0 
    r = a*math.sqrt(1-e2)/(1-e2*math.sin(phi)**2)
    #print(r)

    elSF = r / (r + height)
    return elSF

#Compute the complet MGRS string for UTM coordinates
#From NGA.SIG.0012_2.0.0    
#12/22/2019 by B. Beal
#input parameters: zone number,latitude band (zone+letter), grid x and grid y coordinates    
def UTMtoMGRS(Z,latBand,x,y):
    #To match Geotrans, round to 3 places before MGRS evaluation:
    x = round(x,3)
    y = round(y,3)
    #Sec. 11.2
    #print ('x=',x)
    #print ('y=',y)
    def get_eLetter_1(x):   #if mod(Z,3) = 1
        i = math.floor(x/100000)
        switcher={
                1:'A',
                2:'B',
                3:'C',
                4:'D',
                5:'E',
                6:'F',
                7:'G',
                8:'H'
             }
        return switcher.get(i,"Invalid easting")
    def get_eLetter_2(x):   #if mod(Z,3) = 2
        i = math.floor(x/100000)
        switcher={
                1:'J',
                2:'K',
                3:'L',
                4:'M',
                5:'N',
                6:'P',
                7:'Q',
                8:'R'
             }
        return switcher.get(i,"Invalid easting")
    def get_eLetter_0(x):   #if mod(Z,3) = 0
        i = math.floor(x/100000)
        #print('i=',i)
        switcher={
                1:'S',
                2:'T',
                3:'U',
                4:'V',
                5:'W',
                6:'X',
                7:'Y',
                8:'Z'
             }
        return switcher.get(i,"Invalid easting")

    def get_nLetter_1(y):   #if mod(Z,2) = 1
        i = math.floor((y%2000000)/100000)
        switcher={
                0:'A',
                1:'B',
                2:'C',
                3:'D',
                4:'E',
                5:'F',
                6:'G',
                7:'H',
                8:'J',
                9:'K',
                10:'L',
                11:'M',
                12:'N',
                13:'P',
                14:'Q',
                15:'R',
                16:'S',
                17:'T',
                18:'U',
                19:'V'
             }
        return switcher.get(i,"Invalid northing")
    def get_nLetter_0(y):   #if mod(Z,2) = 0
        i = math.floor((y%2000000)/100000)
        switcher={
                0:'F',
                1:'G',
                2:'H',
                3:'J',
                4:'K',
                5:'L',
                6:'M',
                7:'N',
                8:'P',
                9:'Q',
                10:'R',
                11:'S',
                12:'T',
                13:'U',
                14:'V',
                15:'A',
                16:'B',
                17:'C',
                18:'D',
                19:'E'
             }
        return switcher.get(i,"Invalid northing")

    #Easting letter
    eGroup = abs(Z) % 3     #% is modulo
    if eGroup == 1:
        eLetter = get_eLetter_1(x)
    elif eGroup == 2:
        eLetter = get_eLetter_2(x)
    else:
        eLetter = get_eLetter_0(x) 
    
    #Northing letter
    nGroup = abs(Z) % 2
    if nGroup == 1:
        nLetter = get_nLetter_1(y)
    else:
        nLetter = get_nLetter_0(y)

    n = 5 #maximum digits allowed for MGRS
    eCoord = math.floor((x%10**5)/10**(5-n)) 
    nCoord = math.floor((y%10**5)/10**(5-n))
    e_str = str(eCoord).zfill(5)
    n_str = str(nCoord).zfill(5)
    MGRS_str = latBand+eLetter+nLetter+e_str+n_str
    #print ('e=',eCoord)
    #print ('n=',nCoord)
    #print ('MGRS=',MGRS_str)
    return MGRS_str


#Compute the complete MGRS string for UPS coordinates
#From NGA.SIG.0012_2.0.0    
#12/22/2019 by B. Beal    
def UPStoMGRS(Z,x,y):
    #To match Geotrans, round to 3 places before MGRS evaluation:
    x = round(x,3)
    y = round(y,3)
    #Sec. 11.9
    def get_eLetter_North(x):
        i = math.floor(x/100000)
        switcher={
                13:'YR',
                14:'YS',
                15:'YT',
                16:'YU',
                17:'YX',
                18:'YY',
                19:'YZ',
                20:'ZA',
                21:'ZB',
                22:'ZC',
                23:'ZF',
                24:'ZG',
                25:'ZH',
                26:'ZJ'
             }
        return switcher.get(i,"Invalid easting")
    
    def get_nLetter_North(y):
        i = math.floor(y/100000)
        switcher={
                13:'A',
                14:'B',
                15:'C',
                16:'D',
                17:'E',
                18:'F',
                19:'G',
                20:'H',
                21:'J',
                22:'K',
                23:'L',
                24:'M',
                25:'N',
                26:'P'             }
        return switcher.get(i,"Invalid northing")
    
    def get_eLetter_South(x):
        i = math.floor(x/100000)
        switcher={
                8:'AJ',
                9:'AK',
                10:'AL',
                11:'AP',
                12:'AQ',
                13:'AR',
                14:'AS',
                15:'AT',
                16:'AU',
                17:'AX',
                18:'AY',
                19:'AZ',
                20:'BA',
                21:'BB',
                22:'BC',
                23:'BF',
                24:'BG',
                25:'BH',
                26:'BJ',
                27:'BK',
                28:'BL',
                29:'BP',
                30:'BQ',
                31:'BR'
             }
        return switcher.get(i,"Invalid easting")

    def get_nLetter_South(y):
        i = math.floor(y/100000)
        switcher={
                8:'A',
                9:'B',
                10:'C',
                11:'D',
                12:'E',
                13:'F',
                14:'G',
                15:'H',
                16:'J',
                17:'K',
                18:'L',
                19:'M',
                20:'N',
                21:'P',
                22:'Q',
                23:'R',
                24:'S',
                25:'T',
                26:'U',
                27:'V',
                28:'W',
                29:'X',
                30:'Y',
                31:'Z'
             }
        return switcher.get(i,"Invalid northing")

    #ASSIGN letters
    if Z == 1:
        eLetter = get_eLetter_North(x)
        nLetter = get_nLetter_North(y)
    elif Z == -1:
        eLetter = get_eLetter_South(x)
        nLetter = get_nLetter_South(y)
    else:
        eLetter = "invalidE"
        nLetter = "invalidN"
    #12.3 Rounding v. truncating
    #The intended usage of UTM and UPS coordinates for the calculating or recording of positions complies with the usual
    #rounding rules of science and engineering. When a precise coordinate, e.g. x = 512 378 m, is to be converted to a less
    #precise coordinate, e.g. x = 512 380 m or x = 512 400 m, the operation is rounding, not dropping of digits (truncating).
    #For UTM (and UPS) conversions to MGRS, the operation is truncating, not rounding (see Section 11).
    n = 5 #maximum digits allowed for MGRS
    eCoord = math.floor((x%10**5)/10**(5-n)) 
    nCoord = math.floor((y%10**5)/10**(5-n))
    e_str = str(eCoord).zfill(5)
    n_str = str(nCoord).zfill(5)
    MGRS_str = eLetter+nLetter+e_str+n_str
    print ('e=',eCoord)
    print ('n=',nCoord)
    print ('MGRS=',MGRS_str)
    return MGRS_str
