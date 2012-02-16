#define R_NO_REMAP
#include "rmsk_utils_mosek.h"

#include "rmsk_utils_interface.h"
#include "rmsk_utils_sexp.h"

#include "local_stubs.h"

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>


___RMSK_INNER_NS_START___
using std::string;
using std::vector;
using std::auto_ptr;
using std::exception;


// ------------------------------
// MOSEK-UTILS
// ------------------------------

string get_objective(MSKobjsensee sense)
{
	switch (sense) {
		case MSK_OBJECTIVE_SENSE_MAXIMIZE:
			return "maximize";
		case MSK_OBJECTIVE_SENSE_MINIMIZE:
			return "minimize";
		case MSK_OBJECTIVE_SENSE_UNDEFINED:
			return "UNDEFINED";
		default:
			throw msk_exception("A problem sense was not supported");
	}
}

MSKobjsensee get_mskobjective(string sense)
{
	strtoupper(sense);

	if (sense == "MIN" || sense == "MINIMIZE") {
		return MSK_OBJECTIVE_SENSE_MINIMIZE;

	} else if (sense == "MAX" || sense == "MAXIMIZE") {
		return MSK_OBJECTIVE_SENSE_MAXIMIZE;

	} else {
		throw msk_exception("Variable 'sense' must be either 'min', 'minimize', 'max' or 'maximize'");
	}
}

void append_mskprefix(string &str, string prefix)
{
	string::size_type prefix_size = prefix.size();
	string::size_type inclusion = 0, j = 0;

	for (string::size_type i = 0; i < prefix_size; i++) {
		if (prefix[i] == str[j]) {
			if (++j >= str.size()) {
				inclusion = prefix_size;
				break;
			}
		} else {
			j = 0;
			inclusion = i+1;
		}
	}
	str = prefix.substr(0, inclusion) + str;
}

void remove_mskprefix(string &str, string prefix)
{
	if (str.size() > prefix.size()) {
		if (str.substr(0, prefix.size()) == prefix) {
			str = str.substr(prefix.size(), str.size() - prefix.size());
		}
	}
}

