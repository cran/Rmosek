#ifndef RMSK_NUMERIC_CASTS_H_
#define RMSK_NUMERIC_CASTS_H_

#include "rmsk_namespace.h"
#include "rmsk_msg_base.h"

#include <limits>

___RMSK_INNER_NS_START___


template<class OUT, class IN>
struct numeric_cast_type;

template<class OUT, class IN>
OUT numeric_cast(IN var) { return numeric_cast_type<OUT,IN>::numeric_cast(var); }

#define RMSK_OUT_OF_BOUNDS_MSG "Numeric value out of bounds"


//----------------------------------------
// ----------------------------------------
//  sign-type equal, bitsize equal
// ----------------------------------------
//----------------------------------------

//
// IDENTITIES
//
template<class T>
struct numeric_cast_type <T, T> {
	static T numeric_cast(T var) {
		return var;
	}
};



//----------------------------------------
// ----------------------------------------
// sign-type equal, bitsize different
// ----------------------------------------
//----------------------------------------

#define numeric_cast_type_upcast_samesign(OUT, IN) 		\
		template<>										\
		struct numeric_cast_type <OUT, IN> {			\
			static OUT numeric_cast(IN var) {			\
				return var;								\
			}											\
		}

#define numeric_cast_type_downcast_unsigned(OUT, IN) 							\
		template<>																\
		struct numeric_cast_type <OUT, IN> {									\
			static OUT numeric_cast(IN var) {									\
				if ( var <= static_cast<IN>(std::numeric_limits<OUT>::max()) )	\
					return var;													\
				else															\
					throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);				\
			}																	\
		}

#define numeric_cast_type_downcast_signed(OUT, IN) 																							\
		template<>																															\
		struct numeric_cast_type <OUT, IN> {																								\
			static OUT numeric_cast(IN var) {																								\
				if ( static_cast<IN>(std::numeric_limits<OUT>::min()) <= var && var <= static_cast<IN>(std::numeric_limits<OUT>::max()) )	\
					return var;																												\
				else																														\
				throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);																				\
			}																																\
		}

//
// UNSIGNED X <- UNSIGNED Y
//
numeric_cast_type_upcast_samesign(unsigned long, unsigned int);
numeric_cast_type_upcast_samesign(unsigned long long, unsigned int);
numeric_cast_type_upcast_samesign(unsigned long long, unsigned long);

numeric_cast_type_downcast_unsigned(unsigned int, unsigned long);
numeric_cast_type_downcast_unsigned(unsigned int, unsigned long long);
numeric_cast_type_downcast_unsigned(unsigned long, unsigned long long);

//
// SIGNED X <- SIGNED Y
//
numeric_cast_type_upcast_samesign(long, int);
numeric_cast_type_upcast_samesign(long long, int);
numeric_cast_type_upcast_samesign(long long, long);

numeric_cast_type_downcast_signed(int, long);
numeric_cast_type_downcast_signed(int, long long);
numeric_cast_type_downcast_signed(long, long long);



//----------------------------------------
// ----------------------------------------
// sign-type different, bitsize equal
// ----------------------------------------
//----------------------------------------

#define numeric_cast_type_samesize_remove_sign(OUT, IN) 		\
		template<>												\
		struct numeric_cast_type <OUT, IN> {					\
			static OUT numeric_cast(IN var) {					\
				if ( 0 <= var )									\
					return var;									\
				else											\
				throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);	\
			}													\
		}

#define numeric_cast_type_samesize_append_sign(OUT, IN) 						\
		template<>																\
		struct numeric_cast_type <OUT, IN> {									\
			static OUT numeric_cast(IN var) {									\
				if ( var <= static_cast<IN>(std::numeric_limits<OUT>::max()) )	\
					return var;													\
				else															\
				throw msk_exception(RMSK_OUT_OF_BOUNDS_MSG);					\
			}																	\
		}

//
// UNSIGNED X <- SIGNED X
//
numeric_cast_type_samesize_remove_sign(unsigned int, int);
numeric_cast_type_samesize_remove_sign(unsigned long, long);
numeric_cast_type_samesize_remove_sign(unsigned long long, long long);

//
// SIGNED X <- UNSIGNED X
//
numeric_cast_type_samesize_append_sign(int, unsigned int);
numeric_cast_type_samesize_append_sign(long, unsigned long);
numeric_cast_type_samesize_append_sign(long long, unsigned long long);



//----------------------------------------
// ----------------------------------------
// sign-type different, bitsize different
// ----------------------------------------
//----------------------------------------

#define numeric_cast_type_diffsize_remove_sign(OUT, IN)									\
		template<>																		\
		struct numeric_cast_type <OUT, IN> {											\
			static OUT numeric_cast(IN var) {											\
				return numeric_cast_type<OUT, unsigned IN>::numeric_cast(				\
							numeric_cast_type<unsigned IN, IN>::numeric_cast( var ) );	\
			}																			\
		}

#define numeric_cast_type_diffsize_append_sign(OUT, IN)									\
		template<>																		\
		struct numeric_cast_type <OUT, IN> {											\
			static OUT numeric_cast(IN var) {											\
				return numeric_cast_type<OUT, unsigned OUT>::numeric_cast(				\
							numeric_cast_type<unsigned OUT, IN>::numeric_cast( var ) );	\
			}																			\
		}

//
// UNSIGNED X <- SIGNED Y
//
numeric_cast_type_diffsize_remove_sign(unsigned long, int);
numeric_cast_type_diffsize_remove_sign(unsigned long long, int);
numeric_cast_type_diffsize_remove_sign(unsigned long long, long);

numeric_cast_type_diffsize_remove_sign(unsigned int, long);
numeric_cast_type_diffsize_remove_sign(unsigned int, long long);
numeric_cast_type_diffsize_remove_sign(unsigned long, long long);

//
// SIGNED X <- UNSIGNED Y
//
numeric_cast_type_diffsize_append_sign(long, unsigned int);
numeric_cast_type_diffsize_append_sign(long long, unsigned int);
numeric_cast_type_diffsize_append_sign(long long, unsigned long);

numeric_cast_type_diffsize_append_sign(int, unsigned long);
numeric_cast_type_diffsize_append_sign(int, unsigned long long);
numeric_cast_type_diffsize_append_sign(long, unsigned long long);


___RMSK_INNER_NS_END___

#endif /* RMSK_NUMERIC_CASTS_H_ */
