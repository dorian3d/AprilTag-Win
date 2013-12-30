// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "matrix.h"
#include <math.h>

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#else
#include <cblas.h>
#include <clapack.h>
#endif

#include <stdio.h>

extern "C" {
#ifndef __APPLE__
extern int ilaenv_(int *, char *, char *, int *, int *, int *, int *, int, int);
#endif

#ifdef F_T_IS_SINGLE
#ifdef __APPLE__
    //apple weirdly defines these with an apparently unnecessary typedef since int and long int are both 32 bits on ios and int is 32 bits on x64. WTF?
    static int ilaenv(int *ispec, char *name, char *opts, int *n1, int *n2, int *n3, int *n4) { return ilaenv_((__CLPK_integer *)ispec, name, opts, (__CLPK_integer *)n1, (__CLPK_integer *)n2, (__CLPK_integer *)n3, (__CLPK_integer *)n4); }
    
    static int sytrf(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *lwork, int *info) { return ssytrf_(uplo, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, (__CLPK_integer *)ipiv, work, (__CLPK_integer *)lwork, (__CLPK_integer *)info); }
    static int sytri(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *info) { return ssytri_(uplo, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, (__CLPK_integer *)ipiv, work, (__CLPK_integer *)info); }
    static int sytrs(char *uplo, int *n, int *nrhs, f_t *a, int *lda, int *ipiv, f_t *b, int *ldb, int *info) { return ssytrs_(uplo, (__CLPK_integer *)n, (__CLPK_integer *)nrhs, a,(__CLPK_integer *)lda, (__CLPK_integer *)ipiv, b, (__CLPK_integer *)ldb, (__CLPK_integer *)info); }
    
    static int potrf(char *uplo, int *n, f_t *a, int *lda, int *info) { return spotrf_(uplo, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, (__CLPK_integer *)info); }
    static int potri(char *uplo, int *n, f_t *a, int *lda, int *info) { return spotri_(uplo, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, (__CLPK_integer *)info); }
    static int potrs(char *uplo, int *n, int *nrhs, f_t *a, int *lda, f_t *b, int *ldb, int *info) { return spotrs_(uplo, (__CLPK_integer *)n, (__CLPK_integer *)nrhs, a,(__CLPK_integer *)lda, b, (__CLPK_integer *)ldb, (__CLPK_integer *)info); }
    
    static int gelsd(int *m, int *n, int *nrhs, f_t *a, int *lda, f_t *b, int *ldb, f_t *s, f_t *rcond, int *rank, f_t *work, int *lwork, int *iwork, int *info) { return sgelsd_((__CLPK_integer *)m, (__CLPK_integer *)n, (__CLPK_integer *)nrhs, a, (__CLPK_integer *)lda, b, (__CLPK_integer *)ldb, s, rcond, (__CLPK_integer *)rank, work, (__CLPK_integer *)lwork, (__CLPK_integer *)iwork, (__CLPK_integer *)info); }
    static int gesvd(char *jobu, char *jobvt, int *m, int *n, f_t *a, int *lda, f_t *s, f_t *u, int *ldu, f_t *vt, int *ldvt, f_t *work, int *lwork, int *info) { return sgesvd_(jobu, jobvt, (__CLPK_integer *)m, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, s, u, (__CLPK_integer *)ldu, vt, (__CLPK_integer *)ldvt, work, (__CLPK_integer *)lwork, (__CLPK_integer *)info); }
    static int gesdd(char *jobz, int *m, int *n, f_t *a, int *lda, f_t *s, f_t *u, int *ldu, f_t *vt, int *ldvt, f_t *work, int *lwork, int *iwork, int *info) { return sgesdd_(jobz, (__CLPK_integer *)m, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, s, u, (__CLPK_integer *)ldu, vt, (__CLPK_integer *)ldvt, work, (__CLPK_integer *)lwork, (__CLPK_integer *)iwork, (__CLPK_integer *)info); }
#else
extern int ssytrf_(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *lwork, int *info);
extern int ssytri_(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *info);
extern int spotrf_(char *, int *, f_t *, int *, int *);
extern int spotri_(char *, int *, f_t *, int *, int *);
extern int spotrs_(char *, int *, int *, f_t *, int *, f_t *, int *, int *);
extern int ssytrs_(char *, int *, int *, f_t *, int *, int *, f_t *, int *, int *);
extern int sgelsd_(int *, int *, int *, f_t *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, int *, int *);
extern int sgesvd_(char *, char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *);
extern int sgesdd_(char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *, int *);
static int (*sytrf)(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *lwork, int *info) = ssytrf_;
static int (*sytri)(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *info) = ssytri_;
static int (*sytrs)(char *, int *, int *, f_t *, int *, int *, f_t *, int *, int *) = ssytrs_;
static int (*potrf)(char *, int *, f_t *, int *, int *) = spotrf_;
static int (*potri)(char *, int *, f_t *, int *, int *) = spotri_;
static int (*potrs)(char *, int *, int *, f_t *, int *, f_t *, int *, int *) = spotrs_;
static int (*gelsd)(int *, int *, int *, f_t *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, int *, int *) = sgelsd_;
static int (*gesvd)(char *, char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *) = sgesvd_;
static int (*gesdd)(char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *, int *) = sgesdd_;
#endif
#endif
#ifdef F_T_IS_DOUBLE
#ifdef __APPLE__
    //apple weirdly defines these with an apparently unnecessary typedef since int and long int are both 32 bits on ios and int is 32 bits on x64. WTF?
    static int ilaenv(int *ispec, char *name, char *opts, int *n1, int *n2, int *n3, int *n4) { return ilaenv_((__CLPK_integer *)ispec, name, opts, (__CLPK_integer *)n1, (__CLPK_integer *)n2, (__CLPK_integer *)n3, (__CLPK_integer *)n4); }
    
    static int sytrf(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *lwork, int *info) { return dsytrf_(uplo, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, (__CLPK_integer *)ipiv, work, (__CLPK_integer *)lwork, (__CLPK_integer *)info); }
    static int sytri(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *info) { return dsytri_(uplo, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, (__CLPK_integer *)ipiv, work, (__CLPK_integer *)info); }
    static int sytrs(char *uplo, int *n, int *nrhs, f_t *a, int *lda, int *ipiv, f_t *b, int *ldb, int *info) { return dsytrs_(uplo, (__CLPK_integer *)n, (__CLPK_integer *)nrhs, a,(__CLPK_integer *)lda, (__CLPK_integer *)ipiv, b, (__CLPK_integer *)ldb, (__CLPK_integer *)info); }
    
    static int potrf(char *uplo, int *n, f_t *a, int *lda, int *info) { return dpotrf_(uplo, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, (__CLPK_integer *)info); }
    static int potri(char *uplo, int *n, f_t *a, int *lda, int *info) { return dpotri_(uplo, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, (__CLPK_integer *)info); }
    static int potrs(char *uplo, int *n, int *nrhs, f_t *a, int *lda, f_t *b, int *ldb, int *info) { return dpotrs_(uplo, (__CLPK_integer *)n, (__CLPK_integer *)nrhs, a,(__CLPK_integer *)lda, b, (__CLPK_integer *)ldb, (__CLPK_integer *)info); }
    
    static int gelsd(int *m, int *n, int *nrhs, f_t *a, int *lda, f_t *b, int *ldb, f_t *s, f_t *rcond, int *rank, f_t *work, int *lwork, int *iwork, int *info) { return dgelsd_((__CLPK_integer *)m, (__CLPK_integer *)n, (__CLPK_integer *)nrhs, a, (__CLPK_integer *)lda, b, (__CLPK_integer *)ldb, s, rcond, (__CLPK_integer *)rank, work, (__CLPK_integer *)lwork, (__CLPK_integer *)iwork, (__CLPK_integer *)info); }
    static int gesvd(char *jobu, char *jobvt, int *m, int *n, f_t *a, int *lda, f_t *s, f_t *u, int *ldu, f_t *vt, int *ldvt, f_t *work, int *lwork, int *info) { return dgesvd_(jobu, jobvt, (__CLPK_integer *)m, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, s, u, (__CLPK_integer *)ldu, vt, (__CLPK_integer *)ldvt, work, (__CLPK_integer *)lwork, (__CLPK_integer *)info); }
    static int gesdd(char *jobz, int *m, int *n, f_t *a, int *lda, f_t *s, f_t *u, int *ldu, f_t *vt, int *ldvt, f_t *work, int *lwork, int *iwork, int *info) { return dgesdd_(jobz, (__CLPK_integer *)m, (__CLPK_integer *)n, a, (__CLPK_integer *)lda, s, u, (__CLPK_integer *)ldu, vt, (__CLPK_integer *)ldvt, work, (__CLPK_integer *)lwork, (__CLPK_integer *)iwork, (__CLPK_integer *)info); }
#else
    extern int dsytrf_(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *lwork, int *info);
    extern int dsytri_(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *info);
    extern int dpotrf_(char *, int *, f_t *, int *, int *);
    extern int dpotri_(char *, int *, f_t *, int *, int *);
    extern int dpotrs_(char *, int *, int *, f_t *, int *, f_t *, int *, int *);
    extern int dsytrs_(char *, int *, int *, f_t *, int *, int *, f_t *, int *, int *);
    extern int dgelsd_(int *, int *, int *, f_t *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, int *, int *);
    extern int dgesvd_(char *, char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *);
    extern int dgesdd_(char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *, int *);
    static int (*sytrf)(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *lwork, int *info) = dsytrf_;
    static int (*sytri)(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *info) = dsytri_;
    static int (*sytrs)(char *, int *, int *, f_t *, int *, int *, f_t *, int *, int *) = dsytrs_;
    static int (*potrf)(char *, int *, f_t *, int *, int *) = dpotrf_;
    static int (*potri)(char *, int *, f_t *, int *, int *) = dpotri_;
    static int (*potrs)(char *, int *, int *, f_t *, int *, f_t *, int *, int *) = dpotrs_;
    static int (*gelsd)(int *, int *, int *, f_t *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, int *, int *) = dgelsd_;
    static int (*gesvd)(char *, char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *) = dgesvd_;
    static int (*gesdd)(char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *, int *) = dgesdd_;
#endif
#endif
}

