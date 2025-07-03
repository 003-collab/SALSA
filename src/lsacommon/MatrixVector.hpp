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
#ifndef LSA_INCLUDE_FOR_MATRIX_AND_SPARSE_MATRIX
#define LSA_INCLUDE_FOR_MATRIX_AND_SPARSE_MATRIX

// Keep RANGECHECK *before* the #includes
// If defined, Matrix and Vector will throw on invalid index; this is very expensive
// in CPU time.
//#define RANGECHECK 1
#include "Vector.hpp"
#include "Matrix.hpp"

// define SPARSE to use SparseMatrix
#define SPARSE 1
#ifdef SPARSE
   #include "SparseMatrix.hpp"
   #define FlexMatrix SparseMatrix
#else
   #define FlexMatrix Matrix
#endif

#endif   // LSA_INCLUDE_FOR_MATRIX_AND_SPARSE_MATRIX
