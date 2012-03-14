#define R_NO_REMAP
#include "rmsk_obj_matrices.h"

#include "rmsk_utils_sexp.h"
#include "rmsk_obj_mosek.h"


___RMSK_INNER_NS_START___
using std::string;


// ------------------------------
// Global variables
// ------------------------------
pkgMatrixCOO_type global_pkgMatrix_COO;
pkgMatrixCSC_type global_pkgMatrix_CSC;


// ------------------------------
// Enum matrixformat_enum
// ------------------------------

matrixformat_enum get_matrixformat(string name)
{
	strtoupper(name);

	if (name.empty() || name == "COO" || name == "PKGMATRIX:COO") {
		return pkgMatrixCOO;

	} else if (name == "CSC" || name == "PKGMATRIX:CSC") {
		return pkgMatrixCSC;

	} else if (name == "SIMPLE:COO") {
		return simple_matrixCOO;
	}

	throw msk_exception("Option 'matrixformat' must be either 'COO' (coordinate/triplet), 'CSC' (compressed sparse column) or 'simple:COO' (package Matrix independent coordinate/triplet)");
}


// ------------------------------
// Class simple_matrixCOO_type
// ------------------------------

const simple_matrixCOO_type::R_ARGS_type simple_matrixCOO_type::R_ARGS;

void simple_matrixCOO_type::R_read(SEXP_LIST object, std::string name) {
	if (initialized) {
		throw msk_exception("Internal error in simple_matrixCOO_type::R_read, a matrix was already loaded");
	}

	if (!isNamedVector(object)) {
		throw msk_exception("Internal function simple_matrixCOO_type::R_read received an unknown request");
	}
	SEXP_Handle arglist(object);

	printdebug("Reading simple coordinate sparse matrix");

	// Read input arguments
	list_seek_Integer(&_nrow, arglist, R_ARGS.nrow);
	list_seek_Integer(&_ncol, arglist, R_ARGS.ncol);

	list_seek_Numeric(valij, arglist, R_ARGS.valij);	_nnz = Rf_length(valij);
	list_seek_Numeric(subi, arglist, R_ARGS.subi);		validate_Numeric(subi, R_ARGS.subi, _nnz);
	list_seek_Numeric(subj, arglist, R_ARGS.subj);		validate_Numeric(subj, R_ARGS.subj, _nnz);

	// Check for bad arguments
	validate_NamedVector(arglist, name, R_ARGS.arglist);
	initialized = true;
}

void simple_matrixCOO_type::R_write(SEXP_Handle &val) {
	if (!initialized) {
		throw msk_exception("Internal error in simple_matrixCOO_type::R_write, no matrix was loaded");
	}

	SEXP_NamedVector mat;
	mat.initVEC( numeric_cast<R_len_t>( simple_matrixCOO_type::R_ARGS.arglist.size() ) );
	val.protect(mat);

	mat.pushback(R_ARGS.nrow, _nrow);
	mat.pushback(R_ARGS.ncol, _ncol);
	mat.pushback(R_ARGS.subi, subi);
	mat.pushback(R_ARGS.subj, subj);
	mat.pushback(R_ARGS.valij, valij);
}

void simple_matrixCOO_type::MOSEK_read(Task_handle &task) {
	if (initialized) {
		throw msk_exception("Internal error in simple_matrixCOO_type::MOSEK_read, a matrix was already loaded");
	}

	printdebug("Started reading simple coordinate sparse matrix from MOSEK");

	errcatch( MSK_getnumanz(task, &_nnz) );				// FIXME: Use MSK_getnumanz64 instead when R_len_t is 'long'
	errcatch( MSK_getnumcon(task, &_nrow) );
	errcatch( MSK_getnumvar(task, &_ncol) );

	SEXP_Vector ivec;	ivec.initINT(_nnz);		subi.protect(ivec);
	MSKidxt *pi = INTEGER(ivec);

	SEXP_Vector jvec;	jvec.initINT(_nnz);		subj.protect(jvec);
	MSKidxt *pj = INTEGER(jvec);

	SEXP_Vector vvec;	vvec.initREAL(_nnz);	valij.protect(vvec);
	MSKrealt *pv = REAL(vvec);

	MSKintt surp[1] = { numeric_cast<MSKintt>(_nnz) };

	// FIXME: 64bit alternative for MSK_getaslicetrip when R_len_t is 'long'??
	errcatch( MSK_getaslicetrip(task, MSK_ACC_VAR, 0, _ncol, numeric_cast<MSKintt>(_nnz), surp,
			pi, pj, pv) );

	// Convert sub indexing (Plus one because MOSEK indexes counts from 0, not from 1 as R)
	for (R_len_t k = 0; k < _nnz; k++) {
		++pi[k];
		++pj[k];
	}
	initialized = true;
}

