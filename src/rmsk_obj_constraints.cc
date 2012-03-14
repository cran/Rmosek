#define R_NO_REMAP
#include "rmsk_obj_constraints.h"

#include "rmsk_utils_sexp.h"
#include "rmsk_utils_scopt.h"
#include "rmsk_utils_mosek.h"


___RMSK_INNER_NS_START___
using std::string;
using std::auto_ptr;


// ------------------------------
// Class conicSOC_type
// ------------------------------

const conicSOC_type::ITEMS_type::R_ARGS_type conicSOC_type::ITEMS_type::R_ARGS;

void conicSOC_type::R_read(SEXP_LIST object) {
	if (initialized) {
		throw msk_exception("Internal error in conicSOC_type::R_read, a 'cones' matrix was already loaded");
	}

	printdebug("Started reading 'cones' matrix from R");
	if (isEmpty(object)) {

		numcones = 0;

	} else {

		if (Rf_nrows(object) != 2) {
			throw msk_exception("Variable 'cones' should have two rows. Optional naming convention for these rows is 'type' and 'sub'.");
		}

		conemat.protect(object);
		numcones = numeric_cast<MSKintt>( Rf_ncols(object) );
	}

	initialized = true;
}

void conicSOC_type::R_write(SEXP_Handle &val) {
	if (!initialized) {
		throw msk_exception("Internal error in conicSOC_type::R_write, no 'cones' matrix loaded");
	}

	printdebug("Started writing second order cone matrix to R");
	val.protect(conemat);
}

void conicSOC_type::MOSEK_read(Task_handle &task) {
	if (initialized) {
		throw msk_exception("Internal error in conicSOC_type::MOSEK_read, a 'cones' matrix was already loaded");
	}

	printdebug("Started reading 'cones' matrix from MOSEK");

	errcatch( MSK_getnumcone(task, &numcones) );
	SEXP_Vector cones;	cones.initVEC(numcones * ITEMS.R_ARGS.arglist.size(), false);	conemat.protect(cones);

	for (MSKidxt idx=0; idx<numcones; ++idx) {
		MSKintt numconemembers;
		MSK_getnumconemem(task, idx, &numconemembers);

		SEXP_Vector subvec;		subvec.initINT(numconemembers);
		MSKlidxt *psub = INTEGER(subvec);

		MSKconetypee msktype;
		errcatch( MSK_getcone(task, idx, &msktype, NULL, &numconemembers, psub) );

		// R indexes count from 1, not from 0 as MOSEK
		for (MSKidxt k=0; k<numconemembers; k++) {
			psub[k]++;
		}

		char type[MSK_MAX_STR_LEN];
		errcatch( MSK_conetypetostr(task, msktype, type) );

		string strtype = type;
		remove_mskprefix(strtype,"MSK_CT_");

		cones.pushback(strtype);
		cones.pushback(subvec);
	}

	// Add dimensions and names
	SEXP_Handle dim( Rf_allocVector(INTSXP, 2) );
	INTEGER(dim)[0] = ITEMS.R_ARGS.arglist.size(); INTEGER(dim)[1] = numcones;
	Rf_setAttrib(conemat, R_DimSymbol, dim);

	SEXP_Handle colnames( Rf_allocVector(STRSXP, 0) );
	SEXP_Handle rownames( Rf_allocVector(STRSXP, ITEMS.R_ARGS.arglist.size()) );
		SET_STRING_ELT(rownames, 0, Rf_mkChar(ITEMS.R_ARGS.type.c_str()));
		SET_STRING_ELT(rownames, 1, Rf_mkChar(ITEMS.R_ARGS.sub.c_str()));

	SEXP_Handle dimnames( Rf_allocVector(VECSXP, 2) );
	SET_VECTOR_ELT(dimnames, 0, rownames);
	SET_VECTOR_ELT(dimnames, 1, colnames);
	Rf_setAttrib(conemat, R_DimNamesSymbol, dimnames);

	initialized = true;
}

