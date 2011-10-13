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
