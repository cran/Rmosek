#define R_NO_REMAP
#include "rmsk_utils_sexp.h"

#include "rmsk_numeric_casts.h"

#include <Rembedded.h>
#include <Rdefines.h>

#include <string>
#include <vector>
#include <memory>


___RMSK_INNER_NS_START___
using std::string;
using std::vector;
using std::auto_ptr;

// ------------------------------
// CHECK SEXP
// ------------------------------

bool isEmpty(SEXP obj) {
	if (Rf_isNull(obj))
		return true;

	if (Rf_length(obj) >= 2)
		return false;

	if (Rf_isLogical(obj) || Rf_isNumeric(obj)) {
		double obj_val = Rf_asReal(obj);

		if (ISNA(obj_val) || ISNAN(obj_val))
			return true;
	}

	return false;
}

bool isNamedVector(SEXP object) {
	if (!Rf_isNewList(object))
		return false;

	if (LENGTH(object) >= 1) {
		SEXP_Handle names( Rf_getAttrib(object, R_NamesSymbol) );
		if (LENGTH(names) != LENGTH(object))
			return false;
	}
	return true;
}


// ------------------------------
// CONVERT SEXP
// ------------------------------

double NUMERIC_ELT(SEXP obj, R_len_t idx) {
	if (Rf_isNumeric(obj) && Rf_length(obj) > idx) {
		if (Rf_isReal(obj))
			return REAL(obj)[idx];
		if (Rf_isInteger(obj))
			return static_cast<double>(INTEGER(obj)[idx]);
	}
	throw msk_exception("Internal function NUMERIC_ELT received an unknown request");
}

int INTEGER_ELT(SEXP obj, R_len_t idx) {
	if (Rf_isNumeric(obj) && Rf_length(obj) > idx) {
		if (Rf_isInteger(obj))
			return INTEGER(obj)[idx];
		if (Rf_isReal(obj))
			return scalar2int(REAL(obj)[idx]);
	}
	throw msk_exception("Internal function INTEGER_ELT received an unknown request");
}

string CHARACTER_ELT(SEXP obj, R_len_t idx) {
	if (Rf_isString(obj) && Rf_length(obj) > idx) {
		return string(CHAR(STRING_ELT(obj, numeric_cast<int>(idx))));
	}
	throw msk_exception("Internal function CHARACTER_ELT received an unknown request");
}

bool BOOLEAN_ELT(SEXP obj, R_len_t idx) {
	if (Rf_isLogical(obj) && Rf_length(obj) > idx) {
		return static_cast<bool>(INTEGER(obj)[idx]);
	}
	throw msk_exception("Internal function BOOLEAN_ELT received an unknown request");
}

SEXP RLISTMATRIX_ELT(SEXP obj, R_len_t rowidx, R_len_t colidx) {
	if (Rf_isMatrix(obj) && Rf_nrows(obj) > rowidx && Rf_ncols(obj) > colidx) {
		return VECTOR_ELT(obj, rowidx + colidx * Rf_nrows(obj));
	}
	throw msk_exception("Internal function RLISTMATRIX_ELT received an unknown request");
}

double RNUMERICMATRIX_ELT(SEXP obj, R_len_t rowidx, R_len_t colidx) {
	if (Rf_isMatrix(obj) && Rf_nrows(obj) > rowidx && Rf_ncols(obj) > colidx) {
		return NUMERIC_ELT(obj, rowidx + colidx * Rf_nrows(obj));
	}
	throw msk_exception("Internal function RNUMERICMATRIX_ELT received an unknown request");
}


