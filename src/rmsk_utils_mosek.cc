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

bool isdef_solitem(MSKsoltypee s, MSKsoliteme v, bool isGeneralNLP)
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
				case MSK_SOL_ITR:  return !isGeneralNLP;  // No conic information in a general non-linear programming solution
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

void getspecs_sk_solitem(MSK_sk_soliteme vtype, MSKintt NUMVAR, MSKintt NUMCON, MSKintt NUMCONES, string &name, MSKidxt &size)
{
	switch (vtype) {
		case MSK_SK_SOL_ITEM_SKC:
			name = "skc";
			size = NUMCON;
			break;
		case MSK_SK_SOL_ITEM_SKX:
			name = "skx";
			size = NUMVAR;
			break;
		case MSK_SK_SOL_ITEM_SKN:
			name = "skn";
			size = NUMCONES;
			break;
		default:
			throw msk_exception("A solution status key item was not supported");
	}
}

void getspecs_solitem(MSKsoliteme vtype, MSKintt NUMVAR, MSKintt NUMCON, string &name, MSKidxt &size)
{
	switch (vtype) {
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
		case MSK_SOL_ITEM_SLC:
			name = "slc";
			size = NUMCON;
			break;
		case MSK_SOL_ITEM_SUC:
			name = "suc";
			size = NUMCON;
			break;
		case MSK_SOL_ITEM_SLX:
			name = "slx";
			size = NUMVAR;
			break;
		case MSK_SOL_ITEM_SUX:
			name = "sux";
			size = NUMVAR;
			break;
		case MSK_SOL_ITEM_SNX:
			name = "snx";
			size = NUMVAR;
			break;
		default:
			throw msk_exception("A solution item was not supported");
	}
}

