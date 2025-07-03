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
/// @file lsabinary.cpp
/// Implement binary output file for lsasolver, including input and output.

#include "lsabinary.hpp"
#include "Point.hpp"
#include "lsaUtils.hpp"
#include "Position.hpp"
#include "logstream.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// ------------------------------------------------------------------------
// write a string to a binary strm
void LSABinaryFile::writeString(ostream& os, string str) throw(Exception)
{
   try {
      int len = str.size();
      os.write(reinterpret_cast<char*>(&len), sizeof(len));
      os.write(const_cast<char*>(str.c_str()), len);
   }
   catch(exception& e) {
      Exception ge(string("Write string failed: ") + e.what());
      GNSSTK_THROW(ge);
   }
}

// write an integer to a binary stream
void LSABinaryFile::writeInt(ostream& os, int in) throw(Exception)
{
   try {
      os.write(reinterpret_cast<char*>(&in), sizeof(int));
   }
   catch(exception& e) {
      Exception ge(string("Write int failed: ") + e.what());
      GNSSTK_THROW(ge);
   }
}

// write a double to a binary stream
void LSABinaryFile::writeDouble(ostream& os, double in) throw(Exception)
{
   try {
      os.write(reinterpret_cast<char*>(&in), sizeof(double));
   }
   catch(exception& e) {
      Exception ge(string("Write double failed: ") + e.what());
      GNSSTK_THROW(ge);
   }
}

// ------------------------------------------------------------------------
// read a string from a binary file
string LSABinaryFile::readString(ifstream& is) throw(Exception)
{
   try {
      int len;
      string str;
      is.read(reinterpret_cast<char*>(&len), sizeof(len));
      str.resize(len);
      is.read(const_cast<char*>(str.c_str()), len);
      return str;
   }
   catch(exception& e) {
      Exception ge(string("Read string failed: ") + e.what());
      GNSSTK_THROW(ge);
   }
}

// read an int from a binary file
double LSABinaryFile::readDouble(ifstream& is) throw(Exception)
{
   try {
      double len;
      is.read(reinterpret_cast<char*>(&len), sizeof(double));
      return len;
   }
   catch(exception& e) {
      Exception ge(string("Read double failed: ") + e.what());
      GNSSTK_THROW(ge);
   }
}

// read an int from a binary file
int LSABinaryFile::readInt(ifstream& is) throw(Exception)
{
   try {
      int len;
      is.read(reinterpret_cast<char*>(&len), sizeof(int));
      return len;
   }
   catch(exception& e) {
      Exception ge(string("Read int failed: ") + e.what());
      GNSSTK_THROW(ge);
   }
}

