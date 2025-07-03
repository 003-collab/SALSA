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
/// @file DirSet.hpp  Include file for class DirSet, a collection of DATDirSet and
/// matching DATDir records

#ifndef LSA_DIRECTION_SET_INCLUDE
#define LSA_DIRECTION_SET_INCLUDE

#include <string>
#include <vector>

/// Class DirSet encapsulates a set DATDir directions belonging to one DATDirSet.
/// It stores the index of the DATDirSet in GD.datfile.DirSets, as well as a vector
/// of indexes in GD.datfile.Dirs for all the valid DATDir records that belong to it.
class DirSet {
public:
   /// echo DATDirSet group name, just for convenience
   std::string group;

   /// instrument station name, just for convenience
   std::string At;

   /// Index in vector GD.datfile.DirSets of the DATDirSet of this direction set.
   unsigned int dsetindex;

   /// vector of indexes in GD.datafile.Dirs for DATDir's that belong to this set.
   std::vector<unsigned int> dirindex;

   /// vector of indexes in GD.Measurements for Dir's that belong to this set.
   std::vector<unsigned int> measindex;

   /// constructor
   DirSet(void) { }

}; // end class DirSet

#endif   // LSA_DIRECTION_SET_INCLUDE
