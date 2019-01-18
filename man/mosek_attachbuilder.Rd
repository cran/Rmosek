\name{mosek_attachbuilder}
\alias{mosek_attachbuilder}
\title{Attach builder functions (e.g., 'install.rmosek') to the search path}
\usage{
mosek_attachbuilder(what_mosek_bindir, 
                    pos=2L, name="Rmosek:builder", warn.conflicts=TRUE)
}
\description{
This function \code{\link{attach}} a database of functions to the search path:

    install.rmosek     - install Rmosek package.
    update.rmosek      - update Rmosek package.
    remove.rmosek      - remove Rmosek package.

These *.rmosek functions wrap the corresponding utils::*.packages functions
with autoconfigured default argument values:
      pkgs           = "Rmosek"
      repos          = "https://download.mosek.com/R/<MOSEKMAJORVER>.<MOSEKMINORVER>"

configuration variables:
      MSK_BINDIR     : MOSEK 'bin' directory.
                       If empty, autoconfigured from mosek executable on PATH.

      MSK_HEADERDIR  : MOSEK 'h' directory.
                       If empty, autoconfigured from 'MSK_BINDIR'.

      MSK_LIB        : MOSEK library file.
                       If empty, autoconfigured from 'MSK_BINDIR'.

and installation styles:
      using_pkgbuild : Logical. Whether to execute in the 'pkgbuild' environment
                       as is recommended on Windows to resolve Rtools.
                        
      using_sysenv   : Logical. Whether to transmit configuration variables via 
                       Sys.setenv() as opposed to configure.vars.
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
\examples{
# Build Rmosek for the given MOSEK Optimization Suite.
\dontrun{
mosek_attachbuilder("<MSKHOME>/mosek/<MSKVERSION>/tools/platform/<PLATFORM>/bin")
install.rmosek()
}

# Update Rmosek for the MOSEK Optimization Suite of the last build.
\dontrun{
mosek_attachbuilder()    # Note: the path is read from packageDescription("Rmosek")
update.rmosek()
}

# Build Rmosek (from offline package file) for the given MOSEK Optimization Suite.
\dontrun{
mosek_attachbuilder("<MSKHOME>/mosek/<MSKVERSION>/tools/platform/<PLATFORM>/bin")
install.rmosek("<RMOSEKPATH>/Rmosek.tar.gz", repos=NULL)
}
}
