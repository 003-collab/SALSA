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
#!/usr/bin/python
import argparse
import os
import re
import sys
import string
import math as m

fname = []
try: fname.append(sys.argv[1])
except IndexError:
  print('Not enough arguments given. Need two files to compare\n python Compare.py file1 file2\n')
try: fname.append(sys.argv[2])
except IndexError:
  print('Not enough arguments given. Need two files to compare\n python Compare.py file1 file2\n')

OH,G,X,Y,Z,siteN = ([] for i in range(6))
indOH,indG,indX,indY,indZ,indN = ([] for i in range(6))
diffO,diffG,diffX,diffY,diffZ,diffR = ([] for i in range(6))
maxdiffR,maxdiffR2,diffNR,diffNR2 = 0,0,'',''
maxdiff,maxdiff2,diffN,diffN2,cnt,cnt2 = ([] for i in range(6))
for ii in range(0,5):
  maxdiff.append(0)
  maxdiff2.append(0)
  cnt.append(-1)
  cnt2.append(-1)
  diffN.append('')
  diffN2.append('')

strOHG_SALSA = " Final Adjusted Positions Astronomic"
strSiteN_SALSA = " Final Adjusted Positions XYZ"
strOH_GeoLab = " PLO"
strG_GeoLab="                    0  0"
strG_GeoLab2="                 - "
strSiteN_GeoLab = " XYZ"

strOH,strG,strN,ftype = ([] for i in range(4))
for ii in range (0,len(fname)): ftype.append((os.path.splitext(fname[ii])[-1]).replace('.',''))
for ii in range (0,len(ftype)):
  if ftype[ii] == 'out':
    strOH.append(strOHG_SALSA)
    strG.append(strOHG_SALSA)
    strN.append(strSiteN_SALSA)
    indOH.append(-4)
    indG.append(-1)
    indX.append(1)
    indY.append(2)
    indZ.append(3)
    indN.append(0)
  if ftype[ii] == 'lst':
    strOH.append(strOH_GeoLab)
    strG.append(strG_GeoLab)
    strN.append(strSiteN_GeoLab)
    indOH.append(-3)
    indG.append(-1)
    indX.append(-5)
    indY.append(-4)
    indZ.append(-3)
    indN.append(1)
for ii in range (0,len(fname)):
  OH.append([])
  G.append([])
  siteN.append([])
  X.append([])
  Y.append([])
  Z.append([])
  try: fin = open(fname[ii],"r")
  except IOError:
    sys.stderr.write('Could not open input file\n')
    exit()
  if ftype[ii] == "lst":
    fline = fin.readlines()
    for line in fline:
      if re.match(strOH[ii],line):
        linesplt=filter(None,re.split(" ",line))
        OH[ii].append(linesplt[indOH[ii]].lstrip().rstrip())
      if re.match(strG[ii],line) or re.match(strG_GeoLab2,line):
        linesplt=filter(None,re.split(" ",line))
        G[ii].append(linesplt[indG[ii]].lstrip().rstrip())
      if re.match(strN[ii],line):
        linesplt=filter(None,re.split(" ",line))
        try:
          float(linesplt[indN[ii]+1].lstrip().rstrip())
          tmpN=linesplt[indN[ii]].lstrip().rstrip()
        except ValueError:
          tmpN = linesplt[indN[ii]].lstrip().rstrip()+linesplt[indN[ii]+1].lstrip().rstrip()
        siteN[ii].append(tmpN)
        X[ii].append(linesplt[indX[ii]].lstrip().rstrip())
        Y[ii].append(linesplt[indY[ii]].lstrip().rstrip())
        Z[ii].append(linesplt[indZ[ii]].lstrip().rstrip())
  if ftype[ii] == "out":
    in_Astro_data = 0 #will increment to skip header
    in_XYZ_data = 0 #will increment to skip header
    fline = fin.readlines()
    for line in fline:
      if re.match(strOH[ii],line): in_Astro_data=1
      if in_Astro_data>0: in_Astro_data+=1
      if re.match(strN[ii],line): in_XYZ_data = 1
      if in_XYZ_data>0: in_XYZ_data+=1
      if in_Astro_data>3:
        linesplt=filter(None,re.split(" ",line))
        if (linesplt[0]=='\n') or (linesplt[0]=='\r\n'): in_Astro_data = 0
        else:
          OH[ii].append(linesplt[indOH[ii]].lstrip().rstrip())
          G[ii].append(linesplt[indG[ii]].lstrip().rstrip())
      if in_XYZ_data>3:
        linesplt=filter(None,re.split(" ",line))
        if (linesplt[0]=='\n') or (linesplt[0]=='\r\n'): in_XYZ_data = 0
        else:
          try:
            float(linesplt[indN[ii]+1].lstrip().rstrip())
            tmpN=linesplt[indN[ii]].lstrip().rstrip()
          except ValueError:
            tmpN = linesplt[indN[ii]].lstrip().rstrip()+linesplt[indN[ii]+1].lstrip().rstrip()
          siteN[ii].append(tmpN)
          try:
            float(linesplt[indX[ii]].lstrip().rstrip())
            X[ii].append(linesplt[indX[ii]].lstrip().rstrip())
            Y[ii].append(linesplt[indY[ii]].lstrip().rstrip())
            Z[ii].append(linesplt[indZ[ii]].lstrip().rstrip())
          except ValueError:
            X[ii].append(linesplt[indX[ii]+1].lstrip().rstrip())
            Y[ii].append(linesplt[indY[ii]+1].lstrip().rstrip())
            Z[ii].append(linesplt[indZ[ii]+1].lstrip().rstrip())
  fin.close()

