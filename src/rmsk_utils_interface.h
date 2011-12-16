#ifndef RMSK_UTILS_INTERFACE_H_
#define RMSK_UTILS_INTERFACE_H_

#include "rmsk_namespace.h"
#include "rmsk_msg_mosek.h"

#include "rmsk_obj_arguments.h"
#include "rmsk_obj_sexp.h"


___RMSK_INNER_NS_START___

// ------------------------------
// Cleaning and termination code
// ------------------------------
void reset_global_ressources();
void reset_global_variables();
void terminate_successfully(SEXP_NamedVector &ret_val);
void terminate_unsuccessfully(SEXP_NamedVector &ret_val, const msk_exception &e);
void terminate_unsuccessfully(SEXP_NamedVector &ret_val, const char* msg);
void msk_addresponse(SEXP_NamedVector &ret_val, const msk_response &res, bool overwrite=true);


// ------------------------------
// Main interface functionality
// ------------------------------

// Solve a loaded problem and return the solution
void msk_solve(SEXP_NamedVector &ret_val, Task_handle &task, options_type &options);

// Load a problem description from file
void msk_loadproblemfile(Task_handle &task, std::string filepath, options_type &options);

// Save a problem description to file
void msk_saveproblemfile(Task_handle &task, std::string filepath, options_type &options);

___RMSK_INNER_NS_END___

#endif /* RMSK_UTILS_INTERFACE_H_ */
