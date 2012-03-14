// Copyright (C) 2011   MOSEK ApS
// Made by:
//   Henrik Alsing Friberg   <haf@mosek.com>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or
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
// Linking this program statically or dynamically with other modules is making
// a combined work based on this program. Thus, the terms and conditions of
// the GNU Lesser General Public License cover the whole combination.
//
// In addition, as a special exception, the copyright holders of this program
// give you permission to combine this program with the MOSEK C/C++ API
// (or modified versions of such code). You may copy and distribute such a system
// following the terms of the GNU LGPL for this program and the licenses
// of the other code concerned.
//
// Note that people who make modified versions of this program are not obligated
// to grant this special exception for their modified versions; it is their choice
// whether to do so. The GNU Lesser General Public License gives permission
// to release a modified version without this exception; this exception also makes
// it possible to release a modified version which carries forward this exception.
//

#define R_NO_REMAP
#include "rmsk_namespace.h"
#include "rmsk_msg_mosek.h"
#include "rmsk_utils_interface.h"
#include "rmsk_utils_mosek.h"
#include "rmsk_utils_sexp.h"

#include "rmsk_obj_arguments.h"
#include "rmsk_obj_mosek.h"

#include <string>
#include <exception>

using namespace RMSK_INNER_NS;
using std::string;
using std::exception;


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
		reset_global_ressources();
	}

	// Create handled for returned data
	SEXP_NamedVector ret_val;

	try {
		// Start the program
		reset_global_variables();
		printdebug("Function 'mosek' was called");

		// Make room for: response, sol, dinfo, iinfo
		ret_val.initVEC(4);

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

		// Extract extra information items
		if (probin.options.getinfo) {
			msk_getoptimizationinfo(ret_val, task);
		}

	} catch (msk_exception const& e) {
		terminate_unsuccessfully(ret_val, e);
		return ret_val;

	} catch (exception const& e) {
		terminate_unsuccessfully(ret_val, e.what());
		return ret_val;
	}

	// Clean allocations and exit (msk_solve adds response)
	terminate_successfully(ret_val);
	return ret_val;
}


SEXP mosek_clean()
{
	if (!mosek_interface_termination_success) {

		// If an Rf_error happens before vebosity is set, output everything!
		if isnan(mosek_interface_verbose) {
			mosek_interface_verbose = typeALL;
			printoutput("----- PENDING MESSAGES -----\n", typeERROR);

			try {
				printpendingmsg();
			} catch (exception const& e) { /* Just continue.. */ }
		}

		// Print exclamation in case of an Rf_error in previous run
		printoutput("----- ERROR CAUGHT! CALLING MOSEK_CLEAN -----\n", typeERROR);

	} else {

		// The mosek_clean function use verbose=typeINFO by default (should we read options?)
		reset_global_variables();
		mosek_interface_verbose = typeINFO;

	}

	// Clean global resources and release the MOSEK environment
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
		reset_global_ressources();
	}

	// Create handled for returned data
	SEXP_NamedVector ret_val;

	try {
		// Start the program
		reset_global_variables();
		printdebug("Function 'mosek_read' was called");

		// Make room for: response, prob, dinfo, iinfo
		ret_val.initVEC(4);

		// Validate input arguments
		if (!Rf_isString(arg0)) {
			throw msk_exception("Input argument " + ARGNAMES[0] + " should be a " + ARGTYPES[0] + ".");
		}
		if (!isNamedVector(arg1)) {
			throw msk_exception("Input argument " + ARGNAMES[1] + " should be a " + ARGTYPES[1] + ".");
		}

		string filepath = CHARACTER_ELT(arg0, 0);

		// Define new default values for options
		options_type default_opts; {
			default_opts.useparam = false;
			default_opts.usesol = false;
		}

		// Read input arguments: options (with modified defaults)
		problem_type probin;
		probin.options = default_opts;
		probin.options.R_read(arg1);

		if (filepath.empty() && probin.options.scofile.empty()) {
			throw msk_exception("Input argument " + ARGNAMES[0] + " was empty. No problem description was loaded.");
		}

		// Read the problem if specified
		if ( !filepath.empty() ) {

			// Create task and load filepath-model into MOSEK
			Task_handle &task = global_task;
			msk_loadproblemfile(task, filepath, probin.options);

			// Read the problem from MOSEK
			probin.MOSEK_read(task);

			// Extract extra information items
			if (probin.options.getinfo) {
				msk_getoptimizationinfo(ret_val, task);
			}
		}

		// Read the problem sco-file if specified
		if ( !probin.options.scofile.empty() ) {
			msk_loadproblemscofile(probin);
		}

		// Write the problem to R
		SEXP_NamedVector prob_val;
		probin.R_write(prob_val);
		ret_val.pushback("prob", prob_val);

	} catch (msk_exception const& e) {
		terminate_unsuccessfully(ret_val, e);
		return ret_val;

	} catch (exception const& e) {
		terminate_unsuccessfully(ret_val, e.what());
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
		reset_global_ressources();
	}

	// Create handled for returned data
	SEXP_NamedVector ret_val;

	try {
		// Start the program
		reset_global_variables();
		printdebug("Function 'mosek_write' was called");

		// Make room for: response, dinfo, iinfo
		ret_val.initVEC(3);

		// Validate input arguments
		if (!isNamedVector(arg0)) {
			throw msk_exception("Input argument '" + ARGNAMES[0] + "' should be a '" + ARGTYPES[0] + "'.");
		}
		if (!Rf_isString(arg1) || CHARACTER_ELT(arg1, 0).empty()) {
			throw msk_exception("Input argument '" + ARGNAMES[1] + "' should be a non-empty '" + ARGTYPES[1] + "'.");
		}
		if (!isNamedVector(arg2)) {
			throw msk_exception("Input argument '" + ARGNAMES[2] + "' should be a '" + ARGTYPES[2] + "'.");
		}

		string filepath = CHARACTER_ELT(arg1, 0);

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

		// Ignore the scopt operators when writing problem to MOSEK
		int probin_scopt_numopro = probin.scopt.numopro;	probin.scopt.numopro = 0;
		int probin_scopt_numoprc = probin.scopt.numoprc;	probin.scopt.numoprc = 0;

		// Create task and load problem into MOSEK
		Task_handle &task = global_task;
		probin.MOSEK_write(task);

		if (probin_scopt_numopro || probin_scopt_numoprc) {

			if ( probin.options.scofile.empty() )
				throw msk_exception("Field 'scopt' of input argument '" + ARGNAMES[0] + "' could not be written. The option 'scofile' needs a non-empty definition.");

			// Write the loaded scopt operators to a file
			probin.scopt.numopro = probin_scopt_numopro;
			probin.scopt.numoprc = probin_scopt_numoprc;
			msk_saveproblemscofile(probin);
		}

		// Write the loaded MOSEK problem to a file
		msk_saveproblemfile(task, filepath, probin.options);

		// Extract extra information items
		if (probin.options.getinfo) {
			msk_getoptimizationinfo(ret_val, task);
		}

	} catch (msk_exception const& e) {
		terminate_unsuccessfully(ret_val, e);
		return ret_val;

	} catch (exception const& e) {
		terminate_unsuccessfully(ret_val, e.what());
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
			{"mosek_sym", (DL_FUNC) &mosek, 2},
			{"mosek_clean_sym", (DL_FUNC) &mosek_clean, 0},
			{"mosek_version_sym", (DL_FUNC) &mosek_version, 0},
			{"mosek_write_sym", (DL_FUNC) &mosek_write, 3},
			{"mosek_read_sym" , (DL_FUNC) &mosek_read, 2},
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
