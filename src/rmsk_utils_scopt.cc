#define SCDEBUG 0
#define R_NO_REMAP
#include "rmsk_utils_scopt.h"

#include "rmsk_obj_arguments.h"
#include "rmsk_utils_mosek.h"
#include "rmsk_utils_sexp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

___RMSK_INNER_NS_START___

// ----------------------------------------
// SCOPT methods - User-defined unary operators
// ----------------------------------------

static MSKrescodee
evalopr(MSKscopre opr,
        double f,
        double g,
        double h,
        double xj,
        double *fxj,
        double *grdfxj,
        double *hesfxj)
/* Purpose: Evaluates an operator and its derivatives.
            fxj:    Is the function value
            grdfxj: Is the first derivative.
            hexfxj: Is the second derivative.
 */
{
  double rtemp;

  switch ( opr )
  {
    case MSK_OPR_ENT:
#if SCDEBUG && 1
      printf("%s(%d): ENT (%lf > 0)\n",__FILE__,__LINE__,xj);
#endif
      if ( xj<=0.0 )
      {
        return ( MSK_RES_ERR_USER_NLO_FUNC );
      }

      if ( fxj )
        fxj[0] = f*xj*log(xj);

      if ( grdfxj )
        grdfxj[0] = f*(1.0+log(xj));

      if ( hesfxj )
        hesfxj[0] = f/xj;
      break;
    case MSK_OPR_EXP:
#if SCDEBUG && 1
      printf("%s(%d): EXP\n",__FILE__,__LINE__);
#endif
      if ( fxj || grdfxj || hesfxj ) 
      {  
        rtemp = exp(g*xj+h); 

        if ( fxj )
          fxj[0] = f*rtemp;

        if ( grdfxj )
          grdfxj[0] = f*g*rtemp;

        if ( hesfxj )
          hesfxj[0] = f*g*g*rtemp;
      } 
      break;
    case MSK_OPR_LOG:
      rtemp = g*xj+h;
#if SCDEBUG && 1
      printf("%s(%d): LOG (%lf > 0)\n",__FILE__,__LINE__,rtemp);
#endif
      if ( rtemp<=0.0 )
      {
        return ( MSK_RES_ERR_USER_NLO_FUNC );
      }

      if ( fxj )
        fxj[0] = f*log(rtemp);

      if ( grdfxj )
        grdfxj[0] = (f*g)/(rtemp);

       if ( hesfxj )
        hesfxj[0] = -(f*g*g)/(rtemp*rtemp);
      break;
    case MSK_OPR_POW:
#if SCDEBUG && 1
      printf("%s(%d): POW\n",__FILE__,__LINE__);
#endif
      if ( fxj )
        fxj[0] = f*pow(xj+h,g);

      if ( grdfxj )
        grdfxj[0] = f*g*pow(xj+h,g-1.0);

       if ( hesfxj )
       { 
         hesfxj[0] = f*g*(g-1.0)*pow(xj+h,g-2.0);
       } 
      break;
    default:
      printoutput("Internal error in evalopr (rmsk_utils_scopt.cc): Operator with MSKscopre = " + tostring(opr) + " not recognized!", typeERROR);
      return ( MSK_RES_ERR_USER_NLO_FUNC );
  }

  return ( MSK_RES_OK );
} /* evalopr */


