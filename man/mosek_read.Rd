\name{mosek_read}
\alias{mosek_read}
\title{Read problem from a model file}
\usage{
mosek_read(modelfile, opts = list())
}
\description{
  Interprets a model from any standard modelling fileformat (e.g. lp, opf, mps, mbt, etc.), controlled by a set of options. The result contains an optimization problem which is compliant with the input specifications of function \code{mosek}.
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
}}
}
\details{
The \code{modelfile} should be an absolute path to a model file. 

The amount of information printed by the interface can be limited by \code{verbose} (default=10). Whether to read the initial solution, if one such exists in the model file, is indicated by \code{usesol} which by default is FALSE. Whether to read the full list of parameter settings, some of which may have been changed by the model file, is indicated by \code{useparam} which by default is FALSE.

\tabular{ll}{
  modelfile		\tab Filepath to the model \cr
  opts 			\tab Options \cr
  .$verbose 		\tab Output logging verbosity \cr
  .$usesol		\tab Whether to read an initial solution \cr
  .$useparam		\tab Whether to read all parameter settings \cr
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
}}

The result is a named list containing the response of the MOSEK Optimization Library when reading the model file. Note that the result can be NULL if interfacing errors are encountered.

On success, the result contains the problem specification with all problem data. This problem specification is compliant with the input specifications of function \code{\link{mosek}}.

\tabular{lll}{
  r			\tab Result \cr
  .$response		\tab Response from the MOSEK Optimization Library \cr
  ..$code		\tab ID-code of response \cr
  ..$msg		\tab Human-readable message \cr
  .$prob		\tab Problem desciption \cr
}
}
\examples{
 modelfile <- system.file(package="rmosek", "extdata", "lo1.opf")
 rr <- mosek_read(modelfile)
 if (is.null(rr) || rr$response$code != 0)
   stop("Failed to read model file")
 rlo1 <- mosek(rr$prob)

 modelfile <- system.file(package="rmosek", "extdata", "milo1.opf")
 rr <- mosek_read(modelfile)
 if (is.null(rr) || rr$response$code != 0)
   stop("Failed to read model file")
 rmilo1 <- mosek(rr$prob)

 modelfile <- system.file(package="rmosek", "extdata", "cqo1.opf")
 rr <- mosek_read(modelfile)
 if (is.null(rr) || rr$response$code != 0)
   stop("Failed to read model file")
 rcqo1 <- mosek(rr$prob)
}
\seealso{
  \code{\link{mosek}}
  \code{\link{mosek_write}}
}