smaller = min(len(X[0]),len(X[1]))
for ii in range(0,smaller):
  for jj in range(0,smaller):
    if siteN[0][ii]==siteN[1][jj]:
      diffX.append(abs(float(X[0][ii])-float(X[1][jj])))
      diffY.append(abs(float(Y[0][ii])-float(Y[1][jj])))
      diffZ.append(abs(float(Z[0][ii])-float(Z[1][jj])))
      diffR.append(m.sqrt(diffX[-1]**2.+diffY[-1]**2.+diffZ[-1]**2))
      if len(G[0])>1 and len(G[1])>1:
        diffG.append(abs(float(G[0][ii])-float(G[1][jj])))
        diffO.append(abs(float(OH[0][ii])-float(OH[1][jj])))
      break

for ii in range(0,smaller):
  if len(G[0])>1 and len(G[1])>1:
    if diffG[ii]>maxdiff[1]:
      maxdiff[1]=diffG[ii]
      diffN[1]=siteN[0][ii]
    if (diffG[ii]<maxdiff[1]) and (diffG[ii]>maxdiff2[1]):
      maxdiff2[1]=diffG[ii]
      diffN2[1]=siteN[0][ii]
    if diffO[ii]>maxdiff[0]:
      maxdiff[0]=diffO[ii]
      diffN[0]=siteN[0][ii]
    if (diffO[ii]<maxdiff[0]) and (diffO[ii]>maxdiff2[0]):
      maxdiff2[0]=diffO[ii]
      diffN2[0]=siteN[0][ii]
  if diffR[ii]>maxdiffR:
    maxdiffR=diffR[ii]
    diffNR=siteN[0][ii]
  if (diffR[ii]<maxdiffR) and (diffR[ii]>maxdiffR2):
    maxdiffR2=diffR[ii]
    diffNR2=siteN[0][ii]
  if diffX[ii]>maxdiff[2]:
    maxdiff[2]=diffX[ii]
    diffN[2]=siteN[0][ii]
  if (diffX[ii]<maxdiff[2]) and (diffX[ii]>maxdiff2[2]):
    maxdiff2[2]=diffX[ii]
    diffN2[2]=siteN[0][ii]
  if diffY[ii]>maxdiff[3]:
    maxdiff[3]=diffY[ii]
    diffN[3]=siteN[0][ii]
  if (diffY[ii]<maxdiff[3]) and (diffY[ii]>maxdiff2[3]):
    maxdiff2[3]=diffY[ii]
    diffN2[3]=siteN[0][ii]
  if diffZ[ii]>maxdiff[4]:
    maxdiff[4]=diffZ[ii]
    diffN[4]=siteN[0][ii]
  if (diffZ[ii]<maxdiff[4]) and (diffZ[ii]>maxdiff2[4]):
    maxdiff2[4]=diffZ[ii]
    diffN2[4]=siteN[0][ii]

for ii in range(0,smaller):
  if diffX[ii]==maxdiff[2]: cnt[2] += 1
  if diffY[ii]==maxdiff[3]: cnt[3] += 1
  if diffZ[ii]==maxdiff[4]: cnt[4] += 1
  if len(G[0])>1 and len(G[1])>1:
    if diffO[ii]==maxdiff[0]: cnt[0] += 1
    if diffG[ii]==maxdiff[1]: cnt[1] += 1
    if diffG[ii]==maxdiff2[1]: cnt2[1] += 1
    if diffO[ii]==maxdiff2[0]: cnt2[0] += 1
  if diffX[ii]==maxdiff2[2]: cnt2[2] += 1
  if diffY[ii]==maxdiff2[3]: cnt2[3] += 1
  if diffZ[ii]==maxdiff2[4]: cnt2[4] += 1
  
if len(G[0])>1 and len(G[1])>1:
  outputG1 = str(round(maxdiff[1],6))+" ("+str(diffN[1])+"+"+str(cnt[1])+")"
  outputG2 = str(round(maxdiff2[1],6))+" ("+str(diffN2[1])+"+"+str(cnt2[1])+")"
  outputOH1 = str(round(maxdiff[0],6))+" ("+str(diffN[0])+"+"+str(cnt[0])+")"
  outputOH2 = str(round(maxdiff2[0],6))+" ("+str(diffN2[0])+"+"+str(cnt2[0])+")"
else:
  outputG1 = "N/A"
  outputG2 = "N/A"
  outputOH1 = "N/A"
  outputOH2 = "N/A"
print "(Largest Error) OHdiff: "+outputOH1+",Gdiff: "+outputG1+", Xdiff: "+str(round(maxdiff[2],6))+" ("+str(diffN[2])+"+"+str(cnt[2])+"), Ydiff: "+str(round(maxdiff[3],6))+" ("+str(diffN[3])+"+"+str(cnt[3])+"), Zdiff: "+str(round(maxdiff[4],6))+" ("+str(diffN[4])+"+"+str(cnt[4])+"), Rdiff: "+str(round(maxdiffR,6))+" ("+str(diffNR)+")"
print "(2nd Largest) OHdiff: "+outputOH2+",Gdiff: "+outputG2+", Xdiff: "+str(round(maxdiff2[2],6))+" ("+str(diffN2[2])+"+"+str(cnt2[2])+"), Ydiff: "+str(round(maxdiff2[3],6))+" ("+str(diffN2[3])+"+"+str(cnt2[3])+"), Zdiff: "+str(round(maxdiff2[4],6))+" ("+str(diffN2[4])+"+"+str(cnt2[4])+"), Rdiff: "+str(round(maxdiffR2,6))+" ("+str(diffNR2)+")"

