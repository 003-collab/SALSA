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
/// @file lsasolver.cpp  ARL:UT least squares solver for LSA. Read *.dat file for
/// a priori information, unknowns and data, compute a priori values for unknowns
/// where necessary, and solve the LS problem.

//------------------------------------------------------------------------------------
#include "lsasolver.hpp"
#include "lsaobseqn.hpp"
#include "lsaextr.hpp"
#include "SunEarthSatGeometry.hpp"
#include "SpecialFuncs.hpp"
#include "ENUUtil.hpp"
#include "expandpath.hpp"

#include "lsaeigen.hpp"
#include <iostream>
#include <omp.h>
#include "qdir.h"

//------------------------------------------------------------------------------------
using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
#include "LSAVersion.hpp"
const string GlobalData::Version(string("0.6.0 1/11/16")
                            + string(", SALSA Ver. ") + LSAVERSION);

//------------------------------------------------------------------------------------
// prototypes
//                                                              called by:
int ProcessCommandLine(int argc, char **argv) throw(Exception);// main
int ReadInputFile(void) throw(Exception);                      // main

int MoveInputToMeasurements(void) throw(Exception);            // main
int ComputeDirSetObjects(void) throw(Exception);               // MoveInputToM

int SummarizeInput(void) throw(Exception);                     // main
// ComputeAPrioiri  in lsapriori.hpp                           // main
int DefineLSProblem(void) throw(Exception);                    // main
int DefineConstraints(int &nCons) throw(Exception);             // DefineLSProblem

int SolveNonLinearLSProblem(void) throw(Exception);            // main
// called by SolveNonLinearLSProblem
int DefineLSEquation(void) throw(Exception);
int DefineConstraintEquations(void) throw(Exception);
int OutputLSEquation(const string& basename, const int& n) throw(Exception);
int SolveLLSProblem(const string& msg) throw(Exception);
void StatisticalTests(bool doAll=false) throw(Exception);
int ComputeResiduals(double& relRMSresid, double& lenRMSresid, double& angRMSresid)
   throw(Exception);
int UpdatePoints(void) throw(Exception);
int CalculateReliability(void) throw(Exception);
int CalculateInternalReliability(void) throw(Exception);
int CalculateExternalReliability(void) throw(Exception);
int CalculateRectangles(void) throw(Exception);

int FinalOutput(void) throw(Exception);                        // main
int OutputLLSSolution(const string& msg) throw(Exception);     // SolveNonL + FinalOut
int OutputLLSDataResid(const string& msg, bool doAll=false) throw(Exception);
                                                               // SolveNonL + FinalOut
//int OutputDirSets(const string& msg) throw(Exception);         // OutputLLSDataRes
int OutputLLSRMSresid(const string& msg, bool doChiSq=false)   // SolveNonL + FinalOut
   throw(Exception);

int ComputeExtraction(void) throw(Exception);                  // main
int DumpDAT(void) throw(Exception);                            // main

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
/// LSA Solver main program
int main(int argc, char **argv)
{
   string PrgmName;        // for catch
try {
   clock_t totaltime=clock();

#ifdef _WIN32
   #if (_MSC_VER < 1900)//Visual Studio 2015
      _set_output_format(_TWO_DIGIT_EXPONENT);
   #endif
#endif

   // get (create) the global data object (a singleton);
   // since this is the first instance, this will also set default values
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();
   PrgmName = GD.PrgmName;

   // Build title
   Epoch ttag;
   ttag.setLocalTime();
   GD.RunStr = GD.PrgmName + ", Ver. "
      + GD.Version + ttag.printf(", Run %04Y/%02m/%02d at %02H:%02M:%02S");

   // display title on screen
   LOG(INFO) << GD.RunStr;

   // TEMP, for debugging CommandLine;
   //LOGlevel = ConfigureLOG::Level("DEBUG");

   // process : loop once -----------------------------------------------------
   unsigned int i,j,k,nunkn;
   string label;
   int iret;

   for(bool go=true; go; go=false) {

      // --------------------------------------------------------------
      // process the command line
      iret = ProcessCommandLine(argc,argv);        // lsacmdline.cpp
      LOG(VERBOSE) << GD.cmdlineDump;
      if(iret) break;

      // --------------------------------------------------------------
      // read input file
      iret = ReadInputFile();
      if(iret) break;

      // --------------------------------------------------------------
      // copy data from DATfile into Measurements array,
      iret = MoveInputToMeasurements();
      if(iret) break;

      // --------------------------------------------------------------
      // summarize input and search for undefined Points
      iret = SummarizeInput();
      if(iret) break;

      // --------------------------------------------------------------
      // find apriori for Points (number nunkn) with no information
      // TD this needs to also compute the apriori DirSet biases
      iret = ComputeAPriori(nunkn, GD.allowAPcentroid, EQD.is2D);  // lsapriori.cpp

      // --------------------------------------------------------------
      // compute apriori estimate of DIRSET biases
      if(iret == lsa::LSASOLVER_OK) {
         if(GD.DirSets.size() > 0)
            LOG(INFO) << "\n# Compute a priori bias for each DirSet";
         for(i=0; i<GD.DirSets.size(); i++) {
            // get the instrument position
            map<string,Point>::const_iterator it = GD.Points.find(GD.DirSets[i].At);
            if(it == GD.Points.end()) continue;       // TD Warning?
            Point At(it->second);

            // get rotation matrix here
            Matrix<double> Rot(At.getRotation());

            // loop over DIRs, computing a bias estimate for each
            double b,sd;
            vector<double> bias;
            vector<string> sites;
            Stats<double> stb;

            for(j=0; j<GD.DirSets[i].measindex.size(); j++) {
               unsigned int k = GD.DirSets[i].measindex[j];
               const Dir& d(dynamic_cast<Dir&>(*(GD.Measurements[k])));
               map<string,Point>::const_iterator jt;
               jt = GD.Points.find(d.To);
               if(jt == GD.Points.end()) continue;       // TD Warning?
               Point To(jt->second);

               // compute To minus At
               Vector<double> ABxyz(3),ABneu;
               ABxyz[0] = To.x - At.x;
               ABxyz[1] = To.y - At.y;
               ABxyz[2] = To.z - At.z;
               ABneu = Rot * ABxyz;
               // compute d.value - azimuth(same Points)
               b = d.value - azimuth(ABneu[1],ABneu[0]);
               if(bias.size() > 0) {
                  while(b-bias[0] > Pi) b -= TwoPi;
                  while(bias[0]-b > Pi) b += TwoPi;
               }
               sites.push_back(To.label);
               bias.push_back(b);
               stb.Add(b);
               //LOG(INFO) << " Estimate bias for sites " << At.label
               //   << " " << To.label << " = " << fixed << setprecision(4) << b;
            }
            if(bias.size() > 0) {
               //LOG(INFO) << "Stats for bias estimate "
               //   << GD.DirSets[i].group << " : " << stb;
               b = stb.Average();
               sd = stb.StdDev();

               // warn if stddev is large
               if(sd > 0.1745) {            // 10 degrees
                  LOG(WARNING) << " Warning - large uncertainty in estimation of "
                     << "a priori bias for DirSet " << GD.DirSets[i].group;
                  for(k=0; k<bias.size(); k++) {
                     LOG(INFO) << "  " << GD.DirSets[i].group << " bias for dir "
                        << At.label << "-" << sites[k] << " is "
                        << fixed << setprecision(4) << bias[k]*::RAD_TO_DEG << " deg";
                  }
               }

               while(b < -Pi) b += TwoPi;
               while(b > Pi) b -= TwoPi;
               EQD.DSbiases["bias"+GD.DirSets[i].group] = b;
               LOG(INFO) << " Estimated a priori bias for DirSet "
                  << GD.DirSets[i].group << " = "
                  << fixed << setprecision(3) << setw(6) << b
                  << scientific << setprecision(2)
                  << " +- " << sd << " radians = "
                  << fixed << setprecision(3) << setw(8)
                  << b*::RAD_TO_DEG << " +- " << sd*::RAD_TO_DEG << " degrees.";
            }
         }
      }


      // --------------------------------------------------------------
      // find the size of the largest Point label, for printing
      GD.plwidth = 0;
      map<string,Point>::iterator it;
      for(it=GD.Points.begin(); it!=GD.Points.end(); ++it) {
         if(it->second.label.size() > GD.plwidth)
            GD.plwidth = it->second.label.size();
      }

      // print all the a priori positions
      try {
         // print a priori Points
         LOGstrm << "\n# Complete a priori positions";
         if(nunkn > 0) LOGstrm << " (There were " << nunkn << " unknown sites)";
         LOGstrm << endl;
         for(it=GD.Points.begin(); it!=GD.Points.end(); ++it) {
            LOG(INFO) << " Point"
               << leftJustify(" ",GD.plwidth - it->second.label.size())
               << " " << it->second.asString(GD.linprecP,GD.linwidth)
               << " " << (vectorindex(GD.computeAPlabels, it->first) == -1
                             ? "(given)":"(computed)");

            // add to binary file
            if(!GD.binfile.empty()) {
               apriori_pos ap;
               ap.X = it->second.x;
               ap.Y = it->second.y;
               ap.Z = it->second.z;
               ap.fixtype = it->second.fixtype;
               if(vectorindex(GD.computeAPlabels, it->first)==-1)
               {
                   ap.computed = apriori_pos::ComputedState::Provided;
               }

               else
               {
                   if(it->second.constraint == "")
                   {
                       ap.computed = apriori_pos::ComputedState::ComputedFloating;
                   }

                   else if(it->second.constraint == "NE")
                   {
                       ap.computed = apriori_pos::ComputedState::ComputedLatLonFixed;
                   }

                   else
                   {
                       throw Exception("Unknown constraint type given to a priori point.");
                   }
               }
               // If apriori_pos gets ammended to add constraint, ammend code to grab
               // constraint value here
               GD.LSABin.apPoints.insert(
                     map<std::string,apriori_pos>::value_type(it->first,ap));
            }
         }
      } catch(Exception& e) { GNSSTK_RETHROW(e); }

      // if user chose to quit after ComputeAPriori(), extract and break
      if(iret == lsa::EARLY_EXIT) {               // quit after apriori
         ComputeExtraction();       // TD ignoring return
         break;
      }
      if(iret) break;               // ComputeAPriori failed (11)

      // --------------------------------------------------------------
      // load geoid file
      // note: failure to load the geoid results in a nonfatal warning
      try {
         GD.applyGeoid = false;
         if(!GD.geoidfile.empty()) {
            LOG(INFO) << "\n# Loading geoid file " << GD.geoidfile << setprecision(6);

            // find the bounding box
            double latmin,latmax,lonmin,lonmax;
            FindBoundingBox(GD.Points, latmin,latmax,lonmin,lonmax);
            LOG(INFO) << " Latitude range:  " << latmin << ", " << latmax << " degN";
            LOG(INFO) << " Longitude range: " << lonmin << ", " << lonmax << " degE";

            // build the Geoid object
            GD.geoid = Geoid(GD.geoidfile);                    // create a Geoid

            // set local geoid .h5 output file
            if(GD.geoidfile.find(".h5") == std::string::npos)
            {
               QString geoidProjPath = QString::fromStdString(getPathWithoutFileName(GD.logfile)+"/geoid/");
               QDir dir(geoidProjPath);
               if (!dir.exists(geoidProjPath))
               {
                  dir.mkpath(geoidProjPath);
               }

               // split off file prefix for .h5 patch file
               // assumptions:
               //  - geoidfile0 has been set in lsacmdline::ValidateInput due to if(!GD.geoidfile.empty()) above
               //  - geoidfile0 is of the form of one of the default geoid files: (egm1996_<>.und, egm2008_<>.und, g2018u0.asc)
               QString geoidfilePrefix = QString::fromStdString(GD.geoidfile0).split(".")[0];
               QString geoidH5File = geoidProjPath + geoidfilePrefix + "_patch.h5";
               GD.geoid.setGeoidFileH5(geoidH5File.toStdString());
               if (!QFileInfo::exists(geoidH5File) || (QFileInfo::exists(geoidH5File)  && GD.overwriteGeoidPatch)) 
               {
                  GD.geoid.setWriteH5Patch();
               }
            }

            GD.geoid.setInterpolation(GD.geoidinterp);         // set interp method
            LOG(INFO) << " Using " << GD.geoidinterp << " geoid interpolation.";
            GD.geoid.loadGeoid(latmin,latmax,lonmin,lonmax);   // load the box
            GD.applyGeoid = true;                              // success

            // get initial values
            GD.geoid.UpdateGeoidValues(GD.Points);
         }
         else {
            LOG(WARNING) << " Warning - geoid file is not given; "
                        << "local gravity corrections will not be applied.";
         }
      } catch(Exception& e) {
         LOG(ERROR) << " Error - " << e.getText();
         iret = lsa::UNOPENED_GEOID_FILE;
         break; // GNSSTK_RETHROW(e);
      }

      // --------------------------------------------------------------
      // count state vector elements, building namelists EQD.StateNames, EQD.DataNames
      // and nominal state GD.NomState0
      iret = DefineLSProblem();

      try {
         ostringstream oss;
         oss << " There are " << EQD.NMeas << " data, "
            << EQD.StateNames.size() << " state elements, and "
            << EQD.Ncon << " constraints,  leaving "
            << SD.ndof << " degrees of freedom";
         if(GD.DirSets.size() > 0)
            oss << " and the state includes " << GD.DirSets.size()
                  << " DirSet parameter" << (GD.DirSets.size() > 1 ? "s":"");
         oss << ".";
         LOG(INFO) << "\n# Build LS problem:\n" << oss.str();
         if(GD.doProgress) cout << oss.str() << endl;

         // print the unused data message
         if(GD.unusedMsg.size() > 0) LOG(INFO) << "\n" << GD.unusedMsg;

         // check for invalid and underdetermined problems
         if(EQD.NMeas == 0 && !GD.apquit) {//Fix to Bug #1388
            LOG(ERROR) << "\n Error - invalid problem - no data found.";
            iret = lsa::INVALID_NO_DATA;
            break;
         }
         if(EQD.StateNames.size() == 0) {
            LOG(ERROR) << "\n Error - invalid problem - there are no state elements.";
            iret = lsa::INVALID_NO_STATE;
            break;
         }
         if(SD.ndof < 0) {
            LOG(ERROR) << "\n Error - the problem is underdetermined - abort.";
            iret = lsa::PROBLEM_UNDERDETERMINED;
            break;
         }

         // write to binary file
         if(!GD.binfile.empty()) {
            GD.LSABin.ndof = SD.ndof;
            GD.LSABin.nunknown = EQD.StateNames.size()-EQD.Ncon;//3xUnfixedPtsNum-Constrs
            GD.LSABin.ndata = EQD.DataNames.size();
            GD.LSABin.nmeas = EQD.NMeas;
            GD.LSABin.nconstr = EQD.Ncon;
         }

         // pretty print. NB only works if state is Px Py Pz Ax Ay Az Rx Ry Rz etc
         // length of largest of [Point names, "Point"]
         int labw = (GD.stwidth > 7 ? GD.stwidth : 7);
         string tmplabel;
         LOG(INFO) << "\n A priori state\n "
                   << leftJustify("Point",labw)
                   << center(XYZ[0],GD.linwidth+1)
                   << center(XYZ[1],GD.linwidth+1)
                   << center(XYZ[2],GD.linwidth+1);
         // list position states, 2:3 per line
         for(i=0; i<EQD.StateNames.size(); i+=(EQD.is2D ? 2:3)) {
            label = EQD.StateNames.getName(i);
            if(isLike(label,"bias")) continue;
            j = label.size();
            tmplabel = label = label.substr(0,j-1);
            LOG(INFO) << " " << leftJustify(tmplabel,labw)
               << fixed << setprecision(GD.linprecP)
               << " " << setw(GD.linwidth) << GD.Points[label].x
               << " " << setw(GD.linwidth) << GD.Points[label].y
               << " " << setw(GD.linwidth) << GD.Points[label].z;
         }
         // list bias states, one per line
         for(i=0; i<EQD.StateNames.size(); i++) {
            tmplabel = label = EQD.StateNames.getName(i);
            if(!isLike(label,"bias")) continue;
            LOG(INFO) << " " << leftJustify(tmplabel,labw) << " "
               << fixed << setprecision(GD.angprecM) << setw(6)
               << EQD.DSbiases[label] << " rad = " << setw(7)
               << EQD.DSbiases[label] * ::RAD_TO_DEG << " degrees.";
         }

      } catch(Exception& e) { GNSSTK_RETHROW(e); }

      if(iret) break;

      // --------------------------------------------------------------
      // solve the full non-linear least squares problem
      iret = SolveNonLinearLSProblem();
      GD.timing("SolveNLLS");
      if(iret) break;

      // --------------------------------------------------------------
      // output the final solution
      iret = FinalOutput();
      GD.timing("Final_Output");
      if(iret) break;

      // --------------------------------------------------------------
      // compute extractions and print
      iret = ComputeExtraction();
      if(iret) break;

      iret = DumpDAT();
      if(iret) break;

   }  // end loop once

   // write and close BIN file
   if((iret==lsa::LSASOLVER_OK || iret==lsa::EARLY_EXIT) && !GD.binfile.empty()) {
      int ibret = GD.LSABin.WriteBinaryFile(GD.binfile);
      if(ibret) {
         LOG(ERROR) << " Error - could not open the output binary file.";
         if(iret == lsa::LSASOLVER_OK) iret = ibret;
      }
   }

   // error condition ---------------------------------------------------------
   // return codes: 0 ok 1 help 2 cmd line errors etc
   //LOG(INFO) << "Return code is " << iret;
   if(iret != lsa::LSASOLVER_OK) {
      string msg;
      if(iret != lsa::COMMAND_LINE_USAGE) {
         msg = GD.PrgmName + " is terminating with code " + asString(iret) + ": ";
         //LOG(ERROR) << msg;
      }

      if(iret == lsa::COMMAND_LINE_USAGE) { msg = GD.cmdlineUsage; }
      else if(iret == lsa::COMMAND_LINE_ERRORS) { msg = GD.cmdlineErrors; }
      else if(iret == lsa::USER_VALIDATION) { msg += "The user requested input validation."; }
      else if(iret == lsa::INVALID_INPUT) { msg += "The input is invalid."; }
      else if(iret == lsa::UNOPENED_LOG_FILE) { msg += "The output file " + GD.logfile
                                       + " could not be opened."; }
      else if(iret == lsa::UNOPENED_INPUT_FILE) { msg += "The input file " + GD.infile
                                       + " could not be opened."; }
      else if(iret == lsa::EARLY_EXIT) { msg += "The user requested early exit."; }
      else if(iret == lsa::PROBLEM_UNDERDETERMINED) { msg += "The problem is underdetermined."; }
      else if(iret == lsa::SINGULAR_PROBLEM) { msg += "No solution, the problem is singular."; }
      else if(iret == lsa::PROBLEM_APRIORI) { msg += "Unable to compute all a priori positions.";}
      else if(iret == lsa::INVALID_NO_DATA) { msg += "The problem is invalid - no data."; }
      else if(iret == lsa::INVALID_NO_STATE) { msg += "The problem is invalid - no state."; }
      else if(iret == lsa::PROBLEM_INCONSISTENT_CONSTRAINTS) { msg += "The constraints are inconsistent."; }
      else if(iret == lsa::SINGULAR_COVARIANCE) { msg += "There is a singular measurement covariance."; }
      else if(iret == lsa::SINGULAR_LS_SOLUTION) { msg += "The LS solution is singular."; }
      else if(iret == lsa::INVALID_DIR_SETS) { msg += "DIRSET(s) are invalid."; }
      else if(iret == lsa::UNOPENED_GEOID_FILE) { msg += "Unable to open geoid file " + GD.geoidfile; }
      else if(iret == lsa::PROBLEM_EIGEN) { msg += "Problem encountered using Eigen3 methods."; }
      else if(iret == lsa::PROBLEM_MEM_ALLOC) { msg += "Could not allocate enough memory."; }
      else if(iret == lsa::PROBLEM_NAN_DETECTED) { msg += "NaNs detected in solution. Divergence has likely occurred."; }
      else if(iret == lsa::UNOPENED_COMP_FILE) { msg += "Could not open the comp file."; }
      else if(iret == lsa::UNOPENED_DAT_FILE) { msg += "Could not open the output dat file."; }
      else if(iret == lsa::UNOPENED_BIN_FILE) { msg += "Could not open the output binary file."; }
      else if(iret == lsa::INVALID_BIN_FILE_VER) { msg += "Binary file version is obsolete."; }
      else if(iret == lsa::INVALID_CMD_LINE_DEF) { // cmd line definition invalid
         msg += "The command line definition is invalid.\n" + GD.cmdlineErrors;
      }
      else if(iret == lsa::EIGEN_CHOLESKY_FAIL) { msg += "Eigen choleksy decomposition failed to yield constructable covariance matrix."; }
      else                 // fix this
         msg += "temp - Some other return code..." + asString(iret);

      LOG(ERROR) << msg;
      if(GD.doProgress) cout << msg << endl;
   }

   // compute and print run time ----------------------------------------------
   if(iret != lsa::COMMAND_LINE_USAGE) {
      GD.wallend.setLocalTime();
      ostringstream oss;
      oss << endl << PrgmName << " timing: " << fixed << setprecision(3)
         << double(clock()-totaltime)/double(CLOCKS_PER_SEC)
         << " seconds. (" << (GD.wallend - GD.wallbegin) << " sec)";
      if(pLOGstrm != &cout) LOG(INFO) << oss.str();
      cout << oss.str() << endl;
   }

   if(iret == lsa::LSASOLVER_OK) return lsa::LSASOLVER_OK; else return lsa::LSASOLVER_ERROR;
}
catch(Exception& e) {

  std::string emsg = e.what();
  if(emsg.find("Sigma is zero")){
      LOG(ERROR) << "Error - the solver encountered measurement with no uncertainty." << endl;
  }
  else{
      cerr << PrgmName << " caught Exception:\n" << e.what() << endl;
      // don't use LOG here - causes hangup - don't know why
  }
}
catch (...) {
   cerr << "Unknown error in " << PrgmName << ".  Abort." << endl;
}
   return lsa::LSASOLVER_ERROR;
}   // end main()