void append_initsol(MSKtask_t task, SEXP_LIST initsol, MSKintt NUMCON, MSKintt NUMVAR)
{
	// VECTOR_ELT and STRING_ELT needs type 'int'
	R_len_t numobjects = numeric_cast<int>(Rf_length(initsol));

	SEXP namelst = Rf_getAttrib(initsol, R_NamesSymbol);
	if (Rf_length(namelst) != numobjects) {
		throw msk_exception("Mismatching number of elements and names in variable named '" + problem_type::R_ARGS.sol + "'");
	}

	for (R_len_t idx = 0; idx < numobjects; idx++) {
		SEXP val = VECTOR_ELT(initsol, static_cast<int>(idx));
		string name = CHAR(STRING_ELT(namelst, static_cast<int>(idx)));
		printdebug("Reading the initial solution '" + name + "'");

		// Get current solution type
		MSKsoltypee cursoltype;
		if (name == "bas") {
			cursoltype = MSK_SOL_BAS;
		} else if (name == "itr") {
			cursoltype = MSK_SOL_ITR;
		} else if (name == "int") {
			cursoltype = MSK_SOL_ITG;
		} else {
			printwarning("The initial solution '" + name + "' was not recognized.");
			continue;
		}

		// Get current solution
		if (isEmpty(val)) {
			printwarning("The initial solution '" + name + "' was ignored.");
			continue;
		}
		if(!isNamedVector(val)) {
			throw msk_exception("The cone at index " + tostring(idx+1) + " should be a 'named list'");
		}
		SEXP_LIST cursol = val;

		// Get current solution items
		SEXP_Handle skc; list_seek_Character(skc, cursol, "skc", true);	validate_Character(skc, "skc", NUMCON, true);
		SEXP_Handle xc;  list_seek_Numeric(xc, cursol, "xc", true);		validate_Numeric(xc, "xc", NUMCON, true);
		SEXP_Handle slc; list_seek_Numeric(slc, cursol, "slc", true);	validate_Numeric(slc, "slc", NUMCON, true);
		SEXP_Handle suc; list_seek_Numeric(suc, cursol, "suc", true);	validate_Numeric(suc, "suc", NUMCON, true);

		SEXP_Handle skx; list_seek_Character(skx, cursol, "skx", true);	validate_Character(skx, "skx", NUMVAR, true);
		SEXP_Handle xx;  list_seek_Numeric(xx, cursol, "xx", true);		validate_Numeric(xx, "xx", NUMVAR, true);
		SEXP_Handle slx; list_seek_Numeric(slx, cursol, "slx", true);	validate_Numeric(slx, "slx", NUMVAR, true);
		SEXP_Handle sux; list_seek_Numeric(sux, cursol, "sux", true);	validate_Numeric(sux, "sux", NUMVAR, true);
		SEXP_Handle snx; list_seek_Numeric(snx, cursol, "snx", true);	validate_Numeric(snx, "snx", NUMVAR, true);

		bool anyinfocon = !isEmpty(skc) || !isEmpty(xc) || !isEmpty(slc) || !isEmpty(suc);
		bool anyinfovar = !isEmpty(skx) || !isEmpty(xx) || !isEmpty(slx) || !isEmpty(sux) || !isEmpty(snx);

		// Set all constraints
		if (anyinfocon) {
			for (MSKintt ci = 0; ci < NUMCON; ci++)
			{
				MSKstakeye curskc;
				if (isEmpty(skc) || CHARACTER_ELT(skc,ci).empty()) {
					curskc = MSK_SK_UNK;
				} else {
					MSKintt skcval;
					errcatch( MSK_strtosk(task, const_cast<MSKCONST char*>(CHARACTER_ELT(skc,ci).c_str()), &skcval) );
					curskc = static_cast<MSKstakeye>(skcval);
				}

				MSKrealt curxc;
				if (isEmpty(xc)) {
					curxc = 0.0;
				} else {
					curxc = NUMERIC_ELT(xc, ci);
				}

				MSKrealt curslc;
				if (isEmpty(slc)) {
					curslc = 0.0;
				} else {
					curslc = NUMERIC_ELT(slc, ci);
				}

				MSKrealt cursuc;
				if (isEmpty(suc)) {
					cursuc = 0.0;
				} else {
					cursuc = NUMERIC_ELT(suc, ci);
				}

				errcatch( MSK_putsolutioni(task,
						MSK_ACC_CON, ci, cursoltype,
						curskc, curxc, curslc, cursuc, 0.0));
			}
		}

		// Set all variables
		if (anyinfovar) {
			for (MSKintt xi = 0; xi < NUMVAR; xi++)
			{
				MSKstakeye curskx;
				if (isEmpty(skx) || CHARACTER_ELT(skx,xi).empty()) {
					curskx = MSK_SK_UNK;
				} else {
					MSKintt skxval;
					errcatch( MSK_strtosk(task, const_cast<MSKCONST char*>(CHARACTER_ELT(skx,xi).c_str()), &skxval) );
					curskx = static_cast<MSKstakeye>(skxval);
				}

				MSKrealt curxx;
				if (isEmpty(xx)) {

					// Variable not fully defined => use bound information
					MSKboundkeye bk;
					MSKrealt bl,bu;
					errcatch( MSK_getbound(task,
							MSK_ACC_VAR, xi, &bk, &bl, &bu));

					switch (bk) {
						case MSK_BK_FX:
						case MSK_BK_LO:
						case MSK_BK_RA:
							curxx = bl;
							break;
						case MSK_BK_UP:
							curxx = bu;
							break;
						case MSK_BK_FR:
							curxx = 0.0;
							break;
						default:
							throw msk_exception("Unexpected boundkey when loading initial solution");
					}

				} else {
					curxx = NUMERIC_ELT(xx, xi);
				}

				MSKrealt curslx;
				if (isEmpty(slx)) {
					curslx = 0.0;
				} else {
					curslx = NUMERIC_ELT(slx, xi);
				}

				MSKrealt cursux;
				if (isEmpty(sux)) {
					cursux = 0.0;
				} else {
					cursux = NUMERIC_ELT(sux, xi);
				}

				MSKrealt cursnx;
				if (isEmpty(snx)) {
					cursnx = 0.0;
				} else {
					cursnx = NUMERIC_ELT(snx, xi);
				}

				errcatch( MSK_putsolutioni(
						task, MSK_ACC_VAR, xi, cursoltype,
						curskx, curxx, curslx, cursux, cursnx));
			}
		}

		if (!anyinfocon && !anyinfovar) {
			printwarning("The initial solution '" + name + "' was ignored.");
		}
	}
}

