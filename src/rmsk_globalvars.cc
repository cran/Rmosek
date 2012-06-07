/*
 * The order of initialization of global variable in multiple object files
 * is undefined. As a result, "memory not mapped" errors can occur when
 * constructors/destructors of some global variables refer to other
 * global variables.
 *
 * This file fixes the order of initialization.
 *
 * Global variables are used for:
 * 		- Static classes of constants (e.g. strings)
 * 		- Simple execution flags
 * 		- Resources circumventing the R garbage collector, e.g.:
 * 			- when it is cached between function calls (the MOSEK license)
 * 			- when its type is not supported in R (in global scope, because Rf_error terminates without calling destructors in local scope)
 */

#define R_NO_REMAP
#include "rmsk_globalvars.h"

#include "rmsk_msg_base.h"

#include "rmsk_obj_mosek.h"
#include "rmsk_obj_arguments.h"

#include <math.h>
#include <vector>
#include <string>

___RMSK_INNER_NS_START___
using std::vector;
using std::string;


/*
 * Declarations: rmsk_msg_base.h
 */
double mosek_interface_verbose  = R_NaN;			// Declare messages as pending
int    mosek_interface_warnings = 0;				// Start warning count from zero
bool   mosek_interface_signal_caught = false;		// Indicate whether the CTRL+C interrupt has been registered
bool   mosek_interface_termination_success = true;	// Indicate whether the interfaced terminated properly


/*
 * Declarations: rmsk_msg_base.cc
 */
vector<string>      mosek_pendingmsg_str;
vector<verbosetype> mosek_pendingmsg_type;


/*
 * Declarations: rmsk_obj_mosek.h
 */
Env_handle global_env;
Task_handle global_task;


/*
 * Declarations: rmsk_obj_arguments.h
 */
const options_type::R_ARGS_type options_type::R_ARGS;
const problem_type::R_ARGS_type problem_type::R_ARGS;


/*
 * Declarations: rmsk_obj_constraints.h
 */
const conicSOC_type::ITEMS_type::R_ARGS_type conicSOC_type::ITEMS_type::R_ARGS;
const scoptOPR_type::R_ARGS_type scoptOPR_type::R_ARGS;
const scoptOPR_type::ITEMS_type::OPRO_type::R_ARGS_type scoptOPR_type::ITEMS_type::OPRO_type::R_ARGS;
const scoptOPR_type::ITEMS_type::OPRC_type::R_ARGS_type scoptOPR_type::ITEMS_type::OPRC_type::R_ARGS;


/*
 * Declarations: rmsk_obj_matrices.h
 */
const simple_matrixCOO_type::R_ARGS_type simple_matrixCOO_type::R_ARGS;

CHM_TR pkgMatrixCOO_type::matrix = NULL;
bool pkgMatrixCOO_type::initialized = false;
bool pkgMatrixCOO_type::cholmod_allocated = false;

CHM_SP pkgMatrixCSC_type::matrix = NULL;
bool pkgMatrixCSC_type::initialized = false;
bool pkgMatrixCSC_type::cholmod_allocated = false;

pkgMatrixCOO_type global_pkgMatrix_COO;
pkgMatrixCSC_type global_pkgMatrix_CSC;



/*
 * Global variable management functions
 */

void reset_global_ressources(bool includeCachedResources) {

	global_pkgMatrix_CSC.~pkgMatrixCSC_type();
	global_pkgMatrix_COO.~pkgMatrixCOO_type();
	global_task.~Task_handle();
	delete_all_pendingmsg();

	if (includeCachedResources) {
		// The mosek environment 'global_env' should normally not be cleared, as we wish to
		// reuse the license until mosek_clean() is called or package is unloaded.
		global_env.~Env_handle();
	}

}

void reset_global_execution_flags() {
	// This should be done at the beginning of a new function call

	mosek_interface_verbose  = R_NaN;   			// Declare messages as pending
	mosek_interface_warnings = 0;
	mosek_interface_signal_caught = false;
	mosek_interface_termination_success = false;	// No success before we call 'terminate_successfully' or 'mosek_clean'
}

___RMSK_INNER_NS_END___
