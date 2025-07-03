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
/// @file IOBfile.hpp  Include file for *.iob file: read and parse, write, and fill
///                    IOB* objects.


// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <string>
#include <vector>
#include <map>
#include "Exception.hpp"
#include "expandtilde.hpp"
#include "expandpath.hpp"//get_file

#include "IOBPLH.hpp"
#include "IOBXYZ.hpp"
#include "IOBDXYZ.hpp"
#include "IOBDIST.hpp"
#include "IOBComment.hpp"
#include "IOBInclude.hpp"
#include "IOBSIGM.hpp"
#include "IOBHGHT.hpp"
#include "IOBANGL.hpp"
#include "IOBVANG.hpp"
#include "IOBZANG.hpp"
#include "IOBAZIM.hpp"
#include "IOBHDIR.hpp"
#include "IOBHDIF.hpp"
#include "IOBVSCA.hpp"
#include "IOBrecord.hpp"

#ifndef LSA_IOB_FILE_PARSER_WRITER_INCLUDE
#define LSA_IOB_FILE_PARSER_WRITER_INCLUDE

/**
\verbatim

# IOB files are ASCII and whitespace-delimited with one record per line
\endverbatim
*/
/// Class IOBfile encapsulates reading, parsing, writing and data storage for all
/// records in *.dat files.
class IOBfile {
public:
   std::vector<std::string> target_HGHT_recs;
   std::vector<std::string> instrument_HGHT_recs;
   std::vector<std::string> all_HGHT_recs;
   std::map<std::string,IOBSIGM*> SIGMRecords;
   std::map<std::string,IOBVSCA*> VSCARecords;

   /// Write all the DocStrings from all the IOB record types, and return as a string
   /// @return the entire documentation for the IOB records
   std::string asString();

    /// Open, read and parse a .iob file, filling a vector of pointers to IOBrecord.
    /// param filename file name
    /// param recPtrs a vector of pointers of 'IOBrecord *' containing records
    ///           read.
    /// param badrecs return vector<string> of records that could not be parsed
    /// param cCounter reference to int of counter for COV/CORR records
    /// param dsetCounter reference to int of counter for DSET records
    /// param vscaCounter reference to int of counter for VSCA records
    /// param numCommentedVSCA reference to int of counter for commented VSCA records
    /// param projDir reference to string of directory above which .lsa files should not be created (if empty, ignore)
    /// param isCommentedFile boolean to indicate whether the #include for this file in the parent was commented out
    /// param projectIncludedFiles vector of string of absolute file paths to included files (including the parent)
    /// return number of records stored, or an error code
    /// throw if file cannot be opened
    int ReadAndParseIOBfile(const std::string& filename,
                                 const std::string& iobpath,
                                 const std::string& lsapath,
                                 std::vector<IOBrecord *>& recPtrs,
                                 std::vector<std::string>& badrecs,
                                 int &cCounter, int &dsetCounter,
                                 int &vscaCounter, int &numCommentedVSCA,
                                 bool &nonTrivialVSCA, std::string& projDir,
                                 bool isCommentedFile, std::vector<std::string>& projectIncludeFiles);


   /// Add a HI/HT line to the current vector of HI/HT lines, if a new station/value combination
   /// @param line line from the .iob file
   /// @return true if new combination, false if redundant
   bool updateTargetHGHTrecs(const std::string& line);
   bool updateInstrumentHGHTrecs(const std::string& line);

   /// Return the substring in the IOB file position corresponding to the record type indicator
   /// @param line line from the .iob file
   /// @return string corresponding to the record type indicator
   std::string getRecordType(std::string line);

   /// Return the substring in a COV/CORR record corresponding to the matrix form (UPPR/DIAG)
   /// @param line line from the .iob file
   /// @return string corresponding to the matrix form
   std::string getMatrixForm(std::string line);

   /// Return true if line only contains white space characters
   /// @param line line from the .iob file
   /// @return true if all whitespace, false if not
   bool isAllWhiteSpace(std::string line);

   /// Determines if the line is a potentially valid GeoLab record, commented out.
   /// If so, the line is uncommented true is returned.
   /// @param line line from the .iob file, will be uncommented if supported GeoLab record type
   /// @return true if line is a commented out GeoLab record
   bool checkForCommentedIOBRecord(std::string &line);

   /// Replaces labels of SIGM records that duplicate other SIGM record labels with unique labels
   /// and updated SIGMRecords with the current duplicate_label->unique label map
   /// @param sigmRecord the current SIGM record being converted
   /// @return true if label uniqified, false if record is complete duplicate and not to be written to .lsa file
   bool uniqifySIGMLabel(IOBSIGM *sigmRecord);

   ///Check if all measurement and include records have the same VSCA tag.
   /// If so, remove those tags from the records and return that tag.
   /// @param recPtrs - vector of IOBRecord pointers of all records in an include file
   /// @return tag of common VSCA records of children, empty string if none common.
   std::string VSCACleanUp(std::vector<IOBrecord *>& recPtrs);

   void removeVSCATag(IOBrecord *rec, std::string VSCATag);

private:

}; // end class IOBfile

#endif // LSA_IOB_FILE_PARSER_WRITER_INCLUDE
