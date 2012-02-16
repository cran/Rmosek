#define R_NO_REMAP
#include "rmsk_obj_sexp.h"

#include "rmsk_numeric_casts.h"

#include <Rinternals.h>
#include <string>


___RMSK_INNER_NS_START___
using std::string;


// ------------------------------
// Class SEXP_Handle
// ------------------------------

SEXP_Handle::SEXP_Handle() {
	object = R_NilValue; initialized = true;
	PROTECT_WITH_INDEX(object, &object_ipx);
}

SEXP_Handle::SEXP_Handle(SEXP input) {
	object = R_NilValue; initialized = true;
	PROTECT_WITH_INDEX(object, &object_ipx);
	protect(input);
}

void SEXP_Handle::protect(SEXP object) {
	REPROTECT(this->object = object, object_ipx);
}

SEXP_Handle::~SEXP_Handle() {
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

// ------------------------------
// Class SEXP_NamedVector
// ------------------------------

void SEXP_NamedVector::initVEC(R_len_t maxsize) {
	if (initialized)
		throw msk_exception("An internal SEXP_NamedVector was attempted initialized twice!");

	this->maxsize = maxsize;

	items.protect( Rf_allocVector(VECSXP, maxsize) );
	SETLENGTH(items, 0);

	names.protect( Rf_allocVector(STRSXP, maxsize) );
	SETLENGTH(names, 0);

	Rf_setAttrib(items, R_NamesSymbol, names);

	initialized = true;
}

R_len_t SEXP_NamedVector::size() {
	return LENGTH(items);
}

void SEXP_NamedVector::pushback(std::string name, SEXP item) {
	if (!initialized)
		throw msk_exception("A SEXP_NamedVector was not initialized in pushback call");

	// SET_VECTOR_ELT and SETLENGTH needs type 'int'
	R_len_t pos = numeric_cast<int>(size());
	if (pos >= maxsize)
		throw msk_exception("Internal SEXP_NamedVector did not have enough capacity");

	SET_VECTOR_ELT(items, pos, item);
	SETLENGTH(items, pos+1);

	SET_STRING_ELT(names, pos, Rf_mkChar(name.c_str()));
	SETLENGTH(names, pos+1);
}

void SEXP_NamedVector::pushback(std::string name, char* item) {
	pushback(name, Rf_mkString(item));
}

void SEXP_NamedVector::pushback(std::string name, std::string item) {
	pushback(name, Rf_mkString(item.c_str()));
}

void SEXP_NamedVector::pushback(std::string name, double item) {
	pushback(name, Rf_ScalarReal(item));
}

void SEXP_NamedVector::pushback(std::string name, int item) {
	pushback(name, Rf_ScalarInteger(item));
}

void SEXP_NamedVector::set(std::string name, SEXP item, int pos) {
	if (!initialized)
		throw msk_exception("A SEXP_NamedVector was not initialized in set call");

	if (!(0 <= pos && pos < numeric_cast<int>(size())))
		throw msk_exception("Internal SEXP_NamedVector failed to place an element");

	SET_VECTOR_ELT(items, pos, item);
	SET_STRING_ELT(names, pos, Rf_mkChar(name.c_str()));
}


// ------------------------------
// Class SEXP_Vector
// ------------------------------

void SEXP_Vector::initSTR(R_len_t maxsize, bool static_size) {
	if (initialized)
		throw msk_exception("An internal SEXP_Vector was attempted initialized twice!");

	this->static_size = static_size;
	this->maxsize = maxsize;
	itemstype = STRSXP;

	items.protect( Rf_allocVector(itemstype, maxsize) );

	if (!static_size) {
		SETLENGTH(items, 0);
	}

	initialized = true;
}

void SEXP_Vector::initREAL(R_len_t maxsize, bool static_size) {
	if (initialized)
		throw msk_exception("An internal SEXP_Vector was attempted initialized twice!");

	this->static_size = static_size;
	this->maxsize = maxsize;
	itemstype = REALSXP;

	items.protect( Rf_allocVector(itemstype, maxsize) );

	if (!static_size) {
		SETLENGTH(items, 0);
	}

	initialized = true;
}

void SEXP_Vector::initINT(R_len_t maxsize, bool static_size) {
	if (initialized)
		throw msk_exception("An internal SEXP_Vector was attempted initialized twice!");

	this->static_size = static_size;
	this->maxsize = maxsize;
	itemstype = INTSXP;

	items.protect( Rf_allocVector(itemstype, maxsize) );

	if (!static_size) {
		SETLENGTH(items, 0);
	}

	initialized = true;
}

void SEXP_Vector::initVEC(R_len_t maxsize, bool static_size) {
	if (initialized)
		throw msk_exception("An internal SEXP_Vector was attempted initialized twice!");

	this->static_size = static_size;
	this->maxsize = maxsize;
	itemstype = VECSXP;

	items.protect( Rf_allocVector(itemstype, maxsize) );

	if (!static_size) {
		SETLENGTH(items, 0);
	}

	initialized = true;
}

R_len_t SEXP_Vector::size() {
	if (static_size)
		return maxsize;
	else
		return LENGTH(items);
}

void SEXP_Vector::pushback(SEXP item) {
	if (!initialized)
		throw msk_exception("A SEXP_Vector was not initialized in pushback call");

	if (static_size)
		throw msk_exception("A static sized SEXP_Vector recieved a dynamic pushback call");

	// SET_VECTOR_ELT and SETLENGTH needs type 'int'
	R_len_t pos = numeric_cast<int>(size());
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

void SEXP_Vector::pushback(char* item) {
	if (itemstype != STRSXP && itemstype != VECSXP)
		throw msk_exception("An internal SEXP_Vector experienced a pushback of the wrong type STRSXP!");

	pushback(Rf_mkChar(item));
}

void SEXP_Vector::pushback(std::string item) {
	if (itemstype != STRSXP && itemstype != VECSXP)
		throw msk_exception("An internal SEXP_Vector experienced a pushback of the wrong type STRSXP!");

	if (itemstype == STRSXP)
		pushback(Rf_mkChar(item.c_str()));

	if (itemstype == VECSXP) {
		// Item have to be nested in STRSXP
		SEXP_Vector temp;		SEXP_Handle temph;
		temp.initSTR(1, true);	temph.protect(temp);
		temp.set(Rf_mkChar(item.c_str()), 0);
		pushback(temph);
	}
}

void SEXP_Vector::pushback(double item) {
	if (itemstype != REALSXP && itemstype != VECSXP)
		throw msk_exception("An internal SEXP_Vector experienced a pushback of the wrong type REALSXP!");

	pushback(Rf_ScalarReal(item));
}

void SEXP_Vector::pushback(int item) {
	if (itemstype != INTSXP && itemstype != VECSXP)
		throw msk_exception("An internal SEXP_Vector experienced a pushback of the wrong type INTSXP!");

	pushback(Rf_ScalarInteger(item));
}

void SEXP_Vector::set(SEXP item, int pos) {
	if (!initialized)
		throw msk_exception("A SEXP_Vector was not initialized in set call");

	if (!(0 <= pos && pos < numeric_cast<int>(size())))
		throw msk_exception("Internal SEXP_Vector failed to place an element");

	if (itemstype == STRSXP)
		SET_STRING_ELT(items, pos, item);
	else
		SET_VECTOR_ELT(items, pos, item);
}


___RMSK_INNER_NS_END___