//------------------------------------------------------------------------------------
/// Read the input DAT file, and create the DATfile object.
int ReadInputFile(void) throw(Exception)
{
try {
   int iret;
   GlobalData& GD=GlobalData::Instance();

   // read the dat file
   vector<string> badrecs;
   iret = GD.datfile.Read(GD.infile, badrecs);
   if(iret < 0) {
      LOG(ERROR) << " Error - the input DAT file could not be opened.";
      return lsa::UNOPENED_INPUT_FILE;              // could not open file
   }

   // were there bad records (errors)
   for(unsigned int j=0; j<badrecs.size(); j++) {
      LOG(ERROR) << " Error - DAT reader failed to parse line >" << badrecs[j] << "<";
   }

   // TEMP
   //LOG(INFO) << datfile.asString();
   //LOG(INFO) << datfile.exampleOutput();

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Use the DATfile object to define the GD configuration and fill the GD arrays
/// including Points, Measurements and Extractions. Clear the DATfile object.
/// Called by main().
int MoveInputToMeasurements(void) throw(Exception)
{
try {
   unsigned int i,j;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

      // configuration
   if(!GD.datfile.Config.title.empty()) GD.Title = GD.datfile.Config.title;
   if(GD.datfile.Config.maxiterations) GD.nitermax = GD.datfile.Config.maxiterations;
   if(GD.datfile.Config.convergence) GD.converge = GD.datfile.Config.convergence;
   if(GD.datfile.Config.linprec) { GD.linprecP = GD.datfile.Config.linprec; GD.linprecM = GD.datfile.Config.linprec; }
   if(GD.datfile.Config.angprecM) GD.angprecM = GD.datfile.Config.angprecM;
   if(GD.datfile.Config.angprecP) GD.angprecP = GD.datfile.Config.angprecP;
   if(GD.datfile.Config.dim == 2) EQD.is2D = true;
   if(GD.datfile.Config.confidence > 0.0) SD.alpha = GD.datfile.Config.confidence;
   if(!GD.datfile.Config.applyAPV) GD.noAPV = true;
   if(GD.datfile.Config.apQuit) GD.apquit = true;
   if(GD.datfile.Config.noExtRelVect) GD.noExtRelVect = true;
   if(!GD.datfile.Config.geoidfile.empty())
      GD.geoidfile = GD.datfile.Config.geoidfile;
   if(!GD.datfile.Config.geoidinterp.empty())
      GD.geoidinterp = GD.datfile.Config.geoidinterp;
   if(GD.datfile.Config.comments.size() > 0)
      for(i=0; i<GD.datfile.Config.comments.size(); i++)
         GD.comments.push_back(GD.datfile.Config.comments[i]);
   if(!GD.datfile.Config.hashstring.empty())
      GD.hashstr = GD.datfile.Config.hashstring;

      // overwrite only if cmdline option was present
   if(GD.cmd_nitermax > 0) GD.nitermax = GD.cmd_nitermax;
   if(GD.cmd_converge > 0) GD.converge = GD.cmd_converge;
   // NB ordering means --noAPV has precedence over --APV
   if(GD.noAPV && GD.cmd_doAPV) GD.noAPV = false;
   if(!GD.noAPV && GD.cmd_noAPV) GD.noAPV = true;

      // points
   for(i=0; i<GD.datfile.Points.size(); i++) {
      Point p(GD.datfile.Points[i]);
      GD.Points.insert(map<string,Point>::value_type(p.label,p));
   }

      // measurement data
   for(i=0; i<GD.datfile.Azimuths.size(); i++) {         // Azimuths
      Azimuth a(GD.datfile.Azimuths[i]);
      GD.Measurements.push_back(a);
   }

   for(i=0; i<GD.datfile.Deltas.size(); i++) {           // Deltas
      Delta d(GD.datfile.Deltas[i]);
      GD.Measurements.push_back(d);
   }

   // Use DATDirSet and DATDir records to create DirSets in GD.DirSets
   i = ComputeDirSetObjects();
   if(i) return i;

   for(i=0; i<GD.DirSets.size(); i++) {                  // Dirs (DirSets)
      unsigned int ii(GD.DirSets[i].dsetindex);
      for(j=0; j<GD.DirSets[i].dirindex.size(); j++) {
         unsigned int jj(GD.DirSets[i].dirindex[j]);
         Dir d(GD.datfile.Dirs[jj], GD.datfile.DirSets[ii]);
         GD.Measurements.push_back(d);

         // save GD.Measurements index in GD.DirSets[i] so output can print DirSets
         GD.DirSets[i].measindex.push_back(GD.Measurements.size()-1);
      }
   }

   for(i=0; i<GD.datfile.Dists.size(); i++) {            // Dists
      Dist d(GD.datfile.Dists[i]);
      GD.Measurements.push_back(d);
   }

   for(i=0; i<GD.datfile.HAngles.size(); i++) {          // HAngles
      HAngle h(GD.datfile.HAngles[i]);
      GD.Measurements.push_back(h);
   }

   for(i=0; i<GD.datfile.Heights.size(); i++) {          // Heights
      Height h(GD.datfile.Heights[i]);
      GD.Measurements.push_back(h);
   }

   for(i=0; i<GD.datfile.VAngles.size(); i++) {          // VAngles
      VAngle v(GD.datfile.VAngles[i]);
      GD.Measurements.push_back(v);
   }

   for(i=0; i<GD.datfile.ZAngles.size(); i++) {          // ZAngles
      ZAngle z(GD.datfile.ZAngles[i]);
      GD.Measurements.push_back(z);
   }

   // -------------------------------------------------------------------
   // pull out extraction records
   for(i=0; i<GD.datfile.Extract.size(); i++) {
      // save extraction string
      if(!isLike(GD.datfile.Extract[i],"DIRSET"))
         GD.Extracts.push_back(GD.datfile.Extract[i]);

      // must replace DIRSET records with all their DIRs
      else {
         vector<string> fields = split(GD.datfile.Extract[i],' ');
         if(fields.size() < 5 || fields[2] != "DIRSET") {
            LOG(WARNING)
               << " Warning - invalid EXTR record: " << GD.datfile.Extract[i];
            continue;
         }

         // find this DIRSET, and loop over DIRs in this group
         // NB group (DIRSET) is f[3] and block (EXTR) is f[1]
         for(j=0; j<GD.DirSets.size(); j++) {
            if(GD.DirSets[j].group == fields[3]) {     // found it
               for(unsigned int k=0; k<GD.DirSets[j].measindex.size(); k++) {
                  ostringstream oss;
                  oss << "EXTR " << fields[1] << " DIR ";
                  unsigned int kk = GD.DirSets[j].measindex[k];
                  Dir d(static_cast<Dir&>(*(GD.Measurements[kk])));
                  oss << d.From << " " << d.To;
                  GD.Extracts.push_back(oss.str());
               }
               break;
            }
         }
      }
   }

   // empty the DATfile object, its not needed anymore
   GD.datfile.clear();

   // -------------------------------------------------------------------
   // add pseudo-measurements for Points with ADJ
   map<string,Point>::iterator pit;
   for(pit=GD.Points.begin(); pit!=GD.Points.end(); ++pit) {
      if(pit->second.fixtype != 3) continue;          // only ADJ, skip the rest

      // construct a pseudo-measurement and add it
      PointMeas pm(pit->second);
      GD.Measurements.push_back(pm);
   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// For each DATDirSet in DAT file, create a DirSet object, find DATDirs that belong
/// to it and add them to the DirSet, check for orphaned DIRs, duplicate group names,
/// etc. and then save all valid DirSets in GD.DirSets. Also create an entry in
/// EQD.DSbiases for the bias estimates. Called by MoveInputToMeasurements().
/// @return lsa::INVALID_DIR_SETS if an invalid DirSet is found.
int ComputeDirSetObjects(void) throw(Exception)
{
try {
   unsigned int i,j;
   string From, Group, To, tag;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();

   // use groups to find all DIRs for each DIRSET, add them
   vector<string> groups;        // save groups - looking for duplicates.
   for(i=0; i<GD.datfile.DirSets.size(); i++) {
      bool hasFloatPoint(false);          // if DIRSET involves one non-FIXed Point
      DirSet ds;
      ds.dsetindex = i;
      ds.At = From = GD.datfile.DirSets[i].From;
      ds.group = Group = GD.datfile.DirSets[i].DSGroup;

      // is From a non-FIXed point?
      if(GD.Points.find(From) == GD.Points.end() || GD.Points[From].fixtype != 2)
         hasFloatPoint = true;

      tag = GD.datfile.DirSets[i].getTag();
      if(!tag.empty())
         LOG(WARNING) << " Warning - tag " << tag
            << " on DIRSET " << Group << " is ignored (apply tag to DIR).";

      // look for duplicate group names - esp. bad b/c all DIRs get added to first set
      if(vectorindex(groups, Group) != -1) {
         LOG(ERROR) << " Error - duplicate DIRSET group name: " << Group;
         return lsa::INVALID_DIR_SETS;
      }
      groups.push_back(Group);

      // loop over directions
      for(j=0; j<GD.datfile.Dirs.size(); j++) {
         To = GD.datfile.Dirs[j].To;

         // if group does not match this DirSet group, skip it (for now)
         if(Group != GD.datfile.Dirs[j].DSGroup) continue;

         // To cannot be From
         if(To == From) {
            LOG(ERROR) << " Error - in DIRSET " << Group
               << ", found DIR with 'To' identical to 'From': " << To << " " << From;
            return lsa::INVALID_DIR_SETS;
         }

         // is To a non-FIXed point?
         if(!hasFloatPoint &&
            (GD.Points.find(To) == GD.Points.end() || GD.Points[To].fixtype != 2))
               hasFloatPoint = true;

         // add DIR to DIRSET
         ds.dirindex.push_back(j);
      }

      // Ignore if fewer than 2 DIRs were added
      if(ds.dirindex.size() < 2) {
         LOG(WARNING) << " Warning - ignore DirSet "<< Group << " : too few DIRs.";
      }
      else if(!hasFloatPoint) {
         LOG(WARNING) << " Warning - ignore DirSet "<< Group << " : all Points FIXed.";
      }
      else {
         GD.DirSets.push_back(ds);                 // Add DIRSET to the list
                                                   // and add a bias state for it
         EQD.DSbiases.insert(map<string,double>::value_type(ds.group,0.0));
      }
   }

   // Check that there are no DIRs without a DirSet
   for(j=0; j<GD.datfile.Dirs.size(); j++) {
      bool found(false);
      for(i=0; i<GD.datfile.DirSets.size(); i++) {
         if(GD.datfile.DirSets[i].DSGroup == GD.datfile.Dirs[j].DSGroup)
            { found = true; break; }
      }
      if(!found) LOG(WARNING) << " Warning - ignore Dir with no DirSet: "
                                       << GD.datfile.Dirs[j].asString();
   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Summarize input; output configuration, Points and measurements.
/// Called by main().
int SummarizeInput(void) throw(Exception)
{
try {
   unsigned int i,j,k;
   string str;
   vector<string> flds;
   map<string,Point>::iterator it;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();

   LOG(INFO) << "\n# Title: " << GD.Title << "\n# Input file is " << GD.infile;
   if(!GD.hashstr.empty()) LOG(INFO) << "# Hash string is " << GD.hashstr;

   // comments from dat file ---------------------------------------
   if(GD.comments.size() > 0) {
      LOG(INFO) << "# Comments from input DAT file:";
      for(i=0; i<GD.comments.size(); i++)
         LOG(INFO) << " " << GD.comments[i];
   }

   // search for undefined points ----------------------------------
   // first collect all the labels found on the data
   map<string,int> allabs;                   // use map so key == unique
   for(i=0; i<GD.Measurements.size(); i++) {
      flds = GD.Measurements[i]->allLabels();
      for(j=0; j<flds.size(); j++)
         if(allabs.find(flds[j]) == allabs.end()) {
            allabs.insert(map<string,int>::value_type(flds[j],1));
         }
   }

   //Fix to Bug #1431
   if(GD.Points.size() == 0)//all points need to be autogenerated
   {
       LOG(ERROR) << " Error - invalid problem - project must contain at least one position record.";
       return lsa::PROBLEM_APRIORI;//will be unable to compute all a priori positions
   }

   // now search Points for labels that are not found
   j = 0;
   for(map<string,int>::const_iterator it=allabs.begin(); it!=allabs.end(); ++it) {
      if(GD.Points.find(it->first) == GD.Points.end()) {
         if(j++ == 0) LOG(INFO) << "\n# Unknown Points";
         LOG(INFO) << " Point " << it->first
                  << " has no initial coordinate estimate.";
         Point p(it->first);
         GD.Points.insert(map<string,Point>::value_type(it->first,p));
      }
   }

   // find a typical width for output of Point coordinates ---------
   k = GD.linwidth;
   for(it=GD.Points.begin(); it!=GD.Points.end(); ++it) {
        std::ostringstream xstream, ystream, zstream;

        xstream << std::fixed << std::setprecision(GD.linprecP) << it->second.x;
        ystream << std::fixed << std::setprecision(GD.linprecP) << it->second.y;
        zstream << std::fixed << std::setprecision(GD.linprecP) << it->second.z;

        if(xstream.str().size() > k) k = xstream.str().size();
        if(ystream.str().size() > k) k = ystream.str().size();
        if(zstream.str().size() > k) k = zstream.str().size();
   }
   if(k > unsigned(GD.linwidth)) GD.linwidth = k;

   // find the size of the largest Point label --------------------
   k=0;
   for(it=GD.Points.begin(); it!=GD.Points.end(); ++it)
      if(it->second.label.size() > k) k = it->second.label.size();

   // dump the input --------------------------------------------------
   LOG(INFO) << "\n# Summarize input:";
   LOG(INFO) << " Solve a " << (EQD.is2D ? "2":"3") << "-D problem";
   LOG(INFO) << " Output precision: linear(measurements) " << GD.linprecM
             << ", linear(positions) " << GD.linprecP
             << ", angular(measurements,SOA) " << GD.angprecM
             << ", angular(positions,SOA) " << GD.angprecP;
   LOG(INFO) << " Convergence criteria: " << GD.nitermax
            << " iterations, and convergence limit "
            << scientific << setprecision(3) << GD.converge;
   if(EQD.doSOA) { LOG(INFO) << " Express angles in equations in SOA"; }
   if(GD.noAPV) { LOG(INFO) << " Do not scale the covariance with APV"; }
   else         { LOG(INFO) << " Scale the covariance with APV"; }
   if(GD.apquit) { LOG(INFO) << " Quit after computing a priori positions."; }
   if(!GD.noExtRelVect) {LOG(INFO) << " Calculating external reliability."; }
   else                  {LOG(INFO) << " No external reliability calculated."; }
   if(!GD.geoidfile.empty()) {
      LOG(INFO) << " Geoid values extracted from file \"" << GD.geoidfile << "\"";
      LOG(INFO) << " Interpolating geoid values using the " << GD.geoidinterp
                << " interpolation method";
   } else {
       LOG(INFO) << " No geoid file specified; "
         << "corrections due to local gravity will not be applied.";
   }
   if(!GD.binfile.empty()) {
      LOG(INFO) << " GUI output directed to binary file " << GD.binfile;
   }
#ifdef SPARSE
   LOG(INFO) << " Use sparse matrices";
#endif

   LOG(INFO) << "\n Input:";
   // points
   LOG(INFO) << " Points (" << GD.Points.size() << "):";
   for(it=GD.Points.begin(); it!=GD.Points.end(); ++it)
      LOG(INFO) << " Point" << leftJustify(" ",k-it->second.label.size())
         << " " << it->second.asString(GD.linprecP,GD.linwidth);

   LOG(INFO) << "\n Measurements (" << GD.Measurements.size() << "):";
   for(i=0; i<GD.Measurements.size(); i++) {
      const int p(GD.Measurements[i]->isAngleMeas() ? GD.angprecM : GD.linprecM);
      const int w(GD.Measurements[i]->isAngleMeas() ? GD.angwidthM : GD.linwidth);
      LOG(INFO) << " " << GD.Measurements[i]->asDataString(p,w);
   }

   if(GD.DirSets.size() > 0) {
      LOG(INFO) << "\n Direction sets (" << GD.DirSets.size() << "):";
      for(i=0; i<GD.DirSets.size(); i++) {
         LOGstrm << " Group " << GD.DirSets[i].group << " with instrument at "
            << GD.DirSets[i].At << " and directions to:";
         for(j=0; j<GD.DirSets[i].measindex.size(); j++) {
            k = GD.DirSets[i].measindex[j];
            const Dir& d(dynamic_cast<Dir&>(*(GD.Measurements[k])));
            LOGstrm << " " << d.To;
         }
         LOGstrm << endl;
      }
   }

   if(GD.Points.size() == 0 || GD.Measurements.size() == 0) {
      if(GD.Points.size() == 0)
      {
          LOG(ERROR) << " Error - invalid problem - no points found.";
          return lsa::INVALID_NO_DATA;
      }
      if(GD.Measurements.size() == 0  && !GD.apquit)//Fix to Bug #1388
      {
          LOG(ERROR) << " Error - invalid problem - no data found.";
          return lsa::INVALID_NO_DATA;
      }
   }

   // write to binary file
   if(!GD.binfile.empty()) {
      GD.LSABin.Title = GD.Title;
      GD.LSABin.converge = GD.converge;
      GD.LSABin.nitermax = GD.nitermax;
      GD.LSABin.geoint = GD.geoidinterp;
      GD.LSABin.geoidfile = GD.geoidfile0;
      GD.LSABin.dim = (EQD.is2D ? 2:3);
      SolutionData& SD=SolutionData::Instance();
      if(!GD.hashstr.empty()) GD.LSABin.hashstr = GD.hashstr;
   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Define the LS problem by counting state elements and building the state Namelist
/// EQD.StateNames; count data elements and define the data Namelist EQD.DataNames.
/// Called by main().
int DefineLSProblem(void) throw(Exception)
{
try {
   unsigned int i,j,k;
   string label;
   map<string,Point>::iterator it;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   //-------------------------------------------------------------
   // find unused Points and data, and form Namelists
   //-------------------------------------------------------------
   // get list of all labels, and count how many data use each
   map<string,int> lcount;
   // start with all Point labels to the list of labels
   for(it=GD.Points.begin(); it!=GD.Points.end(); ++it)
      lcount.insert(map<string,int>::value_type(it->second.label,0));

   // loop over measurements, collecting labels and adding to the count
   for(i=0; i<GD.Measurements.size(); i++) {
      // add to list of all labels found in measurements
      vector<string> labels = GD.Measurements[i]->allLabels();
      for(j=0; j<labels.size(); j++)
         if(lcount.find(labels[j]) == lcount.end())
            lcount.insert(map<string,int>::value_type(labels[j],0));
         else
            lcount[labels[j]]++;
   }

   //-------------------------------------------------------------
   // form the Namelist for the State Vector, excluding unused Points
   // also compute a vector of all the labels actually used
   GD.usedLabels.clear();
   EQD.StateNames.clear();
   for(it=GD.Points.begin(); it!=GD.Points.end(); ++it) {
      if(it->second.fixtype == 2) continue;        // do not add fixed points
      if(lcount[it->second.label] == 0) continue;  // do not add unused points

      // keep vector of labels of Points included in the state
      GD.usedLabels.push_back(it->first);

      // add <label>X etc to the state Namelist
      EQD.StateNames += it->first + XYZ[0];
      EQD.StateNames += it->first + XYZ[1];
      if(!EQD.is2D)
         EQD.StateNames += it->first + XYZ[2];
   }

   // add states for biases of DIRSETs
   for(i=0; i<GD.DirSets.size(); i++) {
      label = string("bias") + GD.DirSets[i].group;
      EQD.StateNames += label;
   }

   // size of longest State name
   GD.stwidth = 0;
   for(i=0; i<EQD.StateNames.size(); i++)
      if(GD.stwidth < EQD.StateNames.getName(i).size())
         GD.stwidth = EQD.StateNames.getName(i).size();

   //-------------------------------------------------------------
   // must mark "unused" any data that has no partials i.e. involves only fixed Points
   // find unused measurements and mark them
   ostringstream oss;
   vector<string> unused;
   for(i=0; i<GD.Measurements.size(); i++) {
      if(GD.Measurements[i]->setUnused(GD.usedLabels)) {
         //LOG(INFO) << "Measurement " << GD.Measurements[i]->name()
         //      << " " << GD.Measurements[i]->dlabel() << " is unused.\n";
         unused.push_back(GD.Measurements[i]->name() + string(" ")
                           + GD.Measurements[i]->dlabel());
      }
   }
   if(unused.size() > 0) {
      if(unused.size() == 1)
         oss << " Warning - measurement " << unused[0] << " is unused.\n";
      else {
         oss << " Warning - there are " << unused.size() << " unused measurements.\n";
         for(i=0; i<unused.size(); i++)
            oss << "  Measurement " << unused[i] << " is unused.\n";
      }
   }

   // find unused Points
   unused.clear();
   for(map<string,int>::const_iterator jt=lcount.begin(); jt!=lcount.end(); ++jt) {
      //LOG(INFO) << "Label count " << jt->first << " " << jt->second;
      if(jt->second == 0) {
         GD.unUsedLabels.push_back(jt->first);
         //oss << " Warning - position " << jt->first << " is unused.\n";
         unused.push_back(jt->first);
      }
   }
   if(unused.size() > 0) {
      if(unused.size() == 1)
         oss << " Warning - position " << unused[0] << " is unused.\n";
      else {
         oss << " Warning - there are " << unused.size() << " unused positions.\n";
         for(i=0; i<unused.size(); i++)
            oss << "  Position " << unused[i] << " is unused.\n";
      }
   }

   GD.unusedMsg = oss.str();
   stripTrailing(GD.unusedMsg,'\n');

   //-------------------------------------------------------------
   // form the Namelist for the Data Vector
   int n(0),dn;
   vector<string> newnames;
   for(k=0,i=0; i<GD.Measurements.size(); i++) {
      label = GD.Measurements[i]->tag();
      if(label.empty()) label = StringUtils::asString(i);
      dn = GD.Measurements[i]->addDataNames(label,newnames);
      n += dn;
   }

   EQD.DataNames.clear();
   for(i=0; i<newnames.size(); i++) {
      try {
         EQD.DataNames += newnames[i];
      }
      catch(Exception& e) {
         LOG(ERROR) << " Error - tags in DAT file have yielded"
                     << " a non-unique data label: " << newnames[i];
         GNSSTK_RETHROW(e);
      }
   }

   //Determine number of constraints, EQD.Ncon
   // nCons counts number of "Constrained" Pts (cov specified pts)
   // Do not count covariance specification as observation (-3*nCons) in dof
   int nCons = 0;
   int iNcon = DefineConstraints(nCons);
   EQD.NMeas = EQD.DataNames.size() - nCons;

   //-------------------------------------------------------------
   // define the degrees of freedom
   SD.ndof = EQD.NMeas - EQD.StateNames.size() + EQD.Ncon;

   // size of longest DataName
   GD.dnwidth = 0;
   for(i=0; i<EQD.DataNames.size(); i++)
      if(GD.dnwidth < EQD.DataNames.getName(i).size())
         GD.dnwidth = EQD.DataNames.getName(i).size();

   oss.str(""); oss << "State elements (columns) are:";
   for(unsigned int i=0; i<EQD.StateNames.size(); i++)
      oss << " " << EQD.StateNames.getName(i) << "(" << i << ")";
   oss << " [Data]";
   GD.stateMsg = oss.str();

   oss.str(""); oss << "Data (rows) are:";
   for(i=0; i<EQD.DataNames.size(); i++)
      oss << " " << EQD.DataNames.getName(i) << "(" << i << ")";
   GD.dataMsg = oss.str();

   //-------------------------------------------------------------
   // build the a priori state vector
   GD.NomState0 = Vector<double>(EQD.StateNames.size(),0.0);
   for(i=0; i<EQD.StateNames.size(); i++) {
      label = EQD.StateNames.getName(i);
      if(isLike(label,"bias")) {
         GD.NomState0(i) = EQD.DSbiases[label];
      }
      else {
         j = label.size();
         string xyz = label.substr(j-1,1);
         label = label.substr(0,j-1);
         GD.NomState0(i) = (xyz == XYZ[0] ? GD.Points[label].x :
                           (xyz == XYZ[1] ? GD.Points[label].y :
                                            GD.Points[label].z));
      }
   }

   //-------------------------------------------------------------
   if(iNcon) return iNcon;

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int DefineConstraints(int &nCons) throw(Exception)
{
try {
   unsigned int i;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();

   //-------------------------------------------------------------
   // loop over Points, looking for constraints, save them for now
   EQD.Ncon = 0;
   EQD.Pts.clear(); EQD.axes.clear();
   // are there constraints on Points?
   map<string,Point>::iterator it;
   for(it=GD.Points.begin(); it!=GD.Points.end(); ++it) {
      if(it->second.constraint.empty()) continue;
      string cons(it->second.constraint);

      // skip unused points
      if(vectorindex(GD.unUsedLabels,it->first) != -1) {
         // TD Warning?
         continue;
      }

      for(i=0; i<cons.size(); i++) {
         EQD.Pts.push_back(it->first);      // must be parallel
         EQD.axes.push_back(cons[i]);       // must be parallel
         EQD.Ncon++;
         if(it->second.fixtype != 1)
             nCons++;  // constraint not from Flt
      }
   }

   if(EQD.Ncon != 0) {
      // Dump info
      LOG(INFO) << "\n# Constraints (" << EQD.Ncon << "):";
      for(i=0; i<EQD.Pts.size(); i++)
         LOG(INFO) << " Constrain "
            << (EQD.axes[i] == 'N' ? "North" :
               (EQD.axes[i] == 'E' ? "East" :
               (EQD.axes[i] == 'U' ? "Vertical" :
               (EQD.axes[i] == 'X' ? "ECEF-X" :
               (EQD.axes[i] == 'Y' ? "ECEF-Y" : "ECEF-Z")))))
            << " component of Point " << EQD.Pts[i];
   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Solve the non-linear least squares problem. Iterate the linearized least squares
/// problem: define the equation, solve it, output residuals, compute the change in
/// solution, compute statistical tests, update the Points solution, check for
/// divergence and convergence, and output.
/// Called by main().
int SolveNonLinearLSProblem(void) throw(Exception)
{
try {
   int iret(0);
   double oldchange, relresid, lenresid, angresid;
   vector<double> changes;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   // size the components
   int ndata = EQD.DataNames.size();
   int nstate = EQD.StateNames.size();
   int nconstr = EQD.Ncon;
   EQD.Data = Vector<double>(ndata, 0.0);
   EQD.MeasData = Vector<double>(ndata, 0.0);
   EQD.NomData = Vector<double>(ndata, 0.0);
   EQD.Partials = FlexMatrix<double>(ndata, nstate);
   EQD.MCov = FlexMatrix<double>(ndata, ndata);
   bool isOver(ndata > nstate-nconstr);

   // define the constraint equations
   iret = DefineConstraintEquations();
   GD.timing("DefineConstraints");
   if(iret) return iret;

   // define the SRI - can't use state names if constrained - transformation destroys
   if(EQD.Ncon > 0)
      GD.sri = SRI(EQD.StateNames.size() - EQD.V0.rows());
   else
      GD.sri = SRI(EQD.StateNames);

   // -----------------------------------------------------------------
   // iterate the linearized LS problem
   GD.niter = 1;
   oldchange = 0.0;
   GD.NomState = GD.NomState0;
   while(1) {
      // check for over-determined
      string msg("at iteration " + StringUtils::asString<int>(GD.niter));
      if(!isOver) {
         msg = string("(evenly-determined)");
         LOG(INFO) << "";
      }
      GD.timing("Start_iter_"+StringUtils::asString<int>(GD.niter));

      // clear the SRI
      GD.sri.zeroAll();

      // TD add apriori state and covariance ?
      //GD.sri.addApriori(Cov, State);

      // ---------------------------------------------
      // define the equations, filling Partials, MeasData, NomData and MCov
      //LOG(INFO) << "# Define partials, measurement covariance and data";
      iret = DefineLSEquation();
      GD.timing("DefineLSE");
      if(iret) break;

      // ---------------------------------------------
      // output the equations to a matrix file
      if(!GD.obseqnfile.empty()) {
         OutputLSEquation(GD.obseqnfile, (isOver ? GD.niter : -1));
         GD.timing("OutputLSE");
      }

      // ---------------------------------------------
      // output a table of full sigma for each measurement
      if(GD.niter == 1) {
         LOG(INFO) << "\n Data and scaled full sigma (before iteration 1)";
         unsigned int i,j,labw,datw = GD.angprecM+11;
         string label, label2;
         // get the length of the longest label
         for(labw=0,i=0; i<EQD.DataNames.size(); i++) {
            label = EQD.DataNames.getName(i);
            j = label.size();
            if(label.substr(0,3) == "Del") {    // NB keep the {}
               if(j+1 > labw) labw=j+1;         // b/c Del is followed by X|Y|Z
            }
            else if(j > labw) labw=j;
         }

         LOGstrm << " " << leftJustify("Label",labw+1)
               << setw(datw) << "Measurement"
               << " " << setw(10) << "Sigma" << endl;

         // print for all measurements
         for(i=0; i<EQD.DataNames.size(); i++) {
            label = label2 = EQD.DataNames.getName(i);

            // units on this line
            bool isLin(true);
            int prec(GD.linprecM);
            if(label.substr(0,3) == "Han" || label.substr(0,3) == "Azm" ||
               label.substr(0,3) == "Dir" ||
               label.substr(0,3) == "Zan" || label.substr(0,3) == "Van")
            {
               isLin = false;
               prec = GD.angprecM-5;       // SOA not RAD
            }
            else if(label.substr(0,3) == "Del" || label.substr(0,3) == "Dis" ||
                    label.substr(0,3) == "Hgt" || label.substr(0,5) == "PsPos")
            {
               isLin = true;
            }
            else {
               LOG(WARNING) << " Warning - unknown label: " << label;
            }
            label2 = leftJustify(label2,labw);

            // write the row
            // label, measured and nominal value
            LOGstrm << fixed << setprecision(prec) << " " << setw(labw) << label2;
            if(isLin) LOGstrm
               << " " << setw(datw) << EQD.MeasData(i);
            else LOGstrm
               << " " << setw(datw) << angleAsDMSstring(EQD.MeasData(i),GD.angprecM);

            // full sigma
            double sig(::sqrt(EQD.MCov(i,i)));
            if(!isLin) sig *= ::RAD_TO_SOA;
            LOGstrm << fixed << setprecision(5) << " " << setw(10) << sig
                  << (isLin ?  "  meters" : "  SOA") << endl;
         }
      }

      // ---------------------------------------------
      // solve the linearized least squares problem
      LOG(INFO) << "\n------------ iteration " << GD.niter << " ----------------";
      LOG(INFO) << "# Solve the least squares problem " << msg;
      iret = SolveLLSProblem(msg);
      GD.timing("SolveLLS");
      if(iret) break;

      // compute RMS residuals
      ComputeResiduals(relresid, lenresid, angresid);
      GD.timing("ComputeResid");

      // ---------------------------------------------
      // compute convergence quantities
      double change = RMS(SD.Sol);
      // NB. Leick 2.3.43 uses |dAPV|. This is usually slower, no diff in solution
      //change = ::fabs(relresid*relresid - SD.relRMSresid*SD.relRMSresid)
      //                         * double(EQD.DataNames.size())/SD.ndof;

      double fracRel(1.0), fracLen(1.0), fracAng(1.0);
      if(isOver) {
         if(GD.niter > 1) {
            fracRel = (relresid - SD.relRMSresid)/SD.relRMSresid;
            if(lenresid > 0.0)
               fracLen = ::fabs((lenresid - SD.lenRMSresid)/SD.lenRMSresid);
            if(angresid > 0.0)
               fracAng = ::fabs((angresid - SD.angRMSresid)/SD.angRMSresid);
         }

         // time history of changes
         changes.push_back(change-oldchange);

         ostringstream oss;
         if(changes.size() > 1) {
            oss << "   delta " << scientific << setprecision(2)
               << change-oldchange << " ";
            for(unsigned int i=1; i<changes.size(); i++)
               oss << (changes[i] > 0.0 ? "+" : (changes[i] < 0.0 ? "-":"0"));
         }

         LOG(INFO) << " Convergence test " << msg << " is " << scientific
                     << setprecision(3) << change << " <? " << GD.converge
                     << " : " << (change < GD.converge ? " PASS":" FAIL")
                     << oss.str();

         if(GD.doProgress)
            cout << " Convergence test " << msg << " is " << scientific
                     << setprecision(3) << change << " <? " << GD.converge
                     << " : " << (change < GD.converge ? " PASS":" FAIL")
                     << oss.str() << endl;

         //LOG(INFO) << " Convergence (RSS Adjust) " << msg << " : "
         //            << fixed << setprecision(10) << setw(13) << change
         //            << " delta " << setw(13) << change - oldchange;
         //LOG(INFO) << " Convergence (RMS RelRes) " << msg << " : "
         //            << fixed << setprecision(10) << setw(13) << relresid
         //            << " delta " << setw(13) << relresid - SD.relRMSresid
         //            << " frac " << scientific << setprecision(3) << fracRel;
         //if(lenresid > 0.0) {
         //   LOG(INFO) << " Convergence (RMS LenRes) " << msg << " : "
         //               << fixed << setprecision(10) << setw(13) << lenresid
         //               << " delta " << setw(13) << lenresid - SD.lenRMSresid
         //               << " frac " << scientific << setprecision(3) << fracLen;
         //}
         //if(angresid > 0.0) {
         //   LOG(INFO) << " Convergence (RMS AngRes) " << msg << " : "
         //               << fixed << setprecision(10) << setw(13) << angresid
         //               << " delta " << setw(13) << angresid - SD.angRMSresid
         //               << " frac " << scientific << setprecision(3) << fracAng;
         //}

         oldchange = change;
      }

      // ---------------------------------------------
      // update before breaking loop
      SD.relRMSresid = relresid;
      SD.angRMSresid = angresid;
      SD.lenRMSresid = lenresid;
      GD.timing("Output_resids");

      // ---------------------------------------------
      // check convergence here, implement below
      int endIt(0);           // -2 diverge, -1 niter limit, 0 not yet, 1 converged
      string status("continuing");
      const int Ndiverge(2);  // TD make input?
      if(GD.niter > 1 && change < GD.converge) {
         endIt = 1;                                // convergence
         status = "converged";
      }
      else if(GD.niter >= GD.nitermax) {
         endIt = -1;                               // niter limit reached
         status = "exceed iteration max";
      }
      else if(GD.niter > 1 && changes.size() > Ndiverge) {
         endIt = -2;                               // divergence

         // divergence defined as last N iterations have an increasing residual
         // NB ignore the first change: changes[0]
         int n(changes.size()-1);
         for(unsigned int i=0; i<Ndiverge; i++) {
            if(changes[n-i] < 0.0) { endIt = 0; break; }
         }
         if(endIt == -2) status = "diverged";
      }

      if(endIt != 0 && !GD.useSRIF)
      {
            if(ObtainCovarianceWithEigen())
                return lsa::PROBLEM_EIGEN;
      }

      // ---------------------------------------------
      // GUI output
      if(!GD.binfile.empty()) {
         GD.LSABin.changes.push_back(change);
         GD.LSABin.relresid.push_back(relresid);
         GD.LSABin.status.push_back(endIt);
      }

      // ---------------------------------------------
      // statistical tests / qa : APV, redundancy and std resid
      StatisticalTests(endIt != 0 || GD.statTests);
      GD.timing("StatTests");

      // update Points and NomState
      UpdatePoints();
      GD.timing("UpdatePoints");

      // ---------------------------------------------
      // break if evenly determined - iteration can do nothing and resids are zero
      //if(!isOver) break;

      // ---------------------------------------------
      // break the loop here, but first set up for final output
      if(endIt != 0) {
         string msg2("\n");
         if(endIt == 1)
            msg2 += string(" Convergence reached");
         else if(endIt == -1)
            msg2 += string(" Warning - iteration maximum met; stop");
         else if(endIt == -2)
            msg2 += string(" Warning - divergence detected; stop");
         LOG(WARNING) << msg2 << " " << msg;

         OutputLLSRMSresid(msg,GD.statTests);

         // this keeps Partials, NomData, consistent with NomState
         DefineLSEquation();

         // NomState is now the final state b/c of UpdatePoints()
         // don't change Sol as it is output as 'last iteration'
         Vector<double> dS = GD.NomState - GD.NomState0; // total adjusts
         EQD.Data = EQD.MeasData - EQD.NomData;          // net change in data
         SD.RawResid = EQD.Data - EQD.Partials * dS;     // raw resids
         //no they come out white
         //SD.Resid = EQD.White * SD.RawResid;             // relative resids


         // Calculate the reliability metrics only for the final iteration
         CalculateReliability();

         break;
      }

      // ---------------------------------------------
      // continue the iteration
      // output the solution, covariance and residuals
      if(endIt != 0 || GD.statTests || GD.verbose) {
         OutputLLSSolution(msg);
         OutputLLSDataResid(msg,GD.statTests);
      }
      OutputLLSRMSresid(msg,GD.statTests);
      GD.timing("Output(SolveLLS)");

      // increment counter
      GD.niter++;

   }  // end iteration loop

   return iret;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Define the LS equation by building the partials matrix, data vectors and
/// measurement covariance matrix.
/// Called by SolveNonLinearLSProblem().
int DefineLSEquation(void) throw(Exception)
{
try {
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();

   // compute the observation equations and measurement covariances,
   // filling the matrix equations one row at a time
   unsigned int i,row = 0;
   int n;

   EQD.MCov *= 0.0;
   EQD.Partials *= 0.0;
   for(i=0; i<GD.Measurements.size(); i++) {
      n = GD.Measurements[i]->ContributeObsEquation(GD.Points, row);
      if(n > 0) row += n;
   }

   // compute Data vector for LS problem
   EQD.Data = EQD.MeasData - EQD.NomData;

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// find constraints, define equations and get the SVD.
int DefineConstraintEquations(void) throw(Exception)
{
try {
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   int ii;

   if(EQD.Ncon == 0) return lsa::LSASOLVER_OK;

   if(!GD.useSRIF) {
      try {
         return DefineConstraintEquationsWithEigen();
      }
      catch(Exception& e) { GNSSTK_RETHROW(e); }
   }


   // Build the constraint equation
   EQD.F = Matrix<double>(EQD.Ncon, EQD.StateNames.size(), 0.0);
   //EQD.h = Vector<double>(EQD.Ncon, 0.0);

   Matrix<double> Part;
   for(ii=0; ii<EQD.Pts.size(); ii++) {
      string label = EQD.Pts[ii];
      char axis = EQD.axes[ii];

      // get the partials matrix for this Point
      if(axis == 'N' || axis == 'E' || axis == 'U')
         Part = GD.Points[label].getRotation();
      else
         Part = ident<double>(3);

      // now copy this into the current row and correct columns for the State
      int i = EQD.StateNames.index(label + XYZ[0]);
      if(i == -1)
         GNSSTK_THROW(Exception("Constraint axis not in state: " + label+XYZ[0]));
      int j = EQD.StateNames.index(label + XYZ[1]);
      if(j == -1)
         GNSSTK_THROW(Exception("Constraint axis not in state: " + label+XYZ[1]));
      int k = EQD.StateNames.index(label + XYZ[2]);
      if(k == -1)
         GNSSTK_THROW(Exception("Constraint axis not in state: " + label+XYZ[2]));

      unsigned int jj(((axis == 'N' || axis == 'X') ? 0 :
                      ((axis == 'E' || axis == 'Y') ? 1 : 2)));
      EQD.F(ii,i) = Part(jj,0);
      EQD.F(ii,j) = Part(jj,1);
      EQD.F(ii,k) = Part(jj,2);
      //EQD.h(ii) = 0.0;
   }

   //LOG(INFO) << " F: " << cleanMatrixString(EQD.F,4,6,false,false);
   //Namelist rows; for(ii=0; ii<EQD.Ncon; ii++) rows += "Cons " + asString(ii+1);
   //LabeledMatrix LF(rows,EQD.StateNames,EQD.F);
   //LF.message("Constraint F:  ");
   ////LF.setprecision(3).setw(7);
   //LF.scientific().setprecision(2).setw(9);
   //LOG(INFO) << LF;

   // get the SVD of the constraint matrix F
   SVD<double> svd;
   svd(EQD.F);
   svd.sort(true);            // now the last singular value is the smallest (0)
   const unsigned int N(EQD.F.cols());
   //LOG(INFO) << " Constraint SVs: " << scientific << setprecision(3) << svd.S;
   //LOG(INFO) << " N is " << N << " and size of state is " << EQD.StateNames.size();

   // are the constraints consistent?
   if(::fabs(svd.S[EQD.Ncon-1]) < 1.e-10) {
      LOG(ERROR) << "\nError - constraints are inconsistent "
         << scientific << svd.S[EQD.Ncon-1] << " Abort.";
      return lsa::PROBLEM_INCONSISTENT_CONSTRAINTS;
   }

   // separate the SVD into Null space and range space, and save
   // p==EQD.Ncon is the number of non-zero singular values
   // NB Only need V0 b/c h is always zero.
   //EQD.Up = svd.U;          // EQD.Ncon columns <-> non-0 SVs
   //EQD.Vp = Matrix<double>(svd.V,0,0,EQD.Ncon,N); // first Ncon columns, Range space
   // V0 = last N-Ncon columns of V = Null space of the constraints
   EQD.V0 = FlexMatrix<double>(svd.V,0,EQD.Ncon,N,N-EQD.Ncon);
   //EQD.Wp = ident<double>(EQD.Ncon);            // inverse of non-0 singular values
   //for(ii=0; ii<EQD.Ncon; ii++) EQD.Wp(ii,ii) = 1.0/svd.S[ii];

   //LOG(INFO) << " The state has length " << EQD.StateNames.size()
   //   << "; V0(" << EQD.V0.rows() << "," << EQD.V0.cols() << "):\n"
   //   << cleanMatrixString(EQD.V0,3,6,false,false);

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Compute APV (a posteriori variance of unit weight) for global fit test (sometimes
/// referred to as the Chi-Squared Test), redundancy for each observation, and standard
/// residuals for each observation. Also compute 'possible blunder/outlier' thresholds
/// for each observation using tau distribution, and mark any residuals/observations
/// that exceed (in magnitude) the corresponding threshold value (a process sometimes
/// referred to as 'data snooping').
///
/// Called by SolveNonLinearLSProblem().
void StatisticalTests(bool doAll) throw(Exception)
{
try {
   // Bring in global data objects
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   // Number of observations
   size_t NumObs = EQD.White.rows();

   // -------------------------------------------------------
   // Compute a posterior variance of unit weight (APV), the square root of which is the
   // a posteriori standard deviation of unit weight (unitless)
   // If the Degrees of Freedom (number of obs - number of estimated states) is at least 1:
   if(SD.ndof > 0) {
      // APV = (RMS(relative residuals))^2 * NumberOfObs/DOF
      SD.APV = SD.relRMSresid*SD.relRMSresid*double(NumObs)/SD.ndof;
      // don't scale the covariance in SD, otherwise redundancy is wrong.
      //if(!GD.noAPV) SD.Cov *= SD.APV;
   }
   else
      SD.APV = 0.0;        // undefined for exactly-determined problems

   // -------------------------------------------------------
   // Compute redundancy redundancy for each observation, and standard residuals for
   // each observation. Also compute 'possible blunder/outlier' thresholds for each
   // observation using tau distribution, and mark any residuals/observations that exceed
   // (in magnitude) the corresponding threshold value (a process sometimes referred to
   // as 'data snooping')
   if(doAll) {
      int i; //,j,k; j, k not used

      // Compute redundancy for each observation, equal to the residuals cofactor matrix Q_RR
      // diagonal value corresonding to the observation divided by the measurement noise
      // covariance matrix diagonal value corresonding to the observation
      // Q_RR = MCov - H Cov H^T
      // where MCov is the measurement noise covariance matrix, H is the measurement partials
      // matrix, and Cov is the estimated state covariance
      // Redundancy vector = diag(MCov^-1 * Q_RR)

      // The alternative equivalent formulation used in this method:
      // Redundancy vector = diag( I - (chol(MCov^-1) H) Cov (chol(MCov^-1) H)^T )
      // where chol is the cholesky decomposition method

      // A FlexMatrix object is equal to a GNSSTk Matrix object if the SPARSE flag is false,
      // and a GNSSTk SparseMatrix object if the SPARSE flag is true
      // EQD.White is the inverse Cholesky decomposition matrix of the measurement noise
      // covariance matrix MCov, size MxM
      // EQD.Partials is the measurement partials matrix, size MxN
      // A new matrix P is created that is equal to the inverse cholesky decomp matrix of the
      // measurement noise covariance matrix TIMES the measurement partials matrix
      FlexMatrix<double> P(EQD.White * EQD.Partials);

      // If the SPARSE flag is on
   #ifdef SPARSE
      // the sparse method transformDiag is used to compute Qvv = (chol(MCov^-1) H) Cov (chol(MCov^-1) H)^T
      Vector<double> Qvv = transformDiag(P, SD.Cov);
      // The vector of redundancy values is then computed by subtracting Qvv (a vector of length M) from a
      // a vector of 1's equal in length to the number of rows of the inverse cholesky decomp matrix of the
      // measurement noise covariance matrix (i.e. M, the number of observations)
      SD.Redund = Vector<double>(NumObs,1.0)-Qvv;
      // The vector of standard residuals SD.StdResid is initialized here, for the correct size
      SD.StdResid = Vector<double>(NumObs,0.0);
      // Also initialize SD.PossibleOutlier
      SD.PossibleOutlier = Vector<bool>(NumObs,false);
   #else
      // compute Qvv = (chol(MCov^-1) H) Cov (chol(MCov^-1) H)^T
      Matrix<double> Qvv = P * SD.Cov * transpose(P);
      // The vector of redundancy values is then computed by subtracting Qvv.diagCopy() (the vector diagonal
      // values of Qvv, length M) from a vector of 1's equal in length to the number of rows of the inverse
      // cholesky decomp matrix of the measurement noise covariance matrix (i.e. M, the number of observations)
      SD.Redund = Vector<double>(NumObs,1.0)-Qvv.diagCopy();
      // The vector of standard residuals SD.StdResid is initialized here, for the correct size
      SD.StdResid = Vector<double>(NumObs,0.0);
      // Also initialize SD.PossibleOutlier
      SD.PossibleOutlier = Vector<bool>(NumObs,false);
   #endif

      // Confirm at least 2 DOF, otherwise tau distribution breaks
      // If not, all SD.PossibleOutlier(i) kept as false - fine for unrealistic scenarios with
      // only 1 or 0 DOF
      if(SD.ndof < 2) {
          LOG(WARNING) << " Warning - DOF = 1, not enough DOF to compute Standard Residuals with Tau "
                       << "Threshold, no residuals will be marked as outliers.";
      }

      // Compute standard residuals, looping over all obs
      #pragma omp parallel for
      for(i=0; i<NumObs; i++) {
         // If the redundancy is too small, set all redundancy and standard residual values to 0
         if(SD.Redund(i) < 1.e-8)                        // entirely arbitrary
            SD.Redund(i) = SD.StdResid(i) = 0.0;
         else {
            // Divide the relative residual SD.Resid(i) by the sqrt of (APV times redundancy) to
            // compute the standard residual
            SD.StdResid(i) = SD.Resid(i)/::sqrt(SD.APV*SD.Redund(i));

            // Confirm at least 2 DOF, otherwise tau distribution breaks
            // If not, all SD.PossibleOutlier(i) kept as false - fine for unrealistic scenarios with
            // only 1 or 0 DOF
            if(SD.ndof > 1) {
                // Compute the test threshold based on the tau distribution, DOF, and confidence value
//                double confidence = 0.94; //0.95; //0.999; // TODO: make user-settable
                double confidence = SD.StdResConfid;
                // The threshold value for alpha (1-confidence) value for the two-sided test is
                // obtained by using alpha/2 in the inverse CDF calculation (assuming symmetric
                // distribution)
                double confidenceTwoSided = (1.0-confidence)/2.0 + confidence;
                // the degrees of freedom, minus 1 (for student's t-distribution below)
                int DOFM1 = SD.ndof-1;
                // Employ GNSSTk method invStudentsCDF to compute the inverse of the student's t-distribution
                double tnuM1 = invStudentsCDF(confidenceTwoSided,DOFM1);
                // Compute the corresponding tau threshold value
                double tau = tnuM1 * ::sqrt(double(SD.ndof)) / ::sqrt(double(SD.ndof) - 1.0 + tnuM1*tnuM1);
                // Then scale by the inverse of the square root of the redundancy to obtain the test threshold
                double threshold = tau / ::sqrt(SD.Redund(i));

                // determine if standard residual magnitude exceeds threshold, mark as potentially anomalous
                if(::fabs(SD.StdResid(i)) > threshold){
                    SD.PossibleOutlier(i) = true;
                }
            }
//            else {
//                LOG(WARNING) << " Warning - DOF = 1, not enough DOF to compute Standard Residuals with Tau "
//                             << "Threshold, no residuals will be marked as outliers."; }

         }
      }
   }
   // set redundancy and standard residual vectors equal to zero if statistics are not computed
   // (e.g. StatisticalTests is called prior to the final iteration, when doAll is set False)
   else {
       SD.Redund = SD.StdResid = Vector<double>(NumObs,0.0);
       SD.PossibleOutlier = Vector<bool>(NumObs,false);
   }

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Write the observation equations to a file, given the output filename
/// and the iteration number n; if n > 0, modify the filename to end in ".it<n>.mat"
/// Called by SolveNonLinearLSProblem().
int OutputLSEquation(const string& fname, const int& n) throw(Exception)
{
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   // mangle the file name
   string filename(fname);
   if(n > 0) {                                  // regular iteration number n
      vector<string> fields = split(fname,'.');
      string basename(fname);
      stripTrailing(basename,"."+fields[fields.size()-1]);
      filename = basename + ".it" + StringUtils::asString<int>(n)
                           + "." + fields[fields.size()-1];
   }

   // -------------------------------------------------------
   // open the file
   ofstream ofs;
   ofs.open(filename.c_str(),ios_base::out);
   if(!ofs.is_open()) {
      LOG(WARNING) << " Warning - failed to open observation equation output file "
         << filename << ". Abort output of obs.eqn.";
      return lsa::LSASOLVER_OK;
   }


   string msg(" at iteration " + asString<int>(n));
   if(n < 0) msg = "";
   LOG(INFO) << "\n# Writing full observation equations with "
      << "measurement covariance" << msg << " to file " << filename;

   // switch log to the new file
   // -------------------------------------------------------
   pLOGstrm = &ofs; // ConfigureLOG::Stream() = &ofs;
   LOG(INFO) << "# " << GD.RunStr;

   // -------------------------------------------------------
   static const int prec(8), width(15);      // TD input?
   int ndata(EQD.Partials.rows());
   int nstate(EQD.Partials.cols());

   // dump it
   LOG(INFO) << "# Partials concatentated with Data (J||K) from lsasolver" << msg
               << " with input file " << GD.infile;
   LOG(INFO) << "# " << GD.stateMsg;
   LOG(INFO) << "# " << GD.dataMsg;
   LOG(INFO) << "t=GEN r=" << ndata << " c=" << nstate+1;
   LOG(INFO) << ":::";

   Matrix<double> PD(EQD.Partials || EQD.Data);
   LOG(INFO) << cleanMatrixString(PD,prec,width);

   // dump the weight matrx
   LOG(INFO) << "\n# Meas.Cov from lsasolver" << msg
               << " with input file "<< GD.infile;
   LOG(INFO) << "t=SYM r=" << ndata << " c=" << ndata;
   LOG(INFO) << ":::";

   //                                                 sci  symm
   LOG(INFO) << cleanMatrixString(EQD.MCov,prec,width,true,true);

   ofs.close();                           // close the equation output file

   // -------------------------------------------------------
   // go back to the original log file
   if(GD.logfile.empty())
      pLOGstrm = &cout;
   else
      pLOGstrm = &GD.oflog;

   return lsa::LSASOLVER_OK;
}

//------------------------------------------------------------------------------------
/// Use EQD.Data, EQD.Partials, EQD.MCov to solve the linear least squares problem,
/// yielding SD.Sol, SD.Cov, SD.Resid, SD.RawResid
/// Called by SolveNonLinearLSProblem().
int SolveLLSProblem(const string& msg) throw(Exception)
{
try {
   unsigned int i,row;
   int iret;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   GD.timing("Begin SolveLLSProblem");

   if(!GD.useSRIF) {
      try {
         return SolveLLSProblemWithEigen(msg);
      }
      catch(Exception& e) { GNSSTK_RETHROW(e); }
   }

   // weight (whiten) Partials and Data
   FlexMatrix<double> L;
   try {
      L = lowerCholesky(EQD.MCov);
      GD.timing("Cholesky(SolveLLS)");
      EQD.White = inverseLT(L);
      GD.timing("WhiteInv(SolveLLS)");
   }
   catch(Exception& e) {
      LOG(ERROR) << " Error - singular measurement covariance: " << e.what();
      //GNSSTK_RETHROW(e);
      return lsa::SINGULAR_COVARIANCE;
   }

   // whiten the data and partials matrix
   // SD.Resid = whitened data on input, residuals on output
   SD.Resid = EQD.White * EQD.Data;
   GD.timing("Whiten_Data(SolveLLS)");

   // if there are constraints, project Partials into null space of constraints
   // NB this assumes EQD.h == 0
   FlexMatrix<double> P;
   if(EQD.Ncon > 0)
      P = EQD.White * EQD.Partials * EQD.V0;
   else
      P = EQD.White * EQD.Partials;
   GD.timing("Whiten_Partials(SolveLLS)");

   // print whitened problem
   //Matrix<double> PD(P || SD.Resid);
   //LOG(INFO) << "Whitened problem (" << PD.rows() << "," << PD.cols() << ")";
   //LOG(INFO) << cleanMatrixString(PD,8,15);

   // add the observation equations to the SRI
   try {
      GD.sri.measurementUpdate(P,SD.Resid);
   }
   catch(MatrixException& e) {
      //GNSSTK_RETHROW(Exception(e));
      LOG(ERROR) << " the solution is singular: " << e.what();
      return lsa::SINGULAR_LS_SOLUTION;
   }
   GD.timing("MU(SolveLLS)");

   double big,small,cond;
   try {
      GD.sri.getStateAndCovariance(SD.Sol,SD.Cov,&small,&big);
      double cond(big/small);
      LOG(INFO) << " Condition number " << msg << " is "
                  << scientific << setprecision(3)
                  << big << " / " << small << " = "
                  << (cond > 1.e3 ? scientific : fixed) << cond;
      if(cond > 1.e+13 || big != big || small != small) {
         LOG(ERROR) << " Error - condition number is too high"
                     << " - problem is too close to singular.";
         return lsa::SINGULAR_PROBLEM;
      }
   }
   catch(Exception& e) {
      // parse the error message to get coordinate at which it failed
      string errmsg(e.getText());

      if(isLike(errmsg,"Singular matrix at element")) {
         vector<string> F = split(errmsg,' ');
         int k = asInt(F[F.size()-1]);
         std::string siteName = EQD.StateNames.getName(k);
         if(siteName.substr(0,4) == std::string("bias"))
         {
             errmsg = "Singular matrix at the state element for the "+ siteName.substr(4,siteName.length()-4) + " bias";
         }
         else
         {
             std::string siteDir = siteName.substr(siteName.length()-1);
             siteName.erase(siteName.length() - 1);
             errmsg = "Singular matrix at state element "+ siteName + ", " + siteDir + " component";
         }
      }

      LOG(ERROR) << " Error - problem has no solution " << msg << " : " << errmsg;

      return lsa::SINGULAR_PROBLEM;
   }

   // if there are constraints, transform back to the full solution space
   // NB again this assumes h==0
   if(EQD.Ncon > 0) {
      SD.Sol = EQD.V0 * SD.Sol;
      GD.timing("Transform solution back to full solution space");
      SD.Cov = EQD.V0 * SD.Cov * transpose(EQD.V0);
      GD.timing("Transform cov back to full solution space");
      // must undo the Partials transform, but just so Partials has right dimension
      //EQD.Partials = EQD.Partials * transpose(EQD.V0);
      // Partials is used after this point, before redef. in next iteration
      // in StatisticalTests() - also to get RawResiduals in last iteration
   }

   // color the residuals: invWhite * SD.Resid
   SD.RawResid = L * SD.Resid;
   GD.timing("Coloring(SolveLLS)");

   //Matrix<double> PD(EQD.Partials || SD.Resid);
   //LOG(INFO) << "PostMU problem";
   //LOG(INFO) << cleanMatrixString(PD,8,15);
   //LOG(INFO) << " SRI after MU";
   //Matrix<double> SRImatrix(GD.sri.getR() || GD.sri.getZ());
   //LOG(INFO) << cleanMatrixString(SRImatrix,2,10,true);

   double sol_rms = RMS(SD.Sol);

   // the method to check for NaNs is std::isnan for GCC, _isnan for MSVC
   // instead of adding #ifdef(WIN32) blocks, this is another way to check for NaNs
   // According to IEEE 754-1985, NaN values have a property in that comparisons involving always return false
   bool nansDetectedInSolution = sol_rms != sol_rms;
   if(nansDetectedInSolution) {
       LOG(ERROR) << " Error - NaNs detected in solution. Divergence has likely occurred." << std::endl;
       return lsa::PROBLEM_NAN_DETECTED;
   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) {
   LOG(ERROR) << " Error - SolveLLS threw " << e.what();
   GNSSTK_RETHROW(e);
}
}

//------------------------------------------------------------------------------------
/// Compute relative (whitened), linear and angular RMS residuals.
/// Called by SolveNonLinearLSProblem().
int ComputeResiduals(double& relRMSresid, double& lenRMSresid, double& angRMSresid)
   throw(Exception)
{
try {
   unsigned int i;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   // compute relative (whitened) RMS residual
   relRMSresid = RMS(SD.Resid);

   // compute the raw (colored) RMS residuals
   // NB must separate angles and distances for RMS residual to make sense
   lenRMSresid = angRMSresid = 0.0;
   int nang(0),nlen(0);
   for(i=0; i<EQD.DataNames.size(); i++) {
      string label = EQD.DataNames.getName(i);
      if(label.substr(0,3) == "Han" || label.substr(0,3) == "Azm" ||
         label.substr(0,3) == "Dir" ||
         label.substr(0,3) == "Zan" || label.substr(0,3) == "Van")
      {
         angRMSresid += SD.RawResid(i)*SD.RawResid(i);
         nang++;
      }
      if(label.substr(0,3) == "Del" || label.substr(0,3) == "Dis") {
         lenRMSresid += SD.RawResid(i)*SD.RawResid(i);
         nlen++;
      }
   }
   if(nang > 1) angRMSresid = ::sqrt(angRMSresid/double(nang));
   if(nlen > 1) lenRMSresid = ::sqrt(lenRMSresid/double(nlen));

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Update the Points in GD with the linearized solution.
/// Called by SolveNonLinearLSProblem().
int UpdatePoints(void) throw(Exception)
{
try {
   unsigned int i,j;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   string comp,label;
   map<string,Point>::iterator it;
   map<string,double>::iterator jt;
   for(i=0; i<EQD.StateNames.size(); i++) {
      label = EQD.StateNames.getName(i);

      // update DirSet biases
      if(isLike(label,"bias")) {
         if((jt = EQD.DSbiases.find(label)) == EQD.DSbiases.end())
            GNSSTK_THROW(Exception("Failed to find DirSet parameter " + label));

         jt->second += SD.Sol(i);
         GD.NomState(i) = jt->second;
      }

      // update Points
      else {
         j = label.size();
         comp = label.substr(j-1,1);
         label = label.substr(0,j-1);

         // find this point and component
         if( (it = GD.Points.find(label)) == GD.Points.end())
            GNSSTK_THROW(Exception("Failed to find Point " + label));
         if(comp != "X" && comp != "Y" && comp != "Z")
            GNSSTK_THROW(Exception("State Namelist corrupted: " + label + comp));

         // add change to component of Point
         (comp == "X" ? GD.Points[label].x :
         (comp == "Y" ? GD.Points[label].y :
                        GD.Points[label].z)) += SD.Sol(i);

         // build the a priori state vector
         GD.NomState(i) = (comp == "X" ? GD.Points[label].x :
                          (comp == "Y" ? GD.Points[label].y :
                                         GD.Points[label].z));
      }
   }

   if(GD.applyGeoid) GD.geoid.UpdateGeoidValues(GD.Points);

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Calculate the reliability metrics.
/// Called by SolveNonLinearLSProblem().
int CalculateReliability(void) throw(Exception)
{
try {
   unsigned int i,j,k = 0;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   /// Calculate the number of points used for calculating reliability
   map<string,Point>::iterator pit;
   unsigned int num_points = 0;
   for(pit=GD.Points.begin(); pit!=GD.Points.end(); ++pit){
      string label = pit->first;
      if(vectorindex(GD.unUsedLabels,label) != -1 || pit->second.fixtype == 2)
      {
         continue; /// only ADJ points
      }
      else
      {
         num_points++;/// Determines how many seperate points will have values
      }
   }

   /// Establish data objects used later
   SD.vertrr = Vector<double>(num_points, 0.0);   ///< vertical (U component of ENU) portion of reliability rectangles
   SD.maj2Drr = Vector<double>(num_points, 0.0); ///< 2D major (NA) axis of reliability rectangle
   SD.maj3Drr = Vector<double>(num_points, 0.0); ///< 3D major component of reliability rectangle (includes vertrr essentially)
   SD.min2Drr = Vector<double>(num_points, 0.0); ///< 2D minor (NB) axis of reliability rectangle
   SD.azrr = Vector<double>(num_points, 0.0);    ///< Azimuth of NA axis of reliability

   EQD.ExternalReliabilityMags = Vector<double>(EQD.NMeas, 0.0);                             ///< Magnitudes of the external reliability vectors
   EQD.ExternalReliabilityVectorsENU = Matrix<double>(EQD.StateNames.size(),EQD.NMeas, 0.0); ///< External reliability vectors rotated into the ENU frame
   EQD.ExternalReliabilityVectorsRot = Matrix<double>(2,EQD.NMeas, 0.0);                     ///< External reliability vectors rotated into the reliability rectangle frame (used to calculate NA/NB of the RRs)
   EQD.MinDetectableBias = Vector<double>(EQD.NMeas, 0.0);                                   ///< Internal reliability
   EQD.BiasArray = Vector<double>(EQD.NMeas, 0.0);                                           ///< Array used to calculate external reliability [0,0,0,...,EQD.MinDetectableBias(i),0,0...,0]
   EQD.RedundancyZero = Vector<bool>(EQD.NMeas, false);                                      ///< Array used to determine if redundancy is equal to zero
   EQD.ExternalReliabilityVectors = Matrix<double>(EQD.StateNames.size(), EQD.NMeas, 0.0);   ///< The external reliability vectors calculated from the bias array

   /// Calculate internal reliability no matter what
   CalculateInternalReliability();

   /// If the user wants to calculate external reliabilty, do it
   if(!GD.noExtRelVect)
   {
      cout<< "Calculating reliability metrics...." << endl << "This can take several minutes for large problems...." << endl;
      CalculateExternalReliability();
      CalculateRectangles();
   }
   else
   {
      cout << "No external reliability calculations performed." << endl;
   }

   // Want to display internal reliability in SOA, not Rad
   if(!EQD.doSOA)
   {
      for(i=0;i<EQD.NMeas;i++){
         string label = EQD.DataNames.getName(i);
         string label_check = label.substr(0,3);
         // Only applies to angle measurements
         if(label_check == "Han" || label_check == "Azm" ||
               label_check == "Dir" ||
               label_check == "Zan" || label_check == "Van")
         {
            EQD.MinDetectableBias(i) = EQD.MinDetectableBias(i) * RAD_TO_SOA;
         }
      }
   }



   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Calculate internal reliability.
/// Called by CalculateReliability().
int CalculateInternalReliability(void) throw(Exception)
{
try {
   unsigned int i = 0;
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();
   GlobalData& GD=GlobalData::Instance();

   double delta0 = 4.22068; ///< Non-centrality parameter, as specified in math spec, equation ??
   /// For each observation, compute internal reliability metric

   /// Counts the total number of zero redundancy measurements
   unsigned int zeroRedunCount = 0;
   vector<string> zeroRedun;
   for(i=0;i<EQD.NMeas;i++){
      /// Internal reliability
      /// Compute minimum detectible bias from non-centrality perameter (delta0), measurement noise
      /// variance (EQD.MCov(i,i)), and redundancy (SD.Redund(i)), as described in math spec
      string label = EQD.DataNames.getName(i);
      if(label.substr(0,5) == "PsPos")
      {
         SD.Redund(i) = 1e-8;
         EQD.MinDetectableBias(i) = 0.0;
      }
      else if(SD.Redund(i)<=1e-9)
      {
         string label = EQD.DataNames.getName(i);
         SD.Redund(i) = 1e-8;
         EQD.MinDetectableBias(i) = delta0*sqrt(EQD.MCov(i,i)/SD.Redund(i));
         EQD.RedundancyZero(i) = true;
         zeroRedunCount++;
         zeroRedun.push_back(label);
      }
      else
      {
         EQD.MinDetectableBias(i) = delta0*sqrt(EQD.MCov(i,i)/SD.Redund(i));
      }
   }

   if(zeroRedunCount > 0)
   {
      LOG(WARNING) << string(" Warning - zero redundancy found in ") << zeroRedunCount << string(" measurements.") << endl;
      for(i = 0; i < zeroRedunCount; ++i)
      {
         LOG(WARNING) << string(" Warning - measurement ") << zeroRedun[i] << string(" has insufficient redundancy; setting redundancy value to 1E-8.") << endl;
      }
   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}


void computeReliabilityVectors(unsigned int measurementIndex, const Eigen::SparseMatrix<double>& PreMultMatrix, const Eigen::SparseMatrix<double>& sparse_v0)
{
    Eigen::VectorXd vec_data,vec_sol;
    ObsEqnData& EQD=ObsEqnData::Instance();
    Vector<double> biasArray(EQD.NMeas, 0.0);
    gnsstk::Vector<double> ExtRelVect(EQD.StateNames.size(), 0.0);

    /// Place single minimum detectible bias for this observation in the corresponding slot of the
    /// measurements residuals array
    biasArray(measurementIndex) = EQD.MinDetectableBias(measurementIndex);

    /// Convert EQD.BiasArray to eigen object
    eigen_copy_vec_from_tk(biasArray,vec_data);

    vec_sol = PreMultMatrix*vec_data;

    /// Transform back into full solution space, if needed
    if(EQD.Ncon > 0)
    {
       vec_sol = sparse_v0 * vec_sol;
    }

    /// Convert eigen solution vector to Tk vector
    eigen_copy_vec_to_tk(ExtRelVect,vec_sol);

    /// Place external reliability vector in column corresponding to this observation
    #pragma omp parallel for
    for(int j=0;j<EQD.StateNames.size();j++){
       EQD.ExternalReliabilityVectors(j,measurementIndex) = ExtRelVect(j);
    }
}


//------------------------------------------------------------------------------------
/// Calculate external reliability.
/// Called by CalculateReliability().
int CalculateExternalReliability(void) throw(Exception)
{
try {
   unsigned int i,j,k = 0;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   gnsstk::Matrix<double> ExtRelVectMatrix; ///< The matrix that is multipled by the minimum detectable bias array
   gnsstk::Vector<double> ExtRelVect(EQD.StateNames.size(),0.0);

   /// Initialize required Eigen matrices and vectors
   Eigen::SparseMatrix<double> sparse_partials(EQD.Partials.rows(),EQD.Partials.cols());
   Eigen::SparseMatrix<double> sparse_v0(EQD.V0.rows(),EQD.V0.cols());
   Eigen::SparseMatrix<double> sparse_invcov(EQD.Partials.cols(),EQD.Partials.cols());
   Eigen::SparseMatrix<double> sparse_cov, PreMultMatrix;
   Eigen::SparseMatrix<double> sparse_mcov(EQD.MCov.rows(),EQD.MCov.cols());
   Eigen::SparseMatrix<double> sparse_weight(EQD.MCov.rows(),EQD.MCov.cols());
   Eigen::VectorXd vec_data,vec_sol,vec_weighted;

   int eigen_return_status = 0;

   /// Calculation of external reliability metrics

   /// Copy values from gnsstk objects to eigen objects
   eigen_copy_mat_from_tk(EQD.V0,sparse_v0);
   eigen_copy_mat_from_tk(EQD.Partials,sparse_partials);
   eigen_copy_mat_from_tk(EQD.MCov,sparse_mcov);

   /// Compute measurement weight matrix = inverse of measurement noise covariance matrix
   eigen_return_status = eigen_sparse_inverse(sparse_mcov,sparse_weight);
   if(eigen_return_status != 0)
   {
      LOG(ERROR) << lsa::FASTSOLVER_ERRORSTRING << " - MCov inversion failed in CalculateReliability";
      return lsa::PROBLEM_EIGEN;
   }

   /// If there is at least one constraint, reduce the partials matrix to the range space (i.e. rotate
   /// into the space that accounts for the constraints)
   if(EQD.Ncon > 0)
   {
      sparse_partials = sparse_partials * sparse_v0;
   }

   /// Compute information matrix (inverse of the covariance)
   sparse_invcov = sparse_partials.transpose()*sparse_weight*sparse_partials;
   sparse_invcov.makeCompressed(); /// needed?

   /// Compute matrix PreMultMatrix = (H'*W*H)^-1 * H'*W before measurement loop
   /// To do that, compute sparse_cov (from ObtainCovarianceWithEigen method in lsaeigen.cpp)
   eigen_return_status = eigen_sparse_inverse(sparse_invcov,sparse_cov);
   if(eigen_return_status != 0)
   {
      LOG(ERROR) << lsa::FASTSOLVER_ERRORSTRING << " - covariance inversion failed";
      return lsa::PROBLEM_EIGEN;
   }

   /// Now compute full (H'*W*H)^-1 * H'*W matrix
   PreMultMatrix = sparse_cov*sparse_partials.transpose()*sparse_weight;

   /// For each observation, compute internal reliability metric and external reliability vector/magnitude
   omp_set_nested(1);
   #pragma omp parallel for
   for(int itr=0;itr<EQD.NMeas;itr++){
      computeReliabilityVectors(itr, PreMultMatrix, sparse_v0);
   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Calculate rectangle parameters.
/// Called by CalculateReliability().
int CalculateRectangles(void) throw(Exception)
{
try {
   unsigned int i,j,k = 0;
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();
   GlobalData& GD=GlobalData::Instance();

   gnsstk::Matrix<double> R_Rot(2,2,0.0);           ///< The rotation matrix to convert to reliability rectangles
   gnsstk::Matrix<double> RotENU(3,3, 0.0);         ///< The rotation matrix to convert ECEF external reliability vectors to ENU
   gnsstk::Vector<double> localMag(EQD.NMeas, 0.0); ///< Store the external reliability vector mags on a per-point basis
   gnsstk::Vector<double> PreRotVec(3, 0.0);        ///< vector that holds ecef external reliability vector
   gnsstk::Vector<double> PostRotVec(3, 0.0);       ///< vector that holds enu external reliability vector

   double norm_val = 0, vertrr = 0, maxValue2D =0, maxValue3D = 0, t_rot=0;
   unsigned int state_locator = 0, start_index=0, maxIndex=0;

   /// Calculate the points used
   map<string,Point>::iterator pit;
   unsigned int num_points = 0;
   for(pit=GD.Points.begin(); pit!=GD.Points.end(); ++pit){
      string label = pit->first;
      if(vectorindex(GD.unUsedLabels,label) != -1 || pit->second.fixtype == 2)
      {
         continue; /// only ADJ points
      }
      else
      {
         num_points++;/// Determines how many seperate points will have values
      }
   }

   for(pit=GD.Points.begin(); pit!=GD.Points.end(); ++pit){
      string label2 = pit->first;
      if(vectorindex(GD.unUsedLabels,label2) != -1 || pit->second.fixtype == 2)
      {
         continue; /// only ADJ points
      }
      else
      {
         /// This must be done on a point by point basis
         double phi, lambda;
         gnsstk::Position p;
         p.setECEF(pit->second.x, pit->second.y, pit->second.z);
         phi = p.getGeodeticLatitude();
         lambda = p.getLongitude();
         phi *= M_PI/180;
         lambda *= M_PI/180;
         RotENU(0,0) = -sin(lambda);          RotENU(0,1) = cos(lambda);           RotENU(0,2) = 0;
         RotENU(1,0) = -cos(lambda)*sin(phi); RotENU(1,1) = -sin(lambda)*sin(phi); RotENU(1,2) = cos(phi);
         RotENU(2,0) = cos(lambda)*cos(phi);  RotENU(2,1) = sin(lambda)*cos(phi);  RotENU(2,2) = sin(phi);

         /// Iterate through the measurements and rotate them
         for(i=0;i<EQD.NMeas;i++){
            /// Reset the rotation vectors
            PreRotVec *= 0.0;
            /// Check 2D or 3D
            if(!EQD.is2D) /// 3D case
            {
               start_index = state_locator*3;
               /// Prepare vector to be rotated
               for(j=0;j<3;j++){
                  PreRotVec(j) = EQD.ExternalReliabilityVectors(start_index + j,i);
               }
               /// External magnitude calculation (3D)
               unsigned int tracker = 3;
               for(j=0;j<num_points;j++){
                  norm_val = 0;
                  norm_val += EQD.ExternalReliabilityVectors(j*tracker,i) * EQD.ExternalReliabilityVectors(j*tracker,i);
                  norm_val += EQD.ExternalReliabilityVectors(j*tracker+1,i) * EQD.ExternalReliabilityVectors(j*tracker+1,i);
                  norm_val += EQD.ExternalReliabilityVectors(j*tracker+2,i) * EQD.ExternalReliabilityVectors(j*tracker+2,i);
                  norm_val = ::sqrt(norm_val);
                  if(norm_val >= EQD.ExternalReliabilityMags(i))
                  {
                     EQD.ExternalReliabilityMags(i) = norm_val;
                  }
               }
            }
            else /// 2D case
            {
               start_index = state_locator*2;
               /// Prepare vector to be rotated
               for(j=0;j<2;j++){
                  PreRotVec(j) = EQD.ExternalReliabilityVectors(start_index + j,i);
               }
               PreRotVec(2) = 0;

               /// External magnitude calculation (2D)
               unsigned int tracker = 2;
               for(j=0;j<num_points;j++){
                  norm_val = 0;
                  norm_val += EQD.ExternalReliabilityVectors(j*tracker,i) * EQD.ExternalReliabilityVectors(j*tracker,i);
                  norm_val += EQD.ExternalReliabilityVectors(j*tracker+1,i) * EQD.ExternalReliabilityVectors(j*tracker+1,i);
                  norm_val = ::sqrt(norm_val);

                  if(norm_val >= EQD.ExternalReliabilityMags(i))
                  {
                     EQD.ExternalReliabilityMags(i) = norm_val;
                  }
               }
            }

            /// Convert to ENU
            for(j=0;j<3;j++){
               for(k=0;k<3;k++){
                  EQD.ExternalReliabilityVectorsENU(start_index+j,i) += RotENU(j,k) * PreRotVec(k);
               }
            }

            /// Save the largest vertical component
            if(abs(EQD.ExternalReliabilityVectorsENU(start_index+2,i)) > SD.vertrr(state_locator))
            {
               SD.vertrr(state_locator) = abs(EQD.ExternalReliabilityVectorsENU(start_index+2,i));
            }

            /// Check the 3D/2D maj component
            if(EQD.is2D) /// 2D case
            {
               for(j=0;j<2;j++){
                  localMag(i) += EQD.ExternalReliabilityVectorsENU(start_index+j,i)*EQD.ExternalReliabilityVectorsENU(start_index+j,i);
               }
               localMag(i) = ::sqrt(localMag(i));
               /// Store the largest vector and which observation it occurs at

               if(localMag(i) > SD.maj2Drr(state_locator))
               {
                  SD.maj2Drr(state_locator) = localMag(i);
                  SD.maj3Drr(state_locator) = localMag(i);

                  maxIndex = i;
               }
            }
            else /// 3D case
            {
               for(j=0;j<3;j++){
                  localMag(i) += EQD.ExternalReliabilityVectorsENU(start_index+j,i)*EQD.ExternalReliabilityVectorsENU(start_index+j,i);
               }

               localMag(i) = ::sqrt(localMag(i));

               /// Store the largest vector and which observation it occurs at 2D
               if(localMag(i) > SD.maj2Drr(state_locator))
               {
                  SD.maj2Drr(state_locator) = localMag(i);
               }

               /// Including 3D
               localMag(i) = localMag(i) * localMag(i);
               localMag(i) += EQD.ExternalReliabilityVectorsENU(start_index+2,i)*EQD.ExternalReliabilityVectorsENU(start_index+2,i);
               localMag(i) = ::sqrt(localMag(i));
               /// Store the largest vector and which observation it occurs at 3D
               if(abs(localMag(i)) > SD.maj3Drr(state_locator))
               {
                  SD.maj3Drr(state_locator) = abs(localMag(i));

                  maxIndex = i;
               }
            }
         } /// End of measurement loop

         /// Using the new ENU external reliability vectors, find the RR values
         /// Calculation of 2D & 3D major axis lengths. These are 1/2 length values of the full rectangle (semi-axis)

         EQD.ExternalReliabilityVectorsRot = Matrix<double>(2, EQD.NMeas); ///< Defining the size of the rotated external reliability vectors, only need 2D because graphic is 2D
         t_rot = atan2(EQD.ExternalReliabilityVectorsENU(start_index+1,maxIndex), EQD.ExternalReliabilityVectorsENU(start_index,maxIndex));  ///< rotation angle magnitude CW from East radians
         R_Rot(0,0) = cos(t_rot); ///< rotation matrix
         R_Rot(0,1) = -sin(t_rot);
         R_Rot(1,0) = sin(t_rot);
         R_Rot(1,1) = cos(t_rot);

         /// Rotating the vectors for display
         unsigned int ii;
         for(ii = 0; ii < EQD.NMeas; ii++){
            for(j=0;j<2;j++){
               for(k=0;k<2;k++){
                  EQD.ExternalReliabilityVectorsRot(j,ii) = R_Rot(j,k) * EQD.ExternalReliabilityVectorsENU(start_index+k,ii);
               }
            }
            ///Check to store NB value
            if(abs(EQD.ExternalReliabilityVectorsRot(1,ii)) > SD.min2Drr(state_locator))
            {
               SD.min2Drr(state_locator) = abs(EQD.ExternalReliabilityVectorsRot(1,ii));
            }
         }

         ///Az value for plotting the rectangles
         double theta = M_PI/2 - t_rot;
         if(theta < 0)
         {
            theta += 2*M_PI;
         }
         theta *= 180/M_PI; ///degrees
         SD.azrr(state_locator) = theta;

         /// End of loop actions
         state_locator++;
         maxValue2D = maxValue3D = maxIndex = 0; /// reset for next point
         localMag *= 0.0;
      } /// end this points calculation
   }/// end of all the point iterations

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
/// Output (pretty print) the solution of the linearized problem, including solution,
/// covariance, a priori, adjustment for the state vector (Points).
/// Called by SolveNonLinearLSProblem().
int OutputLLSSolution(const string& msg) throw(Exception)
{
try {
   unsigned int i,j,k,datw;
   string label;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   // length of largest of [state names, "Component"]
   int labw = (GD.stwidth+1 > 5 ? GD.stwidth+1 : 5);

   //-------------------------------------------------------------------------
   // pretty print Solution and covariance
   // NB this only works if state is Px Py Pz Ax Ay Az Rx Ry Rz etc
   LOGstrm << "\n Solution: Adjusts XYZ (m) Biases (rad) " << msg << endl
             << " " << leftJustify("State",labw)
             << rightJustify("Apriori",GD.linwidth+1)
             << rightJustify("Adjust",GD.linwidth+1)
             << rightJustify("Result",GD.linwidth+1)
             << rightJustify("Sigma",12)
             << rightJustify("Covariance",32) << endl;

   // scale if user chooses to
   Matrix<double> Cov(SD.Cov);
   if(!GD.noAPV) Cov *= SD.APV;

   // print the table
   for(j=0,i=0; i<EQD.StateNames.size(); i++) {
      label = EQD.StateNames.getName(i);
      k = label.size();
      bool isBias(isLike(label,"bias"));
      if(!isBias) label = label.substr(0,k-1) + " " + label.substr(k-1,1);
      LOGstrm << " " << leftJustify(label,labw)              // NB label is modified
         << fixed << setprecision(GD.linprecM)
         << " " << setw(GD.linwidth) << GD.NomState(i) - SD.Sol(i)  // Apriori: must use NomState - Sol, because NomState0 is the a priori for the entire adjustment, not each iteration
         << " " << setw(GD.linwidth) << SD.Sol(i)                   // Adjustment
         << " " << setw(GD.linwidth) << GD.NomState(i)              // Result
         << scientific << setprecision(2)
         << "   " << setw(9) << ::sqrt(Cov(i,i));

      if(isBias) {
         LOGstrm << " radians = " << fixed
                     << ::sqrt(Cov(i,i))*::RAD_TO_SOA << " SOA.";
      }
      else if(EQD.is2D) {
         if(i==2*j) LOGstrm << "   " << setw(9) << Cov(i,i)
                     << " " << setw(9) << Cov(i,i+1)
                     << " " << setw(9) << 0;
         else if(i==2*j+1) LOGstrm << "   " << leftJustify(" ",9)
                     << " " << setw(9) << Cov(i,i) << " " << setw(9) << 0;
         if((i+1) % 2 == 0) j++;
      }
      else {
         if(i==3*j) LOGstrm << "   " << setw(9) << Cov(i,i)
                     << " " << setw(9) << Cov(i,i+1)
                     << " " << setw(9) << Cov(i,i+2);
         else if(i==3*j+1) LOGstrm << "   " << leftJustify(" ",9)
                     << " " << setw(9) << Cov(i,i)
                     << " " << setw(9) << Cov(i,i+1);
         else if(i==3*j+2) LOGstrm << "   " << leftJustify(" ",9)
                     << " " << leftJustify(" ",9)
                     << " " << setw(9) << Cov(i,i);
         if((i+1) % 3 == 0) j++;
      }
      LOGstrm << endl;
   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Output (pretty print) data and residuals
/// Called by SolveNonLinearLSProblem() and FinalOutput()
int OutputLLSDataResid(const string& msg, bool doAll) throw(Exception)
{
try {
   unsigned int i,j,k,labw;
   double resid;
   string label,label2,str;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();
   // sorted list of residuals
   multimap<double,string> saveRows;

   // sorted list of ext rel vect mags
   multimap<double,string> extMagRows;

   LOG(INFO) << "\n Data and residuals " << msg;
   int datw = GD.angprecM+11;
   // get the length of the longest label
   for(labw=0,i=0; i<EQD.DataNames.size(); i++) {
      label = EQD.DataNames.getName(i);
      j = label.size();
      if(label.substr(0,3) == "Del") {    // NB keep the {}
         if(j+1 > labw) labw=j+1;         // b/c Del is followed by X|Y|Z
      }
      else if(j > labw) labw=j;
   }

   LOGstrm << " " << leftJustify("Label",labw+1)
           << setw(datw) << "Meas(m|DMS)"
           << " " << setw(datw) << "Nomin(m|DMS)"
           << " " << setw(datw) << "Pre-R(m|soa)"
           << " " << setw(datw) << "Post-R(m|soa)"
           << " " << setw(datw-3) << "Rel-Res";
   if(doAll) LOGstrm
           << " " << setw(9) << "Std-Res"
           << " " << setw(9) << "Redund"
           << " " << setw(datw) << "Int-Rel(m|SOA)"
           << " " << setw(datw) << "Ext-Rel-E(m)"
           << " " << setw(datw) << "Ext-Rel-N(m)"
           << " " << setw(datw) << "Ext-Rel-U(m)"
           << " " << setw(datw) << "Ext-Rel-Mag(m)";

   LOGstrm << endl;

   // print for all measurements - output for DIRSETs follows
   for(i=0; i<EQD.DataNames.size(); i++) {
      label = label2 = EQD.DataNames.getName(i);

      // units on this line
      bool isLin(true);
      int prec(GD.linprecM);
      if(label.substr(0,3) == "Han" || label.substr(0,3) == "Azm" ||
         label.substr(0,3) == "Dir" ||
         label.substr(0,3) == "Zan" || label.substr(0,3) == "Van")
      {
         isLin = false;
         prec = GD.angprecM;        // SOA not RAD
      }
      else if(label.substr(0,3) == "Del" || label.substr(0,3) == "Dis" ||
              label.substr(0,3) == "Hgt" || label.substr(0,5) == "PsPos")
      {
         isLin = true;
      }
      else { LOG(WARNING) << " Warning - unknown label: " << label; }
      label2 = leftJustify(label2,labw);

      // write the row
      ostringstream oss;

      // label, measured and nominal value
      oss << fixed << setprecision(prec)
         << " " << setw(labw) << label2;
      if(isLin) oss
         << " " << setw(datw) << EQD.MeasData(i)
         << " " << setw(datw) << EQD.NomData(i);
      else oss
         << " " << setw(datw) << angleAsDMSstring(EQD.MeasData(i),GD.angprecM)
         << " " << setw(datw) << angleAsDMSstring(EQD.NomData(i),GD.angprecM);

      // residuals
      oss << " " << setw(datw)
         << SD.RawResid(i) * (isLin ? 1:RAD_TO_SOA)
         << " " << setw(datw+1) << (EQD.MeasData(i)-EQD.NomData(i)) * (isLin ? 1:RAD_TO_SOA)
         << " " << setw(datw-3) << SD.Resid(i);       // RelResid

      // redundancy and std residual
      if(doAll) {

         // std residual and blunder detection
         resid = SD.StdResid(i);
         if(resid == 0.0)
            oss << " " << setw(9) << "--";
         else {
            // flag possible blunders
            oss << " " << setw(9) << resid;
            if(SD.PossibleOutlier(i)) oss << " **";
         }

         ///< redundancy
         oss << " " << setw(9) << setprecision(5) << SD.Redund(i);

         /// Internal reliability value
         oss << " " << setw(datw) << setprecision(5) << EQD.MinDetectableBias(i);

         /// External Reliability vector values
         if(!GD.noExtRelVect)
         {
            oss << " " << setw(datw) << setprecision(5) << EQD.ExternalReliabilityVectorsENU(0,i);
            oss << " " << setw(datw) << setprecision(5) << EQD.ExternalReliabilityVectorsENU(1,i);
            oss << " " << setw(datw) << setprecision(5) << EQD.ExternalReliabilityVectorsENU(2,i);
            oss << " " << setw(datw) << setprecision(5) << EQD.ExternalReliabilityMags(i);

         }
         else
         {
            oss << " " << setw(datw) << setprecision(5) << "--";
            oss << " " << setw(datw) << setprecision(5) << "--";
            oss << " " << setw(datw) << setprecision(5) << "--";
            oss << " " << setw(datw) << setprecision(5) << "--";

         }

         // save |resid| in map with index
         str = oss.str();
         saveRows.insert(map<double,string>::value_type(::fabs(resid),str));

         // save |ext mag| in map with index
         if(!GD.noExtRelVect)
         {
            extMagRows.insert(map<double,string>::value_type(::fabs(EQD.ExternalReliabilityMags(i)),str));
         }

         // GUI output
         if(msg==string("(Final)") && !GD.binfile.empty()) {
            data_result dr;
            dr.meas = EQD.MeasData(i);
            dr.nomin = EQD.NomData(i);
            dr.rawresid =  SD.RawResid(i);
            dr.resid = SD.Resid(i);
            dr.redundancy = SD.Redund(i);
            dr.minDectBias = EQD.MinDetectableBias(i);
            dr.extVectMag = EQD.ExternalReliabilityMags(i);
            dr.stdresid = SD.StdResid(i);
            dr.PossibleOutlier = SD.PossibleOutlier(i);
            dr.RedundancyZero = EQD.RedundancyZero(i);
            GD.LSABin.dataResults.insert(
               map<std::string,data_result>::value_type(label,dr));

         }
      }

      // units
      //oss << (isLin ?  "   meters" : "   SOA");

      LOG(INFO) << oss.str();
   }  // end loop over DataNames

   // finish the GUI output
   if(msg==string("(Final)") && !GD.binfile.empty()) {
      // save the final Names and Covariance here
      GD.LSABin.DataNames = EQD.DataNames;
      GD.LSABin.StateNames = EQD.StateNames;
      //GD.LSABin.Partials = Matrix<double>(EQD.Partials);
      GD.LSABin.Covariance = SD.Cov;

   }

   // print the largest 3 residuals
   if(doAll) {
      static const int Nrows(3);

      if(!GD.noExtRelVect)
      {
         LOG(INFO) << "\n Largest " << Nrows
            << " external reliability vector magnitudes:";
         map<double,string>::reverse_iterator jt = extMagRows.rbegin();
         j = 0;
         while(j < Nrows && jt != extMagRows.rend()) {
            LOG(INFO) << jt->second;
            ++j; ++jt;
         }
      }
      LOG(INFO) << "\n Largest " << Nrows
         << " standard residuals (** denotes possible blunder:"
         << " threshold based on Tau distribution and redundancy for each obs)";
      map<double,string>::reverse_iterator it = saveRows.rbegin();
      i = 0;
      while(i < Nrows && it != saveRows.rend()) {
         LOG(INFO) << it->second;
         ++i; ++it;
      }


   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Output (pretty print) the RMS post-fit relative, linear and angular residuals.
/// Also print the APV.
/// Called by SolveNonLinearLSProblem() and FinalOutput()
int OutputLLSRMSresid(const string& msg, bool doChiSq) throw(Exception)
{
try {
   GlobalData& GD=GlobalData::Instance();
   SolutionData& SD=SolutionData::Instance();

   ostringstream oss;
   oss << "\n RMS post-fit relative residual " << msg << " = "
      << scientific << setprecision(GD.linprecM) << SD.relRMSresid << endl;
   oss << " Degrees of freedom = " << SD.ndof
      << fixed << setprecision(GD.linprecM)
      << "; Std dev of unit weight = " << ::sqrt(SD.APV)
      << "; APV = " << SD.APV << " " << msg;
   if(SD.angRMSresid > 0.0)
      oss << "\n RMS post-fit raw residual (angles) " << msg << " = "
         << scientific << setprecision(GD.angprecM) << SD.angRMSresid
            << " rad = " << SD.angRMSresid*RAD_TO_SOA << " soa.";
   if(SD.lenRMSresid > 0.0)
      oss << "\n RMS post-fit raw residual (length) " << msg << " = "
         << scientific << setprecision(GD.linprecM) << SD.lenRMSresid << " m.";

   LOG(INFO) << oss.str();
   if(GD.doProgress && isLike(msg,"Final")) cout << oss.str() << endl;

   // -------------------------------------------------------
   // test: Ghilani.16.2, Section 16.7
   if(doChiSq && SD.ndof > 0) {
      try {
         // chi squared test at probability alpha
         // upper Chi squared test
         SD.upperChiSq = invChisqCDF(1.0-SD.alpha,SD.ndof);
         // lower Chi squared test
         SD.lowerChiSq = invChisqCDF(SD.alpha,SD.ndof);
      }
      catch(Exception& e) {
         LOG(WARNING) << " Warning - chi-squared could not be computed;"
                              << " too many degrees of freedom.";
         SD.upperChiSq = SD.lowerChiSq = 0.0;
      }

      oss.str("");
      oss << " Chi-squared test lower bound (" << fixed << setprecision(3)
          << SD.alpha << "): " << SD.lowerChiSq/SD.ndof << " < " << SD.APV;
      SD.lowerChiSq/SD.ndof <= SD.APV ? oss << " pass" : oss << " fail";
      oss << "\n Chi-squared test upper bound (" << fixed << setprecision(3)
          << 1.0 - SD.alpha << "): " << SD.APV << " < " << SD.upperChiSq/SD.ndof;
      SD.APV <= SD.upperChiSq/SD.ndof ? oss << " pass" : oss << " fail";


      LOG(INFO) << oss.str();
      if(GD.doProgress) cout << oss.str() << endl;

      // add to binary file
      if(!GD.binfile.empty()) {
         GD.LSABin.relRMSresid = SD.relRMSresid;
         GD.LSABin.APV = SD.APV;
         GD.LSABin.angRMSresid = SD.angRMSresid;
         GD.LSABin.lenRMSresid = SD.lenRMSresid;
         GD.LSABin.confidence = SD.alpha;
         GD.LSABin.lowerChiSq = SD.lowerChiSq;
         GD.LSABin.upperChiSq = SD.upperChiSq;

      }
   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Output the final adjusted positions with net adjustments and sigma/covariance.
/// Called by main().
int FinalOutput(void) throw(Exception)
{
try {
   int i,j,k,datw,iret;
   string label,label2,str;
   vector<string> fields;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   LOG(INFO) << "\n# Final solution --------------------------------";

   // NB SolveNonLinearLSProblem has set up the quantities to reflect total change
   OutputLLSDataResid("(Final)",true);
   GD.timing("Final_OutputLLSDataResid");

   OutputLLSRMSresid("(Final)",true);
   GD.timing("Final_OutputLLSRMSresid");

   //-------------------------------------------------------------------------
   // compute and save per Point: CovXYZ CovNEU geodeticLLH astronLLH, etc
   map<string, FinalData> DataMap;
   map<string, FinalData>::const_iterator kt;

   // copy the full covariance matrix - do not scale with APV
   Matrix<double> FullCov(SD.Cov);
   // never do this, by design. if(!GD.noAPV) FullCov *= SD.APV;

   // loop over points computing FinalData products, save in a map
   const int labw = (GD.plwidth > 8 ? GD.plwidth : 8);
   map<string,Point>::iterator it;
   for(it=GD.Points.begin(); it != GD.Points.end(); ++it) {
      const Point& p(it->second);
      label = p.label;

      // fill an object for this Point
      FinalData fd;
      fd.Nominal0 = Vector<double>(3,0.0);
      fd.CovXYZ = Matrix<double>(3,3,0.0);
      fd.CovNEU = Matrix<double>(3,3,0.0);
      fd.offsetNEU = Vector<double>(3,0.0);
      fd.APV = (GD.noAPV ? 1.0 : SD.APV);
      fd.APS = (GD.noAPV ? 1.0 : ::sqrt(SD.APV));

      if(!EQD.is2D) {
         double lat,lon,ht;
         p.getLatLonHeight(lat, lon, ht, false);   // false for radians
         fd.glat = lat;    // geodetic lat N in rad
         fd.glon = lon;    // geodetic lon E in rad
         fd.ght = ht;      // geodetic height in m

         if(GD.applyGeoid) {
            p.getAstroLatLonHeight(lat, lon, ht, false);
            fd.olat = lat; // orthometric lat N in rad
            fd.olon = lon; // orthometric lon E in rad
            fd.oht = ht;   // orthometric height in m
         }  // end if geoid
      }  // end if not 2D

      // the rest assumes it is used, and not fixed
      if(vectorindex(GD.unUsedLabels,label) != -1 || p.fixtype == 2) {
         fd.siglat = fd.siglon = fd.sight = 0.0;
      }
      else {
         // find the state indexes for X,Y,Z of this point
         i = EQD.StateNames.index(label + XYZ[0]);
         j = EQD.StateNames.index(label + XYZ[1]);
         k = EQD.StateNames.index(label + XYZ[2]);

         fd.Nominal0(0) = GD.NomState0(i);
         fd.Nominal0(1) = GD.NomState0(j);
         fd.Nominal0(2) = GD.NomState0(k);
         fd.CovXYZ(0,0) = FullCov(i,i);
         fd.CovXYZ(0,1) = fd.CovXYZ(1,0) = FullCov(i,j);
         fd.CovXYZ(0,2) = fd.CovXYZ(2,0) = (EQD.is2D ? 0.0 : FullCov(i,k));
         fd.CovXYZ(1,1) = FullCov(j,j);
         fd.CovXYZ(1,2) = fd.CovXYZ(2,1) = (EQD.is2D ? 0.0 : FullCov(j,k));
         fd.CovXYZ(2,2) = (EQD.is2D ? 0.0 : FullCov(k,k));

         if(!EQD.is2D) {
            // rotation matrix and covariance in NEU
            fd.Rot = p.getRotation();
            fd.CovNEU = fd.Rot * fd.CovXYZ * transpose(fd.Rot);

            // sigmas NEU
            fd.siglat = ::sqrt(fd.CovNEU(0,0));
            fd.siglon = ::sqrt(fd.CovNEU(1,1));
            fd.sight = ::sqrt(fd.CovNEU(2,2));
            if(fd.siglat != fd.siglat) fd.siglat = 0.0;        // don't print nan's
            if(fd.siglon != fd.siglon) fd.siglon = 0.0;
            if(fd.sight != fd.sight) fd.sight = 0.0;

            // rotate the offset vector (adjustments) into NEU
            Vector<double> offset(3);
            offset(0) = p.x - fd.Nominal0(0);
            offset(1) = p.y - fd.Nominal0(1);
            offset(2) = p.z - fd.Nominal0(2);
            fd.offsetNEU = fd.Rot*offset;
         }  // end if not 2D
      }  // end if used and not fixed

      // add to map
      DataMap.insert(map<string,FinalData>::value_type(label,fd));
   }

   //-------------------------------------------------------------------------
   // table of XYZ positions with adjustments and covariance
   LOG(INFO) << "\n Final Adjusted Positions XYZ";
   LOGstrm << " " << leftJustify("Position",labw)
           << rightJustify("X(m)",GD.linwidth+1)
           << rightJustify("Y(m)",GD.linwidth+1)
           << rightJustify("Z(m)",GD.linwidth+1)
           << "     "
           << rightJustify("Xadj(m)",GD.linprecP+5)
           << rightJustify("Yadj(m)",GD.linprecP+5)
           << rightJustify("Zadj(m)",GD.linprecP+5)
           << "   "
           << center("Covariance (XX XY XZ YY YZ ZZ) m*m",54) << endl;

   map<double,string> saveRows;                 // save largest Nrows
   for(kt=DataMap.begin(); kt != DataMap.end(); ++kt) {
      label2 = label = kt->first;
      const Point& p(GD.Points[label]);

      ostringstream oss;
      oss << " " << leftJustify(label2,labw)          // NB leftJustify changes label2
         << fixed << setprecision(GD.linprecP)         // final coordinates
         << " " << setw(GD.linwidth) << p.x
         << " " << setw(GD.linwidth) << p.y;
      if(EQD.is2D) oss << " " << setw(GD.linwidth) << '0';
      else oss << " " << setw(GD.linwidth) << p.z;

      if(vectorindex(GD.unUsedLabels,label) != -1) oss << "  UNUSED";
      else if(p.fixtype == 2) oss << "  FIXED";
      else {
         const FinalData& fd(kt->second);
         oss << "     "                               // adjustments
             << " " << setw(GD.linprecP+4) << p.x - fd.Nominal0(0)
             << " " << setw(GD.linprecP+4) << p.y - fd.Nominal0(1);
         if(EQD.is2D)
            oss << " " << setw(GD.linprecP+4) << '0';
         else
            oss << " " << setw(GD.linprecP+4) << p.z - fd.Nominal0(2);

         oss << scientific << setprecision(2)         // covariance
            << "   " << setw(9) << fd.CovXYZ(0,0)*fd.APV
            << " " << setw(9) << fd.CovXYZ(0,1)*fd.APV;
         if(EQD.is2D) oss << " " << setw(9) << '0'
                          << " " << setw(9) << fd.CovXYZ(1,1)*fd.APV
                          << " " << setw(9) << '0'
                          << " " << setw(9) << '0';
         else oss << " " << setw(9) << fd.CovXYZ(0,2)*fd.APV
                  << " " << setw(9) << fd.CovXYZ(1,1)*fd.APV
                  << " " << setw(9) << fd.CovXYZ(1,2)*fd.APV
                  << " " << setw(9) << fd.CovXYZ(2,2)*fd.APV;

         // save the line
         double dx(p.x - fd.Nominal0(0));
         double dy(p.y - fd.Nominal0(1));
         double dz(EQD.is2D ? 0.0 : p.z - fd.Nominal0(2));
         double tot(std::sqrt(dx*dx+dy*dy+dz*dz));
         saveRows.insert(map<double,string>::value_type(tot,oss.str()));
      }
      LOGstrm << oss.str() << endl;
   }

   //-------------------------------------------------------------------------
   // print Nrows largest adjustments
   static const int Nrows(3);
   if(saveRows.size() > Nrows) {
      LOG(INFO) << "\n Largest adjustments:";
      map<double,string>::reverse_iterator rit = saveRows.rbegin();
      i = 0;
      while(i < Nrows && rit != saveRows.rend()) {
         LOG(INFO) << rit->second;
         ++i; ++rit;
      }
   }

   //-------------------------------------------------------------------------
   // if its truly 2D, LLH and the rest makes no sense....
   if(EQD.is2D) return lsa::LSASOLVER_OK;

   //-------------------------------------------------------------------------
   // table of LLH
   int lwid(4+GD.linprecP > 9 ? 4+GD.linprecP : 9);
   int awid(4+GD.angprecM > 10 ? 4+GD.angprecM : 10);
   LOGstrm << "\n Final Adjusted Positions LLH" << endl
           << " " << leftJustify("Position",labw)
           << rightJustify("Latitude N",GD.angprecP+11)
           << rightJustify(GD.doWest ? "Longitude W":"Longitude E",GD.angprecP+12)
           << rightJustify("Height(m)",lwid+1)
           << rightJustify("Nadj(m)",lwid+1)
           << rightJustify("Eadj(m)",lwid+1)
           << rightJustify("Vadj(m)",lwid+1)
           << rightJustify("Nsig(m)",10)
           << rightJustify("Esig(m)",10)
           << rightJustify("Vsig(m)",10)
           << endl;

   for(kt=DataMap.begin(); kt != DataMap.end(); ++kt) {
      label2 = label = kt->first;
      const Point& p(GD.Points[label]);
      const FinalData& fd(kt->second);

      LOGstrm << " " << leftJustify(label2,labw)         // final positions LLH
              << " " << setw(GD.angprecP+10)
              << angleAsDMSstring(fd.glat,GD.angprecP)
              << " " << setw(GD.angprecP+11)
              << angleAsDMSstring(GD.doWest ? TwoPi-fd.glon: fd.glon,GD.angprecP)
              << fixed
              << " " << setprecision(GD.linprecP) << setw(lwid) << fd.ght;

      if(vectorindex(GD.unUsedLabels,label) != -1) LOGstrm << "  UNUSED";
      else if(p.fixtype == 2) LOGstrm << "  FIXED";
      else {
         LOGstrm
            << " " << setw(lwid) << fd.offsetNEU(0) // adjustments NEU
            << " " << setw(lwid) << fd.offsetNEU(1)
            << " " << setw(lwid) << fd.offsetNEU(2)
            << scientific << setprecision(2)
            << " " << setw(9) << fd.siglat*fd.APS               // sigma NEU
            << " " << setw(9) << fd.siglon*fd.APS
            << " " << setw(9) << fd.sight*fd.APS;

      }  // end if not unused or fixed
      LOGstrm << endl;

   }  // end loop over points

   //-------------------------------------------------------------------------
   // table of Astronomic Positions
   if(GD.applyGeoid) {
      // write to log file
      //int lwid(4+GD.linprec > 9 ? 4+GD.linprec : 9);
      //int awid(4+GD.angprecM > 10 ? 4+GD.angprecM : 10);
      LOGstrm << "\n Final Adjusted Positions Astronomic\n"
              << " " << leftJustify("Position",labw)
              << rightJustify("Latitude N",GD.angprecP+11)
              << rightJustify(GD.doWest?"Longitude W":"Longitude E",GD.angprecP+12)
              << rightJustify("Ortho(m)",lwid+1)
              << rightJustify("DoV(SoA) N",awid+1)
              << rightJustify("East",awid+1)
              << rightJustify("Und (m)",lwid+1) << endl;

      for(kt=DataMap.begin(); kt != DataMap.end(); ++kt) {
         label2 = label = kt->first;
         const Point& p(GD.Points[label]);
         const FinalData& fd(kt->second);

         LOGstrm << " " << leftJustify(label2,labw) << fixed
                 << " " << setw(GD.angprecP+10)
                 << angleAsDMSstring(fd.olat,GD.angprecP)
                 << " " << setw(GD.angprecP+11)
                 << angleAsDMSstring(GD.doWest ? TwoPi-fd.olon : fd.olon,GD.angprecP)
                 << " " << setprecision(GD.linprecP) << setw(lwid) << fd.oht
                 << " " << setprecision(GD.angprecM) << setw(awid) << p.dovN
                 << " " << setw(awid) << p.dovE
                 << " " << setprecision(GD.linprecP) << setw(lwid) << p.und
                 << endl;
      }  // end loop over points
   }  // end if geoid

   //-------------------------------------------------------------------------
   // add to binary file - never scale in the binary file
   if(!GD.binfile.empty()) {
      // loop over points
       unsigned int state_locator = 0;
      for(kt=DataMap.begin(); kt != DataMap.end(); ++kt) {
         label2 = label = kt->first;
         const Point& p(GD.Points[label]);
         const FinalData& fd(kt->second);

         // ignore unused Points
         if(vectorindex(GD.unUsedLabels,label) != -1)
         {
            continue;
         }
         else
         {
            // XYZ
            posXYZ_result prxyz;
            prxyz.X = p.x;
            prxyz.Y = p.y;
            prxyz.Z = p.z;
            prxyz.adjX = p.x - fd.Nominal0(0);
            prxyz.adjY = p.y - fd.Nominal0(1);
            prxyz.adjZ = (EQD.is2D ? 0.0 : p.z - fd.Nominal0(2));
            prxyz.sigX = ::sqrt(fd.CovXYZ(0,0));
            prxyz.sigY = ::sqrt(fd.CovXYZ(1,1));
            prxyz.sigZ = (EQD.is2D ? 0.0 : ::sqrt(fd.CovXYZ(2,2)));
            prxyz.Cxx = fd.CovXYZ(0,0);
            prxyz.Cxy = fd.CovXYZ(0,1);
            prxyz.Cxz = fd.CovXYZ(0,2);
            prxyz.Cyy = fd.CovXYZ(1,1);
            prxyz.Cyz = fd.CovXYZ(1,2);
            prxyz.Czz = fd.CovXYZ(2,2);
            prxyz.computed = (vectorindex(GD.computeAPlabels, label)==-1 ? 0:1);
            prxyz.constraint[0] = (p.constraint.find("N") == string::npos ? 0 : 1);
            prxyz.constraint[1] = (p.constraint.find("E") == string::npos ? 0 : 1);
            prxyz.constraint[2] = (p.constraint.find("U") == string::npos ? 0 : 1);
            GD.LSABin.posXYZResults.insert(
                  map<std::string,posXYZ_result>::value_type(label,prxyz));

            // LLH
            posLLH_result prllh;
            prllh.lat = fd.glat;                // radians North
            prllh.lon = fd.glon;                // radians East
            prllh.ht = fd.ght;

            str = angleAsDMSstring(fd.glat,GD.angprecP);  // Lat D M S
            fields = split(str,' ');
            prllh.latd = asInt(fields[0]);
            prllh.latm = asInt(fields[1]);
            prllh.lats = asDouble(fields[2]);

            str = angleAsDMSstring(fd.glon,GD.angprecP);  // Lon D M S
            fields = split(str,' ');
            prllh.lond = asInt(fields[0]);
            prllh.lonm = asInt(fields[1]);
            prllh.lons = asDouble(fields[2]);

            prllh.adjN = fd.offsetNEU(0);
            prllh.adjE = fd.offsetNEU(1);
            prllh.adjU = fd.offsetNEU(2);

            prllh.sigN = fd.siglat;
            prllh.sigE = fd.siglon;
            prllh.sigU = fd.sight;

            prllh.Cnn = fd.CovNEU(0,0);
            prllh.Cne = fd.CovNEU(0,1);
            prllh.Cnu = fd.CovNEU(0,2);
            prllh.Cee = fd.CovNEU(1,1);
            prllh.Ceu = fd.CovNEU(1,2);
            prllh.Cuu = fd.CovNEU(2,2);

            prllh.und = p.und;      // m
            prllh.dovN = p.dovN;    // SOA
            prllh.dovE = p.dovE;    // SOA

            if(p.fixtype == 2)
            {
                prllh.maj2Drr = 0;
                prllh.min2Drr = 0;
                prllh.maj3Drr = 0;
                prllh.vertrr = 0;
                prllh.azrr = 0;//SD.azrr;

            }
            else
            {
                prllh.maj2Drr = SD.maj2Drr(state_locator);
                prllh.min2Drr = SD.min2Drr(state_locator);
                prllh.maj3Drr = SD.maj3Drr(state_locator);
                prllh.vertrr = SD.vertrr(state_locator);
                prllh.azrr = SD.azrr(state_locator);//SD.azrr;
                state_locator++;
            }
            prllh.computed = (vectorindex(GD.computeAPlabels, label) == -1 ? 0:1);
            GD.LSABin.posLLHResults.insert(
                           map<std::string,posLLH_result>::value_type(label,prllh));
         }
      }  // end loop over points
   } // end of binary file


   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
/// Use extraction records in the input to extract and print values.
int ComputeExtraction(void) throw(Exception)
{
try {
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();
   if(GD.Extracts.size() == 0) return lsa::LSASOLVER_OK;

   unsigned int i,j;
   int k;
   string str,blk,msg;
   vector<Extraction> Extrs;     // use vector not map so order is preserved

   // TD? preprocess Extracts strings - allow blank blk,
   // and blk=="*" meaning all measurements of that type.

   // build the Extraction objects, and add to vector
   for(i=0; i<GD.Extracts.size(); i++) {
      str = GD.Extracts[i];
      blk = Extraction::getBlockFromString(str,msg);

      // find the Extraction object; add one if necessary
      for(k=-1,j=0; j<Extrs.size(); j++)
         if(blk == Extrs[j].getBlock()) { k=j; break; }

      if(k == -1) {
         Extraction e(blk);
         k = Extrs.size();
         Extrs.push_back(e);
      }

      // add the string
      if(!Extrs[k].Add(str,msg,EQD.StateNames)) {
         LOG(WARNING) << " Warning - parsing failed for >" << str << "<: " << msg;
         continue;
      }
   }

   // compute and output for each extraction
   for(i=0; i<Extrs.size(); i++) {
      try {
         Extrs[i].Compute(EQD.StateNames, GD.Points, SD.Cov, SD.APV,
                           EQD.is2D, GD.noAPV);
         Extrs[i].Output(LOGstrm,GD.linprecM,EQD.is2D,EQD.doSOA);
      }
      catch(Exception& e) {
         LOG(WARNING) << " Warning - extraction " << Extrs[i].getBlock()
            << " failed: " << e.getText();
      }
   }

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
// Should probably use DATfile::Write()
int DumpDAT(void) throw(Exception)
{
try {
   unsigned int i;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();

   if(GD.outdatfile.empty()) return lsa::LSASOLVER_OK;
   string filename(GD.outdatfile);
   ofstream ostrm;
   ostrm.open(filename.c_str(), ios::out);
   if(!ostrm.is_open()) {
      LOG(ERROR) << "Error - could not open file " << filename;
      return lsa::UNOPENED_DAT_FILE;
   }

   ostream *saveostrm(pLOGstrm);
   ConfigureLOG::Stream() = &ostrm;

   LOG(INFO) << "# Final DAT file";

   LOG(INFO) << "TITLE " << GD.Title;
   LOG(INFO) << "CONV " << GD.nitermax << " ITER "
               << scientific << setprecision(2) << GD.converge << " CONV";
   LOG(INFO) << "PREC " << ::pow(1.0,-GD.linprecM) << " M "
                        << ::pow(1.0,-GD.angprecM) << " RAD";
   if(GD.noAPV) LOG(INFO) << "OUT NOAPV";
   if(!GD.noExtRelVect) LOG(INFO) << "EXTRELVECT YES";
   if(GD.noExtRelVect) LOG(INFO) << "EXTRELVECT NO";
   if(EQD.is2D) LOG(INFO) << "DIM 2";

   map<string,Point>::iterator it;
   for(it=GD.Points.begin(); it!=GD.Points.end(); ++it) {
      DATPoint d(static_cast<DATPoint&>(it->second));
      LOG(INFO) << d.asDATString();
   }

   for(i=0; i<GD.Measurements.size(); i++) {
      string name(GD.Measurements[i]->name());
      if(name == string("Del")) {
         Delta d(static_cast<Delta&>(*(GD.Measurements[i])));
         LOG(INFO) << d.asDATString();
      }
      else if(name == string("Dir")) {
         Dir d(static_cast<Dir&>(*(GD.Measurements[i])));
         LOG(INFO) << d.asDATString();
      }
      else if(name == string("Dis")) {
         Dist d(static_cast<Dist&>(*(GD.Measurements[i])));
         LOG(INFO) << d.asDATString();
      }
      else if(name == string("Han")) {
         HAngle d(static_cast<HAngle&>(*(GD.Measurements[i])));
         LOG(INFO) << d.asDATString();
      }
      else if(name == string("Azm")) {
         Azimuth d(static_cast<Azimuth&>(*(GD.Measurements[i])));
         LOG(INFO) << d.asDATString();
      }
      else if(name == string("Hgt")) {
         Height d(static_cast<Height&>(*(GD.Measurements[i])));
         LOG(INFO) << d.asDATString();
      }
      else if(name == string("Van")) {
         VAngle d(static_cast<VAngle&>(*(GD.Measurements[i])));
         LOG(INFO) << d.asDATString();
      }
      else if(name == string("Zan")) {
         ZAngle d(static_cast<ZAngle&>(*(GD.Measurements[i])));
         LOG(INFO) << d.asDATString();
      }
   }

   //LOG(INFO) << "Finished output to dat file " << filename;
   ostrm.close();

   pLOGstrm = saveostrm;
   LOG(INFO) << "\nWrote output dat file " << filename;

   return lsa::LSASOLVER_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
