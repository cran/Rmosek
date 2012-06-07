#define R_NO_REMAP
#include "rmsk_obj_arguments.h"

#include "rmsk_utils_sexp.h"
#include "rmsk_utils_mosek.h"
#include "rmsk_obj_mosek.h"

#include "rmsk_numeric_casts.h"


___RMSK_INNER_NS_START___
using std::string;
using std::auto_ptr;
using std::exception;


// ------------------------------
// Class options_type
// ------------------------------

options_type::options_type() :
	initialized(false),

	verbose(10),
	useparam(true),
	usesol(true),
	getinfo(false),
	soldetail(0),
	writebefore(""),
	writeafter(""),
	matrixformat(pkgMatrixCOO),
	scofile("")
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
	printpendingmsg("");

	// Read simple input arguments
	list_seek_Boolean(&useparam, arglist, R_ARGS.useparam,  true);
	list_seek_Boolean(&usesol, arglist, R_ARGS.usesol, true);
	list_seek_Boolean(&getinfo, arglist, R_ARGS.getinfo, true);
	list_seek_Scalar(&soldetail, arglist, R_ARGS.soldetail, true);
	list_seek_String(&writebefore, arglist, R_ARGS.writebefore, true);
	list_seek_String(&writeafter, arglist, R_ARGS.writeafter, true);
	list_seek_String(&scofile, arglist, R_ARGS.scofile, true);

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

