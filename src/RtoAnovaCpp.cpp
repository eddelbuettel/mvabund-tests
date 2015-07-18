// Interface between R and anova.cpp (Rcpp API >= 0.7.11)
//
// Author: Yi Wang (yi dot wang at unsw dot edu dot au)
// Last modified: 20-April-2010

#include <RcppGSL.h>
extern "C"{
#include "resampTest.h"
#include "time.h"
}

// [[Rcpp::export]]
Rcpp::List RtoAnovaCpp(Rcpp::List rparam,
                       RcppGSL::matrix<double> Y, // SEXP Ysexp,
                       RcppGSL::matrix<double> X, // SEXP Xsexp,
                       RcppGSL::matrix<double> isXvarIn,
                       SEXP bIDsexp) 
{
    using namespace Rcpp;

    // Get parameters in rparam.
    // pass parameters
    mv_Method mm;	
//    mm.tol = as<double>(rparam["tol"]);
    mm.nboot = as<unsigned int>(rparam["nboot"]);
    mm.corr = as<unsigned int>(rparam["cor_type"]);
    mm.shrink_param = as<double>(rparam["shrink_param"]);
    mm.test = as<unsigned int>(rparam["test_type"]);
    mm.resamp = as<unsigned int>(rparam["resamp"]);
    mm.reprand = as<unsigned int>(rparam["reprand"]);
    mm.student = as<unsigned int>(rparam["studentize"]);
    mm.punit = as<unsigned int>(rparam["punit"]);
    mm.rsquare = as<unsigned int>(rparam["rsquare"]);

// for debug
//    Rprintf("Input param arguments:\n tol=%g, nboot=%d, cor_type=%d, shrink_param=%g, test_type=%d, resamp=%d, reprand=%d\n",mm.tol, mm.nboot, mm.corr, mm.shrink_param, mm.test, mm.resamp, mm.reprand);

    unsigned int nRows = Y.nrow();
    unsigned int nVars = Y.ncol();
    unsigned int nParam = X.ncol();
    unsigned int nModels = isXvarIn.nrow();
// for debug
//    Rprintf("nRows=%d, nVars=%d, nParam=%d\n", nRows, nVars, nParam);

    // Rcpp -> gsl
    unsigned int i, j, k;

// initialize anova class
    AnovaTest anova(&mm, Y, X, isXvarIn);
	
// Resampling indices
    if ( !Rf_isNumeric(bIDsexp) || !Rf_isMatrix(bIDsexp) ) {
//      Rprintf("Calc bootID on the fly.\n");
     }
    else {
        if ( mm.resamp == SCOREBOOT ) {
            NumericMatrix bIDr(bIDsexp);
            mm.nboot = bIDr.nrow();	   
            anova.bootID = gsl_matrix_alloc(mm.nboot, nRows);
//	    std::copy ( bIDr.begin(), bIDr.end(), anova.bootID->data);
	    for (i=0; i<mm.nboot; i++)
	    for (j=0; j<nRows; j++)
                gsl_matrix_set(anova.bootID, i, j, bIDr(i, j));
	 }
	else{
	    IntegerMatrix bIDr(bIDsexp);
            mm.nboot = bIDr.nrow();	   
	    anova.bootID = gsl_matrix_alloc(mm.nboot, nRows);
	    // integer -> double
	    for (i=0; i<mm.nboot; i++)
            for (j=0; j<nRows; j++)
                gsl_matrix_set(anova.bootID, i, j, bIDr(i, j)-1);
    }  } 

    // resampling test
    anova.resampTest();
//    anova.display();


    // Wrap the gsl objects with Rcpp 
    NumericVector Vec_mul(anova.multstat, anova.multstat+nModels-1);
    NumericVector Vec_Pm(anova.Pmultstat, anova.Pmultstat+nModels-1);
    NumericVector Vec_df(anova.dfDiff, anova.dfDiff+nModels-1);
    
    RcppGSL::matrix<double> Mat_statj(anova.statj);
    RcppGSL::matrix<double> Mat_Pstatj(anova.Pstatj);
    
    // Rcpp -> R
    List rs = List::create(_["multstat" ] = Vec_mul,
                           _["Pmultstat"] = Vec_Pm,
                           _["dfDiff"   ] = Vec_df,
                           _["statj"    ] = Mat_statj,
                           _["Pstatj"   ] = Mat_Pstatj,
                           _["nSamp"    ] = anova.nSamp);

    // clear objects
    anova.releaseTest();
    
    return rs;
}

