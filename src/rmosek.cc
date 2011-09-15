// Copyright (C) 2011   MOSEK ApS
// Made by:
//   Henrik Alsing Friberg   <haf@mosek.com>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

#include "Matrix.h"
#include "mosek.h"

#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <stdexcept>

extern "C"
{
	SEXP mosek(SEXP arg0, SEXP arg1);
	SEXP mosek_read(SEXP arg0, SEXP arg1);
	SEXP mosek_write(SEXP arg0, SEXP arg1, SEXP arg2);
	SEXP mosek_clean();
	SEXP mosek_version();
	void R_init_rmosek(DllInfo *info);
	void R_unload_rmosek(DllInfo *info);
}

using namespace std;

// TODO: Replace these typedefs with SEXP_handles. Consider the use of
//       SEXP_LIST_handle, and so forth, to avoid unnecessary type-validations.
typedef SEXP SEXP_LIST;
typedef SEXP SEXP_NUMERIC;
typedef SEXP SEXP_CHARACTER;

// FIXME: Hard coded hack for non-exported functions of the Matrix Package
#define CHOLMOD_DOUBLE 0				// amesos_cholmod_core.h file reference
bool Matrix_isclass_csparse(SEXP x);
bool Matrix_isclass_triplet(SEXP x);
bool Matrix_isclass_dense(SEXP x);
bool Matrix_isclass_factor(SEXP x);

// Global initialization
cholmod_common chol;					    		// Construct cholmod_common struct local to the mosek package
double mosek_interface_verbose  = NAN;				// Declare messages as pending
int    mosek_interface_warnings = 0;				// Start warning count from zero
bool   mosek_interface_signal_caught = false;		// Indicate whether the CTRL+C interrupt has been registered
bool   mosek_interface_termination_success = true;	// Indicate whether the interfaced terminated properly

// ------------------------------
// RESPONSE AND EXCEPTION SYSTEM
// ------------------------------

string msk_responsecode2text(MSKrescodee r) {
	char symname[MSK_MAX_STR_LEN];
	char desc[MSK_MAX_STR_LEN];

	if (MSK_getcodedesc(r, symname, desc) == MSK_RES_OK) {
		return string(symname) + ": " + string(desc) + "";
	} else {
		return "The response code could not be identified";
	}
}

struct msk_response {
public:
	double code;
	string msg;

	msk_response(MSKrescodee r) {
		code = (double)r;
		msg = msk_responsecode2text(r);
	}

	msk_response(const string &msg) :
		code(R_NaN), msg(msg) {}

	msk_response(double code, const string &msg) :
		code(code), msg(msg) {}
};

struct msk_exception : public runtime_error {
public:
	const double code;

	// Used for MOSEK errors with response codes
	msk_exception(const msk_response &res) : runtime_error(res.msg), code(res.code)
	{}

	// Used for interface errors without response codes
	msk_exception(const string &msg) : runtime_error(msg), code(R_NaN)
	{}

	msk_response getresponse() const {
		return msk_response(code, what());
	}
};

// ------------------------------
// PRINTING SYSTEM
// ------------------------------
enum verbosetype { typeERROR=1, typeMOSEK=2, typeWARNING=3, typeINFO=4, typeDEBUG=50, typeALL=100 };
vector<string>      mosek_pendingmsg_str;
vector<verbosetype> mosek_pendingmsg_type;

void printoutput(string str, verbosetype strtype) {
	if (isnan(mosek_interface_verbose)) {
		mosek_pendingmsg_str.push_back(str);
		mosek_pendingmsg_type.push_back(strtype);

	} else if (mosek_interface_verbose >= strtype) {
		if (typeERROR == strtype)
			REprintf(str.c_str());
		else
			Rprintf(str.c_str());
	}
}

void printerror(string str) {
	printoutput("ERROR: " + str + "\n", typeERROR);
}

void printwarning(string str) {
	printoutput("WARNING: " + str + "\n", typeWARNING);
	mosek_interface_warnings++;
}

void printinfo(string str) {
	printoutput(str + "\n", typeINFO);
}

void printdebug(string str) {
	printoutput(str + "\n", typeDEBUG);
}

void printdebugdata(string str) {
	printoutput(str, typeDEBUG);
}

void printpendingmsg() {
	if (mosek_pendingmsg_str.size() != mosek_pendingmsg_type.size())
		throw msk_exception("Error in handling of pending messages");

	for (size_t i=0; i<mosek_pendingmsg_str.size(); i++) {
		printoutput(mosek_pendingmsg_str[i], mosek_pendingmsg_type[i]);
	}

	mosek_pendingmsg_str.clear();
	mosek_pendingmsg_type.clear();
}

// This function translates MOSEK response codes into msk_exceptions.
void errcatch(MSKrescodee r, string str) {
	if (r != MSK_RES_OK) {
		if (!str.empty())
			printerror(str);

		throw msk_exception(r);
	}
}
void errcatch(MSKrescodee r) {
	errcatch(r, "");
}

// ------------------------------
// R-UTILS AND HELPERS
// ------------------------------

template <class T>
string tostring(T val)
{
	ostringstream ss;
	ss << val;
	return ss.str();
}

void strtoupper(string &str) {
	string::iterator i = str.begin();
	string::iterator end = str.end();

	while (i != end) {
	    *i = toupper((int)*i);
	    ++i;
	}
}

/* This function returns the sign of doubles even when incomparable (isinf true). */
bool ispos(double x) {
	return (copysign(1.0, x) > 0);
}

/* This function converts from the normal double type to integers (supports infinity) */
int scalar2int(double scalar) {
	if (isinf(scalar))
		return INT_MAX;

	int retval = (int)scalar;
	double err = scalar - retval;

	if (err <= -1e-6 || 1e-6 <= err)
		printwarning("A scalar with fractional value was truncated to an integer.");

	return retval;
}

bool isEmpty(SEXP obj) {
	if (isNull(obj))
		return true;

//	if (length(obj) == 0)
//		return true;

	if (isLogical(obj) || isNumeric(obj)) {
		double obj_val = asReal(obj);

		if (ISNA(obj_val) || ISNAN(obj_val))
			return true;
	}

	return false;
}

double NUMERIC_ELT(SEXP obj, R_len_t idx) {
	if (isNumeric(obj) && length(obj) > idx) {
		if (isReal(obj))
			return REAL(obj)[idx];
		if (isInteger(obj))
			return (double)(INTEGER(obj)[idx]);
	}
	throw msk_exception("Internal function NUMERIC_ELT received an unknown request");
}

int INTEGER_ELT(SEXP obj, R_len_t idx) {
	if (isNumeric(obj) && length(obj) > idx) {
		if (isInteger(obj))
			return INTEGER(obj)[idx];
		if (isReal(obj))
			return scalar2int(REAL(obj)[idx]);
	}
	throw msk_exception("Internal function INTEGER_ELT received an unknown request");
}

string CHARACTER_ELT(SEXP obj, R_len_t idx) {
	if (isString(obj) && length(obj) > idx) {
		return string(CHAR(STRING_ELT(obj,idx)));
	}
	throw msk_exception("Internal function CHARACTER_ELT received an unknown request");
}

bool BOOLEAN_ELT(SEXP obj, R_len_t idx) {
	if (isLogical(obj) && length(obj) > idx) {
		return (bool)(INTEGER(obj)[idx]);
	}
	throw msk_exception("Internal function BOOLEAN_ELT received an unknown request");
}

/* This function prints the output log from MOSEK to the terminal. */
static void MSKAPI mskprintstr(void *handle, char str[]) {
	printoutput(str, typeMOSEK);
}

// ------------------------------
// HANDLES FOR PROTECTED RESOURCES
// ------------------------------
class SEXP_handle {
private:
	bool initialized;

	PROTECT_INDEX object_ipx;
	SEXP object;

	// Overwrite copy constructor and provide no implementation
	SEXP_handle(const SEXP_handle& that);

public:
	operator SEXP() { return object; }

	void protect(SEXP object) {
		REPROTECT(this->object = object, object_ipx);
	}

	SEXP_handle() {
		object = R_NilValue; initialized = true;
		PROTECT_WITH_INDEX(object, &object_ipx);
	}

	SEXP_handle(SEXP input) {
		object = R_NilValue; initialized = true;
		PROTECT_WITH_INDEX(object, &object_ipx);
		protect(input);
	}

	~SEXP_handle() {
		if (initialized) {
			// This destructor just pops the top elements from the protection-stack! This assumes
			// that all objects in the code are UNPROCTED in the bottom of the scope they were PROTECTED in.
			// This works only
			//	- Because C++ has a stack based construction/deconstruction.
			//  - Because R enables the creation of a PROTECTION INDEX already at the construction phase.
			UNPROTECT(1);
			initialized = false;
		}
	}
};

class SEXP_NamedVector {
private:
	bool initialized;

	SEXP_handle items;
	SEXP_handle names;
	R_len_t maxsize;

	// Overwrite copy constructor and provide no implementation
	SEXP_NamedVector(const SEXP_NamedVector& that);

public:
	SEXP_NamedVector() 	{ initialized = false; }
	operator SEXP() 	{ return items; }

	void initVEC(R_len_t maxsize) {
		if (initialized)
			throw msk_exception("An internal SEXP_Vector was attempted initialized twice!");

		this->maxsize = maxsize;

		items.protect( allocVector(VECSXP, maxsize) );
		SETLENGTH(items, 0);

		names.protect( allocVector(STRSXP, maxsize) );
		SETLENGTH(names, 0);

		setAttrib(items, R_NamesSymbol, names);

		initialized = true;
	}

	R_len_t size() {
		return LENGTH(items);
	}

	void pushback(string name, SEXP item) {
		if (!initialized)
			throw msk_exception("A SEXP_Vector was not initialized in pushback call");

		int pos = size();
		if (pos >= maxsize)
			throw msk_exception("Internal SEXP_Vector did not have enough capacity");

		SET_VECTOR_ELT(items, pos, item);
		SETLENGTH(items, pos+1);

		SET_STRING_ELT(names, pos, mkChar(name.c_str()));
		SETLENGTH(names, pos+1);
	}

	void pushback(string name, char* item) {
		pushback(name, mkString(item));
	}

	void pushback(string name, string item) {
		pushback(name, mkString(item.c_str()));
	}

	void pushback(string name, double item) {
		pushback(name, ScalarReal(item));
	}

	void pushback(string name, int item) {
		pushback(name, ScalarInteger(item));
	}

	void set(string name, SEXP item, int pos) {
		if (!initialized)
			throw msk_exception("A SEXP_Vector was not initialized in set call");

		if (!(0 <= pos && pos < size()))
			throw msk_exception("Internal SEXP_Vector failed to place an element");

		SET_VECTOR_ELT(items, pos, item);
		SET_STRING_ELT(names, pos, mkChar(name.c_str()));
	}
};