MSKbooleant validate_scopr(MSKscopre opr, int i, int j, double f, double g, double h,
		problem_type &probin)
{

	// Check i and j
	if (! (0 <= i && i <= probin.numcon) )
		throw msk_exception("Constraint index was out of bounds");

	if (! (1 <= j && j <= probin.numvar) )
		throw msk_exception("Variable index was out of bounds");

	// Check coefficients
	switch ( opr )
	{
	case MSK_OPR_ENT:
		if (g != 0.0 || h != 0.0)
			throw msk_exception("Coefficients 'h' and 'g' should be zero or have an empty definition in operator of type 'ENT'");
		break;

	case MSK_OPR_EXP:
	case MSK_OPR_LOG:
	case MSK_OPR_POW:
		break;

	default:
		throw msk_exception("Internal error in validate_scopr (rmsk_utils_scopt.cc): Operator with MSKscopre = " + tostring(opr) + " not recognized!");
	}

	//
	// TODO: Check variable bounds (warn if not safe)
	//
//	double blx = NUMERIC_ELT(probin.blx, j - 1);
//	double bux = NUMERIC_ELT(probin.bux, j - 1);
//	MSKboundkeye bkx; set_boundkey(blx, bux, &bkx);
//	OR ALTERNATIVELY
//	errcatch( MSK_getbound(task, MSK_ACC_VAR, j, &bkx, &blx, &bux) );


	if (i == 0) {
		//
		// Check term-wise convexity in objective (warn if not safe)
		//

//		MSKobjsensee sense = probin.sense;
//		OR ALTERNATIVELY
//		errcatch( MSK_getobjsense(task, &sense) );

	} else {
		//
		// Check term-wise convexity in constraint (warn if not safe)
		//
//		double blc = NUMERIC_ELT(probin.blc, i - 1);
//		double buc = NUMERIC_ELT(probin.buc, i - 1);
//		MSKboundkeye bkc; set_boundkey(blc, buc, &bkc);
//		OR ALTERNATIVELY
//		errcatch( MSK_getbound(task, MSK_ACC_CON, i, &bkc, &blc, &buc) );

	}

	return true;

} /* validate_scopr */


MSKbooleant MSK_scsymnamtovalue (
    MSKCONST char * name,
    MSKscopre * value)
{
  if ( strcmp(name, "MSK_OPR_ENT") == 0 ) { *value = MSK_OPR_ENT; } else
  if ( strcmp(name, "MSK_OPR_EXP") == 0 ) { *value = MSK_OPR_EXP; } else
  if ( strcmp(name, "MSK_OPR_LOG") == 0 ) { *value = MSK_OPR_LOG; } else
  if ( strcmp(name, "MSK_OPR_POW") == 0 ) { *value = MSK_OPR_POW; } else
    return false;

  return true;
} /* MSK_scsymnamtovalue */


MSKbooleant MSK_scvaltosymnam (
	MSKscopre whichvalue,
    char * symbolicname)
{
  switch ( whichvalue )
  {
    case MSK_OPR_ENT:
      strcpy(symbolicname, "MSK_OPR_ENT");  break;

    case MSK_OPR_EXP:
      strcpy(symbolicname, "MSK_OPR_EXP");  break;

    case MSK_OPR_LOG:
      strcpy(symbolicname, "MSK_OPR_LOG");  break;

    case MSK_OPR_POW:
      strcpy(symbolicname, "MSK_OPR_POW");  break;

    default:
    	throw msk_exception("Internal error in MSK_scvaltosymnam (rmsk_utils_scopt.cc): Operator with MSKscopre = " + tostring(whichvalue) + " not recognized!");
  }

  return true;
} /* MSK_scvaltosymnam */


// ----------------------------------
// SCOPT methods - helpers for scstruc
// ----------------------------------

static void scgrdobjstruc(nlhand_t nlh,
                          int      *nz,
                          int      *sub)
/* Purpose: Compute number of nonzeros and sparsity
            pattern of the gradiant of the objective function.
 */
{
  int j,k,
      *zibuf;

  zibuf = nlh->zibuf;

  if ( nz )
  {
    nz[0] = 0;
    for(k=0; k<nlh->numopro; ++k)
    {
      j = nlh->oprjo[k];

      if ( !zibuf[j] )
      {
        /* A new nonzero in the gradiant has been located. */

        if ( sub )
          sub[nz[0]] = j;

                 ++ nz[0];
        zibuf[j]  = 1;
      }
    }

    /* Zero zibuf again. */
    for(k=0; k<nlh->numopro; ++k)
    {
      j        = nlh->oprjo[k];
      zibuf[j] = 0;
    }
  }
} /* scgrdobjstruc */

