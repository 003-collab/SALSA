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
/// @file lsapriori.cpp  Compute a priori positions for unknown Points for LSA solver.
/// The ComputeAPriori() function is the single entry point here; this function
/// attempts to apply each of several algorithms, in a fixed order, to find a priori
/// values (positions) for unknown Points. Unknown Points are found by searching the
/// GD.Points map. The attempts are iterated until either no more unknown Points
/// remain (return 0 = success) or no more can be found (return failure). If the
/// user has requested a 'quit' after ComputeAPriori(), this routine returns non-zero.
///
/// The algorithms are as follows. Generally each consists of two calls, first to
/// xxxAlgorithm(label, responses), to find combinations of known Points and
/// Measurements to which the algorithm can be applied, and then a call within the
/// xxxAlgorithm() function to Implementyyy() which actually computes the position.
/// The computed position (as Point) is added to the 'responses' vector and the
/// xxxAlgorithm() call returns the number of responses added. After all the
/// algorithms are attempted, the responses are averaged to yield a final position
/// estimate, and the corresponding Point in GD.Points is marked at known. This is
/// iterated until no more Points are changed.
///\verbatim
///   Here are the algorithms; note that DAT record tags are used - the meaning
/// should be clear. Most of the algorithm are horizontal only, meaning they begin
/// by rotating into the local NEU frame, usually using the first known POS, and
/// using only North and East components. HAN and AZM are defined only for the
/// horizontal, and they are signed, which is very useful.
///
/// List of algorithms. POS U is the unknown Point, all other POS are known.
/// 
///   1. Delta addition. Given a POS A and DEL AU, simply add the delta to get POS U.
///      There is no Implement...() routine, since this is trivial.
///
///   2. AZMVectorAdd. Given POS A, DIS AU and AZM AU, simply apply trigonometry and
///      vector addition to get U (ImplementAZMVectorAdd).
///             N           U
///             ^          /
///             |         /                U(N) = A(N) + AU*cos(AZM(AU))
///             |        /                 U(E) = A(E) + AU*sin(AZM(AU))
///             |       / DIS AU
///             |AZM AU/
///             |     /
///             |    /
///             |   /
///             |  /
///             | /
///             |/
///             A----------------> E
///
///   3. HANVectorAdd. Given POS A,B DIS AU and HAN BAU, simply apply trigonometry and
///      vector addition to get U (ImplementHANVectorAdd).
///             N           U
///             ^          /
///             |         /           Note that a = HAN BAU but must worry about sign
///             |        /                   U(N) = A(N) + AU*sin(a)
///             |       /                    U(E) = A(E) + AU*cos(a)
///             |   AU /
///             |     /
///             |    /
///             |   /
///             |  /
///             | /
///             |/ a
///             A---------------------- B --> E
///
///   4. Triangulation. Given POS A,B and HAN ABU,UAB, this forms a triangle with two
///      known angles and a known side between them. The third angle is
///      c = PI/2-a-b, and the law of sines will yield the other two sides
///      (ImplementLawSines()). See NGA25
///             N           U
///             ^          /c\             a+b+c=PI
///             |         /   \            Note that a = |HAN UAB| but
///             |        /     \                     b = |(2PI - HAN ABU)|
///             |       /       \          (interior angles of a triangle always < Pi)
///             |      /         \
///             |     /           \
///             |    /             \
///             |   /               \
///             |  /                 \
///             | /                   \
///             |/a                   b\
///             A---------------------- B --> E
///
///   5. TwoAzimuthLines. Given POS A,B and AZM AU,BU, two lines are defined that
///      must meet at a point = U; call ImplementTwoAzimuths().
///      There are two different ways to collect AZM data (AZM AU for known POS A):
///      a) AZM AU is in the measurements
///      b) HAN UAC and POS C are known, then HAN(UAC)=AZM(AC)-AZM(AU)
///           and AC can be computed
///               N           U
///               ^          / \
///               |         /   \
///               |        /     \2pi-azBU|
///               |       /       \       |
///               |      /         \      |
///               |     /           \     |
///               |azAU/             \    |
///               |   /               \   |
///               |  /                 \  |
///               | /                   \ |
///               |/a                   b\|   AZM AB is ~Pi/2 here
///               A---------------------- B --> E
///   
///   6. Resection. Given known POS A,B,C and HANs BUA,CUB, solve for U.
///      This is the 3-point Resection Problem. See NGA33.
///      Must handle the case (semi-singular) where one of the angles is zero;
///         the problem reduces to finding the intersection of a circle and a line
///         (e.g. ADB in the diagram, when HAN(AUD)=0.)
///      Ref Ghilani Section 15.5, and
///          Ligas, M., "Simple Solution to the Three Point Resection Problem,"
///             Journal of Surveying Engineering, Vol. 139, No. 3, August 2013.
///
///                           U-----------------------A---------------D
///                          / \ HAN(AUB)            /
///                         /   \                   /
///                        /     \                 /
///                       /       \               /
///                      / HAN(BUC)\             /
///                     /           \           /
///                    /             \         /
///                   /               \       /
///                  /                 \     /
///                 /                   \   /
///                /                     \ /
///               C-----------------------B
///
///   7. Ranging. This is similar to the GNSS pseudorange-only solution, and its
///      implementation is based on the Bancroft algebraic solution of GPS ranging.
///      Given 2+ points and the distances (ranges) from these points to the unknown,
///      a least squares solution can be formed yielding the position of the unknown.
///      This is a 3-D algorithm; however in this application the positions are very
///      nearly in a plane. To avoid the near singularity in the height, the data is
///      transformed to the N-E plane before applying a 2-D algorithm. In every case
///      the algorithm yields two different solutions, but internal consistency
///      allows it to pick the "correct" one, with one exception.
///      In the case of only 2 positions and distances, the two solutions are
///      identical, and some other, independent, datum must be used to pick one.
///
///   8. CenterOfMass. This is the default fall-back; if nothing else can determine U,
///      then compute the average of all the known positions (the centroid of
///      the network) and use that. This may or may not work, depending on the problem
///      but it probably always requires a large number of iterations.
///
///   These identities are used throughout the algorithms
///   HAN and AZM are SIGNED in the sense that they are positive in the clock-wise
///   direction. Note that both AZ and HAN are signed <=> order of ABU etc matters.
///   Remember HAN(ABC) is HAN(From-At-To), and AZM(AB) is AZM(From-To).
///   HAN(UAB) = AZM(AB)-AZM(AU)       // definition HAN = difference of 2 AZM
///   AZM(AB) = AZM(BA) + Pi           // mod 2Pi of course
///   HAN(UAB) + HAN(BAU) = TwoPi      // HAN is signed! - order of UAB,BAU matters
///   HAN(AUC) = HAN(AUB) + HAN(BUC)   // from definition of HAN
///
///   Notes
///   In the Law of Sines (and other) algorithms, numerical error in the solution
///   will increase as any of the angles approaches PI or 0, eventually failing
///   (1/sin~0 and 1/tan~0 in others). These angles are tested using |angle-Pi| and
///   |angle| < small limit; however these limits are rather arbitrary. The TwoAzimuth
///   algorithm seems to yield very poor results when the angle at the unknown
///   approaches even 20 degrees.
///
///\endverbatim
///

#include <iostream>
#include <iterator>
#include <utility>
#include <algorithm>

#include "lsasolver.hpp"
#include "LSAConstants.hpp"

//------------------------------------------------------------------------------------
// TODO

//------------------------------------------------------------------------------------
using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
/// Simple addition of the Delta from known to unknown Points yields the unknown.
/// @param lab  string label of the unknown Point
/// @param resp vector of Point; on output append solution to this vector.
/// @return the number of responses added
/// @throw on std exception
int DeltaAddAlgorithm(const string lab, vector<Point>& resp) throw(Exception);

/// Have POS A,B and HAN ABU and UAB - triangulation
/// @param lab  string label of the unknown Point
/// @param resp vector of Point; on output append solution to this vector.
/// @param is2D if true, problem is 2 dimensional
/// @return the number of responses added
/// @throw on std exception
int TriangulationAlgorithm(const string lab, vector<Point>& resp, bool is2D)
   throw(Exception);

/// Have POS A, DIS AU and AZM AU - vector addition with trig
/// @param lab  string label of the unknown Point
/// @param resp vector of Point; on output append solution to this vector.
/// @return the number of responses added
/// @throw on std exception
int AZMVectorAddAlgorithm(const string lab, vector<Point>& resp) throw(Exception);

/// Have HAN ABU, POS A,B and DIS AU - convert HAN to AZM AU and call 3.
/// @param lab  string label of the unknown Point
/// @param resp vector of Point; on output append solution to this vector.
/// @return the number of responses added
/// @throw on std exception
int HANVectorAddAlgorithm(const string lab, vector<Point>& resp) throw(Exception);

/// Find all the azimuths which connect the unknown Point U to another, known Point.
/// For each pair of azimuths, call ImplementTwoAzimuths() to get an estimate of U.
/// @param lab  string label of the unknown Point
/// @param resp vector of Point; on output append solution to this vector.
/// @return the number of responses added
/// @throw on std exception
int TwoAzimuthLinesAlgorithm(const string lab, vector<Point>& resp) throw(Exception);

/// U is At. see NGA33.dat
/// Have Pos A,B and HAN AUB
/// Solve the (NL?) LS problem for U horizontal
/// @param lab  string label of the unknown Point
/// @param resp vector of Point; on output append solution to this vector.
/// @return the number of responses added
/// @throw on std exception
int ResectionAlgorithm(const string lab, vector<Point>& resp) throw(Exception);

/// Have POS A,B,.. and DIS AU,BU,... - ranging
/// 2-ranging will require another datum to break the ambiguity
/// @param lab  string label of the unknown Point
/// @param resp vector of Point; on output append solution to this vector.
/// @return the number of responses added
/// @throw on std exception
int RangingAlgorithm(const string lab, vector<Point>& resp) throw(Exception);

/// Default algorithm, to be used when all else fails.
/// Compute center of mass using all known Points.
/// @param lab  string label of the unknown Point
/// @param resp vector of Point; on output append solution to this vector.
/// @return the number of responses added (1)
/// @throw on std exception
int DefaultAlgorithm(const string lab, vector<Point>& resp) throw(Exception);

/// Combine all the responses by averaging to get a final position. Set the Point in
/// GD.Points for label lab to this value, and set its type to "Estimate".
/// @param lab  string label of the unknown Point
/// @param resp vector of Point; on output append solution to this vector.
/// @throw if unknown labels get confused.
void EstimateUnknownPoint(const string lab, vector<Point>& resp) throw(Exception);

/// Implementation of the Azimuth vector addition algorithm; use trig and vector
/// addition to go from POS A through AZM AU and DIS AU to get U.
/// @param A Point that is known
/// @param azAU azimuth of AU, signed, in radians
/// @param AU distance AU
/// @return the solution as an unknown Point
/// @throw on std exception
Point ImplementAZMVectorAdd(const Point& A, const double azAU, const double AU)
   throw(Exception);

/// Implementation of the HAN vector algorithm; simple application of trig and vector
/// addition.
/// @param A Point that is known
/// @param B Point that is known
/// @param hanUAB triangle interior angle (NOT HAN) in radians
/// @param AU distance AU
/// @return the solution as an unknown Point
/// @throw on std exception
Point ImplementHANVectorAdd(const Point& A, const Point& B,
                            const double hanUAB, const double AU)
   throw(Exception);

/// Implementation of the law of sines.
/// @param A Point that is known
/// @param B Point that is known
/// @param hanUAB triangle interior angle (NOT HAN) in radians
/// @param hanUBA triangle interior angle (NOT HAN) in radians
/// @param is2D  true if the problem is 2D
/// @return the solution as an unknown Point
/// @throw on std exception
Point ImplementLawSines(const Point& A, const Point& B,
                        double hanUAB, double hanUBA, bool is2D=false)
   throw(Exception);

/// Implementation of the two azimuth lines intersecting algorithm.
/// Given POS A,B and AZM AU,BU, two lines are defined that must meet at the point U.
/// @param A Point that is known
/// @param B Point that is known
/// @param azAU azimuth AU in radians
/// @param azBU azimuth BU in radians
/// @return the solution as an unknown Point
/// @throw on std exception
Point ImplementTwoAzimuths(const Point& A, const Point& B, double azAU, double azBU)
   throw(Exception);

/// Solve the 3-point resection problem, including the case when one of the angles
/// is zero. See the discussion and references in the file description.
/// Note that BOTH angles zero is a singular problem.
/// @param EN Reference to Matrix<double> of dimension 4,2 containing
/// (columns) East,North components of points (rows) A, B, C, and U;
/// on output the U row (3) contains the solution.
/// @param hanAUB horizontal angle A-U-B in radians, 0 <= hanAUB < Pi
/// @param hanBUC horizontal angle B-U-C in radians, 0 <= hanBUC < Pi
/// @return true if the algorithm succeeded.
/// @throw on std exception or if singular (should never happen)
bool ImplementResection(Matrix<double>& EN, const double hanAUB, const double hanBUC)
   throw(Exception);

