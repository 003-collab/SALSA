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
/// @file SigPtTransform.cpp  Generalized sigma point transform function.

//#include <cmath>
//#include <sstream>
//#include <iomanip>
//#include <limits.h>
//#ifdef _WIN32
//    #include <windows.h>//exe path
//    #include <tchar.h>
//#else
//    #include <unistd.h>//exe path
//#endif
//#include "StringUtils.hpp"
//#include "LSAConstants.hpp"
//#include "lsaUtils.hpp"

#include "SigPtTransform.hpp"

//using namespace std;
//using namespace gnsstk;

///------------------------------------------------------------------------------------
/// Employ sigma point transform to map covariance via nominal mapping function
/// input Eigen::VectorXd State                   nominal input state
/// input Eigen::MatrixXd Cov                     nominal input state covariance
/// input Eigen::VectorXd (*f)(Eigen::VectorXd)   pointer to mapping function that
///                                               expects a single Eigen::VectorXd input
///                                               and outputs a single Eigen::VectorXd
/// output Eigen::MatrixXd                        mapped covariance
Eigen::MatrixXd SigmaPointTransformFunc(Eigen::VectorXd State, Eigen::MatrixXd Cov,
                                        Eigen::VectorXd (*f)(Eigen::VectorXd))
{

   // dimesion of input state
   int InputDim = State.size();

   // determine dimension of mapped state
   Eigen::VectorXd NominalMappedState = (*f)(State);
   int MappedDim = NominalMappedState.size();

   // declare output mapped covariance object
   Eigen::MatrixXd MappedCov(MappedDim,MappedDim);
   MappedCov.setZero();

   double eps = 1e-30; // to handle semi-positive definite covariances

   // Compute covariance matrix "square root" by setting any negative or zero eigenvalues
   // (e.g. the up direction for one of the points is fixed) to super small positive values,
   // then recombining (if any eigenvalue changes were made) to get a new coveriance matrix,
   // and then taking cholesky decomposition.  The cholesky decomposition requires a positive
   // definite matrix to produce a unique solution, but is also stable for extremely small
   // eigenvalues. This approach allows us to determine the cholesky decomposition "in the
   // limit" as the eigenvalues approach zero (i.e. to the desired precision).

   // First create the eigensolver object
   Eigen::EigenSolver<Eigen::MatrixXd> es(Cov);

   // Extract eigenvalues as vector:
   Eigen::VectorXd EvalsV = es.eigenvalues().real();
   // Can also extract eigenvalues as matrix with values on diagonal:
   // Eigen::MatrixXd EvalsD = es.eigenvalues().real().asDiagonal();

   // Extract eigenvectors as matrix, with each column an eigenvector:
   Eigen::MatrixXd Evects = es.eigenvectors().real();

   // Check all eigenvalues, setting any small negative or zero values to super small
   // positive values
   // Initialize:
   bool RecombineCovFlag = false;

   for(int ct=0; ct<InputDim; ct++){
      // Set any negative evals to zero (they should be very small, if the cov is
      // reasonable)
      if (EvalsV(ct) <= 0.0) {
         RecombineCovFlag = true;
         EvalsV(ct) = eps;
      }
   }
   // Place eigenvalues on diagonal of matrix
   Eigen::MatrixXd EvalsDiag = EvalsV.asDiagonal();

   // if there was a negative or zero-value eigenvalue, then it was reset to a small
   // positive value and thus the covariance must be recombined and the result used
   // for the cholesky decomposition below
   if (RecombineCovFlag){
      // Save original covariance
      Eigen::MatrixXd CovOriginal = Cov;
      // Recombine the cov to get a non-singular covariance
      Cov = Evects * EvalsDiag * Evects.transpose();
      // If there were zero-value off-diagonal elements in CovOriginal that are now very small,
      // they should still be zero (they became non-zero due to numerical noise)
      // Loop over all off-diagonal elements
      for(int i=0; i<InputDim; i++){
          for(int j=0; j<InputDim; j++){
              if(i!=j){
                  if(CovOriginal(i,j)==0 && fabs(Cov(i,j))<std::pow(eps,0.5)){
                      Cov(i,j) = 0;
                  }
              }
          }
      }
   }

   // Compute Cholesky decomposition
   // Example:
   //   LLT<MatrixXd> lltOfA(A); // compute the Cholesky decomposition of A
   //   MatrixXd L = lltOfA.matrixL(); // retrieve factor L  in the decomposition
   //   // The previous two lines can also be written as "L = A.llt().matrixL()"
   Eigen::MatrixXd CholCovLowerEigen = Cov.llt().matrixL();

   // Check for any diagonal values equal to the square root of the above super small
   // value used, set those back to zero
   for(int ct=0; ct<InputDim; ct++){
      if (CholCovLowerEigen(ct,ct) == pow(eps,0.5)) {
         CholCovLowerEigen(ct,ct) = 0.0;
      }
   }

   // Generate initial sigma points
   Eigen::MatrixXd InitSigPts(InputDim,InputDim*2);
   for(int j=0; j<InputDim; j++) { //columns
       for(int i=0; i<InputDim; i++) { //rows
          // casting InputDim to double isn't strictly necessary here, just playing it safe
          InitSigPts(i,j) = State(i) + std::sqrt(double(InputDim))*CholCovLowerEigen(i,j);
          InitSigPts(i,j+InputDim) = State(i) - std::sqrt(double(InputDim))*CholCovLowerEigen(i,j);
       }
   }

   // Map each of the sigma points through the nonlinear function
   Eigen::MatrixXd MappedSigPts(MappedDim,InputDim*2);
   Eigen::VectorXd InitSigPtVect;
   Eigen::VectorXd MappedSigPtVect;
   for(int j=0; j<2*InputDim; j++) { //columns
      // Convert InitSigPts(:,j) to Eigen::VectorXd
      InitSigPtVect = InitSigPts.col(j);

      // map the vector
      MappedSigPtVect = (*f)(InitSigPtVect);

      // place in MappedSigPts
      MappedSigPts.col(j) = MappedSigPtVect;
   }

   // Compute the average of the sigma points
   Eigen::VectorXd MappedSigPtAccum(MappedDim);
   MappedSigPtAccum.setZero();
   for(int j=0; j<2*InputDim; j++) { //columns
          MappedSigPtAccum = MappedSigPtAccum + MappedSigPts.col(j);
   }
   Eigen::VectorXd MappedSigPtsAvg = MappedSigPtAccum/(2*double(InputDim));

   // Compute the mapped covariance
   Eigen::MatrixXd MappedSigPtsCovAccum(MappedDim,MappedDim);
   MappedSigPtsCovAccum.setZero();
   Eigen::VectorXd DiffVector;
   for(int j=0; j<2*InputDim; j++) { //columns
      DiffVector = MappedSigPts.col(j) - MappedSigPtsAvg;
      MappedSigPtsCovAccum = MappedSigPtsCovAccum + DiffVector*DiffVector.transpose();
   }
   MappedCov = MappedSigPtsCovAccum/(2*double(InputDim));

   return MappedCov;
}