static void scgrdconistruc(nlhand_t nlh,
                           int      i,
                           int      *nz,
                           int      *sub)
{
  int j,k,
      *zibuf;

  zibuf = nlh->zibuf;

  nz[0] = 0;
  if ( nlh->ptrc )
  {
    for(k=nlh->ptrc[i]; k<nlh->ptrc[i+1]; ++k)
    {
      j = nlh->oprjc[nlh->subc[k]];

      if ( !zibuf[j] )
      {
        /* A new nonzero in the gradiant has been located. */

        if ( sub )
          sub[nz[0]] = j;

                 ++ nz[0];
        zibuf[j]  = 1;
      }
    }

    /* Zero zibuf again. */
    for(k=nlh->ptrc[i]; k<nlh->ptrc[i+1]; ++k)
    {
      j        = nlh->oprjc[nlh->subc[k]];
      zibuf[j] = 0;
    }
  }
} /* scgrdconistruc */

static MSKrescodee 
schesstruc(nlhand_t nlh,
                      int      yo,
                      int      numycnz,
                      int      *ycsub,
                      int      *nz,
                      int      *sub)
{
  int i,j,k,p,
      *zibuf;

  zibuf = nlh->zibuf;

  nz[0] = 0;

  if ( yo )
  {
    for(k=0; k<nlh->numopro; ++k)
    {
      j = nlh->oprjo[k];

      if ( !zibuf[j] )
      {
        /* A new nonzero in the gradiant has been located. */

        if ( sub )
          sub[nz[0]] = j;

                 ++ nz[0];
        zibuf[j]  = 1;
      }
    }
  }

  if ( nlh->ptrc )
  {
    for(p=0; p<numycnz; ++p)
    {
      i = ycsub[p];
      for(k=nlh->ptrc[i]; k<nlh->ptrc[i+1]; ++k)
      {
        j = nlh->oprjc[nlh->subc[k]];

        if ( !zibuf[j] )
        {
          /* A new nonzero in the gradiant has been located. */

          if ( sub )
            sub[nz[0]] = j;

                 ++ nz[0];
          zibuf[j]  = 1;
        }
      }
    }
  }

  if ( yo )
  {
    for(k=0; k<nlh->numopro; ++k)
    {
      j        = nlh->oprjo[k];
      zibuf[j] = 0;
    }
  }

  if ( nlh->ptrc )
  {
    for(p=0; p<numycnz; ++p)
    {
      i = ycsub[p];
      for(k=nlh->ptrc[i]; k<nlh->ptrc[i+1]; ++k)
      {
        j        = nlh->oprjc[nlh->subc[k]];
        zibuf[j] = 0;
      }
    }
  }

  return ( MSK_RES_OK );
} /* schestruc */

MSKintt MSKAPI 
SCstruc(void *nlhandle,
        MSKintt   *numgrdobjnz,
        MSKidxt   *grdobjsub,
        MSKintt    i,
        MSKintt   *convali,
        MSKintt   *grdconinz,
        MSKidxt   *grdconisub,
        MSKintt    yo,
        MSKintt    numycnz,
        MSKidxt   *ycsub,
        MSKlidxt   maxnumhesnz,
        MSKlidxt  *numhesnz,
        MSKidxt   *hessubi,
        MSKidxt   *hessubj)
     /* Purpose: Provide information to MOSEK about the
        problem structure and sparsity.
     */
{
  int      k,itemp;
  nlhand_t self;

  self = (nlhand_t) nlhandle;
  if ( numgrdobjnz )
    scgrdobjstruc(self,numgrdobjnz,grdobjsub);

  if ( convali || grdconinz )
  {
    scgrdconistruc(self,i,&itemp,grdconisub);
      
    if ( convali )
      convali[0] = itemp>0;

    if ( grdconinz )
      grdconinz[0] = itemp;
  }

  if ( numhesnz )
  {
    schesstruc(self,yo,numycnz,ycsub,numhesnz,hessubi);

    if ( numhesnz[0]>maxnumhesnz && hessubi )
    {
      printoutput("Internal error in SCstruc (rmsk_utils_scopt.cc): Hessian size error.", typeERROR);
      return ( MSK_RES_ERR_USER_NLO_FUNC );
    }

    if ( hessubi )
      for(k=0; k<numhesnz[0]; ++k)
        hessubj[k] = hessubi[k];
  }

  if (self->log)
  {
    FILE * out = self->log;
    int j;
 
    fprintf (out,"-[ STRUC ]------------------------------------------------\n");
    if (numhesnz && hessubi && hessubj)
    {
      fprintf (out,"hessubi = [ ");
      for (j = 0; j < *numhesnz; ++j) fprintf (out, "%d ", hessubi[j]); fprintf(out,"]\n");
      fprintf (out,"hessubj = [ ");
      for (j = 0; j < *numhesnz; ++j) fprintf (out, "%d ", hessubj[j]); fprintf(out,"]\n");
    }
  }

  return ( MSK_RES_OK );
} /* scstruc */


