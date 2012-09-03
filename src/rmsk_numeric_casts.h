#ifndef RMSK_NUMERIC_CASTS_H_
#define RMSK_NUMERIC_CASTS_H_

#include "rmsk_namespace.h"
#include "rmsk_msg_base.h"

#include "compatibility/cpp98_mosek.h"

#include <limits>

___RMSK_INNER_NS_START___

#define RMSK_OUT_OF_BOUNDS_MSG "Numeric value out of bounds"

template<class OUT, class IN>
OUT numeric_cast(IN var) {

  // TODO: Keep this updated!
  typedef MSKint64t LARGEST_SIGNED_INTEGRAL;
  typedef MSKuint64t LARGEST_UNSIGNED_INTEGRAL;

  const bool osigned = std::numeric_limits<OUT>::is_signed;
  const OUT omin = std::numeric_limits<OUT>::min();
  const OUT omax = std::numeric_limits<OUT>::max();

  if (osigned) {

    if (var >= static_cast<IN>(0)) {
      // var >= 0 >= omin and omax >= 0

      LARGEST_UNSIGNED_INTEGRAL var2 = static_cast<LARGEST_UNSIGNED_INTEGRAL>(var);
      LARGEST_UNSIGNED_INTEGRAL omax2 = static_cast<LARGEST_UNSIGNED_INTEGRAL>(omax);

      if (var2 <= omax2) {
        return static_cast<OUT>(var);
      } else {
        throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);
      }

    } else {
      // var <= 0 <= omax and omin <= 0

      LARGEST_SIGNED_INTEGRAL var2 = static_cast<LARGEST_SIGNED_INTEGRAL>(var);
      LARGEST_SIGNED_INTEGRAL omin2 = static_cast<LARGEST_SIGNED_INTEGRAL>(omin);

      if (omin2 <= var2) {
        return static_cast<OUT>(var);
      } else {
        throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);
      }
    }

  } else {

    if (var >= static_cast<IN>(0)) {
      // var >= 0 >= omin and omax >= 0

      LARGEST_UNSIGNED_INTEGRAL var2 = static_cast<LARGEST_UNSIGNED_INTEGRAL>(var);
      LARGEST_UNSIGNED_INTEGRAL omax2 = static_cast<LARGEST_UNSIGNED_INTEGRAL>(omax);

      if (var2 <= omax2) {
        return static_cast<OUT>(var);
      } else {
        throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);
      }

    } else {
      throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);
    }

  }
}


