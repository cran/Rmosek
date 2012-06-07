#ifndef RMSK_MSG_MOSEK_H_
#define RMSK_MSG_MOSEK_H_

#include "rmsk_msg_base.h"
#include "rmsk_namespace.h"

#include "compatibility/cpp98_mosek.h"

#include <string>


___RMSK_INNER_NS_START___

// ------------------------------
// RESPONSE AND EXCEPTION SYSTEM
// ------------------------------

void errcatch(MSKrescodee r, std::string str);
void errcatch(MSKrescodee r);

msk_response get_msk_response(MSKrescodee r);

___RMSK_INNER_NS_END___

#endif /* RMSK_MSG_MOSEK_H_ */