/// Implement the ranging algorithm based on Bancroft algorithm. If there are only
/// 2 data, the two solutions are equally valid and some other information must be
/// used to choose between them.
/// @param XYZ matrix of components, input
/// @param D vector of , input
/// @param S1 on output, vector solution of "best" solution
/// @param S2 on output, vector solution of second solution
/// @return true if successful
/// @throw on std exception
bool ImplementRanging(const Matrix<double>& XYZ, const Vector<double>& D,
                      Vector<double>& S1, Vector<double>& S2)
   throw(Exception);

/// In the event that there are still unknown points after iterating through the
/// other algorithms, check the unknown points to see if they exist as part
/// of a leveling project
/// @param map of label strings to unknown points
void checkForLeveling(std::map<string, Point> &unknownPoints);

/// As a follow up to checkForLeveling, look for the presence of side shots
/// Will execute in a similar manner to checkForLeveling but will look for chains
/// where one of the end points remains unknown. Should only be two points in a given segment.
///  Will set the lat and lon of the side shot position to be equivalent to the known point
void checkForSideShots(std::map<string, Point> &unknownPoints);

/// A container of Height records. These constitute a level chain. To be
/// valid the from label of the first element and the to label of the last element must be known/estimated
typedef std::vector<Height> levelChain;

/// Find all possible leveling chains from a given start point. Not responsible for any chain filtering
/// logic, just provides all possible leveling chains
/// @param label of the starting point for all of the chains
vector<levelChain> findAllChains(const string &startFromLabel);

/// Find all Height records which contain the fromLabel parameters as the value for DATHeight::From
/// @param From point label that is looked for in the measurements
/// @param A global data instance that is used to provide an easy reference to all of the measurements
vector<Height> findAllHeightRecords(const string &fromLabel, GlobalData &gdInstance);

/// For a given starting point level find a chain that meets some predefined conditions
/// to help assign values to temporary points. Makes a call to findAllChains
/// and from that return executes some logic to pick one final chain
/// @param starting from label for the chain
levelChain givelevelChain(const string &startFromLabel);

//------------------------------------------------------------------------------------
// TD make input?
const static int maxresults(11);       // quit after estimating one point this often
#include <iostream>
using std::cout;
using std::endl;

