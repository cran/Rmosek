\name{mosek_write}
\alias{mosek_write}
\title{Write problem to a model file}
\usage{
mosek_write(problem, modelfile, opts = list())
}
\description{
  Outputs a model of an optimization problem in any standard modeling fileformat (e.g. lp, opf, mps, mbt, etc.), controlled by a set of options. The modeling fileformat is selected based on the extension of the modelfile.
}
\arguments{
\item{problem}{The optimization problem.
\tabular{lll}{
problem 			\tab LIST		\tab			\cr
}}
\item{modelfile}{The file to write the optimization model.
\tabular{lll}{
modelfile 			\tab STRING (filepath)	\tab			\cr
}}
\item{opts}{The interface options.
\tabular{lll}{
opts 				\tab LIST		\tab (OPTIONAL)		\cr
..$verbose 			\tab NUMERIC 		\tab (OPTIONAL)		\cr
..$usesol			\tab BOOLEAN		\tab (OPTIONAL)		\cr
..$useparam 			\tab BOOLEAN		\tab (OPTIONAL)		\cr
..$scofile 			\tab STRING (filepath)	\tab (MANDATORY IN SCOPT)		\cr
}}
}
\details{
The \code{problem} should be compliant with the input specification of function \code{\link{mosek}}. Please see this function for more details.

The \code{modelfile} should be an absolute or relative path to the model file. If the file extension is \code{.opf}, the model will be written in the Optimization Problem Format. Other formats include lp, mps and mbt.

The amount of information printed by the interface can be limited by \code{verbose} (default=10). Whether to write the initial solution, if one such exists in the problem description, is indicated by \code{usesol} which by default is FALSE. Whether to write the full list of parameter settings, some of which may have been specified by the problem description, is indicated by \code{useparam} which by default is FALSE.

The option \code{scofile} is used in separable convex optimization to specify the absolute or relative path to the operator file.

\tabular{ll}{
  problem 		\tab Problem description \cr
  modelfile		\tab Filepath to the model \cr
  opts 			\tab Options \cr
  .$verbose 		\tab Output logging verbosity \cr
  .$usesol		\tab Whether to write an initial solution \cr
  .$useparam		\tab Whether to write all parameter settings \cr
  .$scofile		\tab Destination of operators from scopt \cr
}
}
\value{
\item{r}{The returned result.
\tabular{lll}{
r				\tab LIST		\tab 			\cr
..$response			\tab LIST		\tab 			\cr
....$code			\tab NUMERIC		\tab 			\cr
....$msg			\tab STRING		\tab 			\cr
}}

The result is a named list containing the response of the MOSEK Optimization Library when writing to the model file. A response code of zero is the signal of success.

\tabular{lll}{
  r			\tab Result \cr
  .$response		\tab Response from the MOSEK Optimization Library \cr
  ..$code		\tab ID-code of response \cr
  ..$msg		\tab Human-readable message \cr
}
}
\examples{
 lo1 <- list()
 lo1$sense <- "max"
 lo1$c <- c(3,1,5,1)
 lo1$A <- Matrix(c(3,1,2,0,
                   2,1,3,1,
                   0,2,0,3), nrow=3, byrow=TRUE, sparse=TRUE)
 lo1$bc <- rbind(blc = c(30,15,-Inf),
                 buc = c(30,Inf,25));
 lo1$bx <- rbind(blx = c(0,0,0,0),
                 bux = c(Inf,10,Inf,Inf));
 rr <- mosek_write(lo1, "lo1.opf")
 if (!identical(rr$response$code, 0))
   stop("Failed to write model file to current working directory")
}
\seealso{
  \code{\link{mosek}}
  \code{\link{mosek_read}}
}
