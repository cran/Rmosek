fileLocation <- "./src/compatibility/pkgMatrixVersion.h";
pkgMatrixVersion <- unlist(as.list( numeric_version(packageDescription("Matrix",fields="Version")) ));

out <- rep(0, 3);
out[1:max(length(out),length(pkgMatrixVersion))] <- pkgMatrixVersion;


write("#ifndef RMSK_PKGMATRIXVERSION_H_", file=fileLocation, append=FALSE)
write("#define RMSK_PKGMATRIXVERSION_H_", file=fileLocation, append=TRUE)
write("", file=fileLocation, append=TRUE)
write(paste("#define pkgMatrixVersion_MAJOR", out[1]), file=fileLocation, append=TRUE)
write(paste("#define pkgMatrixVersion_MINOR", out[2]), file=fileLocation, append=TRUE)
write(paste("#define pkgMatrixVersion_PATCH", out[3]), file=fileLocation, append=TRUE)
write("
#define pkgMatrixVersion_LESS_THAN_OR_EQUAL(x,y,z)                                              \\
    (pkgMatrixVersion_MAJOR  < x) ||                                                            \\
    (pkgMatrixVersion_MAJOR == x && pkgMatrixVersion_MINOR  < y) ||                             \\
    (pkgMatrixVersion_MAJOR == x && pkgMatrixVersion_MINOR == y && pkgMatrixVersion_PATCH <= z)
", file=fileLocation, append=TRUE)
write("", file=fileLocation, append=TRUE)
write("#endif /* RMSK_PKGMATRIXVERSION_H_ */", file=fileLocation, append=TRUE)