void conicSOC_type::MOSEK_write(Task_handle &task) {
	if (!initialized) {
		throw msk_exception("Internal error in conicSOC_type::MOSEK_write, no 'cones' matrix loaded");
	}

	printdebug("Started writing 'cones' matrix to MOSEK");

	for (MSKidxt idx=0; idx<numcones; ++idx) {
		SEXP_Handle typevalue( RLISTMATRIX_ELT(conemat, 0, idx) );
		SEXP_Handle subvalue( RLISTMATRIX_ELT(conemat, 1, idx) );

		string type;		list_obtain_String(&type, typevalue, "cones[1," + tostring(idx+1) + "]' specifying cone '" + ITEMS.R_ARGS.type);
		SEXP_Handle sub;	list_obtain_Numeric(sub, subvalue, "cones[2," + tostring(idx+1) + "]' specifying cone '" + ITEMS.R_ARGS.sub);

		// Convert type to mosek input
		strtoupper(type);
		append_mskprefix(type, "MSK_CT_");
		char msktypestr[MSK_MAX_STR_LEN];
		if(!MSK_symnamtovalue(const_cast<MSKCONST char*>(type.c_str()), msktypestr))
			throw msk_exception("The type of cone at index " + tostring(idx+1) + " was not recognized");

		MSKconetypee msktype = (MSKconetypee)atoi(msktypestr);

		// Convert sub type and indexing (Minus one because MOSEK indexes counts from 0, not from 1 as R)
		MSKintt numconesub = numeric_cast<MSKintt>(Rf_length(sub));
		MSKidxt msksub[numconesub];
		for (MSKidxt i=0; i < numconesub; i++)
			msksub[i] = INTEGER_ELT(sub,i) - 1;

		// Append the cone
		errcatch( MSK_appendcone(task,
				msktype,	/* The type of cone */
				0.0, 		/* For future use only, can be set to 0.0 */
				numconesub,	/* Number of variables */
				msksub) );	/* Variable indexes */
	}
}


// ------------------------------
// Class scoptOPR_type
// ------------------------------

const scoptOPR_type::R_ARGS_type scoptOPR_type::R_ARGS;
const scoptOPR_type::ITEMS_type::OPRO_type::R_ARGS_type scoptOPR_type::ITEMS_type::OPRO_type::R_ARGS;
const scoptOPR_type::ITEMS_type::OPRC_type::R_ARGS_type scoptOPR_type::ITEMS_type::OPRC_type::R_ARGS;

void scoptOPR_type::R_read(SEXP_LIST object) {
	if (initialized) {
		throw msk_exception("Internal error in scoptOPR_type::R_read, 'scopt' operator matrices were already loaded");
	}

	printdebug("Started reading 'scopt' matrix from R");

	if (object) {
		SEXP_Handle scopt(object);

		validate_NamedVector(scopt, "scopt", R_ARGS.arglist);
		list_seek_RListMatrix(opromat, scopt, "opro", true);
		list_seek_RListMatrix(oprcmat, scopt, "oprc", true);

		if (isEmpty(opromat)) {
			numopro = 0;
		} else {
			if (Rf_nrows(opromat) != 5) {
				throw msk_exception("Variable 'scopt$opro' should have five rows. Optional naming convention for these rows is 'type', 'j', 'f', 'g' and 'h'.");
			}
			numopro = numeric_cast<MSKintt>( Rf_ncols(opromat) );
		}

		if (isEmpty(oprcmat)) {
			numoprc = 0;
		} else {
			if (Rf_nrows(oprcmat) != 6) {
				throw msk_exception("Variable 'scopt$oprc' should have six rows. Optional naming convention for these rows is 'type', 'i', 'j', 'f', 'g' and 'h'.");
			}
			numoprc = numeric_cast<MSKintt>( Rf_ncols(oprcmat) );
		}

	}
	else {
		numopro = 0;
		numoprc = 0;
	}

	initialized = true;
}

void scoptOPR_type::R_write(SEXP_Handle &val) {
	if (!initialized) {
		throw msk_exception("Internal error in scoptOPR_type::R_write, no 'scopt' operator matrix loaded");
	}

	printdebug("Started writing 'scopt' matrix to R");

	if ( numopro || numoprc ) {

		SEXP_NamedVector scopt;	scopt.initVEC(R_ARGS.arglist.size());	val.protect(scopt);

		scopt.pushback(R_ARGS.opro, opromat);
		scopt.pushback(R_ARGS.oprc, oprcmat);
	}
}