bool matrix::is_symmetric(f_t eps = 1.e-5) const
{
    for(int i = 0; i < rows; ++i) {
        for(int j = i; j < cols; ++j) {
            if(fabs((*this)(i,j) - (*this)(j,i)) > eps) return false;
        }
    }
    return true;
}

void matrix::print() const
{
    for(int i = 0; i < rows; ++i) {
        for(int j = 0; j < cols; ++j) {
            f_t v = (*this)(i, j);
            if(i == j) fprintf(stderr, "[% .1e]", v);
            else if(v == 0) fprintf(stderr, "     0    ");
            else fprintf(stderr, " % .1e ", v);
        }
        fprintf(stderr, "\n");
    }
}

void matrix::print_high() const
{
    for(int i = 0; i < rows; ++i) {
        for(int j = 0; j < cols; ++j) {
            f_t v = (*this)(i, j);
            if(i == j) fprintf(stderr, "[% .10e]", v);
            else if(v == 0) fprintf(stderr, "     0    ");
            else fprintf(stderr, " % .10e ", v);
        }
        fprintf(stderr, "\n");
    }
}

void matrix::print_diag() const
{
    for(int i = 0; i < rows; ++i) {
        fprintf(stderr, "% .1e ", (*this)(i, i));
    }
    fprintf(stderr, "\n");
}
void matrix_product(matrix &res, const matrix &A, const matrix &B, bool trans1, bool trans2, const f_t dst_scale, const f_t scale)
{
    int d1, d2, d3, d4;
    if(trans1) {
        d1 = A.cols;
        d2 = A.rows;
    } else {
        d1 = A.rows;
        d2 = A.cols;
    }
    if(trans2) {
        d3 = B.cols;
        d4 = B.rows;
    } else {
        d3 = B.rows;
        d4 = B.cols;
    }
    assert(d2 == d3);
    assert(res.rows == d1);
    assert(res.cols == d4);
#ifdef F_T_IS_DOUBLE
    cblas_dgemm(CblasRowMajor, trans1?CblasTrans:CblasNoTrans, trans2?CblasTrans:CblasNoTrans, 
                res.rows, res.cols, A.cols, 
                scale, A.data, A.stride,
                B.data, B.stride, 
                dst_scale, res.data, res.stride);
#else
    cblas_sgemm(CblasRowMajor, trans1?CblasTrans:CblasNoTrans, trans2?CblasTrans:CblasNoTrans, 
                res.rows, res.cols, A.cols, 
                scale, A.data, A.stride,
                B.data, B.stride, 
                dst_scale, res.data, res.stride);
#endif
}    