bool isNamedVector(SEXP object) {
	if (!isNewList(object))
		return false;

	if (LENGTH(object) >= 1) {
		SEXP_handle names( getAttrib(object, R_NamesSymbol) );
		if (LENGTH(names) != LENGTH(object))
			return false;
	}
	return true;
}

class SEXP_Vector {
private:
	bool initialized;
	bool static_size;

	SEXPTYPE itemstype;
	SEXP_handle items;
	R_len_t maxsize;

	// Overwrite copy constructor and provide no implementation
	SEXP_Vector(const SEXP_Vector& that);

public:
	SEXP_Vector() 	{ initialized = false; }
	operator SEXP() { return items; }

	void initSTR(R_len_t maxsize, bool static_size=true) {
		if (initialized)
			throw msk_exception("An internal SEXP_Vector was attempted initialized twice!");

		this->static_size = static_size;
		this->maxsize = maxsize;
		itemstype = STRSXP;

		items.protect( allocVector(itemstype, maxsize) );

		if (!static_size) {
			SETLENGTH(items, 0);
		}

		initialized = true;
	}

	void initREAL(R_len_t maxsize, bool static_size=true) {
		if (initialized)
			throw msk_exception("An internal SEXP_Vector was attempted initialized twice!");

		this->static_size = static_size;
		this->maxsize = maxsize;
		itemstype = REALSXP;

		items.protect( allocVector(itemstype, maxsize) );

		if (!static_size) {
			SETLENGTH(items, 0);
		}

		initialized = true;
	}

	void initINT(R_len_t maxsize, bool static_size=true) {
		if (initialized)
			throw msk_exception("An internal SEXP_Vector was attempted initialized twice!");

		this->static_size = static_size;
		this->maxsize = maxsize;
		itemstype = INTSXP;

		items.protect( allocVector(itemstype, maxsize) );

		if (!static_size) {
			SETLENGTH(items, 0);
		}

		initialized = true;
	}

	R_len_t size() {
		if (static_size)
			return maxsize;
		else
			return LENGTH(items);
	}

	void pushback(SEXP item) {
		if (!initialized)
			throw msk_exception("A SEXP_Vector was not initialized in pushback call");

		if (static_size)
			throw msk_exception("A static sized SEXP_Vector recieved a dynamic pushback call");

		int pos = size();
		if (pos >= maxsize)
			throw msk_exception("Internal SEXP_Vector did not have enough capacity");

		if (itemstype == STRSXP)
			SET_STRING_ELT(items, pos, item);
		else
			SET_VECTOR_ELT(items, pos, item);

		if (!static_size) {
			SETLENGTH(items, pos+1);
		}
	}

	void pushback(char* item) {
		if (itemstype != STRSXP)
			throw msk_exception("An internal SEXP_Vector experienced a pushback of the wrong type STRSXP!");

		pushback(mkChar(item));
	}

	void pushback(string item) {
		if (itemstype != STRSXP)
			throw msk_exception("An internal SEXP_Vector experienced a pushback of the wrong type STRSXP!");

		pushback(mkChar(item.c_str()));
	}

	void pushback(double item) {
		if (itemstype != REALSXP)
			throw msk_exception("An internal SEXP_Vector experienced a pushback of the wrong type REALSXP!");

		pushback(ScalarReal(item));
	}

	void pushback(int item) {
		if (itemstype != INTSXP)
			throw msk_exception("An internal SEXP_Vector experienced a pushback of the wrong type INTSXP!");

		pushback(ScalarInteger(item));
	}

	void set(SEXP item, int pos) {
		if (!initialized)
			throw msk_exception("A SEXP_Vector was not initialized in set call");

		if (!(0 <= pos && pos < size()))
			throw msk_exception("Internal SEXP_Vector failed to place an element");

		SET_VECTOR_ELT(items, pos, item);
	}
};

/* This class handles the MOSEK environment resource */
class Env_handle {
private:
	bool initialized;

	// Overwrite copy constructor and provide no implementation
	Env_handle(const Env_handle& that);

public:
	MSKenv_t env;
	Env_handle() 		{ initialized = false; }
	operator MSKenv_t() { return env; }

	void init() {
		if (!initialized) {
			printinfo("Acquiring MOSEK environment");

			try {
				/* Create the mosek environment. */
				errcatch( MSK_makeenv(&env, NULL, NULL, NULL, NULL) );

				try {
					/* Directs the env log stream to the 'mskprintstr' function. */
					errcatch( MSK_linkfunctoenvstream(env, MSK_STREAM_LOG, NULL, mskprintstr) );

					try {
						/* Initialize the environment. */
						errcatch( MSK_initenv(env) );

					} catch (exception const& e) {
						MSK_unlinkfuncfromenvstream(env, MSK_STREAM_LOG);
						throw;
					}
				} catch (exception const& e) {
					MSK_deleteenv(&env);
					throw;
				}
			} catch (exception const& e) {
				printerror("Failed to acquire MOSEK environment");
				throw;
			}

			initialized = true;
		}
	}

	~Env_handle() {
		if (initialized) {
			printinfo("Releasing MOSEK environment");
			MSK_unlinkfuncfromenvstream(env, MSK_STREAM_LOG);
			MSK_deleteenv(&env);
			initialized = false;
		}
	}
} global_env;

/* This class handles the MOSEK task resource */
class Task_handle {
private:
	MSKtask_t task;
	bool initialized;

	// Overwrite copy constructor and provide no implementation
	Task_handle(const Task_handle& that);

public:
	Task_handle() 		 { initialized = false; }
	operator MSKtask_t() { return task; }

	void init(MSKenv_t env, MSKintt maxnumcon, MSKintt maxnumvar) {
		if (initialized)
			throw msk_exception("No support for multiple tasks yet!");

		printdebug("Creating an optimization task");

		/* Create the optimization task. */
		errcatch( MSK_maketask(env, maxnumcon, maxnumvar, &task) );

		try {
			/* Directs the log task stream to the 'mskprintstr' function. */
			errcatch( MSK_linkfunctotaskstream(task, MSK_STREAM_LOG, NULL, mskprintstr) );

		} catch (exception const& e) {
			MSK_deletetask(&task);
			throw;
		}

		initialized = true;
	}

	~Task_handle() {
		if (initialized) {
			printdebug("Removing an optimization task");
			MSK_unlinkfuncfromtaskstream(task, MSK_STREAM_LOG);
			MSK_deletetask(&task);
			initialized = false;
		}
	}
} global_task;

/* This class handles the Csparse Matrix resource from the Matrix Package */
class CHM_SP_handle {
private:
	bool initialized;

	CHM_SP cpointer;
	bool cholmod_allocated;

	// Overwrite copy constructor and provide no implementation
	CHM_SP_handle(const CHM_SP_handle& that);

public:
	CHM_SP_handle() 	 { initialized = false; }
	operator CHM_SP() 	 { return cpointer; }
	CHM_SP operator ->() { return cpointer; }

	void init_placeholder() {
		if (initialized)
			throw msk_exception("No support for multiple sparse matrices yet!");

		cpointer = new cholmod_sparse;
		cholmod_allocated = false;
		initialized = true;
	}

	void init(CHM_SP csparse) {
		if (initialized)
			throw msk_exception("No support for multiple sparse matrices yet!");

		printdebug("Initializing CHM_SP_handle");
		cpointer = csparse;
		cholmod_allocated = true;
		initialized = true;
	}

	~CHM_SP_handle() {
		if (initialized) {
			printdebug("Removing a sparse matrix");

			if (cholmod_allocated) {
				M_cholmod_free_sparse(&cpointer, &chol);
			} else {
				delete cpointer;
			}
			initialized = false;
		}
	}
} global_SparseMatrix;

// ------------------------------
// R-UTILS AND HELPERS
// ------------------------------

//
// Handling of: SEXP
//
void list_seek_Value(SEXP *out, SEXP_LIST list, string name, bool optional=false)
{
	SEXP namelst = getAttrib(list, R_NamesSymbol);
	for (int i = 0; i < length(list); i++) {
		if (name == CHARACTER_ELT(namelst, i)) {
			*out = VECTOR_ELT(list, i);
			return;
		}
	}

	if (!optional)
		throw msk_exception("An expected variable named '" + name + "' was not found");
}
void list_seek_Index(int *out, SEXP_LIST list, string name, bool optional=false)
{
	SEXP namelst = getAttrib(list, R_NamesSymbol);
	for (int i = 0; i < length(list); i++) {
		if (name == CHARACTER_ELT(namelst, i)) {
			*out = i;
			return;
		}
	}

	if (!optional)
		throw msk_exception("An expected variable named '" + name + "' was not found");
}

//
// Handling of: NamedVector
//
void list_seek_NamedVector(SEXP_handle &out, SEXP_LIST list, string name, bool optional=false)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Variable '" + name + "' needs a non-empty definition");
	}

	if (!isNewList(val))
		throw msk_exception("Variable '" + name + "' should be of class List");

	out.protect(val);
}
void validate_NamedVector(SEXP_handle &object, string name, vector<string> keywords, bool optional=false)
{
	if (optional && length(object)==0)
		return;

	SEXP namelst = getAttrib(object, R_NamesSymbol);
	for (int p = 0; p < length(namelst); p++) {
		string key( CHAR(STRING_ELT(namelst, p)) );
		bool recognized = false;

		for (size_t j = 0; j < keywords.size() && !recognized; j++)
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

//
// Handling of: NUMERIC
//
void list_seek_Numeric(SEXP_handle &out, SEXP_LIST list, string name, bool optional=false)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Variable '" + name + "' needs a non-empty definition");
	}

	if (!isNumeric(val))
		throw msk_exception("Variable '" + name + "' should be of class Numeric");

	out.protect(val);
}
void validate_Numeric(SEXP_handle &object, string name, size_t nrows, bool optional=false)
{
	if (optional && isEmpty(object))
		return;

	if ((unsigned)length(object) != nrows)
		throw msk_exception("Numeric '" + name + "' has the wrong dimensions");
}

//
// Handling of: CHARACTER
//
void list_seek_Character(SEXP_handle &out, SEXP_LIST list, string name, bool optional=false)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Variable '" + name + "' needs a non-empty definition");
	}

	if (!isString(val))
		throw msk_exception("Variable '" + name + "' should be of class Character");

	out.protect(val);
}
void validate_Character(SEXP_handle &object, string name, size_t nrows, bool optional=false)
{
	if (optional && isEmpty(object))
		return;

	if ((unsigned)length(object) != nrows)
		throw msk_exception("Character '" + name + "' has the wrong dimensions");
}

