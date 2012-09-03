#ifndef RMSK_MSG_BASE_H_
#define RMSK_MSG_BASE_H_

#include "rmsk_namespace.h"

#include <R_ext/Arith.h>
#include <stdexcept>
#include <string>
#include <sstream>
#include <memory>
#include <stdio.h>


___RMSK_INNER_NS_START___

// Global variables
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
void printpendingmsg(std::string header);
void delete_all_pendingmsg();


// ------------------------------
// RESPONSE AND EXCEPTION SYSTEM
// ------------------------------
struct msk_response {
  double code;
  std::string msg;

  msk_response(const std::string &msg) : code(R_NaN), msg(msg) {}
  msk_response(double code, const std::string &msg) : code(code), msg(msg) {}
};

struct msk_exception : public std::runtime_error {
  const double code;

  // Used for MOSEK errors with response codes
  msk_exception(const msk_response &res) : std::runtime_error(res.msg), code(res.code) {}

  // Used for interface errors without response codes
  msk_exception(const std::string &msg) : std::runtime_error(msg), code(R_NaN) {}

  msk_response getresponse() const {
    return msk_response(code, what());
  }
};


// ------------------------------
// BASIC TYPE MANIPULATION
// ------------------------------
void strtoupper(std::string& str);
int scalar2int(double scalar);
bool ISPOS(double x);

template <class T>
std::string tostring(T val)
{
  std::ostringstream ss;
  ss << val;
  return ss.str();
}

// TODO: Upgrade to new C++11 unique_ptr
template<class T>
class auto_array {
private:
  T* arr;

  // Overwrite copy constructor (assignment operator) and provide no implementation
  auto_array(const auto_array& that);
  auto_array& operator=(const auto_array& that);

  void init(T *obj) {
    arr = obj;
  }

public:
  operator T*() { return arr; }

  explicit auto_array(T *obj = NULL) { init(obj); }

  ~auto_array() {
    if (arr != NULL) {
      delete[] arr;
    }
  }

  void protect(T *obj) {
    this->~auto_array();
    init(obj);
  }
};

class FILE_handle {
private:
  FILE * f;

  // Overwrite copy constructor (assignment operator) and provide no implementation
  FILE_handle(const FILE_handle& that);
  FILE_handle& operator=(const FILE_handle& that);

  void init(FILE *obj) {
    f = obj;
  }

public:
  operator FILE*() { return f; }

  explicit FILE_handle(FILE *obj = NULL) { init(obj); }

  ~FILE_handle() {
    if (f != NULL) {
      fclose(f);
    }
  }

  void protect(FILE *obj) {
    this->~FILE_handle();
    init(obj);
  }
};


___RMSK_INNER_NS_END___

#endif /* RMSK_MSG_BASE_H_ */
