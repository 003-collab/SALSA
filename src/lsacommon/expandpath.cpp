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
#pragma ident "$Id$"

/// expandpath.hpp Expand tilde (~) in filenames and append path to filenames.

#include <iostream>
#include <fstream>
#include "expandpath.hpp"
#include "StringUtils.hpp"
#include "lsaUtils.hpp"

#include <QtGlobal>
#include <QDir>
#include <QFileInfo>
#include <QString>

#ifdef _WIN32
    #include "windows.h"
    #include "tchar.h"
    #include <stdio.h>
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <sys/stat.h>
    #include <limits.h>
    #include <stdlib.h>
    #include <unistd.h>//exe path
    #include <linux/limits.h>
    #define GetCurrentDir getcwd
#endif


using namespace std;
using namespace gnsstk;

void expand_lsafilename(string& filename)
{
#ifndef _WIN32
   static char *chome = getenv("HOME");
   if(chome == NULL) return;
   static string home = string(chome);

   // assume tilde occurs only once
   string::size_type pos = filename.find_first_of("~");
   if(pos == string::npos) return;
   string newname;
   if(pos > 0) newname = filename.substr(0,pos);
   filename = filename.substr(pos+1);
   StringUtils::stripLeading(filename,"/");
   StringUtils::stripTrailing(home,"/");
   newname += home + string("/") + filename;
   filename = newname;
#endif
}

void expand_lsafilename(vector<string>& sarray)
{
   for(int i=0; i<sarray.size(); i++) expand_lsafilename(sarray[i]);
}

void include_lsapath(string path, string& file)
{
   if(!path.empty()) {
      StringUtils::stripTrailing(path,"/");
      StringUtils::stripTrailing(path,"\\");
      file = path + string("/") + file;
   }
}

string getPathWithoutFileName(string full_file_path)
{
   string::size_type pos;

   if(full_file_path.empty())
      return string("");//no file name
   else if((pos = full_file_path.find_last_of("/")) != string::npos)
      return full_file_path.substr(0,pos);//linux path
   else if((pos = full_file_path.find_last_of("\\")) != string::npos)
      return full_file_path.substr(0,pos);//Windows path
   else
      return string("");//no path
}

string get_lsapath_with_delimiter(string full_file_path)
{
   string::size_type pos;

   if(full_file_path.empty())
      return string("");//no file name
   else if((pos = full_file_path.find_last_of("/")) != string::npos)
      return full_file_path.substr(0,pos+1);//linux path
   else if((pos = full_file_path.find_last_of("\\")) != string::npos)
      return full_file_path.substr(0,pos+1);//Windows path
   else
      return string("");//no path
}

string get_file(string full_file_path)
{
   string::size_type pos;

   if(full_file_path.empty())
      return string("");//no file name
   else if((pos = full_file_path.find_last_of("/")) != string::npos)
      return full_file_path.substr(pos+1,full_file_path.size()-pos);//linux path
   else if((pos = full_file_path.find_last_of("\\")) != string::npos)
      return full_file_path.substr(pos+1,full_file_path.size()-pos);//Windows path
   else
      return full_file_path;//no path
}

string replaceEnvVars(const std::string &fileName)
{
    vector<string> splitComponents;
    if(fileName.find("/") != string::npos)
        splitComponents = gnsstk::StringUtils::splitWithDoubleQuotes(fileName, '/');
    else if(fileName.find("\\") != string::npos)
        splitComponents = gnsstk::StringUtils::splitWithDoubleQuotes(fileName, '\\');
    else
        splitComponents.push_back(fileName);

    bool foundEnvVar = false;
    if(splitComponents[0].find("%") != string::npos)
    {
        foundEnvVar = true;
        gnsstk::StringUtils::stripTrailing(splitComponents[0] ,'%');
        gnsstk::StringUtils::stripLeading(splitComponents[0] ,'%');
    }

    else if(splitComponents[0].find("$") != string::npos)
    {
        foundEnvVar = true;
        gnsstk::StringUtils::stripLeading(splitComponents[0] ,'$');
    }

    if(!foundEnvVar)
    {
        return fileName;
    }

    splitComponents[0] = QString::fromLocal8Bit(qgetenv(splitComponents[0].c_str())).toStdString();
    string rejoinedFile = "";
    for(vector<string>::const_iterator components_iter = splitComponents.begin();
        components_iter != splitComponents.end(); ++components_iter)
    {
        rejoinedFile += *components_iter;
        if(components_iter != std::prev(splitComponents.end()))
            rejoinedFile += "/";
    }

    return rejoinedFile;
}

