fmsg <- "Unknown exported object to be built. Please use 'Rmosek::mosek_attachbuilder' to complete the installation of Rmosek."

mosek           <- function(...) { stop(fmsg) }
mosek_clean     <- function(...) { stop(fmsg) }
mosek_version   <- function(...) { stop(fmsg) }
mosek_read      <- function(...) { stop(fmsg) }
mosek_write     <- function(...) { stop(fmsg) }


startmsg <- "
   The Rmosek meta-package is ready. Please call

      mosek_attachbuilder(what_mosek_bindir)

   to complete the installation. See also '?mosek_attachbuilder'.

"

.onAttach <- function(libname, pkgname) { packageStartupMessage(startmsg) }

