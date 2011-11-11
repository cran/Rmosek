// Copyright (C) 2011   MOSEK ApS
// Made by:
//   Henrik Alsing Friberg   <haf@mosek.com>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#define R_NO_REMAP
#include "rmsk_msg_system.h"
#include "rmsk_namespace.h"

#include "rmsk_sexp_methods.h"
#include "rmsk_utils.h"

using std::string;
using std::exception;
using namespace RMSK_INNER_NS;


extern "C"
{
	SEXP mosek(SEXP arg0, SEXP arg1);
	SEXP mosek_read(SEXP arg0, SEXP arg1);
	SEXP mosek_write(SEXP arg0, SEXP arg1, SEXP arg2);
	SEXP mosek_clean();
	SEXP mosek_version();
	void R_init_Rmosek(DllInfo *info);
	void R_unload_Rmosek(DllInfo *info);
}


SEXP mosek(SEXP arg0, SEXP arg1)
{
	const string ARGNAMES[] = {"problem","options"};
	const string ARGTYPES[] = {"list","list"};

	// Clean in case of Rf_error in previous run
	if (!mosek_interface_termination_success) {
		terminate_successfully();
	}

	// Create handled for returned data
	SEXP_NamedVector ret_val;

	try {
		// Start the program
		reset_global_variables();
		ret_val.initVEC(2);

		// Validate input arguments
		if (!isNamedVector(arg0)) {
			throw msk_exception("Input argument " + ARGNAMES[0] + " should be a " + ARGTYPES[0] + ".");
		}
		if (!isNamedVector(arg1)) {
			throw msk_exception("Input argument " + ARGNAMES[1] + " should be a " + ARGTYPES[1] + ".");
		}

		// Read input arguments: problem and options
		problem_type probin;
		probin.options.R_read(arg1);
		probin.R_read(arg0);

		// Create task and load problem into MOSEK
		Task_handle &task = global_task;
		probin.MOSEK_write(task);

		// Solve the problem
		msk_solve(ret_val, task, probin.options);

		// Print warning summary
		if (mosek_interface_warnings > 0) {
			printoutput("The R-to-MOSEK interface completed with " + tostring(mosek_interface_warnings) + " warning(s)\n\n", typeWARNING);
		}

	} catch (msk_exception const& e) {
		terminate_unsuccessfully(ret_val, e);
		return ret_val;

	} catch (exception const& e) {
		terminate_unsuccessfully(e.what());
		return ret_val;
	}

	// Clean allocations and exit (msk_solve adds response)
	terminate_successfully();
	return ret_val;
}


SEXP mosek_clean()
{
	// Print in case of Rf_error in previous run
	if (!mosek_interface_termination_success) {
		printoutput("----- MOSEK_CLEAN -----\n", typeERROR);
	}
	mosek_interface_verbose = typeALL;

	// Clean resources such as tasks before the environment!
	reset_global_ressources();
	global_env.~Env_handle();
	mosek_interface_termination_success = true;

	return R_NilValue;
}


SEXP mosek_version()
{
	MSKintt major, minor, build, revision;
	MSK_getversion(&major, &minor, &build, &revision);

	// Construct output
	SEXP_Vector ret_val;
	ret_val.initSTR(1, false);
	ret_val.pushback("MOSEK " + tostring(major) + "." +  tostring(minor) + "." + tostring(build) + "." + tostring(revision));

	return ret_val;
}


SEXP mosek_read(SEXP arg0, SEXP arg1)
{
	const string ARGNAMES[] = {"filepath","options"};
	const string ARGTYPES[] = {"string","list"};

	// Clean in case of Rf_error in previous run
	if (!mosek_interface_termination_success) {
		terminate_successfully();
	}

	// Create handled for returned data
	SEXP_NamedVector ret_val;

	try {
		// Start the program
		reset_global_variables();
		ret_val.initVEC(2);

		// Validate input arguments
		printdebug("Validate input arguments");
		string filepath = CHARACTER_ELT(arg0, 0);
		if (filepath.empty()) {
			throw msk_exception("Input argument " + ARGNAMES[0] + " should be a " + ARGTYPES[0] + ".");
		}
		if (!isNamedVector(arg1)) {
			throw msk_exception("Input argument " + ARGNAMES[1] + " should be a " + ARGTYPES[1] + ".");
		}

		// Define new default values for options
		options_type default_opts; {
			default_opts.useparam = false;
			default_opts.usesol = false;
		}

		// Read input arguments: options (with modified defaults)
		printdebug("Reading input arguments");
		problem_type probin;
		probin.options = default_opts;
		probin.options.R_read(arg1);

		// Create task and load filepath-model into MOSEK
		printdebug("Loading filepath-model into MOSEK");
		Task_handle &task = global_task;
		msk_loadproblemfile(task, filepath, probin.options.useparam);

		// Read the problem from MOSEK
		printdebug("Reading the problem from MOSEK");
		probin.MOSEK_read(task);

		// Write the problem to R
		printdebug("Writing the problem to R");
		SEXP_NamedVector prob_val;
		probin.R_write(prob_val);
		ret_val.pushback("prob", prob_val);

		// Print warning summary
		if (mosek_interface_warnings > 0) {
			printoutput("The R-to-MOSEK interface completed with " + tostring(mosek_interface_warnings) + " warning(s)\n", typeWARNING);
		}

	} catch (msk_exception const& e) {
		terminate_unsuccessfully(ret_val, e);
		return ret_val;

	} catch (exception const& e) {
		terminate_unsuccessfully(e.what());
		return ret_val;
	}

	// Clean allocations, add response and exit
	terminate_successfully(ret_val);
	return ret_val;
}