void scoptOPR_type::FILE_read(string &filepath) {
	if (initialized) {
		throw msk_exception("Internal error in scoptOPR_type::FILE_read, 'scopt' operator matrices were already loaded");
	}

	// open file
	FILE_handle file( fopen(filepath.c_str(), "rt") );

	if (static_cast<FILE*>(file) == NULL) {
		throw msk_exception("An error occurred while opening file '" + filepath + "'.");
	}

	// read file
	const string read_error_msg = "The variable 'scopt' could not be read from file '" + filepath + "'. Bad file format.";

	char buffer[MSK_MAX_STR_LEN];
	int mskoprtype;
	int idx,i,j;
	char fbuf[80], gbuf[80], hbuf[80];

	if (fgets(buffer, sizeof(buffer), file) == NULL)
		throw msk_exception(read_error_msg);
	if (sscanf(buffer, "%d", &numopro) != 1)
		throw msk_exception(read_error_msg);

	if ( numopro )
	{
		SEXP_Vector opro;	opro.initVEC(numopro * ITEMS.OPRO.R_ARGS.arglist.size(), false);	opromat.protect(opro);

		for (idx=0; idx<numopro; ++idx) {

			if (fgets(buffer, sizeof(buffer), file) == NULL)
				throw msk_exception(read_error_msg);

			for(int p=0; buffer[p]; ++p)
				if ( buffer[p]==' ' )
					buffer[p] = '\n';

			if (numeric_cast<int>(ITEMS.OPRO.R_ARGS.arglist.size()) !=
					sscanf(buffer,"%d %d %s %s %s",
						&mskoprtype,
						&j,
						fbuf,
						gbuf,
						hbuf)) {
				throw msk_exception(read_error_msg);
			}

			MSK_scvaltosymnam(static_cast<MSKscopre>(mskoprtype), buffer);
			string strtype(buffer);	 remove_mskprefix(strtype,"MSK_OPR_");

			opro.pushback(strtype);
			opro.pushback(j + 1);			// MOSEK counts from 0
			opro.pushback(atof(fbuf));
			opro.pushback(atof(gbuf));
			opro.pushback(atof(hbuf));
		}

		// Add dimensions and names
		SEXP_Handle dim( Rf_allocVector(INTSXP, 2) );
		INTEGER(dim)[0] = ITEMS.OPRO.R_ARGS.arglist.size(); INTEGER(dim)[1] = numopro;
		Rf_setAttrib(opromat, R_DimSymbol, dim);

		SEXP_Handle colnames( Rf_allocVector(STRSXP, 0) );
		SEXP_Handle rownames( Rf_allocVector(STRSXP, ITEMS.OPRO.R_ARGS.arglist.size()) );
			SET_STRING_ELT(rownames, 0, Rf_mkChar(ITEMS.OPRO.R_ARGS.type.c_str()));
			SET_STRING_ELT(rownames, 1, Rf_mkChar(ITEMS.OPRO.R_ARGS.j.c_str()));
			SET_STRING_ELT(rownames, 2, Rf_mkChar(ITEMS.OPRO.R_ARGS.f.c_str()));
			SET_STRING_ELT(rownames, 3, Rf_mkChar(ITEMS.OPRO.R_ARGS.g.c_str()));
			SET_STRING_ELT(rownames, 4, Rf_mkChar(ITEMS.OPRO.R_ARGS.h.c_str()));

		SEXP_Handle dimnames( Rf_allocVector(VECSXP, 2) );
		SET_VECTOR_ELT(dimnames, 0, rownames);
		SET_VECTOR_ELT(dimnames, 1, colnames);
		Rf_setAttrib(opromat, R_DimNamesSymbol, dimnames);
	}

	if (fgets(buffer, sizeof(buffer), file) == NULL)
		throw msk_exception(read_error_msg);
	if (sscanf(buffer, "%d", &numoprc) != 1)
		throw msk_exception(read_error_msg);

	if ( numoprc )
	{
		SEXP_Vector oprc;	oprc.initVEC(numoprc * ITEMS.OPRC.R_ARGS.arglist.size(), false);	oprcmat.protect(oprc);

		for (idx=0; idx<numoprc; ++idx) {

			if (fgets(buffer, sizeof(buffer), file) == NULL)
				throw msk_exception(read_error_msg);

			for(int p=0; buffer[p]; ++p)
				if ( buffer[p]==' ' )
					buffer[p] = '\n';

			if (numeric_cast<int>(ITEMS.OPRC.R_ARGS.arglist.size()) !=
					sscanf(buffer,"%d %d %d %s %s %s",
						&mskoprtype,
						&i,
						&j,
						fbuf,
						gbuf,
						hbuf)) {
				throw msk_exception(read_error_msg);
			}

			MSK_scvaltosymnam(static_cast<MSKscopre>(mskoprtype), buffer);
			string strtype(buffer);	 remove_mskprefix(strtype,"MSK_OPR_");

			oprc.pushback(strtype);
			oprc.pushback(i + 1);			// MOSEK counts from 0
			oprc.pushback(j + 1);			// MOSEK counts from 0
			oprc.pushback(atof(fbuf));
			oprc.pushback(atof(gbuf));
			oprc.pushback(atof(hbuf));
		}

		// Add dimensions and names
		SEXP_Handle dim( Rf_allocVector(INTSXP, 2) );
		INTEGER(dim)[0] = ITEMS.OPRC.R_ARGS.arglist.size(); INTEGER(dim)[1] = numoprc;
		Rf_setAttrib(oprcmat, R_DimSymbol, dim);

		SEXP_Handle colnames( Rf_allocVector(STRSXP, 0) );
		SEXP_Handle rownames( Rf_allocVector(STRSXP, ITEMS.OPRC.R_ARGS.arglist.size()) );
			SET_STRING_ELT(rownames, 0, Rf_mkChar(ITEMS.OPRC.R_ARGS.type.c_str()));
			SET_STRING_ELT(rownames, 1, Rf_mkChar(ITEMS.OPRC.R_ARGS.i.c_str()));
			SET_STRING_ELT(rownames, 2, Rf_mkChar(ITEMS.OPRC.R_ARGS.j.c_str()));
			SET_STRING_ELT(rownames, 3, Rf_mkChar(ITEMS.OPRC.R_ARGS.f.c_str()));
			SET_STRING_ELT(rownames, 4, Rf_mkChar(ITEMS.OPRC.R_ARGS.g.c_str()));
			SET_STRING_ELT(rownames, 5, Rf_mkChar(ITEMS.OPRC.R_ARGS.h.c_str()));

		SEXP_Handle dimnames( Rf_allocVector(VECSXP, 2) );
		SET_VECTOR_ELT(dimnames, 0, rownames);
		SET_VECTOR_ELT(dimnames, 1, colnames);
		Rf_setAttrib(oprcmat, R_DimNamesSymbol, dimnames);
	}

	initialized = true;
}

