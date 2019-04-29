\name{mosek_attachbuilder}
\alias{mosek_attachbuilder}
\title{Attach builder functions to the search path (install,update,remove)}
\usage{
mosek_attachbuilder(what_mosek_bindir, 
                    pos=2L, name="Rmosek:builder", warn.conflicts=TRUE)
}
\description{
This function \code{\link{attach}} a database to the search path with utilities
to manage Rmosek in respect to the system requirement (i.e., MOSEK):
\tabular{ll}{
\code{install.rmosek}   \tab Install Rmosek package.\cr
\code{update.rmosek}    \tab Update Rmosek package. \cr
\code{remove.rmosek}    \tab Remove Rmosek package. \cr
}
These utilities are equivalent to, and accept the same arguments as,
\code{\link{install.packages}}, \code{\link{update.packages}} and 
\code{\link{remove.packages}}, differing only in default argument values 
and the transmission of Rmosek-specific configuration variables (see details below).
}
\arguments{
\item{what_mosek_bindir}{
    The path to the MOSEK 'bin' directory to build against. Should look like 
    \code{<MSKHOME>/mosek/<MSKVERSION>/tools/platform/<PLATFORM>/bin},
    where <MSKHOME> is the folder in which the MOSEK Optimization Suite has 
    been installed. 
    If \code{\link{NA}}, it uses the MOSEK binary directory of the last build (if any).
    If \code{""}, it attaches a generic builder.}
\item{pos}{Integer specifying position in \code{\link{search}()} where to attach.}
\item{name}{Name to use for the attached database. Names starting with \code{package:} 
           are reserved for \code{\link{library}}.}
\item{warn.conflicts}{
    Logical. If \code{TRUE}, warnings are
    printed about \code{\link{conflicts}} from attaching the database,
    unless that database contains an object \code{.conflicts.OK}.  A
    conflict is a function masking a function, or a non-function masking
    a non-function.}
}
\details{
The attached *.rmosek functions (install, update, remove) wrap the corresponding 
utils::*.packages functions with autoconfigured default argument values:
\tabular{ll}{
    pkgs            \tab = "Rmosek"\cr
    repos           \tab = "https://download.mosek.com/R/<MOSEKMAJORVER>.<MOSEKMINORVER>"\cr
}
configuration variables:
\tabular{ll}{
    MSK_BINDIR      \tab MOSEK 'bin' directory.\cr
                    \tab If empty, autoconfigured from mosek executable on PATH.\cr
\cr
    MSK_HEADERDIR   \tab MOSEK 'h' directory.\cr
                    \tab If empty, autoconfigured from 'MSK_BINDIR'.\cr
\cr
    MSK_LIB         \tab MOSEK library file.\cr
                    \tab If empty, autoconfigured from 'MSK_BINDIR'.\cr
}
and installation styles:
\tabular{ll}{
    using_pkgbuild  \tab Logical. Whether to execute in the 'pkgbuild' environment\cr
                    \tab as is recommended on Windows to resolve Rtools.\cr
                        
    using_sysenv    \tab Logical. Whether to transmit configuration variables via\cr
                    \tab Sys.setenv() as opposed to configure.vars.\cr
}
}
\examples{
\dontrun{
#
# Build Rmosek for the given MOSEK Optimization Suite.
#
mosek_attachbuilder("<MSKHOME>/mosek/<MSKVERSION>/tools/platform/<PLATFORM>/bin")
install.rmosek()

#
# Update Rmosek for the MOSEK Optimization Suite of the last build (if any).
#
mosek_attachbuilder()    # Note: path is read from packageDescription("Rmosek")
update.rmosek()

#
# Build Rmosek (from offline package file) for the given MOSEK Optimization Suite.
#
mosek_attachbuilder("<MSKHOME>/mosek/<MSKVERSION>/tools/platform/<PLATFORM>/bin")
install.rmosek("<RMOSEKPATH>/Rmosek.tar.gz", repos=NULL)

}
}
