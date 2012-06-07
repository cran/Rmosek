#ifndef RMSK_SCOPT_H
#define RMSK_SCOPT_H

#include "rmsk_obj_sexp.h"
#include "rmsk_msg_mosek.h"
#include "rmsk_namespace.h"

#include <memory>

___RMSK_INNER_NS_START___
class problem_type;

// ----------------------------------------
// SCOPT methods - Definition of unary operators
// ----------------------------------------
typedef
enum MSKscopre_enum
{
	MSK_OPR_ENT = 0,
	MSK_OPR_EXP = 1,
	MSK_OPR_LOG = 2,
	MSK_OPR_POW = 3
}
MSKscopre;

const bool
MSKscopr_using_coef[][3] = {
		//f		g		h
		{true,	false,	false},		// MSK_OPR_ENT
		{true,	true,	true},		// MSK_OPR_EXP
		{true,	true,	true},		// MSK_OPR_LOG
		{true,	true,	true}		// MSK_OPR_POW
};


/*
 * The structure defining the non-linear part of a problem.
 */
typedef struct
{
	/*
	 * Data structure for storing
	 * data about the nonlinear
	 * functions.
	 */

	int        numcon;      /* Number of constraints. */
	int        numvar;      /* Number of variables.   */

	int                   numopro;
	auto_array<MSKscopre> opro;
	auto_array<int>       oprjo;
	auto_array<double>    oprfo;
	auto_array<double>    oprgo;
	auto_array<double>    oprho;

	int                   numoprc;
	auto_array<MSKscopre> oprc;
	auto_array<int>       opric;
	auto_array<int>       oprjc;
	auto_array<double>    oprfc;
	auto_array<double>    oprgc;
	auto_array<double>    oprhc;

	/* Internal work vectors */
	auto_array<int>       ptrc;
	auto_array<int>       subc;

	auto_array<int>       ibuf;
	auto_array<int>       zibuf;
	auto_array<double>    zdbuf;

	/* Internal Log */
	FILE * log;

} nlhandt;

typedef nlhandt * nlhand_t;
//typedef void * schand_t;


/* Initialize internal working vectors. No allocation takes place.
 * No attachment of non-linear callbacks to the task takes place. */
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
            nlhand_t  nlh);

/* Close internal debugging session. */
MSKrescodee MSKAPI
MSK_scend(nlhand_t  nlh);

MSKbooleant
MSK_scsymnamtovalue(
    MSKCONST char * name,
    MSKscopre * value);

MSKbooleant
MSK_scvaltosymnam (
	MSKscopre whichvalue,
    char * symbolicname);

void
validate_scopr(MSKscopre opr, int i, int j, double f, double g, double h,
		problem_type &probin, std::string oprname);


MSKintt MSKAPI
SCeval(MSKuserhandle_t nlhandle,
   MSKCONST MSKrealt   *xx,
			MSKrealt    yo,
   MSKCONST MSKrealt   *yc,
			MSKrealt   *objval,
			MSKintt    *numgrdobjnz,
			MSKidxt    *grdobjsub,
			MSKrealt   *grdobjval,
			MSKintt     numi,
   MSKCONST MSKidxt    *subi,
   	   	    MSKrealt   *conval,
   MSKCONST MSKidxt    *grdconptrb,
   MSKCONST MSKidxt    *grdconptre,
   MSKCONST MSKidxt    *grdconsub,
       	    MSKrealt   *grdconval,
       	    MSKrealt   *grdlag,
       	    MSKlintt    maxnumhesnz,
       	    MSKlintt   *numhesnz,
       	    MSKidxt    *hessubi,
       	    MSKidxt    *hessubj,
       	    MSKrealt   *hesval);

MSKintt MSKAPI
SCstruc(MSKuserhandle_t  nlhandle,
			MSKintt   *numgrdobjnz,
			MSKidxt   *grdobjsub,
			MSKintt    i,
			MSKintt   *convali,
			MSKintt   *grdconinz,
			MSKidxt   *grdconisub,
			MSKintt    yo,
			MSKintt    numycnz,
   MSKCONST MSKidxt   *ycsub,
			MSKlidxt   maxnumhesnz,
			MSKlidxt  *numhesnz,
			MSKidxt   *hessubi,
			MSKidxt   *hessubj);

___RMSK_INNER_NS_END___

#endif /* RMSK_SCOPT_H */