//
// Handling of: SPARSEMATRIX
//
void list_seek_SparseMatrix(CHM_SP *out, SEXP_LIST list, string name, bool optional=false)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Variable '" + name + "' needs a non-empty definition");
	}

	if (Matrix_isclass_csparse(val)) {
		global_SparseMatrix.init_placeholder();
		M_as_cholmod_sparse(global_SparseMatrix, val, (Rboolean)FALSE, (Rboolean)FALSE);
	}
	else if (Matrix_isclass_triplet(val)) {
		printwarning("Converting triplet matrix to Csparse");
		cholmod_triplet mat;
		M_as_cholmod_triplet(&mat, val, (Rboolean)FALSE);
		//CHM_TR mat = AS_CHM_TR__(val);
		global_SparseMatrix.init(M_cholmod_triplet_to_sparse(&mat, mat.nzmax, &chol));
	}
	else if (Matrix_isclass_dense(val)) {
		printwarning("Converting dense matrix to Csparse");
		cholmod_dense mat;
		M_as_cholmod_dense(&mat, val);
		//CHM_DN mat = AS_CHM_DN(val);
		global_SparseMatrix.init(M_cholmod_dense_to_sparse(&mat, mat.nzmax, &chol));
	}
	else if (Matrix_isclass_factor(val)) {
		printwarning("Converting factored matrix to Csparse");
		cholmod_factor mat;
		M_as_cholmod_factor(&mat, val);
		//CHM_FR mat = AS_CHM_FR(val);
		global_SparseMatrix.init(M_cholmod_factor_to_sparse(&mat, &chol));
	} else {
		throw msk_exception("Variable '" + name + "' should be a Matrix from the Matrix Package (preferably Csparse)");
	}

	*out = global_SparseMatrix;
}
void validate_SparseMatrix(CHM_SP &object, string name, size_t nrows, size_t ncols, bool optional=false)
{
	if (optional && !object)
		return;

	if (object->nrow != nrows || object->ncol != ncols)
		throw msk_exception("SparseMatrix '" + name + "' has the wrong dimensions");
}

//
// Handling of: SCALAR
//
void list_seek_Scalar(double *out, SEXP list, string name, bool optional=false)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Argument '" + name + "' needs a non-empty definition");
	}

	if (length(val) == 1) {
		try {
			*out = NUMERIC_ELT(val, 0);
			return;
		}
		catch (msk_exception const& e) {}
	}

	throw msk_exception("Argument '" + name + "' should be a Scalar");
}

//
// Handling of: STRING
//
void list_seek_String(string *out, SEXP_LIST list, string name, bool optional=false)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Argument '" + name + "' needs a non-empty definition");
	}

	if (length(val) == 1) {
		try {
			*out = CHARACTER_ELT(val,0);
			return;
		}
		catch (msk_exception const& e) {}
	}

	throw msk_exception("Argument '" + name + "' should be a String");
}

//
// Handling of: BOOLEAN
//
void list_seek_Boolean(bool *out, SEXP_LIST list, string name, bool optional=false)
{
	SEXP val = R_NilValue;
	list_seek_Value(&val, list, name, optional);

	if (isEmpty(val)) {
		if (optional)
			return;
		else
			throw msk_exception("Argument '" + name + "' needs a non-empty definition");
	}

	if (length(val) == 1) {
		try {
			*out = BOOLEAN_ELT(val,0);
			return;
		}
		catch (msk_exception const& e) {}
	}

	throw msk_exception("Argument '" + name + "' should be a Boolean");
}

// ------------------------------
// MOSEK-UTILS
// ------------------------------

/* This function explains the solution types of MOSEK. */
string getspecs_sense(MSKobjsensee sense)
{
	switch (sense) {
		case MSK_OBJECTIVE_SENSE_MAXIMIZE:
			return "maximize";
		case MSK_OBJECTIVE_SENSE_MINIMIZE:
			return "minimize";
		case MSK_OBJECTIVE_SENSE_UNDEFINED:
			return "UNDEFINED";
		default:
			throw msk_exception("A problem sense was not supported");
	}
}

/* This function interprets string "sense"  */
MSKobjsensee get_objective(string sense)
{
	if (sense == "min" || sense == "minimize") {
		return MSK_OBJECTIVE_SENSE_MINIMIZE;

	} else if (sense == "max" || sense == "maximize") {
		return MSK_OBJECTIVE_SENSE_MAXIMIZE;

	} else {
		throw msk_exception("Variable 'sense' must be either 'min', 'minimize', 'max' or 'maximize'");
	}
}

/* Enable fast typing without prefixes */
void append_mskprefix(string &str, string prefix)
{
	size_t inclusion = 0, j = 0;
	for (size_t i = 0; i < prefix.length(); i++) {
		if (prefix[i] == str[j]) {
			if (++j >= str.length()) {
				inclusion = prefix.length();
				break;
			}
		} else {
			j = 0;
			inclusion = i+1;
		}
	}
	str = prefix.substr(0, inclusion) + str;
}

/* Enable fast typing without prefixes */
void remove_mskprefix(string &str, string prefix)
{
	if (str.size() > prefix.size()) {
		if (str.substr(0, prefix.size()) == prefix) {
			str = str.substr(prefix.size(), str.size() - prefix.size());
		}
	}
}

/* This function appends a conic constraint to the MOSEK task */
void append_cone(MSKtask_t task, SEXP conevalue, int idx)
{
	// Get input list
	if(!isNamedVector(conevalue)) {
		throw msk_exception("The cone at index " + tostring(idx+1) + " should be a 'named list'");
	}
	SEXP_handle cone(conevalue);

	const struct R_CONES_type {
		vector<string> arglist;
		const string type;
		const string sub;

		R_CONES_type() :
			type("type"),
			sub("sub")
		{
			string temp[] = {type, sub};
			arglist = vector<string>(temp, temp + sizeof(temp)/sizeof(string));
		}
	} R_CONES = R_CONES_type();

	string type;		list_seek_String(&type, cone, R_CONES.type);
	SEXP_handle sub;	list_seek_Numeric(sub, cone, R_CONES.sub);
	validate_NamedVector(cone, "cones[[" + tostring(idx+1) + "]]", R_CONES.arglist);

	// Convert type to mosek input
	strtoupper(type);
	append_mskprefix(type, "MSK_CT_");
	char msktypestr[MSK_MAX_STR_LEN];
	if(!MSK_symnamtovalue(const_cast<MSKCONST char*>(type.c_str()), msktypestr))
		throw msk_exception("The type of cone at index " + tostring(idx+1) + " was not recognized");

	MSKconetypee msktype = (MSKconetypee)atoi(msktypestr);

	// Convert sub type and indexing (Minus one because MOSEK indexes counts from 0, not from 1 as R)
	int msksub[length(sub)];
	for (int i=0; i<length(sub); i++)
		msksub[i] = INTEGER_ELT(sub,i) - 1;

	// Append the cone
	errcatch( MSK_appendcone(task,
			msktype,		/* The type of cone */
			0.0, 			/* For future use only, can be set to 0.0 */
			length(sub),	/* Number of variables */
			msksub) );		/* Variable indexes */
}

void append_initsol(MSKtask_t task, SEXP_LIST initsol, int NUMCON, int NUMVAR)
{
	SEXP namelst = getAttrib(initsol, R_NamesSymbol);
	for (int idx = 0; idx < length(initsol); idx++) {
		SEXP val = VECTOR_ELT(initsol, idx);
		string name = CHAR(STRING_ELT(namelst, idx));
		printdebug("Reading the initial solution '" + name + "'");

		// Get current solution type
		MSKsoltypee cursoltype;
		if (name == "bas") {
			cursoltype = MSK_SOL_BAS;
		} else if (name == "itr") {
			cursoltype = MSK_SOL_ITR;
		} else if (name == "int") {
			cursoltype = MSK_SOL_ITG;
		} else {
			printwarning("The initial solution '" + name + "' was not recognized.");
			continue;
		}

		// Get current solution
		if (isEmpty(val)) {
			printwarning("The initial solution '" + name + "' was ignored.");
			continue;
		}
		if(!isNamedVector(val)) {
			throw msk_exception("The cone at index " + tostring(idx+1) + " should be a 'named list'");
		}
		SEXP_LIST cursol = val;

		// Get current solution items
		SEXP_handle skc; list_seek_Character(skc, cursol, "skc", true);	validate_Character(skc, "skc", NUMCON, true);
		SEXP_handle xc;  list_seek_Numeric(xc, cursol, "xc", true);		validate_Numeric(xc, "xc", NUMCON, true);
		SEXP_handle slc; list_seek_Numeric(slc, cursol, "slc", true);	validate_Numeric(slc, "slc", NUMCON, true);
		SEXP_handle suc; list_seek_Numeric(suc, cursol, "suc", true);	validate_Numeric(suc, "suc", NUMCON, true);

		SEXP_handle skx; list_seek_Character(skx, cursol, "skx", true);	validate_Character(skx, "skx", NUMVAR, true);
		SEXP_handle xx;  list_seek_Numeric(xx, cursol, "xx", true);		validate_Numeric(xx, "xx", NUMVAR, true);
		SEXP_handle slx; list_seek_Numeric(slx, cursol, "slx", true);	validate_Numeric(slx, "slx", NUMVAR, true);
		SEXP_handle sux; list_seek_Numeric(sux, cursol, "sux", true);	validate_Numeric(sux, "sux", NUMVAR, true);
		SEXP_handle snx; list_seek_Numeric(snx, cursol, "snx", true);	validate_Numeric(snx, "snx", NUMVAR, true);

		bool anyinfocon = !isEmpty(skc) || !isEmpty(xc) || !isEmpty(slc) || !isEmpty(suc);
		bool anyinfovar = !isEmpty(skx) || !isEmpty(xx) || !isEmpty(slx) || !isEmpty(sux) || !isEmpty(snx);

		// Set all constraints
		if (anyinfocon) {
			for (int ci = 0; ci < NUMCON; ci++)
			{
				MSKstakeye curskc;
				if (isEmpty(skc) || CHARACTER_ELT(skc,ci).empty()) {
					curskc = MSK_SK_UNK;
				} else {
					MSKintt skcval;
					errcatch( MSK_strtosk(task, const_cast<MSKCONST char*>(CHARACTER_ELT(skc,ci).c_str()), &skcval) );
					curskc = (MSKstakeye)skcval;
				}

				MSKrealt curxc;
				if (isEmpty(xc)) {
					curxc = 0.0;
				} else {
					curxc = NUMERIC_ELT(xc, ci);
				}

				MSKrealt curslc;
				if (isEmpty(slc)) {
					curslc = 0.0;
				} else {
					curslc = NUMERIC_ELT(slc, ci);
				}

				MSKrealt cursuc;
				if (isEmpty(suc)) {
					cursuc = 0.0;
				} else {
					cursuc = NUMERIC_ELT(suc, ci);
				}

				errcatch( MSK_putsolutioni(task,
						MSK_ACC_CON, ci, cursoltype,
						curskc, curxc, curslc, cursuc, 0.0));
			}
		}

		// Set all variables
		if (anyinfovar) {
			for (int xi = 0; xi < NUMVAR; xi++)
			{
				MSKstakeye curskx;
				if (isEmpty(skx) || CHARACTER_ELT(skx,xi).empty()) {
					curskx = MSK_SK_UNK;
				} else {
					MSKintt skxval;
					errcatch( MSK_strtosk(task, const_cast<MSKCONST char*>(CHARACTER_ELT(skx,xi).c_str()), &skxval) );
					curskx = (MSKstakeye)skxval;
				}

				MSKrealt curxx;
				if (isEmpty(xx)) {

					// Variable not fully defined => use bound information
					MSKboundkeye bk;
					MSKrealt bl,bu;
					errcatch( MSK_getbound(task,
							MSK_ACC_VAR, xi, &bk, &bl, &bu));

					switch (bk) {
						case MSK_BK_FX:
						case MSK_BK_LO:
						case MSK_BK_RA:
							curxx = bl;
							break;
						case MSK_BK_UP:
							curxx = bu;
							break;
						case MSK_BK_FR:
							curxx = 0.0;
							break;
						default:
							throw msk_exception("Unexpected boundkey when loading initial solution");
					}

				} else {
					curxx = NUMERIC_ELT(xx, xi);
				}

				MSKrealt curslx;
				if (isEmpty(slx)) {
					curslx = 0.0;
				} else {
					curslx = NUMERIC_ELT(slx, xi);
				}

				MSKrealt cursux;
				if (isEmpty(sux)) {
					cursux = 0.0;
				} else {
					cursux = NUMERIC_ELT(sux, xi);
				}

				MSKrealt cursnx;
				if (isEmpty(snx)) {
					cursnx = 0.0;
				} else {
					cursnx = NUMERIC_ELT(snx, xi);
				}

				errcatch( MSK_putsolutioni(
						task, MSK_ACC_VAR, xi, cursoltype,
						curskx, curxx, curslx, cursux, cursnx));
			}
		}

		if (!anyinfocon && !anyinfovar) {
			printwarning("The initial solution '" + name + "' was ignored.");
		}
	}
}

