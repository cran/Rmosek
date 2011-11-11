#define R_NO_REMAP
#include "rmsk_obj_arguments.h"
#include "rmsk_obj_mosek.h"

#include "rmsk_sexp_methods.h"
#include "rmsk_utils.h"

___RMSK_INNER_NS_START___

using std::string;
using std::auto_ptr;
using std::exception;


// ------------------------------
// Class options_type
// ------------------------------

const options_type::R_ARGS_type options_type::R_ARGS;

options_type::options_type() :
	initialized(false),

	useparam(true),
	usesol(true),
	verbose(10),
	writebefore(""),
	writeafter(""),
	matrixformat(pkgMatrixCOO)
{}


void options_type::R_read(SEXP_LIST object) {
	if (!isNamedVector(object)) {
		throw msk_exception("Internal function options_type::read received an unknown request");
	}
	SEXP_Handle arglist(object);

	printdebug("Reading options");

	// Read verbose and update message system
	list_seek_Scalar(&verbose, arglist, R_ARGS.verbose, true);
	mosek_interface_verbose = verbose;
	printpendingmsg();

	// Read simple input arguments
	list_seek_Boolean(&useparam, arglist, R_ARGS.useparam,  true);
	list_seek_Boolean(&usesol, arglist, R_ARGS.usesol, true);
	list_seek_String(&writebefore, arglist, R_ARGS.writebefore, true);
	list_seek_String(&writeafter, arglist, R_ARGS.writeafter, true);

	// Read matrix format
	string matrixformat_name;
	list_seek_String(&matrixformat_name, arglist, R_ARGS.matrixformat, true);
	matrixformat = get_matrixformat(matrixformat_name);

	// Check for bad arguments
	validate_NamedVector(arglist, "", R_ARGS.arglist);

	initialized = true;
}


// ------------------------------
// Class problem_type
// ------------------------------

const problem_type::R_ARGS_type problem_type::R_ARGS;

// Default values of optional arguments
problem_type::problem_type() :
	initialized(false),

	sense	(MSK_OBJECTIVE_SENSE_UNDEFINED),
	c0		(0),
	options	(options_type())
{}


void problem_type::R_read(SEXP_LIST object) {
	if (initialized) {
		throw msk_exception("Internal error in problem_type::R_read, a problem was already loaded");
	}

	if (!isNamedVector(object)) {
		throw msk_exception("Internal error in problem_type::R_read, received an unknown request");
	}
	SEXP_Handle arglist(object);

	printdebug("Started reading R problem input");

	// Constraint Matrix
	list_seek_SparseMatrix(A, arglist, R_ARGS.A);
	numnz = A->nnz();
	numcon = A->nrow();
	numvar = A->ncol();

	// Objective sense
	string sensename = "UNDEFINED";
	list_seek_String(&sensename, arglist, R_ARGS.sense);
	sense = get_mskobjective(sensename);

	// Objective function
	list_seek_Numeric(c, arglist, R_ARGS.c);		validate_Numeric(c, R_ARGS.c, numvar);
	list_seek_Scalar(&c0, arglist, R_ARGS.c0, true);

	// Constraint and Variable Bounds
	list_seek_Numeric(blc, arglist, R_ARGS.blc);	validate_Numeric(blc, R_ARGS.blc, numcon);
	list_seek_Numeric(buc, arglist, R_ARGS.buc);	validate_Numeric(buc, R_ARGS.buc, numcon);
	list_seek_Numeric(blx, arglist, R_ARGS.blx);	validate_Numeric(blx, R_ARGS.blx, numvar);
	list_seek_Numeric(bux, arglist, R_ARGS.bux);	validate_Numeric(bux, R_ARGS.bux, numvar);

	// Cones
	SEXP_Handle sexpcones;	list_seek_NamedVector(sexpcones, arglist, R_ARGS.cones, true);
	cones.R_read(sexpcones);
	numcones = cones.numcones;

	// Integers variables and initial solutions
	list_seek_Numeric(intsub, arglist, R_ARGS.intsub, true);
	list_seek_NamedVector(initsol, arglist, R_ARGS.sol, true);
	numintvar = Rf_length(intsub);

	// Parameters
	list_seek_NamedVector(iparam, arglist, R_ARGS.iparam, true);
	list_seek_NamedVector(dparam, arglist, R_ARGS.dparam, true);
	list_seek_NamedVector(sparam, arglist, R_ARGS.sparam, true);

//		// Options (use this to allow options to overwrite other input channels)
//		SEXP_LIST options_arglist = R_NilValue;
//		list_seek_List(&options_arglist, arglist, R_ARGS.options, true);
//		if (options_arglist != R_NilValue) {
//			options.R_read(options_arglist);
//		}

	// Check for bad arguments
	validate_NamedVector(arglist, "", R_ARGS.arglist);

	initialized = true;
}

