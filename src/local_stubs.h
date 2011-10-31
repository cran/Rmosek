/*
 * The Matrix Package
 *
 * 2011-08-13	Version 0.9996875-3 	Based on revision 2701		Last release requiring R (>= 2.10.0)	<-- Backported
 * 2010-07-13	Version 0.999375-42		Based on revision 2566		Last release requiring R (>= 2.9.1)
 * 2009-10-06	Version 0.999375-31		Based on revision 2486		Last release requiring R (>= 2.9.0)
 *
 * The Rmosek package works in the innermost namespace (MSK1). For any function
 * used by the Rmosek package, the compiler will look for prototype declarations in
 * this namespace. If none is found, the parent namespace is searched (MSK2),
 * then (MSK3), then (MSK4), and finally the global namespace.
 *
 * This ordered name lookup can be used to overwrite header functionality of the
 * "Matrix.h" file by placing "overwriteHead.h" in an innermore namespace. It is
 * also possible to provide fallback header functionality in case something is
 * missing in "Matrix.h", by placing "fallbacks.h" in an outermore namespace.
 *
 * Notice that because of Koenig Lookup, the prototype declaration for the return
 * and argument typenames of a function makes a difference. If the Rmosek package
 * calls a function with argument SEXP (and SEXP was declared in MSK2), then
 * even if the function call was made from within MSK1 and there exist a
 * prototype declaration matching the function call in MSK1, the namespace
 * MSK2 would still be searched which could lead to ambiguity. This is why all
 * typenames are defined in the global namespace prior to namespace declarations,
 * and why the fallback functions are not in the global namespace.
 */

#ifndef RMSK_LOCAL_STUBS_H_
#define RMSK_LOCAL_STUBS_H_

#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rdefines.h>
#include <Rconfig.h>
#include <Rversion.h>
#include "cholmod.h"

#include "compatibility/pkgMatrixVersion.h"

namespace MSK4 {
	namespace MSK3 {
		namespace MSK2 {
			namespace MSK1 {
				#include "compatibility/overwriteHead.h"
			}
			#include "Matrix.h"
		}
	}
	#include "compatibility/fallbacks.h"
}

#endif /* RMSK_LOCAL_STUBS_H_ */
