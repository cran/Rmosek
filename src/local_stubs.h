/*
 * The Matrix Package
 * 2011-08-13	Version 0.9996875-3 	Based on revision 2701		Last release requiring R (>= 2.10.0)	<-- Backported
 * 2010-07-13	Version 0.999375-42		Based on revision 2566		Last release requiring R (>= 2.9.1)
 * 2009-10-06	Version 0.999375-31		Based on revision 2486		Last release requiring R (>= 2.9.0)
 *
 * The rmosek package works in the innermost namespace (MSK_LVL1). For any function
 * used by the rmosek package, the compiler will look for prototype declarations in
 * this namespace. If none is found, the parent namespace is searched (MSK_LVL2),
 * then (MSK_LVL3), then (MSK_LVL4), and finally the global namespace.
 *
 * This ordered name lookup can be used to overwrite header functionality of the
 * "Matrix.h" file by placing "overwriteHead.h" in an innermore namespace. It is
 * also possible to provide fallback header functionality in case something is
 * missing in "Matrix.h", by placing "fallbacks.h" in an outermore namespace.
 *
 * Notice that because of Koenig Lookup, the prototype declaration for the return
 * and argument typenames of a function makes a difference. If the rmosek package
 * calls a function with argument SEXP (and SEXP was declared in MSK_LVL2), then
 * even if the function call was made from within MSK_LVL1 and there exist a
 * prototype declaration matching the function call in MSK_LVL1, the namespace
 * MSK_LVL2 would still be searched which could lead to ambiguity. This is why all
 * typenames are defined in the global namespace prior to namespace declarations,
 * and why the fallback functions are not in the global namespace.
 */

#include <R.h>
#include <Rdefines.h>
#include <Rconfig.h>
#include <Rversion.h>
#include "cholmod.h"

namespace MSK_LVL4 {
	namespace MSK_LVL3 {
		namespace MSK_LVL2 {
			namespace MSK_LVL1 {
				#include "compatibility/overwriteHead.h"
			}
			#include "Matrix.h"
		}
	}
	#include "compatibility/fallbacks.h"
}


