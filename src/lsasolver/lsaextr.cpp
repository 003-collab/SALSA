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
/// @file lsaextr.cpp  Extraction for lsasolver

#include "lsaextr.hpp"

#include "HAngle.hpp"
#include "VAngle.hpp"
#include "Azimuth.hpp"
#include "Delta.hpp"
#include "Dir.hpp"
#include "Dist.hpp"
#include "Height.hpp"
#include "ZAngle.hpp"

#include "logstream.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
// Add an extraction record, of the form EXTR block TYP label label...
// and convert to measurement
// Block must match this->block, unless it is undefined, in which case block is set.
// param extraction string to add
// param msg return msg explaining why record could not be stored.
// return true if the block string matches and the record parses and is stored.
bool Extraction::Add(const string& extrstr, string& msg, const Namelist& StateNames)
   throw(Exception)
{
try {
   string line(extrstr);

   // parse the string
   stripTrailing(line,"\n");
   stripTrailing(line,"\r");
   stripTrailing(line," ");
   stripLeading(line," ");
   if(line.empty()) {
      msg = "Record is blank";
      return false;
   }

   // F[0] = EXTR
   // F[1] = block
   // F[2] = measurement type e.g. HAN POS etc
   // F[3+] = labels
   vector<string> F = splitWithDoubleQuotes(line,' ');
   int n = F.size();
   if(n < 4) {
      msg = "Fewer than 4 fields";
      return false;
   }

   // test block
   string myblk(block);            // don't define blk yet
   if(myblk.empty()) myblk = F[1];
   if(myblk != F[1]) {
      msg = "Tags do not match";
      return false;
   }

   // test type
   if(!DATtype::isDATtype(F[2])) {
      msg = "Third field is not a DAT type";
      return false;
   }

   // check all the two-label records
   if(F[2] != string("POS") && n < 5) {
      msg = "Not enough fields in record";
      return false;
   }
   if(F[2] == string("HAN") && n < 6) {
      msg = "HAN needs three labels";
      return false;
   }

   // define the point/measurement and add it
   if(F[2] == string("POS")) {
      Point pt(F[3], 0.0,0.0,0.0, 0.0,0.0,0.0, 0.0,0.0,0.0);
      // make sure this Point is in the state
      string label = pt.label + "X";
      if(StateNames.index(label) == -1) {
         msg = "POS is not in the state";
         return false;
      }
      ExtrPoints.push_back(pt);
   }
   else if(F[2] == string("DEL")) {
      DATDelta dat(F[3],F[4], 0.0,0.0,0.0, 0.0,0.0,0.0, 0.0,0.0,0.0);
      Delta del(dat);
      ExtrMeas.push_back(del);
   }

   else if(F[2] == string("DIRSET")) {
      // TD how to extract a full DIRSET?  keep group? Allow extraction of DIR alone?
   }
   else if(F[2] == string("DIR")) {
      // NB not the DAT format, since we don't have DIRSET
      // EXTR block DIR <from> <to>
      Dir dir;
      dir.From = F[3];
      dir.To = F[4];
      ExtrMeas.push_back(dir);
   }

   else if(F[2] == string("DIS")) {
      DATDist dat(F[3],F[4],0.0,0.0);
      Dist dis(dat);
      ExtrMeas.push_back(dis);
   }
   else if(F[2] == string("HGT")) {
      DATHeight dat(F[3],F[4],0.0,0.0);
      Height ht(dat);
      ExtrMeas.push_back(ht);
   }
   else if(F[2] == string("AZM")) {
      DATAzimuth dat(F[3],F[4],0.0,0.0);
      Azimuth azm(dat);
      ExtrMeas.push_back(azm);
   }
   else if(F[2] == string("HAN")) {
      DATHAngle dat(F[3], F[4], F[5], 0.0, 0.0);
      HAngle han(dat);
      ExtrMeas.push_back(han);
   }
   else if(F[2] == string("VAN")) {
      DATVAngle dat(F[3],F[4],0.0,0.0);
      VAngle van(dat);
      ExtrMeas.push_back(van);
   }
   else if(F[2] == string("ZAN")) {
      DATZAngle dat(F[3],F[4],0.0,0.0);
      ZAngle zan(dat);
      ExtrMeas.push_back(zan);
   }
   else {
      msg = "Measurement is unknown: " + F[2];
      return false;
   }

   if(block.empty()) block = myblk;

   return true;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
void Extraction::Compute(const Namelist& StateNames,
                         map<string,Point>& AllPoints,
                         const Matrix<double>& Cov,
                         const double& APV,
                         const bool& is2D,
                         const bool& noAPV) throw(Exception)
{
try {
   unsigned int i,j;
   int k;
   string label;

   // count states
   int N(0);
   N += (is2D ? 2 : 3)*ExtrPoints.size();
   for(i=0; i<ExtrMeas.size(); i++) {
      if(ExtrMeas[i]->name() == string("Del"))
         N += (is2D ? 2 : 3);
      else
         N++;
   }
   LOG(DEBUG) << " Extraction " << block << " has " << N << " states, "
      << ExtrPoints.size() << " points, and " << ExtrMeas.size() << " measurements.";

   // construct partials matrix and nominal data
   Partials = Matrix<double>(N,StateNames.size(),0.0);
   NominalData = Vector<double>(N);
   labels.clear();                     // member
   Vector<double> mdata(N);            // dummy
   int row(0);
   
   // add ExtrPoints first
   // NB this must parallel the construction of StateNames above
   for(i=0; i<ExtrPoints.size(); i++) {
      for(j=0; j<3; j++) {                // loop over XYZ
         label = ExtrPoints[i].label + XYZ[j];
         labels.push_back("Pos " + label);
         LOG(DEBUG) << " Add Point " << label;

         // find it in state
         k = StateNames.index(label);
         label = ExtrPoints[i].label;
         if(k == -1) {
            LOG(DEBUG) << " Point not found in state " << label;
            GNSSTK_THROW(Exception("Point not found in state: " + label));
         }

         // add to partials and data
         Partials(row,k) = 1.0;
         NominalData(row) = (j == 0 ? AllPoints[label].x :
                            (j == 1 ? AllPoints[label].y : AllPoints[label].z));
         row++;

         // quit early if 2D (no Z)
         if(is2D && j==1) break;
      }
   }

   // now add Measurements
   bool doSOA=false;//doesn't work with doSOA=true
   for(i=0; i<ExtrMeas.size(); i++) {
      ExtrMeas[i]->FillObservationEquation(AllPoints, StateNames, Partials,
                                     mdata, NominalData, row, doSOA, is2D);
      row++;
      label = ExtrMeas[i]->dlabel();

      // save the label
      if(ExtrMeas[i]->name() != string("Del"))
         labels.push_back(ExtrMeas[i]->name() + " " + label);

      else {      // but Deltas contribute 3 (2 if 2D)
         // save the indexes for covariance matrix
         //Vector<int> indexes(is2D ? 2 : 3);

         //indexes(0) = row;
         labels.push_back("Del "+label+"X");

         row++;
         labels.push_back("Del "+label+"Y");
         //indexes(1) = row;

         if(!is2D) {
            row++;
            labels.push_back("Del "+label+"Z");
            //indexes(2) = row;
         }
       //DeltaIndexesMap.insert(map< string,Vector<int> >::value_type(label,indexes));
      }

      // TwoPi ambiguity arises b/c measurement = 0, nominal-meas is adjusted to < 2pi
      if(ExtrMeas[i]->name() == string("Azm") || ExtrMeas[i]->name() == string("Han"))
      {
          if(NominalData(row-1) < 0.0) NominalData(row-1) += TwoPi;
      }
   }

   // form the covariance matrix for this set of data
   Covariance = Partials * Cov * transpose(Partials);
   if(!noAPV) Covariance *= APV;

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
void Extraction::Output(ostream& os, const int& prec, const bool& is2D, const bool& doSOA)
   throw(Exception)
{
try {
   unsigned int i,j;
   double sig;

   // build the list of values first
   vector<string> values;
   isangle.clear();                 // member = 1/0 as angle/not
   j = 0;
   for(i=0; i<ExtrPoints.size(); i++) {
      values.push_back(asString(NominalData(j++),prec));
      isangle.push_back(0);
      values.push_back(asString(NominalData(j++),prec));
      isangle.push_back(0);
      if(!is2D) {
         values.push_back(asString(NominalData(j++),prec));
         isangle.push_back(0);
      }
   }
   for(i=0; i<ExtrMeas.size(); i++) {
      if(ExtrMeas[i]->isAngleMeas()) {
         values.push_back(angleAsDMSstring(NominalData(j++)));
         isangle.push_back(1);
      }
      else if(ExtrMeas[i]->name() == string("Del")) {
         values.push_back(asString(NominalData(j++),prec));
         isangle.push_back(0);
         values.push_back(asString(NominalData(j++),prec));
         isangle.push_back(0);
         if(!is2D) {
            values.push_back(asString(NominalData(j++),prec));
            isangle.push_back(0);
         }
      }
      else {
         values.push_back(asString(NominalData(j++),prec));
         isangle.push_back(0);
      }
   }
   if(labels.size() != Covariance.rows())
      GNSSTK_THROW(Exception("Dimension mis-match"));

   // are DIS and/or DEL in the labels?
   bool wantPPM(false);
   for(i=0; i<labels.size(); i++) {
      if(labels[i].substr(0,3) == "Dis") { wantPPM = true; break; }
      if(labels[i].substr(0,3) == "Del") { wantPPM = true; break; }
   }

   // TD how to get rotation matrix to get NEU ?
   // get the horizontal semi-major axis of the covariance matrix for each Delta
   //map<string,double> HorizontalSigma;
   //map< string,Vector<int> >::const_iterator it = DeltaIndexesMap.begin();
   //for( ; it != DeltaIndexesMap.end(); ++it) {
   //   string str(it->first);
   //   int dim(is2D ? 2 : 3);
   //   Matrix<double> cov(dim,dim);
   //   //labels.push_back("Del "+label+"X");
   //}

   // get the length of the longest label and value
   int labw(11+block.size()), valw(10);
   for(i=0; i<labels.size(); i++) {
      if(labels[i].size() > labw) labw = labels[i].size();
      if(values[i].size() > valw) valw = values[i].size();
   }

   os << "\n " << leftJustify("Extraction "+block,labw)
      << " " << setw(valw) << "Value";
   if(wantPPM) os << " " << setw(9) << "Sig(PPM)";
   os << " " << setw(9) << "Sigma";
   if(labels.size() > 1) os << "     Correlation Matrix";
   os << endl;
   
   // loop over extracted data
   for(i=0; i<labels.size(); i++) {
      // output label and value
      string lab(labels[i]);
      os << " " << leftJustify(lab,labw) << " " << setw(valw) << values[i];

      // compute (PPM for Dis or Del and) sigma
      sig = ::sqrt(Covariance(i,i));
      if(isangle[i] && doSOA) sig *= RAD_TO_SOA;
      if(wantPPM && (lab.substr(0,3)=="Dis" || lab.substr(0,3)=="Del"))
         os << " " << setprecision(2) << setw(9) << 1.e6*sig/asDouble(values[i]);
      else if(wantPPM)
         os << " " << setw(9) << " ";
      os << " " << scientific << setprecision(2) << setw(9) << sig;

      // output correlation matrix
      if(labels.size() > 1) {
         os << fixed << setprecision(3);
         for(j=0; j<labels.size(); j++) {
            os << " " << setw(j==0 ? 5:9);
            if(j == i)   os << "1";
            else if(j>i) {
               if(Covariance(i,i)*Covariance(j,j) > 0.0)
                  os << Covariance(i,j)/::sqrt(Covariance(i,i)*Covariance(j,j));
               else
                  os << "NA";
            }
            else         os << " ";
         }
      }

      os << endl;
   }

   //// output again, with full unscaled covariance matrix
   //for(i=0; i<labels.size(); i++) {
   //   // output label and value
   //   string lab(labels[i]);
   //   os << " " << leftJustify(lab,labw) << " " << setw(valw) << values[i];

   //   // output sigma
   //   sig = ::sqrt(Covariance(i,i));
   //   if(isangle[i]) sig *= RAD_TO_SOA;
   //   os << " " << scientific << setprecision(2) << setw(9) << sig;

   //   // output covariance matrix
   //   for(j=0; j<labels.size(); j++) {
   //      os << " " << setw(9);
   //      if(j>=i) os << Covariance(i,j);
   //      else     os << " ";
   //   }
   //   os << "\n";
   //}

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

