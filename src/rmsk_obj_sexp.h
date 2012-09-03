#ifndef RMSK_OBJ_SEXP_H_
#define RMSK_OBJ_SEXP_H_

#include "rmsk_msg_base.h"
#include "rmsk_namespace.h"

#include <Rinternals.h>
#include <string>


___RMSK_INNER_NS_START___

// TODO: Replace these typedefs..
typedef SEXP SEXP_LIST;
typedef SEXP SEXP_NUMERIC;
typedef SEXP SEXP_CHARACTER;


// ------------------------------
// COMMON SEXP RESOURCES
// ------------------------------

class SEXP_Handle {
private:
  bool initialized;

  PROTECT_INDEX object_ipx;
  SEXP object;

  // Overwrite copy constructor (assignment operator) and provide no implementation
  SEXP_Handle(const SEXP_Handle& that);
  SEXP_Handle& operator=(const SEXP_Handle& that);

public:
  // Allow implicit conversion to SEXP
  operator SEXP() { return object; }

  SEXP_Handle();                // Protect an empty object on protection stack
  SEXP_Handle(SEXP input);      // Protect input object on protection stack
  void protect(SEXP object);    // Change protection to input object (stack position intact)

  ~SEXP_Handle();               // Pop object from protection stack
};


class SEXP_NamedVector {
// TODO: Implement as a child of SEXP_Vector?

private:
  bool initialized;

  SEXP_Handle items;
  SEXP_Handle names;
  R_len_t maxsize;

  // Overwrite copy constructor (assignment operator) and provide no implementation
  SEXP_NamedVector(const SEXP_NamedVector& that);
  SEXP_NamedVector& operator=(const SEXP_NamedVector& that);

public:
  // Simple constructor and implicit SEXP conversion
  SEXP_NamedVector() : initialized(false), maxsize(0) { }
  operator SEXP() { return items; }

  // Initialise vector with 'maxsize' capacity
  void initVEC(R_len_t maxsize);

  // The number of items added to the vector
  R_len_t size();

  // Various types can be added to the vector
  void pushback(std::string name, SEXP item);
  void pushback(std::string name, char* item);
  void pushback(std::string name, std::string item);
  void pushback(std::string name, double item);
  void pushback(std::string name, int item);

  // Replace an item with a new one (only SEXP type implemented)
  void set(std::string name, SEXP item, int pos);
};


class SEXP_Vector {
private:
  bool initialized;
  bool static_size;

  SEXPTYPE itemstype;
  SEXP_Handle items;
  R_len_t maxsize;

  // Overwrite copy constructor (assignment operator) and provide no implementation
  SEXP_Vector(const SEXP_Vector& that);
  SEXP_Vector& operator=(const SEXP_Vector& that);

public:
  // Simple constructor and implicit SEXP conversion
  SEXP_Vector() : initialized(false), maxsize(0), static_size(true), itemstype(VECSXP) { }
  operator SEXP() { return items; }

  // Initialise vector with type and capacity
  //   static_size=true   =>  Array-like behaviour
  //   static_size=false  =>  Stack-like behaviour
  void initSTR(R_len_t maxsize, bool static_size=true);
  void initREAL(R_len_t maxsize, bool static_size=true);
  void initINT(R_len_t maxsize, bool static_size=true);
  void initVEC(R_len_t maxsize, bool static_size=true);

  // Stack-like: The number of items added to the vector
  // Array-like: The capacity of the vector
  R_len_t size();

  // Stack-like: Various types can be added to the vector
  // Array-like: Not allowed
  void pushback(SEXP item);
  void pushback(char* item);
  void pushback(std::string item);
  void pushback(double item);
  void pushback(int item);

  // Stack-like: Replace an item with a new one (only SEXP type implemented)
  // Array-like: Insert an item in the vector (only SEXP type implemented)
  void set(SEXP item, int pos);
};

___RMSK_INNER_NS_END___

#endif /* RMSK_OBJ_SEXP_H_ */
