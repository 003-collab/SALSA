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

// expandpath.hpp Expand tilde (~) in filenames and append path to filenames.

#ifndef EXPAND_PATH_INCLUDE
#define EXPAND_PATH_INCLUDE

#include <string>
#include <vector>

void expand_lsafilename(std::string& filename);
void expand_lsafilename(std::vector<std::string>& sarray);
void include_lsapath(std::string path, std::string& file);
void include_lsapath(std::string path, std::vector<std::string>& sarray);
std::string getPathWithoutFileName(std::string full_file_path);
std::string get_lsapath_with_delimiter(std::string full_file_path);
std::string get_file(std::string full_file_path);
std::string replaceEnvVars(const std::string &fileName);

// return false if file cannot be opened
bool expand_list_lsafile(std::string& filename, std::vector<std::string>& values);
bool pathIsRelative(std::string path);

// formatting include paths
std::string getCanonicalPath(std::string inputPath);

//Paths should either end with filenames or final delimiters
std::string formatIncludePath(std::string absProjectPath, std::string absParentPath, std::string absChildPath);

std::string getWorkingDirectory();

// THIS METHOD IS DEPRECATED.  PLEASE USE getAbsoluteChildPath AND getLSAPathFromAbsolutePaths BELOW.
std::string getAbsolutePathFromIncludePath(std::string projectDirectory, std::string includePath);

std::string getAbsoluteChildPath(std::string childLSAPath, std::string parentAbsolutePath);
std::string getLSAPath(std::string projectDir, std::string childAbsolutePath, std::string parentAbsolutePath);

//assumes arguments are paths, sans files
std::string getRelativePath(std::string reference, std::string test);
void CreateLSADir(std::string path);
#endif // EXPAND_PATH_INCLUDE