// ------------------------------
// Seek object: SEXP
// ------------------------------
void list_seek_Value(SEXP *out, SEXP_LIST list, string name, bool optional)
{
	// VECTOR_ELT needs type 'int'
	R_len_t numobjects = numeric_cast<int>(Rf_length(list));

	if (name.empty() && numobjects >= 1) {
		*out = VECTOR_ELT(list, 0);
		return;
	}

	SEXP namelst = Rf_getAttrib(list, R_NamesSymbol);
	if (Rf_length(namelst) != numobjects) {
		throw msk_exception("Mismatching number of elements and names in a named list");;
	}

	for (R_len_t i = 0; i < numobjects; i++) {
		if (name == CHARACTER_ELT(namelst, static_cast<int>(i))) {
			*out = VECTOR_ELT(list, i);
			return;
		}
	}

	if (!optional)
		throw msk_exception("An expected variable named '" + name + "' was not found");
}
void list_seek_Index(int *out, SEXP_LIST list, string name, bool optional)
{
	// output needs type 'int'
	R_len_t numobjects = numeric_cast<int>(Rf_length(list));

	SEXP namelst = Rf_getAttrib(list, R_NamesSymbol);
	if (Rf_length(namelst) != numobjects) {
		throw msk_exception("Mismatching number of elements and names in variable");
	}

	for (R_len_t i = 0; i < numobjects; i++) {
		if (name == CHARACTER_ELT(namelst, i)) {
			*out = static_cast<int>(i);
			return;
		}
	}

	if (!optional)
		throw msk_exception("An expected variable named '" + name + "' was not found");
}


// ------------------------------
// Seek object: SPARSEMATRIX
// ------------------------------
void list_seek_SparseMatrix(auto_ptr<matrix_type> &out, SEXP_LIST list, string name, bool optional)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	list_obtain_SparseMatrix(out, val, name, optional);
}
void list_obtain_SparseMatrix(auto_ptr<matrix_type> &out, SEXP val, string name, bool optional)
{
	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Variable '" + name + "' needs a non-empty definition");
	}

	auto_ptr<matrix_type> tempMatrix;

	if (isNamedVector(val)) {
		// Is this a simple matrix coordinate format??
		tempMatrix.reset( new simple_matrixCOO_type );

	} else {

		int errorOccured = 0;
		SEXP_Handle mosek_interface_pkgNamespace, funcval;

		// Call R-function 'obtain_sparsematrix'
		mosek_interface_pkgNamespace.protect(R_tryEval( Rf_lang2(Rf_install("getNamespace"),Rf_mkString("Rmosek")), R_GlobalEnv, &errorOccured ));
		if (!errorOccured) {
			funcval.protect(R_tryEval( Rf_lang2(Rf_install("obtain_sparsematrix"),val), mosek_interface_pkgNamespace, &errorOccured ));
		}
		if (errorOccured) {
			throw msk_exception("Internal error in list_obtain_SparseMatrix: Failed to call 'obtain_sparsematrix' when processing variable '" + name + "'.");
		}

		// React to output of 'obtain_sparsematrix'
		if (Rf_isNumeric(funcval)) {
			switch (INTEGER_ELT(funcval, 0)) {
			case 0: // Object 'val' was fine
				break;
			case 1: // Object 'val' was bad
				throw msk_exception("Variable '" + name + "' should either be a list or a matrix from the Matrix Package (preferably sparse).");
				break;
			default: // Should never happen..
				throw msk_exception("Internal error in obtain_sparsematrix: Numeric exit code of 'obtain_sparsematrix' not supported.");
				break;
			}
		} else {
			// Object 'val' was converted
			printwarning("Variable '" + name + "' converted to a CSC matrix using as(obj,'CsparseMatrix'). Alternative was a COO matrix using as(obj,'TsparseMatrix'). See function 'mosek_read' in the userguide or R-manual for supported formats.");
			val = funcval;
		}

		if (Matrix_isclass_triplet(val)) {
			// This is a sparse coordinate matrix from package Matrix
			tempMatrix.reset( new pkgMatrixCOO_type );

		} else {
			// Otherwise: Try to read compressed sparse column format with package 'Matrix'
			tempMatrix.reset( new pkgMatrixCSC_type );
		}
	}

	tempMatrix->R_read(val, name);
	out = tempMatrix;
}
void validate_SparseMatrix(auto_ptr<matrix_type> &object, string name, R_len_t nrows, R_len_t ncols, bool optional)
{
	if (optional && object->isempty())
		return;

	if (object->nrow() != nrows || object->ncol() != ncols)
		throw msk_exception("SparseMatrix '" + name + "' has the wrong dimensions");
}


