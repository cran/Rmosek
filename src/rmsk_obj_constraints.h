#ifndef RMSK_OBJ_CONSTRAINTS_H_
#define RMSK_OBJ_CONSTRAINTS_H_

#include "rmsk_msg_system.h"
#include "rmsk_namespace.h"

#include "rmsk_obj_sexp.h"
#include "rmsk_obj_mosek.h"

#include <string>
#include <vector>

___RMSK_INNER_NS_START___

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
	int numcones;
	SEXP_Handle conevec;

	// Simple construction and destruction
	conicSOC_type() : initialized(false) {}
	~conicSOC_type() {}

	// Read and write matrix from and to R
	void R_read(SEXP_LIST object);
	void R_write(SEXP_Handle &val);

	// Read and write matrix from and to MOSEK
	void MOSEK_read(Task_handle &task);
	void MOSEK_write(Task_handle &task);
};

___RMSK_INNER_NS_END___

#endif /* RMSK_OBJ_CONSTRAINTS_H_ */
