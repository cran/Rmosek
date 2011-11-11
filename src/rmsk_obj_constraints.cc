#define R_NO_REMAP
#include "rmsk_obj_constraints.h"
#include "rmsk_sexp_methods.h"
#include "rmsk_utils.h"

___RMSK_INNER_NS_START___

using std::string;


// ------------------------------
// Class conicSOC_type
// ------------------------------

const conicSOC_type::ITEMS_type::R_ARGS_type conicSOC_type::ITEMS_type::R_ARGS;

void conicSOC_type::R_read(SEXP_LIST object) {
	if (initialized) {
		throw msk_exception("Internal error in conicSOC_type::R_read, a SOC list was already loaded");
	}

	printdebug("Started reading second order cone list from R");
	conevec.protect(object);
	numcones = Rf_length(object);

	initialized = true;
}

void conicSOC_type::R_write(SEXP_Handle &val) {
	if (!initialized) {
		throw msk_exception("Internal error in conicSOC_type::R_write, no SOC list loaded");
	}

	printdebug("Started writing second order cone list to R");
	val = conevec;
}

void conicSOC_type::MOSEK_read(Task_handle &task) {
	if (initialized) {
		throw msk_exception("Internal error in conicSOC_type::MOSEK_read, a SOC list was already loaded");
	}

	printdebug("Started reading second order cone list from MOSEK");

	errcatch( MSK_getnumcone(task, &numcones) );
	SEXP_NamedVector cones;	cones.initVEC(numcones);	conevec.protect(cones);

	for (MSKidxt idx=0; idx<numcones; ++idx) {
		SEXP_NamedVector cone;
		cone.initVEC(2);

		MSKintt numconemembers;
		MSK_getnumconemem(task, idx, &numconemembers);

		SEXP_Vector subvec;		subvec.initINT(numconemembers);
		MSKlidxt *psub = INTEGER(subvec);

		MSKconetypee msktype;
		errcatch( MSK_getcone(task, idx, &msktype, NULL, &numconemembers, psub) );

		// R indexes count from 1, not from 0 as MOSEK
		for (int k=0; k<numconemembers; k++) {
			psub[k]++;
		}

		char type[MSK_MAX_STR_LEN];
		errcatch( MSK_conetypetostr(task, msktype, type) );

		string strtype = type;
		remove_mskprefix(strtype,"MSK_CT_");

		cone.pushback("type", strtype);
		cone.pushback("sub", subvec);

		cones.pushback("", cone);
	}
	initialized = true;
}

void conicSOC_type::MOSEK_write(Task_handle &task) {
	if (!initialized) {
		throw msk_exception("Internal error in conicSOC_type::MOSEK_write, no SOC list loaded");
	}

	printdebug("Started writing second order cone list to MOSEK");

	for (MSKidxt idx=0; idx<numcones; ++idx) {
		SEXP conevalue = VECTOR_ELT(conevec, idx);

		// Get input list
		if(!isNamedVector(conevalue)) {
			throw msk_exception("The cone at index " + tostring(idx+1) + " should be a 'named list'");
		}
		SEXP_Handle cone(conevalue);

		string type;		list_seek_String(&type, cone, ITEMS.R_ARGS.type);
		SEXP_Handle sub;	list_seek_Numeric(sub, cone, ITEMS.R_ARGS.sub);
		validate_NamedVector(cone, "cones[[" + tostring(idx+1) + "]]", ITEMS.R_ARGS.arglist);

		// Convert type to mosek input
		strtoupper(type);
		append_mskprefix(type, "MSK_CT_");
		char msktypestr[MSK_MAX_STR_LEN];
		if(!MSK_symnamtovalue(const_cast<MSKCONST char*>(type.c_str()), msktypestr))
			throw msk_exception("The type of cone at index " + tostring(idx+1) + " was not recognized");

		MSKconetypee msktype = (MSKconetypee)atoi(msktypestr);

		// Convert sub type and indexing (Minus one because MOSEK indexes counts from 0, not from 1 as R)
		int msksub[Rf_length(sub)];
		for (int i=0; i<Rf_length(sub); i++)
			msksub[i] = INTEGER_ELT(sub,i) - 1;

		// Append the cone
		errcatch( MSK_appendcone(task,
				msktype,		/* The type of cone */
				0.0, 			/* For future use only, can be set to 0.0 */
				Rf_length(sub),	/* Number of variables */
				msksub) );		/* Variable indexes */
	}
}

___RMSK_INNER_NS_END___
