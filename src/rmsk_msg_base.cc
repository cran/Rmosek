#define R_NO_REMAP
#include "rmsk_msg_base.h"

#include <R_ext/Print.h>
#include <math.h>
#include <vector>
#include <limits>


___RMSK_INNER_NS_START___
using std::vector;
using std::string;
using std::numeric_limits;


// Global initialization
double mosek_interface_verbose  = NAN;				// Declare messages as pending
int    mosek_interface_warnings = 0;				// Start warning count from zero
bool   mosek_interface_signal_caught = false;		// Indicate whether the CTRL+C interrupt has been registered
bool   mosek_interface_termination_success = true;	// Indicate whether the interfaced terminated properly


// ------------------------------
// PRINTING SYSTEM
// ------------------------------
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
	if (mosek_pendingmsg_str.size() != mosek_pendingmsg_type.size()) {
		delete_all_pendingmsg();
		throw msk_exception("Error in handling of pending messages");
	}

	for (vector<string>::size_type i=0; i<mosek_pendingmsg_str.size(); i++) {
		printoutput(mosek_pendingmsg_str[i], mosek_pendingmsg_type[i]);
	}
	delete_all_pendingmsg();
}

void delete_all_pendingmsg() {
	mosek_pendingmsg_str.clear();
	mosek_pendingmsg_type.clear();
}


// ------------------------------
// BASIC TYPE MANIPULATION
// ------------------------------

void strtoupper(string &str) {
	string::iterator i = str.begin();
	string::iterator end = str.end();

	while (i != end) {
	    *i = toupper(static_cast<int>(*i));
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
		return numeric_limits<int>::max();

	int retval = static_cast<int>(scalar);
	double err = scalar - retval;

	if (err <= -1e-6 || 1e-6 <= err)
		printwarning("A scalar with fractional value was truncated to an integer.");

	return retval;
}

___RMSK_INNER_NS_END___
