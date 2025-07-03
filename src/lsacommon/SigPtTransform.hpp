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
/// @file SigPtTransform.hpp  Include file for generalized sigma point transform function.

#ifndef SIG_PT_TRANSFORM_INCLUDE
#define SIG_PT_TRANSFORM_INCLUDE

//#include <string>
//#include "Exception.hpp"
//#include "Matrix.hpp"
//#include <QString>
// system includes
#include <ctime>
#include <set>
#include <string>
#include <vector>
// GNSSTk
#include "Epoch.hpp"
//#include "StringUtils.hpp"
#include <Point.hpp>
#include "singleton.hpp"
#include "expandtilde.hpp"
#include "logstream.hpp"
#include "CommandLine.hpp"
//LSA
#include <lsah5.hpp>
#include "lsaextr.hpp"
#include <lsaUtils.hpp>
#include <LSAConstants.hpp>
//Qt
#include <QFileInfo>
// WGS84 ellipsoid
#include <WGS84Ellipsoid.hpp>
// Eigen
// pragma commands used to squelch useless warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <Eigen/Dense>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop


///------------------------------------------------------------------------------------
/// Employ sigma point transform to map covariance via nominal mapping function
/// input Eigen::VectorXd State                   nominal input state
/// input Eigen::MatrixXd Cov                     nominal input state covariance
/// input Eigen::VectorXd (*f)(Eigen::VectorXd)   pointer to mapping function that
///                                               expects a single Eigen::VectorXd input
///                                               and outputs a single Eigen::VectorXd
/// output Eigen::MatrixXd                        mapped covariance
Eigen::MatrixXd SigmaPointTransformFunc(Eigen::VectorXd State, Eigen::MatrixXd Cov,
                                        Eigen::VectorXd (*f)(Eigen::VectorXd));

#endif   // SIG_PT_TRANSFORM_INCLUDE
