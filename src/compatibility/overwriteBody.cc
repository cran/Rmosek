#if R_VERSION <= R_Version(2, 10, 0)

	/*
	 * Bug fixed in revision 2715
	 */
	CHM_TR attribute_hidden
	M_as_cholmod_triplet(CHM_TR ans, SEXP x, Rboolean check_Udiag)
	{
		static CHM_TR(*fun)(CHM_TR,SEXP,Rboolean)= NULL;
		if(fun == NULL)
		fun = (CHM_TR(*)(CHM_TR,SEXP,Rboolean))
			R_GetCCallable("Matrix", "as_cholmod_triplet");
		return fun(ans, x, check_Udiag);
	}

	/*
	 * Bug fixed in revision 2715
	 */
	SEXP attribute_hidden
	M_chm_sparse_to_SEXP(const_CHM_SP a, int dofree,
				 int uploT, int Rkind, const char *diag, SEXP dn)
	{
		static SEXP(*fun)(const_CHM_SP,int,int,int,const char*,SEXP) = NULL;
		if(fun == NULL)
		fun = (SEXP(*)(const_CHM_SP,int,int,int,const char*,SEXP))
			R_GetCCallable("Matrix", "chm_sparse_to_SEXP");
		return fun(a, dofree, uploT, Rkind, diag, dn);
	}

#endif
