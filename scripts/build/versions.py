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

# Builds LSAVersion.hpp using the current date/time and the Salsa
# version info found in windows_build/SALSA.aip.
# Updates data/default.cfg as well.
# This script is invoked when cmake is executed.

import datetime as dt
import re
import os
import sys
import subprocess

# Don't attempt to generate LSAVersion.hpp for public builds
if 'LSA_BUILD_TYPE' not in os.environ: 
    sys.exit()

if str(os.environ['LSA_BUILD_TYPE']).upper() != 'ARL':
    sys.exit()

# Get the directory of this script
ThisDir = os.path.dirname(os.path.realpath(__file__))

SalsaConfigFile = os.path.normpath(os.path.join(ThisDir,   '../../SalsaConfig.in'))
VERFile = os.path.normpath(os.path.join(ThisDir,   '../../src/lsacommon/LSAVersion.hpp'))
SUPVERFile = os.path.normpath(os.path.join(ThisDir,'../../src/lsacommon/LSASupportedToVersion.hpp'))
CFGFile = os.path.normpath(os.path.join(ThisDir,   '../../data/default.cfg'))
CopyrightFile = os.path.normpath(os.path.join(ThisDir, '..', '..', 'copyright.html'))
LicenseNoticeFile = os.path.normpath(os.path.join(ThisDir, '..', '..', 'license-notice.html'))
AddendumFile = ''
PublicReleaseFile = os.path.normpath(os.path.join(ThisDir, '..', '..', 'publicRelease.html'))

# Get the git sha
GitSha = subprocess.check_output(["git","rev-parse","--short","HEAD"], cwd=ThisDir).rstrip().decode()

def getSalsaVer(SalsaConfigFile):
    """Parse SalsaConfig.in and return product version as a string"""
    with open(SalsaConfigFile) as f:
        majorNum = "0"
        minorNum = "0"
        patchNum = "0"
        for line in f.readlines():
            major = re.search(r'CPACK_PACKAGE_VERSION_MAJOR[^"]*"(\d*)"', line)
            minor = re.search(r'CPACK_PACKAGE_VERSION_MINOR[^"]*"(\d*)"', line)
            patch = re.search(r'CPACK_PACKAGE_VERSION_PATCH[^"]*"(\d*)"', line)
            if major:
                majorNum = major.group(1)
            if minor:
                minorNum = minor.group(1)
            if patch:
                patchNum = patch.group(1)
        
        return "%s.%s.%s" % (majorNum, minorNum, patchNum)

def getSalsaReleaseType(SalsaConfigFile):
    """Parse SalsaConfig.in and return release type as a string"""
    with open(SalsaConfigFile) as f:
        releaseType = ""
        for line in f.readlines():
            type = re.search(r'CPACK_PACKAGE_RELEASE_TYPE[^"]*"(.*)"', line)
            if type:
                releaseType = type.group(1)
        
        return "%s" % (releaseType)
      
def getSALSABuildType():
    """Return the build type"""
    buildType = ''

    return "%s" % (buildType)

