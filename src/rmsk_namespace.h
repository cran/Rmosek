/*
 * Namespace alias can only be using namespace-scoped identifiers, as opposed using them in definitions.
 * Notice the C++ standard in 7.3.1.1 [namespace.def] of ISO/IEC 14882:1998(E).
 *
 * extension-namespace-definition:
 *       namespace original-namespace-name { namespace-body }
 *
 * Thus we define a macro..
 */

#ifndef RMSK_NAMESPACE_H_
#define RMSK_NAMESPACE_H_

#define ___RMSK_INNER_NS_START___       \
namespace MSK4 {                        \
  namespace MSK3 {                      \
    namespace MSK2 {                    \
      namespace MSK1 {

#define ___RMSK_INNER_NS_END___         \
      }                                 \
    }                                   \
  }                                     \
}

// Create namespace
___RMSK_INNER_NS_START___
___RMSK_INNER_NS_END___

// Create namespace alias (read-only)
namespace RMSK_INNER_NS = MSK4::MSK3::MSK2::MSK1;

#endif /* RMSK_NAMESPACE_H_ */
