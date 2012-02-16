#ifndef RMSK_UTILS_MOSEK_H_
#define RMSK_UTILS_MOSEK_H_

#include "rmsk_namespace.h"
#include "rmsk_msg_mosek.h"

#include "rmsk_obj_sexp.h"
#include "rmsk_obj_mosek.h"
#include "rmsk_obj_arguments.h"
#include "rmsk_obj_matrices.h"

#include <string>
#include <memory>


___RMSK_INNER_NS_START___


// ------------------------------
// MOSEK-UTILS
// ------------------------------

// Convert objective sense to and from MOSEK and R
std::string get_objective(MSKobjsensee sense);
MSKobjsensee get_mskobjective(std::string sense);

// Enable fast typing without prefixes
void append_mskprefix(std::string &str, std::string prefix);
void remove_mskprefix(std::string &str, std::string prefix);

// Gets and sets the constraint and variable bounds in task
void set_boundkey(double bl, double bu, MSKboundkeye *bk);
void get_boundvalues(MSKtask_t task, double *lower, double* upper, MSKaccmodee boundtype, MSKintt numbounds);

// Gets and sets the parameters in task
void set_parameter(MSKtask_t task, std::string type, std::string name, SEXP value);
void append_parameters(MSKtask_t task, SEXP_LIST iparam, SEXP_LIST dparam, SEXP_LIST sparam);
void get_int_parameters(SEXP_NamedVector &paramvec, MSKtask_t task);
void get_dou_parameters(SEXP_NamedVector &paramvec, MSKtask_t task);
void get_str_parameters(SEXP_NamedVector &paramvec, MSKtask_t task);

// Get and set solutions in task
void msk_getsolution(SEXP_Handle &sol, MSKtask_t task);
void append_initsol(MSKtask_t task, SEXP_LIST initsol, MSKintt NUMCON, MSKintt NUMVAR);

// Initialise the task and load problem from arguments
void msk_loadproblem(Task_handle &task, problem_type &probin);

___RMSK_INNER_NS_END___

#endif /* RMSK_UTILS_MOSEK_H_ */