/* This function checks and sets the bounds of constraints and variables. */
void set_boundkey(double bl, double bu, MSKboundkeye *bk)
{
	if (isnan(bl) || isnan(bu))
		throw msk_exception("NAN values not allowed in bounds");

	if (!isinf(bl) && !isinf(bu) && bl > bu)
		throw msk_exception("The upper bound should be larger than the lower bound");

	if (isinf(bl) && ispos(bl))
		throw msk_exception("+INF values not allowed as lower bound");

	if (isinf(bu) && !ispos(bu))
		throw msk_exception("-INF values not allowed as upper bound");

	// Return the bound key
	if (isinf(bl))
	{
		if (isinf(bu))
			*bk = MSK_BK_FR;
		else
			*bk = MSK_BK_UP;
	}
	else
	{
		if (isinf(bu))
			*bk = MSK_BK_LO;
		else
		{
			// Normally this should be abs(bl-bu) <= eps, but we
			// require bl and bu stems from same double variable.
			if (bl == bu)
				*bk = MSK_BK_FX;
			else
				*bk = MSK_BK_RA;
		}
	}
}

/* This function checks and sets the parameters of the MOSEK task. */
void set_parameter(MSKtask_t task, string type, string name, SEXP value)
{
	if (isEmpty(value)) {
		printwarning("The parameter '" + name + "' was ignored due to an empty definition.");
		return;
	}

	if (length(value) >= 2) {
		throw msk_exception("The parameter '" + name + "' had more than one element in its definition.");
	}

	// Convert name to mosek input with correct prefix
	strtoupper(name);
	if (type == "iparam") append_mskprefix(name, "MSK_IPAR_"); else
	if (type == "dparam") append_mskprefix(name, "MSK_DPAR_"); else
	if (type == "sparam") append_mskprefix(name, "MSK_SPAR_"); else
		throw msk_exception("A parameter type was not recognized");

	MSKparametertypee ptype;
	MSKintt pidx;
	errcatch( MSK_whichparam(task, const_cast<MSKCONST char*>(name.c_str()), &ptype, &pidx) );

	switch (ptype) {
		case MSK_PAR_INT_TYPE:
		{
			int mskvalue;

			if (isNumeric(value))
				mskvalue = INTEGER_ELT(value, 0);

			else if (isString(value)) {
				string valuestr = CHARACTER_ELT(value, 0);

				// Convert value string to mosek input
				strtoupper(valuestr);
				append_mskprefix(valuestr, "MSK_");

				char mskvaluestr[MSK_MAX_STR_LEN];
				if (!MSK_symnamtovalue(const_cast<MSKCONST char*>(valuestr.c_str()), mskvaluestr))
					throw msk_exception("The value of parameter " + name + " was not recognized");

				mskvalue = atoi(mskvaluestr);

			} else {
				throw msk_exception("The value of parameter " + name + " should be an integer or string");
			}

			errcatch( MSK_putintparam(task,(MSKiparame)pidx,mskvalue) );
			break;
		}

		case MSK_PAR_DOU_TYPE:
		{
			if (!isNumeric(value))
				throw msk_exception("The value of parameter " + name + " should be a double");

			double mskvalue = NUMERIC_ELT(value, 0);

			errcatch( MSK_putdouparam(task,(MSKdparame)pidx,mskvalue) );
			break;
		}

		case MSK_PAR_STR_TYPE:
		{
			if (!isString(value))
				throw msk_exception("The value of parameter " + name + " should be a string");

			string mskvalue = CHARACTER_ELT(value, 0);

			errcatch( MSK_putstrparam(task, (MSKsparame)pidx, const_cast<MSKCONST char*>(mskvalue.c_str())) );
			break;
		}

		default:
			throw msk_exception("Parameter " + name + " was not recognized.");
	}
}

void append_parameters(MSKtask_t task, SEXP_LIST iparam, SEXP_LIST dparam, SEXP_LIST sparam) {
	/* Set integer parameters */
	SEXP iparnames = getAttrib(iparam, R_NamesSymbol);
	for (MSKidxt j=0; j<length(iparam); ++j)
		set_parameter(task, "iparam", CHARACTER_ELT(iparnames,j), VECTOR_ELT(iparam,j));

	/* Set double parameters */
	SEXP dparnames = getAttrib(dparam, R_NamesSymbol);
	for (MSKidxt j=0; j<length(dparam); ++j)
		set_parameter(task, "dparam", CHARACTER_ELT(dparnames,j), VECTOR_ELT(dparam,j));

	/* Set string parameters */
	SEXP sparnames = getAttrib(sparam, R_NamesSymbol);
	for (MSKidxt j=0; j<length(sparam); ++j)
		set_parameter(task, "sparam", CHARACTER_ELT(sparnames,j), VECTOR_ELT(sparam,j));
}

/* This function tells if MOSEK define solution item in solution type. */
bool isdef_solitem(MSKsoltypee s, MSKsoliteme v)
{
	switch (v)
	{
		// Primal variables
		case MSK_SOL_ITEM_XC:
		case MSK_SOL_ITEM_XX:
			switch (s) {
			case MSK_SOL_ITR:  return true;
			case MSK_SOL_BAS:  return true;
			case MSK_SOL_ITG:  return true;
			default:
				throw msk_exception("A solution type was not supported");
			}

		// Linear dual variables
		case MSK_SOL_ITEM_SLC:
		case MSK_SOL_ITEM_SLX:
		case MSK_SOL_ITEM_SUC:
		case MSK_SOL_ITEM_SUX:
			switch (s) {
			case MSK_SOL_ITR:  return true;
			case MSK_SOL_BAS:  return true;
			case MSK_SOL_ITG:  return false;
			default:
				throw msk_exception("A solution type was not supported");
			}

		// Conic dual variable
		case MSK_SOL_ITEM_SNX:
			switch (s) {
			case MSK_SOL_ITR:  return true;
			case MSK_SOL_BAS:  return false;
			case MSK_SOL_ITG:  return false;
			default:
				throw msk_exception("A solution type was not supported");
			}

		// Ignored variables
		case MSK_SOL_ITEM_Y:
			return false;

		// Unsupported variables
		default:
			throw msk_exception("A solution item was not supported");
	}
}

/* This function explains the solution types of MOSEK. */
void getspecs_soltype(MSKsoltypee stype, string &name)
{
	switch (stype) {
		case MSK_SOL_BAS:
			name = "bas";
			break;
		case MSK_SOL_ITR:
			name = "itr";
			break;
		case MSK_SOL_ITG:
			name = "int";
			break;
		default:
			throw msk_exception("A solution type was not supported");
	}
}

/* This function explains the solution items of a MOSEK solution type. */
void getspecs_solitem(MSKsoliteme vtype, int NUMVAR, int NUMCON, string &name, int &size)
{
	switch (vtype) {
		case MSK_SOL_ITEM_SLC:
			name = "slc";
			size = NUMCON;
			break;
		case MSK_SOL_ITEM_SLX:
			name = "slx";
			size = NUMVAR;
			break;
		case MSK_SOL_ITEM_SNX:
			name = "snx";
			size = NUMVAR;
			break;
		case MSK_SOL_ITEM_SUC:
			name = "suc";
			size = NUMCON;
			break;
		case MSK_SOL_ITEM_SUX:
			name = "sux";
			size = NUMVAR;
			break;
		case MSK_SOL_ITEM_XC:
			name = "xc";
			size = NUMCON;
			break;
		case MSK_SOL_ITEM_XX:
			name = "xx";
			size = NUMVAR;
			break;
		case MSK_SOL_ITEM_Y:
			name = "y";
			size = NUMCON;
			break;
		default:
			throw msk_exception("A solution item was not supported");
	}
}

