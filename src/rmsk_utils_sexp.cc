#define R_NO_REMAP
#include "rmsk_utils_sexp.h"

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


// ------------------------------
// Seek object: SEXP
// ------------------------------
void list_seek_Value(SEXP *out, SEXP_LIST list, string name, bool optional)
{
	// VECTOR_ELT needs type 'int'
	R_len_t numobjects = numeric_cast<int>(Rf_length(list));

	SEXP namelst = Rf_getAttrib(list, R_NamesSymbol);
	if (Rf_length(namelst) != numobjects) {
		throw msk_exception("Mismatching number of elements and names in variable named '" + name + "'");;
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
		throw msk_exception("Mismatching number of elements and names in variable named '" + name + "'");
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

	} else if (Matrix_isclass_triplet(val)) {
		// This is a sparse coordinate matrix from package Matrix
		tempMatrix.reset( new pkgMatrixCOO_type );

	} else {
		// Otherwise: Try to read with package 'Matrix' and convert to compressed sparse column format
		tempMatrix.reset( new pkgMatrixCSC_type );
	}

	tempMatrix->R_read(val, name);
	out = tempMatrix;
}
void validate_SparseMatrix(auto_ptr<matrix_type> &object, string name, int nrows, int ncols, bool optional)
{
	if (optional && object->isempty())
		return;

	if (object->nrow() != nrows || object->ncol() != ncols)
		throw msk_exception("SparseMatrix '" + name + "' has the wrong dimensions");
}


// ------------------------------
// Seek object: NamedVector
// ------------------------------
void list_seek_NamedVector(SEXP_Handle &out, SEXP_LIST list, string name, bool optional)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

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
void validate_NamedVector(SEXP_Handle &object, string name, vector<string> keywords, bool optional)
{
	// STRING_ELT needs type 'int'
	R_len_t numobjects = numeric_cast<int>(Rf_length(object));

	if (optional && numobjects==0)
		return;

	SEXP namelst = Rf_getAttrib(object, R_NamesSymbol);
	if (Rf_length(namelst) != numobjects) {
		throw msk_exception("Mismatching number of elements and names in variable named '" + name + "'");
	}

	for (R_len_t p = 0; p < numobjects; p++) {
		string key( CHAR(STRING_ELT(namelst, static_cast<int>(p))) );
		bool recognized = false;

		for (vector<string>::size_type j = 0; j < keywords.size() && !recognized; j++)
			if (key == keywords[j])
				recognized = true;

		if (!recognized) {
			if (name == "")
				throw msk_exception("Variable '" + key + "' in List not recognized");
			else
				throw msk_exception("Variable '" + key + "' in List '" + name + "' not recognized");
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