void problem_type::R_write(SEXP_NamedVector &prob_val) {
	if (!initialized) {
		throw msk_exception("Internal error in problem_type::R_write, no problem was loaded");
	}

	printdebug("Started writing R problem output");

	prob_val.initVEC( problem_type::R_ARGS.arglist.size() );

	// Objective sense
	prob_val.pushback("sense", get_objective(sense));

	// Objective
	prob_val.pushback("c", c);
	prob_val.pushback("c0", c0);

	// Constraint Matrix A
	SEXP_Handle MatrixA;	A->R_write(MatrixA);
	prob_val.pushback("A", MatrixA);

	// Constraint and variable bounds
	prob_val.pushback("blc", blc);
	prob_val.pushback("buc", buc);
	prob_val.pushback("blx", blx);
	prob_val.pushback("bux", bux);

	// Cones
	if (numcones > 0) {
		SEXP_Handle sexpcones;	cones.R_write(sexpcones);
		prob_val.pushback("cones", sexpcones);
	}

	// Integer subindexes
	if (numintvar > 0) {
		prob_val.pushback("intsub", intsub);
	}

	// Parameters
	if (options.useparam) {
		if (!isEmpty(iparam))
			prob_val.pushback("iparam", iparam);

		if (!isEmpty(dparam))
			prob_val.pushback("dparam", dparam);

		if (!isEmpty(sparam))
			prob_val.pushback("sparam", sparam);
	}

	// Initial solution
	if (options.usesol) {
		if (!isEmpty(initsol))
			prob_val.pushback("sol", initsol);
	}
}