bool matrix_invert(matrix &m)
{
    char uplo = 'U';
    int ipiv[m.stride];
    //just make a work array as big as the input
    //TODO: call ssytrf_ with lwork = -1 -- returns optimal size as work[0] ? wtf?
    int ign = -1;
    const char *name = "DSYTRF";
    const char *tp = "U";
    int ispec = 1;
#ifdef __APPLE__
    int lwork = m.stride * ilaenv(&ispec, (char *)name, (char *)tp, &m.cols, &ign, &ign, &ign);
#else
    int lwork = m.stride * ilaenv_(&ispec, name, tp, &m.cols, &ign, &ign, &ign, 6, 1);
#endif
    if(lwork < 1) lwork = m.stride*4;
    v_intrinsic work[lwork/4];
    int info;
    sytrf(&uplo, &m.cols, m.data, &m.stride, ipiv, (f_t *)work, &lwork, &info);
    if(info) {
        fprintf(stderr, "matrix_invert: ssytrf failed: %d\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    sytri(&uplo, &m.cols, m.data, &m.stride, ipiv, (f_t *)work, &info);
    if(info) {
        fprintf(stderr, "matrix_invert: ssytri failed: %d\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    //only generates half the matrix. so-called upper is actually lower (fortran)
    for(int i = 0; i < m.rows; ++i) {
        for(int j = i+1; j < m.rows; ++j) {
            m(i, j) = m(j, i);
        }
    }
    return true;
}

void matrixest();
bool matrix_solve_syt(matrix &A, matrix &B)
{
    char uplo = 'U';
    int ipiv[A.stride];
    //just make a work array as big as the input
    //TODO: call ssytrf_ with lwork = -1 -- returns optimal size as work[0] ? wtf?
    int ign = -1;
    const char *name = "DSYTRF";
    const char *tp = "U";
    int ispec = 1;
#ifdef __APPLE__
    int lwork = A.stride * ilaenv(&ispec, (char *)name, (char *)tp, &A.cols, &ign, &ign, &ign);
#else
    int lwork = A.stride * ilaenv_(&ispec, name, tp, &A.cols, &ign, &ign, &ign, 6, 1);
#endif
    if(lwork < 1) lwork = A.stride*4;
    f_t work[lwork];
    int info;
    sytrf(&uplo, &A.cols, A.data, &A.stride, ipiv, work, &lwork, &info);
    if(info) {
        fprintf(stderr, "matrix_solve: sytrf failed: %d\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    sytrs(&uplo, &A.cols, &B.rows, A.data, &A.stride, ipiv, B.data, &B.stride, &info);
    if(info) {
        fprintf(stderr, "matrix_solve: sytrs failed: %d\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    return true;
}

void test_cholesky(matrix &A)
{
    assert(A.rows = A.cols);
    assert(A.is_symmetric());
    int N = A.rows;
    MAT_TEMP(res, N, N);
    MAT_TEMP(B, N, N);
    fprintf(stderr, "original matrix is: \n");
    A.print();


    for (int i = 0; i < N; ++i) {
        for(int j = 0; j < N; ++j) {
            B(i, j) = A(i, j);
        }
    }

    char uplo = 'U';
    int info;
    potrf(&uplo, &B.cols, B.data, &B.stride, &info);
    if(info) {
        fprintf(stderr, "cholesky: potrf failed: %d\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
    }
    fprintf(stderr, "cholesky result is: \n");
    B.print();
    //clear out any leftover data in the upper part of B
    for(int i = 0; i < N; ++i) {
        for(int j = i + 1; j < N; ++j) {
            B(i, j) = 0.;
        }
    }
    fprintf(stderr, "after clearing: \n");
    B.print();
    matrix_product(res, B, B, false, true);
    fprintf(stderr, "after multiplication: \n");
    res.print();
    for (int i = 0; i < N; ++i) {
        for(int j = 0; j < N; ++j) {
            /*if(fabs(res(i, j) - A(i,j)) > 1.e-45) {
                            fprintf(stderr, "(%d, %d) res is %e, cov is %e\n", i, j, res(i,j), A(i,j));
                            }*/
            B(i, j) = res(i, j) - A(i, j);
        }
    }
    fprintf(stderr, "residual is: \n");
    B.print();
}

//returns lower triangular (by my conventions) cholesky matrix
bool matrix_cholesky(matrix &A)
{
    //test_cholesky(A);
    //A.print();
    char uplo = 'U';
    int info;
    potrf(&uplo, &A.cols, A.data, &A.stride, &info);
    if(info) {
        fprintf(stderr, "cholesky: potrf failed: %d\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    //potrf only computes upper fortran (so really lower) triangle
    //clear out any leftover data in the upper part of A
    for(int i = 0; i < A.rows; ++i) {
        for(int j = i + 1; j < A.rows; ++j) {
            A(i, j) = 0.;
        }
    }
    return true;
}

bool matrix_solve(matrix &A, matrix &B)
{
    char uplo = 'U';
    int info;
    potrf(&uplo, &A.cols, A.data, &A.stride, &info);
    if(info) {
        fprintf(stderr, "solve: spotrf failed: %d\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false; //could return matrix_solve_syt here instead
    }
    potrs(&uplo, &A.cols, &B.rows, A.data, &A.stride, B.data, &B.stride, &info);
    if(info) {
        fprintf(stderr, "solve: spotrs failed: %d\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    return true;
}

bool matrix_solve_svd(matrix &A, matrix &B)
{
    assert("fail! transposed! maybe fixed: test it!" == 0);
    int info;
    f_t sv[A.rows];
    int rank;
    f_t rcond = -1;
    int lwork = -1;
    f_t work0;
    gelsd(&A.cols, &A.rows, &B.cols, A.data, &A.stride, B.data, &B.stride, sv, &rcond, &rank, &work0, &lwork, 0, &info);
    lwork = (int)work0;
    f_t work[lwork];
    int iwork[lwork];
    gelsd(&A.cols, &A.rows, &B.cols, A.data, &A.stride, B.data, &B.stride, sv, &rcond, &rank, work, &lwork, iwork, &info);
    fprintf(stderr, "svd reported rank: %d\n", rank);
    if(info) {
        fprintf(stderr, "solve: sgelsd failed: %d\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    return true;
}

//returns in new matrices, but destroys A
// S should have dimension at least max(1, min(m,n))
bool matrix_svd(matrix &A, matrix &U, matrix &S, matrix &Vt)
{
    int info;

    int lwork = -1;
    f_t work0;
    int iwork[8 * A.cols];
    //gesvd/dd is fortran, so V^T and U are swapped
    gesdd((char *)"A", &A.cols, &A.rows, A.data, &A.stride, S.data, Vt.data, &Vt.stride, U.data, &U.stride, &work0, &lwork, iwork, &info);
    lwork = (int)work0;
    f_t work[lwork];
    gesdd((char *)"A", &A.cols, &A.rows, A.data, &A.stride, S.data, Vt.data, &Vt.stride, U.data, &U.stride, work, &lwork, iwork, &info);
    if(info) {
        fprintf(stderr, "svd: gesvd failed: %d\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    return true;
}

void matrix_transpose(matrix &dst, matrix &src)
{
    assert(dst.cols == src.rows && dst.rows == src.cols);
    for(int i = 0; i < dst.rows; ++i) {
        for(int j = 0; j < dst.cols; ++j) {
            dst(i, j) = src(j, i);
        }
    }
}

matrix &matrix_dereference(matrix *m)
{
    return *m;
}
