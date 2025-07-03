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
/// @file lsaeigen.hpp  Header file for lsaeigen.cpp.
/// Methods for solving the linearized least-squares problem using Eigen3

#ifndef LSAEIGEN_HPP
#define LSAEIGEN_HPP

#include <string>
#include <vector>

// gnsstk
#include "Exception.hpp"
#include "SparseVector.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <Eigen/Sparse>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

/// @brief Compute the covariance matrix using Eigen3 after the final iteration.
/// This should be called in lsasolver.cpp to populate SD.Cov after the final iteration.
/// @return Returns error code defined in main() in lsasolver.cpp
int ObtainCovarianceWithEigen() throw(gnsstk::Exception);

/// @brief Define the constraint equations using Eigen3 methods if GD.useSRIF is true.
/// Use EQD.Pts, EQD.axes, EQD.StateNames to build the constraint equation F
/// performing SVD on F to yield EQD.V0.
/// @return Returns error code defined in main() in lsasolver.cpp
int DefineConstraintEquationsWithEigen(void) throw(gnsstk::Exception);

//------------------------------------------------------------------------------------
/// @brief Solve LLS Problem using Eigen3 methods if GD.useSRIF is true.
/// Use EQD.Data, EQD.Partials, EQD.MCov to solve the linear least squares problem,
/// yielding SD.Sol, SD.Resid, SD.RawResid
/// Called by SolveLLSProblem().
/// @param msg Information about the iteration to be printed in error message
/// @return Returns error code defined in main() in lsasolver.cpp
int SolveLLSProblemWithEigen(const std::string& msg) throw(gnsstk::Exception);

/// @brief Invert Eigen::SparseMatrix<double> using LU.
/// @param sparse_mat Reference to matrix to be inverted, A.
/// @param sparse_invmat Reference to inverted sparse_mat, inv(A).
/// @return 0 if successful, 1 if there was a problem
int eigen_sparse_inverse(Eigen::SparseMatrix<double>&  sparse_mat, Eigen::SparseMatrix<double>& sparse_invmat);

/// @brief Solve Ax=b and update the referenced output vector.
/// @param sparse_mat Reference to system matrix, A.
/// @param vec Reference data vector, b.
/// @param output_vec Reference to solution vector, x.
/// @return 0 if successful, 1 if there was a problem
int eigen_sparseCG_iterate(Eigen::SparseMatrix<double>&  sparse_mat, Eigen::VectorXd& vec, Eigen::VectorXd& output_vec);

/// @brief Copy matrix from gnsstk::SparseMatrix<double> to Eigen::SparseMatrix<double>.
/// @param tk_sparse Reference to originating gnsstk matrix.
/// @param sparse_mat Reference to destination Eigen::SparseMatrix<double>.
void eigen_copy_mat_from_tk(gnsstk::SparseMatrix<double>& tk_sparse, Eigen::SparseMatrix<double>&  sparse_mat);

/// @brief Copy vector from gnsstk::Vector<double> to Eigen::VectorXd.
/// @param tk_vector Reference to originating gnsstk Vector.
/// @param vec Reference to destination Eigen::VectorXd.
void eigen_copy_vec_from_tk(const gnsstk::Vector<double>& tk_vector, Eigen::VectorXd& vec);

/// @brief Copy matrix from Eigen::SparseMatrix<double> to gnsstk::SparseMatrix<double>.
/// @param tk_sparse Reference to destination gnsstk::SparseMatrix<double>.
/// @param sparse_mat Reference to originating Eigen::SparseMatrix<double>.
void eigen_copy_mat_to_tk(gnsstk::SparseMatrix<double>& tk_sparse, Eigen::SparseMatrix<double>&  sparse_mat);

/// @brief Copy matrix from Eigen::SparseMatrix<double> to gnsstk::Matrix<double>.
/// The operation which uses this method needs to be re-evaluated as using Eigen Dense matrices
/// @param tk_mat Reference to destination gnsstk::Matrix<double>.
/// @param sparse_mat Reference to originating Eigen::SparseMatrix<double>.
void eigen_copy_mat_to_tk(gnsstk::Matrix<double>& tk_mat, Eigen::SparseMatrix<double>&  sparse_mat);

/// @brief Copy vector from Eigen::VectorXd to gnsstk::Vector<double>.
/// @param tk_vector Reference to destination gnsstk::Vector<double>.
/// @param vec Reference to originating Eigen::VectorXd.
void eigen_copy_vec_to_tk(gnsstk::Vector<double>& tk_vector, Eigen::VectorXd& vec);

/// @brief Copy matrix from Eigen::MatrixXd to gnsstk::Matrix<double>.
/// @param tk_mat Reference to destination gnsstk::Matrix<double>.
/// @param mat Reference to originating Eigen::MatrixXd.
void eigen_copy_mat_to_tk(gnsstk::Matrix<double>& tk_mat, Eigen::MatrixXd&  mat);

#endif // LSAEIGEN_HPP