//------------------------------------------------------------------------------------
/// Given POS A,B and the value of HAN UAB, compute azimuth AU
/// First compute DIS AB and AZM AB; then HAN(UAB) = AZM(AB)-AZM(AU).
/// @param A known Point (rotation to NEU is done here).
/// @param B known Point
/// @param hanUAB Horizontal angle (+clockwise, radians) from U, at A, to B.
/// @param dht return the difference in height (m) of A and B.
/// @return azimuth AU in radians
/// @throw std exception
double computeAZM(const Point& A, const Point& B, const double hanUAB, double& dht)
   throw(Exception)
{
try {
   Matrix<double> Rot(A.getRotation());

   // compute A-B and rotate into NEU at A
   Vector<double> ABxyz(3),ABneu;
   ABxyz[0] = B.x - A.x;
   ABxyz[1] = B.y - A.y;
   ABxyz[2] = B.z - A.z;

   ABneu = Rot * ABxyz;
   //LOG(INFO) << A.label << " " << B.label << fixed << setprecision(3)
   //   << " " << ABneu[0] << " " << ABneu[1] << " " << ABneu[2];

   // compute azimuth(AB) and horizontal distance AB
   double azAB(azimuth(ABneu[1],ABneu[0]));
   double AB(::sqrt(ABneu[0]*ABneu[0] + ABneu[1]*ABneu[1]));
   dht = ABneu[2];

   // get AZM(AU) from HAN(UAB) and AZM(AB)
   // HAN(UAB) = AZM(AB)-AZM(AU)       // definition HAN = difference of 2 AZM
   double azAU = azAB - hanUAB;
   while(azAU < 0.0) azAU += TwoPi;
   while(azAU >= TwoPi) azAU -= TwoPi;

   return azAU;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// return 0 ok, 8 return after a priori
int ComputeAPriori(unsigned int& nUnknown, bool allowDefault, bool is2D)
   throw(Exception)
{
try {
   unsigned int i,j,k,iret(0),count;
   string str;
   map<string,Point>::iterator it;
   GlobalData& GD=GlobalData::Instance();

   // create (by copying) a map of unknown Points
   // add labels in computeAPlabels for binary file
   map<string,Point> Unknowns;
   for(it=GD.Points.begin(); it!=GD.Points.end(); ++it) {
      // save it to unknowns
      if(it->second.fixtype == 0) {
         Unknowns.insert(map<string,Point>::value_type(it->first,it->second));
         GD.computeAPlabels.push_back(it->first);
      }
   }
   nUnknown = Unknowns.size();

   // if there are no unknowns, quit; return 8 if user requested quit after this
   if(nUnknown == 0)
      return (GD.apquit ? 8 : 0);
   else if(GD.doProgress)
      cout << " Computing initial coordinates for " << Unknowns.size() << " points..";

   // compute HANs for use here - from Measurements or by differencing DirSets
   // copy measured HANs
   for(i=0; i<GD.Measurements.size(); i++) {
      if(GD.Measurements[i]->type() != DATtype::HAN) continue;
      const HAngle& han(dynamic_cast<HAngle&>(*(GD.Measurements[i])));
      GD.computeAPHANs.push_back(han);
   }

   // compute HANs from DIRSETs; don't duplicate - assumes all measurements are of
   // same quality.
   count = 0;
   vector<string> labels;
   for(i=0; i<GD.DirSets.size(); i++) {
      // copy out all DIRs for this DIRSET
      vector<Dir> dirs;
      for(j=0; j<GD.DirSets[i].measindex.size(); j++) {
         k = GD.DirSets[i].measindex[j];
         const Dir& d(dynamic_cast<Dir&>(*(GD.Measurements[k])));
         //LOG(INFO) << "Count " << j << " dir " << d.asString();
         dirs.push_back(d);
      }
      // difference them all to get a maximal unique set
      int debugStop = 0;
      for(j=0; j<dirs.size(); j++) {
         for(k=j+1; k<dirs.size(); k++) {
            if(dirs[j].To == dirs[k].To) continue;          // no zeros
            count++;

            str = dirs[j].To + "-" + GD.DirSets[i].At + "-" + dirs[k].To;
            if(vectorindex(labels,str) != -1) continue;     // no duplicates
            labels.push_back(str);

            double value(dirs[k].value - dirs[j].value);
            double sigma(::sqrt(dirs[j].sigma*dirs[j].sigma + 
                                dirs[k].sigma*dirs[k].sigma));
            while(value < 0.0) value += TwoPi;
            while(value > TwoPi) value -= TwoPi;

            HAngle han(dirs[j].To, dirs[j].From, dirs[k].To, value, sigma);
            //LOG(INFO) << "Difference " << dirs[j].asString() << endl
            //   << "      with " << dirs[k].asString() << endl
            //   << "   to get " << han.asString()
            //   << " = " << angleAsDMSstring(TwoPi-han.value);
            GD.computeAPHANs.push_back(han);
         }
      }
   }
   if(labels.size() > 0)
      LOG(VERBOSE) << " Created " << labels.size() << " (of " << count
         << " possible) HANs from DIRSETs by differencing";

   // loop until either there are no unknowns left, or until nothing is changing
   vector<Point> responses;
   vector<string> fixed_labels;
   count = 0;
   while(Unknowns.size() > 0) {
      count++;

      // loop over the unknowns, saving the label of those fixed
      fixed_labels.clear();
      for(it=Unknowns.begin(); it!=Unknowns.end(); ++it) {
         LOG(INFO) << "\n# Compute a priori(" << count
                     << ") position for unknown Point " << it->first;

         // try each algorithm in turn, collecting responses
         // if one of the algorithms is successful, save the results here
         responses.clear();
         for(;;) {               // only one type of algorithm per iteration

            if(DeltaAddAlgorithm(it->first, responses)) break;

            if(AZMVectorAddAlgorithm(it->first, responses)) break;

            if(HANVectorAddAlgorithm(it->first, responses)) break;

            if(TriangulationAlgorithm(it->first, responses, is2D)) break;

            if(TwoAzimuthLinesAlgorithm(it->first, responses)) break;

            if(ResectionAlgorithm(it->first, responses)) break;

            if(RangingAlgorithm(it->first, responses)) break;

            break;      // mandatory
         }  // end one-time loop

         // if responses have been received, find their average and define the POS
         if(responses.size() > 0) {
            EstimateUnknownPoint(it->first,responses);
            fixed_labels.push_back(it->first);
         }
      }

      // no change - must break the iteration loop and fail
      if(fixed_labels.size() == 0) break;

      // remove the fixed ones from Unknowns
      for(i=0; i<fixed_labels.size(); i++)
         Unknowns.erase(fixed_labels[i]);

   }  // end iteration loop

   // clean up
   GD.computeAPHANs.clear();

   // Perform check to see if set up as leveling project
   if(Unknowns.size() > 0)
   {
       checkForLeveling(Unknowns);
       checkForSideShots(Unknowns);
   }

   // failed to find them all - go to the fall-back
   if(allowDefault && Unknowns.size() > 0) {
      fixed_labels.clear();
      for(it=Unknowns.begin(); it!=Unknowns.end(); ++it) {
         responses.clear();
         if(DefaultAlgorithm(it->first, responses)) {
            map<string,Point>::iterator jt = GD.Points.find(it->first);
            if(jt == GD.Points.end())
               GNSSTK_THROW(Exception("Unknown Point label " + it->first));

            jt->second = responses[0];
            jt->second.setEstimate();

            LOG(INFO) << " Result of a priori fixing: \""
                  << jt->second.asString(GD.linprecP,GD.linwidth) << "\"";

            fixed_labels.push_back(it->first);
         }
      }

      // remove the fixed ones from Unknowns
      for(i=0; i<fixed_labels.size(); i++)
         Unknowns.erase(fixed_labels[i]);
   }

   // --------------------------------------------------------------------------
   // done
   if(Unknowns.size() > 0) {
      LOGstrm << " Error - unable to compute a priori positions for "
         << Unknowns.size() << " points:";
      for(it=Unknowns.begin(); it!=Unknowns.end(); ++it) 
         LOGstrm << " " << it->first;
      LOGstrm << endl;
      iret = 11;
   }

   if(iret) {              // failure
      if(GD.doProgress) cout << "failed on " << Unknowns.size() << "." << endl;
      return iret;
   }
   if(GD.doProgress) cout << "done." << endl;

   // user requested quit after a priori algorithm
   if(GD.apquit) return 8;

   return 0;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int DeltaAddAlgorithm(const string lab, vector<Point>& resp) throw(Exception)
{
try {
   int iret(0);
   GlobalData& GD=GlobalData::Instance();

   // search Deltas for a pair that involve this Point + 1 known one
   for(int i=0; i<GD.Measurements.size(); i++) {
      if(GD.Measurements[i]->type() != DATtype::DEL) continue;

      const Delta& delta(dynamic_cast<Delta&>(*(GD.Measurements[i])));
      if(delta.From == lab) {
         const Point& ref(GD.Points[delta.To]);
         if(ref.fixtype == 0) continue;
         Point p(lab, ref.x - delta.dx,
                      ref.y - delta.dy,
                      ref.z - delta.dz, 0,0,0,0,0,0);
         resp.push_back(p);
         // TD?
         LOG(INFO) << " Result(DeltaAdd " << delta.label() << "): " << p.asString();
      }
      else if(delta.To == lab) {
         const Point& ref(GD.Points[delta.From]);
         if(ref.fixtype == 0) continue;
         Point p(lab, ref.x + delta.dx,
                      ref.y + delta.dy,
                      ref.z + delta.dz, 0,0,0,0,0,0);
         resp.push_back(p);
         iret++;
         // TD?
         LOG(INFO) << " Result(DeltaAdd " << delta.label() << "): " << p.asString();
      }
   }

   return iret;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
/// Helper class for TriangulationAlgorithm storing HANs that may yield triangle.
class TAlgoHan {
public:
   string From, At, To;       ///< Point labels
   double value;              ///< angle in radians

   // constructor - NB do NOT make v a reference!
   TAlgoHan(const string& F, const string& A, const string& T, const double v)
      : From(F), At(A), To(T), value(v)
   {
      while(value >= TwoPi) value -= TwoPi;
      while(value < 0.0) value += TwoPi;
   }

   // asString
   string asString(void)
   {
      ostringstream oss;
      oss << From << "-" << At << "-" << To << " = "
         << fixed << setprecision(2) << value*::RAD_TO_DEG << " deg";
      return oss.str();
   }
};

/// Helper class for TriangulationAlgorithm storing pairs of angles that are input to
/// the implementation algorithm ImplementLawSines().
class TAlgoPair {
public:
   string Alab, Blab;            ///< Point labels
   double hanUAB, hanUBA;        ///< angle values in radians
   // constructor - NB do NOT make val's references!
   TAlgoPair(const string& A, const string& B,
             const double valUAB, const double valUBA)
             : Alab(A), Blab(B), hanUAB(valUAB), hanUBA(valUBA)
   {
      while(hanUAB >= TwoPi) hanUAB -= TwoPi;
      while(hanUAB < 0.0) hanUAB += TwoPi;
      while(hanUBA >= TwoPi) hanUBA -= TwoPi;
      while(hanUBA < 0.0) hanUBA += TwoPi;
   }

   string asString(void)
   {
      ostringstream oss;
      oss << fixed << setprecision(2)
         << " UAB: U-" << Alab << "-" << Blab << " = " << hanUAB*::RAD_TO_DEG<< " deg"
         << "; UBA = " << hanUBA*::RAD_TO_DEG << " deg";
      return oss.str();
   }
};

// Find two known Points A and B, and two different horizontal angles involving A, B
// and another, unknown, Point U(unlab), and call ImplementLawSines() to compute U.
// This algorithm requires 2 HANs of {BAU, ABU, AUB} with A and B fixed;
// note that the "At" Point in this list {} is A, B or U.
// Create 2 lists, a) the At list {AUB} where U is At and A,B are fixed, and
//                 b) the "not-At" list {ABU or UAB} where A,B are fixed.
// Now find pairs of angles that have the same A and B but different "At"s. Keep in
// mind the identity ABC + CBA = 2pi, but don't use this identity to add to the lists.
// There are two ways to find a pair (again, note that the "At" is crucial):
//   a. For each AUB in the "At" list, search the other list for ABU UBA BAU or UAB
//   b. For each ABU in the "not-At" list, search the rest of that list for BAU or UAB
// With any such pair of angles, you can get the third angle in the triangle (if
// necessary) and call the implementation routine. All the pairs you find from the
// lists will be independent triangulations, b/c each is an independent measurement.
int TriangulationAlgorithm(const string unlab, vector<Point>& resp, bool is2D)
   throw(Exception)
{
try {
   int iret(0);
   unsigned int i,j,k,n;
   string From, At, To;
   GlobalData& GD=GlobalData::Instance();

   // Generate 2 list of horizontal angles that involve the unknown Point U (unlab)
   // and two other fixed Points. In one list U is the At site, in the other it is not
   vector<TAlgoHan> Atlist,NotAtlist;
   for(i=0; i<GD.computeAPHANs.size(); i++) {
      // copy it
      const HAngle& HanRST(GD.computeAPHANs[i]);

      // At is U, if others are fixed, add to Atlist
      if(HanRST.At == unlab) {
         if(GD.Points[HanRST.To].isKnown() && GD.Points[HanRST.From].isKnown()){
            TAlgoHan h(HanRST.From, unlab, HanRST.To, HanRST.value);
            Atlist.push_back(h);
            //LOG(INFO) << " Found Atlist " << h.asString();
         }
      }

      // From is U
      else if(HanRST.From == unlab) {
         // UAB is found explicitly
         if(GD.Points[HanRST.At].isKnown() && GD.Points[HanRST.To].isKnown()) {
            TAlgoHan h(unlab, HanRST.At, HanRST.To, HanRST.value);
            NotAtlist.push_back(h);
            //LOG(INFO)<< " Found NotAtlist1 " << h.asString();
         }

         // UAB = (RST==UAC) + (CAB or 2Pi-BAC)
         for(j=0; j<GD.computeAPHANs.size(); j++) {
            if(j==i) continue;
            const HAngle& HanCAB(GD.computeAPHANs[j]);
            // At == A must match
            if(HanCAB.At != HanRST.At) continue;

            // C must match and either To or From (B) must be fixed
            if(HanCAB.To == HanRST.To && GD.Points[HanCAB.From].isKnown()) {
               // HanCAB is BAC and B is fixed
               TAlgoHan h(unlab, HanRST.At, HanCAB.From,    // UAB=UAC+CAB
                                       HanRST.value + (TwoPi - HanCAB.value));
               NotAtlist.push_back(h);
               //LOG(INFO) << " Found NotAtlist2 " << h.asString();
            }
            else if(HanCAB.From == HanRST.To && GD.Points[HanCAB.To].isKnown()) {
               // HanCAB is CAB and B is fixed           // UAB=UAC+CAB
               TAlgoHan h(unlab, HanRST.At, HanCAB.To, HanRST.value + HanCAB.value);
               NotAtlist.push_back(h);
               //LOG(INFO) << " Found NotAtlist3 " << h.asString();
            }
         }
      }

      // To is U
      else if(HanRST.To == unlab) {
         // ABU is found explicitly 
         if(GD.Points[HanRST.From].isKnown() && GD.Points[HanRST.At].isKnown()) {
            TAlgoHan h(HanRST.From, HanRST.At, unlab, HanRST.value);
            NotAtlist.push_back(h);
            //LOG(INFO) << " Found NotAtlist4 " << h.asString();
         }

         // ABU = (ABC or CBA) + (RST==CBU)
         for(j=0; j<GD.computeAPHANs.size(); j++) {
            if(j==i) continue;
            const HAngle& HanABC(GD.computeAPHANs[j]);
            // At == B must match
            if(HanABC.At != HanRST.At) continue;

            // C must match and either To or From (A) must be fixed
            if(HanABC.To == HanRST.From && GD.Points[HanABC.From].isKnown()) {
               // HanABC is ABC and A is fixed                 // ABU=ABC+CBU
               TAlgoHan h(HanABC.From, HanRST.At, unlab, HanRST.value + HanABC.value);
               NotAtlist.push_back(h);
               //LOG(INFO) << " Found NotAtlist5 " << h.asString();
            }
            else if(HanABC.From == HanRST.From && GD.Points[HanABC.To].isKnown()) {
               // HanABC is CBA and A is fixed
               TAlgoHan h(HanABC.To, HanRST.At, unlab,   // ABU=ABC+CBU
                                    (TwoPi - HanABC.value) + HanRST.value);
               NotAtlist.push_back(h);
               //LOG(INFO) << " Found NotAtlist6 " << h.asString();
            }
         }
      }
   }

   // Find pairs of angles that have the same fixed Points but different "At"s.
   double a,b,c;
   vector<TAlgoPair> pairs;         // need A, B, a=hanUAB, b=hanUBA

   // For each AUB in Atlist, search the NotAtlist for ABU UBA or BAU UAB
   for(i=0; i<Atlist.size(); i++) {
      From = Atlist[i].From;                 // call this AUB: From == A, To == B
      To = Atlist[i].To;

      for(j=0; j<NotAtlist.size(); j++) {
         if(From == NotAtlist[j].At) {          // At is A -------------
            bool isBAU(To == NotAtlist[j].From);                        // BAU : UAB
            a = (isBAU ? TwoPi - NotAtlist[j].value : NotAtlist[j].value);    // UAB
            c = TwoPi - Atlist[i].value;                                      // BUA
            // UAB+ABU+BUA=Pi  UBA=2Pi-(ABU) = 2Pi - (Pi - BUA - UAB)
            b = Pi + c + a;
            TAlgoPair p(From,To,a,b);
            pairs.push_back(p);
            //LOG(INFO) << " Found pair AUB/" << (isBAU?"BAU":"UAB") << p.asString();
         }
         else if(To == NotAtlist[j].At) {       // At is B -------------
            bool isABU(From == NotAtlist[j].From);                      // ABU : UBA
            b = (isABU ? NotAtlist[j].value : TwoPi - NotAtlist[j].value);    // ABU
            c = TwoPi - Atlist[i].value;                                      // BUA
            // UAB = Pi-ABU-BUA
            a = Pi - b - c;
            TAlgoPair p(From,To,a,b);
            pairs.push_back(p);
            //LOG(INFO) << " Found pair AUB/" << (isABU?"ABU":"UBA") << p.asString();
         }
      }
   }

   // for each UAB or BAU in NotAtlist, search the rest of the list for UBA or ABU
   for(i=0; i<NotAtlist.size(); i++) {
      At = NotAtlist[i].At;                     // call this A
      From = NotAtlist[i].From;                 // U or B
      To = NotAtlist[i].To;                     // B or U
      bool iisUAB(From == unlab);

      for(j=i+1; j<NotAtlist.size(); j++) {
         // At for j angle must be B
         if(NotAtlist[j].At != (iisUAB ? To : From)) continue;
         bool jisUBA(NotAtlist[j].From == unlab);
         // other angle must be A
         if(At != (jisUBA ? NotAtlist[j].To : NotAtlist[j].From)) continue;

         // need A, B, a=hanUAB, b=hanUBA
         if(iisUAB) {                           // i,j = UAB,UBA|ABU
            a = NotAtlist[i].value;
            b = (jisUBA ? NotAtlist[j].value : TwoPi - NotAtlist[j].value);
            TAlgoPair p(At,NotAtlist[j].At,a,b);
            pairs.push_back(p);
            //LOG(INFO) << " Found pair UAB/" << (jisUBA?"UBA":"ABU") << p.asString();
         }
         else {                                 // i,j = BAU,UBA|ABU
            a = TwoPi - NotAtlist[i].value;
            b = (jisUBA ? NotAtlist[j].value : TwoPi - NotAtlist[j].value);
            TAlgoPair p(At,NotAtlist[j].At,a,b);
            pairs.push_back(p);
            //LOG(INFO) << " Found pair BAU/" << (jisUBA?"UBA":"ABU") << p.asString();
         }
      }
   }

   for(n=0,i=0; i<pairs.size(); i++) {
      string msg = pairs[i].Alab + "-" + pairs[i].Blab + "-" + unlab;

      //LOG(INFO) << " Call with " << fixed << setprecision(4)
      //   << unlab << "-" << pairs[i].Alab << "-" << pairs[i].Blab
      //      << " = " << ::RAD_TO_DEG*pairs[i].hanUAB << " deg; "
      //   << unlab << "-" << pairs[i].Blab << "-" << pairs[i].Alab
      //      << " = " << ::RAD_TO_DEG*pairs[i].hanUBA << " deg.";

      Point p(ImplementLawSines(GD.Points[pairs[i].Alab], GD.Points[pairs[i].Blab],
                                          pairs[i].hanUAB, pairs[i].hanUBA, is2D));
      if(p.isUnknown()) {
         LOG(INFO)
            << " Failed(Triangulation " << msg << "): sine of angle too small.";
      }
      else {
         p.label = unlab;
         resp.push_back(p);
         iret++;
         LOG(INFO) << " Result(Triangulation " << msg << "): " << p.asString();
      }

      if(++n > maxresults) {                // TD see Test42 - redundant triangles
         LOG(VERBOSE) << " Maximum number of results reached - quit.";
         break;
      }
   }

   return iret;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Given a known Point A, distance AU to an unknown Point U, and azimuth azAU, find U.
int AZMVectorAddAlgorithm(const string unlab, vector<Point>& resp)
   throw(Exception)
{
try {
   int iret(0),i,j;
   GlobalData& GD=GlobalData::Instance();

   // search for azimuths that involve the unknown and a known Point,
   // and distances that involve the same pair of Points.
   for(i=0; i<GD.Measurements.size(); i++) {
      if(GD.Measurements[i]->type() != DATtype::AZM) continue;
      const Azimuth& azim(dynamic_cast<Azimuth&>(*(GD.Measurements[i])));
      double azAU;
      string To(azim.To), From(azim.From);
      string Alab("");
      if(unlab == To && GD.Points[From].isKnown()) {
         Alab = From;
         azAU = azim.value;
      }
      else if(unlab == From && GD.Points[To].isKnown()) {
         Alab = To;
         azAU = Pi + azim.value;
         if(azAU > TwoPi) azAU -= TwoPi;
      }
      else continue;

      // now look for distance with unlab and A
      for(j=0; j<GD.Measurements.size(); j++) {
         if(GD.Measurements[j]->type() != DATtype::DIS) continue;
         const Dist& dist(dynamic_cast<Dist&>(*(GD.Measurements[j])));
         if((unlab == dist.From && Alab == dist.To) ||
            (unlab == dist.To && Alab == dist.From))
         {
            double AU = dist.value;
            //LOG(INFO) << " AZMVectorAddAlgorithm (" << unlab << ") found AZM "
            //   << azim.label() << " and DIS " << dist.label();

            Point p = ImplementAZMVectorAdd(GD.Points[Alab], azAU, AU);
            p.label = unlab;
            resp.push_back(p);
            iret++;

            LOG(INFO) << " Result(AZMVectorAdd " << azim.label()
               << " & " << dist.label() << "): " << p.asString();
         }
      }  // end loop over Dists
   }  // end loop over Azimuths

   return iret;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Given known Points A and B, distance AU to unknown Point U, and HAN(BAU), find U.
// Call ImplementHANVectorAdd().
int HANVectorAddAlgorithm(const string unlab, vector<Point>& resp)
   throw(Exception)
{
try {
   int iret(0),i,j,n;
   GlobalData& GD=GlobalData::Instance();

   // search for HANs that involve this Point plus 2 known ones 
   // NB unknown must NOT be "At"
   for(n=0,i=0; i<GD.computeAPHANs.size(); i++) {
      const HAngle& hang(GD.computeAPHANs[i]);

      // ignore the case where the unknown is at the instrument
      if(hang.At == unlab) continue;

      // At Point in HAN must be known
      string Alab(hang.At), Blab;
      if(GD.Points[Alab].isUnknown()) continue;

      // is this a HAN(BAU) or HAN(UAB) where B is known?
      if(hang.From == unlab && GD.Points[hang.To].isKnown())
         Blab = hang.To;
      else if(hang.To == unlab && GD.Points[hang.From].isKnown())
         Blab = hang.From;
      else
         continue;

      // finally, must have distance AU; try to find it
      bool found(false);
      double AU;
      string AUlabel;
      for(j=0; j<GD.Measurements.size(); j++) {
         if(GD.Measurements[j]->type() != DATtype::DIS) continue;
         const Dist& dist(dynamic_cast<Dist&>(*(GD.Measurements[j])));
         if((dist.From == unlab && dist.To   == Alab) ||
            (dist.To   == unlab && dist.From == Alab))
         {
            found = true;
            AU = dist.value;
            AUlabel = dist.label();
            break;
         }
      }
      // no distance AU found
      if(!found) continue;

      // got it, now get angle in standard form
      double hanUAB(hang.value);
      if(hang.To == unlab) {              // BAU - convert to UAB
         hanUAB = TwoPi - hanUAB;
         while(hanUAB > TwoPi) hanUAB -= TwoPi;
         while(hanUAB < 0.0)   hanUAB += TwoPi;
      }

      //LOG(INFO) << " HANVectorAddAlgorithm (" << unlab << ") found HAN "
      //   << hang.label() << " and DIS " << AUlabel
      //   << " (" << angleAsDMSstring(hanUAB)
      //   << fixed << setprecision(3) << " and " << AU << ")";

      Point p = ImplementHANVectorAdd(GD.Points[Alab], GD.Points[Blab], hanUAB, AU);
      p.label = unlab;
      resp.push_back(p);
      iret++;

      LOG(INFO) << " Result(HANVectorAdd " << hang.label()
                     << " & " << AUlabel << "): " << p.asString();

      if(++n > maxresults) {                // see DirSetCentDIR
         LOG(VERBOSE) << " Maximum number of results reached - quit.";
         break;
      }

   }  // end loop over HANs

   return iret;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int TwoAzimuthLinesAlgorithm(const string unlab, vector<Point>& resp) throw(Exception)
{
try {
   int iret(0);
   unsigned i,j;
   double azAU;
   vector<string> which;
   GlobalData& GD=GlobalData::Instance();

   // search for 2+ azimuths that involve the unknown and a different known POS
   vector<Azimuth> Azimuths;

   // a) All azimuths that include U and a known position
   // b) All HANs CAU such that A,C are known positions
   for(i=0; i<GD.Measurements.size(); i++) {
      if(GD.Measurements[i]->type() == DATtype::AZM) {
         Azimuth& azim(dynamic_cast<Azimuth&>(*(GD.Measurements[i])));
         if((unlab == azim.To && GD.Points[azim.From].isKnown()) ||
            (unlab == azim.From && GD.Points[azim.To].isKnown()))
         {
            Azimuths.push_back(azim);                          // (a)
            which.push_back("a");
         }
      }
   }

   for(i=0; i<GD.computeAPHANs.size(); i++) {
      HAngle& han(GD.computeAPHANs[i]);

      // Exclude U at "At"
      if(unlab != han.To && unlab != han.From) continue;

      // yes, han is either CAU or UAC
      string labA(han.At), labC(unlab==han.To ? han.From : han.To);
      // Are A and C known?
      if(GD.Points[labA].isUnknown() || GD.Points[labC].isUnknown()) continue;

      double hanCAU(han.value),hanUAC,dht;
      if(han.To == labC) {                // han is UAC, make it CAU
         hanUAC = hanCAU;
         hanCAU = TwoPi - hanCAU;
      }
      else {                              // han is CAU
         hanUAC = TwoPi - hanCAU;
      }
      while(hanCAU < 0.0) hanCAU += TwoPi;
      while(hanCAU >= TwoPi) hanCAU -= TwoPi;
      while(hanUAC < 0.0) hanUAC += TwoPi;
      while(hanUAC >= TwoPi) hanUAC -= TwoPi;
      //LOG(INFO) << " HAN(" << labC << "-" << labA << "-" << unlab << ") is "
      //<< angleAsDMSstring(hanCAU);
      //LOG(INFO) << " HAN(" << unlab << "-" << labA << "-" << labC << ") is "
      //<< angleAsDMSstring(hanUAC);

      // A,C known; find AZM AU from A, C and HAN UAC
      azAU = computeAZM(GD.Points[labA],GD.Points[labC],hanUAC,dht);
      //LOG(INFO) << " AZM(" << labA << "-" << unlab << ") is "
      //<< angleAsDMSstring(azAU) << "\n";
      DATAzimuth daz(labA,unlab,azAU,0.0);
      Azimuth azmAU(daz);
      Azimuths.push_back(azmAU);                            // (b)
      which.push_back("b");
   }

   // Azimuths must be unique
   vector<string> labels;
   vector<unsigned> rejects;
   for(i=0; i<Azimuths.size(); i++) {
      if(vectorindex(labels, Azimuths[i].From) == -1) {
         labels.push_back(Azimuths[i].From);
      }
      else {
         rejects.push_back(i);
      }
   }

   for(int k=rejects.size()-1; k>=0; k--)
      Azimuths.erase(Azimuths.begin()+rejects[k]);

   // Summary output
   if(Azimuths.size() > 1) {
      LOG(INFO) << " TwoAzimuthAlgorithm (" << unlab << ") found "
                              << Azimuths.size() << " azimuths:";
      for(i=0; i<Azimuths.size(); i++)
         LOG(INFO) << "  AZM (" << which[i] << ") " << Azimuths[i].asString();
   }

   if(Azimuths.size() < 2) {
      //LOG(INFO) << " Not enough data for TwoAzimuth algorithm.";
      return 0;   // need two lines to meet
   }

   // loop over pairs of Azimuths; for each pair:
   //    find fixed Points A(i) and B(j)
   //    rotate into NEU at A
   //    compute azimuth of A->B and horizontal distance AB
   //    NB. HAN(URS) = AZM(RS)-AZM(RU) and AZM(RS) = 2PI - AZM(SR)
   //    compute HAN(UAB) and HAN(ABU)
   //    now apply the triangulation algorithm to get U:
   //       sum angles of a triangle => HAN(AUB) = PI - HAN(UAB) - HAN(ABU)
   //       law of sines gives you sin(AUB)/AB = sin(ABU)/AU
   //       with AU and AZM(AU) use vector addition to get U = A + vector(AU,AZM(AU))
   for(i=0; i<Azimuths.size(); i++) {
      string Alab;
      azAU = Azimuths[i].value;
      if(unlab == Azimuths[i].To)
         Alab = Azimuths[i].From;
      else {
         Alab = Azimuths[i].To;
         azAU = TwoPi - azAU;
      }
      const Point& A(GD.Points[Alab]);

      for(j=0; j<i; j++) {
         double azBU(Azimuths[j].value);
         string Blab;
         if(unlab == Azimuths[j].To)
            Blab = Azimuths[j].From;
         else {
            Blab = Azimuths[j].To;
            azBU = TwoPi - azBU;
         }
         const Point& B(GD.Points[Blab]);

         // TD check that azAU != azBU +- Pi  i.e. triangle is a line
         double del(azAU-azBU);
         while(del < 0.0) del += TwoPi;
         while(del >= TwoPi) del -= TwoPi;
         //LOG(INFO) << " Implement TwoAzimuths algorithm with critical angle "
         //   << angleAsDMSstring(del);
         //LOG(INFO) << "Call ImplementTwoAzimuths with AZM(" << Alab << "-" << unlab
         //   << ") = " << angleAsDMSstring(azAU)
         //   << "\n                           and AZM(" << Blab << "-" << unlab
         //   << ") = " << angleAsDMSstring(azBU);

         Point p = ImplementTwoAzimuths(A, B, azAU, azBU);
         if(p.isUnknown()) {
            LOG(INFO) << " Failed(TwoAzimuths " << Azimuths[i].label()
               << " + " << Azimuths[j].label() << "): sine of angle too small.";
            continue;
         }
         resp.push_back(p);
         iret++;
         LOG(INFO) << " Result(TwoAzimuths " << Azimuths[i].label()
            << " + " << Azimuths[j].label() << "): " << p.asString();
      }
   }

   return iret;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int ResectionAlgorithm(const string unlab, vector<Point>& resp) throw(Exception)
{
try {
   int iret(0);
   unsigned i,j,k,n;
   string Alab,Blab,Clab,Tlab;
   GlobalData& GD=GlobalData::Instance();

   // find all HANs that have unlab in the "At" station
   vector<string> Plabels;
   vector<HAngle> HANs;
   for(i=0; i<GD.computeAPHANs.size(); i++) {
      const HAngle& hang(GD.computeAPHANs[i]);

      // ignore the case where the unknown is not at the instrument
      if(hang.At != unlab) continue;

      // To and From Points in HAN(AUB) must be known
      Alab = hang.From, Blab = hang.To;
      if(GD.Points[Alab].isUnknown()) continue;
      if(GD.Points[Blab].isUnknown()) continue;

      //LOG(INFO) << "Resection found HAN " << hang.asString();

      HANs.push_back(hang);
      if(vectorindex(Plabels, hang.To) == -1) Plabels.push_back(hang.To);
      if(vectorindex(Plabels, hang.From) == -1) Plabels.push_back(hang.From);
   }

   if(HANs.size() < 2) return 0;

   // find pairs of HANs that share one common Point, but not both:  AB,BC etc
   double hanAUB,hanBUC,hanCUA,dtmp;
   for(n=0,i=0; i<HANs.size(); i++) {

      // find Clab and the other angles - BUC and CUA
      // identities:    AUB + BUC + CUA = TwoPi    FAT + TAF = TwoPi
      for(j=i+1; j<HANs.size(); j++) {
         // HANs[i] is AUB by definition ... for now
         // NB Alab, Blab, hanAUB gets modified below
         Alab = HANs[i].From;
         Blab = HANs[i].To;
         hanAUB = HANs[i].value;
         while(hanAUB < 0.0) hanAUB += TwoPi;
         while(hanAUB >= TwoPi) hanAUB -= TwoPi;

         if(HANs[j].From == Alab && HANs[j].To != Blab) {         // HANs[j] is AUC
            Clab = HANs[j].To;
            hanCUA = TwoPi - HANs[j].value;
            hanBUC = TwoPi - hanAUB - hanCUA;
         }
         else if(HANs[j].To == Alab && HANs[j].From != Blab) {    // HANs[j] is CUA
            Clab = HANs[j].From;
            hanCUA = HANs[j].value;
            hanBUC = TwoPi - hanAUB - hanCUA;
         }
         else if(HANs[j].From == Blab && HANs[j].To != Alab) {    // HANs[j] is BUC
            Clab = HANs[j].To;
            hanBUC = HANs[j].value;
            hanCUA = TwoPi - hanAUB - hanBUC;
         }
         else if(HANs[j].To == Blab && HANs[j].From != Alab) {    // HANs[j] is CUB
            Clab = HANs[j].From;
            hanBUC = TwoPi - HANs[j].value;
            hanCUA = TwoPi - hanAUB - hanBUC;
         }
         else
            continue;   // no common point

         // -----------------------------------------------------------
         // found three angles AUB and BUC and CUA, so Resection is possible
         //LOG(INFO) << "Have resection raw form:"
         //   << " A=" << Alab << " B=" << Blab << " C=" << Clab
         //   << " AUB=" << angleAsDMSstring(hanAUB)
         //   << "  and BUC=" << angleAsDMSstring(hanBUC);

         // must order them so they are all positive and <= Pi
         // first make them all positive
         while(hanBUC < 0.0) hanBUC += TwoPi;
         while(hanBUC >= TwoPi) hanBUC -= TwoPi;
         while(hanCUA < 0.0) hanCUA += TwoPi;
         while(hanCUA >= TwoPi) hanCUA -= TwoPi;

         // if BUC is > Pi, switch B and C
         // NB BUC>Pi must be first here, b/c if AUB>Pi and BUC>Pi, must switch B,C 
         if(hanBUC > Pi) {
            //LOG(INFO) << "Switch B and C";
            Tlab=Blab; Blab=Clab; Clab=Tlab;
            hanBUC = TwoPi - hanBUC;         // BUC is replaced with CUB
            dtmp = hanCUA;
            hanCUA = TwoPi - hanAUB;         // CUA is replaced with BUA
            hanAUB = TwoPi - dtmp;           // AUB is replaced with AUC
            // make positive again
            while(hanAUB < 0.0) hanAUB += TwoPi;
            while(hanAUB >= TwoPi) hanAUB -= TwoPi;
            while(hanBUC < 0.0) hanBUC += TwoPi;
            while(hanBUC >= TwoPi) hanBUC -= TwoPi;
            while(hanCUA < 0.0) hanCUA += TwoPi;
            while(hanCUA >= TwoPi) hanCUA -= TwoPi;
         }
         // if AUB is > Pi, switch A and B
         else if(hanAUB > Pi) {
            //LOG(INFO) << "Switch A and B";
            Tlab=Alab; Alab=Blab; Blab=Tlab;
            hanAUB = TwoPi - hanAUB;         // AUB is replaced with BUA
            dtmp = hanCUA;
            hanCUA = TwoPi - hanBUC;         // CUA is replaced with CUB
            hanBUC = TwoPi - dtmp;           // BUC is replaced with AUC
            // make positive again
            while(hanAUB < 0.0) hanAUB += TwoPi;
            while(hanAUB >= TwoPi) hanAUB -= TwoPi;
            while(hanBUC < 0.0) hanBUC += TwoPi;
            while(hanBUC >= TwoPi) hanBUC -= TwoPi;
            while(hanCUA < 0.0) hanCUA += TwoPi;
            while(hanCUA >= TwoPi) hanCUA -= TwoPi;
         }

         // --------------------------------------
         // if alpha is equal to Pi, must switch A-C-B to C-B-A; cf Ligas Fig 4,5
         if(::fabs(hanAUB - Pi) < 1.e-12) {
            //LOG(INFO) << "alpha is Pi, switch A-B-C";
            Tlab=Alab; Alab=Blab; Blab=Clab; Clab=Tlab;
            hanAUB = hanBUC;
            hanBUC = Pi - hanBUC;
            while(hanAUB < 0.0) hanAUB += TwoPi;
            while(hanBUC < 0.0) hanBUC += TwoPi;
            while(hanAUB >= TwoPi) hanAUB -= TwoPi;
            while(hanBUC >= TwoPi) hanBUC -= TwoPi;
         }
         // if either angle (only one can be) is >= Pi, switch
         else if(hanAUB >= Pi) {
            //     C             A        B             C
            //      \           /          \           /
            //       \         /            \         /
            //        \       /     =>       \       /
            //         \ CUA /                \ BUC /
            //          \   /                  \   /
            //      BUC  \ /\              AUB  \ /
            // B----------U  \        A----------U
            //          \ AUB/
            Tlab = Alab; Alab = Blab; Blab = Clab; Clab = Tlab;
            dtmp = hanAUB;
            hanAUB = hanBUC;
            hanBUC = TwoPi - hanBUC - dtmp;
            hanCUA = dtmp;
            if(hanBUC < 0.0) {               // have to switch B,C
               Tlab = Blab; Blab = Clab; Clab = Tlab;
               hanBUC = -hanBUC;
               hanAUB = TwoPi - hanCUA;
               //LOG(INFO) << "Switch C-B-A and trade B-C";
            }
            //else { LOG(INFO) << "Switch C-B-A"; }
         }
         else if(hanBUC >= Pi) {
            //     A             B        B             C
            //      \           /          \           /
            //       \         /            \         /
            //        \       /     =>       \       /
            //         \ AUB /                \ BUC /
            //          \   /                  \   /
            //           \ /\              AUB  \ /
            // C----------U  \        A----------U
            //          \ BUC/
            Tlab = Alab; Alab = Clab; Clab = Blab; Blab = Tlab;
            dtmp = hanBUC;
            hanBUC = hanAUB;
            hanAUB = (TwoPi - dtmp) - hanAUB;
            if(hanAUB < 0.0) {               // have to switch A,B
               Tlab = Alab; Alab = Blab; Blab = Tlab;
               hanAUB = -hanAUB;
               hanBUC = TwoPi - dtmp;
               //LOG(INFO) << "Switch A-C-B and trade A-B";
            }
            //else { LOG(INFO) << "Switch A-C-B"; }
         }
         //else { LOG(INFO) << "No switches"; }

         // --------------------------------------------------
         // standard form: both angles are between zero and Pi
         //LOG(INFO) << "Have resection standard form:"
         //   << " A=" << Alab << " B=" << Blab << " C=" << Clab
         //   << " AUB=" << angleAsDMSstring(hanAUB)
         //   << "  and BUC=" << angleAsDMSstring(hanBUC);
         const Point& A(GD.Points[Alab]);
         const Point& B(GD.Points[Blab]);
         const Point& C(GD.Points[Clab]);

         // convenience
         vector<string> labels;
         labels.push_back(Alab);
         labels.push_back(Blab);
         labels.push_back(Clab);

         // compute the centroid of the Points and debias the coordinates
         int npts(0);
         Triple xyz0(0,0,0);
         for(k=0; k<3; k++) {       // labels.size() == 3
            xyz0[0] += (GD.Points[labels[k]].x - xyz0[0])/(npts+1);
            xyz0[1] += (GD.Points[labels[k]].y - xyz0[1])/(npts+1);
            xyz0[2] += (GD.Points[labels[k]].z - xyz0[2])/(npts+1);
            npts++;
         }
         //LOG(INFO) << "Resection center: " << fixed << setprecision(3)
         //   << xyz0[0] << " " << xyz0[1] << " " << xyz0[2];

         // get the rotation matrix at center
         Position cent(xyz0);
         Matrix<double> Rot(northEastUp(cent));

         // rotate into NEU, and put results in Matrix, rows A, B, C, U, cols E,N
         Matrix<double> EN(4,2);       // labels.size() == 3
         Vector<double> S(3), RS, Up(3);
         for(k=0; k<3; k++) {
            S(0) = GD.Points[labels[k]].x - xyz0[0];
            S(1) = GD.Points[labels[k]].y - xyz0[1];
            S(2) = GD.Points[labels[k]].z - xyz0[2];
            RS = Rot*S;
            EN(k,0) = RS(1);     // E
            EN(k,1) = RS(0);     // N
            Up(k) = RS(2);       // U
         }

         // for Ligas example 1
         //LOG(INFO) << " ************ packing in Ligas example 1 **************";
         //EN(0,0) = 87.391; EN(0,1) = 258.061; Up(0) = 0.0; hanAUB = ;
         //EN(1,0) = 47.268; EN(1,1) = 274.028; Up(1) = 0.0; hanBUC = ;
         //EN(2,0) = 11.747; EN(2,1) = 258.918; Up(2) = 0.0;
         //LOG(INFO) << " ************ packing in Ligas example 4 **************";
         //EN(0,0) = 289.729; EN(0,1) = 59.288; Up(0) = 0.0; hanAUB = ;
         //EN(1,0) = 259.142; EN(1,1) = 59.288; Up(1) = 0.0; hanBUC = ;
         //EN(2,0) = 216.447; EN(2,1) = 91.116; Up(2) = 0.0;

         // call the Ligas algorithm
         // negative signs b/c Ligas assumes the opposite sign convention
         if(ImplementResection(EN, -hanAUB, -hanBUC)) {
            //LOG(INFO) << "Resection solution E-N:" << fixed << setprecision(3)
            //   << " U: " << setw(8) << EN(3,0) << " " << setw(8) << EN(3,1);

            // rotate back and remove the bias
            S(0) = EN(3,1);      // N
            S(1) = EN(3,0);      // E
            S(2) = 0.0;          // ht of centroid
            RS = transpose(Rot)*S;
            S(0) = RS(0) + xyz0[0];
            S(1) = RS(1) + xyz0[1];
            S(2) = RS(2) + xyz0[2];

            Point p(unlab, S(0), S(1), S(2), 0,0,0,0,0,0);
            resp.push_back(p);
            LOG(INFO) << " Result(Resection): " << p.asString();

            ++n;
         }
      }  // end j-loop over other han

      if(n > maxresults) {                  // see NGA33c
         LOG(VERBOSE) << " Maximum number of results reached - quit.";
         break;
      }

   }  // end i-loop over all hans

   return iret;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Given n different known Points each with known distance to an unknown Point U,
// find U by solving a "ranging problem" using a variation of the Bancroft algorithm.
// This algorithm solves the range equations directly, yielding two possible solutions
// (because the distance is quadratic in the components). By substituting these
// solutions back into the original equations, the valid solution is determined.
// This applys only if n > 2; if n < 2 there is no solution, and if n==2 some other
// piece of information must be used to determine the correct root.
int RangingAlgorithm(const string unlab, vector<Point>& resp) throw(Exception)
{
try {
   int iret(0);
   unsigned i;
   GlobalData& GD=GlobalData::Instance();

   vector<string> labels;
   vector<double> dists;
   for(i=0; i<GD.Measurements.size(); i++) {
      if(GD.Measurements[i]->type() != DATtype::DIS) continue;

      const Dist& dist(dynamic_cast<Dist&>(*(GD.Measurements[i])));
      if(dist.To == unlab && GD.Points[dist.From].isKnown()) {
          if(vectorindex(labels, dist.From) != -1) continue;

          labels.push_back(dist.From);
         dists.push_back(dist.value);
         //LOG(INFO) << "Found distance to " << unlab << " : " << dist.asString();
      }

      if(dist.From == unlab && GD.Points[dist.To].isKnown()) {
         if(vectorindex(labels, dist.To) != -1) continue;

         labels.push_back(dist.To);
         dists.push_back(dist.value);
         //LOG(INFO) << "Found distance from " << unlab << " : " << dist.asString();
      }

   }  // end loop over dists

   // no good without at least 2
   if(labels.size() == 2) {
      LOG(WARNING) << " Warning - ranging algorithm found two data.";
      return 0;
   }
   if(labels.size() < 2) return 0;

   // compute the centroid of the Points and debias the coordinates
   int n(0);
   Triple xyz0(0,0,0);
   for(i=0; i<labels.size(); i++) {
      xyz0[0] += (GD.Points[labels[i]].x - xyz0[0])/(n+1);
      xyz0[1] += (GD.Points[labels[i]].y - xyz0[1])/(n+1);
      xyz0[2] += (GD.Points[labels[i]].z - xyz0[2])/(n+1);

      n++;
   }
   //LOG(INFO) << "Ranging center is (" << n << "): " << fixed << setprecision(2)
    //<< xyz0[0] << " " << xyz0[1] << " " << xyz0[2];

   // get the rotation matrix at center
   Position cent(xyz0);
   Matrix<double> Rot(northEastUp(cent));

   // rotate into NEU, and put N,E results in
   // Matrix form, one Point per line, columns N, E and Vector D.
   // TD do it 3 at a time
   const unsigned int dim(2);
   //LOG(INFO) << "Ranging data: label  N  E  Dist";
   Matrix<double> NE(labels.size(),dim);
   Vector<double> V(3), RV, D(labels.size());
   for(i=0; i<labels.size(); i++) {

      V(0) = GD.Points[labels[i]].x - xyz0[0];
      V(1) = GD.Points[labels[i]].y - xyz0[1];
      V(2) = GD.Points[labels[i]].z - xyz0[2];

      RV = Rot*V;
      NE(i,0) = RV(0);
      NE(i,1) = RV(1);
      if(dim == 3) NE(i,2) = RV(2);
      //D(i) = ::sqrt(dists[i]*dists[i] - RV(2)*RV(2));
      D(i) = dists[i];
      //LOG(INFO) << " DATA(NEUd) " << labels[i] << " " << fixed << setprecision(6)
      //   << NE(i,0) << " " << NE(i,1) << " " << RV(2) << " " << D(i);
   }

   // call the algorithm
   Vector<double> S1, S2;
   if(!ImplementRanging(NE, D, S1, S2)) return 0;

   // rotate back and remove the bias
   V(0) = S1(0); V(1) = S1(1); V(2) = (dim == 3 ? S1(2) : 0.0);
   RV = transpose(Rot)*V;
   S1.resize(3);
   S1(0) = RV(0) + xyz0[0];
   S1(1) = RV(1) + xyz0[1];
   S1(2) = RV(2) + xyz0[2];
   //V(0) = S2(0); V(1) = S2(1); V(2) = 0.0;
   //RV = transpose(Rot)*V;
   //S2.resize(3);
   //S2(0) = RV(0) + xyz0[0];
   //S2(1) = RV(1) + xyz0[1];
   //S2(2) = RV(2) + xyz0[2];

   // handle the case labels.size() == 2 .. some other data must resolve ambiguity
   // print and quit

   Point p("RAN", S1(0), S1(1), S1(2), 0,0,0,0,0,0);
   resp.push_back(p);
   LOG(INFO) << " Result(Ranging " << asString(labels.size())  << "): "
                  << p.asString();
   // TD print sigmas

   return iret;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int DefaultAlgorithm(const string lab, vector<Point>& resp) throw(Exception)
{
try {
   GlobalData& GD=GlobalData::Instance();

   // compute the centroid of the Points
   int n(0);
   Triple xyz0(0,0,0);
   map<string,Point>::iterator it;
   for(it=GD.Points.begin(); it!=GD.Points.end(); ++it) {
      if(it->second.isUnknown()) continue;
      xyz0[0] += (it->second.x - xyz0[0])/(n+1);
      xyz0[1] += (it->second.y - xyz0[1])/(n+1);
      xyz0[2] += (it->second.z - xyz0[2])/(n+1);
      n++;
   }

   Point p(lab, xyz0[0], xyz0[1], xyz0[2], 0,0,0,0,0,0);
   resp.push_back(p);
   LOG(INFO) << " Result(Center-of-mass): " << p.asString();

   return 1;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
void EstimateUnknownPoint(const string lab, vector<Point>& resp) throw(Exception)
{
try {
   map<string,Point>::iterator it;
   GlobalData& GD=GlobalData::Instance();

   it = GD.Points.find(lab);
   if(it == GD.Points.end()) GNSSTK_THROW(Exception("Unknown Point label " + lab));

   // average results
   bool first(true);
   double biasx,biasy,biasz;
   Stats<double> stx,sty,stz;
   for(int i=0; i<resp.size(); i++) {
      if(first) {
         biasx = resp[i].x;
         biasy = resp[i].y;
         biasz = resp[i].z;
         first = false;
      }
      stx.Add(resp[i].x - biasx);
      sty.Add(resp[i].y - biasy);
      stz.Add(resp[i].z - biasz);
   }
   it->second.x = biasx + stx.Average();
   it->second.y = biasy + sty.Average();
   it->second.z = biasz + stz.Average();
   it->second.setEstimate();

   ostringstream oss;
   oss << " Result of a priori fixing: \""
      << it->second.asString(GD.linprecP,GD.linwidth)
      << "\" with " << stx.N() << " estimate";
   if(stx.N() > 1) oss << "s, sigmas " << scientific << setprecision(2)
      << stx.StdDev() << " " << sty.StdDev() << " " << stz.StdDev();
   LOG(INFO) << oss.str();

   // warning if sigma are too big
   static const double limit(10.0);        // meters - arbitrary
   if(stx.StdDev() > limit || sty.StdDev() > limit || stz.StdDev() > limit)
      LOG(WARNING) << " Warning - abnormally large variation in a priori estimates"
         << " of position "<< it->second.label << ".";
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
// Given known Points A,B HAN(UAB) and distance AU to unknown Point U, find U.
//
// Note that HANs are always positive and measured in the clockwise sense;
// a negative HAN should be replaced with TwoPi+HAN.
//    HAN(UAB) + HAN(BAU) = TwoPi      // identity - order of UAB,BAU matters
// NB HAN is the difference of two azimuths (order matters):
//    HAN(UAB) = AZM(AB)-AZM(AU)       // definition HAN = difference of 2 AZM
//    AZM(AB) = AZM(BA) + Pi           // identity - mod 2Pi of course
//
// Rotate to NEU using A, form vect B-A, compute AB(horiz) and azimuth of A->B = azAB.
// Then find the azimuth of AU via
//    AZM(AU) = AZM(AB)-HAN(UAB)
// Now U(N) = A(N) + AU*cos(AZM(AU)) and U(E) = A(E) + AU*sin(AZM(AU))
// Rotate U back into XYZ
//
// Notes: Point A takes a special role b/c thats where the rotation is done.
//        Height(U) is underdetermined, but best guess is Ht(A) + Ht(B-A)/2.
// 
//    N           U
//    ^          /c\
//    |         /   \
//    |        /     \                  recall azimuth = 0 at North, + clockwise
//    |       /       \
//    |      /         \
//    |     /           \               azBU is almost 2Pi here
//    |azAU/             \
//    |   /               \
//    |  /                 \
//    | /                   \
//    |/a                   b\          azAB is ~Pi/2 here
//    A---------------------- B --> E
//
Point ImplementHANVectorAdd(const Point& A, const Point& B,
                            const double hanUAB, const double AU)
   throw(Exception)
{
try {
   Point U;

   Matrix<double> Rot(A.getRotation());
   double dht;
   double azAU = computeAZM(A, B, hanUAB, dht);

   // now use AU, azAU and A to get U
   Vector<double> AUneu(3),AUxyz;
   AUneu[0] = AU * ::cos(azAU);
   AUneu[1] = AU * ::sin(azAU);
   AUneu[2] = 0.5*dht;
   AUxyz = transpose(Rot)*AUneu;

   U = Point("UNK", A.x+AUxyz[0], A.y+AUxyz[1], A.z+AUxyz[2], 0,0,0,0,0,0);

   return U;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Given a known Point A, azimuth AU and distance AU to unknown Point U, find U.
// Rotate to NEU using A. Then
//    U(N) = A(N) + AU*cos(AZM(AU))
//    U(E) = A(E) + AU*sin(AZM(AU))
// Rotate U back to XYZ
//
//    N           U
//    ^          /
//    |         /
//    |        /
//    |       /
//    |      / AU
//    |     /
//    |azAU/
//    |   /
//    |  /
//    | /
//    |/
//    A------------------------> E
//
Point ImplementAZMVectorAdd(const Point& A, const double azAU, const double AU)
   throw(Exception)
{
try {
   Point U;

   // Compute AU in NEU frame
   Vector<double> AUneu(3),AUxyz;
   AUneu[0] = AU * ::cos(azAU);
   AUneu[1] = AU * ::sin(azAU);
   AUneu[2] = 0.0;

   // Rotate into XYZ at A
   Matrix<double> Rot(A.getRotation());
   AUxyz = transpose(Rot)*AUneu;

   // U = A + AUxyz;
   U = Point("UNK", A.x+AUxyz[0], A.y+AUxyz[1], A.z+AUxyz[2], 0,0,0,0,0,0);

   return U;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Given two Points A and B, and two different horizontal angles between A,B and a
// third, unknown Point U, use the law of sines to find U.
//
// Note that HANs are always positive and measured in the clockwise sense;
// a negative HAN should be replaced with TwoPi+HAN.
//    HAN(UAB) + HAN(BAU) = TwoPi      // identity - order of UAB,BAU matters
// NB HAN is the difference of two azimuths (order matters):
//    HAN(UAB) = AZM(AB)-AZM(AU)       // definition HAN = difference of 2 AZM
//    AZM(AB) = AZM(BA) + Pi           // identity - mod 2Pi of course
//
// Given two HANs and two known Points in a triangle, the third Point can be found.
// There are two cases: 1) HAN(UAB),HAN(UBA) and 2) HAN(UAB),HAN(AUB).
// (Note that with the above identities you can easily switch To and From in HAN.)
//
// Given the two angles, get them into the form UAB and UBA or AUB using identities.
// The acute angles are then just a=HAN(UAB) and b=HAN(UBA) or c=HAN(AUB),
//   where in each case if the angle is > Pi, replace it with TwoPi-angle.
// Sum of angles of a triangle is PI, so a + b + c = PI gives you the third angle c|b.
// There is poor geometry, and no good solution, if any angle is close to 0 or PI.
// Rotate to NEU using A, form vect B-A, compute AB(horiz) and azimuth of A->B = azAB.
// The Law of sines then gives AU = ABsin(b)/sin(c) and BU = ABsin(a)/sin(c).
// Then find the azimuth of AU and BU via
//    AZM(AU) = AZM(AB)-HAN(UAB)
//    AZM(BU) = AZM(BA)-HAN(UBA) = (Pi+AZM(AB))-HAN(UBA)
// Now U(N) = A(N) + AU*cos(AZM(AU))  ;  U(N) = B(N) + BU*cos(AZM(BU))
// and U(E) = A(E) + AU*sin(AZM(AU))  ;  U(E) = B(E) + BU*sin(AZM(BU))
//
// Notes: Point A takes a special role b/c thats where the rotation is done.
//        Height(U) is underdetermined, but best guess is Ht(A) + Ht(B-A)/2.
// 
//    N           U
//    ^          /c\
//    |         /   \
//    |        /     \                  recall azimuth = 0 at North, + clockwise
//    |       /       \
//    |      /         \
//    |     /           \               azBU is almost 2Pi here
//    |azAU/             \
//    |   /               \
//    |  /                 \
//    | /                   \
//    |/a                   b\          azAB is ~Pi/2 here
//    A---------------------- B --> E
//
Point ImplementLawSines(const Point& A, const Point& B, double hanUAB, double hanUBA,
         bool is2D) throw(Exception)
{
try {
   // limit on angles near 0 or Pi - poor geometry
   static const double limit(1.0 * ::DEG_TO_RAD);
   Point U;

   //LOG(INFO) << " ImplLawSines: A is " << A.label << " and B is " << B.label;
   //LOG(INFO) << " ImplLawSines: hanUAB is " << hanUAB << " radians, and hanUBA is "
   //   << hanUBA << " radians.";
   //LOG(INFO) << " ImplLawSines: hanUAB is " << angleAsDMSstring(hanUAB)
   //                   << " and hanUBA is " << angleAsDMSstring(hanUBA);

   // get angles a, b, c
   double a = hanUAB;
   if(a > Pi) a = TwoPi - a;
   double b = hanUBA;
   if(b > Pi) b = TwoPi - b;
   double c = Pi - a - b;

   // if any angle is small or nearly Pi, geometry is too poor (triangle is squashed).
   if(c < limit || Pi-c < limit) return U;
   if(a < limit || Pi-a < limit) return U;
   if(b < limit || Pi-b < limit) return U;

   // compute A-B
   Vector<double> ABxyz(3),ABneu;
   ABxyz[0] = B.x - A.x;
   ABxyz[1] = B.y - A.y;
   ABxyz[2] = B.z - A.z;

   // rotate into NEU at A
   Matrix<double> Rot(A.getRotation());
   ABneu = Rot * ABxyz;

   if(is2D) {
      ABneu[0] = B.y - A.y;
      ABneu[1] = B.x - A.x;
      ABneu[2] = 0.0;
   }

   // compute azimuth(AB) and horizontal distance AB
   double azAB(azimuth(ABneu[1],ABneu[0]));
   double AB(::sqrt(ABneu[0]*ABneu[0] + ABneu[1]*ABneu[1]));
   //LOG(INFO) << " TSA: A->B" << fixed << setprecision(3)
   //   << " (" << ABneu[0] << "," << ABneu[1] << "," << ABneu[2] << ")"
   //   << " distAB " << AB << " azmAB " << angleAsDMSstring(azAB);

   // use law of sines to solve for AU and Bu
   double AU = AB * ::sin(b) / ::sin(c);
   double BU = AB * ::sin(a) / ::sin(c);

   // compute the azimuth of AU and BU (don't mind mod 2Pi)
   // HAN(UAB) = AZM(AB)-AZM(AU)
   double azAU = azAB - hanUAB;
   // HAN(UBA) = AZM(BA)-AZM(BU) = AZM(AB) - Pi - AZM(BU)
   double azBU = azAB - Pi - hanUBA;

   // now use AU, azAU and A to get U
   Vector<double> AUneu(3),AUxyz;
   AUneu[0] = AU * ::cos(azAU);     // north
   AUneu[1] = AU * ::sin(azAU);     // east
   AUneu[2] = 0.5*ABneu[2];         // just for fun
   AUxyz = transpose(Rot)*AUneu;

   //LOG(INFO) << " TSA: A->U (" << fixed << setprecision(3)
   //   << AUneu[0] << "," << AUneu[1] << "," << AUneu[2] << ")"
   //   << " distAU " << AU << " azmAU " << angleAsDMSstring(azAU);

   if(is2D) {
      U = Point("UNK", A.x+AUneu[1], A.y+AUneu[0], A.z, 0,0,0,0,0,0);
   }
   else {
      U = Point("UNK", A.x+AUxyz[0], A.y+AUxyz[1], A.z+AUxyz[2], 0,0,0,0,0,0);
   }
   U.setEstimate();

   // TD compute an expected error, choose AU or BU
   //Vector<double> BUneu(3),BUxyz;
   //BUneu[0] = BU * ::cos(azBU);     // north
   //BUneu[1] = BU * ::sin(azBU);     // east
   //BUneu[2] = 0.5*ABneu[2];         // just for fun
   //BUxyz = transpose(Rot)*BUneu;
   //LOG(INFO) << " TSA: B->U (" << fixed << setprecision(3)
   //   << BUneu[0] << "," << BUneu[1] << "," << BUneu[2] << ")"
   //   << " distBU " << BU << " azmBU " << angleAsDMSstring(azBU);

   //Point UB("UnkB", B.x+BUxyz[0], B.y+BUxyz[1], B.z+BUxyz[2], 0,0,0,0,0,0);
   //LOG(INFO) << " Result(Triangulation " << A.label << "-" << B.label << "-UNKN): "
   //         << UB.asString();

   return U;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Given POS A,B and AZM AU,BU, two lines are defined that must meet at a point = U.
// Rotate to NEU using A, form vect B-A, compute AB(horiz) and azimuth of A->B = azAB.
//    HAN(UAB) = AZM(AB)-AZM(AU)       // definition HAN = difference of 2 AZM
//    HAN(ABU) = AZM(BU)-AZM(BA).      // same definition
//    AZM(AB) = AZM(BA) + Pi           // identity - mod 2Pi of course
// From these get HAN(UAB) and HAN(ABU), then call ImplementLawSines()
//    N           U
//    ^          /c\                  recall azimuth = 0 at North, + clockwise
//    |         /   \
//    |        /     \2pi-azBU|
//    |       /       \       |
//    |      /         \      |
//    |     /           \     |         azBU is almost 2Pi here
//    |azAU/             \    |
//    |   /               \   |
//    |  /                 \  |
//    | /                   \ |
//    |/a                   b\|         azAB is ~Pi/2 here
//    A---------------------- B --> E
//
Point ImplementTwoAzimuths(const Point& A, const Point& B, double azAU, double azBU)
   throw(Exception)
{
   static const double limit(1.0 * ::DEG_TO_RAD);

   // here is the 3-D triangulation algorithm: given A,B,hanUAB,hanABU: find U
   // sum angles of a triangle => HAN(AUB) = PI - HAN(UAB) - HAN(ABU)
   // law of sines gives you sin(AUB)/AB = sin(ABU)/AU
   // with AU and AZM(AU) use vector addition to get U = A + vector(AU,AZM(AU))

   // compute A-B and rotate into NEU at A
   Vector<double> ABXYZ(3),ABNEU;
   ABXYZ[0] = B.x - A.x;
   ABXYZ[1] = B.y - A.y;
   ABXYZ[2] = B.z - A.z;
   Matrix<double> Rot(A.getRotation());
   ABNEU = Rot * ABXYZ;
   //LOG(INFO) << "NEU for " << B.label << "-" << A.label << fixed << setprecision(3)
   //   << " " << ABNEU[0] << " " << ABNEU[1] << " " << ABNEU[2];

   // compute azimuth(AB) and horizontal distance AB
   double azAB(azimuth(ABNEU[1],ABNEU[0]));
   double AB(::sqrt(ABNEU[0]*ABNEU[0] + ABNEU[1]*ABNEU[1]));
   //LOG(INFO) << "   ImplementTwoAzimuths has AZM(" << A.label << "-" << B.label
   //   << ") = " << angleAsDMSstring(azAB);

   // compute horizontal angles
   double hanUAB(TwoPi - (azAU - azAB));
   while(hanUAB < 0.0) hanUAB += TwoPi;
   while(hanUAB >= TwoPi) hanUAB -= TwoPi;
   if(hanUAB > Pi) hanUAB = TwoPi - hanUAB;        // interior angle of triangle
   //LOG(INFO) << "   ImplementTwoAzimuths has HAN(U-" << A.label << "-" << B.label
   //   << ") = " << angleAsDMSstring(hanUAB);

   double hanABU(azBU - (Pi + azAB));
   while(hanABU < 0.0) hanABU += TwoPi;
   while(hanABU >= TwoPi) hanABU -= TwoPi;
   if(hanABU > Pi) hanABU = TwoPi - hanABU;        // interior angle of triangle
   //LOG(INFO) << "   ImplementTwoAzimuths has HAN(" << A.label << "-" << B.label
   //   << "-U) = " << angleAsDMSstring(hanABU);

   // solve for AU
   double hanAUB = Pi - hanUAB - hanABU;     // sum angles of triangle = Pi
   while(hanAUB < 0.0) hanAUB += TwoPi;
   while(hanAUB >= TwoPi) hanAUB -= TwoPi;
   if(hanAUB > Pi) hanAUB = TwoPi - hanAUB;        // interior angle of triangle
   //LOG(INFO) << "   ImplementTwoAzimuths has HAN(" << A.label << "-U-" << B.label
   //   << ") = " << angleAsDMSstring(hanAUB) << " critical angle, with distance "
   //   << fixed << setprecision(2) << AB;

   // large error when AUB is close to 0 or Pi - flat triangle
   if(hanAUB < limit || Pi-hanAUB < limit) return Point();

   double AU = ::fabs(AB * ::sin(hanABU) / ::sin(hanAUB));
   //LOG(INFO) << "   ImplementTwoAzimuths has sin(hanAUB) = "
   //   << fixed << setprecision(9) << ::sin(hanAUB);
   //LOG(INFO) << "   ImplementTwoAzimuths has DIS(" << A.label << "-U) = " << AU;

   // now use AU, azAU and A to get U
   Vector<double> AUNEU(3),AUXYZ;
   AUNEU[0] = AU * ::cos(azAU);
   AUNEU[1] = AU * ::sin(azAU);
   AUNEU[2] = 0.5*ABNEU[2];         // just for fun
   //LOG(INFO) << "NEU for U-" << A.label << fixed << setprecision(3)
   //   << " " << AUNEU[0] << " " << AUNEU[1] << " " << AUNEU[2];
   AUXYZ = transpose(Rot)*AUNEU;

   Point p("2AZ", A.x + AUXYZ[0], A.y + AUXYZ[1], A.z + AUXYZ[2], 0,0,0,0,0,0);

   return p;
}

//------------------------------------------------------------------------------------
// Solve the 3-point resection problem, including the case when one of the angles
// is zero. See the discussion and references in the file description.
// Note that BOTH angles zero is a singular problem.
// param EN Reference to Matrix<double> of dimension 4,2 containing the East,North
//  components of points A, B, C, and U; on output the U row (3) contains solution.
// param hanAUB horizontal angle A-U-B in radians, 0 <= hanAUB < Pi
// param hanBUC horizontal angle B-U-C in radians, 0 <= hanBUC < Pi
// return true if the algorithm succeeded.      // TD never returns false
// throw on std exception or if singular (should never happen)
bool ImplementResection(Matrix<double>& EN, const double hanAUB, const double hanBUC)
   throw(Exception)
{
try {
   unsigned i,j;

   static const double limit(1.e-12);     // limit on tan(angles)
   const double tana(::tan(hanAUB));
   const double tanb(::tan(hanBUC));
   // this should never happen, ResectionAlgorithm() should prevent it
   if(::fabs(tana) < limit && ::fabs(tanb) < limit)
      GNSSTK_THROW(Exception("Both angles 0 or pi"));

   double cota(0), cotb(0);
   if(::fabs(tana) > limit) cota = 1.0/tana;
   if(::fabs(tanb) > limit) cotb = 1.0/tanb;

   const double A1(-EN(0,0)-EN(1,0));
   const double A2(-EN(0,1)-EN(1,1));
   const double A3(EN(0,0)*EN(1,0)+EN(0,1)*EN(1,1));
   const double A4(EN(0,1)-EN(1,1));
   const double A5(EN(1,0)-EN(0,0));
   const double A6(EN(0,0)*EN(1,1)-EN(1,0)*EN(0,1));

   const double B1(-EN(1,0)-EN(2,0));
   const double B2(-EN(1,1)-EN(2,1));
   const double B3(EN(1,0)*EN(2,0)+EN(1,1)*EN(2,1));
   const double B4(EN(1,1)-EN(2,1));
   const double B5(EN(2,0)-EN(1,0));
   const double B6(EN(1,0)*EN(2,1)-EN(2,0)*EN(1,1));

   const double a(A1-A4*cota);
   const double b(A2-A5*cota);
   const double d(B1-B4*cotb);
   const double e(B2-B5*cotb);

   //LOG(INFO) << " A1 " << fixed << setprecision(5) << A1;
   //LOG(INFO) << " A2 " << fixed << setprecision(5) << A2;
   //LOG(INFO) << " A3 " << fixed << setprecision(5) << A3;
   //LOG(INFO) << " A4 " << fixed << setprecision(5) << A4;
   //LOG(INFO) << " A5 " << fixed << setprecision(5) << A5;
   //LOG(INFO) << " A6 " << fixed << setprecision(5) << A6;
   //LOG(INFO) << " B1 " << fixed << setprecision(5) << B1;
   //LOG(INFO) << " B2 " << fixed << setprecision(5) << B2;
   //LOG(INFO) << " B3 " << fixed << setprecision(5) << B3;
   //LOG(INFO) << " B4 " << fixed << setprecision(5) << B4;
   //LOG(INFO) << " B5 " << fixed << setprecision(5) << B5;
   //LOG(INFO) << " B6 " << fixed << setprecision(5) << B6;
   //LOG(INFO) << " cota " << scientific << setprecision(3) << cota;
   //LOG(INFO) << " cotb " << scientific << setprecision(3) << cotb;
   //LOG(INFO) << " tana " << scientific << setprecision(3) << tana;
   //LOG(INFO) << " tanb " << scientific << setprecision(3) << tanb;
   //LOG(INFO) << " a " << fixed << setprecision(5) << a;
   //LOG(INFO) << " b " << fixed << setprecision(5) << b;
   //LOG(INFO) << " d " << fixed << setprecision(5) << d;
   //LOG(INFO) << " e " << fixed << setprecision(5) << e;

   // singular case - straight line
   // the "semi-singular" case is when one angle is zero.
   //                             C
   //                            /
   //                           /
   //                          /
   //                         /
   //                        /
   //    AUB=0         BUC  /
   // A---------B----------U
   if(::fabs(tana) < limit) {                             // AUB must be 0
      //LOG(INFO) << "Resection semi-singular case (a)";
      //if(::fabs(hanAUB - Pi) < limit)
      //   GNSSTK_THROW(Exception("AUB is Pi; switch angles and call again"));

      if(::fabs(A4) > ::fabs(A5)) {
         EN(3,1) = -(2.0*A6*A5+e*A4*A4-d*A5*A4)/(A4*A4+A5*A5) - EN(1,1);
         EN(3,0) = -(A5*EN(3,1)+A6)/A4;
      }
      else {
         EN(3,0) = -(2.0*A4*A6+d*A5*A5-e*A4*A5)/(A4*A4+A5*A5) - EN(1,0);
         EN(3,1) = -(A4*EN(3,0)+A6)/A5;
      }
   }

   else if(::fabs(tanb) < limit) {                         // BUC must be 0
      //LOG(INFO) << "Resection semi-singular case (b)";
      //if(::fabs(hanBUC - Pi) < limit)
      //   GNSSTK_THROW(Exception("BUC is Pi; switch angles and call again"));

      if(::fabs(B4) > ::fabs(B5)) {
         EN(3,1) = -(2.0*B6*B5+b*B4*B4-a*B5*B4)/(B4*B4+B5*B5) - EN(1,1);
         EN(3,0) = -(B5*EN(3,1)+B6)/B4;
      }
      else {
         EN(3,0) = -(2.0*B4*B6+a*B5*B5-b*B4*B5)/(B4*B4+B5*B5) - EN(1,0);
         EN(3,1) = -(B4*EN(3,0)+B6)/B5;
      }
   }

   else {            // regular case - neither angle is zero or pi
      const double c(A3-A6*cota);
      const double f(B3-B6*cotb);
      const double A(a-d);
      const double B(b-e);
      const double C(c-f);
      const double D(A*A+B*B);
      //LOG(INFO) << " c " << fixed << setprecision(5) << c;
      //LOG(INFO) << " f " << fixed << setprecision(5) << f;
      //LOG(INFO) << " A " << fixed << setprecision(5) << A;
      //LOG(INFO) << " B " << fixed << setprecision(5) << B;
      //LOG(INFO) << " C " << fixed << setprecision(5) << C;
      //LOG(INFO) << " D " << fixed << setprecision(5) << D;

      // should never happen, ResectionAlgorithm() should prevent it
      // but A or B can be zero
      if(D < limit) GNSSTK_THROW(Exception("Singular resection"));

      if(::fabs(A) > ::fabs(B)) {
         //LOG(INFO) << "Resection regular case (c)A";
         EN(3,1) = -(2.0*C*B+b*A*A-a*B*A)/D - EN(1,1);
         EN(3,0) = -(B*EN(3,1)+C)/A;
      }
      else {
         //LOG(INFO) << "Resection regular case (c)B";
         EN(3,0) = -(2.0*C*A+a*B*B-b*A*B)/D - EN(1,0);
         EN(3,1) = -(A*EN(3,0)+C)/B;
      }
   }

   return true;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Can do N-ranging with a Bancroft-like algorithm
// Put N control points at Si, unknown at X, and distances (Si,X) = di
// di^2 = X^T*X - 2Si^T*X + Si^T*Si
// call lam = (X^T*X)/2 an unknown constant
// Si^T*X = lam + ri   where ri = (Si^T*Si - di^2)/2
// call A = [ -- S1 -- ]
//          [ -- S2 -- ]
//          [ -- S3 -- ]
//          [ -- SN -- ]
// then A*X = lam*(vec of 1's) + vec(ri)
// let B = (A^T*A)^-1 * A^T = inverse of A; then
// X = lam*W + V   where W = B*(1vec) and V = B*vec(ri)
// square this
// X^T*X = lam^2 W^T*W + 2*lam*W^T*V + V^T*V
// but X^T*X = 2*lam, so
// 0 = e*lam^2 + 2f*lam + g   where e=W^T*W  f= (W^T*V - 1)  g = V^T*V
// Solve this for lam1, lam2 using lam = [-2f +- sqrt(4f^2-4eg)]/2e
// Then X = lam*W + V
// Resolve the ambiguity (lam1 or lam2) by (if N>dim) substituting back into di eqn
// or (if n==dim) some other method.
//
// Return the two solutions, "best" one first.
// TD implement numerical limits type tests on the residuals and return some
//    indication of the quality of the solution
//
bool ImplementRanging(const Matrix<double>& XYZ, const Vector<double>& D,
                      Vector<double>& S1, Vector<double>& S2)
   throw(Exception)
{
try {
   unsigned int i,j;

   // TD check dimensions, etc
   const unsigned int N(XYZ.rows()), M(XYZ.cols());

   //Matrix<double> XYZD(XYZ || D);
   //LOG(INFO) << " Ranging Input:\n" << fixed << setprecision(2) << setw(9) << XYZD;

   // invert XYZ
   Matrix<double> B = inverseLUD(transpose(XYZ)*XYZ) * transpose(XYZ);
   //LOG(INFO) << " Ranging B:\n" << fixed << setprecision(2) << setw(9) << B;

   // construct the W and V Vectors
   Vector<double> W(N,1.0),V(N,0.0);
   for(i=0; i<N; i++) {
      for(j=0; j<M; j++)
         V(i) += XYZ(i,j)*XYZ(i,j)/2.0;
      V(i) -= D(i)*D(i)/2.0;              // r vector
   }
   W = B * W;
   V = B * V;
   //LOG(INFO) << " Ranging W:\n" << fixed << setprecision(2) << setw(9) << W;
   //LOG(INFO) << " Ranging V:\n" << fixed << setprecision(2) << setw(9) << V;

   // form the quadratic equation eL^2 + 2fL + g = 0
   double e = dot(W,W);
   double f = dot(W,V)-1.0;
   double g = dot(V,V);
   //LOG(INFO) << "\nRangingAlgorithm: e f g are "
   //   << scientific << setprecision(3) << e << " " << f << " " << g;

   // solve the quadratic equation
   double lam1,lam2,tmp;
   tmp = f*f-e*g;
   if(tmp < 0.0) {
      LOG(WARNING) << " Warning - ranging algorithm finds singular problem "
         << scientific << setprecision(3) << tmp;
      return false;
   }
   tmp = ::sqrt(tmp);
   lam1 = (-f + tmp) / e;     // -b +- sqrt(b^2-4ac) / 2a which is
   lam2 = (-f - tmp) / e;     // -2f +- sqrt(4f^2-4eg) / 2e
   //LOG(INFO) << "RangingAlgorithm: lam1 and lam2 are "
   //   << scientific << setprecision(3) << lam1 << " " << lam2 << endl;

   // form the solutions
   S1 = lam1 * W + V;
   S2 = lam2 * W + V;

   // compute residuals - use this to determine best solution
   Vector<double> T;
   W.resize(N);
   V.resize(N);
   for(i=0; i<N; i++) {
      T = XYZ.rowCopy(i) - S1;
      W(i) = D(i)*D(i) - dot(T,T);
      T = XYZ.rowCopy(i) - S2;
      V(i) = D(i)*D(i) - dot(T,T);
   }
   double res1 = RMS(W);
   double res2 = RMS(V);

   // make S1 the better solution based on residuals
   if(res1 != res1 || res2 < res1) {         // test for nan == res1
      T = S1; S1 = S2; S2 = T;
      tmp = res1; res1 = res2; res2 = tmp;
   }

   //LOG(INFO) << "RangingAlgorithm: Sol1 " << scientific << setprecision(3) << S1
   //   << " with resid " << res1;
   //LOG(INFO) << "RangingAlgorithm: Sol2 " << scientific << setprecision(3) << S2
   //   << " with resid " << res2;

   // TD test for errors ~ numerical limits, and estimate quality of resolution

   return true;
}
catch(exception &e) { GNSSTK_THROW(Exception("std::exception : "+string(e.what()))); }
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

/*
 * Plan on calling this method before the Center of Mass algorithm iff
 * the number of unknown points at that time is non-zero
 *
 * unknownPoints will be modified throughout the method if a point is
 * estimated as part of a leveling chain. After method execution, Center of Mass
 * should not be applicable unless there were data entries in the map that could
 * not be estimated in this method
*/

void checkForLeveling(std::map<string, Point> &unknownPoints)
{
    GlobalData &GD = GlobalData::Instance();

    bool continueCheck = true;

    while(continueCheck)
    {
        continueCheck = false;

        // Iterate through list of points
        for(std::map<string, Point>::iterator it = GD.Points.begin();
            it != GD.Points.end(); ++it)
        {
            if(it->second.isUnknown())
            {
                continue;
            }

            string fromLabel = it->first;
            levelChain trialChain = givelevelChain(fromLabel);
            unsigned int levelChainSize = trialChain.size();
            if(levelChainSize > 1)
            {
                // At least one set of level chain will be constructed so set this flag
                // to true so it will iterate through again
                continueCheck = true;

                // Gather the name of the first from label and the last to label
                // Corresponds to known / estimated Point data
                string firstFromLabel = trialChain[0].From;
                string lastToLabel = trialChain[levelChainSize - 1].To;

                // Find them in the map
                std::map<string, Point>::iterator firstFromIter = GD.Points.find(firstFromLabel);
                std::map<string, Point>::iterator lastToIter = GD.Points.find(lastToLabel);

                Vector<double> deltaVector(3), deltaRotated;

                // Set the delta vector
                deltaVector[0] = lastToIter->second.x - firstFromIter->second.x;
                deltaVector[1] = lastToIter->second.y - firstFromIter->second.y;
                deltaVector[2] = lastToIter->second.z - firstFromIter->second.z;

                // Get the delta vector in NEU components
                Matrix<double> Rot = firstFromIter->second.getRotation();
                deltaRotated = Rot * deltaVector; // Delta vector expressed in NEU components
                // Index 0 - N; Index 1 - E; Index 2 - U

                // Grab the unit vectors.
                Vector<double> N = Rot.rowCopy(0);
                Vector<double> E = Rot.rowCopy(1);

                // Nx, Ny, Nz
                double Nx = N[0] * deltaRotated[0];
                double Ny = N[1] * deltaRotated[0];
                double Nz = N[2] * deltaRotated[0];

                // Ex, Ey, Ez
                double Ex = E[0] * deltaRotated[1];
                double Ey = E[1] * deltaRotated[1];
                double Ez = E[2] * deltaRotated[1];

                for(unsigned int index = 1; index < levelChainSize; index++)
                {
                    double tmpPointX = firstFromIter->second.x + (double(index) / double(levelChainSize)) * (Nx + Ex);
                    double tmpPointY = firstFromIter->second.y + (double(index) / double(levelChainSize)) * (Ny + Ey);
                    double tmpPointZ = firstFromIter->second.z + (double(index) / double(levelChainSize)) * (Nz + Ez);

                    std::map<string, Point>::iterator tmpPointIter = GD.Points.find(trialChain[index].From);
                    tmpPointIter->second.x = tmpPointX;
                    tmpPointIter->second.y = tmpPointY;
                    tmpPointIter->second.z = tmpPointZ;

                    tmpPointIter->second.setEstimate();
                    tmpPointIter->second.setConstraintAxis("NE");

                    unknownPoints.erase(trialChain[index].From);
                }
            }
        }

    }

}

void checkForSideShots(std::map<string, Point> &unknownPoints)
{
    GlobalData &GD = GlobalData::Instance();

    bool continueCheck = true;

    while(continueCheck)
    {
        continueCheck = false;

        // Iterate through list of points
        for(std::map<string, Point>::iterator it = GD.Points.begin();
            it != GD.Points.end(); ++it)
        {
            if(it->second.isUnknown())
            {
                continue;
            }

            string fromLabel = it->first;
            std::vector<Height> allHeights_wFrom = findAllHeightRecords(fromLabel, GD);
            for(std::vector<Height>::const_iterator heightIter = allHeights_wFrom.begin();
                heightIter != allHeights_wFrom.end(); ++heightIter)
            {
                std::map<string, Point>::iterator toPoint_iter = GD.Points.find(heightIter->To);
                if(toPoint_iter == GD.Points.end())
                {
                    continue;
                }

                else if(toPoint_iter->second.isUnknown())
                {
                    continueCheck = true;

                    // Set the unknown side shot coordinates to be the same as the known point
                    toPoint_iter->second.x = it->second.x;
                    toPoint_iter->second.y = it->second.y;
                    toPoint_iter->second.z = it->second.z;

                    toPoint_iter->second.setEstimate();
                    toPoint_iter->second.setConstraintAxis("NE");

                    unknownPoints.erase(toPoint_iter->first);
                }
            }

        }
    }
}

levelChain givelevelChain(const std::string &startFromLabel)
{
    GlobalData &GD = GlobalData::Instance();

    std::vector<levelChain> allPaths = findAllChains(startFromLabel);

    bool initialized = false;
    double tmpDistance, minDistance;

    levelChain chosenChain;

    for(std::vector<levelChain>::const_iterator allPaths_iter = allPaths.begin();
        allPaths_iter != allPaths.end(); ++allPaths_iter)
    {
        // Verify that the final to station is an estimated / known point
        levelChain::const_reverse_iterator lastMeas_iter = allPaths_iter->rbegin();
        std::string toStationLabel = lastMeas_iter->To;

        std::map<std::string, Point>::iterator fromPointIter = GD.Points.find(startFromLabel);
        std::map<std::string, Point>::iterator toPointIter = GD.Points.find(toStationLabel);
        if(fromPointIter == GD.Points.end() || toPointIter == GD.Points.end() )
        {
            continue;
        }

        // The last to point needs to be known / estimated. There needs to be more than one
        // measurements associated with the chain
        if(allPaths_iter->size() < 2 || toPointIter->second.isUnknown() )
        {
            continue;
        }

        // Calculate the distance between two points
        tmpDistance = fromPointIter->second.distanceTo(toPointIter->second);
        if(!initialized)
        {
            minDistance = tmpDistance;
            chosenChain = *allPaths_iter;
            initialized = true;
        }

        // Search for the point with the minimum distance. If two or more points share this distance
        // select the chain with the smallest number of measurements.
        else
        {
            if( std::abs(tmpDistance - minDistance) < lsa::ZERO_BOUND)
            {
                if(allPaths_iter->size() < chosenChain.size())
                {
                    minDistance = tmpDistance;
                    chosenChain = *allPaths_iter;
                }
            }

            else if(tmpDistance < minDistance)
            {
                minDistance = tmpDistance;
                chosenChain = *allPaths_iter;
            }
        }

    }

    return chosenChain;
}

vector<levelChain> findAllChains(const string &startFromLabel)
{
    vector<levelChain> allChains;
    GlobalData &GD = GlobalData::Instance();

    vector<Height> vectHeightRecords = findAllHeightRecords(startFromLabel, GD);

    // Exit early if the starting label does not have any associated height records
    if(vectHeightRecords.size() == 0)
    {
        return allChains;
    }

    // Initialize the set of chains
    allChains.resize(vectHeightRecords.size());
    for(unsigned int heightIndex = 0; heightIndex < vectHeightRecords.size();
        heightIndex++)
    {
        allChains[heightIndex].push_back(vectHeightRecords[heightIndex]);
    }

    bool toStationKnown, noHeightRecords, circReference;

    /*
     * Iterate through the list of chains.
     * Finish off each chain individually and add copies
     * if one of the points in the chain has multiple HDIF records
     * associated with it.
    */

    for(std::vector<levelChain>::iterator chainIter = allChains.begin(); chainIter != allChains.end();
        ++chainIter)
    {
        /*
         * Work on each individual chain.
         * Grab the last Height record. Check a handful of conditions
         * to see if endChainCondition should be set to false
         * 1. To Station corresponds to a known / estimated point
         * 2. findAllHeightRecords(toStation) is empty
         * 3. You get a circular reference
         *
         * Any of those conditions should set continueChain to false
         */

        bool continueChain = true;
        // Keep track of where current iterator is with respect to begin iterator.
        // Important as chainIter will need to be reset if copy insertion occurs.

        int currentDistance = std::distance(allChains.begin(), chainIter);

        while(continueChain)
        {
            circReference = false;

            // Reference the last pair in the current chain
            levelChain::reverse_iterator lastLinkIter = chainIter->rbegin();

            string toStationLabel = lastLinkIter->To;

            // Check that To station has not appeared in the chain prior.
            // If it has flag as a circular reference
            for(levelChain::const_iterator currentChainIter = chainIter->begin();
                currentChainIter != chainIter->end(); ++currentChainIter)
            {
                if(currentChainIter->From == toStationLabel)
                {
                    circReference = true;
                    break;
                }
            }

            // Is toStationLabel associated with a known point
            std::map<string, Point>::iterator toLabelPoint_iter = GD.Points.find(toStationLabel);

            // Probably need to throw an exception or raise some warning if this happens.
            if(toLabelPoint_iter == GD.Points.end())
            {
                break;
            }

            if(toLabelPoint_iter->second.isUnknown() && !circReference)
            {
                toStationKnown = false;
                // Swap labels to progress down chain
                string newFromLabel = toStationLabel;
                vectHeightRecords = findAllHeightRecords(newFromLabel, GD);
                if(!vectHeightRecords.empty())
                {
                    noHeightRecords = false;
                    // If two or more Height records, this indicates a branch point.
                    // Insert n - 1 copies after current iterator position
                    if(vectHeightRecords.size() > 1)
                    {
                        allChains.insert(chainIter, vectHeightRecords.size() - 1, *chainIter);

                        // Reset the iterator position as it is no longer valid after calling insert
                        chainIter = std::next(allChains.begin(), currentDistance);
                    }

                    for(unsigned int heightIndex = 0; heightIndex < vectHeightRecords.size();
                        heightIndex++)
                    {
                        std::vector<levelChain>::iterator branchIter = std::next(chainIter, heightIndex);
                        branchIter->push_back(vectHeightRecords[heightIndex] );
                    }
                }

                // If this occurs then we have reached an end of the chain. An unknown point
                // that does not link to any other points (likely indicates a side shot point)
                else
                {
                    noHeightRecords = true;
                }
            }

            // Station is known / estimated. Don't need to proceed further with chain
            else
            {
                toStationKnown = true;
                noHeightRecords = true;
            }

            if(toStationKnown || noHeightRecords || circReference)
            {
                continueChain = false;
            }
        }
    }

    return allChains;
}

vector<Height> findAllHeightRecords(const string &fromLabel, GlobalData &gdInstance)
{
    std::vector<Height> recordsWithFrom;

    for(unsigned int index = 0; index < gdInstance.Measurements.size(); index++)
    {
        if(gdInstance.Measurements[index]->type() != DATtype::HGT)
        {
            continue;
        }

        // Index 0 - from label; Index 1 - to label
        std::vector<string> measLabels = gdInstance.Measurements[index]->allLabels();

        const Height& height(dynamic_cast<Height&>(*(gdInstance.Measurements[index]) ) );
        if(measLabels[0] == fromLabel)
        {
            recordsWithFrom.push_back(height);
        }
    }

    return recordsWithFrom;
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
