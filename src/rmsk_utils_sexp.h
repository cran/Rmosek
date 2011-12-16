#ifndef RMSK_UTILS_SEXP_H_
#define RMSK_UTILS_SEXP_H_

#include "rmsk_msg_base.h"
#include "rmsk_namespace.h"

#include "rmsk_obj_sexp.h"
#include "rmsk_obj_matrices.h"

#include <memory>

___RMSK_INNER_NS_START___

// ------------------------------
// COMMON SEXP METHODS
// ------------------------------

// Check SEXP object
bool isEmpty(SEXP obj);
bool isNamedVector(SEXP object);

// Convert SEXP list item to other types
bool 		BOOLEAN_ELT(SEXP obj, R_len_t idx);
int			INTEGER_ELT(SEXP obj, R_len_t idx);
double		NUMERIC_ELT(SEXP obj, R_len_t idx);
std::string	CHARACTER_ELT(SEXP obj, R_len_t idx);

// Seek value and index of SEXP in list
void list_seek_Value(SEXP *out, SEXP_LIST list, std::string name, bool optional=false);
void list_seek_Index(int  *out, SEXP_LIST list, std::string name, bool optional=false);

// Seek other types in list
void list_seek_SparseMatrix(std::auto_ptr<matrix_type>  &out, SEXP_LIST list, std::string name, bool optional=false);
void list_seek_NamedVector (SEXP_Handle 				&out, SEXP_LIST list, std::string name, bool optional=false);
void list_seek_Numeric	   (SEXP_Handle 				&out, SEXP_LIST list, std::string name, bool optional=false);
void list_seek_Character   (SEXP_Handle 				&out, SEXP_LIST list, std::string name, bool optional=false);
void list_seek_Scalar	   (double 	    				*out, SEXP_LIST list, std::string name, bool optional=false);
void list_seek_Integer	   (int 						*out, SEXP_LIST list, std::string name, bool optional=false);
void list_seek_String	   (std::string 				*out, SEXP_LIST list, std::string name, bool optional=false);
void list_seek_Boolean	   (bool 	    				*out, SEXP_LIST list, std::string name, bool optional=false);

// Validate other types fetched from list
void validate_SparseMatrix(std::auto_ptr<matrix_type> &object, std::string name, size_t nrows, size_t ncols, bool optional=false);
void validate_NamedVector(SEXP_Handle &object, std::string name, std::vector<std::string> keywords, bool optional=false);
void validate_Numeric(SEXP_Handle &object, std::string name, R_len_t nrows, bool optional=false);
void validate_Character(SEXP_Handle &object, std::string name, R_len_t nrows, bool optional=false);

___RMSK_INNER_NS_END___

#endif /* RMSK_UTILS_SEXP_H_ */