/*
 * Solution using template specification had better performance, but
 * could not be ported properly:
 *   if int, long and long long is specified: "warning: use of long long"
 *   if int and long is specified: "warning: missing match for int64_t (windows)"
 *   if int32_t and int64_t is specified: "error: missing match for long (linux)"
 *   if int32_t, long and int64_t is specified: "error: redefinition of long == int64_t (linux)"
 *
//
//template<class OUT, class IN>
//struct numeric_cast_type;
//
//template<class OUT, class IN>
//OUT numeric_cast(IN var) { return numeric_cast_type<OUT,IN>::numeric_cast(var); }
//
//
////
//// IDENTITIES
////
//template<class T>
//struct numeric_cast_type <T, T> {
//	static T numeric_cast(T var) {
//		return var;
//	}
//};
//
//
//
// * Template specification solution:
// * (if int, long and long long is specified: "warning: use of long long")
// * (if int and long is specified: "warning: missing match for int64_t (windows)")
// * (if int32_t and int64_t is specified: "error: missing match for long (linux)")
// * (if int32_t, long and int64_t is specified: "error: redefinition of long, int64_t (linux)")
//
////
////
//
////----------------------------------------
//// ----------------------------------------
//// sign-type equal, bitsize different
//// ----------------------------------------
////----------------------------------------
//
//#define numeric_cast_type_upcast_samesign(OUT, IN) 		\
//		template<>										\
//		struct numeric_cast_type <OUT, IN> {			\
//			static OUT numeric_cast(IN var) {			\
//				return var;								\
//			}											\
//		}
//
//#define numeric_cast_type_downcast_unsigned(OUT, IN) 							\
//		template<>																\
//		struct numeric_cast_type <OUT, IN> {									\
//			static OUT numeric_cast(IN var) {									\
//				if ( var <= static_cast<IN>(std::numeric_limits<OUT>::max()) )	\
//					return var;													\
//				else															\
//					throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);				\
//			}																	\
//		}
//
//#define numeric_cast_type_downcast_signed(OUT, IN) 																							\
//		template<>																															\
//		struct numeric_cast_type <OUT, IN> {																								\
//			static OUT numeric_cast(IN var) {																								\
//				if ( static_cast<IN>(std::numeric_limits<OUT>::min()) <= var && var <= static_cast<IN>(std::numeric_limits<OUT>::max()) )	\
//					return var;																												\
//				else																														\
//				throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);																				\
//			}																																\
//		}
//
////
//// UNSIGNED X <- UNSIGNED Y
////
//numeric_cast_type_upcast_samesign(uint64_t, uint32_t);
////#if UINT64_MAX != Uint64_t_MAX
////numeric_cast_type_upcast_samesign(uint64_t int64_t, uint32_t);
////numeric_cast_type_upcast_samesign(uint64_t int64_t, uint64_t);
////#endif
//
//numeric_cast_type_downcast_unsigned(uint32_t, uint64_t);
////#if UINT64_MAX != Uint64_t_MAX
////numeric_cast_type_downcast_unsigned(uint32_t, uint64_t int64_t);
////numeric_cast_type_downcast_unsigned(uint64_t, uint64_t int64_t);
////#endif
////
////
//// SIGNED X <- SIGNED Y
////
//numeric_cast_type_upcast_samesign(int64_t, int32_t);
////#if UINT64_MAX != Uint64_t_MAX
////numeric_cast_type_upcast_samesign(int64_t int64_t, int32_t);
////numeric_cast_type_upcast_samesign(int64_t int64_t, int64_t);
////#endif
//
//numeric_cast_type_downcast_signed(int32_t, int64_t);
////#if UINT64_MAX != Uint64_t_MAX
////numeric_cast_type_downcast_signed(int32_t, int64_t int64_t);
////numeric_cast_type_downcast_signed(int64_t, int64_t int64_t);
////#endif
//
////----------------------------------------
//// ----------------------------------------
//// sign-type different, bitsize equal
//// ----------------------------------------
////----------------------------------------
//
//#define numeric_cast_type_samesize_remove_sign(OUT, IN) 		\
//		template<>												\
//		struct numeric_cast_type <OUT, IN> {					\
//			static OUT numeric_cast(IN var) {					\
//				if ( 0 <= var )									\
//					return var;									\
//				else											\
//				throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);	\
//			}													\
//		}
//
//#define numeric_cast_type_samesize_append_sign(OUT, IN) 						\
//		template<>																\
//		struct numeric_cast_type <OUT, IN> {									\
//			static OUT numeric_cast(IN var) {									\
//				if ( var <= static_cast<IN>(std::numeric_limits<OUT>::max()) )	\
//					return var;													\
//				else															\
//				throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);					\
//			}																	\
//		}
//
////
//// UNSIGNED X <- SIGNED X
////
//numeric_cast_type_samesize_remove_sign(uint32_t, int32_t);
//numeric_cast_type_samesize_remove_sign(uint64_t, int64_t);
////#if UINT64_MAX != Uint64_t_MAX
////numeric_cast_type_samesize_remove_sign(uint64_t int64_t, int64_t int64_t);
////#endif
//
////
//// SIGNED X <- UNSIGNED X
////
//numeric_cast_type_samesize_append_sign(int32_t, uint32_t);
//numeric_cast_type_samesize_append_sign(int64_t, uint64_t);
////#if UINT64_MAX != Uint64_t_MAX
////numeric_cast_type_samesize_append_sign(int64_t int64_t, uint64_t int64_t);
////#endif
//
////----------------------------------------
//// ----------------------------------------
//// sign-type different, bitsize different
//// ----------------------------------------
////----------------------------------------
//
//#define numeric_cast_type_diffsize_remove_sign_path(OUT, UNSIGNED_IN, IN)					\
//		template<>																		\
//		struct numeric_cast_type <OUT, IN> {											\
//			static OUT numeric_cast(IN var) {											\
//				return numeric_cast_type<OUT, UNSIGNED_IN>::numeric_cast(				\
//							numeric_cast_type<UNSIGNED_IN, IN>::numeric_cast( var ) );	\
//			}																			\
//		}
//
//#define numeric_cast_type_diffsize_remove_sign(OUT, IN)									\
//		numeric_cast_type_diffsize_remove_sign_path(OUT, unsigned IN, IN)
//
//#define numeric_cast_type_diffsize_append_sign_path(OUT, UNSIGNED_OUT, IN)					\
//		template<>																		\
//		struct numeric_cast_type <OUT, IN> {											\
//			static OUT numeric_cast(IN var) {											\
//				return numeric_cast_type<OUT, UNSIGNED_OUT>::numeric_cast(				\
//							numeric_cast_type<UNSIGNED_OUT, IN>::numeric_cast( var ) );	\
//			}																			\
//		}
//
//#define numeric_cast_type_diffsize_append_sign(OUT, IN)									\
//		numeric_cast_type_diffsize_append_sign_path(OUT, unsigned OUT, IN)
//
////
//// UNSIGNED X <- SIGNED Y
////
//numeric_cast_type_diffsize_remove_sign_path(uint64_t, uint32_t, int32_t);
////#if UINT64_MAX != Uint64_t_MAX
////numeric_cast_type_diffsize_remove_sign(uint64_t int64_t, int32_t);
////numeric_cast_type_diffsize_remove_sign(uint64_t int64_t, int64_t);
////#endif
//
//numeric_cast_type_diffsize_remove_sign_path(uint32_t, uint64_t, int64_t);
////#if UINT64_MAX != Uint64_t_MAX
////numeric_cast_type_diffsize_remove_sign(uint32_t, int64_t int64_t);
////numeric_cast_type_diffsize_remove_sign(uint64_t, int64_t int64_t);
////#endif
//
////
//// SIGNED X <- UNSIGNED Y
////
//numeric_cast_type_diffsize_append_sign_path(int64_t, uint64_t, uint32_t);
////#if UINT64_MAX != Uint64_t_MAX
////numeric_cast_type_diffsize_append_sign(int64_t int64_t, uint32_t);
////numeric_cast_type_diffsize_append_sign(int64_t int64_t, uint64_t);
////#endif
//
//numeric_cast_type_diffsize_append_sign_path(int32_t, uint32_t, uint64_t);
////#if UINT64_MAX != Uint64_t_MAX
////numeric_cast_type_diffsize_append_sign(int32_t, uint64_t int64_t);
////numeric_cast_type_diffsize_append_sign(int64_t, uint64_t int64_t);
////#endif
*/



___RMSK_INNER_NS_END___

#endif /* RMSK_NUMERIC_CASTS_H_ */