// Default values of optional arguments
problem_type::problem_type() :
	initialized(false),

	numnz		(0),
	numcon		(0),
	numvar		(0),
	numintvar	(0),
	numcones	(0),
	numscoprs	(0),
	sense	(MSK_OBJECTIVE_SENSE_MINIMIZE),
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
	numnz  = A->nnz();
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
	list_seek_RNumericMatrix(bc, arglist, R_ARGS.bc);	validate_RMatrix(bc, R_ARGS.bc, 2, numcon);
	list_seek_RNumericMatrix(bx, arglist, R_ARGS.bx);	validate_RMatrix(bx, R_ARGS.bx, 2, numvar);

	// Cones
	SEXP_Handle sexpcones;	list_seek_RListMatrix(sexpcones, arglist, R_ARGS.cones, true);
	cones.R_read(sexpcones);
	numcones = cones.numcones;

	// Scopt operators
	SEXP_Handle sexpscopt;  list_seek_NamedVector(sexpscopt, arglist, R_ARGS.scopt, true);
	scopt.R_read(sexpscopt);
	numscoprs = scopt.numoprc + scopt.numopro;

	// Integers variables and initial solutions
	list_seek_Numeric(intsub, arglist, R_ARGS.intsub, true);
	list_seek_NamedVector(initsol, arglist, R_ARGS.sol, true);
	numintvar = numeric_cast<MSKintt>(Rf_length(intsub));

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

	printdebug("Started writing R problem output");
	prob_val.initVEC( numeric_cast<R_len_t>( problem_type::R_ARGS.arglist.size() ) );

	if (!initialized) {

		// Special treatment of scopt operators
		if (numscoprs > 0) {
			SEXP_Handle sexpscopt;	scopt.R_write(sexpscopt);
			prob_val.pushback("scopt", sexpscopt);
			return;
		}

		throw msk_exception("Internal error in problem_type::R_write, no problem was loaded");
	}

	// Objective sense
	prob_val.pushback("sense", get_objective(sense));

	// Objective
	prob_val.pushback("c", c);
	if (c0 != 0.0) {
		prob_val.pushback("c0", c0);
	}

	// Constraint Matrix A
	SEXP_Handle MatrixA;	A->R_write(MatrixA);
	prob_val.pushback("A", MatrixA);

	// Constraint and variable bounds
	prob_val.pushback("bc", bc);
	prob_val.pushback("bx", bx);

	// Cones
	if (numcones > 0) {
		SEXP_Handle sexpcones;	cones.R_write(sexpcones);
		prob_val.pushback("cones", sexpcones);
	}

	// Scopt operators
	if (numscoprs > 0) {
		SEXP_Handle sexpscopt;	scopt.R_write(sexpscopt);
		prob_val.pushback("scopt", sexpscopt);
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

		// Scopt operators are not stored in MOSEK!
		numscoprs = 0;
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

		auto_array<double> pblc( new double[numcon] );
		auto_array<double> pbuc( new double[numcon] );
		get_boundvalues(task, pblc, pbuc, MSK_ACC_CON, numcon);

		SEXP_Vector bcvec;		bcvec.initREAL(2*numcon);	bc.protect(bcvec);
		double *pbc = REAL(bc);
		for (int i=0; i<numcon; ++i) {
			pbc[2*i+0] = pblc[i];
			pbc[2*i+1] = pbuc[i];
		}

		// Add dimensions and names
		SEXP_Handle dim( Rf_allocVector(INTSXP, 2) );
		INTEGER(dim)[0] = 2; INTEGER(dim)[1] = numcon;
		Rf_setAttrib(bc, R_DimSymbol, dim);

		SEXP_Handle colnames( Rf_allocVector(STRSXP, 0) );
		SEXP_Handle rownames( Rf_allocVector(STRSXP, 2) );
			SET_STRING_ELT(rownames, 0, Rf_mkChar("blc"));
			SET_STRING_ELT(rownames, 1, Rf_mkChar("buc"));

		SEXP_Handle dimnames( Rf_allocVector(VECSXP, 2) );
		SET_VECTOR_ELT(dimnames, 0, rownames);
		SET_VECTOR_ELT(dimnames, 1, colnames);
		Rf_setAttrib(bc, R_DimNamesSymbol, dimnames);
	}

	// Variable bounds
	{
		printdebug("problem_type::MOSEK_read - Variable bounds");

		auto_array<double> pblx( new double[numvar] );
		auto_array<double> pbux( new double[numvar] );
		get_boundvalues(task, pblx, pbux, MSK_ACC_VAR, numvar);

		SEXP_Vector bxvec;		bxvec.initREAL(2*numvar);	bx.protect(bxvec);
		double *pbx = REAL(bx);
		for (int i=0; i<numvar; ++i) {
			pbx[2*i+0] = pblx[i];
			pbx[2*i+1] = pbux[i];
		}

		// Add dimensions and names
		SEXP_Handle dim( Rf_allocVector(INTSXP, 2) );
		INTEGER(dim)[0] = 2; INTEGER(dim)[1] = numvar;
		Rf_setAttrib(bx, R_DimSymbol, dim);

		SEXP_Handle colnames( Rf_allocVector(STRSXP, 0) );
		SEXP_Handle rownames( Rf_allocVector(STRSXP, 2) );
			SET_STRING_ELT(rownames, 0, Rf_mkChar("blx"));
			SET_STRING_ELT(rownames, 1, Rf_mkChar("bux"));

		SEXP_Handle dimnames( Rf_allocVector(VECSXP, 2) );
		SET_VECTOR_ELT(dimnames, 0, rownames);
		SET_VECTOR_ELT(dimnames, 1, colnames);
		Rf_setAttrib(bx, R_DimNamesSymbol, dimnames);
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

		MSKidxt idx = 0;
		MSKvariabletypee type;
		for (MSKidxt i=0; i<numvar; i++) {
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
		msk_getsolution(initsol, task, options);
	}

	initialized = true;
}

void problem_type::MOSEK_write(Task_handle &task) {
	if (!initialized) {
		throw msk_exception("Internal error in problem_type::MOSEK_write, no problem was loaded");
	}

	printdebug("Started writing MOSEK problem input");

	/* Set problem description */
	msk_loadproblem(task, *this);

	/* Set initial solution */
	if (options.usesol) {
		append_initsol(task, initsol, numcon, numvar, numcones);
	}

	/* Set parameters */
	if (options.useparam) {
		append_parameters(task, iparam, dparam, sparam);
	}

	printdebug("MOSEK_write finished");
}

___RMSK_INNER_NS_END___
