#ifndef RMSK_OBJ_MATRICES_H_
#define RMSK_OBJ_MATRICES_H_

#include "rmsk_msg_base.h"
#include "rmsk_namespace.h"

#include "rmsk_obj_sexp.h"

#include "local_stubs.h"
#include "rmsk_numeric_casts.h"

#include <string>
#include <vector>


___RMSK_INNER_NS_START___

// Forward declarations
#ifndef RMSK_OBJ_MOSEK_H_
  class Task_handle;
#endif

// ------------------------------
// MATRIX TYPES
// ------------------------------


/* Matrix formats:
 *  - pkgMatrix:        An object from package 'Matrix'
 *  - simple:           A package-independent input format using simple types
 *
 * COO: Coordinate matrix (aka triplet)
 * CSC: Compressed sparse column matrix
 * DNS: Dense matrix (not implemented)
 */
enum matrixformat_enum { pkgMatrixCOO, pkgMatrixCSC, simple_matrixCOO };
matrixformat_enum get_matrixformat(std::string name);


class matrix_type {
public:
  // Common data definition
  virtual int nrow() = 0;
  virtual int ncol() = 0;
  virtual R_len_t nnz()  = 0;
  virtual bool isempty() = 0;

  // Common interface functions
  virtual void R_read(SEXP val, std::string name) = 0;
  virtual void R_write(SEXP_Handle &val) = 0;
  virtual void MOSEK_read(Task_handle &task) = 0;
  virtual void MOSEK_write(Task_handle &task) = 0;

  // Virtual destructor propagate calls to derived classes
  virtual ~matrix_type() {}
};


class simple_matrixCOO_type : public matrix_type {
private:
  bool initialized;

  // Data definition (intentionally kept close to R types)
  int _nrow;
  int _ncol;
  R_len_t _nnz;
  SEXP_Handle subi;
  SEXP_Handle subj;
  SEXP_Handle valij;

public:
  // Recognised problem arguments in R
  static const struct R_ARGS_type {
  public:
    std::vector<std::string> arglist;
    const std::string nrow;
    const std::string ncol;
    const std::string subi;
    const std::string subj;
    const std::string valij;
    const std::string dimnames;    // ignored, but enables direct input from package 'slam'

    R_ARGS_type() :
      nrow("nrow"),
      ncol("ncol"),
      subi("i"),
      subj("j"),
      valij("v"),
      dimnames("dimnames")
    {
      std::string temp[] = {nrow, ncol, subi, subj, valij, dimnames};
      arglist = std::vector<std::string>(temp, temp + sizeof(temp)/sizeof(std::string));
    }
  } R_ARGS;

  // Simple construction and destruction
  simple_matrixCOO_type() : initialized(false), _nrow(0), _ncol(0), _nnz(0) {}
  ~simple_matrixCOO_type() {}

  // Matrix properties
  int nrow() { return _nrow; }
  int ncol() { return _ncol; }
  R_len_t  nnz() { return _nnz;  }
  bool isempty() {
    return (!initialized || nnz() == 0 || nrow() == 0 || ncol() == 0);
  }

  // Read and write matrix from and to R
  void R_read(SEXP_LIST object, std::string name);
  void R_write(SEXP_Handle &val);

  // Read and write matrix from and to MOSEK
  void MOSEK_read(Task_handle &task);
  void MOSEK_write(Task_handle &task);
};


extern class pkgMatrixCOO_type : public matrix_type {
private:
  // Currently only one instance is supported
  static bool initialized;
  static bool cholmod_allocated;
  static CHM_TR matrix;

public:
  // Deallocate CHM_TR resource if initialised
  ~pkgMatrixCOO_type();

  // Matrix properties
  int nrow() { return numeric_cast<int>(matrix->nrow); }
  int ncol() { return numeric_cast<int>(matrix->ncol); }
  R_len_t   nnz() { return numeric_cast<R_len_t>(matrix->nnz);  }
  bool isempty() {
    return (!initialized || nnz() == 0 || nrow() == 0 || ncol() == 0);
  }

  // Read and write matrix from and to R
  void R_read(SEXP val, std::string name);
  void R_write(SEXP_Handle &val);

  // Read and write matrix from and to MOSEK
  void MOSEK_read(Task_handle &task);
  void MOSEK_write(Task_handle &task);
} global_pkgMatrix_COO;


extern class pkgMatrixCSC_type : public matrix_type {
private:
  // Currently only one instance is supported
  static bool initialized;
  static bool cholmod_allocated;
  static CHM_SP matrix;

public:
  // Deallocate CHM_SP resource if initialised
  ~pkgMatrixCSC_type();

  // Matrix properties
  int nrow() { return numeric_cast<int>(matrix->nrow);  }
  int ncol() { return numeric_cast<int>(matrix->ncol);  }
  R_len_t   nnz() { return numeric_cast<R_len_t>(matrix->nzmax); }
  bool isempty() {
    return (!initialized || nnz() == 0 || nrow() == 0 || ncol() == 0);
  }

  // Read and write matrix from and to R
  void R_read(SEXP val, std::string name);
  void R_write(SEXP_Handle &val);

  // Read and write matrix from and to MOSEK
  void MOSEK_read(Task_handle &task);
  void MOSEK_write(Task_handle &task);
} global_pkgMatrix_CSC;


___RMSK_INNER_NS_END___

#endif /* RMSK_OBJ_MATRICES_H_ */