void simple_matrixCOO_type::MOSEK_write(Task_handle &task) {
	if (!initialized) {
		throw msk_exception("Internal error in simple_matrixCOO_type::MOSEK_write, no matrix was loaded");
	}

	// Convert sub indexing (Minus one because MOSEK indexes counts from 0, not from 1 as R)
	MSKintt msksubi[_nnz];
	for (R_len_t i = 0; i < _nnz; i++)
		msksubi[i] = INTEGER_ELT(subi,i) - 1;

	// Convert sub indexing (Minus one because MOSEK indexes counts from 0, not from 1 as R)
	MSKintt msksubj[_nnz];
	for (R_len_t j = 0; j < _nnz; j++)
		msksubj[j] = INTEGER_ELT(subj,j) - 1;

	MSKrealt *pvalij = REAL(valij);

	// FIXME: 64bit alternative for MSK_putaijlist when R_len_t is 'long'??
	MSK_putaijlist(task, numeric_cast<MSKintt>(_nnz), msksubi, msksubj, pvalij);
}


// ------------------------------
// Class pkgMatrixCOO_type
// ------------------------------

CHM_TR pkgMatrixCOO_type::matrix = NULL;
bool pkgMatrixCOO_type::initialized = false;
bool pkgMatrixCOO_type::cholmod_allocated = false;

pkgMatrixCOO_type::~pkgMatrixCOO_type() {
	if (initialized) {
		printdebug("Removing a pkgMatrixCOO_type");

		if (cholmod_allocated) {
			M_cholmod_free_triplet(&matrix, &chol);
		} else {
			delete matrix;
		}
		initialized = false;
	}
}

void pkgMatrixCOO_type::R_read(SEXP val, string name) {
	if (initialized) {
		throw msk_exception("Internal error in pkgMatrixCOO_type::R_read, a matrix was already loaded");
	}

	// Read a coordinate sparse matrix using package Matrix
	if (Matrix_isclass_triplet(val)) {
		cholmod_allocated = false;

		matrix = new cholmod_triplet; initialized = true;
		M_as_cholmod_triplet(matrix, val, (Rboolean)FALSE);
	}
	else {
		throw msk_exception("Internal error in pkgMatrixCOO_type::R_read: Call to Matrix_isclass_triplet returned false.");
	}

	// Verify sparse matrix
	if (matrix->itype == CHOLMOD_LONG)
		if (sizeof(MSKlidxt) != sizeof(UF_long))
			throw msk_exception("Matrix from package 'Matrix' has wrong itype");

	if (matrix->xtype != CHOLMOD_REAL)
		throw msk_exception("Matrix from package 'Matrix' has wrong xtype");

	if (matrix->dtype != CHOLMOD_DOUBLE)
		throw msk_exception("Matrix from package 'Matrix' has wrong dtype");
}


void pkgMatrixCOO_type::R_write(SEXP_Handle &val) {
	if (!initialized) {
		throw msk_exception("Internal error in pkgMatrixCOO_type::R_write, a matrix was not loaded");
	}

	val.protect( M_chm_triplet_to_SEXP(matrix, 0, 0, 0, NULL, R_NilValue) );
}


void pkgMatrixCOO_type::MOSEK_read(Task_handle &task) {
	if (initialized) {
		throw msk_exception("Internal error in pkgMatrixCOO_type::MOSEK_read, a matrix was already loaded");
	}

	printdebug("Started reading coordinate sparse matrix from MOSEK");

	R_len_t nzmax;
	MSKintt nrow, ncol;
	errcatch( MSK_getnumanz(task, &nzmax) );		// FIXME: Use MSK_getnumanz64 instead when R_len_t is 'long'
	errcatch( MSK_getnumcon(task, &nrow) );
	errcatch( MSK_getnumvar(task, &ncol) );

	matrix = M_cholmod_allocate_triplet(nrow, ncol, nzmax, 0, CHOLMOD_REAL, &chol);
	cholmod_allocated = true;	initialized = true;

	MSKidxt *pi = static_cast<MSKidxt*>(matrix->i);
	MSKidxt *pj = static_cast<MSKidxt*>(matrix->j);
	MSKrealt *pv = static_cast<MSKrealt*>(matrix->x);
	MSKintt surp[1] = { numeric_cast<MSKintt>(nzmax) };

	// FIXME: 64bit alternative for MSK_getaslicetrip when R_len_t is 'long'??
	errcatch( MSK_getaslicetrip(task, MSK_ACC_VAR, 0, ncol, numeric_cast<MSKintt>(nzmax), surp,
			pi, pj, pv) );

	matrix->nnz = nzmax;
}


void pkgMatrixCOO_type::MOSEK_write(Task_handle &task) {
	if (!initialized) {
		throw msk_exception("Internal error in pkgMatrixCOO_type::MOSEK_write, a matrix was not loaded");
	}

	MSKidxt *pi = static_cast<MSKidxt*>(matrix->i);
	MSKidxt *pj = static_cast<MSKidxt*>(matrix->j);
	MSKrealt *pv = static_cast<MSKrealt*>(matrix->x);

	// FIXME: 64bit alternative for MSK_putaijlist when R_len_t is 'long'??
	MSK_putaijlist(task, numeric_cast<MSKintt>(nnz()), pi, pj, pv);
}


