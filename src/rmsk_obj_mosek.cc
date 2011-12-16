#define R_NO_REMAP
#include "rmsk_obj_mosek.h"

#include <stdexcept>


___RMSK_INNER_NS_START___
using std::exception;


// ------------------------------
// Global variables
// ------------------------------
Env_handle global_env;
Task_handle global_task;


// ------------------------------
// MOSEK message output function
// ------------------------------
static void MSKAPI msk_printoutput(void *handle, char str[]) {
	printoutput(str, typeMOSEK);
}


// ------------------------------
// Class Env_handle
// ------------------------------
void Env_handle::init() {
	if (!initialized) {
		printinfo("Acquiring MOSEK environment");

		try {
			/* Create the mosek environment. */
			errcatch( MSK_makeenv(&env, NULL, NULL, NULL, NULL) );

			try {
				/* Directs the env log stream to the 'msk_printoutput' function. */
				errcatch( MSK_linkfunctoenvstream(env, MSK_STREAM_LOG, NULL, msk_printoutput) );

				try {
					/* Initialize the environment. */
					errcatch( MSK_initenv(env) );

				} catch (exception const& e) {
					MSK_unlinkfuncfromenvstream(env, MSK_STREAM_LOG);
					throw;
				}
			} catch (exception const& e) {
				MSK_deleteenv(&env);
				throw;
			}
		} catch (exception const& e) {
			printerror("Failed to acquire MOSEK environment");
			throw;
		}

		initialized = true;
	}
}

Env_handle::~Env_handle() {
	if (initialized) {
		printinfo("Releasing MOSEK environment");
		MSK_unlinkfuncfromenvstream(env, MSK_STREAM_LOG);
		MSK_deleteenv(&env);
		initialized = false;
	}
}


// ------------------------------
// Class Task_handle
// ------------------------------

void Task_handle::init(MSKenv_t env, MSKintt maxnumcon, MSKintt maxnumvar) {
	if (initialized)
		throw msk_exception("No support for multiple tasks yet!");

	printdebug("Creating an optimization task");

	/* Create the optimization task. */
	errcatch( MSK_maketask(env, maxnumcon, maxnumvar, &task) );

	try {
		/* Directs the log task stream to the 'msk_printoutput' function. */
		errcatch( MSK_linkfunctotaskstream(task, MSK_STREAM_LOG, NULL, msk_printoutput) );

	} catch (exception const& e) {
		MSK_deletetask(&task);
		throw;
	}

	initialized = true;
}

Task_handle::~Task_handle() {
	if (initialized) {
		printdebug("Removing an optimization task");
		MSK_unlinkfuncfromtaskstream(task, MSK_STREAM_LOG);
		MSK_deletetask(&task);
		initialized = false;
	}
}

___RMSK_INNER_NS_END___