// ------------------------------
// Seek object: RLISTMATRIX
// ------------------------------
void list_seek_RListMatrix(SEXP_Handle &out, SEXP_LIST list, string name, bool optional)
{
	printdebug("Called list_seek_RListMatrix with name: " + name);
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	list_obtain_RListMatrix(out, val, name, optional);
	printdebug("Returned from list_seek_RListMatrix");
}
void list_obtain_RListMatrix(SEXP_Handle &out, SEXP val, string name, bool optional)
{
	printdebug("Called list_obtain_RListMatrix");
	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Variable '" + name + "' needs a non-empty definition");
	}

	// Should be a matrix with list-typed entries
	if (!Rf_isMatrix(val) || !Rf_isNewList(val)) {
		throw msk_exception("Variable '" + name + "' should be of class 'matrix' with typeof() = 'list'");
	}

	out.protect(val);
}
void validate_RMatrix(SEXP_Handle &object, string name, R_len_t nrows, R_len_t ncols, bool optional)
{
	if (optional && isEmpty(object))
		return;

	if (Rf_nrows(object) != nrows || Rf_ncols(object) != ncols)
		throw msk_exception("The 'matrix'-classed object '" + name + "' has the wrong dimensions");
}


// ------------------------------
// Seek object: RNUMERICMATRIX
// ------------------------------
void list_seek_RNumericMatrix(SEXP_Handle &out, SEXP_LIST list, string name, bool optional)
{
	printdebug("Called list_seek_RNumericMatrix with name: " + name);
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	list_obtain_RNumericMatrix(out, val, name, optional);
	printdebug("Returned from list_seek_RNumericMatrix");
}
void list_obtain_RNumericMatrix(SEXP_Handle &out, SEXP val, string name, bool optional)
{
	printdebug("Called list_obtain_RNumericMatrix");
	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Variable '" + name + "' needs a non-empty definition");
	}

	// Should be a matrix with list-typed entries
	if (!Rf_isMatrix(val) || !Rf_isNumeric(val)) {
		throw msk_exception("Variable '" + name + "' should be of class 'matrix' with typeof() = 'list'");
	}

	out.protect(val);
}


// ------------------------------
// Seek object: NamedVector
// ------------------------------
void list_seek_NamedVector(SEXP_Handle &out, SEXP_LIST list, string name, bool optional)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	list_obtain_NamedVector(out, val, name, optional);
}
void list_obtain_NamedVector(SEXP_Handle &out, SEXP val, string name, bool optional)
{
	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Variable '" + name + "' needs a non-empty definition");
	}

	if (!Rf_isNewList(val))
		throw msk_exception("Variable '" + name + "' should be of class List");

	out.protect(val);
}
void validate_NamedVector(SEXP_Handle &object, string objectname, vector<string> keywords, bool optional)
{
	// STRING_ELT needs type 'int'
	R_len_t numobjects = numeric_cast<int>(Rf_length(object));

	if (optional && numobjects==0)
		return;

	SEXP namelst = Rf_getAttrib(object, R_NamesSymbol);
	if (Rf_length(namelst) != numobjects) {
		throw msk_exception("Mismatching number of elements and names in variable named '" + objectname + "'");
	}

	for (R_len_t p = 0; p < numobjects; p++) {
		string key( CHAR(STRING_ELT(namelst, static_cast<int>(p))) );
		bool recognized = false;

		for (vector<string>::size_type j = 0; j < keywords.size() && !recognized; j++)
			if (key == keywords[j])
				recognized = true;

		if (!recognized) {
			if (objectname == "")
				throw msk_exception("Variable '" + key + "' in List not recognized");
			else
				throw msk_exception("Variable '" + key + "' in List '" + objectname + "' not recognized");
		}
	}
}


