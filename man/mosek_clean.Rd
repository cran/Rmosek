\name{mosek_clean}
\alias{mosek_clean}
\title{Release an acquired MOSEK license}
\usage{
mosek_clean()
}
\description{
  Forces the early release of any previously acquired MOSEK license. If you do not share a limited number of licenses among multiple users, you do not need to use this function. Notice that the acquisition of a new MOSEK license will automatically take place at the next call to the function \code{mosek} given a valid problem definition, using a small amount of extra time.

  For advanced users: If you utilize the .Call convention directly, bypassing the mosek R-function defintion, an Rf_error will result in an unclean memory space. For this reason you can also use \code{mosek_clean} to tidy up in uncleaned ressources after an error has occured. Otherwise this will not happen until the next call to \code{mosek} or until the library is unloaded.
}
\seealso{
  \code{\link{mosek}}
}