// ------------------------------------------------------------------------
// Write the binary file using this object. This routine must strictly parallel Read()
// param filename name of binary file to be written
// throw on stream error
int LSABinaryFile::WriteBinaryFile(string& fn) throw(Exception)
{
try {
   unsigned int i,j;

   // save filename
   filename = fn;

   // open file
   ofstream strm;
   strm.open(filename.c_str(), ios::out | ios::binary);
   if(!strm.is_open()) {
      LOG(ERROR) << " Error - failed to open output file " << filename;
      return 92;
   }

   // version number
   writeInt(strm, Version);

   // header
   writeString(strm, Title);
   writeDouble(strm, converge);
   writeInt(strm, nitermax);
   writeString(strm, geoint);
   writeString(strm, geoidfile);
   writeInt(strm, ndof);
   writeInt(strm, nunknown);
   writeInt(strm, ndata);
   writeInt(strm, nmeas);
   writeInt(strm, nconstr);
   writeInt(strm, dim);
   writeString(strm, hashstr);

   // a priori
   i = apPoints.size();
   writeInt(strm,i);
   map<string,apriori_pos>::iterator apit;
   for(apit=apPoints.begin(); apit!=apPoints.end(); ++apit) {
      try {
         writeString(strm,apit->first); 
         strm.write(reinterpret_cast<char*>(&apit->second), sizeof(apriori_pos));
      }
      catch(exception& e) {
         Exception ge(string("Write apriori failed: ")+e.what());
         GNSSTK_THROW(ge);
      }
   }

   // iterations
   i = changes.size();
   writeInt(strm,i);
   for(i=0; i<changes.size(); i++) {
      writeDouble(strm,changes[i]);
      writeDouble(strm,relresid[i]);
      writeInt(strm,status[i]);
   }

   // data results
   i = dataResults.size();
   writeInt(strm,i);
   map<string,data_result>::iterator it;
   for(it=dataResults.begin(); it!=dataResults.end(); ++it) {
      try {
         writeString(strm,it->first); 
         strm.write(reinterpret_cast<char*>(&it->second), sizeof(data_result));
      }
      catch(exception& e) {
         Exception ge(string("Write data_result failed: ")+e.what());
         GNSSTK_THROW(ge);
      }
   }

   // stats
   writeDouble(strm,relRMSresid);
   writeDouble(strm,APV);
   writeDouble(strm,angRMSresid);
   writeDouble(strm,lenRMSresid);
   writeDouble(strm,confidence);
   writeDouble(strm,lowerChiSq);
   writeDouble(strm,upperChiSq);
   
   // XYZ results
   i = posXYZResults.size();
   writeInt(strm,i);
   map<string,posXYZ_result>::iterator jt;
   for(jt=posXYZResults.begin(); jt!=posXYZResults.end(); ++jt) {
      try {
         writeString(strm,jt->first); 
         strm.write(reinterpret_cast<char*>(&jt->second), sizeof(posXYZ_result));
      }
      catch(exception& e) {
         Exception ge(string("Write posXYZ_result failed: ")+e.what());
         GNSSTK_THROW(ge);
      }
   }

   // LLH results
   i = posLLHResults.size();
   writeInt(strm,i);
   map<string,posLLH_result>::iterator kt;
   for(kt=posLLHResults.begin(); kt!=posLLHResults.end(); ++kt) {
      try {
         writeString(strm,kt->first); 
         strm.write(reinterpret_cast<char*>(&kt->second), sizeof(posLLH_result));
      }
      catch(exception& e) {
         Exception ge(string("Write posLLH_result failed: ")+e.what());
         GNSSTK_THROW(ge);
      }
   }

   // Namelist and Covariance
   // TD why does write and/or read of a Matrix object fail?
   try {
      string str;

      i = DataNames.size();
      writeInt(strm,i);
      for(i=0; i<DataNames.size(); i++) {
         str = DataNames.getName(i);
         writeString(strm,str);
      }

      i = StateNames.size();
      writeInt(strm,i);
      for(i=0; i<StateNames.size(); i++) {
         str = StateNames.getName(i);
         writeString(strm,str);
      }

      // Partials matrix
//      i = Partials.rows();
//      writeInt(strm,i);
//      i = Partials.cols();
//      writeInt(strm,i);
//#ifdef SPARSE
//      vector<unsigned int> rows, cols;
//      vector<double> values;
//      Partials.flatten(rows,cols,values);
//      i = rows.size();
//      writeInt(strm,i);
//      for(i=0; i<rows.size(); i++) {
//         writeInt(strm,rows[i]);   
//         writeInt(strm,cols[i]);   
//         writeDouble(strm,values[i]);   
//      }
//#else
//      for(i=0; i<Partials.rows(); i++)
//         for(j=0; j<Partials.cols(); j++)
//            writeDouble(strm,Partials(i,j));
//#endif

      i = Covariance.rows();
      writeInt(strm,i);
      i = Covariance.cols();
      writeInt(strm,i);
      for(i=0; i<Covariance.rows(); i++)
         for(j=i; j<Covariance.cols(); j++)
            writeDouble(strm,Covariance(i,j));

   }




   catch(Exception& e) { GNSSTK_RETHROW(e); }
   catch(exception& e) {
      Exception ge(string("Write Names, Covariance failed: ")+e.what());
      GNSSTK_THROW(ge);
   }

   // close file
   strm.clear();
   strm.close();

   return 0;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::WriteBinaryFile(string& fn) throw(Exception)

// ------------------------------------------------------------------------
// Read the binary file and load the information into this object
// param filename name of binary file to be read
// throw on stream error
int LSABinaryFile::ReadBinaryFile(string& fn) throw(Exception)
{
try {
   unsigned int i,j,n;

   // save filename
   filename = fn;

   // open file
   ifstream strm;
   strm.open(filename.c_str(), ios::in | ios::binary);
   if(!strm.is_open()) {
      LOG(ERROR) << " Error - failed to open input binary file " << filename << ".";
      return 92;
   }
   strm.clear();

   // first read version number and check that it equals current version number
   i = readInt(strm);
   if(i != Version) {
      LOG(ERROR) << " Error - binary file version is obsolete - regenerate binary.";
      return 93;
   }

   Title = readString(strm);
   converge = readDouble(strm);
   nitermax = readInt(strm);
   geoint = readString(strm);
   geoidfile = readString(strm);
   ndof = readInt(strm);
   nunknown = readInt(strm);
   ndata = readInt(strm);
   nmeas = readInt(strm);
   nconstr = readInt(strm);
   dim = readInt(strm);
   hashstr = readString(strm);

   // apriori positions
   n = readInt(strm);
   for(i=0; i<n; i++) {
      try {
         string lab = readString(strm);
         apriori_pos ap;
         strm.read(reinterpret_cast<char*>(&ap), sizeof(apriori_pos));
         apPoints.insert(map<string,apriori_pos>::value_type(lab,ap));
      }
      catch(exception& e) {
         Exception ge(string("Read apriori failed: ")+e.what());
         GNSSTK_THROW(ge);
      }
   }

   // iteration
   n = readInt(strm);
   for(i=0; i<n; i++) {
      changes.push_back(readDouble(strm));
      relresid.push_back(readDouble(strm));
      status.push_back(readInt(strm));
   }
   
   // data results
   n = readInt(strm);
   for(i=0; i<n; i++) {
      try {
         string lab = readString(strm);
         data_result dr;
         strm.read(reinterpret_cast<char*>(&dr), sizeof(data_result));
         dataResults.insert(map<string,data_result>::value_type(lab,dr));
      }
      catch(exception& e) {
         Exception ge(string("Read data_result failed: ")+e.what());
         GNSSTK_THROW(ge);
      }
   }
   
   // stats
   relRMSresid = readDouble(strm);
   APV = readDouble(strm);
   angRMSresid = readDouble(strm);
   lenRMSresid = readDouble(strm);
   confidence = readDouble(strm);
   lowerChiSq = readDouble(strm);
   upperChiSq = readDouble(strm);

   
   // posXYZ results
   n = readInt(strm);
   for(i=0; i<n; i++) {
      try {
         string lab = readString(strm);
         posXYZ_result dr;
         strm.read(reinterpret_cast<char*>(&dr), sizeof(posXYZ_result));
         posXYZResults.insert(
            map<string,posXYZ_result>::value_type(lab,dr));
      }
      catch(exception& e) {
         Exception ge(string("Read posXYZ_result failed: ")+e.what());
         GNSSTK_THROW(ge);
      }
   }
   
   // posLLH results
   n = readInt(strm);
   for(i=0; i<n; i++) {
      try {
         string lab = readString(strm);
         posLLH_result dr;
         strm.read(reinterpret_cast<char*>(&dr), sizeof(posLLH_result));
         posLLHResults.insert(
            map<string,posLLH_result>::value_type(lab,dr));
      }
      catch(exception& e) {
         Exception ge(string("Read posLLH_result failed: ")+e.what());
         GNSSTK_THROW(ge);
      }
   }
   
   // Namelist and Covariance
   // TD why does write and/or read of Matrix object fail?
   try {
      unsigned int ii,jj;
      string str;

      DataNames.clear();
      ii = readInt(strm);
      for(i=0; i<ii; i++) {
         str = readString(strm);
         DataNames += str;
      }

      StateNames.clear();
      ii = readInt(strm);
      for(i=0; i<ii; i++) {
         str = readString(strm);
         StateNames += str;
      }

      // partials matrix
//      ii = readInt(strm);
//      jj = readInt(strm);
//#ifdef SPARSE
//      unsigned int r,c;
//      Partials = SparseMatrix<double>(ii,jj);      // NB NO initializer
//      ii = readInt(strm);           // the number of non-zero elements
//      for(i=0; i<ii; i++) {
//         r = readInt(strm);
//         c = readInt(strm);
//         Partials(r,c) = readDouble(strm);
//      }
//#else
//      Partials = Matrix<double>(ii,jj,0.0);
//      for(i=0; i<ii; i++)
//         for(j=0; j<jj; j++)
//            Partials(i,j) = readDouble(strm);
//#endif

      ii = readInt(strm);
      jj = readInt(strm);
      Covariance = Matrix<double>(ii,jj,0.0);
      for(i=0; i<ii; i++)
         for(j=i; j<jj; j++)
            Covariance(i,j) = Covariance(j,i) = readDouble(strm);


   }
   catch(Exception& e) { GNSSTK_RETHROW(e); }
   catch(exception& e) {
      Exception ge(string("Read Names, Covariance failed: ")+e.what());
      GNSSTK_THROW(ge);
   }

   strm.close();

   return 0;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::ReadBinaryFile(string& fn) throw(Exception)

// ------------------------------------------------------------------------
// Dump the header information
// param os output stream
void LSABinaryFile::DumpHeader(ostream& os) throw(Exception)
{
try {
   os << " Title : \"" << Title << "\"" << endl;
   os << " convergence criterion = " << scientific << setprecision(2) << converge
      << ";  iteration max = " << nitermax
      << ";  geoid interpolator : " << geoint
      << ";  geoid file name : " << geoidfile << endl;
   os << " degrees of freedom = " << ndof
      << ";  N unknowns = " << nunknown
      << ";  N data = " << ndata
      << ";  N meas = " << nmeas
      << ";  N constraints = " << nconstr
      << ";  dimension = " << dim << endl;
   os << " hash string is " << hashstr << endl;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::DumpHeader(ostream& os, const int prec) throw(Exception)

// ------------------------------------------------------------------------
// Dump the initial positions
// param os output stream
// param linprecM int precision for linear measurements (m)
// param linprecP int precision for geocentric positions (m)
// param angprecM int precision for angular measurements (SOA)
// param angprecP int precision for angular positions (SOA)
void LSABinaryFile::DumpInitial(ostream& os, bool doWest, const int linprecM,
                                const int linprecP, const int angprecM, const int angprecP)
   throw(Exception)
{
try {
   os << "\n# Initial positions (meters)" << endl;
   map<string,apriori_pos>::iterator apit;
   string aposStr[4] = {"unknown","estimate","fixed","adjust"};

   // find the size of the largest Point label, for printing
   unsigned int labw(6);
   for(apit=apPoints.begin(); apit!=apPoints.end(); ++apit)
      if(apit->first.size() > labw) labw = apit->first.size();

   // print header
   os << " " << rightJustify("Label",labw)
      << " " << setw(linprecP+10) << "ECEF-X"
      << " " << setw(linprecP+10) << "ECEF-Y"
      << " " << setw(linprecP+10) << "ECEF-Z"
      << " " << setw(angprecP+11) << "Latitude N"
      << " " << setw(angprecP+11) << (doWest ? "Longitude W":"Longitude E")
      << " " << setw(linprecP+6) << "Height"
      << "  fix-type     a-priori" << endl;

   // print all a priori positions
   double lat,lon,ht;
   Position p;
   for(apit=apPoints.begin(); apit!=apPoints.end(); ++apit) {
      p.setECEF(apit->second.X,apit->second.Y,apit->second.Z);
      lat = p.getGeodeticLatitude() * ::DEG_TO_RAD;
      lon = p.getLongitude() * ::DEG_TO_RAD;
      if(doWest) lon = TwoPi - lon;
      ht = p.getHeight();
      os << " " << setw(labw) << apit->first << fixed << setprecision(linprecP)
         << " " << setw(linprecP+10) << apit->second.X
         << " " << setw(linprecP+10) << apit->second.Y
         << " " << setw(linprecP+10) << apit->second.Z
         << " " << setw(angprecP+11) << angleAsDMSstring(lat,angprecP)
         << " " << setw(angprecP+11) << angleAsDMSstring(lon,angprecP)
         << " " << setw(linprecP+6) << fixed << setprecision(linprecP) << ht
         << " " << setw(2) << apit->second.fixtype
         << " = " << setw(8) << aposStr[apit->second.fixtype]
         << " " << setw(8) << (apit->second.computed == 0 ? "given" : "computed")
         << endl;
   }
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::DumpInitial

// ------------------------------------------------------------------------
// Dump the iteration information
// param os output stream
// param prec int precision for residuals
void LSABinaryFile::DumpIterations(ostream& os, const int prec) throw(Exception)
{
try {
   os << "\n Iterations:\n Iter.  RMSadj(m)      delta  RMSrelResid   Status\n";
   string statStr[4] = {"diverged","too many iterations","continue","converged"};
   double oldchange(0);
   for(unsigned int i=0; i<changes.size(); i++) {
      os << " " << setw(5) << i+1
         << scientific << setprecision(2)
         << " " << setw(10) << changes[i]
         << " " << setw(10) << changes[i] - oldchange
         << fixed << setprecision(prec)
         << " " << setw(10) << relresid[i]
         << " " << setw(5) << status[i] << " = " << statStr[status[i]+2] << endl;
      oldchange = changes[i];
   }
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::DumpIterations

// ------------------------------------------------------------------------
// Dump the data and residuals
// param os output stream
// param linprec int precision for linear (m)
// param angprecM int precision for angular measurements (SOA)
// param angprecP int precision for angular positions (SOA)
void LSABinaryFile::DumpData(ostream& os, const int linprec,
                                          const int angprecM, const int angprecP)
   throw(Exception)
{
try {
   os << "\n Data results:" << endl;
   map<string,data_result>::iterator it;
   string label, label2;

   // get the length of the longest label
   unsigned int j,labw(0),datw(angprecM+11);
   for(it=dataResults.begin(); it!=dataResults.end(); ++it) {
      label = it->first;
      j = label.size();
      if(label.substr(0,3) == "Del") {    // NB must keep the {}
         if(j+1 > labw) labw=j+1;
      }
      else if(j > labw) labw=j;
   }

   os << " " << leftJustify("Label",labw+1)
      << setw(datw) << "Meas(m|soa)"
      << " " << setw(datw) << "Nomin(m|soa)"
      << " " << setw(datw) << "Pre-R(m|soa)"
      << " " << setw(datw) << "Post-R(m|soa)"
      << " " << setw(datw-3) << "Rel-Resid"
      << " " << setw(7) << "Redund"
      << " " << setw(9) << "Std-Resid" << endl;

   for(it=dataResults.begin(); it!=dataResults.end(); ++it) {
      label = label2 = it->first;
      label2 = leftJustify(label2,labw);

      // units on this line
      bool isLin(true);
      int prec(linprec);
      if(label.substr(0,3) == "Han" || label.substr(0,3) == "Azm" ||
         label.substr(0,3) == "Zan" || label.substr(0,3) == "Van") {
         isLin = false;
         prec = angprecM;        // SOA not RAD
      }
      else if(label.substr(0,3) == "Del" || label.substr(0,4) == "Dist")
         isLin = true;

      // write the row
      const data_result& dr(it->second);

      // label, measured and nominal value
      os << fixed << setprecision(prec)
         << " " << setw(labw) << label2;
      if(isLin) os
         << " " << setw(datw) << dr.meas
         << " " << setw(datw) << dr.nomin;
      else os
         << " " << setw(datw) << angleAsDMSstring(dr.meas,angprecM)
         << " " << setw(datw) << angleAsDMSstring(dr.nomin,angprecM);

      // residuals
      os << " " << setw(datw)
         << (dr.meas - dr.nomin) * (isLin ? 1 : ::RAD_TO_SOA)
         << " " << setw(datw) << dr.rawresid * (isLin ? 1 : ::RAD_TO_SOA)
         << " " << setw(datw-3) << dr.resid;

      // redundancy and std residual
      os << " " << setw(7) << setprecision(3) << dr.redundancy;

      // std residual and blunder detection
      double resid = dr.stdresid;
      if(resid == 0.0)
         os << " " << setw(9) << "--";
      else {
         // flag possible blunders
         os << " " << setw(9) << resid;
         if(dr.PossibleOutlier) os << " **";
      }

      os << endl;
   }
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::DumpData

// ------------------------------------------------------------------------
void LSABinaryFile::DumpFinalStats(ostream& os, const int linprec,
                            const int angprecM, const int angprecP) throw(Exception)
{
try {
   os << "\n Final Statistics and Chi squared test" << endl;

   os << " RMS post-fit relative residual = "
      << scientific << setprecision(linprec) << relRMSresid << endl;

   os << " Degrees of freedom = " << ndof
      << fixed << setprecision(linprec)
      << "; Std dev of unit weight = " << ::sqrt(APV)
      << "; APV = " << APV << endl;

   if(angRMSresid > 0.0)
      os << " RMS post-fit raw residual (angles) = "
         << scientific << setprecision(angprecM) << angRMSresid
            << " rad = " << angRMSresid*RAD_TO_SOA << " soa." << endl;

   if(lenRMSresid > 0.0)
      os << " RMS post-fit raw residual (length) = "
         << scientific << setprecision(linprec) << lenRMSresid << " m." << endl;

   os << " Upper Chi-squared test (" << fixed << setprecision(3)
      << 1.0-confidence << "): " << fixed << setprecision(2)
      << APV << " < " << (ndof > 0 ? upperChiSq/ndof : 0)
      << (APV < (ndof > 0 ? upperChiSq/ndof : 0) ? " pass":" fail") << endl;
   os << " Lower Chi-squared test (" << fixed << setprecision(3)
      << confidence << "): " << fixed << setprecision(2)
      << (ndof > 0 ? lowerChiSq/ndof : 0) << " < " << APV
      << (APV > (ndof > 0 ? lowerChiSq/ndof : 0) ? " pass":" fail") << endl;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

// ------------------------------------------------------------------------
// Dump the final positions in XYZ
// param os output stream
// param noAPV if true, do not scale with APV
// param linprecP int precision for geocentric positions (m)
// param angprecM int precision for angular measurements (SOA)
// param angprecP int precision for angular positions (SOA)
void LSABinaryFile::DumpFinalXYZ(ostream& os, bool noAPV,
      const int linprecP, const int angprecM, const int angprecP) throw(Exception)
{
try {
   // Pos XYZ results - NB don't print computed flag
   os << "\n Final Adjusted Positions XYZ" << endl;
   unsigned int labw(8);                        // get label width for points
   string label,label2;

   // scale with APV
   double scaleV(noAPV ? 1.0 : APV);
   double scaleS(::sqrt(scaleV));

   // get label width
   map<string,posXYZ_result>::iterator jt;
   for(jt=posXYZResults.begin(); jt!=posXYZResults.end(); ++jt)
      if(jt->first.size() > labw) labw = jt->first.size();

   os << " " << leftJustify("Position",labw)
      << rightJustify("X(m)",linprecP+11)
      << rightJustify("Y(m)",linprecP+11)
      << rightJustify("Z(m)",linprecP+11)
      << "     "
      << rightJustify("Xadj(m)",linprecP+5)
      << rightJustify("Yadj(m)",linprecP+5)
      << rightJustify("Zadj(m)",linprecP+5)
      << "  "
      << rightJustify("Xsig(m)",linprecP+4)
      << rightJustify("Ysig(m)",linprecP+4)
      << rightJustify("Zsig(m)",linprecP+4)
      << "   "
      << center("Covariance (XX XY XZ YY YZ ZZ) m*m",54)
      << endl;

   for(jt=posXYZResults.begin(); jt!=posXYZResults.end(); ++jt) {
      label = label2 = jt->first;
      const posXYZ_result& pr(jt->second);

      // NB *Justify mod.s its string arg (label, in this case)
      os << " " << leftJustify(label2,labw)
         << fixed << setprecision(linprecP)
         << " " << setw(linprecP+10) << pr.X
         << " " << setw(linprecP+10) << pr.Y;
      if(dim == 2) os << " " << setw(linprecP+10) << '0';
      else os << " " << setw(linprecP+10) << pr.Z;

      // is it fixed?
      if(apPoints.find(label) != apPoints.end() && apPoints[label].fixtype == 2) {
         os << "     FIXED" << endl;
         continue;
      }

      os << "     "                               // adjustments
         << " " << setw(linprecP+4) << pr.adjX
         << " " << setw(linprecP+4) << pr.adjY;
      if(dim == 2) os
         << " " << setw(linprecP+4) << '0';
      else os
         << " " << setw(linprecP+4) << pr.adjZ;

      os << "  "                               // sigmas
         << " " << setw(linprecP+3) << pr.sigX*scaleS
         << " " << setw(linprecP+3) << pr.sigY*scaleS;
      if(dim == 2) os
         << " " << setw(linprecP+3) << '0';
      else os
         << " " << setw(linprecP+3) << pr.sigZ*scaleS;

      os << scientific << setprecision(2)         // covariance
         << "   " << setw(9) << pr.Cxx*scaleV
         << " " << setw(9) << pr.Cxy*scaleV;
      if(dim == 2) os
         << " " << setw(9) << '0'
         << " " << setw(9) << pr.Cyy*scaleV
         << " " << setw(9) << '0'
         << " " << setw(9) << '0';
      else os
         << " " << setw(9) << pr.Cxz*scaleV
         << " " << setw(9) << pr.Cyy*scaleV
         << " " << setw(9) << pr.Cyz*scaleV
         << " " << setw(9) << pr.Czz*scaleV;
      os << endl;
   }
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::DumpFinalXYZ

// ------------------------------------------------------------------------
// Dump the positions in LLH
// param os output stream
// param noAPV if true, do not scale with APV
// param linprec int precision for linear (m)
// param angprecM int precision for angular measurements (SOA)
// param angprecP int precision for angular positions (SOA)
void LSABinaryFile::DumpFinalLLH(ostream& os, bool doWest, bool noAPV,
      const int linprec, const int angprecM, const int angprecP)
   throw(Exception)
{
try {
   // Pos LLH results - NB don't print computed flag
   os << "\n Final Adjusted Positions LLH" << endl;
   unsigned int labw(8);                        // get label width for points
   string label,label2;

   // scale with APV
   double scaleV(noAPV ? 1.0 : APV);
   double scaleS(::sqrt(scaleV));

   // get label width
   map<string,posLLH_result>::iterator kt;
   for(kt=posLLHResults.begin(); kt!=posLLHResults.end(); ++kt)
      if(kt->first.size() > labw) labw = kt->first.size();

   int lwid(4+linprec > 9 ? 4+linprec : 9);
   int awid(4+angprecM > 10 ? 4+angprecM : 10);
   os << " " << leftJustify("Position",labw)
              << rightJustify("Latitude N",angprecP+11)
              << rightJustify((doWest ? "Longitude W":"Longitude E"),angprecP+12)
              << rightJustify("Height(m)",lwid+1)
              << "  "
              << rightJustify("Nadj(m)",lwid+1)
              << rightJustify("Eadj(m)",lwid+1)
              << rightJustify("Vadj(m)",lwid+1)
              << "  "
              << rightJustify("Nsig(m)",10)
              << rightJustify("Esig(m)",10)
              << rightJustify("Vsig(m)",10)
              << "   "
              << center("Covariance (NN NE NU EE EU UU) m*m",54)
              << endl;

   for(kt=posLLHResults.begin(); kt!=posLLHResults.end(); ++kt) {
      label = label2 = kt->first;
      const posLLH_result& pr(kt->second);

      os << " " << leftJustify(label2,labw)
         << " " << setw(angprecP+10) << angleAsDMSstring(pr.lat,angprecP)
         << " " << setw(angprecP+11)
            << angleAsDMSstring(doWest ? TwoPi-pr.lon : pr.lon,angprecP)        
         << " " << setw(lwid) << fixed << setprecision(linprec) << pr.ht;

      // is it fixed?
      if(apPoints.find(label) != apPoints.end() && apPoints[label].fixtype == 2) {
         os << "     FIXED" << endl;
         continue;
      }

      os << "  "
         << " " << setw(lwid) << pr.adjN             // adjusts
         << " " << setw(lwid) << pr.adjE
         << " " << setw(lwid) << pr.adjU
         << scientific << setprecision(2)
         << "  " 
         << " " << setw(9) << pr.sigN*scaleS      // sigma
         << " " << setw(9) << pr.sigE*scaleS
         << " " << setw(9) << pr.sigU*scaleS
         << scientific << setprecision(2)         // covariance
         << "   " << setw(9) << pr.Cnn*scaleV
         << " " << setw(9) << pr.Cne*scaleV;
      if(dim == 2) os
         << " " << setw(9) << "0"
         << " " << setw(9) << pr.Cee*scaleV
         << " " << setw(9) << "0"
         << " " << setw(9) << "0";
      else os
         << " " << setw(9) << pr.Cnu*scaleV
         << " " << setw(9) << pr.Cee*scaleV
         << " " << setw(9) << pr.Ceu*scaleV
         << " " << setw(9) << pr.Cuu*scaleV;
      
      os << endl;
   }
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::DumpFinalLLH

// ------------------------------------------------------------------------
// Dump the covariance matrix
// param os output stream
// param noAPV if true, do not scale with APV
// param linprec int precision for linear (m)
// param angprecM int precision for angular measurements (SOA)
// param angprecP int precision for angular positions (SOA)
void LSABinaryFile::DumpCovariance(ostream& os, bool noAPV,
      const int linprec, const int angprecM, const int angprecP) throw(Exception)
{
try {
   // solution covariance matrix
   // edit the DataNames to remove "(labels...)"
   string label,label2;
   unsigned int labw(8);                        // get label width for points

   // get label width
   map<string,posXYZ_result>::iterator jt;
   for(jt=posXYZResults.begin(); jt!=posXYZResults.end(); ++jt)
      if(jt->first.size() > labw) labw = jt->first.size();

   Namelist dn;
   for(unsigned int i=0; i<DataNames.size(); i++) {
      string str = DataNames.getName(i);
      string::size_type pos = str.find_last_of(")",str.size()-1);
      label2 = str.substr(pos+1);
      pos = str.find_first_of("(",0);
      label = str.substr(0,pos);
      dn += label + label2;
   }

   // partials matrix
//#ifdef SPARSE
//   Matrix<double> P(Partials);
//   LabeledMatrix LP(dn,StateNames,P);  // TD why does implicit cast not work?
//#else
//   LabeledMatrix LP(dn,StateNames,Partials);
//#endif
//   LP.message("Partials:");
//   LP.fixed().setprecision(5).setw(10).clean(true);
//   os << endl << LP << endl;

   Matrix<double> Cov(Covariance);
   if(!noAPV) Cov *= APV;
   // NB windows does not like LC(StateNames,APV*Covariance); !?
   LabeledMatrix LC(StateNames,Cov);
   LC.message("Covariance:");
   LC.scientific().setprecision(2).setw(labw+1);
   LC.symmetric(true);
   os << endl << LC << endl;
   //os << "\nCovariance:\n" << cleanMatrixString(Covariance,2,9,true,true) << endl;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::DumpCovariance

// ------------------------------------------------------------------------
// Dump the contents of this object
// param os output stream
// param doWest if true, output west longitude
// param noAPV if true, do not scale with APV
// param linprecM int precision for linear measurements (m)
// param linprecP int precision for geocentric positions (m)
// param angprecM int precision for angular measurements (SOA)
// param angprecP int precision for angular positions (SOA)
void LSABinaryFile::Dump(ostream& os, bool doWest, bool noAPV,
                  const int linprecM, const int linprecP,
                  const int angprecM, const int angprecP)
   throw(Exception)
{
try {
   // -----------------------------------------------
   os << "\n# Dump of binary file " << filename
      << " : binary file version " << Version
      << " (uncertainties " << (noAPV ? "not ":"") << "scaled with APV)"
      << endl;
   DumpHeader(os);

   // -----------------------------------------------
   DumpInitial(os,doWest,linprecM,linprecP,angprecM,angprecP);

   if(changes.size() == 0) {
      os << "\n# No further data - quit after a priori.";
      os << "\n# End of Dump of binary file\n";
      return;
   }

   // -----------------------------------------------
   DumpIterations(os,linprecM);

   // -----------------------------------------------
   DumpData(os,linprecM,angprecM,angprecP);

   // -----------------------------------------------
   DumpFinalStats(os,linprecM,angprecM,angprecP);

   // -----------------------------------------------
   DumpFinalXYZ(os,noAPV,linprecP,angprecM,angprecP);

   // -----------------------------------------------
   DumpFinalLLH(os,noAPV,doWest,linprecP,angprecM,angprecP);

   // -----------------------------------------------
   DumpCovariance(os,noAPV,linprecM,angprecM,angprecP);

   // -----------------------------------------------
   os << "\n# End of Dump of binary file\n";

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::Dump(ostream& os)

// ------------------------------------------------------------------------
// Dump the points with confidence ellipse information
// param os output stream
// param doWest if true, output west longitude
// param noAPV if true, do not scale with APV
// param linprec int precision for linear (m)
// param angprec int precision for angular positions (SOA)
// param conf string confidence level for ellipse (one of 1sig, 90, 95)
void LSABinaryFile::DumpPoints(ostream& os, bool doWest, bool noAPV,
                               const int linprec, const int angprec, string conf)
   throw(Exception)
{
try {
   unsigned int i,j,labw(8);
   string label,label2;
   map<string,posLLH_result>::iterator kt;

   // get label width for Points
   for(kt=posLLHResults.begin(); kt!=posLLHResults.end(); ++kt)
      if(kt->first.size() > labw) labw = kt->first.size();

   // header
   os << "\n Final Positions with Confidence Ellipse (m) "
         << "(confidence level " << (conf == "1sig" ? "68" : conf) << "%)" << endl;
   //Point  Lat DMS  Lon DMS  EllipHt  OrthoHt 3D maj 2D maj Azm  2D min Vertical 
   os << " " << leftJustify("Position",labw)
             << rightJustify("Latitude N",11+angprec)
             << rightJustify((doWest ? "Longitude W" : "Longitude E"),11+angprec)
             << rightJustify("EHt(m)",6+linprec)
             << rightJustify("OHt(m)",6+linprec)
             //<< rightJustify("3Dmaj",9)
             << rightJustify("2Dmaj",8)
             << rightJustify("Azm",5)
             << rightJustify("2Dmin",8)
             << rightJustify("Vert",8)
             << endl;

   // get scaling for confidence level
   double scale_1d(getConfidenceFactor(conf,1,ndof));
   double scale_2d(getConfidenceFactor(conf,2,ndof));
   //double scale_3d(confidenceFactor(conf,3,ndof));

   for(kt=posLLHResults.begin(); kt!=posLLHResults.end(); ++kt) {
      label = label2 = kt->first;
      const posLLH_result& pr(kt->second);

      os << " " << leftJustify(label,labw)
         << " " << setw(10+angprec) << angleAsDMSstring(pr.lat,angprec)
         << " " << setw(10+angprec)
         << angleAsDMSstring((doWest ? TwoPi-pr.lon : pr.lon),angprec)
         << fixed << setprecision(linprec)
         << " " << setw(5+linprec) << pr.ht
         << " " << setw(5+linprec) << pr.ht - pr.und;

      // compute confidence ellipse
      Matrix<double> Cov(3,3);
      Cov(0,0) = pr.Cnn;
      Cov(0,1) = pr.Cne;
      Cov(0,2) = pr.Cnu;
      Cov(1,0) = pr.Cne;
      Cov(1,1) = pr.Cee;
      Cov(1,2) = pr.Ceu;
      Cov(2,0) = pr.Cnu;
      Cov(2,1) = pr.Ceu;
      Cov(2,2) = pr.Cuu;

      // scale with APV
      if(!noAPV) Cov *= APV;

      double maj3,min3,maj2,min2,vert,azm;
      getEllipse(Cov,maj3,min3,maj2,min2,vert,azm);

      if(fabs(maj2)<1e-9 && fabs(min2)<1e-9 && fabs(vert)<1e-9)//fixed point -- Fix to Bug #1036
      {
          os << endl;
      }
      else
      {
          os << setprecision(4)
             //<< " " << setw(8) << maj3*scale
             << " " << setw(7) << maj2*scale_2d
             << " " << setw(4) << int(azm+0.5)
             << " " << setw(7) << min2*scale_2d
             << " " << setw(7) << vert*scale_1d;
          os << endl;
      }
   }

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::DumpPoints

// ------------------------------------------------------------------------
// Write the contents of the CSV file
// param os output stream
// param linprec int precision for linear (m)
// param angprec int precision for angular positions (SOA)
void LSABinaryFile::DumpCSV(ostream& os, const int linprec, const int angprecP)
      throw(Exception)
{
try {
   double lon, lat, decdegLon;
   string str;
   string latDir("N");
   string lonDir("E");
   vector<string> fields;

   // print header
   os << "Station,LatHem,Lat_deg,Lat_min,Lat_sec,Lat_decdeg,LonHem,Lon_deg"
            << ",Lon_min,Lon_sec,Lon_decdeg"
            << ",EllipHt_m,OrthHt_m,sigmaN_m,sigmaE_m,sigmaU_m" << endl;

   // loop over points
   map<string,posLLH_result>::iterator kt;
   for(kt=posLLHResults.begin(); kt!=posLLHResults.end(); ++kt) {
      const posLLH_result& pr(kt->second);

      os << kt->first << fixed;                          // Point label

      if(pr.lat < 0.0)
      {
          lat = -pr.lat;
          latDir = string("S");
      }
      else
      {
          lat = pr.lat;
          latDir = string("N");
      }


      str = angleAsDMSstring(lat,angprecP);           // Lat D M S
      fields = splitWithDoubleQuotes(str,' ');
      os << "," << latDir
         << "," << fields[0]
         << "," << fields[1]
         << "," << fields[2]
         << "," << setprecision(9) << pr.lat * ::RAD_TO_DEG;

      //restrict DMS lon to 0-180 E/W
      decdegLon = pr.lon;
      if(pr.lon > Pi)
      {
          lon = TwoPi - pr.lon;
          lonDir = string("W");
          //restrict decdeg lon to +/-180 E
          decdegLon = pr.lon - TwoPi;
      }
      else if(pr.lon < 0 && pr.lon > -Pi)
      {
          lon = -pr.lon;
          lonDir = string("W");
      }
      else if(pr.lon < -Pi)
      {
          lon = TwoPi + pr.lon;
          lonDir = string("E");
          decdegLon = TwoPi + pr.lon;
      }
      else
      {
          lon = pr.lon;
          lonDir = string("E");
      }

      str = angleAsDMSstring(lon,angprecP);              // Lon D M S
      fields = splitWithDoubleQuotes(str,' ');
      os << "," << lonDir
         << "," << fields[0]
         << "," << fields[1]
         << "," << fields[2]
         << "," << setprecision(9) << decdegLon * ::RAD_TO_DEG
         << "," << setprecision(linprec) << pr.ht;

      // TD need a flag to indicate geoid was used and pr.und is valid
      if(pr.und == 0.0)
         os << ",?";
      else
         os << "," << pr.ht - pr.und;                    // OrthoHt_m

      //Fix to Bug #922 --> Always scale CSV file by APV
      os << "," << ::sqrt(APV) * pr.sigN // sigma LLH
         << "," << ::sqrt(APV) * pr.sigE
         << "," << ::sqrt(APV) * pr.sigU << endl;

   }  // end loop over points

   // if there are no points (solution), then dump the apriori positions
   // this is a really stupid kludge
   if(posLLHResults.size() == 0) {
      double plat,plon,ht;
      Position p;
      map<string,apriori_pos>::iterator jt;
      for(jt=apPoints.begin(); jt!=apPoints.end(); ++jt) {
         p.setECEF(jt->second.X,jt->second.Y,jt->second.Z);
         lat = p.getGeodeticLatitude() * ::DEG_TO_RAD;
         lon = p.getLongitude() * ::DEG_TO_RAD;
         ht = p.getHeight();

         os << jt->first << fixed;                          // Point label

         if(lat < 0.0)
         {
             plat = -lat;
             latDir = string("S");
         }
         else
         {
             plat = lat;
             latDir = string("N");
         }
         str = angleAsDMSstring(plat,angprecP);              // Lat D M S
         fields = splitWithDoubleQuotes(str,' ');
         os << "," << latDir
            << "," << fields[0]
            << "," << fields[1]
            << "," << fields[2]
            << "," << setprecision(9) << lat * ::RAD_TO_DEG;

         //restrict DMS lon to 0-180 E/W
         decdegLon = lon;
         if(lon > Pi)
         {
             plon = TwoPi - lon;
             lonDir = string("W");
             //restrict decdeg lon to +/-180 E
             decdegLon = lon - TwoPi;
         }
         else if(lon < 0 && lon > -Pi)
         {
             plon = -lon;
             lonDir = string("W");
         }
         else if(lon < -Pi)
         {
             plon = TwoPi + lon;
             lonDir = string("E");
             decdegLon = TwoPi + lon;
         }
         else
         {
             plon = lon;
             lonDir = string("E");
         }

         str = angleAsDMSstring(plon,angprecP);              // Lon D M S
         fields = splitWithDoubleQuotes(str,' ');
         os << "," << lonDir
            << "," << fields[0]
            << "," << fields[1]
            << "," << fields[2]
            << "," << setprecision(9) << decdegLon * ::RAD_TO_DEG
            << "," << setprecision(linprec) << ht;

         os << "," << 0.0;                                  // OrthoHt_m

         os << "," << 0.0                                   // sigma LLH
            << "," << 0.0       
            << "," << 0.0 << endl;
      }
   }
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}  // end void LSABinaryFile::DumpCSV

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