void set_boundkey(double bl, double bu, MSKboundkeye *bk)
{
	if (isnan(bl) || isnan(bu))
		throw msk_exception("NAN values not allowed in bounds");

	if (!isinf(bl) && !isinf(bu) && bl > bu)
		throw msk_exception("The upper bound should be larger than the lower bound");

	if (isinf(bl) && ispos(bl))
		throw msk_exception("+INF values not allowed as lower bound");

	if (isinf(bu) && !ispos(bu))
		throw msk_exception("-INF values not allowed as upper bound");

	// Return the bound key
	if (isinf(bl))
	{
		if (isinf(bu))
			*bk = MSK_BK_FR;
		else
			*bk = MSK_BK_UP;
	}
	else
	{
		if (isinf(bu))
			*bk = MSK_BK_LO;
		else
		{
			// Normally this should be abs(bl-bu) <= eps, but we
			// require bl and bu stems from same double variable.
			if (bl == bu)
				*bk = MSK_BK_FX;
			else
				*bk = MSK_BK_RA;
		}
	}
}

void get_boundvalues(MSKtask_t task, double *lower, double* upper, MSKaccmodee boundtype, MSKintt numbounds)
{
	auto_array<MSKboundkeye> bk( new MSKboundkeye[numbounds] );

	// Get bound keys from MOSEK
	errcatch( MSK_getboundslice(task, boundtype, 0, numbounds, bk, lower, upper) );

	for (MSKintt i=0; i<numbounds; i++) {
		switch (bk[i]) {
			case MSK_BK_FR:
				lower[i] = -INFINITY;
				upper[i] = INFINITY;
				break;
			case MSK_BK_LO:
				upper[i] = INFINITY;
				break;
			case MSK_BK_UP:
				lower[i] = -INFINITY;
				break;
			default:
				break;
		}
	}
}

void get_mskparamtype(MSKtask_t task, string type, string name, MSKparametertypee *ptype, MSKintt *pidx)
{
	// Convert name to mosek input with correct prefix
	strtoupper(name);
	if (type == "iparam") append_mskprefix(name, "MSK_IPAR_"); else
	if (type == "dparam") append_mskprefix(name, "MSK_DPAR_"); else
	if (type == "sparam") append_mskprefix(name, "MSK_SPAR_"); else
		throw msk_exception("A parameter type was not recognized");

	errcatch( MSK_whichparam(task, const_cast<MSKCONST char*>(name.c_str()), ptype, pidx) );
}

/* This function checks and sets the parameters of the MOSEK task. */
void set_parameter(MSKtask_t task, string type, string name, SEXP value)
{
	if (isEmpty(value)) {
		printwarning("The parameter '" + name + "' from " + type + " was ignored due to an empty definition.");
		return;
	}

	if (Rf_length(value) >= 2) {
		throw msk_exception("The parameter '" + name + "' from " + type + " had more than one element in its definition.");
	}

	MSKparametertypee ptype;
	MSKintt pidx;
	get_mskparamtype(task, type, name, &ptype, &pidx);

	switch (ptype) {
		case MSK_PAR_INT_TYPE:
		{
			int mskvalue;

			if (Rf_isNumeric(value))
				mskvalue = INTEGER_ELT(value, 0);

			else if (Rf_isString(value)) {
				string valuestr = CHARACTER_ELT(value, 0);

				// Convert value string to mosek input
				strtoupper(valuestr);
				append_mskprefix(valuestr, "MSK_");

				char mskvaluestr[MSK_MAX_STR_LEN];
				if (!MSK_symnamtovalue(const_cast<MSKCONST char*>(valuestr.c_str()), mskvaluestr))
					throw msk_exception("The value of parameter '" + name + "' from " + type + " was not recognized");

				mskvalue = atoi(mskvaluestr);

			} else {
				throw msk_exception("The value of parameter '" + name + "' from " + type + " should be an integer or string");
			}

			errcatch( MSK_putintparam(task, static_cast<MSKiparame>(pidx), mskvalue) );
			break;
		}

		case MSK_PAR_DOU_TYPE:
		{
			if (!Rf_isNumeric(value))
				throw msk_exception("The value of parameter '" + name + "' from " + type + " should be a double");

			double mskvalue = NUMERIC_ELT(value, 0);

			errcatch( MSK_putdouparam(task, static_cast<MSKdparame>(pidx), mskvalue) );
			break;
		}

		case MSK_PAR_STR_TYPE:
		{
			if (!Rf_isString(value))
				throw msk_exception("The value of parameter '" + name + "' from " + type + " should be a string");

			string mskvalue = CHARACTER_ELT(value, 0);

			errcatch( MSK_putstrparam(task, static_cast<MSKsparame>(pidx), const_cast<MSKCONST char*>(mskvalue.c_str())) );
			break;
		}

		default:
			throw msk_exception("Parameter '" + name + "' from " + type + " was not recognized.");
	}
}