void scoptOPR_type::FILE_write(string &filepath, problem_type &probin)
{
	if (!initialized) {
		throw msk_exception("Internal error in scoptOPR_type::FILE_write, no 'scopt' operator matrix loaded");
	}

	// open file
	printdebug("scoptOPR_type::FILE_write: Open file");
	FILE_handle file( fopen(filepath.c_str(), "wt") );

	if (static_cast<FILE*>(file) == NULL) {
		throw msk_exception("An error occurred while opening file '" + filepath + "'.");
	}

	// write file
	MSKscopre mskoprtype;
	int idx,i=0,j=0;
	double f,g,h;

	printdebug("scoptOPR_type::FILE_write: Write file opro");
	fprintf(file, "%d\n", numopro);
	for (idx=0; idx<numopro; ++idx) {
		printdebug("scoptOPR_type::FILE_write: Operator " + tostring(idx));
		SEXP_Handle typevalue( RLISTMATRIX_ELT(opromat, 0, idx) );
		SEXP_Handle jvalue( RLISTMATRIX_ELT(opromat, 1, idx) );
		SEXP_Handle fvalue( RLISTMATRIX_ELT(opromat, 2, idx) );
		SEXP_Handle gvalue( RLISTMATRIX_ELT(opromat, 3, idx) );
		SEXP_Handle hvalue( RLISTMATRIX_ELT(opromat, 4, idx) );

		// Obtain type
		string type;
		list_obtain_String(&type, typevalue, "scopt$opro[1," + tostring(idx+1) + "]' specifying operator '" + ITEMS.OPRO.R_ARGS.type);

		// Convert type to mosek input
		strtoupper(type);
		append_mskprefix(type, "MSK_OPR_");
		if(!MSK_scsymnamtovalue(const_cast<MSKCONST char*>(type.c_str()), &mskoprtype))
			throw msk_exception("The type of operator at scopt$opro[[" + tostring(idx+1) + "]] was not recognized");

					list_obtain_Integer(&j, jvalue, "scopt$opro[2," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRO.R_ARGS.j, false);
		f = 0.0; 	list_obtain_Scalar(&f, fvalue, "scopt$opro[3," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRO.R_ARGS.f, !MSKscopr_using_coef[mskoprtype][0]);
		g = 0.0; 	list_obtain_Scalar(&g, gvalue, "scopt$opro[4," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRO.R_ARGS.g, !MSKscopr_using_coef[mskoprtype][1]);
		h = 0.0; 	list_obtain_Scalar(&h, hvalue, "scopt$opro[5," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRO.R_ARGS.h, !MSKscopr_using_coef[mskoprtype][2]);

		validate_scopr(mskoprtype, 0, j, f, g, h, probin, "scopt$opro[," + tostring(idx+1) + "]");

		fprintf(file,"%-8d %-8d %-24.16e %-24.16e %-24.16e\n",
				mskoprtype,
				j - 1,			// MOSEK counts from 0
				f,
				g,
				h);
	}

	printdebug("scoptOPR_type::FILE_write: Write file oprc");
	fprintf(file, "%d\n", numoprc);
	for (idx=0; idx<numoprc; ++idx) {
		printdebug("scoptOPR_type::FILE_write: Operator " + tostring(idx));
		SEXP_Handle typevalue( RLISTMATRIX_ELT(oprcmat, 0, idx) );
		SEXP_Handle ivalue( RLISTMATRIX_ELT(oprcmat, 1, idx) );
		SEXP_Handle jvalue( RLISTMATRIX_ELT(oprcmat, 2, idx) );
		SEXP_Handle fvalue( RLISTMATRIX_ELT(oprcmat, 3, idx) );
		SEXP_Handle gvalue( RLISTMATRIX_ELT(oprcmat, 4, idx) );
		SEXP_Handle hvalue( RLISTMATRIX_ELT(oprcmat, 5, idx) );

		// Obtain type
		string type;
		list_obtain_String(&type, typevalue, "scopt$oprc[1," + tostring(idx+1) + "]' specifying operator '" + ITEMS.OPRC.R_ARGS.type);

		// Convert type to mosek input
		strtoupper(type);
		append_mskprefix(type, "MSK_OPR_");
		if(!MSK_scsymnamtovalue(const_cast<MSKCONST char*>(type.c_str()), &mskoprtype))
			throw msk_exception("The type of operator at scopt$opro[[" + tostring(idx+1) + "]] was not recognized");

					list_obtain_Integer(&i, ivalue, "scopt$oprc[2," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRC.R_ARGS.i, false);
					list_obtain_Integer(&j, jvalue, "scopt$oprc[3," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRC.R_ARGS.j, false);
		f = 0.0; 	list_obtain_Scalar(&f, fvalue, "scopt$oprc[4," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRC.R_ARGS.f, !MSKscopr_using_coef[mskoprtype][0]);
		g = 0.0; 	list_obtain_Scalar(&g, gvalue, "scopt$oprc[5," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRC.R_ARGS.g, !MSKscopr_using_coef[mskoprtype][1]);
		h = 0.0; 	list_obtain_Scalar(&h, hvalue, "scopt$oprc[6," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRC.R_ARGS.h, !MSKscopr_using_coef[mskoprtype][2]);

		if (i == 0)
			throw msk_exception("Constraint index at 'scopt$oprc[2," + tostring(idx+1) + "]' was out of bounds." );

		validate_scopr(mskoprtype, i, j, f, g, h, probin, "scopt$oprc[," + tostring(idx+1) + "]");

		fprintf(file,"%-8d %-8d %-8d %-24.16e %-24.16e %-24.16e\n",
				mskoprtype,
				i - 1,			// MOSEK counts from 0
				j - 1,			// MOSEK counts from 0
				f,
				g,
				h);
	}
}

