#define R_NO_REMAP
#include "rmsk_msg_mosek.h"

___RMSK_INNER_NS_START___
using std::string;


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

msk_response get_msk_response(MSKrescodee r) {
  return msk_response(static_cast<double>(r), msk_responsecode2text(r));
}

// This function translates MOSEK response codes into msk_exceptions.
void errcatch(MSKrescodee r, string str) {
  if (r != MSK_RES_OK) {
    if (!str.empty())
      printerror(str);

    throw msk_exception(get_msk_response(r));
  }
}
void errcatch(MSKrescodee r) {
  errcatch(r, "");
}

___RMSK_INNER_NS_END___