void append_parameters(MSKtask_t task, SEXP_LIST iparam, SEXP_LIST dparam, SEXP_LIST sparam) {

	/* Set integer parameters */
	{
		R_len_t numobjects = numeric_cast<int>(Rf_length(iparam));

		SEXP names = Rf_getAttrib(iparam, R_NamesSymbol);
		if (Rf_length(names) != numobjects)
			throw msk_exception("Mismatching number of elements and names in variable named '" + problem_type::R_ARGS.iparam + "'");

		for (R_len_t j=0; j < numobjects; ++j)
			set_parameter(task, "iparam", CHARACTER_ELT(names,j), VECTOR_ELT(iparam,static_cast<int>(j)));
	}


	/* Set double parameters */
	{
		R_len_t numobjects = numeric_cast<int>(Rf_length(dparam));

		SEXP names = Rf_getAttrib(dparam, R_NamesSymbol);
		if (Rf_length(names) != numobjects)
			throw msk_exception("Mismatching number of elements and names in variable named '" + problem_type::R_ARGS.dparam + "'");

		for (R_len_t j=0; j < numobjects; ++j)
			set_parameter(task, "dparam", CHARACTER_ELT(names,j), VECTOR_ELT(dparam,static_cast<int>(j)));
	}

	/* Set string parameters */
	{
		R_len_t numobjects = numeric_cast<int>(Rf_length(sparam));

		SEXP names = Rf_getAttrib(sparam, R_NamesSymbol);
		if (Rf_length(names) != numobjects)
			throw msk_exception("Mismatching number of elements and names in variable named '" + problem_type::R_ARGS.sparam + "'");

		for (R_len_t j=0; j < numobjects; ++j)
			set_parameter(task, "sparam", CHARACTER_ELT(names,j), VECTOR_ELT(sparam,static_cast<int>(j)));
	}
}

void get_int_parameters(SEXP_NamedVector &paramvec, MSKtask_t task)
{
	char paramname[MSK_MAX_STR_LEN];
	char valuename[MSK_MAX_STR_LEN];	MSKintt value;
	for (int v=MSK_IPAR_BEGIN; v<MSK_IPAR_END; ++v) {

		// Get name of parameter
		errcatch( MSK_getparamname(task, MSK_PAR_INT_TYPE, v, paramname) );

		string paramstr = paramname;
		remove_mskprefix(paramstr, "MSK_IPAR_");

		// Get value of parameter
		errcatch( MSK_getintparam(task, static_cast<MSKiparame>(v), &value) );
		errcatch( MSK_iparvaltosymnam(global_env, static_cast<MSKiparame>(v), value, valuename) );

		string valuestr = valuename;
		remove_mskprefix(valuestr, "MSK_");

		// Append parameter to list
		if (!valuestr.empty())
			paramvec.pushback(paramstr, valuestr);
		else
			paramvec.pushback(paramstr, value);
	}
}

void get_dou_parameters(SEXP_NamedVector &paramvec, MSKtask_t task)
{
	char paramname[MSK_MAX_STR_LEN];	MSKrealt value;
	for (int v=MSK_DPAR_BEGIN; v<MSK_DPAR_END; ++v) {

		// Get name of parameter
		errcatch( MSK_getparamname(task, MSK_PAR_DOU_TYPE, v, paramname) );

		string paramstr = paramname;
		remove_mskprefix(paramstr, "MSK_DPAR_");

		// Get value of parameter
		errcatch( MSK_getdouparam(task, static_cast<MSKdparame>(v), &value) );

		// Append parameter to list
		paramvec.pushback(paramstr, value);
	}
}