// ------------------------------
// Seek object: NUMERIC
// ------------------------------
void list_seek_Numeric(SEXP_Handle &out, SEXP_LIST list, string name, bool optional)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	list_obtain_Numeric(out, val, name, optional);
}
void list_obtain_Numeric(SEXP_Handle &out, SEXP val, string name, bool optional)
{
	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Variable '" + name + "' needs a non-empty definition");
	}

	if (!Rf_isNumeric(val))
		throw msk_exception("Variable '" + name + "' should be of class Numeric");

	out.protect(val);
}
void validate_Numeric(SEXP_Handle &object, string name, R_len_t nrows, bool optional)
{
	if (optional && isEmpty(object))
		return;

	if (Rf_length(object) != nrows)
		throw msk_exception("Numeric '" + name + "' has the wrong dimensions");
}


// ------------------------------
// Seek object: CHARACTER
// ------------------------------
void list_seek_Character(SEXP_Handle &out, SEXP_LIST list, string name, bool optional)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	list_obtain_Character(out, val, name, optional);
}
void list_obtain_Character(SEXP_Handle &out, SEXP val, string name, bool optional)
{
	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Variable '" + name + "' needs a non-empty definition");
	}

	if (!Rf_isString(val))
		throw msk_exception("Variable '" + name + "' should be of class Character");

	out.protect(val);
}
void validate_Character(SEXP_Handle &object, string name, R_len_t nrows, bool optional)
{
	if (optional && isEmpty(object))
		return;

	if (Rf_length(object) != nrows)
		throw msk_exception("Character '" + name + "' has the wrong dimensions");
}


// ------------------------------
// Seek object: SCALAR
// ------------------------------
void list_seek_Scalar(double *out, SEXP_LIST list, string name, bool optional)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	list_obtain_Scalar(out, val, name, optional);
}
void list_obtain_Scalar(double *out, SEXP val, string name, bool optional)
{
	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Argument '" + name + "' needs a non-empty definition");
	}

	if (Rf_length(val) == 1) {
		try {
			*out = NUMERIC_ELT(val, 0);
			return;
		}
		catch (msk_exception const& e) {}
	}

	throw msk_exception("Argument '" + name + "' should be a Scalar");
}


// ------------------------------
// Seek object: INTEGER
// ------------------------------
void list_seek_Integer(int *out, SEXP_LIST list, string name, bool optional)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	list_obtain_Integer(out, val, name, optional);
}
void list_obtain_Integer(int *out, SEXP val, string name, bool optional)
{
	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Argument '" + name + "' needs a non-empty definition");
	}

	if (Rf_length(val) == 1) {
		try {
			*out = INTEGER_ELT(val, 0);
			return;
		}
		catch (msk_exception const& e) {}
	}

	throw msk_exception("Argument '" + name + "' should be an Integer");
}


// ------------------------------
// Seek object: STRING
// ------------------------------
void list_seek_String(string *out, SEXP_LIST list, string name, bool optional)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	list_obtain_String(out, val, name, optional);
}
void list_obtain_String(string *out, SEXP val, string name, bool optional)
{
	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Argument '" + name + "' needs a non-empty definition");
	}

	if (Rf_length(val) == 1) {
		try {
			*out = CHARACTER_ELT(val,0);
			return;
		}
		catch (msk_exception const& e) {}
	}

	throw msk_exception("Argument '" + name + "' should be a String");
}


// ------------------------------
// Seek object: BOOLEAN
// ------------------------------
void list_seek_Boolean(bool *out, SEXP_LIST list, string name, bool optional)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	list_obtain_Boolean(out, val, name, optional);
}
void list_obtain_Boolean(bool *out, SEXP val, string name, bool optional)
{
	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Argument '" + name + "' needs a non-empty definition");
	}

	if (Rf_length(val) == 1) {
		try {
			*out = BOOLEAN_ELT(val,0);
			return;
		}
		catch (msk_exception const& e) {}
	}

	throw msk_exception("Argument '" + name + "' should be a Boolean");
}

___RMSK_INNER_NS_END___
