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
/// @file lsaeigen.cpp  Methods for solving the linearized least-squares problem using Eigen3.
/// Definitions for lsasolver.cpp to call Eigen3 solution for linearized least-squares and helper methods.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <Eigen/SVD>
#include <Eigen/IterativeLinearSolvers>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop


#include "lsasolver.hpp"
#include "lsaobseqn.hpp"
#include "lsaextr.hpp"

#include "LSAConstants.hpp"

#include "lsaeigen.hpp"
#include "logstream.hpp"


//------------------------------------------------------------------------------------
using namespace gnsstk;
using namespace gnsstk::StringUtils;
//------------------------------------------------------------------------------------
int DefineConstraintEquationsWithEigen(void) throw(gnsstk::Exception)
{
    try
    {
       GlobalData& GD=GlobalData::Instance();
       ObsEqnData& EQD=ObsEqnData::Instance();
       unsigned int ii;

       if(EQD.Ncon == 0) return lsa::LSASOLVER_OK;

       // Build the constraint equation
       Eigen::MatrixXd dense_F(EQD.Ncon,EQD.StateNames.size());
       dense_F.setZero();

       EQD.F = gnsstk::Matrix<double>(EQD.Ncon, EQD.StateNames.size(), 0.0);
       //EQD.h = Vector<double>(EQD.Ncon, 0.0);
       gnsstk::Matrix<double> Part;
       for(ii=0; ii<EQD.Pts.size(); ii++) {
          std::string label = EQD.Pts[ii];
          char axis = EQD.axes[ii];

          // get the partials matrix for this Point
          if(axis == 'N' || axis == 'E' || axis == 'U')
             Part = GD.Points[label].getRotation();
          else
             Part = gnsstk::ident<double>(3);

          // now copy this into the current row and correct columns for the State
          int i = EQD.StateNames.index(label + XYZ[0]);
          if(i == -1)
             GNSSTK_THROW(gnsstk::Exception("Constraint axis not in state: " + label+XYZ[0]));
          int j = EQD.StateNames.index(label + XYZ[1]);
          if(j == -1)
             GNSSTK_THROW(gnsstk::Exception("Constraint axis not in state: " + label+XYZ[1]));
          int k = EQD.StateNames.index(label + XYZ[2]);
          if(k == -1)
             GNSSTK_THROW(gnsstk::Exception("Constraint axis not in state: " + label+XYZ[2]));

          unsigned int jj(((axis == 'N' || axis == 'X') ? 0 :
                          ((axis == 'E' || axis == 'Y') ? 1 : 2)));
          EQD.F(ii,i) = Part(jj,0);
          EQD.F(ii,j) = Part(jj,1);
          EQD.F(ii,k) = Part(jj,2);

          dense_F(ii,i) = Part(jj,0);
          dense_F(ii,j) = Part(jj,1);
          dense_F(ii,k) = Part(jj,2);
       }

       const unsigned int N(EQD.F.cols());

       Eigen::JacobiSVD<Eigen::MatrixXd> jacobi_svd(dense_F, Eigen::ComputeFullV);
       Eigen::MatrixXd dense_V;
       Eigen::SparseMatrix<double> sparse_V0;

       dense_V = jacobi_svd.matrixV();

       // are the constraints consistent?
       if(::fabs(jacobi_svd.singularValues()(EQD.Ncon-1)) < 1.e-10) {
          LOG(ERROR) << "\nError - constraints are inconsistent "
             << std::scientific << jacobi_svd.singularValues()(EQD.Ncon-1) << " Abort.";
          return lsa::PROBLEM_INCONSISTENT_CONSTRAINTS;
       }

       // separate the SVD into Null space and range space, and save
       Eigen::SparseMatrix<double> sparse_temporary;
       sparse_temporary = dense_V.sparseView();
       sparse_V0 = sparse_temporary.block(0,EQD.Ncon,N,N-EQD.Ncon);

       eigen_copy_mat_to_tk(EQD.V0,sparse_V0);

       return lsa::LSASOLVER_OK;
    }
    catch(gnsstk::Exception& e) { GNSSTK_RETHROW(e); }
    catch(std::bad_alloc& ba)
    {
        LOG(ERROR) << lsa::FASTSOLVER_ERRORMEMORYALLOCATE << ba.what() << ". Please refer to the Troubleshooting section in the user manual.";
        return lsa::PROBLEM_MEM_ALLOC;
    }
}