void include_lsapath(string path, vector<string>& sarray)
{
   if(!path.empty()) {
      StringUtils::stripTrailing(path,"/");
      StringUtils::stripTrailing(path,"\\");
      for(int i=0; i<sarray.size(); i++)
         sarray[i] = path + string("/") + sarray[i];
   }
}

// return false if file cannot be opened
bool expand_list_lsafile(string& filename, vector<string>& values)
{
   string line,word;
   // DO NOT clear values, add to it

   // open list file
   ifstream infile;
   infile.open(filename.c_str());
   if(!infile.is_open()) return false;

   // read the list file
   while(1) {
      getline(infile,line);
      StringUtils::stripTrailing(line,'\r');
      StringUtils::stripLeading(line);
      while(!line.empty()) {
         word = StringUtils::stripFirstWord(line);
         if(word.substr(0,1) == "#") break;        // skip '#...' to end of line
         values.push_back(word);
      }
      if(infile.eof() || !infile.good()) break;
   }

   infile.close();

   return true;
}

bool pathIsRelative(std::string path)
{
    #ifdef _WIN32
    std::string testRemoteDir = QString::fromLocal8Bit(qgetenv("LSA_TEST_REMOTE")).toStdString();
    if(!testRemoteDir.empty())
    {
        if(path.find(testRemoteDir) != std::string::npos)
        {
            return false;
        }
    }
    // For windows, look for a : in the path
    return path.find(":") == std::string::npos;
    #else
    // For other os, look for a leading "/"
    return (!path.empty() && path.substr(0,1) != "/");
    #endif
}


std::string getCanonicalPath(std::string inputPath)
{
#ifdef _WIN32
	TCHAR result[MAX_PATH];
	TCHAR** lppPart=(NULL);

    GetFullPathName(inputPath.c_str(), MAX_PATH, result, lppPart);

    // replace all instances of "\\" with "/"
    std::string retVal(result);
    
    size_t position = retVal.find("\\");
    while (position != std::string::npos)
    {
        retVal.replace(position, 1, "/");
        position = retVal.find("\\");
    }

    return retVal;
#else
    QFileInfo fname(inputPath.c_str());
    QString retVal = fname.absoluteFilePath();
    return retVal.toStdString();
#endif
}

std::string formatIncludePath(std::string absProjectPath, std::string absParentPath, std::string absChildPath)
{
    // Get the project QDir
    QDir projectDir(QString::fromStdString(absProjectPath));

    // Get the path from the project dir to the child
    QFileInfo childFileInfo(QString::fromStdString(absChildPath));
    QString childPathFromProjectDir = projectDir.relativeFilePath(childFileInfo.absoluteFilePath());

    // If the childFile isn't a child of the project path, return the child's absolute path
    QStringList childPathParts = childPathFromProjectDir.split(QDir::separator());
    if (childPathParts[0].contains(".."))
    {
        return childFileInfo.absoluteFilePath().toStdString();
    }

    // Otherwise, return the path from the parentDir to the child
    QFileInfo parentFileInfo(QString::fromStdString(absParentPath));
    QDir parentDir = parentFileInfo.absoluteDir();
    QString relativeChildPath = parentDir.relativeFilePath(childFileInfo.absoluteFilePath());

    return relativeChildPath.toStdString();
}

