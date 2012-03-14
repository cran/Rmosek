\name{mosek_read}
\alias{mosek_read}
\title{Read problem from a model file}
\usage{
mosek_read(modelfile, opts = list())
}
\description{
  Interprets a model from any standard modeling fileformat (e.g. lp, opf, mps, mbt, etc.), controlled by a set of options. The result contains an optimization problem which is compliant with the input specifications of function \code{mosek}.
}
\arguments{
\item{modelfile}{The file containing an optimization model.
\tabular{lll}{
modelfile 			\tab STRING (filepath)	\tab			\cr
}}
\item{opts}{The interface options.
\tabular{lll}{
opts 				\tab LIST		\tab (OPTIONAL)		\cr
..$verbose 			\tab NUMERIC 		\tab (OPTIONAL)		\cr
..$usesol			\tab BOOLEAN		\tab (OPTIONAL)		\cr
..$useparam 			\tab BOOLEAN		\tab (OPTIONAL)		\cr
..$getinfo			\tab BOOLEAN		\tab (OPTIONAL)		\cr
..$scofile			\tab STRING (filepath)	\tab (OPTIONAL)		\cr
..$matrixformat 		\tab STRING		\tab (OPTIONAL)		\cr
}}
}
\details{
The \code{modelfile} should be an absolute or relative path to a model file. 

The amount of information printed by the interface can be limited by \code{verbose} (default=10). Whether to read the initial solution, if one such exists in the model file, is indicated by \code{usesol} which by default is FALSE. Whether to read the full list of parameter settings, some of which may have been defined by the model file, is indicated by \code{useparam} which by default is FALSE. 

The option \code{scofile} is used in separable convex optimization to specify the absolute or relative path to the operator file.

The format of the imported constraint matrix is controlled by \code{matrixformat} and can be either sparse coordinate \code{COO}, compressed sparse column \code{CSC}, or a list-based alternative \code{simple:COO}. The matrix formats \code{CSC} and \code{COO} are based on the package 'Matrix' superclasses \code{CsparseMatrix} and \code{TsparseMatrix}.

\tabular{ll}{
  modelfile		\tab Filepath to the model \cr
  opts 			\tab Options \cr
  .$verbose 		\tab Output logging verbosity \cr
  .$usesol		\tab Whether to read an initial solution \cr
  .$useparam		\tab Whether to read all parameter settings \cr
  .$getinfo             \tab Whether to extract MOSEK information items \cr
  .$scofile		\tab Source of operators read to scopt \cr
  .$matrixformat	\tab The sparse format of the constraint matrix \cr
}
}
\value{
\item{r}{The returned result.
\tabular{lll}{
r				\tab LIST		\tab 			\cr
..$response			\tab LIST		\tab 			\cr
....$code			\tab NUMERIC		\tab 			\cr
....$msg			\tab STRING		\tab 			\cr
..$prob				\tab LIST		\tab 			\cr
..$iinfo/$dinfo                 \tab LIST               \tab *                  \cr
....$<MSK_INFO>                 \tab NUMERIC            \tab *                  \cr
}
\tabular{l}{*Starred items must be requested using an option.\cr}
}

The result is a named list containing the response of the MOSEK Optimization Library when reading the model file. A response code of zero is the signal of success.

On success, the result contains the problem specification with all problem data. This problem specification is compliant with the input specifications of function \code{\link{mosek}}.

Setting option \code{getinfo} to \code{TRUE} extracts \code{iinfo} and \code{dinfo}.
\tabular{lll}{
  r			\tab Result \cr
  .$response		\tab Response from the MOSEK Optimization Library \cr
  ..$code		\tab ID-code of response \cr
  ..$msg		\tab Human-readable message \cr
  .$prob		\tab Problem description \cr
  .$iinfo/$dinfo        \tab MOSEK information list \cr
  ..$<MSK_INFO>         \tab Value of any <MSK_INFO> \cr
}
}
\examples{
 modelfile <- system.file(package="Rmosek", "extdata", "lo1.opf")
 rr <- mosek_read(modelfile)
 if (!identical(rr$response$code, 0))
   stop("Failed to read model file")
 rlo1 <- mosek(rr$prob)

 modelfile <- system.file(package="Rmosek", "extdata", "milo1.opf")
 rr <- mosek_read(modelfile)
 if (!identical(rr$response$code, 0))
   stop("Failed to read model file")
 rmilo1 <- mosek(rr$prob)

 modelfile <- system.file(package="Rmosek", "extdata", "cqo1.opf")
 rr <- mosek_read(modelfile)
 if (!identical(rr$response$code, 0))
   stop("Failed to read model file")
 rcqo1 <- mosek(rr$prob)

 modelfile <- system.file(package="Rmosek", "extdata", "sco1.opf")
 modelscofile <- system.file(package="Rmosek", "extdata", "sco1.sco")
 rr <- mosek_read(modelfile, list(scofile=modelscofile))
 if (!identical(rr$response$code, 0))
   stop("Failed to read model file")
 rsco1 <- mosek(rr$prob)
}
\seealso{
  \code{\link{mosek}}
  \code{\link{mosek_write}}
}