def getSalsaGitHash():
    revParse = subprocess.Popen(['git','rev-parse','HEAD'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    currentGitHash = revParse.stdout.read()
    currentGitStderr = revParse.stderr.read()
    
    if currentGitStderr == '':
        return currentGitHash
    else:
        print('stderr: %s\n' % currentGitStderr)
        return -1
    
def getSupportedToSalsaVer(SUPVERFile):
    """Parse LSASupportedToVersion.hpp and return supported to version as a string"""
    with open(SUPVERFile) as f:
        for line in f.readlines():
            if "LSASUPPORTEDTOVERSION_MAJOR" in line:
                MAJOR = line.split('"')[1]                
            if "LSASUPPORTEDTOVERSION_MINOR" in line:
                MINOR = line.split('"')[1]                
            if "LSASUPPORTEDTOVERSION_PATCH" in line:
                PATCH = line.split('"')[1]
                if (MAJOR != None) and (MINOR != None) and (PATCH != None):
                    return MAJOR + '.' + MINOR + '.' + PATCH
                else:# Getting here means we didn't find it
                    return "0.0.0"
                
# Read in appropriate about page file (license, disclaimer, addendum) directly as html text
def getAboutInfo(aboutFile):
    if not os.path.isfile(aboutFile):
        return ''
    with open(aboutFile) as f:
        aboutText = f.read()
        # Give double quotes an escape
        aboutText = aboutText.replace('"', '\\"')
        aboutText = aboutText.replace('\n', '\\n')
    return aboutText


Now = dt.datetime.now()
buildDate = Now.strftime("%b %d %Y")
buildTime = Now.strftime("%H:%M:%S")

verString = getSalsaVer(SalsaConfigFile)
Parts = verString.split('.')
SalsaMajor = 0
SalsaMinor = 0
SalsaPatch = 0
if len(Parts) > 2:
    SalsaPatch = Parts[2]
if len(Parts) > 1:
    SalsaMinor = Parts[1]
if len(Parts) > 0:
    SalsaMajor = Parts[0]
SalsaReleaseType = getSalsaReleaseType(SalsaConfigFile)
SalsaBuildType = getSALSABuildType()
AddendumText = getAboutInfo(AddendumFile)
CopyrightText = getAboutInfo(CopyrightFile)
LicenseNoticeText = getAboutInfo(LicenseNoticeFile)
PublicReleaseText = getAboutInfo(PublicReleaseFile)

# Update the .hpp file
with open(VERFile,'w') as o:
    o.write('#ifndef LSAVERSION_HPP\n')
    o.write('#define LSAVERSION_HPP\n')
    o.write('\n')
    o.write('#include <string>\n')
    o.write('#include <sstream>\n')
    o.write('\n')
    o.write('// THIS FILE IS AUTOGENERATED by lsa/scripts/build/versions.py when cmake builds the project.  Manual edits to this file will be lost.\n')
    o.write('\n')
    o.write('const std::string GIT_SHA =  "{0}";\n'.format(GitSha))
    o.write('const std::string LSAVERSION_MAJOR = "{0}";\n'.format(SalsaMajor))
    o.write('const std::string LSAVERSION_MINOR = "{0}";\n'.format(SalsaMinor))
    o.write('const std::string LSAVERSION_PATCH = "{0}";\n'.format(SalsaPatch))
    o.write('const std::string LSAVERSION_BUILD_DATE = "{0}";\n'.format(buildDate))
    o.write('const std::string LSAVERSION_BUILD_TIME = "{0}";\n'.format(buildTime))
    o.write('const std::string LSAVERSION_MAJOR_MINOR_PATCH = LSAVERSION_MAJOR + "." + LSAVERSION_MINOR + "." + LSAVERSION_PATCH;\n')
    o.write('const std::string LSAVERSION = LSAVERSION_MAJOR + "." + LSAVERSION_MINOR + "." + LSAVERSION_PATCH + " " + LSAVERSION_BUILD_DATE + " " + LSAVERSION_BUILD_TIME;\n')
    o.write('const std::string LSARELEASE_TYPE = "{0}";\n'.format(SalsaReleaseType))
    o.write('const std::string LSABUILD_TYPE = "{0}";\n'.format(SalsaBuildType))
    o.write('const std::string LSAADDENDUM = "{0}";\n'.format(AddendumText))
    o.write('const std::string LSACOPYRIGHT = "{0}";\n'.format(CopyrightText))
    o.write('const std::string LSALICENSENOTICE = "{0}";\n'.format(LicenseNoticeText))
    o.write('const std::string LSAPUBLICRELEASE = "{0}";\n'.format(PublicReleaseText))
    o.write('\n')
    o.write('#endif // LSAVERSION_HPP\n')
    
    print("-- Generated {} using Salsa version {}".format(VERFile, verString))    

#get the "supported to..." version string
supportedToVersion = getSupportedToSalsaVer(SUPVERFile)

# Update our default.cfg
with open(CFGFile) as c:
    Lines = c.readlines()
with open(CFGFile,'w') as c:
    for line in Lines:
        m = re.search(r'# Deployed with Salsa version [\d\.]+', line)
        if m:
            c.write("# Deployed with Salsa version {} -- Backward compatible to {}\n".format(verString,supportedToVersion))
        else:
            c.write(line)

# Delete some old files that can cause problems with code generation
staleFile1 =  os.path.normpath(os.path.join(ThisDir,'../converters/LSAVersion.py'))
staleFile2 =  os.path.normpath(os.path.join(ThisDir,'../test/LSAVersion.py'))
try:
    os.remove(staleFile1)
    os.remove(staleFile2)
except OSError:
    pass


# Create lsaversion.py files for other python scripts to import
# Set the file locations
versionConverterFile = os.path.normpath(os.path.join(ThisDir,'../converters/lsaversion.py'))

# Remove any old (stale) files
try:
    os.remove(versionConverterFile)
except OSError:
    pass

# Create a python .py file with needed the version info
with open(versionConverterFile,'w') as o:
    o.write('# THIS FILE IS AUTOGENERATED by lsa/scripts/build/versions.py when cmake builds the project.  Manual edits to this file will be lost.\n')
    o.write('GIT_SHA =  "{0}"\n'.format(GitSha))
    o.write('LSAVERSION_MAJOR = "{0}"\n'.format(SalsaMajor))
    o.write('LSAVERSION_MINOR = "{0}"\n'.format(SalsaMinor))
    o.write('LSAVERSION_PATCH = "{0}"\n'.format(SalsaPatch))
    o.write('LSAVERSION_BUILD_DATE = "{0}"\n'.format(buildDate))
    o.write('LSAVERSION_BUILD_TIME = "{0}"\n'.format(buildTime))
    o.write('LSAVERSION_MAJOR_MINOR_PATCH = "{0}.{1}.{2}"\n'.format(SalsaMajor, SalsaMinor, SalsaPatch))
    o.write('LSASUPPORTEDTOVERSION_MAJOR_MINOR_PATCH = "{0}"\n'.format(supportedToVersion))

# Copy lsaversion.py to scripts/test
from shutil import copyfile
versionTestFile = os.path.normpath(os.path.join(ThisDir,'../test/lsaversion.py'))
# Remove any old (stale) files
try:
    os.remove(versionTestFile)
except OSError:
    pass
copyfile(versionConverterFile, versionTestFile)

# Copy lsaversion.py to scripts/reporting
reportingFile = os.path.normpath(os.path.join(ThisDir,'../reporting/lsaversion.py'))
# Remove any old (stale) files
try:
    os.remove(reportingFile)
except OSError:
    pass
copyfile(versionConverterFile, reportingFile)

