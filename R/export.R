msg <- "Unknown exported object. Please use 'Rmosek::mosek_attachbuilder' to complete the installation of Rmosek."

mosek           <- function(...) { stop(msg) }
mosek_clean     <- function(...) { stop(msg) }
mosek_version   <- function(...) { stop(msg) }
mosek_read      <- function(...) { stop(msg) }
mosek_write     <- function(...) { stop(msg) }