// ----------------------------------
// SCOPT METHODS - helpers for sceval
// ----------------------------------

static MSKrescodee 
scobjeval(nlhand_t nlh,
                     double   *x,
                     double   *objval,
                     int      *grdnz,
                     int      *grdsub,
                     double   *grdval)
     /* Purpose: Compute number objective value and the gradient.
      */
{
  MSKrescodee r=MSK_RES_OK;
  int    j,k,
         *zibuf;
  double fx,grdfx,
         *zdbuf;

#if SCDEBUG  
  printf("%s(%d) SC obj eval\n",__FILE__,__LINE__);
#endif

  zibuf = nlh->zibuf;
  zdbuf = nlh->zdbuf;

  if ( objval )
    objval[0] = 0.0;

  if ( grdnz )
    grdnz[0] = 0;

  for(k=0; k<nlh->numopro && r==MSK_RES_OK; ++k)
  {
#if SCDEBUG  
    printf("%s(%d) k = %d, opr = %d\n",__FILE__,__LINE__,k,nlh->opro[k]);
#endif
    j = nlh->oprjo[k];

    r = evalopr(nlh->opro[k],nlh->oprfo[k],nlh->oprgo[k],nlh->oprho[k],x[j],&fx,&grdfx,NULL);
    if ( r==MSK_RES_OK )
    {
      if ( objval )
      objval[0] += fx;

      if ( grdnz )
      {
        zdbuf[j] += grdfx;

        if ( !zibuf[j] )
        {
          /* A new nonzero in the gradiant has been located. */

          grdsub[grdnz[0]]  = j;
          zibuf[j]          = 1;
          ++ grdnz[0];
        }
      }
    }
  }

  if ( grdnz!=NULL )
  {
    /* Buffers should be zeroed. */

    for(k=0; k<grdnz[0]; ++k)
    {
      j = grdsub[k];

      if ( grdval )
      grdval[k] = zdbuf[j];

      zibuf[j] = 0;
      zdbuf[j] = 0.0;
    }
  }

#if SCDEBUG  
  printf("%s(%d) END SC obj eval, r = %d\n",__FILE__,__LINE__,r);
#endif

  return ( r );
} /* scobjeval */

static MSKrescodee
scgrdconeval(nlhand_t nlh,
                        int      i,
                        double   *x,
                        double   *objval,
                        int      grdnz,
                        int      *grdsub,
                        double   *grdval)
