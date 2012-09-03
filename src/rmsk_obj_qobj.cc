#define R_NO_REMAP
#include "rmsk_obj_qobj.h"

#include "rmsk_utils_sexp.h"


___RMSK_INNER_NS_START___

void qobj_type::R_read(SEXP_LIST object) {
  if (initialized) {
    throw msk_exception("Internal error in qobj_type::R_read, a quadratic objective matrix was already loaded");
  }

  printdebug("Started reading 'qobj' matrix from R");
  if (isEmpty(object)) {

    // Something with Rf_length zero:
    qobj_subi.protect(R_NilValue);
    qobj_subj.protect(R_NilValue);
    qobj_val.protect(R_NilValue);

  } else {

    SEXP_Handle qobj(object);

    list_seek_Numeric(qobj_val, qobj, R_ARGS.v);      numqobjnz = Rf_length(qobj_val);
    list_seek_Numeric(qobj_subi, qobj, R_ARGS.i);     validate_Numeric(qobj_subi, R_ARGS.i, numqobjnz);
    list_seek_Numeric(qobj_subj, qobj, R_ARGS.j);     validate_Numeric(qobj_subj, R_ARGS.j, numqobjnz);

    validate_NamedVector(qobj, "qobj", R_ARGS.arglist);
  }

  initialized = true;
}

void qobj_type::R_write(SEXP_Handle &val) {
  if (!initialized) {
    throw msk_exception("Internal error in conicSOC_type::R_write, no 'cones' matrix loaded");
  }

  printdebug("Started writing 'qobj' to R");

  SEXP_NamedVector qobj;    qobj.initVEC(R_ARGS.arglist.size());    val.protect(qobj);

  qobj.pushback(R_ARGS.i, qobj_subi);
  qobj.pushback(R_ARGS.j, qobj_subj);
  qobj.pushback(R_ARGS.v, qobj_val);
}

void qobj_type::MOSEK_read(Task_handle &task) {
  if (initialized) {
    throw msk_exception("Internal error in conicSOC_type::MOSEK_read, a 'cones' matrix was already loaded");
  }

  printdebug("Started reading 'qobj' matrix from MOSEK");
  errcatch( MSK_getnumqobjnz64(task, &numqobjnz) );

  if (numqobjnz > 0) {
    SEXP_Vector subi;    subi.initINT(numeric_cast<R_len_t>(numqobjnz));    qobj_subi.protect(subi);
    MSKlidxt *psubi = INTEGER(subi);

    SEXP_Vector subj;    subj.initINT(numeric_cast<R_len_t>(numqobjnz));    qobj_subj.protect(subj);
    MSKlidxt *psubj = INTEGER(subj);

    SEXP_Vector val;     val.initREAL(numeric_cast<R_len_t>(numqobjnz));    qobj_val.protect(val);
    MSKrealt *pval = REAL(val);

    MSKintt surp[1] = { numeric_cast<MSKintt>(numqobjnz) };

    printdebug("Start Calling MSK_getqobj");

    MSKintt numqobjnz_again;

    errcatch( MSK_getqobj(task,
      numeric_cast<MSKintt>(numqobjnz),
      surp,
      &numqobjnz_again,
      psubi,
      psubj,
      pval));

    for (MSKidxt k=0; k < numqobjnz; k++) {
      psubi[k] += 1;
      psubj[k] += 1;
    }

    printdebug("Finished Calling MSK_getqobj");
  }

  initialized = true;
}

void qobj_type::MOSEK_write(Task_handle &task) {
  if (!initialized) {
    throw msk_exception("Internal error in conicSOC_type::MOSEK_write, no 'cones' matrix loaded");
  }

  printdebug("Started writing 'qobj' matrix to MOSEK");

  if (numqobjnz > 0)
  {
    MSKintt numqonz = numeric_cast<MSKintt>(Rf_length(qobj_val));
    auto_array<MSKidxt> msksubi(new MSKidxt[numqonz]);
    auto_array<MSKidxt> msksubj(new MSKidxt[numqonz]);

    for (MSKidxt k=0; k < numqonz; k++) {
      msksubi[k] = INTEGER_ELT(qobj_subi,k) - 1;
      msksubj[k] = INTEGER_ELT(qobj_subj,k) - 1;
    }

    // Append the quadratic objective matrix
    errcatch( MSK_putqobj(task,
        numqonz,                  /* number of non-zero elements */
        msksubi,                  /* triplet row indexes */
        msksubj,                  /* triplet column indexes */
        REAL(qobj_val)) );        /* triplet values */
  }
}

___RMSK_INNER_NS_END___
