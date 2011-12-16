#ifndef RMSK_OBJ_ARGUMENTS_H_
#define RMSK_OBJ_ARGUMENTS_H_

#include "rmsk_msg_base.h"
#include "rmsk_namespace.h"

#include "rmsk_obj_sexp.h"
#include "rmsk_obj_matrices.h"
#include "rmsk_obj_constraints.h"
#include "rmsk_obj_mosek.h"

#include <string>
#include <vector>
#include <memory>

___RMSK_INNER_NS_START___


class options_type {
private:
	bool initialized;

public:
	// Recognised options arguments in R
	// TODO: Upgrade to new C++11 initialisers
	static const struct R_ARGS_type {

		std::vector<std::string> arglist;
		const std::string useparam;
		const std::string usesol;
		const std::string verbose;
		const std::string writebefore;
		const std::string writeafter;
		const std::string matrixformat;

		R_ARGS_type() :
			useparam("useparam"),
			usesol("usesol"),
			verbose("verbose"),
			writebefore("writebefore"),
			writeafter("writeafter"),
			matrixformat("matrixformat")
		{
			std::string temp[] = {useparam, usesol, verbose, writebefore, writeafter, matrixformat};
			arglist = std::vector<std::string>(temp, temp + sizeof(temp)/sizeof(std::string));
		}
	} R_ARGS;


	// Data definition (intentionally kept close to R types)
	bool useparam;
	bool usesol;
	double verbose;
	std::string	writebefore;
	std::string	writeafter;
	matrixformat_enum matrixformat;

	// Set default values
	options_type();

	// Read options from R (write not implemented)
	void R_read(SEXP_LIST object);
};


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
		std::vector<std::string> arglist;
		const std::string sense;
		const std::string c;
		const std::string c0;
		const std::string A;
		const std::string blc;
		const std::string buc;
		const std::string blx;
		const std::string bux;
		const std::string cones;
		const std::string intsub;
		const std::string sol;
		const std::string iparam;
		const std::string dparam;
		const std::string sparam;
//		const std::string options;

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
			std::string temp[] = {sense, c, c0, A, blc, buc, blx, bux, cones, intsub, sol, iparam, dparam, sparam}; //options
			arglist = std::vector<std::string>(temp, temp + sizeof(temp)/sizeof(std::string));
		}

	} R_ARGS;

	//
	// Data definition (intentionally kept close to R types)
	//
	R_len_t	numnz;
	MSKintt	numcon;
	MSKintt	numvar;
	MSKintt	numintvar;
	MSKintt	numcones;

	MSKobjsensee	sense;
	SEXP_Handle 	c;
	double 			c0;

	std::auto_ptr<matrix_type> A;
	SEXP_Handle 	blc;
	SEXP_Handle 	buc;
	SEXP_Handle 	blx;
	SEXP_Handle 	bux;
	conicSOC_type 	cones;
	SEXP_Handle 	intsub;
	SEXP_Handle 	initsol;
	SEXP_Handle 	iparam;
	SEXP_Handle 	dparam;
	SEXP_Handle 	sparam;
	options_type 	options;

	// Set default values
	problem_type();

	// Read and write problem description from and to R
	void R_read(SEXP_LIST object);
	void R_write(SEXP_NamedVector &prob_val);

	// Read and write problem description from and to MOSEK
	void MOSEK_read(Task_handle &task);
	void MOSEK_write(Task_handle &task);
};

___RMSK_INNER_NS_END___

#endif /* RMSK_OBJ_ARGUMENTS_H_ */
