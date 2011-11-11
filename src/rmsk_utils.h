#ifndef RMSK_UTILS_H_
#define RMSK_UTILS_H_

#include "rmsk_msg_system.h"
#include "rmsk_namespace.h"

#include "rmsk_obj_sexp.h"
#include "rmsk_obj_mosek.h"
#include "rmsk_obj_arguments.h"
#include "rmsk_obj_matrices.h"

#include <string>
#include <memory>


___RMSK_INNER_NS_START___

// ------------------------------
// Cleaning and termination code
// ------------------------------
void reset_global_ressources();
void reset_global_variables();
void terminate_successfully();
void terminate_successfully(SEXP_NamedVector &ret_val);
void terminate_unsuccessfully(const char* msg);
void terminate_unsuccessfully(SEXP_NamedVector &ret_val, const msk_exception &e);


// ------------------------------
// MOSEK-UTILS
// ------------------------------

// Convert objective sense to and from MOSEK and R
std::string get_objective(MSKobjsensee sense);
MSKobjsensee get_mskobjective(std::string sense);

// Enable fast typing without prefixes
void append_mskprefix(std::string &str, std::string prefix);
void remove_mskprefix(std::string &str, std::string prefix);

// Checks and sets the constraint and variable bounds in task
void set_boundkey(double bl, double bu, MSKboundkeye *bk);
void get_boundvalues(MSKtask_t task, double *lower, double* upper, MSKaccmodee boundtype, MSKintt numbounds);

// Checks and sets the parameters in task
void get_mskparamtype(MSKtask_t task, std::string type, std::string name, MSKparametertypee *ptype, MSKintt *pidx);
void set_parameter(MSKtask_t task, std::string type, std::string name, SEXP value);
void append_parameters(MSKtask_t task, SEXP_LIST iparam, SEXP_LIST dparam, SEXP_LIST sparam);
void get_int_parameters(SEXP_NamedVector &paramvec, MSKtask_t task);
void get_dou_parameters(SEXP_NamedVector &paramvec, MSKtask_t task);
void get_str_parameters(SEXP_NamedVector &paramvec, MSKtask_t task);

// Checks and read the solution from task
bool isdef_solitem(MSKsoltypee s, MSKsoliteme v);
void getspecs_soltype(MSKsoltypee stype, std::string &name);
void getspecs_solitem(MSKsoliteme vtype, int NUMVAR, int NUMCON, std::string &name, int &size);
void msk_getsolution(SEXP_Handle &sol, MSKtask_t task);
void append_initsol(MSKtask_t task, SEXP_LIST initsol, MSKintt NUMCON, MSKintt NUMVAR);

// Initialise the task and load problem from file
void msk_loadproblemfile(Task_handle &task, std::string filepath, bool readparams);

// Initialise the task and load problem from arguments
void msk_loadproblem(Task_handle &task,
					   MSKobjsensee sense, SEXP_NUMERIC c, double c0,
					   std::auto_ptr<matrix_type> &A,
					   SEXP_NUMERIC blc, SEXP_NUMERIC buc,
					   SEXP_NUMERIC blx, SEXP_NUMERIC bux,
					   conicSOC_type &cones, SEXP_NUMERIC intsub);

// Solve a loaded problem and return the solution
void msk_solve(SEXP_NamedVector &ret_val, Task_handle &task, options_type options);

___RMSK_INNER_NS_END___

#endif /* RMSK_UTILS_H_ */