SEXP mosek_write(SEXP arg0, SEXP arg1, SEXP arg2)
{
	const string ARGNAMES[] = {"problem","filepath","options"};
	const string ARGTYPES[] = {"named list","string","named list"};

	// Clean in case of Rf_error in previous run
	if (!mosek_interface_termination_success) {
		terminate_successfully();
	}

	// Create handled for returned data
	SEXP_NamedVector ret_val;

	try {
		// Start the program
		reset_global_variables();
		ret_val.initVEC(1);

		// Validate input arguments
		if (!isNamedVector(arg0)) {
			throw msk_exception("Input argument '" + ARGNAMES[0] + "' should be a '" + ARGTYPES[0] + "'.");
		}
		string filepath = CHARACTER_ELT(arg1, 0);
		if (filepath.empty()) {
			throw msk_exception("Input argument '" + ARGNAMES[1] + "' should be a '" + ARGTYPES[1] + "'.");
		}
		if (!isNamedVector(arg2)) {
			throw msk_exception("Input argument '" + ARGNAMES[2] + "' should be a '" + ARGTYPES[2] + "'.");
		}

		// Define new default values for options
		options_type default_opts; {
			default_opts.useparam = false;
			default_opts.usesol = false;
		}

		// Read input arguments: options (with modified defaults)
		problem_type probin;
		probin.options = default_opts;
		probin.options.R_read(arg2);
		probin.R_read(arg0);

		// Create task and load problem into MOSEK
		Task_handle &task = global_task;
		probin.MOSEK_write(task);

		// Set export-parameters for whether to write any solution loaded into MOSEK
		if (probin.options.usesol) {
			errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_SOLUTIONS, MSK_ON) );
		} else {
			errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_SOLUTIONS, MSK_OFF) );
		}

		// Set export-parameters for whether to write all parameters
		if (probin.options.useparam) {
			errcatch( MSK_putintparam(task, MSK_IPAR_WRITE_DATA_PARAM,MSK_ON) );
			errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_PARAMETERS,MSK_ON) );
		} else {
			errcatch( MSK_putintparam(task, MSK_IPAR_WRITE_DATA_PARAM,MSK_OFF) );
			errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_PARAMETERS,MSK_OFF) );
		}

		// Write to filepath model (filetypes: .lp, .mps, .opf, .mbt)
		errcatch( MSK_writedata(task, const_cast<MSKCONST char*>(filepath.c_str())) );

		// Print warning summary
		if (mosek_interface_warnings > 0) {
			printoutput("The R-to-MOSEK interface completed with " + tostring(mosek_interface_warnings) + " warning(s)\n", typeWARNING);
		}

	} catch (msk_exception const& e) {
		terminate_unsuccessfully(ret_val, e);
		return ret_val;

	} catch (exception const& e) {
		terminate_unsuccessfully(e.what());
		return ret_val;
	}

	// Clean allocations, add response and exit
	terminate_successfully(ret_val);
	return ret_val;
}


void R_init_Rmosek(DllInfo *info)
{
	// Register mosek utilities to the R console
	R_CallMethodDef callMethods[] = {
			{"mosek", (DL_FUNC) &mosek, 2},
			{"mosek_clean", (DL_FUNC) &mosek_clean, 0},
			{"mosek_version", (DL_FUNC) &mosek_version, 0},
			{"mosek_write", (DL_FUNC) &mosek_write, 3},
			{"mosek_read" , (DL_FUNC) &mosek_read, 2},
			{NULL, NULL, 0}
	};
	R_registerRoutines(info, NULL, callMethods, NULL, NULL);

	// Register mosek utilities to other packages
	R_RegisterCCallable("Rmosek", "mosek", (DL_FUNC) &mosek);
	R_RegisterCCallable("Rmosek", "mosek_clean", (DL_FUNC) &mosek_clean);
	R_RegisterCCallable("Rmosek", "mosek_version", (DL_FUNC) &mosek_version);
	R_RegisterCCallable("Rmosek", "mosek_write", (DL_FUNC) &mosek_write);
	R_RegisterCCallable("Rmosek", "mosek_read", (DL_FUNC) &mosek_read);

	// Start cholmod environment
	M_R_cholmod_start(&chol);
}


void R_unload_Rmosek(DllInfo *info)
{
	// Finish cholmod environment
	M_cholmod_finish(&chol);
}