/* This function extract the solution from MOSEK. */
void msk_getsolution(SEXP_handle &sol, MSKtask_t task)
{
	printdebug("msk_getsolution called");

	MSKintt NUMVAR, NUMCON;
	errcatch( MSK_getnumvar(task, &NUMVAR) );
	errcatch( MSK_getnumcon(task, &NUMCON) );

	SEXP_NamedVector solvec;	solvec.initVEC(MSK_SOL_END - MSK_SOL_BEGIN);	sol.protect(solvec);

	// Construct: result -> solution -> solution types
	for (int s=MSK_SOL_BEGIN; s<MSK_SOL_END; ++s)
	{
		SEXP_NamedVector soltype;
		soltype.initVEC(4 + MSK_SOL_ITEM_END);

		MSKsoltypee stype = (MSKsoltypee)s;
		MSKintt isdef_soltype;
		errcatch( MSK_solutiondef(task, stype, &isdef_soltype) );

		if (!isdef_soltype)
			continue;

		// Add the problem status and solution status
		MSKprostae prosta;
		MSKsolstae solsta;
		errcatch( MSK_getsolutionstatus(task, stype, &prosta, &solsta) );

		char solsta_str[MSK_MAX_STR_LEN];
		errcatch( MSK_solstatostr(task, solsta, solsta_str) );
		soltype.pushback("solsta", solsta_str);

		char prosta_str[MSK_MAX_STR_LEN];
		errcatch( MSK_prostatostr(task, prosta, prosta_str) );
		soltype.pushback("prosta", prosta_str);

		// Add the constraint status keys
		{
			MSKstakeye *mskskc = new MSKstakeye[NUMCON];
			errcatch( MSK_getsolutionstatuskeyslice(task,
									MSK_ACC_CON,	/* Request constraint status keys. */
									stype,			/* Current solution type. */
									0,				/* Index of first variable. */
									NUMCON,			/* Index of last variable+1. */
									mskskc));

			SEXP_Vector skcvec;		skcvec.initSTR(NUMCON, false);
			char skcname[MSK_MAX_STR_LEN];
			for (int ci = 0; ci < NUMCON; ci++) {
				errcatch( MSK_sktostr(task, mskskc[ci], skcname) );
				skcvec.pushback(skcname);
			}
			soltype.pushback("skc", skcvec);
		}

		// Add the variable status keys
		{
			MSKstakeye *mskskx = new MSKstakeye[NUMVAR];
			errcatch( MSK_getsolutionstatuskeyslice(task,
									MSK_ACC_VAR,	/* Request variable status keys. */
									stype,			/* Current solution type. */
									0,				/* Index of first variable. */
									NUMVAR,			/* Index of last variable+1. */
									mskskx));

			SEXP_Vector skxvec;		skxvec.initSTR(NUMVAR, false);
			char skxname[MSK_MAX_STR_LEN];
			for (int xi = 0; xi < NUMVAR; xi++) {
				errcatch( MSK_sktostr(task, mskskx[xi], skxname) );
				skxvec.pushback(skxname);
			}
			soltype.pushback("skx", skxvec);
		}

		// Add solution variable slices
		for (int v=MSK_SOL_ITEM_BEGIN; v<MSK_SOL_ITEM_END; ++v)
		{
			MSKsoliteme vtype = (MSKsoliteme)v;

			if (!isdef_solitem(stype, vtype))
				continue;

			string vname;
			int vsize;
			getspecs_solitem(vtype, NUMVAR, NUMCON, vname, vsize);

			SEXP_Vector xxvec;	xxvec.initREAL(vsize);
			double *pxx = REAL(xxvec);
			errcatch( MSK_getsolutionslice(task,
									stype, 		/* Request current solution type. */
									vtype,		/* Which part of solution. */
									0, 			/* Index of first variable. */
									vsize, 		/* Index of last variable+1. */
									pxx));

			soltype.pushback(vname, xxvec);
		}

		string sname;
		getspecs_soltype(stype, sname);

		solvec.pushback(sname, soltype);
	}

	// No need to proctect the solvec if nothing was added
	if (solvec.size() == 0) {
		sol.protect(R_NilValue);
	}
}

void msk_addresponse(SEXP_NamedVector &ret_val, const msk_response &res, bool overwrite=true) {

	// Construct: result -> response
	SEXP_NamedVector res_vec;
	res_vec.initVEC(2);

	res_vec.pushback("code", res.code);
	res_vec.pushback("msg", res.msg);

	// Append to result and overwrite if structure already exists
	// In this way errors, which calls for immediate exit, can overwrite the regular termination code..
	int pos = -1;
	list_seek_Index(&pos, ret_val, "response", true);
	if (pos > 0) {
		if (overwrite) {
			ret_val.set("response", res_vec, pos);
		}
	} else {
		ret_val.pushback("response", res_vec);
	}
}

/*
void msk_getresponse(SEXP_NamedVector &res, MSKrescodee r, bool overwrite=true) {

	// Construct: result -> response
	SEXP_NamedVector response;
	response.initVEC(2);

	response.pushback("code", (double)r);
	response.pushback("msg", msk_responsecode2text(r));

	// Append to result and overwrite if structure already exists
	// In this way errors, which calls for immediate exit, can overwrite the regular termination code..
	int pos = -1;
	list_seek_Index(&pos, res, "response", true);
	if (pos > 0) {
		if (overwrite) {
			res.set("response", response, pos);
		}
	} else {
		res.pushback("response", response);
	}
}
void msk_getresponse(SEXP &res, MSKrescodee r, bool overwrite=true) {
	SEXP_NamedVector res_handle;
	res_handle.initVEC(1);

	msk_getresponse(res_handle, r, overwrite);
	res = res_handle;
}
*/

/* This function initialize the task and sets up the problem. */
void msk_setup(Task_handle &task,
					   MSKobjsensee sense, SEXP_NUMERIC c, double c0,
					   CHM_SP A,
					   SEXP_NUMERIC blc, SEXP_NUMERIC buc,
					   SEXP_NUMERIC blx, SEXP_NUMERIC bux,
					   SEXP_LIST cones, SEXP_NUMERIC intsub)
{
	int NUMANZ = A->nzmax;
	int NUMCON = A->nrow;
	int NUMVAR = A->ncol;
	int NUMINTVAR = length(intsub);
	int NUMCONES  = length(cones);

	/* Matrix A */
	if (A->itype == CHOLMOD_LONG)
		if (sizeof(MSKlidxt) != sizeof(UF_long))
			throw msk_exception("Matrix from package 'Matrix' has wrong itype");

	if (A->xtype != CHOLMOD_REAL) // ==CHOLMOD_ZOMPLEX)
		throw msk_exception("Matrix from package 'Matrix' has wrong xtype");

	if (A->dtype != CHOLMOD_DOUBLE)
		throw msk_exception("Matrix from package 'Matrix' has wrong dtype");

	if (A->sorted == false)
		throw msk_exception("Matrix from package 'Matrix' is not sorted");

	if (A->packed == false)
		throw msk_exception("Matrix from package 'Matrix' is not packed");

	// Assumes A->itype == CHOLMOD_INT:	When typecasting to MSKlidxt*
	// Assumes A->packed == true: 		When reading A->p, otherwise use A->nz
	MSKlidxt *aptr = (MSKlidxt*)A->p;

	// Assumes A->itype == CHOLMOD_INT:		When typecasting to int*
	// Assumes A->sorted == true;			When reading asub further down
	MSKlidxt *asub = (MSKlidxt*)A->i;

	// Assumes A->dtype == CHOLMOD_DOUBLE:	When typecasting to double*
	// Assumes A->xtype != CHOLMOD_ZOMPLEX: When reading to A->x, otherwise use A->z
	// Assumes A->sorted == true;			When reading aval further down
	double *aval = (double*)A->x;

	/* Bounds on constraints. */
	MSKboundkeye bkc[NUMCON];
	for (int i=0; i<NUMCON; i++)
		set_boundkey(NUMERIC_ELT(blc,i), NUMERIC_ELT(buc,i), &bkc[i]);

	/* Bounds on variables. */
	MSKboundkeye bkx[NUMVAR];
	for (int i=0; i<NUMVAR; i++)
		set_boundkey(NUMERIC_ELT(blx,i), NUMERIC_ELT(bux,i), &bkx[i]);

	// Make sure the environment is initialized
	global_env.init();

	try
	{
		/* Create the task */
		task.init(global_env, NUMCON, NUMVAR);

		/* Give MOSEK an estimate of the size of the input data.
		 * This is done to increase the speed of inputting data.
		 * However, it is optional. */
		errcatch( MSK_putmaxnumvar(task, NUMVAR) );
		errcatch( MSK_putmaxnumcon(task, NUMCON) );
		errcatch( MSK_putmaxnumanz(task, NUMANZ) );

		/* Append 'NUMCON' empty constraints.
		 * The constraints will initially have no bounds. */
		errcatch( MSK_append(task, MSK_ACC_CON, NUMCON) );

		/* Append 'NUMVAR' variables.
		 * The variables will initially be fixed at zero (x=0). */
		errcatch( MSK_append(task, MSK_ACC_VAR, NUMVAR) );

		/* Optionally add a constant term to the objective. */
		errcatch( MSK_putcfix(task, c0) );

		for (MSKidxt j=0; j<NUMVAR; ++j)
		{
			/* Set the linear term c_j in the objective.*/
			errcatch( MSK_putcj(task, j, NUMERIC_ELT(c,j)) );

			/* Set the bounds on variable j.
			   blx[j] <= x_j <= bux[j] */
			errcatch( MSK_putbound(task,
						MSK_ACC_VAR,			/* Put bounds on variables.*/
						j, 						/* Index of variable.*/
						bkx[j],  				/* Bound key.*/
						NUMERIC_ELT(blx,j),  	/* Numerical value of lower bound.*/
						NUMERIC_ELT(bux,j))); 	/* Numerical value of upper bound.*/

			/* Input column j of A */
			errcatch( MSK_putavec(task,
						MSK_ACC_VAR, 		/* Input columns of A.*/
						j, 					/* Variable (column) index.*/
						aptr[j+1]-aptr[j],	/* Number of non-zeros in column j.*/
						asub+aptr[j], 		/* Pointer to row indexes of column j.*/
						aval+aptr[j])); 	/* Pointer to Values of column j.*/
		}

		/* Set the bounds on constraints.
		 * for i=1, ...,NUMCON : blc[i] <= constraint i <= buc[i] */
		for (MSKidxt i=0; i<NUMCON; ++i)
			errcatch( MSK_putbound(task,
					MSK_ACC_CON,			/* Put bounds on constraints.*/
					i,						/* Index of constraint.*/
					bkc[i],					/* Bound key.*/
					NUMERIC_ELT(blc,i),		/* Numerical value of lower bound.*/
					NUMERIC_ELT(buc,i)));	/* Numerical value of upper bound.*/

		/* Set the conic constraints. */
		for (MSKidxt j=0; j<NUMCONES; ++j)
			append_cone(task, VECTOR_ELT(cones,j), j);

		/* Set the integer variables */
		for (MSKidxt j=0; j<NUMINTVAR; ++j)
			errcatch( MSK_putvartype(task, INTEGER_ELT(intsub,j)-1, MSK_VAR_TYPE_INT) );

		/* Set objective sense. */
		errcatch( MSK_putobjsense(task, sense) );

	} catch (exception const& e) {
		printoutput("An error occurred while setting up the problem.\n", typeERROR);
		throw;
	}
}