int ObtainCovarianceWithEigen() throw(gnsstk::Exception)
{
    try
    {
        GlobalData& GD=GlobalData::Instance();
        ObsEqnData& EQD=ObsEqnData::Instance();
        SolutionData& SD=SolutionData::Instance();

        Eigen::SparseMatrix<double> sparse_partials(EQD.Partials.rows(),EQD.Partials.cols());
        Eigen::SparseMatrix<double> sparse_mcov(EQD.MCov.rows(),EQD.MCov.cols());
        Eigen::SparseMatrix<double> sparse_v0(EQD.V0.rows(),EQD.V0.cols());
        Eigen::SparseMatrix<double> sparse_invcov,sparse_cov,sparse_weight;

        eigen_copy_mat_from_tk(EQD.V0,sparse_v0);
        eigen_copy_mat_from_tk(EQD.Partials,sparse_partials);
        eigen_copy_mat_from_tk(EQD.MCov,sparse_mcov);

        int eigen_return_status = 0;

        if(EQD.Ncon > 0)
            sparse_partials = sparse_partials * sparse_v0;

        eigen_return_status = eigen_sparse_inverse(sparse_mcov,sparse_weight);
        if(eigen_return_status != 0)
        {
            LOG(ERROR) << lsa::FASTSOLVER_ERRORSTRING << " - MCov inversion failed.";
            return lsa::PROBLEM_EIGEN;
        }

        sparse_invcov = sparse_partials.transpose()*sparse_weight*sparse_partials;

        eigen_return_status = eigen_sparse_inverse(sparse_invcov,sparse_cov);
        if(eigen_return_status != 0)
        {
            LOG(ERROR) << lsa::FASTSOLVER_ERRORSTRING << " - covariance inversion failed.";
            return lsa::PROBLEM_EIGEN;
        }
        GD.timing("Eigen3 Invert invCov");

        if(EQD.Ncon > 0)
        {
            sparse_cov = sparse_v0*sparse_cov*sparse_v0.transpose();
            sparse_cov.makeCompressed();
            GD.timing("Transform cov back to full solution space");
        }

        // Update SD.Cov
        eigen_copy_mat_to_tk(SD.Cov,sparse_cov);

    }
    catch(gnsstk::Exception& e)
    {
        LOG(ERROR) << " Error - ObtainCovarianceWithEigen threw " << e.what();
        GNSSTK_RETHROW(e);
    }
    catch(std::bad_alloc& ba)
    {
        LOG(ERROR) << lsa::FASTSOLVER_ERRORMEMORYALLOCATE << ba.what() << ". Please refer to the Troubleshooting section in the user manual.";
        return lsa::PROBLEM_MEM_ALLOC;
    }

    return lsa::LSASOLVER_OK;
}