/* Purpose: Compute number value and the gradient of constraint i.
 */
{
  MSKrescodee r=MSK_RES_OK;
  int    j,k,p,gnz,
         *ibuf,*zibuf;
  double fx,grdfx,
         *zdbuf;

#if SCDEBUG  
  printf("%s(%d) SC con eval #%d\n",__FILE__,__LINE__,i);
#endif

  ibuf  = nlh->ibuf; 
  zibuf = nlh->zibuf;
  zdbuf = nlh->zdbuf;

  if ( objval )
  objval[0] = 0.0;



  if ( nlh->ptrc )
  {
    gnz = 0;
    for(p=nlh->ptrc[i]; p<nlh->ptrc[i+1] && r==MSK_RES_OK; ++p)
    {
      k = nlh->subc[p];
      j = nlh->oprjc[k];

      r = evalopr(nlh->oprc[k],nlh->oprfc[k],nlh->oprgc[k],nlh->oprhc[k],x[j],&fx,&grdfx,NULL);
#if SCDEBUG  
      printf("%s(%d) k = %d, opr = %d, r = %d\n",__FILE__,__LINE__,i,nlh->oprc[k],r);
#endif
  
      if ( r==MSK_RES_OK )
      {
        if ( objval )
        objval[0] += fx;

        if ( grdnz>0 )
        {
          zdbuf[j] += grdfx;

          if ( !zibuf[j] )
          {
            /* A new nonzero in the gradiant has been located. */

            ibuf[gnz]  = j;
            zibuf[j]   = 1;
            ++ gnz; 
          }
        }
      }
    }

    if ( grdval!=NULL )
    {
      /* Setup gradiant. */
      for(k=0; k<grdnz; ++k)
      {
        j = grdsub[k];
        grdval[k] = zdbuf[j];
      }
    }

    for(k=0; k<gnz; ++k)
    {
      j        = ibuf[k];
      zibuf[j] = 0;
      zdbuf[j] = 0.0;
    }
  }
  else if ( grdval )
  {
    for(k=0; k<grdnz; ++k)
    grdval[k] = 0.0;
  }

#if SCDEBUG  
  printf("%s(%d) END SC con eval #%d, r = %d\n",__FILE__,__LINE__,i,r);
#endif

  return ( r );
} /* scgrdconeval */