// ------------------------------
// STRUCTS OF INPUT DATA
// ------------------------------
struct options_type {
private:
	bool initialized;

public:
	// Recognised options arguments in R
	// TODO: Upgrade to new C++11 initialisers
	static const struct R_ARGS_type {

		vector<string> arglist;
		const string useparam;
		const string usesol;
		const string verbose;
		const string writebefore;
		const string writeafter;

		R_ARGS_type() :
			useparam("useparam"),
			usesol("usesol"),
			verbose("verbose"),
			writebefore("writebefore"),
			writeafter("writeafter")
		{
			string temp[] = {useparam, usesol, verbose, writebefore, writeafter};
			arglist = vector<string>(temp, temp + sizeof(temp)/sizeof(string));
		}
	} R_ARGS;


	// Data definition (intentionally kept close to R types)
	bool	useparam;
	bool 	usesol;
	double 	verbose;
	string	writebefore;
	string	writeafter;

	// Default values of optional arguments
	options_type() :
		initialized(false),

		useparam(true),
		usesol(true),
		verbose(10),
		writebefore(""),
		writeafter("")
	{}


	void R_read(SEXP_LIST object) {
		if (!isNamedVector(object)) {
			throw msk_exception("Internal function options_type::read received an unknown request");
		}
		SEXP_handle arglist(object);

		printdebug("Reading options");

		// Read verbose and update message system
		list_seek_Scalar(&verbose, arglist, R_ARGS.verbose, true);
		mosek_interface_verbose = verbose;
		printpendingmsg();

		// Read remaining input arguments
		list_seek_Boolean(&useparam, arglist, R_ARGS.useparam,  true);
		list_seek_Boolean(&usesol, arglist, R_ARGS.usesol, true);
		list_seek_String(&writebefore, arglist, R_ARGS.writebefore, true);
		list_seek_String(&writeafter, arglist, R_ARGS.writeafter, true);

		// Check for bad arguments
		validate_NamedVector(arglist, "", R_ARGS.arglist);

		initialized = true;
	}

//	SEXP_LIST R_write() {
//		//TODO: Is this needed?
//	}
};
const options_type::R_ARGS_type options_type::R_ARGS;

// Problem input definition
class problem_type {
private:
	bool initialized;

public:

	//
	// Recognised problem arguments in R
	// TODO: Upgrade to new C++11 initialisers
	//
	static const struct R_ARGS_type {
	public:
		vector<string> arglist;
		const string sense;
		const string c;
		const string c0;
		const string A;
		const string blc;
		const string buc;
		const string blx;
		const string bux;
		const string cones;
		const string intsub;
		const string sol;
		const string iparam;
		const string dparam;
		const string sparam;
//		const string options;

		R_ARGS_type() :
			sense("sense"),
			c("c"),
			c0("c0"),
			A("A"),
			blc("blc"),
			buc("buc"),
			blx("blx"),
			bux("bux"),
			cones("cones"),
			intsub("intsub"),
			sol("sol"),
			iparam("iparam"),
			dparam("dparam"),
			sparam("sparam")
//			options("options")
		{
			string temp[] = {sense, c, c0, A, blc, buc, blx, bux, cones, intsub, sol, iparam, dparam, sparam}; //options
			arglist = vector<string>(temp, temp + sizeof(temp)/sizeof(string));
		}

	} R_ARGS;

	//
	// Data definition (intentionally kept close to R types)
	// Note: 'size_t' can hold 'MSKintt' assuming non-negativity, but 'size_t' is incomparable to 'MSKint64t'
	//
	MSKintt	numnz;
	MSKintt	numcon;
	MSKintt	numvar;
	MSKintt	numintvar;
	MSKintt	numcones;

	MSKobjsensee	sense;
	SEXP_handle 	c;
	double 			c0;
	CHM_SP 			A;		// TODO: Is this extra conversion necessary?  SEXP <-> CHOLMOD_SPARSE <-> MOSEK.
	SEXP_handle 	blc;
	SEXP_handle 	buc;
	SEXP_handle 	blx;
	SEXP_handle 	bux;
	SEXP_handle 	cones;
	SEXP_handle 	intsub;
	SEXP_handle 	initsol;
	SEXP_handle 	iparam;
	SEXP_handle 	dparam;
	SEXP_handle 	sparam;
	options_type 	options;

	// Default values of optional arguments
	problem_type() :
		initialized(false),

		sense	(MSK_OBJECTIVE_SENSE_UNDEFINED),
		c0		(0),
		options	(options_type())
	{}


	void R_read(SEXP_LIST object) {
		if (initialized) {
			throw msk_exception("Internal error in problem_type::R_read, a problem was already loaded");
		}

		if (!isNamedVector(object)) {
			throw msk_exception("Internal error in problem_type::R_read, received an unknown request");
		}
		SEXP_handle arglist(object);

		printdebug("Started reading R problem input");

		// Constraint Matrix
		list_seek_SparseMatrix(&A, arglist, R_ARGS.A);
		numnz = A->nzmax;
		numcon = A->nrow;
		numvar = A->ncol;

		// Objective sense
		string sensename = "UNDEFINED";
		list_seek_String(&sensename, arglist, R_ARGS.sense);
		sense = get_objective(sensename);

		// Objective function
		list_seek_Numeric(c, arglist, R_ARGS.c, numvar);
		list_seek_Scalar(&c0, arglist, R_ARGS.c0, true);

		// Constraint and Variable Bounds
		list_seek_Numeric(blc, arglist, R_ARGS.blc, numcon);
		list_seek_Numeric(buc, arglist, R_ARGS.buc, numcon);
		list_seek_Numeric(blx, arglist, R_ARGS.blx, numvar);
		list_seek_Numeric(bux, arglist, R_ARGS.bux, numvar);

		// Cones, Integers variables and Solutions
		list_seek_NamedVector(cones, arglist, R_ARGS.cones, true);
		list_seek_Numeric(intsub, arglist, R_ARGS.intsub, true);
		list_seek_NamedVector(initsol, arglist, R_ARGS.sol, true);
		numcones = length(cones);
		numintvar = length(intsub);

		// Parameters
		list_seek_NamedVector(iparam, arglist, R_ARGS.iparam, true);
		list_seek_NamedVector(dparam, arglist, R_ARGS.dparam, true);
		list_seek_NamedVector(sparam, arglist, R_ARGS.sparam, true);

//		// Options (use this to allow options to overwrite other input channels)
//		SEXP_LIST options_arglist = R_NilValue;
//		list_seek_List(&options_arglist, arglist, R_ARGS.options, true);
//		if (options_arglist != R_NilValue) {
//			options.R_read(options_arglist);
//		}

		// Check for bad arguments
		validate_NamedVector(arglist, "", R_ARGS.arglist);

		initialized = true;
	}

	void R_write(SEXP_NamedVector &prob_val) {
		if (!initialized) {
			throw msk_exception("Internal error in problem_type::R_write, no problem was loaded");
		}

		printdebug("Started writing R problem output");

		prob_val.initVEC( problem_type::R_ARGS.arglist.size() );

		// Objective sense
		prob_val.pushback("sense", getspecs_sense(sense));

		// Objective
		prob_val.pushback("c", c);
		prob_val.pushback("c0", c0);

		// Constraint Matrix A
		SEXP MatrixA = M_chm_sparse_to_SEXP(A, 0, 0, 0, NULL, R_NilValue);
		prob_val.pushback("A", MatrixA);

		// Constraint and variable bounds
		prob_val.pushback("blc", blc);
		prob_val.pushback("buc", buc);
		prob_val.pushback("blx", blx);
		prob_val.pushback("bux", bux);

		// Cones
		if (numcones > 0) {
			prob_val.pushback("cones", cones);
		}

		// Integer subindexes
		if (numintvar > 0) {
			prob_val.pushback("intsub", intsub);
		}

		// Parameters
		if (options.useparam) {
			if (!isEmpty(iparam))
				prob_val.pushback("iparam", iparam);

			if (!isEmpty(dparam))
				prob_val.pushback("dparam", dparam);

			if (!isEmpty(sparam))
				prob_val.pushback("sparam", sparam);
		}

		// Initial solution
		if (options.usesol) {
			if (!isEmpty(initsol))
				prob_val.pushback("sol", initsol);
		}
	}

