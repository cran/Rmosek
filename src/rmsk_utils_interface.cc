#define R_NO_REMAP
#include "rmsk_utils_interface.h"

#include "rmsk_utils_mosek.h"
#include "rmsk_utils_sexp.h"

#include "rmsk_obj_matrices.h"
#include "rmsk_obj_mosek.h"

#include "rmsk_globalvars.h"

#include <string>
#include <exception>


___RMSK_INNER_NS_START___
using std::string;
using std::exception;


// ------------------------------
// Cleaning and termination code
// ------------------------------

void initiate_package() {
  // Clean in case of Rf_error in previous run
  if (!mosek_interface_termination_success) {
    reset_global_ressources();
  }

  // Start the function call
  reset_global_execution_flags();
}

void terminate_successfully(SEXP_NamedVector &ret_val) /* nothrow */ {
  try {
    reset_global_ressources();
    mosek_interface_termination_success = true;
    msk_addresponse(ret_val, get_msk_response(MSK_RES_OK), false);

    // Print warning summary
    if (mosek_interface_warnings > 0) {
      printoutput("The R-to-MOSEK interface completed with " + tostring(mosek_interface_warnings) + " warning(s)\n\n", typeWARNING);
    }
  }
  catch (exception const& e) { /* Just terminate.. */ }
}

void terminate_unsuccessfully(SEXP_NamedVector &ret_val, const msk_exception &e) /* nothrow */ {
  try {
    // Force pending and future messages through
    if (ISNAN(mosek_interface_verbose)) {
      mosek_interface_verbose = typeALL;
    }
    printpendingmsg("----- PENDING MESSAGES -----\n");
    printerror( e.what() );

    msk_addresponse(ret_val, e.getresponse(), true);
  }
  catch (exception const& e) { /* Just terminate.. */ }
  terminate_successfully(ret_val);
}

void terminate_unsuccessfully(SEXP_NamedVector &ret_val, const char* msg) /* nothrow */ {
  try {
    terminate_unsuccessfully(ret_val, msk_exception(msk_response(string(msg))));
  }
  catch (exception const& e) { /* Just terminate.. */ }
}

void msk_addresponse(SEXP_NamedVector &ret_val, const msk_response &res, bool overwrite) {

  // Construct: result -> response
  SEXP_NamedVector res_vec;
  res_vec.initVEC(2);

  res_vec.pushback("code", res.code);
  res_vec.pushback("msg", res.msg);

  // Append to result and overwrite if structure already exists
  // In this way errors, which calls for immediate exit, can overwrite the regular termination code..
  int pos = -1;
  list_seek_Index(&pos, ret_val, "response", true);
  if (pos >= 0) {
    if (overwrite) {
      ret_val.set("response", res_vec, pos);
    }
  } else {
    ret_val.pushback("response", res_vec);
  }
}


// ------------------------------
// Main interface functionality
// ------------------------------


// Interrupts MOSEK if CTRL+C is caught in R
static void mskcallback_dummyfun(void *data) { R_CheckUserInterrupt(); }
static int MSKAPI mskcallback(MSKtask_t task, MSKuserhandle_t handle, MSKcallbackcodee caller)
{
  // FIXME: This interruption checking has not been documented and may be subject to change.
  mosek_interface_signal_caught = (FALSE == R_ToplevelExec(mskcallback_dummyfun, NULL));

  if (mosek_interface_signal_caught) {
    printoutput("Interruption caught, terminating at first chance...\n", typeMOSEK);
    return 1;
  }
  return 0;
}

