#ifndef RMSK_MSGSYSTEM_H_
#define RMSK_MSGSYSTEM_H_

#include "mosek.h"
#include "rmsk_namespace.h"

#include <R_ext/Arith.h>
#include <stdexcept>
#include <string>
#include <sstream>


___RMSK_INNER_NS_START___

extern double mosek_interface_verbose;
extern int    mosek_interface_warnings;
extern bool   mosek_interface_signal_caught;
extern bool   mosek_interface_termination_success;

// ------------------------------
// PRINTING SYSTEM
// ------------------------------
enum verbosetype { typeERROR=1, typeMOSEK=2, typeWARNING=3, typeINFO=4, typeDEBUG=50, typeALL=100 };

void printoutput(std::string str, verbosetype strtype);
void printerror(std::string str);
void printwarning(std::string str);
void printinfo(std::string str);
void printdebug(std::string str);
void printdebugdata(std::string str);
void printpendingmsg();


// ------------------------------
// RESPONSE AND EXCEPTION SYSTEM
// ------------------------------
struct msk_response {
public:
	double code;
	std::string msg;

	msk_response(MSKrescodee);
	msk_response(const std::string &msg) : code(R_NaN), msg(msg) {}
	msk_response(double code, const std::string &msg) : code(code), msg(msg) {}
};

struct msk_exception : public std::runtime_error {
public:
	const double code;

	// Used for MOSEK errors with response codes
	msk_exception(const msk_response &res) : std::runtime_error(res.msg), code(res.code) {}

	// Used for interface errors without response codes
	msk_exception(const std::string &msg) : std::runtime_error(msg), code(R_NaN) {}

	msk_response getresponse() const {
		return msk_response(code, what());
	}
};

void errcatch(MSKrescodee r, std::string str);
void errcatch(MSKrescodee r);


// ------------------------------
// BASIC TYPE MANIPULATION
// ------------------------------
void strtoupper(std::string&);
bool ispos(double);
int scalar2int(double);

template <class T>
std::string tostring(T val)
{
	std::ostringstream ss;
	ss << val;
	return ss.str();
}


___RMSK_INNER_NS_END___

#endif /* RMSK_MSGSYSTEM_H_ */