void problem_type::MOSEK_read(Task_handle &task) {
	if (initialized) {
		throw msk_exception("Internal error in problem_type::MOSEK_read, a problem was already loaded");
	}

	printdebug("Started reading MOSEK problem output");

	// Get problem dimensions
	{
		errcatch( MSK_getnumanz(task, &numnz) );
		errcatch( MSK_getnumcon(task, &numcon) );
		errcatch( MSK_getnumvar(task, &numvar) );
		errcatch( MSK_getnumintvar(task, &numintvar) );
		errcatch( MSK_getnumcone(task, &numcones) );
	}

	// Objective sense and constant
	{
		printdebug("problem_type::MOSEK_read - Objective sense and constant");
		errcatch( MSK_getobjsense(task, &sense) );
		errcatch( MSK_getcfix(task, &c0) );
	}

	// Objective coefficients
	{
		printdebug("problem_type::MOSEK_read - Objective coefficients");

		SEXP_Vector cvec;	cvec.initREAL(numvar);	c.protect(cvec);
		double *pc = REAL(c);
		errcatch( MSK_getc(task, pc) );
	}

	// Constraint Matrix A
	{
		printdebug("problem_type::MOSEK_read - Constraint matrix");
		auto_ptr<matrix_type> tempMatrix;

		switch (options.matrixformat) {
		case pkgMatrixCOO:
			tempMatrix.reset( new pkgMatrixCOO_type );
			break;
		case pkgMatrixCSC:
			tempMatrix.reset( new pkgMatrixCSC_type );
			break;
		case simple_matrixCOO:
			tempMatrix.reset( new simple_matrixCOO_type );
			break;
		default:
			throw msk_exception("A matrix format was not supported");
		}

		tempMatrix->MOSEK_read(task);
		A = tempMatrix;
	}

	// Constraint bounds
	{
		printdebug("problem_type::MOSEK_read - Constraint bounds");

		SEXP_Vector blcvec;		blcvec.initREAL(numcon);	blc.protect(blcvec);
		SEXP_Vector bucvec;		bucvec.initREAL(numcon);	buc.protect(bucvec);

		double *pblc = REAL(blc);
		double *pbuc = REAL(buc);

		get_boundvalues(task, pblc, pbuc, MSK_ACC_CON, numcon);
	}

	// Variable bounds
	{
		printdebug("problem_type::MOSEK_read - Variable bounds");

		SEXP_Vector blxvec;		blxvec.initREAL(numvar);	blx.protect(blxvec);
		SEXP_Vector buxvec;		buxvec.initREAL(numvar);	bux.protect(buxvec);

		double *pblx = REAL(blx);
		double *pbux = REAL(bux);

		get_boundvalues(task, pblx, pbux, MSK_ACC_VAR, numvar);
	}

	// Cones
	if (numcones > 0) {
		printdebug("problem_type::MOSEK_read - Cones");
		cones.MOSEK_read(task);
	}

	// Integer subindexes
	if (numintvar > 0) {
		printdebug("problem_type::MOSEK_read - Integer subindexes");

		SEXP_Vector intsubvec;	intsubvec.initINT(numintvar);	intsub.protect(intsubvec);

		MSKlidxt *pintsub = INTEGER(intsub);

		int idx = 0;
		MSKvariabletypee type;
		for (int i=0; i<numvar; i++) {
			errcatch( MSK_getvartype(task,i,&type) );

			// R indexes count from 1, not from 0 as MOSEK
			if (type == MSK_VAR_TYPE_INT) {
				pintsub[idx++] = i+1;

				if (idx >= numintvar)
					break;
			}
		}
	}

	// Integer Parameters
	if (options.useparam) {
		printdebug("problem_type::MOSEK_read - Integer Parameters");

		SEXP_NamedVector iparamvec;	iparamvec.initVEC(MSK_IPAR_END - MSK_IPAR_BEGIN);	iparam.protect(iparamvec);
		get_int_parameters(iparamvec, task);
	}

	// Double Parameters
	if (options.useparam) {
		printdebug("problem_type::MOSEK_read - Double Parameters");

		SEXP_NamedVector dparamvec;	dparamvec.initVEC(MSK_DPAR_END - MSK_DPAR_BEGIN);	dparam.protect(dparamvec);
		get_dou_parameters(dparamvec, task);
	}

	// String Parameters
	if (options.useparam) {
		printdebug("problem_type::MOSEK_read - String Parameters");

		SEXP_NamedVector sparamvec;	sparamvec.initVEC(MSK_SPAR_END - MSK_SPAR_BEGIN);	sparam.protect(sparamvec);
		get_str_parameters(sparamvec, task);
	}

	// Initial solution
	if (options.usesol) {
		printdebug("problem_type::MOSEK_read - Initial solution");
		msk_getsolution(initsol, task);
	}

	initialized = true;
}

void problem_type::MOSEK_write(Task_handle &task) {
	if (!initialized) {
		throw msk_exception("Internal error in problem_type::MOSEK_write, no problem was loaded");
	}

	printdebug("Started writing MOSEK problem input");

	/* Set problem description */
	msk_loadproblem(task, sense, c, c0,
			A, blc, buc, blx, bux,
			cones, intsub);

	/* Set initial solution */
	if (options.usesol) {
		append_initsol(task, initsol, numcon, numvar);
	}

	/* Set parameters */
	if (options.useparam) {
		append_parameters(task, iparam, dparam, sparam);
	}

	printdebug("MOSEK_write finished");
}

___RMSK_INNER_NS_END___
