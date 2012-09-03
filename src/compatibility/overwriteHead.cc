#if pkgMatrixVersion_LESS_THAN_OR_EQUAL(1,0,0) // Based on revision 2717

/*
 * Copied from chm_common.c (rev.2718) to support as_cholmod_triplet
 */
// coerce a vector to REAL and copy the result to freshly R_alloc'd memory
static void *RallocedREAL(SEXP x)
{
  SEXP rx = PROTECT(coerceVector(x, REALSXP));
  int lx = LENGTH(rx);
  /* We over-allocate the memory chunk so that it is never NULL. */
  /* The CHOLMOD code checks for a NULL pointer even in the length-0 case. */
  double *ans = (double*)Memcpy((double*)R_alloc(lx + 1, sizeof(double)),
             REAL(rx), lx);
  UNPROTECT(1);
  return (void*)ans;
}

/*
 * Copied from chm_common.c (rev.2718) to support as_cholmod_triplet
 */
static int stype(int ctype, SEXP x)
{
  if ((ctype % 3) == 1) return (*uplo_P(x) == 'U') ? 1 : -1;
  return 0;
}

/*
 * Copied from chm_common.c (rev.2718) to support as_cholmod_triplet
 */
static int xtype(int ctype)
{
  switch(ctype / 3) {
  case 0: /* "d" */
  case 1: /* "l" */
    return CHOLMOD_REAL;
  case 2: /* "n" */
    return CHOLMOD_PATTERN;
  case 3: /* "z" */
    return CHOLMOD_COMPLEX;
  }
  return -1;
}

/*
 * Copied from chm_common.c (rev.2718) to support as_cholmod_triplet
 */
static void *xpt(int ctype, SEXP x)
{
  switch(ctype / 3) {
  case 0: /* "d" */
    return (void *) REAL(GET_SLOT(x, Matrix_xSym));
  case 1: /* "l" */
    return RallocedREAL(GET_SLOT(x, Matrix_xSym));
  case 2: /* "n" */
    return (void *) NULL;
  case 3: /* "z" */
    return (void *) COMPLEX(GET_SLOT(x, Matrix_xSym));
  }
  return (void *) NULL;       /* -Wall */
}

/*
 * Exported in revision 2723
 */
CHM_TR as_cholmod_triplet(CHM_TR ans, SEXP x, Rboolean check_Udiag)
{
  static const char *valid[] = { MATRIX_VALID_Tsparse, ""};
  int *dims = INTEGER(GET_SLOT(x, Matrix_DimSym)),
    ctype = M_Matrix_check_class_etc(x, valid);
  SEXP islot = GET_SLOT(x, Matrix_iSym);
  int m = LENGTH(islot);
  Rboolean do_Udiag = (Rboolean)(check_Udiag && ctype % 3 == 2 && (*diag_P(x) == 'U'));
  if (ctype < 0) error(_("invalid class of object to as_cholmod_triplet"));

  memset(ans, 0, sizeof(cholmod_triplet)); /* zero the struct */

  ans->itype = CHOLMOD_INT;   /* characteristics of the system */
  ans->dtype = CHOLMOD_DOUBLE;
                /* nzmax, dimensions, types and slots : */
  ans->nnz = ans->nzmax = m;
  ans->nrow = dims[0];
  ans->ncol = dims[1];
  ans->stype = stype(ctype, x);
  ans->xtype = xtype(ctype);
  ans->i = (void *) INTEGER(islot);
  ans->j = (void *) INTEGER(GET_SLOT(x, Matrix_jSym));
  ans->x = xpt(ctype, x);

  if(do_Udiag) {

    throw RMSK_INNER_NS::msk_exception("Backported version of as_cholmod_triplet were not able to handle 'do_Udiag' request");

  } /* else :
     * NOTE: if(*diag_P(x) == 'U'), the diagonal is lost (!);
     * ---- that may be ok, e.g. if we are just converting from/to Tsparse,
     *      but is *not* at all ok, e.g. when used before matrix products */

  return ans;
}

/*
 * No export of as_cholmod_triplet until revision 2723
 */
CHM_TR
M_as_cholmod_triplet(CHM_TR ans, SEXP x, Rboolean check_Udiag)
{
  RMSK_INNER_NS::printdebug("Calling backported version of M_as_cholmod_triplet");
  return as_cholmod_triplet(ans, x, check_Udiag);
}

#endif