void append_initsol(MSKtask_t task, SEXP_LIST initsol, MSKintt NUMCON, MSKintt NUMVAR, MSKintt NUMCONES)
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

		bool any_info = false;

		MSKstakeye *solitem_sk[MSK_SK_SOL_ITEM_END];
		auto_array<MSKstakeye> solitem_sk_bank[MSK_SK_SOL_ITEM_END];
		{
			string name; MSKidxt size;

			for (int i = MSK_SK_SOL_ITEM_BEGIN; i < MSK_SK_SOL_ITEM_END; ++i) {
				MSK_sk_soliteme itype = static_cast<MSK_sk_soliteme>(i);
				solitem_sk[i] = NULL;

				SEXP_Handle skvalue;
				getspecs_sk_solitem(itype, NUMVAR, NUMCON, NUMCONES, name, size);
				list_seek_Character(skvalue, cursol, name, true);  validate_Character(skvalue, name, size, true);

				if (!isEmpty(skvalue)) {
					any_info = true;

					solitem_sk_bank[i].protect(new MSKstakeye[size]);
					solitem_sk[i] = solitem_sk_bank[i];

					MSKintt curval;
					for (MSKintt j = 0; j < size; j++) {
						errcatch( MSK_strtosk(task, const_cast<MSKCONST char*>(CHARACTER_ELT(skvalue,j).c_str()), &curval) );
						solitem_sk[i][j] = static_cast<MSKstakeye>(curval);
					}
				}
			}
		}

		MSKrealt *solitem_dou[MSK_SOL_ITEM_END];
		auto_array<MSKrealt> solitem_dou_bank[MSK_SOL_ITEM_END];
		{
			string name; MSKidxt size;

			for (int i = MSK_SOL_ITEM_BEGIN; i < MSK_SOL_ITEM_END; ++i) {
				MSKsoliteme itype = static_cast<MSKsoliteme>(i);
				solitem_dou[i] = NULL;

				SEXP_Handle skvalue;
				getspecs_solitem(itype, NUMVAR, NUMCON, name, size);
				list_seek_Numeric(skvalue, cursol, name, true);  validate_Numeric(skvalue, name, size, true);

				if (!isEmpty(skvalue)) {
					any_info = true;

					if (Rf_isReal(skvalue)) {
						solitem_dou[i] = REAL(skvalue);

					} else {
						solitem_dou_bank[i].protect(new MSKrealt[size]);
						solitem_dou[i] = solitem_dou_bank[i];

						MSKintt *curval = INTEGER(skvalue);
						for (MSKintt j = 0; j < size; j++) {
							solitem_dou[i][j] = static_cast<MSKrealt>(curval[j]);
						}
					}
				}
			}
		}

		if (any_info) {

			errcatch( MSK_putsolution(task, cursoltype,
				solitem_sk[MSK_SK_SOL_ITEM_SKC],
				solitem_sk[MSK_SK_SOL_ITEM_SKX],
				solitem_sk[MSK_SK_SOL_ITEM_SKN],
				solitem_dou[MSK_SOL_ITEM_XC],
				solitem_dou[MSK_SOL_ITEM_XX],
				NULL,
				solitem_dou[MSK_SOL_ITEM_SLC],
				solitem_dou[MSK_SOL_ITEM_SUC],
				solitem_dou[MSK_SOL_ITEM_SLX],
				solitem_dou[MSK_SOL_ITEM_SUX],
				solitem_dou[MSK_SOL_ITEM_SNX] ));

		} else {
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

void msk_getsolution(SEXP_Handle &sol, MSKtask_t task, options_type &options)
{
  printdebug("msk_getsolution was called");

	MSKintt NUMCON, NUMVAR, NUMCONES;
	errcatch( MSK_getnumcon(task, &NUMCON) );
	errcatch( MSK_getnumvar(task, &NUMVAR) );
	errcatch( MSK_getnumcone(task, &NUMCONES) );

	MSKnlgetspfunc nlspfunc; MSKnlgetvafunc nlvafunc;
	errcatch( MSK_getnlfunc(task, NULL, &nlspfunc, &nlvafunc) );
    bool isGeneralNLP = (nlspfunc != NULL) || (nlvafunc != NULL);

	SEXP_NamedVector solvec;	solvec.initVEC(MSK_SOL_END);	sol.protect(solvec);

	// Construct: result -> solution -> solution types
	for (int s=MSK_SOL_BEGIN; s<MSK_SOL_END; ++s)
	{
		SEXP_NamedVector soltype;
		soltype.initVEC(2+2+2+1 + MSK_SOL_ITEM_END);

		MSKsoltypee stype = static_cast<MSKsoltypee>(s);
		MSKintt isdef_soltype;
		errcatch( MSK_solutiondef(task, stype, &isdef_soltype) );

		if (!isdef_soltype)
			continue;

		// Allocate memory structures
		MSKprostae prosta;
		MSKsolstae solsta;
		auto_array<MSKstakeye> solitem_sk_bank[MSK_SK_SOL_ITEM_END];
		{
			string name; MSKidxt size;
			for (int i = MSK_SK_SOL_ITEM_BEGIN; i < MSK_SK_SOL_ITEM_END; ++i) {
				getspecs_sk_solitem(static_cast<MSK_sk_soliteme>(i), NUMVAR, NUMCON, NUMCONES, name, size);
				solitem_sk_bank[i].protect(new MSKstakeye[size]);
			}
		}
		SEXP_Vector solitem_dou_bank[MSK_SOL_ITEM_END];
		{
			string name; MSKidxt size;
			for (int i = MSK_SOL_ITEM_BEGIN; i < MSK_SOL_ITEM_END; ++i) {
				getspecs_solitem(static_cast<MSKsoliteme>(i), NUMVAR, NUMCON, name, size);
				solitem_dou_bank[i].initREAL(size);
			}
		}

		// Extract solution information
		errcatch( MSK_getsolution(task, stype,
						&prosta,
						&solsta,
						solitem_sk_bank[MSK_SK_SOL_ITEM_SKC],
						solitem_sk_bank[MSK_SK_SOL_ITEM_SKX],
						solitem_sk_bank[MSK_SK_SOL_ITEM_SKN],
						REAL(solitem_dou_bank[MSK_SOL_ITEM_XC]),
						REAL(solitem_dou_bank[MSK_SOL_ITEM_XX]),
						NULL,
						REAL(solitem_dou_bank[MSK_SOL_ITEM_SLC]),
						REAL(solitem_dou_bank[MSK_SOL_ITEM_SUC]),
						REAL(solitem_dou_bank[MSK_SOL_ITEM_SLX]),
						REAL(solitem_dou_bank[MSK_SOL_ITEM_SUX]),
						REAL(solitem_dou_bank[MSK_SOL_ITEM_SNX]) ));

		// Add the problem status and solution status
		char solsta_str[MSK_MAX_STR_LEN];
		errcatch( MSK_solstatostr(task, solsta, solsta_str) );
		soltype.pushback("solsta", solsta_str);

		char prosta_str[MSK_MAX_STR_LEN];
		errcatch( MSK_prostatostr(task, prosta, prosta_str) );
		soltype.pushback("prosta", prosta_str);

		// Add the constraint, variable and conic status keys
		{
			string name; MSKidxt size; char skstrvalue[MSK_MAX_STR_LEN];
			for (int i = MSK_SK_SOL_ITEM_BEGIN; i < MSK_SK_SOL_ITEM_END; ++i) {
				MSK_sk_soliteme itype = static_cast<MSK_sk_soliteme>(i);

				if (itype == MSK_SK_SOL_ITEM_SKN) {
					// No conic information in either simplex or general non-linear programming.
					if (stype == MSK_SOL_BAS || isGeneralNLP)
						continue;
				}

				getspecs_sk_solitem(itype, NUMVAR, NUMCON, NUMCONES, name, size);

				SEXP_Vector skvec;		skvec.initSTR(size, false);
				for (MSKintt j = 0; j < size; j++) {
					errcatch( MSK_sktostr(task, solitem_sk_bank[i][j], skstrvalue) );
					skvec.pushback(skstrvalue);
				}
				soltype.pushback(name, skvec);
			}
		}

		// Add solution variables
		{
			string name; MSKidxt size;
			for (int i = MSK_SOL_ITEM_BEGIN; i < MSK_SOL_ITEM_END; ++i) {
				MSKsoliteme itype = static_cast<MSKsoliteme>(i);

				if (!isdef_solitem( stype, itype, isGeneralNLP ))
					continue;

				getspecs_solitem(itype, NUMVAR, NUMCON, name, size);
				soltype.pushback(name, solitem_dou_bank[i]);
			}
		}

		// Optional: Add primal and dual objective
		if (options.soldetail >= 1) {
			MSKrealt primalobj, dualobj;

			const string PRIMAL_OBJECTIVE = "pobjval";
			const string DUAL_OBJECTIVE = "dobjval";
			const string PRIMAL_BOUND = "pobjbound";

			switch (stype) {
				case MSK_SOL_ITR:
				case MSK_SOL_BAS:
					errcatch( MSK_getdouinf(task, MSK_DINF_SOL_ITR_PRIMAL_OBJ, &primalobj) );
					soltype.pushback(PRIMAL_OBJECTIVE, primalobj);

					errcatch( MSK_getdouinf(task, MSK_DINF_SOL_ITR_DUAL_OBJ, &dualobj) );
					soltype.pushback(DUAL_OBJECTIVE, dualobj);
					break;

				case MSK_SOL_ITG:
					errcatch( MSK_getdouinf(task, MSK_DINF_SOL_INT_PRIMAL_OBJ, &primalobj) );
					soltype.pushback(PRIMAL_OBJECTIVE, primalobj);

					MSKintt num_relax;
					errcatch( MSK_getintinf(task, MSK_IINF_MIO_NUM_RELAX, &num_relax) );
					if (num_relax >= 1) {
						MSKrealt primalobjbound;
						errcatch( MSK_getdouinf(task, MSK_DINF_MIO_OBJ_BOUND, &primalobjbound) );
						soltype.pushback(PRIMAL_BOUND, primalobjbound);
					} else {
						soltype.pushback(PRIMAL_BOUND, NA_REAL);
					}
					break;
				default:
					throw msk_exception("A solution type was not supported");
					break;
			}
		}

		// Optional: Add maximal infeasibilities
		if (options.soldetail >= 2) {
			SEXP_NamedVector vecinfeas;
			vecinfeas.initVEC(7);

			MSKrealt maxpbi, maxpcni, maxpeqi, maxinti;
			MSKrealt maxdbi, maxdcni, maxdeqi;
			errcatch( MSK_getsolutioninf(task, stype, NULL, NULL, NULL,
						&maxpbi,
						&maxpcni,
						&maxpeqi,
						&maxinti, NULL,
						&maxdbi,
						&maxdcni,
						&maxdeqi) );

			const string PRIMAL_INEQUALITY 	= "pbound"; 	// MSK_DINF_SOL_ITR_MAX_PBI
			const string PRIMAL_EQUALITY 	= "peq"; 		// MSK_DINF_SOL_ITR_MAX_PEQI
			const string PRIMAL_CONE 		= "pcone"; 		// MSK_DINF_SOL_ITR_MAX_PCNI
			const string DUAL_INEQUALITY 	= "dbound";  	// MSK_DINF_SOL_ITR_MAX_DBI
			const string DUAL_EQUALITY 		= "deq"; 		// MSK_DINF_SOL_ITR_MAX_DEQI
			const string DUAL_CONE 			= "dcone"; 		// MSK_DINF_SOL_ITR_MAX_DCNI
			const string PRIMAL_INT 		= "int"; 		// MSK_DINF_SOL_ITR_MAX_PINTI

			switch (s) {
				case MSK_SOL_ITR:
					vecinfeas.pushback(PRIMAL_INEQUALITY, maxpbi);
					vecinfeas.pushback(PRIMAL_EQUALITY, maxpeqi);
					vecinfeas.pushback(PRIMAL_CONE, maxpcni);
					vecinfeas.pushback(DUAL_INEQUALITY, maxdbi);
					vecinfeas.pushback(DUAL_EQUALITY, maxdeqi);
					vecinfeas.pushback(DUAL_CONE, maxdcni);
					break;

				case MSK_SOL_BAS:
					vecinfeas.pushback(PRIMAL_INEQUALITY, maxpbi);
					vecinfeas.pushback(PRIMAL_EQUALITY, maxpeqi);
					vecinfeas.pushback(DUAL_INEQUALITY, maxdbi);
					vecinfeas.pushback(DUAL_EQUALITY, maxdeqi);
					break;

				case MSK_SOL_ITG:
					vecinfeas.pushback(PRIMAL_INEQUALITY, maxpbi);
					vecinfeas.pushback(PRIMAL_EQUALITY, maxpeqi);
					vecinfeas.pushback(PRIMAL_CONE, maxpcni);
					vecinfeas.pushback(PRIMAL_INT, maxinti);
					break;

				default:
					throw msk_exception("A solution type was not supported");
					break;
			}
			soltype.pushback("maxinfeas", vecinfeas);
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

void msk_getoptimizationinfo(SEXP_NamedVector &ret_val, Task_handle &task)
{
	char mskinfoname[MSK_MAX_STR_LEN];

	// Integer-typed information: iinfo
	{
		SEXP_NamedVector vecinfo;
		vecinfo.initVEC((MSK_IINF_END - MSK_IINF_BEGIN) + (MSK_LIINF_END - MSK_LIINF_BEGIN));
		MSKintt msk32value;
		MSKint64t msk64value;

		// 32 bit integers
		for (int v=MSK_IINF_BEGIN; v<MSK_IINF_END; ++v)
		{
			MSKiinfiteme infotype = static_cast<MSKiinfiteme>(v);

			errcatch( MSK_getinfname(task, MSK_INF_INT_TYPE, infotype, mskinfoname) );
			string infoname = mskinfoname;
			remove_mskprefix(infoname, "MSK_IINF_");

			errcatch( MSK_getintinf(task, infotype, &msk32value) );
			vecinfo.pushback(infoname, msk32value);
		}

		// 64 bit integers
		for (int v=MSK_LIINF_BEGIN; v<MSK_LIINF_END; ++v)
		{
			MSKliinfiteme infotype = static_cast<MSKliinfiteme>(v);

			errcatch( MSK_getinfname(task, MSK_INF_LINT_TYPE, infotype, mskinfoname) );
			string infoname = mskinfoname;
			remove_mskprefix(infoname, "MSK_LIINF_");

			errcatch( MSK_getlintinf(task, infotype, &msk64value) );
			try {
				msk32value = numeric_cast<int>(msk64value);
			} catch (msk_exception const& ex) {
				msk32value = NA_INTEGER;
			}

			vecinfo.pushback(infoname, msk32value);
		}

		ret_val.pushback("iinfo", vecinfo);
	}

	// Double-typed information: dinfo
	{
		SEXP_NamedVector vecinfo;
		vecinfo.initVEC(MSK_DINF_END - MSK_DINF_BEGIN);

		// Double-typed information
		MSKrealt mskvalue;
		for (int v=MSK_DINF_BEGIN; v<MSK_DINF_END; ++v)
		{
			MSKdinfiteme infotype = static_cast<MSKdinfiteme>(v);

			errcatch( MSK_getinfname(task, MSK_INF_DOU_TYPE, infotype, mskinfoname) );
			string infoname = mskinfoname;
			remove_mskprefix(infoname, "MSK_DINF_");

			errcatch( MSK_getdouinf(task, infotype, &mskvalue) );
			vecinfo.pushback(infoname, mskvalue);
		}

		ret_val.pushback("dinfo", vecinfo);
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