// ------------------------------
// Class pkgMatrixCSC_type
// ------------------------------

CHM_SP pkgMatrixCSC_type::matrix = NULL;
bool pkgMatrixCSC_type::initialized = false;
bool pkgMatrixCSC_type::cholmod_allocated = false;

pkgMatrixCSC_type::~pkgMatrixCSC_type() {
	if (initialized) {
		printdebug("Removing a pkgMatrixCSC_type");

		if (cholmod_allocated) {
			M_cholmod_free_sparse(&matrix, &chol);
		} else {
			delete matrix;
		}
		initialized = false;
	}
}

void pkgMatrixCSC_type::R_read(SEXP val, string name) {
	if (initialized) {
		throw msk_exception("Internal error in pkgMatrixCSC_type::R_read, a matrix was already loaded");
	}

	// Read a column compressed sparse matrix using package Matrix
	if (Matrix_isclass_Csparse(val)) {
		cholmod_allocated = false;

		matrix = new cholmod_sparse; initialized = true;
		M_as_cholmod_sparse(matrix, val, (Rboolean)FALSE, (Rboolean)FALSE);
	}
	else {
		throw msk_exception("Internal error in pkgMatrixCSC_type::R_read: Call to Matrix_isclass_Csparse returned false.");
	}

	// Verify sparse matrix
	if (matrix->itype == CHOLMOD_LONG)
		if (sizeof(MSKlidxt) != sizeof(UF_long))
			throw msk_exception("Matrix from package 'Matrix' has wrong itype");

	if (matrix->xtype != CHOLMOD_REAL)
		throw msk_exception("Matrix from package 'Matrix' has wrong xtype");

	if (matrix->dtype != CHOLMOD_DOUBLE)
		throw msk_exception("Matrix from package 'Matrix' has wrong dtype");

	if (matrix->sorted == false)
		throw msk_exception("Matrix from package 'Matrix' is not sorted");

	if (matrix->packed == false)
		throw msk_exception("Matrix from package 'Matrix' is not packed");
}


void pkgMatrixCSC_type::R_write(SEXP_Handle &val) {
	if (!initialized) {
		throw msk_exception("Internal error in pkgMatrixCSC_type::R_write, a matrix was not loaded");
	}

	val.protect( M_chm_sparse_to_SEXP(matrix, 0, 0, 0, NULL, R_NilValue) );
}


void pkgMatrixCSC_type::MOSEK_read(Task_handle &task) {
	if (initialized) {
		throw msk_exception("Internal error in pkgMatrixCSC_type::MOSEK_read, a matrix was already loaded");
	}

	printdebug("Started reading column compressed sparse matrix from MOSEK");

	R_len_t nzmax;
	MSKintt nrow, ncol;
	errcatch( MSK_getnumanz(task, &nzmax) );		// FIXME: Use MSK_getnumanz64 when R_len_t is 'long'??
	errcatch( MSK_getnumcon(task, &nrow) );
	errcatch( MSK_getnumvar(task, &ncol) );

	matrix = M_cholmod_allocate_sparse(nrow, ncol, nzmax, true, true, 0, CHOLMOD_REAL, &chol);
	cholmod_allocated = true;	initialized = true;

	MSKintt *ptrb = static_cast<MSKintt*>(matrix->p);
	MSKlidxt *sub = static_cast<MSKlidxt*>(matrix->i);
	MSKrealt *val = static_cast<MSKrealt*>(matrix->x);
	MSKintt surp[1] = { numeric_cast<MSKintt>(nzmax) };

	// FIXME: 64bit alternative for MSK_putaijlist when R_len_t is 'long'??
	errcatch( MSK_getaslice(task, MSK_ACC_VAR, 0, ncol, numeric_cast<MSKintt>(nzmax), surp,
			ptrb, ptrb+1, sub, val) );
}


void pkgMatrixCSC_type::MOSEK_write(Task_handle &task) {
	if (!initialized) {
		throw msk_exception("Internal error in pkgMatrixCSC_type::MOSEK_write, a matrix was not loaded");
	}

	MSKlidxt *aptr = static_cast<MSKlidxt*>(matrix->p);
	MSKlidxt *asub = static_cast<MSKlidxt*>(matrix->i);
	MSKrealt *aval = static_cast<MSKrealt*>(matrix->x);

	for (MSKidxt j=0; j < numeric_cast<MSKidxt>(matrix->ncol); ++j)
	{
		/* Input column j of A */
		errcatch( MSK_putavec(task,
					MSK_ACC_VAR, 		/* Input columns of A.*/
					j, 					/* Variable (column) index.*/
					aptr[j+1]-aptr[j],	/* Number of non-zeros in column j.*/
					asub+aptr[j], 		/* Pointer to row indexes of column j.*/
					aval+aptr[j])); 	/* Pointer to Values of column j.*/
	}
}


___RMSK_INNER_NS_END___