	void MOSEK_read(Task_handle &task) {
		if (initialized) {
			throw msk_exception("Internal error in problem_type::MOSEK_read, a problem was already loaded");
		}

		printdebug("Started reading MOSEK problem output");

		// Get problem dimensions
		{
			errcatch( MSK_getnumanz(task, &numnz) );
			errcatch( MSK_getnumcon(task, &numcon) );
			errcatch( MSK_getnumvar(task, &numvar) );
			errcatch( MSK_getnumintvar(task, &numintvar) );
			errcatch( MSK_getnumcone(task, &numcones) );
		}

		// Objective sense and constant
		{
			printdebug("problem_type::MOSEK_read - Objective sense and constant");
			errcatch( MSK_getobjsense(task, &sense) );
			errcatch( MSK_getcfix(task, &c0) );
		}

		// Objective coefficients
		{
			printdebug("problem_type::MOSEK_read - Objective coefficients");

			SEXP_Vector cvec;	cvec.initREAL(numvar);	c.protect(cvec);
			double *pc = REAL(c);
			errcatch( MSK_getc(task, pc) );
		}

		// Constraint Matrix A
		{
			printdebug("problem_type::MOSEK_read - Constraint matrix");

			global_SparseMatrix.init( M_cholmod_allocate_sparse(numcon, numvar, numnz, true, true, 0, CHOLMOD_REAL, &chol) );
			A = (CHM_SP)global_SparseMatrix;
			MSKintt *ptrb = (MSKintt*)A->p;
			MSKlidxt *sub = (MSKlidxt*)A->i;
			MSKrealt *val = (MSKrealt*)A->x;
			MSKintt surp[1] = {numnz};

			errcatch( MSK_getaslice(task, MSK_ACC_VAR, 0, numvar, numnz, surp,
					ptrb, ptrb+1, sub, val) );
		}

		// Constraint bounds
		{
			printdebug("problem_type::MOSEK_read - Constraint bounds");

			SEXP_Vector blcvec;		blcvec.initREAL(numcon);	blc.protect(blcvec);
			SEXP_Vector bucvec;		bucvec.initREAL(numcon);	buc.protect(bucvec);

			double *pblc = REAL(blc);
			double *pbuc = REAL(buc);

			MSKboundkeye *bk = new MSKboundkeye[numcon];

			try {
				errcatch( MSK_getboundslice(task, MSK_ACC_CON, 0, numcon, bk, pblc, pbuc) );

				for (int i=0; i<numcon; i++) {
					switch (bk[i]) {
					case MSK_BK_FR:
						pbuc[i] = INFINITY;
						pblc[i] = -INFINITY;
						break;
					case MSK_BK_LO:
						pbuc[i] = INFINITY;
						break;
					case MSK_BK_UP:
						pblc[i] = -INFINITY;
						break;
					default:
						break;
					}
				}

			} catch (exception const& e) {
				delete[] bk;
				throw;
			} /* OTHERWISE */ {
				delete[] bk;
			}
		}

		// Variable bounds
		{
			printdebug("problem_type::MOSEK_read - Variable bounds");

			SEXP_Vector blxvec;		blxvec.initREAL(numvar);	blx.protect(blxvec);
			SEXP_Vector buxvec;		buxvec.initREAL(numvar);	bux.protect(buxvec);

			double *pblx = REAL(blx);
			double *pbux = REAL(bux);

			MSKboundkeye *bk = new MSKboundkeye[numvar];

			try {
				errcatch( MSK_getboundslice(task, MSK_ACC_VAR, 0, numvar, bk, pblx, pbux) );

				for (int i=0; i<numvar; i++) {
					switch (bk[i]) {
					case MSK_BK_FR:
						pbux[i] = INFINITY;
						pblx[i] = -INFINITY;
						break;
					case MSK_BK_LO:
						pbux[i] = INFINITY;
						break;
					case MSK_BK_UP:
						pblx[i] = -INFINITY;
						break;
					default:
						break;
					}
				}

			} catch (exception const& e) {
				delete[] bk;
				throw;
			} /* OTHERWISE */ {
				delete[] bk;
			}
		}

		// Cones
		if (numcones > 0) {
			printdebug("problem_type::MOSEK_read - Cones");

			SEXP_NamedVector conesvec;	conesvec.initVEC(numcones);	cones.protect(conesvec);

			for (int i=0; i<numcones; i++) {
				SEXP_NamedVector cone;
				cone.initVEC(2);

				MSKintt numconemembers;
				MSK_getnumconemem(task, i, &numconemembers);

				SEXP_Vector subvec;		subvec.initINT(numconemembers);
				MSKlidxt *psub = INTEGER(subvec);

				MSKconetypee msktype;
				errcatch( MSK_getcone(task, i, &msktype, NULL, &numconemembers, psub) );

				// R indexes count from 1, not from 0 as MOSEK
				for (int k=0; k<numconemembers; k++) {
					psub[k]++;
				}

				char type[MSK_MAX_STR_LEN];
				errcatch( MSK_conetypetostr(task, msktype, type) );

				string strtype = type;
				remove_mskprefix(strtype,"MSK_CT_");

				cone.pushback("type", strtype);
				cone.pushback("sub", subvec);

				conesvec.pushback("", cone);
			}
		}

		// Integer subindexes
		if (numintvar > 0) {
			printdebug("problem_type::MOSEK_read - Integer subindexes");

			SEXP_Vector intsubvec;	intsubvec.initINT(numintvar);	intsub.protect(intsubvec);

			MSKlidxt *pintsub = INTEGER(intsub);

			int idx = 0;
			MSKvariabletypee type;
			for (int i=0; i<numvar; i++) {
				errcatch( MSK_getvartype(task,i,&type) );

				// R indexes count from 1, not from 0 as MOSEK
				if (type == MSK_VAR_TYPE_INT) {
					pintsub[idx++] = i+1;

					if (idx >= numintvar)
						break;
				}
			}
		}

		// Integer Parameters
		if (options.useparam) {
			printdebug("problem_type::MOSEK_read - Integer Parameters");

			SEXP_NamedVector iparamvec;	iparamvec.initVEC(MSK_IPAR_END - MSK_IPAR_BEGIN);	iparam.protect(iparamvec);

			char paramname[MSK_MAX_STR_LEN];
			char valuename[MSK_MAX_STR_LEN];	MSKintt value;
			for (int v=MSK_IPAR_BEGIN; v<MSK_IPAR_END; ++v) {

				// Get name of parameter
				errcatch( MSK_getparamname(task, MSK_PAR_INT_TYPE, v, paramname) );

				string paramstr = paramname;
				remove_mskprefix(paramstr, "MSK_IPAR_");

				// Get value of parameter
				errcatch( MSK_getintparam(task, (MSKiparame)v, &value) );
				errcatch( MSK_iparvaltosymnam(global_env, (MSKiparame)v, value, valuename) );

				string valuestr = valuename;
				remove_mskprefix(valuestr, "MSK_");

				// Append parameter to list
				if (!valuestr.empty())
					iparamvec.pushback(paramstr, valuestr);
				else
					iparamvec.pushback(paramstr, value);
			}
		}

		// Double Parameters
		if (options.useparam) {
			printdebug("problem_type::MOSEK_read - Double Parameters");

			SEXP_NamedVector dparamvec;	dparamvec.initVEC(MSK_DPAR_END - MSK_DPAR_BEGIN);	dparam.protect(dparamvec);

			char paramname[MSK_MAX_STR_LEN];	MSKrealt value;
			for (int v=MSK_DPAR_BEGIN; v<MSK_DPAR_END; ++v) {

				// Get name of parameter
				errcatch( MSK_getparamname(task, MSK_PAR_DOU_TYPE, v, paramname) );

				string paramstr = paramname;
				remove_mskprefix(paramstr, "MSK_DPAR_");

				// Get value of parameter
				errcatch( MSK_getdouparam(task, (MSKdparame)v, &value) );

				// Append parameter to list
				dparamvec.pushback(paramstr, value);
			}
		}

		// String Parameters
		if (options.useparam) {
			printdebug("problem_type::MOSEK_read - String Parameters");

			SEXP_NamedVector sparamvec;	sparamvec.initVEC(MSK_SPAR_END - MSK_SPAR_BEGIN);	sparam.protect(sparamvec);

			char paramname[MSK_MAX_STR_LEN];	char *value;
			for (int v=MSK_SPAR_BEGIN; v<MSK_SPAR_END; ++v) {

				// Get name of parameter
				errcatch( MSK_getparamname(task, MSK_PAR_STR_TYPE, v, paramname) );

				string paramstr = paramname;
				remove_mskprefix(paramstr, "MSK_SPAR_");

				// Prepare for value of parameter
				size_t strlength;
				errcatch( MSK_getstrparam(task, (MSKsparame)v, 0, &strlength, NULL) );
				value = new char[strlength++];

				try {
					// Get value of parameter
					errcatch( MSK_getstrparam(task, (MSKsparame)v, strlength, NULL, value) );

					// Append parameter to list
					sparamvec.pushback(paramstr, value);

				} catch (exception const& e) {
					delete[] value;
					throw;
				} /* OTHERWISE */ {
					delete[] value;
				}
			}
		}

		// Initial solution
		if (options.usesol) {
			printdebug("problem_type::MOSEK_read - Initial solution");
			msk_getsolution(initsol, task);
		}

		initialized = true;
	}

	void MOSEK_write(Task_handle &task) {
		if (!initialized) {
			throw msk_exception("Internal error in problem_type::MOSEK_write, no problem was loaded");
		}

		printdebug("Started writing MOSEK problem input");

		/* Set problem description */
		msk_setup(task, sense, c, c0,
				A, blc, buc, blx, bux,
				cones, intsub);

		/* Set initial solution */
		if (options.usesol) {
			append_initsol(task, initsol, numcon, numvar);
		}

		/* Set parameters */
		if (options.useparam) {
			append_parameters(task, iparam, dparam, sparam);
		}

		printdebug("MOSEK_write finished");
	}
};
const problem_type::R_ARGS_type problem_type::R_ARGS;

/* This function initialize the task and sets up the problem. */
void msk_loadproblemfile(Task_handle &task, string filepath, bool readparams) {

	// Make sure the environment is initialized
	global_env.init();

	// Initialize the task
	task.init(global_env, 0, 0);

	try
	{
		errcatch( MSK_readdata(task, const_cast<MSKCONST char*>(filepath.c_str())) );

	} catch (exception const& e) {
		printerror("An error occurred while loading up the problem from a file");
		throw;
	}
}

/* This function interrupts MOSEK if CTRL+C is caught in R */
void mskcallback_dummyfun(void *data) { R_CheckUserInterrupt(); }
static int MSKAPI mskcallback(MSKtask_t task, MSKuserhandle_t handle, MSKcallbackcodee caller) {

	// FIXME: This interruption checking has not been documented and may be subject to change.
	mosek_interface_signal_caught = (FALSE == R_ToplevelExec(mskcallback_dummyfun, NULL));

	if (mosek_interface_signal_caught) {
		printoutput("Interruption caught, terminating at first chance...\n", typeMOSEK);
		return 1;
	}
	return 0;
}

/* This function calls MOSEK and returns the solution. */
void msk_solve(SEXP_NamedVector &ret_val, Task_handle &task, options_type options)
{
	//
	// STEP 1 - INITIALIZATION
	//
	printdebug("msk_solve - INITIALIZATION");
	{
		/* Make it interruptible with CTRL+C */
		errcatch( MSK_putcallbackfunc(task, mskcallback, (void*)NULL) );

		/* Write file containing problem description (filetypes: .lp, .mps, .opf, .mbt) */
		if (!options.writebefore.empty()) {
			MSK_putintparam(task, MSK_IPAR_OPF_WRITE_SOLUTIONS, MSK_ON);
			errcatch( MSK_writedata(task, const_cast<MSKCONST char*>(options.writebefore.c_str())) );
		}
	}

	//
	// STEP 2 - OPTIMIZATION
	//
	printdebug("msk_solve - OPTIMIZATION");
	try
	{
		/* Separate interface warnings from MOSEK output */
		if (mosek_interface_warnings > 0)
			printoutput("\n", typeWARNING);

		/* Run optimizer */
		MSKrescodee trmcode;
		errcatch( MSK_optimizetrm(task, &trmcode) );
		msk_addresponse(ret_val, msk_response(trmcode));

	} catch (exception const& e) {
		// Report that the CTRL+C interruption has been handled
		if (mosek_interface_signal_caught) {
			printoutput("Optimization interrupted because of termination signal, e.g. <CTRL> + <C>.\n", typeERROR);
		} else {
			printoutput("Optimization interrupted.\n", typeERROR);
		}

		mosek_interface_signal_caught = false;
		throw;
	}

	//
	// STEP 3 - EXTRACT SOLUTION
	//
	printdebug("msk_solve - EXTRACT SOLUTION");
	try
	{
		/* Write file containing problem description (solution only included if filetype is .opf or .mbt) */
		if (!options.writeafter.empty()) {
			MSK_putintparam(task, MSK_IPAR_OPF_WRITE_SOLUTIONS, MSK_ON);
			errcatch( MSK_writedata(task, const_cast<MSKCONST char*>(options.writeafter.c_str())) );
		}

		/* Print a summary containing information
		 * about the solution for debugging purposes. */
		errcatch( MSK_solutionsummary(task, MSK_STREAM_LOG) );

		/* Extract solution from Mosek to R */
		SEXP_handle sol_val;
		msk_getsolution(sol_val, task);
		ret_val.pushback("sol", sol_val);

	} catch (exception const& e) {
		printoutput("An error occurred while extracting the solution.\n", typeERROR);
		throw;
	}
}

