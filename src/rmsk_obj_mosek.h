#ifndef RMSK_OBJ_MOSEK_H_
#define RMSK_OBJ_MOSEK_H_

#include "rmsk_msg_mosek.h"
#include "rmsk_namespace.h"


___RMSK_INNER_NS_START___

// ------------------------------
// MOSEK environment and task handles
// ------------------------------

extern class Env_handle {
private:
  bool initialized;
  MSKenv_t env;

  // Overwrite copy constructor (assignment operator) and provide no implementation
  Env_handle(const Env_handle& that);
  Env_handle& operator=(const Env_handle& that);

public:
  // Simple constructor and implicit MSKenv_t conversion
  Env_handle() : initialized(false), env(NULL) { }
  operator MSKenv_t() { return env; }

  // Acquire and release MOSEK environment
  void init();
  ~Env_handle();
} global_env;


extern class Task_handle {
private:
  bool initialized;
  MSKtask_t task;

  // Overwrite copy constructor (assignment operator) and provide no implementation
  Task_handle(const Task_handle& that);
  Task_handle& operator=(const Task_handle& that);

public:
  // Simple constructor and implicit MSKtask_t conversion
  Task_handle() : initialized(false), task(NULL) { }
  operator MSKtask_t() { return task; }

  // Acquire and release MOSEK task
  void init(MSKenv_t env, MSKintt maxnumcon, MSKintt maxnumvar);
  ~Task_handle();
} global_task;


___RMSK_INNER_NS_END___

#endif /* RMSK_OBJ_MOSEK_H_ */