void get_str_parameters(SEXP_NamedVector &paramvec, MSKtask_t task)
{
	char paramname[MSK_MAX_STR_LEN];
	for (int v=MSK_SPAR_BEGIN; v<MSK_SPAR_END; ++v) {

		// Get name of parameter
		errcatch( MSK_getparamname(task, MSK_PAR_STR_TYPE, v, paramname) );

		string paramstr = paramname;
		remove_mskprefix(paramstr, "MSK_SPAR_");

		// Prepare for value of parameter by retrieving length
		size_t strlength;
		errcatch( MSK_getstrparam(task, static_cast<MSKsparame>(v), 0, &strlength, NULL) );

		// Terminating null-character not counted by 'MSK_getstrparam'
		++strlength;

		// Get value of parameter
		auto_array<char> value ( new char[strlength] );
		errcatch( MSK_getstrparam(task, static_cast<MSKsparame>(v), strlength, NULL, value) );

		// Append parameter to list
		paramvec.pushback(paramstr, value);
	}
}

bool isdef_solitem(MSKsoltypee s, MSKsoliteme v)
{
	switch (v)
	{
		// Primal variables
		case MSK_SOL_ITEM_XC:
		case MSK_SOL_ITEM_XX:
			switch (s) {
				case MSK_SOL_ITR:  return true;
				case MSK_SOL_BAS:  return true;
				case MSK_SOL_ITG:  return true;
				default:
					throw msk_exception("A solution type was not supported");
			} break;

		// Linear dual variables
		case MSK_SOL_ITEM_SLC:
		case MSK_SOL_ITEM_SLX:
		case MSK_SOL_ITEM_SUC:
		case MSK_SOL_ITEM_SUX:
			switch (s) {
				case MSK_SOL_ITR:  return true;
				case MSK_SOL_BAS:  return true;
				case MSK_SOL_ITG:  return false;
				default:
					throw msk_exception("A solution type was not supported");
			} break;

		// Conic dual variable
		case MSK_SOL_ITEM_SNX:
			switch (s) {
				case MSK_SOL_ITR:  return true;
				case MSK_SOL_BAS:  return false;
				case MSK_SOL_ITG:  return false;
				default:
					throw msk_exception("A solution type was not supported");
			} break;

		// Ignored variables
		case MSK_SOL_ITEM_Y:
			return false;
			break;

		// Unsupported variables
		default:
			throw msk_exception("A solution item was not supported");
	}
}

void getspecs_soltype(MSKsoltypee stype, string &name)
{
	switch (stype) {
		case MSK_SOL_BAS:
			name = "bas";
			break;
		case MSK_SOL_ITR:
			name = "itr";
			break;
		case MSK_SOL_ITG:
			name = "int";
			break;
		default:
			throw msk_exception("A solution type was not supported");
	}
}

void getspecs_solitem(MSKsoliteme vtype, MSKintt NUMVAR, MSKintt NUMCON, string &name, MSKidxt &size)
{
	switch (vtype) {
		case MSK_SOL_ITEM_SLC:
			name = "slc";
			size = NUMCON;
			break;
		case MSK_SOL_ITEM_SLX:
			name = "slx";
			size = NUMVAR;
			break;
		case MSK_SOL_ITEM_SNX:
			name = "snx";
			size = NUMVAR;
			break;
		case MSK_SOL_ITEM_SUC:
			name = "suc";
			size = NUMCON;
			break;
		case MSK_SOL_ITEM_SUX:
			name = "sux";
			size = NUMVAR;
			break;
		case MSK_SOL_ITEM_XC:
			name = "xc";
			size = NUMCON;
			break;
		case MSK_SOL_ITEM_XX:
			name = "xx";
			size = NUMVAR;
			break;
		case MSK_SOL_ITEM_Y:
			name = "y";
			size = NUMCON;
			break;
		default:
			throw msk_exception("A solution item was not supported");
	}
}

