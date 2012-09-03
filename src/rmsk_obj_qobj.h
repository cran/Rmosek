#ifndef RMSK_OBJ_QOBJ_H_
#define RMSK_OBJ_QOBJ_H_

#include "rmsk_msg_base.h"
#include "rmsk_namespace.h"

#include "rmsk_obj_sexp.h"
#include "rmsk_obj_mosek.h"

#include <vector>


___RMSK_INNER_NS_START___

class qobj_type {
private:
  bool initialized;

public:
  // Recognised second order cone arguments in R
  // TODO: Upgrade to new C++11 initialisers
  static const struct R_ARGS_type {

    std::vector<std::string> arglist;
    const std::string i;
    const std::string j;
    const std::string v;

    R_ARGS_type() :
      i("i"),
      j("j"),
      v("v")
    {
      std::string temp[] = {i, j, v};
      arglist = std::vector<std::string>(temp, temp + sizeof(temp)/sizeof(std::string));
    }
  } R_ARGS;

  // Data definition (intentionally kept close to R types)
  MSKint64t numqobjnz;
  SEXP_Handle qobj_subi;
  SEXP_Handle qobj_subj;
  SEXP_Handle qobj_val;

  // Simple construction and destruction
  qobj_type() : initialized(false), numqobjnz(0) {}
  ~qobj_type() {}

  // Read and write cones from and to R
  void R_read(SEXP_LIST object);
  void R_write(SEXP_Handle &val);

  // Read and write cones from and to MOSEK
  void MOSEK_read(Task_handle &task);
  void MOSEK_write(Task_handle &task);
};

___RMSK_INNER_NS_END___

#endif /* RMSK_OBJ_QOBJ_H_ */
