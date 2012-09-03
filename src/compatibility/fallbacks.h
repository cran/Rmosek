/*
 * Introduced in revision 2716
 */
int M_Matrix_check_class_etc(SEXP x, const char **valid);
bool Matrix_isclass_Csparse(SEXP x);
bool Matrix_isclass_triplet(SEXP x);
bool Matrix_isclass_dense(SEXP x);

/*
 * Introduced in revision 2715
 */
#ifndef MATRIX_VALID_dense
#define MATRIX_VALID_dense			\
  "dmatrix", "dgeMatrix",			\
  "lmatrix", "lgeMatrix",			\
  "nmatrix", "ngeMatrix",			\
  "zmatrix", "zgeMatrix"
#endif

/*
 * Introduced in revision 2715
 */
#ifndef MATRIX_VALID_Csparse
#define MATRIX_VALID_Csparse			\
 "dgCMatrix", "dsCMatrix", "dtCMatrix",		\
 "lgCMatrix", "lsCMatrix", "ltCMatrix",		\
 "ngCMatrix", "nsCMatrix", "ntCMatrix",		\
 "zgCMatrix", "zsCMatrix", "ztCMatrix"
#endif

/*
 * Introduced in revision 2715
 */
#ifndef MATRIX_VALID_Tsparse
#define MATRIX_VALID_Tsparse			\
 "dgTMatrix", "dsTMatrix", "dtTMatrix",		\
 "lgTMatrix", "lsTMatrix", "ltTMatrix",		\
 "ngTMatrix", "nsTMatrix", "ntTMatrix",		\
 "zgTMatrix", "zsTMatrix", "ztTMatrix"
#endif

/*
 * Introduced in revision 2715
 */
#ifndef MATRIX_VALID_Rsparse
#define MATRIX_VALID_Rsparse			\
 "dgRMatrix", "dsRMatrix", "dtRMatrix",		\
 "lgRMatrix", "lsRMatrix", "ltRMatrix",		\
 "ngRMatrix", "nsRMatrix", "ntRMatrix",		\
 "zgRMatrix", "zsRMatrix", "ztRMatrix"
#endif


#if pkgMatrixVersion_LESS_THAN_OR_EQUAL(1,0,0) // Based on revision 2717
  /*
   * Copied from init.c (rev.2718) to support as_cholmod_triplet
   */
  static const SEXP Matrix_DimSym = Rf_install("Dim");
  static const SEXP Matrix_iSym = Rf_install("i");
  static const SEXP Matrix_jSym = Rf_install("j");
  static const SEXP Matrix_xSym = Rf_install("x");
  static const SEXP Matrix_uploSym = Rf_install("uplo");
  static const SEXP Matrix_diagSym = Rf_install("diag");

  /*
   * Copied from Mutils.h (rev.2718) to support as_cholmod_triplet
   */
  #define uplo_P(_x_) CHAR(STRING_ELT(GET_SLOT(_x_, Matrix_uploSym), 0))
  #define diag_P(_x_) CHAR(STRING_ELT(GET_SLOT(_x_, Matrix_diagSym), 0))

  /*
   * Copied from Mutils.h (rev.2718) to support M_Matrix_check_class_etc
   */
  #ifdef ENABLE_NLS
  #include <libintl.h>
  #define _(String) dgettext ("Matrix", String)
  #else
  #define _(String) (String)
  #endif
#endif