void msk_getsolution(SEXP_Handle &sol, MSKtask_t task)
{
  printdebug("msk_getsolution was called");

	MSKintt NUMVAR, NUMCON;
	errcatch( MSK_getnumvar(task, &NUMVAR) );
	errcatch( MSK_getnumcon(task, &NUMCON) );

	SEXP_NamedVector solvec;	solvec.initVEC(MSK_SOL_END - MSK_SOL_BEGIN);	sol.protect(solvec);

	// Construct: result -> solution -> solution types
	for (int s=MSK_SOL_BEGIN; s<MSK_SOL_END; ++s)
	{
		SEXP_NamedVector soltype;
		soltype.initVEC(4 + MSK_SOL_ITEM_END);

		MSKsoltypee stype = static_cast<MSKsoltypee>(s);
		MSKintt isdef_soltype;
		errcatch( MSK_solutiondef(task, stype, &isdef_soltype) );

		if (!isdef_soltype)
			continue;

		// Add the problem status and solution status
		MSKprostae prosta;
		MSKsolstae solsta;
		errcatch( MSK_getsolutionstatus(task, stype, &prosta, &solsta) );

		char solsta_str[MSK_MAX_STR_LEN];
		errcatch( MSK_solstatostr(task, solsta, solsta_str) );
		soltype.pushback("solsta", solsta_str);

		char prosta_str[MSK_MAX_STR_LEN];
		errcatch( MSK_prostatostr(task, prosta, prosta_str) );
		soltype.pushback("prosta", prosta_str);

		// Add the constraint status keys
		{
			auto_array<MSKstakeye> mskskc ( new MSKstakeye[NUMCON] );

			errcatch( MSK_getsolutionstatuskeyslice(task,
									MSK_ACC_CON,	/* Request constraint status keys. */
									stype,			/* Current solution type. */
									0,				/* Index of first variable. */
									NUMCON,			/* Index of last variable+1. */
									mskskc));

			SEXP_Vector skcvec;		skcvec.initSTR(NUMCON, false);
			char skcname[MSK_MAX_STR_LEN];
			for (MSKintt ci = 0; ci < NUMCON; ci++) {
				errcatch( MSK_sktostr(task, mskskc[ci], skcname) );
				skcvec.pushback(skcname);
			}
			soltype.pushback("skc", skcvec);
		}

		// Add the variable status keys
		{
			auto_array<MSKstakeye> mskskx ( new MSKstakeye[NUMVAR] );

			errcatch( MSK_getsolutionstatuskeyslice(task,
									MSK_ACC_VAR,	/* Request variable status keys. */
									stype,			/* Current solution type. */
									0,				/* Index of first variable. */
									NUMVAR,			/* Index of last variable+1. */
									mskskx));

			SEXP_Vector skxvec;		skxvec.initSTR(NUMVAR, false);
			char skxname[MSK_MAX_STR_LEN];
			for (MSKintt xi = 0; xi < NUMVAR; xi++) {
				errcatch( MSK_sktostr(task, mskskx[xi], skxname) );
				skxvec.pushback(skxname);
			}
			soltype.pushback("skx", skxvec);
		}

		// Add solution variable slices
		for (int v=MSK_SOL_ITEM_BEGIN; v<MSK_SOL_ITEM_END; ++v)
		{
			MSKsoliteme vtype = (MSKsoliteme)v;

			if (!isdef_solitem(stype, vtype))
				continue;

			string vname;
			MSKidxt vsize;
			getspecs_solitem(vtype, NUMVAR, NUMCON, vname, vsize);

			SEXP_Vector xxvec;	xxvec.initREAL(vsize);
			double *pxx = REAL(xxvec);
			errcatch( MSK_getsolutionslice(task,
									stype, 		/* Request current solution type. */
									vtype,		/* Which part of solution. */
									0, 			/* Index of first variable. */
									vsize, 		/* Index of last variable+1. */
									pxx));

			soltype.pushback(vname, xxvec);
		}

		string sname;
		getspecs_soltype(stype, sname);

		solvec.pushback(sname, soltype);
	}

	// No need to proctect the solvec if nothing was added
	if (solvec.size() == 0) {
		sol.protect(R_NilValue);
	}
}