MSKintt MSKAPI 
SCeval(MSKuserhandle_t nlhandle,
       MSKrealt          *xx,
       MSKrealt           yo,
       MSKrealt          *yc,
       MSKrealt          *objval,
       MSKintt           *numgrdobjnz,
       MSKidxt           *grdobjsub,
       MSKrealt          *grdobjval,
       MSKintt            numi,
       MSKidxt           *subi,
       MSKrealt          *conval,
       MSKidxt           *grdconptrb,
       MSKidxt           *grdconptre,
       MSKidxt           *grdconsub,
       MSKrealt          *grdconval,
       MSKrealt          *grdlag,
       MSKlintt           maxnumhesnz,
       MSKlintt          *numhesnz,
       MSKidxt           *hessubi,
       MSKidxt           *hessubj,
       MSKrealt          *hesval)
{
  double   fx,grdfx,hesfx;
  MSKrescodee r=MSK_RES_OK;
  int      i,j,k,l,p,numvar,numcon,
           *zibuf;
  nlhand_t self;

  self    = (nlhand_t) nlhandle;

  numcon = self->numcon; 
  numvar = self->numvar;

  r      = scobjeval(self,xx,objval,numgrdobjnz,grdobjsub,grdobjval);

  for(k=0; k<numi && r==MSK_RES_OK; ++k)
  {
    i = subi[k];

    r = scgrdconeval(self,i,xx,&fx,
                     grdconsub==NULL ? 0    : grdconptre[k]-grdconptrb[k],
                     grdconsub==NULL ? NULL : grdconsub+grdconptrb[k],
                     grdconval==NULL ? NULL : grdconval+grdconptrb[k]);

    if ( r==MSK_RES_OK )
    {
      if ( conval )
      conval[k]  = fx;
    }
  }

  if ( grdlag && r==MSK_RES_OK )
  {
    /* Compute and store the gradiant of the Lagrangian.
     * Note it is stored as a dense vector.
     */

    for(j=0; j<numvar; ++j)
      grdlag[j] = 0.0;

    if ( yo!=0.0 )
    {
      for(k=0; k<self->numopro && r==MSK_RES_OK; ++k)
      {
        j = self->oprjo[k];
        r = evalopr(self->opro[k],self->oprfo[k],self->oprgo[k],self->oprho[k],xx[j],NULL,&grdfx,NULL);
          grdlag[j] += yo*grdfx;
      }
    }

    if ( self->ptrc )
    {
      for(l=0; l<numi && r==MSK_RES_OK; ++l)
      {
        i = subi[l];
        for(p=self->ptrc[i]; p<self->ptrc[i+1] && r==MSK_RES_OK; ++p)
        {
           k = self->subc[p];
           j = self->oprjc[k];

           r = evalopr(self->oprc[k],self->oprfc[k],self->oprgc[k],self->oprhc[k],xx[j],NULL,&grdfx,NULL);

          grdlag[j] -= yc[i]*grdfx;
        }
      }
    }
  }

  if ( maxnumhesnz && r==MSK_RES_OK )
  {
    /* Compute and store the Hessian of the Lagrangien.
     */

    zibuf       = self->zibuf;
    numhesnz[0] = 0;
    if ( yo!=0.0 )
    {
      for(k=0; k<self->numopro && r==MSK_RES_OK; ++k)
      {
        j = self->oprjo[k];
        r = evalopr(self->opro[k],self->oprfo[k],self->oprgo[k],self->oprho[k],xx[j],NULL,NULL,&hesfx);
        if ( !zibuf[j] )
        {
          ++ numhesnz[0];
          zibuf[j]             = numhesnz[0];
          hessubi[zibuf[j]-1]  = j;
          hesval[zibuf[j]-1]   = 0.0;
        }
        hesval[zibuf[j]-1] += yo*hesfx;
      }
    }

    if ( self->ptrc )
    {
      for(l=0; l<numi && r==MSK_RES_OK; ++l)
      {
        i = subi[l];
        for(p=self->ptrc[i]; p<self->ptrc[i+1] && r==MSK_RES_OK; ++p)
        {
          k = self->subc[p];
          j = self->oprjc[k];

          r = evalopr(self->oprc[k],self->oprfc[k],self->oprgc[k],self->oprhc[k],xx[j],NULL,NULL,&hesfx);

          if ( !zibuf[j] )
          {
            ++ numhesnz[0];
            zibuf[j]             = numhesnz[0];
            hesval[zibuf[j]-1]   = 0.0;
            hessubi[zibuf[j]-1]  = j;
          }
          hesval[zibuf[j]-1] -= yc[i]*hesfx;
        }
      }
    }

    if ( numhesnz[0]>maxnumhesnz )
    {
      printoutput("Internal error in SCeval (rmsk_utils_scopt.cc): Hessian evalauation error.", typeERROR);
      return ( MSK_RES_ERR_USER_NLO_FUNC );
    }

    for(k=0; k<numhesnz[0]; ++k)
    {
      j          = hessubi[k];
      hessubj[k] = j;
      zibuf[j]   = 0;
    }
  }

  if (self->log)
  {
    FILE * out = self->log;
    int i,j;
 
    fprintf (out,"-[ EVAL ]-------------------------------------------------\n");
    fprintf (out,"xx = [ ");
    for (j = 0; j < self->numvar; ++j) fprintf (out, "%lf ", xx[j]); fprintf(out,"]\n");

    if (numgrdobjnz && grdobjsub && grdobjval)
    {
      fprintf (out,"grdobjsub = [ ");
      for (j = 0; j < *numgrdobjnz; ++j) fprintf (out, "%d ", grdobjsub[j]); fprintf(out,"]\n");
      fprintf (out,"grdobjval = [ ");
      for (j = 0; j < *numgrdobjnz; ++j) fprintf (out, "%lf ", grdobjval[j]); fprintf(out,"]\n");
    }

    if (grdconptrb && grdconptre && grdconsub && grdconval && subi)
    {
      for (i = 0; i < numi; ++i)
      {
        fprintf (out,"grdconsub[%d] = [ ", subi[i]);
        for (j = grdconptrb[i]; j < grdconptre[i]; ++j) fprintf (out, "%d ", grdconsub[j]); fprintf(out,"]\n");
        fprintf (out,"grdconval[%d] = [ ", subi[i]);
        for (j = grdconptrb[i]; j < grdconptre[i]; ++j) fprintf (out, "%lf ", grdconval[j]); fprintf(out,"]\n");
      }
    }

    if (numhesnz && hesval && hessubi && hessubj)
    {
      fprintf (out,"hessubi = [ ");
      for (j = 0; j < *numhesnz; ++j) fprintf (out, "%d ", hessubi[j]); fprintf(out,"]\n");
      fprintf (out,"hessubj = [ ");
      for (j = 0; j < *numhesnz; ++j) fprintf (out, "%d ", hessubj[j]); fprintf(out,"]\n");
      fprintf (out,"hesval = [ ");
      for (j = 0; j < *numhesnz; ++j) fprintf (out, "%lf ", hesval[j]); fprintf(out,"]\n");
    }
  }


#if SCDEBUG
  if (self->log)
    printf("%s(%d) eval r = %d\n",__FILE__,__LINE__,r);
#endif
  return ( r );
} /* sceval */