void clean_globalressources() {
	// The mosek environment 'global_env' should not be cleared, as we wish to
	// reuse the license until mosek_clean() is called or .SO/.DLL is unloaded.

	global_SparseMatrix.~CHM_SP_handle();
	global_task.~Task_handle();
}

void reset_globalvariables() {
	mosek_interface_verbose  = NAN;   				// Declare messages as pending
	mosek_interface_warnings = 0;
	mosek_interface_signal_caught = false;
	mosek_interface_termination_success = false;	// No success before we call 'terminate_successfully' or 'init_early_exit'
}

void terminate_successfully() {
	clean_globalressources();
	mosek_interface_termination_success = true;
}

void init_early_exit(const char* msg) {
	// Force pending and future messages through
	if (isnan(mosek_interface_verbose)) {
		mosek_interface_verbose = typeALL;
	}
	printpendingmsg();
	printerror(msg);
	terminate_successfully();
}

SEXP mosek(SEXP arg0, SEXP arg1) {
	const string ARGNAMES[] = {"problem","options"};
	const string ARGTYPES[] = {"list","list"};

	// Clean in case of Rf_error in previous run
	if (!mosek_interface_termination_success) {
		terminate_successfully();
	}

	// Create handled for returned data
	SEXP_NamedVector ret_val;

	try {
		// Start the program
		reset_globalvariables();
		ret_val.initVEC(2);

		// Validate input arguments
		if (!isNamedVector(arg0)) {
			throw msk_exception("Input argument " + ARGNAMES[0] + " should be a " + ARGTYPES[0] + ".");
		}
		if (!isNamedVector(arg1)) {
			throw msk_exception("Input argument " + ARGNAMES[1] + " should be a " + ARGTYPES[1] + ".");
		}

		// Read input arguments: problem and options
		problem_type probin;
		probin.options.R_read(arg1);
		probin.R_read(arg0);

		// Create task and load problem into MOSEK
		Task_handle &task = global_task;
		probin.MOSEK_write(task);

		// Solve the problem
		msk_solve(ret_val, task, probin.options);

	} catch (msk_exception const& e) {
		init_early_exit(e.what());
 		msk_addresponse(ret_val, e.getresponse());
		return ret_val;

	} catch (exception const& e) {
		init_early_exit(e.what());
		return ret_val;
	}

	if (mosek_interface_warnings > 0) {
		printoutput("The R-to-MOSEK interface completed with " + tostring(mosek_interface_warnings) + " warning(s)\n\n", typeWARNING);
	}

	// Clean allocations and exit
	terminate_successfully();
	return ret_val;
}

SEXP mosek_clean() {
	// Print in case of Rf_error in previous run
	if (!mosek_interface_termination_success) {
		printoutput("----- MOSEK_CLEAN -----\n", typeERROR);
	}

	// TODO: We should read verbose from an 'opts' input argument, but on
	// the other hand this recovery function should be as fail-safe as possible
	mosek_interface_verbose = typeALL;

	// Clean resources such as tasks before the environment!
	clean_globalressources();
	global_env.~Env_handle();
	mosek_interface_termination_success = true;

	return R_NilValue;

//	// Add the response code
//	SEXP_NamedVector ret_val; ret_val.initVEC(1);
//	msk_addresponse(ret_val, msk_response(MSK_RES_OK), false);
//	return ret_val;
}

SEXP mosek_version() {
	MSKintt major, minor, build, revision;
	MSK_getversion(&major, &minor, &build, &revision);

	// Output
	SEXP_Vector ret_val;
	ret_val.initSTR(1, false);
	ret_val.pushback("MOSEK " + tostring(major) + "." +  tostring(minor) + "." + tostring(build) + "." + tostring(revision));

	return ret_val;
}

SEXP mosek_read(SEXP arg0, SEXP arg1) {
	const string ARGNAMES[] = {"filepath","options"};
	const string ARGTYPES[] = {"string","list"};

	// Clean in case of Rf_error in previous run
	if (!mosek_interface_termination_success) {
		terminate_successfully();
	}

	// Create handled for returned data
	SEXP_NamedVector ret_val;

	try {
		// Start the program
		reset_globalvariables();
		ret_val.initVEC(2);

		// Validate input arguments
		printdebug("Validate input arguments");
		string filepath = CHARACTER_ELT(arg0, 0);
		if (filepath.empty()) {
			throw msk_exception("Input argument " + ARGNAMES[0] + " should be a " + ARGTYPES[0] + ".");
		}
		if (!isNamedVector(arg1)) {
			throw msk_exception("Input argument " + ARGNAMES[1] + " should be a " + ARGTYPES[1] + ".");
		}

		// Define new default values for options
		options_type default_opts; {
			default_opts.useparam = false;
			default_opts.usesol = false;
		}

		// Read input arguments: options (with modified defaults)
		printdebug("Reading input arguments");
		problem_type probin;
		probin.options = default_opts;
		probin.options.R_read(arg1);

		// Create task and load filepath-model into MOSEK
		printdebug("Loading filepath-model into MOSEK");
		Task_handle &task = global_task;
		msk_loadproblemfile(task, filepath, probin.options.useparam);

		// Read the problem from MOSEK
		printdebug("Reading the problem from MOSEK");
		probin.MOSEK_read(task);

		// Write the problem to R
		printdebug("Writing the problem to R");
		SEXP_NamedVector prob_val;
		probin.R_write(prob_val);
		ret_val.pushback("prob", prob_val);

		// Add the response code
		printdebug("Adding the response code");
		msk_addresponse(ret_val, msk_response(MSK_RES_OK), false);

	} catch (msk_exception const& e) {
		init_early_exit(e.what());
		msk_addresponse(ret_val, e.getresponse());
		return ret_val;

	} catch (exception const& e) {
		init_early_exit(e.what());
		return ret_val;
	}

	if (mosek_interface_warnings > 0) {
		printoutput("The R-to-MOSEK interface completed with " + tostring(mosek_interface_warnings) + " warning(s)\n", typeWARNING);
	}

	// Clean allocations and exit
	terminate_successfully();
	return ret_val;
}

SEXP mosek_write(SEXP arg0, SEXP arg1, SEXP arg2) {
	const string ARGNAMES[] = {"problem","filepath","options"};
	const string ARGTYPES[] = {"named list","string","named list"};

	// Clean in case of Rf_error in previous run
	if (!mosek_interface_termination_success) {
		terminate_successfully();
	}

	// Create handled for returned data
	SEXP_NamedVector ret_val;

	try {
		// Start the program
		reset_globalvariables();
		ret_val.initVEC(1);

		// Validate input arguments
		if (!isNamedVector(arg0)) {
			throw msk_exception("Input argument '" + ARGNAMES[0] + "' should be a '" + ARGTYPES[0] + "'.");
		}
		string filepath = CHARACTER_ELT(arg1, 0);
		if (filepath.empty()) {
			throw msk_exception("Input argument '" + ARGNAMES[1] + "' should be a '" + ARGTYPES[1] + "'.");
		}
		if (!isNamedVector(arg2)) {
			throw msk_exception("Input argument '" + ARGNAMES[2] + "' should be a '" + ARGTYPES[2] + "'.");
		}

		// Define new default values for options
		options_type default_opts; {
			default_opts.useparam = false;
			default_opts.usesol = false;
		}

		// Read input arguments: options (with modified defaults)
		problem_type probin;
		probin.options = default_opts;
		probin.options.R_read(arg2);
		probin.R_read(arg0);

		// Create task and load problem into MOSEK
		Task_handle &task = global_task;
		probin.MOSEK_write(task);

		// Set export-parameters for whether to write any solution loaded into MOSEK
		if (probin.options.usesol) {
			errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_SOLUTIONS, MSK_ON) );
		} else {
			errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_SOLUTIONS, MSK_OFF) );
		}

		// Set export-parameters for whether to write all parameters
		if (probin.options.useparam) {
			errcatch( MSK_putintparam(task, MSK_IPAR_WRITE_DATA_PARAM,MSK_ON) );
			errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_PARAMETERS,MSK_ON) );
		} else {
			errcatch( MSK_putintparam(task, MSK_IPAR_WRITE_DATA_PARAM,MSK_OFF) );
			errcatch( MSK_putintparam(task, MSK_IPAR_OPF_WRITE_PARAMETERS,MSK_OFF) );
		}

		// Write to filepath model (filetypes: .lp, .mps, .opf, .mbt)
		errcatch( MSK_writedata(task, const_cast<MSKCONST char*>(filepath.c_str())) );

		// Add the response code
		msk_addresponse(ret_val, msk_response(MSK_RES_OK), false);

	} catch (msk_exception const& e) {
		init_early_exit(e.what());
		msk_addresponse(ret_val, e.getresponse());
		return ret_val;

	} catch (exception const& e) {
		init_early_exit(e.what());
		return ret_val;
	}

	if (mosek_interface_warnings > 0) {
		printoutput("The R-to-MOSEK interface completed with " + tostring(mosek_interface_warnings) + " warning(s)\n", typeWARNING);
	}

	// Clean allocations and exit
	terminate_successfully();
	return ret_val;
}

void R_init_rmosek(DllInfo *info)
{
	// Register mosek utilities to the R console
	R_CallMethodDef callMethods[] = {
			{"mosek", (DL_FUNC) &mosek, 2},
			{"mosek_clean", (DL_FUNC) &mosek_clean, 0},
			{"mosek_version", (DL_FUNC) &mosek_version, 0},
			{"mosek_write", (DL_FUNC) &mosek_write, 3},
			{"mosek_read" , (DL_FUNC) &mosek_read, 2},
			{NULL, NULL, 0}
	};
	R_registerRoutines(info, NULL, callMethods, NULL, NULL);

	// Register mosek utilities to other packages
	R_RegisterCCallable("rmosek", "mosek", (DL_FUNC) &mosek);
	R_RegisterCCallable("rmosek", "mosek_clean", (DL_FUNC) &mosek_clean);
	R_RegisterCCallable("rmosek", "mosek_version", (DL_FUNC) &mosek_version);
	R_RegisterCCallable("rmosek", "mosek_write", (DL_FUNC) &mosek_write);
	R_RegisterCCallable("rmosek", "mosek_read", (DL_FUNC) &mosek_read);

	// Start cholmod environment
	M_R_cholmod_start(&chol);
}

void R_unload_rmosek(DllInfo *info)
{
	// Finish cholmod environment
	M_cholmod_finish(&chol);
}
