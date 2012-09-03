#ifndef RMSK_OBJ_CONSTRAINTS_H_
#define RMSK_OBJ_CONSTRAINTS_H_

#include "rmsk_msg_base.h"
#include "rmsk_namespace.h"

#include "rmsk_obj_sexp.h"
#include "rmsk_obj_mosek.h"

#include "rmsk_utils_scopt.h"

#include <string>
#include <vector>
#include <memory>

___RMSK_INNER_NS_START___
class problem_type;

// ------------------------------
// Class conicSOC_type
// ------------------------------

class conicSOC_type {
private:
  bool initialized;

public:
  // Recognised second order cone arguments in R
  // TODO: Upgrade to new C++11 initialisers
  struct ITEMS_type {
    static const struct R_ARGS_type {

      std::vector<std::string> arglist;
      const std::string type;
      const std::string sub;

      R_ARGS_type() :
        type("type"),
        sub("sub")
      {
        std::string temp[] = {type, sub};
        arglist = std::vector<std::string>(temp, temp + sizeof(temp)/sizeof(std::string));
      }
    } R_ARGS;
  } ITEMS;


  // Data definition (intentionally kept close to R types)
  MSKintt numcones;
  SEXP_Handle conemat;

  // Simple construction and destruction
  conicSOC_type() : initialized(false), numcones(0) {}
  ~conicSOC_type() {}

  // Read and write cones from and to R
  void R_read(SEXP_LIST object);
  void R_write(SEXP_Handle &val);

  // Read and write cones from and to MOSEK
  void MOSEK_read(Task_handle &task);
  void MOSEK_write(Task_handle &task);
};

// ------------------------------
// Class scoptOPR_type
// ------------------------------

class scoptOPR_type {
private:
  bool initialized;
  bool called_MSK_scbegin;

  // Datastorage used on calls to MOSEK_write
  std::auto_ptr<nlhandt> nlh;
  void init_nlh(problem_type &probin);

  // Overwrite copy constructor (assignment operator) and provide no implementation
  scoptOPR_type(const scoptOPR_type& that);
  scoptOPR_type& operator=(const scoptOPR_type& that);

public:
  // Recognised second order cone arguments in R
  // TODO: Upgrade to new C++11 initialisers
  static const struct R_ARGS_type {

    std::vector<std::string> arglist;
    const std::string opro;
    const std::string oprc;

    R_ARGS_type() :
      opro("opro"),
      oprc("oprc")
    {
      std::string temp[] = {opro, oprc};
      arglist = std::vector<std::string>(temp, temp + sizeof(temp)/sizeof(std::string));
    }
  } R_ARGS;

  struct ITEMS_type {
    struct OPRO_type {
      static const struct R_ARGS_type {

        std::vector<std::string> arglist;
        const std::string type;
        const std::string j;
        const std::string f;
        const std::string g;
        const std::string h;

        R_ARGS_type() :
          type("type"),
          j("j"),
          f("f"),
          g("g"),
          h("h")
        {
          std::string temp[] = {type, j, f, g, h};
          arglist = std::vector<std::string>(temp, temp + sizeof(temp)/sizeof(std::string));
        }
      } R_ARGS;
    } OPRO;

    struct OPRC_type {
      static const struct R_ARGS_type {

        std::vector<std::string> arglist;
        const std::string type;
        const std::string i;
        const std::string j;
        const std::string f;
        const std::string g;
        const std::string h;

        R_ARGS_type() :
          type("type"),
          i("i"),
          j("j"),
          f("f"),
          g("g"),
          h("h")
        {
          std::string temp[] = {type, i, j, f, g, h};
          arglist = std::vector<std::string>(temp, temp + sizeof(temp)/sizeof(std::string));
        }
      } R_ARGS;
    } OPRC;
  } ITEMS;


  // Data definition (intentionally kept close to R types)
  MSKintt numopro;
  MSKintt numoprc;
  SEXP_Handle opromat;
  SEXP_Handle oprcmat;

  // Simple construction and destruction
  scoptOPR_type() : initialized(false), called_MSK_scbegin(false), numopro(0), numoprc(0) { printdebug("scoptOPR START"); }
  ~scoptOPR_type() { printdebug("scoptOPR END"); if (called_MSK_scbegin) { MSK_scend( nlh.get() ); } }

  // Read and write operators from and to R
  void R_read(SEXP_LIST object);
  void R_write(SEXP_Handle &val);

  // Read and write operators from and to FILE
  void FILE_read(std::string &filepath);
  void FILE_write(std::string &filepath, problem_type &probin);

  // Write operators to MOSEK (read is not possible)
  void MOSEK_write(Task_handle &task, problem_type &probin);
};

___RMSK_INNER_NS_END___

#endif /* RMSK_OBJ_CONSTRAINTS_H_ */