/* Solve a loaded problem and return the solution */
void msk_solve(SEXP_NamedVector &ret_val, Task_handle &task, options_type &options)
{
  //
  // STEP 1 - INITIALIZATION
  //
  printdebug("msk_solve - Started initializing");
  {
    /* Make it interruptible with CTRL+C */
    errcatch( MSK_putcallbackfunc(task, mskcallback, static_cast<MSKuserhandle_t>(NULL)) );

    /* Write file containing problem description (filetypes: .lp, .mps, .opf, .mbt) */
    if (!options.writebefore.empty()) {
      MSK_putintparam(task, MSK_IPAR_OPF_WRITE_SOLUTIONS, MSK_ON);
      errcatch( MSK_writedata(task, const_cast<MSKCONST char*>(options.writebefore.c_str())) );
    }
  }

  //
  // STEP 2 - OPTIMIZATION
  //
  printdebug("msk_solve - Started optimization");
  try
  {
    /* Separate interface warnings from MOSEK output */
    if (mosek_interface_warnings > 0)
      printoutput("\n", typeWARNING);

    /* Run optimizer */
    MSKrescodee trmcode;
    errcatch( MSK_optimizetrm(task, &trmcode) );
    msk_addresponse(ret_val, get_msk_response(trmcode));

  } catch (exception const& e) {
    // Report that the CTRL+C interruption has been handled
    if (mosek_interface_signal_caught) {
      printoutput("Optimization interrupted because of termination signal, e.g. <CTRL> + <C>.\n", typeERROR);
    } else {
      printoutput("Optimization interrupted.\n", typeERROR);
    }

    mosek_interface_signal_caught = false;
    throw;
  }

  //
  // STEP 3 - EXTRACT SOLUTION
  //
  printdebug("msk_solve - Started to extract solution");
  try
  {
    /* Write file containing problem description (solution only included if filetype is .opf or .mbt) */
    if (!options.writeafter.empty()) {
      MSK_putintparam(task, MSK_IPAR_OPF_WRITE_SOLUTIONS, MSK_ON);
      errcatch( MSK_writedata(task, const_cast<MSKCONST char*>(options.writeafter.c_str())) );
    }

    /* Print a summary containing information
     * about the solution and optimizer for debugging purposes. */
    if (mosek_interface_verbose >= typeINFO) {
      errcatch( MSK_solutionsummary(task, MSK_STREAM_LOG) );
      errcatch( MSK_optimizersummary(task, MSK_STREAM_LOG) );
    }

    /* Extract solution from MOSEK to R */
    SEXP_Handle sol_val;
    msk_getsolution(sol_val, task, options);
    ret_val.pushback("sol", sol_val);

  } catch (exception const& e) {
    printoutput("An error occurred while extracting the solution.\n", typeERROR);
    throw;
  }
}

/* Load a problem description from file */
void msk_loadproblemfile(Task_handle &task, std::string filepath, options_type &options) {

  // Make sure the environment is initialized
  global_env.init();

  // Initialize the task
  task.init(global_env, 0, 0);

  try {
    errcatch( MSK_readdata(task, const_cast<MSKCONST char*>(filepath.c_str())) );

  } catch (exception const& e) {
    printerror("An error occurred while loading up the problem from a file");
    throw;
  }
}

void msk_loadproblemscofile(problem_type &probin) {
  probin.scopt.FILE_read(probin.options.scofile);
  probin.numscoprs = probin.scopt.numoprc + probin.scopt.numopro;
}

/* Save a problem description to file */
void msk_saveproblemfile(Task_handle &task, string filepath, options_type &options) {

  // Set export-parameters for whether to write any solution loaded into MOSEK
  if (options.usesol) {
    errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_SOLUTIONS, MSK_ON) );
  } else {
    errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_SOLUTIONS, MSK_OFF) );
  }

  // Set export-parameters for whether to write all parameters
  if (options.useparam) {
    errcatch( MSK_putintparam(task, MSK_IPAR_WRITE_DATA_PARAM,MSK_ON) );
    errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_PARAMETERS,MSK_ON) );
  } else {
    errcatch( MSK_putintparam(task, MSK_IPAR_WRITE_DATA_PARAM,MSK_OFF) );
    errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_PARAMETERS,MSK_OFF) );
  }

  // Write to filepath model (filetypes: .lp, .mps, .opf, .mbt)
  errcatch( MSK_writedata(task, const_cast<MSKCONST char*>(filepath.c_str())) );
}

void msk_saveproblemscofile(problem_type &probin) {

  probin.scopt.FILE_write(probin.options.scofile, probin);

}

___RMSK_INNER_NS_END___
