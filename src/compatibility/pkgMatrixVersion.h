#ifndef RMSK_PKGMATRIXVERSION_H_
#define RMSK_PKGMATRIXVERSION_H_

#define pkgMatrixVersion_MAJOR 0
#define pkgMatrixVersion_MINOR 0
#define pkgMatrixVersion_PATCH 0

#define pkgMatrixVersion_LESS_THAN_OR_EQUAL(x,y,z)                                              \
    (pkgMatrixVersion_MAJOR  < x) ||                                                            \
    (pkgMatrixVersion_MAJOR == x && pkgMatrixVersion_MINOR  < y) ||                             \
    (pkgMatrixVersion_MAJOR == x && pkgMatrixVersion_MINOR == y && pkgMatrixVersion_PATCH <= z)


#endif /* RMSK_PKGMATRIXVERSION_H_ */
