/*
 * Copied from Mutils.c (rev.2718) to support M_Matrix_check_class_etc
 */
#if R_VERSION < R_Version(2, 13, 0)
int Matrix_check_class_and_super(SEXP x, const char **valid, SEXP rho)
{
	int ans;
	SEXP cl = getAttrib(x, R_ClassSymbol);
	const char *p_class = CHAR(asChar(cl));
	for (ans = 0; ; ans++) {
		if (!strlen(valid[ans])) // empty string
			break;
		if (!strcmp(p_class, valid[ans])) return ans;
	}
	/* if not found directly, now search the non-virtual super classes :*/
	if(IS_S4_OBJECT(x)) {
		/* now try the superclasses, i.e.,  try   is(x, "....");  superCl :=
		   .selectSuperClasses(getClass("...")@contains, dropVirtual=TRUE)  */
		SEXP classExts, superCl, _call;
		int i;
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
#else
	#define Matrix_check_class_and_super R_check_class_and_super
#endif

/*
 * Copied from Mutils.c (rev.2718) to support M_Matrix_check_class_etc
 */
int Matrix_check_class_etc(SEXP x, const char **valid)
{
	static SEXP s_M_classEnv = NULL;
	SEXP cl = getAttrib(x, R_ClassSymbol), rho = R_GlobalEnv, pkg;
/*     PROTECT(cl); */
	if(!s_M_classEnv)
		s_M_classEnv = install(".M.classEnv");

	pkg = getAttrib(cl, R_PackageSymbol); /* ==R== packageSlot(class(x)) */
	if(!isNull(pkg)) { /* find  rho := correct class Environment */
		SEXP clEnvCall;
		/* need to make sure we find ".M.classEnv" even if Matrix is not
		   attached, but just namespace-loaded: */

		/* Matrix::: does not work here either ... :
		 *          rho = eval(lang2(install("Matrix:::.M.classEnv"), cl), */

		/* Now make this work via .onLoad() hack in ../R/zzz.R  : */
		PROTECT(clEnvCall = lang2(s_M_classEnv, cl));
		rho = eval(clEnvCall, R_GlobalEnv);
		UNPROTECT(1);

		if(!isEnvironment(rho))
			error(_("could not find correct environment; please report!"));
	}
/*     UNPROTECT(1); */
	return Matrix_check_class_and_super(x, valid, rho);
}

/*
 * Introduced in revision 2713
 */
int M_Matrix_check_class_etc(SEXP x, const char *valid[]) {
	RMSK_INNER_NS::printdebug("Calling backported version of M_Matrix_check_class_etc");
	return Matrix_check_class_etc(x, valid);
}

/*
 * Introduced in revision 2713
 */
const char *Matrix_valid_Csparse[]	= { MATRIX_VALID_Csparse, ""};
const char *Matrix_valid_dense[]  	= { MATRIX_VALID_dense, ""};
const char *Matrix_valid_triplet[]	= { MATRIX_VALID_Tsparse, ""};

/*
 * Introduced in revision 2713
 */
bool Matrix_isclass_Csparse(SEXP x) {
	return M_Matrix_check_class_etc(x, Matrix_valid_Csparse) >= 0;
}
bool Matrix_isclass_triplet(SEXP x) {
	return M_Matrix_check_class_etc(x, Matrix_valid_triplet) >= 0;
}
bool Matrix_isclass_dense(SEXP x) {
	return M_Matrix_check_class_etc(x, Matrix_valid_dense) >= 0;
}
