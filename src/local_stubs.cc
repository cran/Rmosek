/*
 * The Matrix Package
 * 2011-08-13	Version 0.9996875-3 	Based on revision 2701		Last release requiring R (>= 2.10.0)	<-- Backported
 * 2010-07-13	Version 0.999375-42		Based on revision 2566		Last release requiring R (>= 2.9.1)		<-- A backport have not been planned
 * 2009-10-06	Version 0.999375-31		Based on revision 2486		Last release requiring R (>= 2.9.0)		<-- A backport have not been planned
 *
 * For any prototype declaration in some namespace (e.g. MSK_LVL1), the linker will
 * first look for a function body in this namespace. If none is found, the parent
 * namespace is searched (e.g. MSK_LVL2), and so on (e.g. MSK_LVL3, MSK_LVL4, and
 * finally the global namespace).
 *
 * This ordered name lookup can be used to overwrite body functionality of the
 * "Matrix_stubs.c" file by placing "overwriteBody.cc" in an innermore namespace.
 * In addition, the prototype declarations of "overwriteHead.h" and "fallbacks.h"
 * need function bodies in the same namespace supplied by "overwriteHead.cc" and
 * "fallbacks.cc".
 */

#include "local_stubs.h"

namespace MSK_LVL4 {
	namespace MSK_LVL3 {
		namespace MSK_LVL2 {
			namespace MSK_LVL1 {
				#include "compatibility/overwriteHead.cc"
			}
			#include "compatibility/overwriteBody.cc"
		}
		#include "Matrix_stubs.c"
	}
	#include "compatibility/fallbacks.cc"
}