int SolveLLSProblemWithEigen(const std::string& msg) throw(gnsstk::Exception)
{
    try {
        GlobalData& GD=GlobalData::Instance();
        ObsEqnData& EQD=ObsEqnData::Instance();
        SolutionData& SD=SolutionData::Instance();


        // Initialize matrices and vectors
        Eigen::SparseMatrix<double> sparse_partials(EQD.Partials.rows(),EQD.Partials.cols());
        Eigen::SparseMatrix<double> sparse_v0(EQD.V0.rows(),EQD.V0.cols());
        Eigen::SparseMatrix<double> sparse_invcov(EQD.Partials.cols(),EQD.Partials.cols());
        Eigen::SparseMatrix<double> sparse_mcov(EQD.MCov.rows(),EQD.MCov.cols());
        Eigen::SparseMatrix<double> sparse_weight(EQD.MCov.rows(),EQD.MCov.cols());

        // SRIF matrix and vector
        Eigen::SparseMatrix<double> sparse_R;
        Eigen::VectorXd vec_Z;

        Eigen::VectorXd vec_data,vec_sol,vec_weighted,raw_residuals;

        int eigen_return_status = 0;

        // Copy values from gnsstk to eigen
        eigen_copy_mat_from_tk(EQD.V0,sparse_v0);
        eigen_copy_mat_from_tk(EQD.Partials,sparse_partials);
        eigen_copy_mat_from_tk(EQD.MCov,sparse_mcov);
        eigen_copy_vec_from_tk(EQD.Data,vec_data);

        if(GD.niter == 1 && GD.doTiming)
            LOG(INFO) << "TIMING BENCHMARK - nonzeros: " << sparse_partials.nonZeros() << std::endl;

        GD.timing("Eigen3 Copy from tk");

        // Determine Weight, invCov, weighted vector (P'*W*b)
        eigen_return_status = eigen_sparse_inverse(sparse_mcov,sparse_weight);
        if(eigen_return_status != 0)
        {
            LOG(ERROR) << lsa::FASTSOLVER_ERRORSTRING << msg << " - MCov inversion failed.";
            return lsa::PROBLEM_EIGEN;
        }

        if(EQD.Ncon > 0)
            sparse_partials = sparse_partials * sparse_v0;

        sparse_invcov = sparse_partials.transpose()*sparse_weight*sparse_partials;
        vec_weighted = sparse_partials.transpose()*sparse_weight*vec_data;

        sparse_invcov.makeCompressed();

        // Solve
        eigen_return_status = eigen_sparseCG_iterate(sparse_invcov,vec_weighted,vec_sol);
        if(eigen_return_status != 0)
        {
            LOG(ERROR) << lsa::FASTSOLVER_ERRORSTRING << msg << " - ConjugateGradient solver error.";
            return lsa::PROBLEM_EIGEN;
        }
        if(vec_sol.hasNaN())
        {
            LOG(ERROR) << lsa::FASTSOLVER_ERRORSTRING << msg << " - NaN detected in solution.";
            return lsa::PROBLEM_EIGEN;
        }

        raw_residuals = vec_data - sparse_partials*vec_sol;
        GD.timing("Eigen3 Invert MCov, Weight and Solve");

        // Determine R
        sparse_R.resize(sparse_invcov.rows(),sparse_invcov.cols());
        Eigen::SimplicialLDLT<Eigen::SparseMatrix<double> > chol;
        chol.compute(sparse_invcov);

        if(chol.info() == Eigen::Success)
        {
            sparse_R = chol.matrixU();
        }
        else
        {
            LOG(ERROR) << lsa::FASTSOLVER_ERRORSTRING << msg << " - Cholesky decomposition failed." << std::endl;
            return lsa::EIGEN_CHOLESKY_FAIL;
        }
        GD.timing("Eigen3 Cholesky Upper");

        // Determine Z
        vec_Z = sparse_R*vec_sol;
        GD.timing("Eigen3 Calculate Z");

        // Prepare R,Z,Namelist and rebuild SRI
        gnsstk::SparseMatrix<double> tk_R;
        gnsstk::Vector<double> tk_Z;

        eigen_copy_mat_to_tk(tk_R,sparse_R);
        eigen_copy_vec_to_tk(tk_Z,vec_Z);

        GD.timing("Eigen3 Copy R,Z");

        // if constraints, then construct GD.sri with Namelist of size R.cols()
        if(EQD.Ncon > 0)
            GD.sri = gnsstk::SRI(tk_R,tk_Z,gnsstk::Namelist(tk_R.cols()));
        else
            GD.sri = gnsstk::SRI(tk_R,tk_Z,EQD.StateNames);

        GD.timing("Rebuild SRI");

        // weight (whiten) Partials and Data
        gnsstk::FlexMatrix<double> L;
        try
        {
            L = lowerCholesky(EQD.MCov);
            GD.timing("Cholesky(SolveLLS)");
            EQD.White = inverseLT(L);
            GD.timing("WhiteInv(SolveLLS)");
        }
        catch(gnsstk::Exception& e)
        {
            LOG(ERROR) << " Error - singular measurement covariance: " << e.what();
            //GNSSTK_RETHROW(e);
            return lsa::SINGULAR_COVARIANCE;
        }

        // Update SD.Sol
        eigen_copy_vec_to_tk(SD.Sol,vec_sol);

        if(EQD.Ncon > 0)
        {
            SD.Sol = EQD.V0 * SD.Sol;
            GD.timing("Transform solution back to full solution space");
        }

        if(GD.verbose)
        {
            ObtainCovarianceWithEigen();
        }

        eigen_copy_vec_to_tk(SD.RawResid,raw_residuals);
        GD.timing("Copy SD.RawResid");

        SD.Resid = EQD.White * SD.RawResid;
        GD.timing("Update SD.Resid");

        return lsa::LSASOLVER_OK;
    }
    catch(gnsstk::Exception& e)
    {
        LOG(ERROR) << " Error - SolveLLSProblemWithEigen threw " << e.what();
        GNSSTK_RETHROW(e);
    }
    catch(std::bad_alloc& ba)
    {
        LOG(ERROR) << lsa::FASTSOLVER_ERRORMEMORYALLOCATE << ba.what() << ". Please refer to the Troubleshooting section in the user manual.";
        return lsa::PROBLEM_MEM_ALLOC;
    }
}