// THIS METHOD IS DEPRECATED.  PLEASE USE getAbsoluteChildPath AND getLSAPathFromAbsolutePaths BELOW.
std::string getAbsolutePathFromIncludePath(std::string projectDirectory, std::string includePath)
{
    std::string absolutePath;
    std::string strippedIncludePath = gnsstk::StringUtils::strip(includePath,'"');

    // if filename param is only a file or relative, try to open it in the projectDirectory
    if ( getPathWithoutFileName(strippedIncludePath).empty() || pathIsRelative(strippedIncludePath))
    {
        absolutePath = projectDirectory + "/" + strippedIncludePath;
    }
    else // try to open the absolute path
    {
        absolutePath = strippedIncludePath;
    }

    return absolutePath;
}


std::string getAbsoluteChildPath(std::string childLSAPath, std::string parentAbsolutePath)
{
    QDir parentDir = QFileInfo(QString::fromStdString(parentAbsolutePath)).absoluteDir();
    return parentDir.relativeFilePath(QString::fromStdString(childLSAPath)).toStdString();
}

std::string getLSAPath(std::string projectDir, std::string childAbsolutePath, std::string parentAbsolutePath)
{
    // TODO: replace this with logic using QDir, QFileInfo and the path parameters
    return formatIncludePath(projectDir, parentAbsolutePath, childAbsolutePath);
}

//------------------------------------------------------------------------------------
string getWorkingDirectory()
{
#ifdef _WIN32
    char workingDir[MAX_PATH]="";
#else
    char workingDir[PATH_MAX]="";
#endif
    char *dummy = GetCurrentDir(workingDir,sizeof(workingDir));
    return string(workingDir);
}

//assumes reference = directory, test = file path
string getRelativePath(string reference, string test)
{
    string relPath=string("");
    vector<string> refParts = gnsstk::StringUtils::splitWithDoubleQuotes(reference,'/');
    vector<string> testParts = gnsstk::StringUtils::splitWithDoubleQuotes(test,'/');
    int ctr=0;

    while(true)
    {
        if(ctr==refParts.size() || ctr==testParts.size())
            break;
        if(refParts[ctr] != testParts[ctr])
        {
            ctr--;
            break;
        }
        else
            ctr++;
    }
    if(ctr==0)//no common path
    {
        relPath = test;
    }
    else if(ctr < refParts.size())//branch is earlier than reference
    {
        for(int i=0;i<refParts.size()-ctr-1;i++)
            relPath += string("../");
        for(int i=ctr+1;i<testParts.size();i++)
            relPath += testParts[i] + string("/");
        relPath = relPath.substr(0,relPath.size()-1);//remove trailing slash
    }
    else//test is in subdirectory of reference
    {
        if(testParts.size()-refParts.size() == 1)//test is in reference directory
            relPath = get_file(test);
        else
        {
            for(int i=refParts.size();i<testParts.size();i++)
                relPath += testParts[i] + string("/");
            relPath = relPath.substr(0,relPath.size()-1);//remove trailing slash
        }
    }

    return relPath;
}

void CreateLSADir(string path)
{
    string pathroot = string("");
    vector<string> parts;
#ifdef _WIN32
    if(GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES)
        return;//path already exists
//    parts = gnsstk::StringUtils::splitWithDoubleQuotes(path,'\\');
    parts = gnsstk::StringUtils::splitWithDoubleQuotes(path,'/');
    pathroot = parts[0];
    for(int i=0;i<parts.size();i++)
    {
        if(i>0)
//            pathroot += string("\\") + parts[i];
            pathroot += string("/") + parts[i];
        if(GetFileAttributes(pathroot.c_str()) == INVALID_FILE_ATTRIBUTES)
            CreateDirectory(pathroot.c_str(),NULL);
    }
#else
    struct stat statbuf;
    if((stat(path.c_str(), &statbuf) !=-1) && S_ISDIR(statbuf.st_mode))
        return;//path already exists
    parts = gnsstk::StringUtils::splitWithDoubleQuotes(path,'/');
    pathroot = string("/") + parts[0];
    for(int i=0;i<parts.size();i++)
    {
        if(i>0)
            pathroot += string("/") + parts[i];
        if(( (stat(pathroot.c_str(), &statbuf) !=-1) && !S_ISDIR(statbuf.st_mode) ) ||
           (stat(pathroot.c_str(), &statbuf) == -1))
            mkdir(pathroot.c_str(),0777);
    }
#endif

}