void scoptOPR_type::MOSEK_write(Task_handle &task, problem_type &probin)
{
	if (!initialized) {
		throw msk_exception("Internal error in scoptOPR_type::MOSEK_write, no 'scopt' operator matrix loaded");
	}

	if (numoprc || numopro) {

		// setup nlh datastruct
		init_nlh(probin);

		// append the operators
		printdebug("Call MSK_scbegin - START");
		errcatch( MSK_scbegin(task,
					nlh->numopro,
					nlh->opro,
					nlh->oprjo,
					nlh->oprfo,
					nlh->oprgo,
					nlh->oprho,
					nlh->numoprc,
					nlh->oprc,
					nlh->opric,
					nlh->oprjc,
					nlh->oprfc,
					nlh->oprgc,
					nlh->oprhc,
					nlh.get() ));
		printdebug("Call MSK_scbegin - DONE");

		called_MSK_scbegin = true;

		errcatch( MSK_putnlfunc(task, nlh.get(), SCstruc, SCeval) );
	}
}

void scoptOPR_type::init_nlh(problem_type &probin) {

	if (nlh.get() != NULL) {
		throw msk_exception("Internal error in scoptOPR_type::init_nlh, 'nlh' was already loaded");
	}

	printdebug("Writing 'scopt' operator matrices to internal format");
	nlh.reset( new nlhandt );
	nlh->numcon = probin.numcon;
	nlh->numvar = probin.numvar;

	if (numopro || numoprc) {

		printdebug("scopt - Allocating memory");

		// sc operators in objective
		nlh->numopro = numopro;
		nlh->opro.protect(new MSKscopre[numopro]);
		nlh->oprjo.protect(new int[numopro]);
		nlh->oprfo.protect(new double[numopro]);
		nlh->oprgo.protect(new double[numopro]);
		nlh->oprho.protect(new double[numopro]);

		// sc operators in constraints
		nlh->numoprc = numoprc;
		nlh->oprc.protect(new MSKscopre[numoprc]);
		nlh->opric.protect(new int[numoprc]);
		nlh->oprjc.protect(new int[numoprc]);
		nlh->oprfc.protect(new double[numoprc]);
		nlh->oprgc.protect(new double[numoprc]);
		nlh->oprhc.protect(new double[numoprc]);

		// Working vectors
		nlh->ptrc.protect(new int[nlh->numcon+1]);
		nlh->subc.protect(new int[nlh->numoprc]);

		nlh->ibuf.protect(new int[nlh->numvar]);
		nlh->zibuf.protect(new int[nlh->numvar]);
		nlh->zdbuf.protect(new double[nlh->numvar]);


		//
		// Initialize datastructures
		//
		MSKscopre mskoprtype;
		int i=0, j=0;
		double f, g, h;

		// opro
		printdebug("scopt - Setting up opro");
		for (MSKidxt idx=0; idx<numopro; ++idx) {
			SEXP_Handle typevalue( RLISTMATRIX_ELT(opromat, 0, idx) );
			SEXP_Handle jvalue( RLISTMATRIX_ELT(opromat, 1, idx) );
			SEXP_Handle fvalue( RLISTMATRIX_ELT(opromat, 2, idx) );
			SEXP_Handle gvalue( RLISTMATRIX_ELT(opromat, 3, idx) );
			SEXP_Handle hvalue( RLISTMATRIX_ELT(opromat, 4, idx) );

			// Obtain type
			string type;
			list_obtain_String(&type, typevalue, "scopt$opro[1," + tostring(idx+1) + "]' specifying operator '" + ITEMS.OPRO.R_ARGS.type);

			// Convert type to mosek input
			strtoupper(type);
			append_mskprefix(type, "MSK_OPR_");
			if(!MSK_scsymnamtovalue(const_cast<MSKCONST char*>(type.c_str()), &mskoprtype))
				throw msk_exception("The type of operator at 'scopt$opro[1," + tostring(idx+1) + "]' was not recognized");

						list_obtain_Integer(&j, jvalue, "scopt$opro[2," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRO.R_ARGS.j, false);
			f = 0.0; 	list_obtain_Scalar(&f, fvalue, "scopt$opro[3," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRO.R_ARGS.f, !MSKscopr_using_coef[mskoprtype][0]);
			g = 0.0; 	list_obtain_Scalar(&g, gvalue, "scopt$opro[4," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRO.R_ARGS.g, !MSKscopr_using_coef[mskoprtype][1]);
			h = 0.0; 	list_obtain_Scalar(&h, hvalue, "scopt$opro[5," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRO.R_ARGS.h, !MSKscopr_using_coef[mskoprtype][2]);

			validate_scopr(mskoprtype, 0, j, f, g, h, probin, "scopt$opro[," + tostring(idx+1) + "]");

			nlh->opro[idx] = mskoprtype;
			nlh->oprjo[idx] = j - 1;        // MOSEK counts from 0
			nlh->oprfo[idx] = f;
			nlh->oprgo[idx] = g;
			nlh->oprho[idx] = h;
		}

		// oprc
		printdebug("scopt - Setting up oprc");
		for (MSKidxt idx=0; idx<numoprc; ++idx) {
			SEXP_Handle typevalue( RLISTMATRIX_ELT(oprcmat, 0, idx) );
			SEXP_Handle ivalue( RLISTMATRIX_ELT(oprcmat, 1, idx) );
			SEXP_Handle jvalue( RLISTMATRIX_ELT(oprcmat, 2, idx) );
			SEXP_Handle fvalue( RLISTMATRIX_ELT(oprcmat, 3, idx) );
			SEXP_Handle gvalue( RLISTMATRIX_ELT(oprcmat, 4, idx) );
			SEXP_Handle hvalue( RLISTMATRIX_ELT(oprcmat, 5, idx) );

			// Obtain type
			string type;
			list_obtain_String(&type, typevalue, "scopt$oprc[1," + tostring(idx+1) + "]' specifying operator '" + ITEMS.OPRC.R_ARGS.type);

			// Convert type to mosek input
			strtoupper(type);
			append_mskprefix(type, "MSK_OPR_");
			if(!MSK_scsymnamtovalue(const_cast<MSKCONST char*>(type.c_str()), &mskoprtype))
				throw msk_exception("The type of operator at scopt$oprc[1," + tostring(idx+1) + "] was not recognized");

						list_obtain_Integer(&i, ivalue, "scopt$oprc[2," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRC.R_ARGS.i, false);
						list_obtain_Integer(&j, jvalue, "scopt$oprc[3," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRC.R_ARGS.j, false);
			f = 0.0; 	list_obtain_Scalar(&f, fvalue, "scopt$oprc[4," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRC.R_ARGS.f, !MSKscopr_using_coef[mskoprtype][0]);
			g = 0.0; 	list_obtain_Scalar(&g, gvalue, "scopt$oprc[5," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRC.R_ARGS.g, !MSKscopr_using_coef[mskoprtype][1]);
			h = 0.0; 	list_obtain_Scalar(&h, hvalue, "scopt$oprc[6," + tostring(idx+1) + "]' specifying operator element '" + ITEMS.OPRC.R_ARGS.h, !MSKscopr_using_coef[mskoprtype][2]);

			if (i == 0)
				throw msk_exception("Constraint index at 'scopt$oprc[2," + tostring(idx+1) + "]' was out of bounds." );

			validate_scopr(mskoprtype, i, j, f, g, h, probin, "scopt$oprc[," + tostring(idx+1) + "]");

			nlh->oprc[idx] = mskoprtype;
			nlh->opric[idx] = i - 1;        // MOSEK counts from 0
			nlh->oprjc[idx] = j - 1;        // MOSEK counts from 0
			nlh->oprfc[idx] = f;
			nlh->oprgc[idx] = g;
			nlh->oprhc[idx] = h;
		}

	} else {
		printdebug("..scopt operator matrices were empty!");
	}
}

___RMSK_INNER_NS_END___