void eigen_copy_mat_to_tk(gnsstk::SparseMatrix<double>& tk_sparse, Eigen::SparseMatrix<double>&  sparse_mat)
{
    tk_sparse = gnsstk::SparseMatrix<double>(sparse_mat.rows(),sparse_mat.cols());
    for (int k=0; k<sparse_mat.outerSize(); ++k)
        for (Eigen::SparseMatrix<double>::InnerIterator it(sparse_mat,k); it; ++it)
        {
            tk_sparse(it.row(),it.col()) = it.value();
        }
}

void eigen_copy_mat_to_tk(gnsstk::Matrix<double>& tk_mat, Eigen::SparseMatrix<double>&  sparse_mat)
{
    tk_mat = gnsstk::Matrix<double>(sparse_mat.rows(),sparse_mat.cols(),0.0);
    for (int k=0; k<sparse_mat.outerSize(); ++k)
        for (Eigen::SparseMatrix<double>::InnerIterator it(sparse_mat,k); it; ++it)
        {
            tk_mat(it.row(),it.col()) = it.value();
        }
}

void eigen_copy_vec_to_tk(gnsstk::Vector<double>& tk_vector, Eigen::VectorXd& vec)
{
    tk_vector = gnsstk::Vector<double>(vec.rows(),0.0);
    for(int i=0;i<vec.rows();i++)
        tk_vector(i) = vec(i);
}

void eigen_copy_vec_from_tk(const gnsstk::Vector<double>& tk_vector, Eigen::VectorXd& vec)
{
    vec.resize(tk_vector.size());
    vec.setZero();

    for(int i=0;i<tk_vector.size();i++)
        vec(i) = tk_vector(i);

}

void eigen_copy_mat_from_tk(gnsstk::SparseMatrix<double>& tk_sparse, Eigen::SparseMatrix<double>&  sparse_mat)
{
    std::vector<unsigned int> rows,cols;
    std::vector<double> matrix_values;

    tk_sparse.flatten(rows,cols,matrix_values);

    sparse_mat.resize(tk_sparse.rows(),tk_sparse.cols());
    sparse_mat.reserve(matrix_values.size());
    sparse_mat.setZero();

    for(unsigned int i=0;i<matrix_values.size();i++)
        sparse_mat.insert(rows[i],cols[i]) = matrix_values[i];

}

int eigen_sparse_inverse(Eigen::SparseMatrix<double>& sparse_mat, Eigen::SparseMatrix<double>& sparse_invmat)
{
    sparse_invmat.resize(sparse_mat.rows(),sparse_mat.cols());
    sparse_invmat.setZero();

    Eigen::SimplicialLLT<Eigen::SparseMatrix<double> > lltsolver;
    Eigen::SparseMatrix<double> I(sparse_mat.rows(),sparse_mat.cols());
    I.setIdentity();

    if(!sparse_mat.isCompressed())
        sparse_mat.makeCompressed();

    lltsolver.compute(sparse_mat);
    if(lltsolver.info() == Eigen::Success)
    {
        sparse_invmat = lltsolver.solve(I);
        return lsa::LSASOLVER_OK;
    }

    return 1;
}

int eigen_sparseCG_iterate(Eigen::SparseMatrix<double>&  sparse_mat, Eigen::VectorXd& vec, Eigen::VectorXd& output_vec)
{
    output_vec.resize(vec.rows());
    output_vec.setZero();

    Eigen::ConjugateGradient<Eigen::SparseMatrix<double> > solver;
    sparse_mat.makeCompressed();
    solver.setMaxIterations(10000);
    solver.compute(sparse_mat);
    if(solver.info() == Eigen::Success)
    {
        output_vec = solver.solve(vec);
        return lsa::LSASOLVER_OK;
    }

    return 1;
}
