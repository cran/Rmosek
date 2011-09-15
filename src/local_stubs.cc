#include <Rdefines.h>
#include <Rversion.h>
//#include <string>

#include "Matrix_stubs.c"

//
// COPY-PASTED FROM MATRIX PACKAGE AS THESE CONSTANTS AND FUNCTIONS ARE NOT EXPORTED
//

const char *CHM_valid_csparse[] =
{"dgCMatrix", "dsCMatrix", "dtCMatrix",
 "lgCMatrix", "lsCMatrix", "ltCMatrix",
 "ngCMatrix", "nsCMatrix", "ntCMatrix",
 "zgCMatrix", "zsCMatrix", "ztCMatrix", ""};

const char *CHM_valid_dense[] =
{"dmatrix", "dgeMatrix",
 "lmatrix", "lgeMatrix",
 "nmatrix", "ngeMatrix",
 "zmatrix", "zgeMatrix", ""};

const char *CHM_valid_triplet[] =
{"dgTMatrix", "dsTMatrix", "dtTMatrix",
 "lgTMatrix", "lsTMatrix", "ltTMatrix",
 "ngTMatrix", "nsTMatrix", "ntTMatrix",
 "zgTMatrix", "zsTMatrix", "ztTMatrix", ""};

const char *CHM_valid_factor[] =
{"dCHMsuper", "dCHMsimpl", "nCHMsuper", "nCHMsimpl", ""};


int Matrix_check_class_and_super(SEXP x, const char *valid[], SEXP rho)
{
    int ans;
    SEXP cl = getAttrib(x, R_ClassSymbol);
    const char *mclass = CHAR(asChar(cl));
    for (ans = 0; ; ans++) {
        if (!strlen(valid[ans]))
            break;
        if (!strcmp(mclass, valid[ans])) return ans;
    }
    /* if not found directly, now search the non-virtual super classes :*/
    if(IS_S4_OBJECT(x)) {
        /* now try the superclasses, i.e.,  try   is(x, "....") : */
        SEXP classExts, superCl, _call;
        int i;
/*      PROTECT(cl); */
        PROTECT(_call = lang2(install("getClassDef"), cl));
        classExts = GET_SLOT(eval(_call, rho),
                                     install("contains"));
        UNPROTECT(1);
        PROTECT(classExts);
        PROTECT(_call = lang3(install(".selectSuperClasses"), classExts,
                              /* dropVirtual = */ ScalarLogical(1)));
        superCl = eval(_call, rho);
        UNPROTECT(2);
        PROTECT(superCl);
        for(i=0; i < length(superCl); i++) {
            const char *s_class = CHAR(STRING_ELT(superCl, i));
            for (ans = 0; ; ans++) {
                if (!strlen(valid[ans]))
                    break;
                if (!strcmp(s_class, valid[ans])) {
                    UNPROTECT(1);
                    return ans;
                }
            }
        }
        UNPROTECT(1);
    }
    return -1;
}

int Matrix_check_class_etc(SEXP x, const char *valid[])
{
    SEXP cl = getAttrib(x, R_ClassSymbol), rho = R_GlobalEnv,
        M_classEnv_sym = install(".M.classEnv"), pkg;

/*     PROTECT(cl); */

#if defined(R_VERSION) && R_VERSION >= R_Version(2, 10, 0)
    pkg = getAttrib(cl, R_PackageSymbol); /* ==R== packageSlot(class(x)) */
#else
    pkg = getAttrib(cl, install("package"));
#endif
    if(!isNull(pkg)) { /* find  rho := correct class Environment */
        SEXP clEnvCall;
        /* need to make sure we find ".M.classEnv" even if Matrix is not
           attached, but just namespace-loaded: */

        /* Matrix::: does not work here either ... :
         *          rho = eval(lang2(install("Matrix:::.M.classEnv"), cl), */

        /* Now make this work via .onLoad() hack in ../R/zzz.R  : */
        PROTECT(clEnvCall = lang2(M_classEnv_sym, cl));
        rho = eval(clEnvCall, R_GlobalEnv);
        UNPROTECT(1);

        if(!isEnvironment(rho))
            throw "could not find correct environment; please report!";
    }
/*     UNPROTECT(1); */
    return Matrix_check_class_and_super(x, valid, rho);
}

bool Matrix_isclass_csparse(SEXP x) {
	return Matrix_check_class_etc(x, CHM_valid_csparse) >= 0;
}

bool Matrix_isclass_triplet(SEXP x) {
	return Matrix_check_class_etc(x, CHM_valid_triplet) >= 0;
}

bool Matrix_isclass_dense(SEXP x) {
	return Matrix_check_class_etc(x, CHM_valid_dense) >= 0;
}

bool Matrix_isclass_factor(SEXP x) {
	return Matrix_check_class_etc(x, CHM_valid_factor) >= 0;
}