//MSKnlgetvafunc scopt_callback_function_handle::sceval = sceval;


// ----------------------------------
// CALLABLE FUNCTIONS
// ----------------------------------

MSKrescodee MSKAPI
MSK_scbegin(MSKtask_t task,
            int        numopro,
            MSKscopre *opro,
            int       *oprjo,
            double    *oprfo,
            double    *oprgo,
            double    *oprho,
            int       numoprc,
            MSKscopre *oprc,
            int       *opric,
            int       *oprjc,
            double    *oprfc,
            double    *oprgc,
            double    *oprhc,
            nlhand_t  nlh)
{
  int         itemp,k,sum;
  MSKrescodee r=MSK_RES_OK;

  if ( nlh )
  {
    // Clear work buffers
    nlh->log = NULL;

    memset((void*)nlh->ibuf, 0, sizeof(int) * nlh->numvar);
    memset((void*)nlh->zibuf, 0, sizeof(int) * nlh->numvar);
    memset((void*)nlh->zdbuf, 0, sizeof(double) * nlh->numvar);

    memset((void*)nlh->ptrc, 0, sizeof(int) * (nlh->numcon+1));
    memset((void*)nlh->subc, 0, sizeof(int) * numoprc);


    if ( nlh->numcon && numoprc>0 )
    {
  #if SCDEBUG
      printf("Setup ptrc and subc\n");
  #endif

      for(k=0; k<numoprc; ++k)
      ++ nlh->ptrc[opric[k]];

      sum = 0;
      for(k=0; k<=nlh->numcon; ++k)
      {
        itemp         = nlh->ptrc[k];
        nlh->ptrc[k]  = sum;
        sum          += itemp;
      }

      for(k=0; k<numoprc; ++k)
      {
        nlh->subc[nlh->ptrc[opric[k]]]  = k;
        ++ nlh->ptrc[opric[k]];
      }

      for(k=nlh->numcon; k; --k)
      nlh->ptrc[k] = nlh->ptrc[k-1];

      nlh->ptrc[0] = 0;

  #if SCDEBUG
      {
        int p;
        for(k=0; k<nlh->numcon; ++k)
        {
          printf("ptrc[%d]: %d subc: \n",k,nlh->ptrc[k]);
          for(p=nlh->ptrc[k]; p<nlh->ptrc[k+1]; ++p)
          printf(" %d",nlh->subc[p]);
          printf("\n");
        }

      }
  #endif
    }

  }
  else
  r = MSK_RES_ERR_SPACE;

  return ( r );
} /* MSK_scbegin */


MSKrescodee MSKAPI
MSK_scend(nlhand_t  nlh)
/* Purpose: Close internal debugging session. */
{
  if ( nlh ) {
    if ( nlh->log ) {
      fclose(nlh->log);
    }
  }

  return ( MSK_RES_OK );
} /* MSK_scend */


___RMSK_INNER_NS_END___