void msk_loadproblem(Task_handle &task, problem_type &probin)
{
	R_len_t NUMANZ = probin.A->nnz();
	MSKintt NUMCON = probin.A->nrow();
	MSKintt NUMVAR = probin.A->ncol();
	MSKintt NUMINTVAR = numeric_cast<MSKintt>(Rf_length(probin.intsub));

	/* Bounds on constraints. */
	MSKboundkeye bkc[NUMCON];
	for (int i=0; i<NUMCON; i++)
		set_boundkey(RNUMERICMATRIX_ELT(probin.bc,0,i), RNUMERICMATRIX_ELT(probin.bc,1,i), &bkc[i]);

	/* Bounds on variables. */
	MSKboundkeye bkx[NUMVAR];
	for (int i=0; i<NUMVAR; i++)
		set_boundkey(RNUMERICMATRIX_ELT(probin.bx,0,i), RNUMERICMATRIX_ELT(probin.bx,1,i), &bkx[i]);

	// Make sure the environment is initialized
	global_env.init();

	try
	{
		/* Create the task */
		task.init(global_env, NUMCON, NUMVAR);

		/* Give MOSEK an estimate of the size of the input data.
		 * This is done to increase the speed of inputting data.
		 * However, it is optional. */
		errcatch( MSK_putmaxnumvar(task, NUMVAR) );
		errcatch( MSK_putmaxnumcon(task, NUMCON) );
		errcatch( MSK_putmaxnumanz(task, numeric_cast<MSKintt>(NUMANZ)) ); 		// FIXME: Use MSK_putmaxnumanz64 when R_len_t is type 'long'

		/* Append 'NUMCON' empty constraints.
		 * The constraints will initially have no bounds. */
		errcatch( MSK_append(task, MSK_ACC_CON, NUMCON) );

		/* Append 'NUMVAR' variables.
		 * The variables will initially be fixed at zero (x=0). */
		errcatch( MSK_append(task, MSK_ACC_VAR, NUMVAR) );

		/* Optionally add a constant term to the objective. */
		errcatch( MSK_putcfix(task, probin.c0) );

		for (MSKidxt j=0; j<NUMVAR; ++j)
		{
			/* Set the linear term c_j in the objective.*/
			errcatch( MSK_putcj(task, j, NUMERIC_ELT(probin.c,j)) );

			/* Set the bounds on variable j.
			   blx[j] <= x_j <= bux[j] */
			errcatch( MSK_putbound(task,
						MSK_ACC_VAR,							/* Put bounds on variables.*/
						j, 										/* Index of variable.*/
						bkx[j],  								/* Bound key.*/
						RNUMERICMATRIX_ELT(probin.bx,0,j),  	/* Numerical value of lower bound.*/
						RNUMERICMATRIX_ELT(probin.bx,1,j))); 	/* Numerical value of upper bound.*/
		}

		/* Input columns of A */
		probin.A->MOSEK_write(task);

		/* Set the bounds on constraints.
		 * for i=1, ...,NUMCON : blc[i] <= constraint i <= buc[i] */
		for (MSKidxt i=0; i<NUMCON; ++i)
			errcatch( MSK_putbound(task,
					MSK_ACC_CON,							/* Put bounds on constraints.*/
					i,										/* Index of constraint.*/
					bkc[i],									/* Bound key.*/
					RNUMERICMATRIX_ELT(probin.bc,0,i),		/* Numerical value of lower bound.*/
					RNUMERICMATRIX_ELT(probin.bc,1,i)));	/* Numerical value of upper bound.*/

		/* Set the conic constraints. */
		probin.cones.MOSEK_write(task);

		/* Set the scopt operators. */
		probin.scopt.MOSEK_write(task, probin);

		/* Set the integer variables */
		for (MSKidxt j=0; j<NUMINTVAR; ++j)
			errcatch( MSK_putvartype(task, INTEGER_ELT(probin.intsub,j)-1, MSK_VAR_TYPE_INT) );

		/* Set objective sense. */
		errcatch( MSK_putobjsense(task, probin.sense) );

	} catch (exception const& e) {
		printoutput("An error occurred while setting up the problem.\n", typeERROR);
		throw;
	}
}

___RMSK_INNER_NS_END___
