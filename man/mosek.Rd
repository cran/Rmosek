\name{mosek}
\alias{mosek}
\title{Solve an optimization problem}
\usage{
mosek(problem, opts = list())
}
\description{
  Solve an optimization problem using the MOSEK Optimization Library.

  Please see the 'userguide.pdf' for a detailed introduction to this package. 
  This file is located in the "doc" directory at the root of this package:
  \cr \code{system.file("doc", "userguide.pdf", package="Rmosek")}
}
\arguments{
\item{problem}{The optimization problem.
\tabular{lll}{
problem 			\tab LIST			\tab			\cr
..$sense			\tab STRING			\tab			\cr
..$c 				\tab NUMERIC VECTOR		\tab			\cr
..$c0 				\tab NUMERIC 			\tab (OPTIONAL)		\cr
..$A 				\tab SPARSE MATRIX		\tab			\cr
..$bc				\tab NUMERIC MATRIX (2 rows)	\tab			\cr
..$bx				\tab NUMERIC MATRIX (2 rows)	\tab			\cr
..$cones			\tab LIST MATRIX (2 rows)	\tab (OPTIONAL)		\cr
..$intsub 			\tab NUMERIC VECTOR		\tab (OPTIONAL)		\cr
..$qobj                         \tab LIST                       \tab (OPTIONAL)         \cr
..$scopt			\tab LIST			\tab (OPTIONAL)		\cr
..$iparam/$dparam/$sparam	\tab LIST			\tab (OPTIONAL) 	\cr
....$<MSK_PARAM> 		\tab STRING / NUMERIC		\tab (OPTIONAL)		\cr
..$sol				\tab LIST			\tab (OPTIONAL) 	\cr
....$itr/$bas/$int		\tab LIST			\tab (OPTIONAL) 	\cr
}}
\item{opts}{The interface options.
\tabular{lll}{
opts				\tab LIST			\tab (OPTIONAL) 	\cr
..$verbose			\tab NUMERIC 			\tab (OPTIONAL)		\cr
..$usesol			\tab BOOLEAN			\tab (OPTIONAL)		\cr
..$useparam 			\tab BOOLEAN			\tab (OPTIONAL)		\cr
..$soldetail			\tab NUMERIC			\tab (OPTIONAL)		\cr
..$getinfo			\tab BOOLEAN			\tab (OPTIONAL)		\cr
..$writebefore			\tab STRING (filepath) 		\tab (OPTIONAL) 	\cr
..$writeafter			\tab STRING (filepath) 		\tab (OPTIONAL) 	\cr
}}
}
\details{
The optimization problem should be described in a named list of definitions.
The number of variables in the problem is determined from the number of 
columns in the constraint matrix \code{A}.

Like a Linear Program it has a linear objective with one coefficient in 
\code{c} for each variable, some optional constant \code{c0}, and the improving
direction \code{sense}. Quadratic terms can be added to the objective with 
\code{qobj}. The constraints can either be linear, specified as rows in 
\code{A} with lower and upper bounds as columns in \code{bc} (you can use 
\code{Inf} if needed), or conic as specified in the list-typed matrix 
\code{cones} (add constraints copyx=x if some variable x appears in multiple 
cones). All variables have lower and upper bounds as columns in \code{bx}, 
and will be integer if they appear in the \code{intsub} list.

As an advanced feature, non-linear unary operators involving exponential or 
logarithmic functions can be added with \code{scopt}. Parameters can also be 
specified for the MOSEK call. \code{iparam} is integer-typed parameters, 
\code{dparam} ia double-typed parameters and \code{sparam} is string-typed 
parameters. These parameters can be ignored by setting the option 
\code{useparam} to FALSE (the default is TRUE).

Initial solutions are specified in \code{sol} and should have the same format 
as the solution returned by the function call. This solution can be ignored by 
setting the option \code{usesol} to FALSE (the default is TRUE).

The amount of information printed by the interface can be limited by 
\code{verbose} (default=10). The generated model can be exported to any 
standard modeling fileformat (e.g. lp, opf, lp or mbt), with (resp. without) 
the identified solution using \code{writeafter} (resp. \code{writebefore}). 

The optimization process can be terminated at any moment using CTRL + C.
\tabular{ll}{
  problem			\tab Problem description \cr
  .$sense			\tab Objective sense, e.g. "max" or "min" \cr
  .$c				\tab Objective coefficients \cr
  .$c0				\tab Objective constant \cr
  .$A				\tab Constraint matrix \cr
  .$bc				\tab Lower and upper constraint bounds \cr
  .$bx				\tab Lower and upper variable bounds \cr
  .$qobj                        \tab Quadratic objective terms \cr
  .$cones			\tab Conic constraints \cr
  .$intsub			\tab Integer variable indexes \cr
  .$scopt			\tab Separable convex optimization \cr
  .$iparam/$dparam/$sparam	\tab Parameter list \cr
  ..$<MSK_PARAM>		\tab Value of any <MSK_PARAM> \cr
  .$sol				\tab Initial solution list \cr
  ..$itr/$bas/$int		\tab Initial solution description \cr
  opts				\tab Options \cr
  .$verbose			\tab Output logging verbosity \cr
  .$usesol			\tab Whether to use the initial solution \cr
  .$useparam			\tab Whether to use the specified parameter settings \cr
  .$soldetail			\tab Level of detail used to describe solutions. \cr
  .$getinfo			\tab Whether to extract MOSEK information items \cr
  .$writebefore			\tab Filepath used to export model \cr
  .$writeafter			\tab Filepath used to export model and solution \cr
}
}
\value{
\item{r}{The returned results.
\tabular{lll}{
r				\tab LIST		\tab 			\cr
..$response			\tab LIST		\tab 			\cr
....$code			\tab NUMERIC		\tab 			\cr
....$msg			\tab STRING		\tab 			\cr
..$sol				\tab LIST		\tab 			\cr
....$itr/$bas/$int		\tab LIST		\tab (SOLVER DEPENDENT) \cr
......$solsta			\tab STRING		\tab 			\cr
......$prosta			\tab STRING		\tab 			\cr
......$skc			\tab STRING VECTOR	\tab                    \cr
......$skx			\tab STRING VECTOR	\tab 			\cr
......$skn			\tab STRING VECTOR	\tab (NOT IN $bas)      \cr
......$xc			\tab NUMERIC VECTOR	\tab 			\cr
......$xx			\tab NUMERIC VECTOR	\tab 			\cr
......$slc			\tab NUMERIC VECTOR	\tab (NOT IN $int) 	\cr
......$suc			\tab NUMERIC VECTOR	\tab (NOT IN $int) 	\cr
......$slx			\tab NUMERIC VECTOR	\tab (NOT IN $int) 	\cr
......$sux 			\tab NUMERIC VECTOR	\tab (NOT IN $int) 	\cr
......$snx 			\tab NUMERIC VECTOR	\tab (NOT IN $int/$bas) \cr
......$pobjval			\tab NUMERIC		\tab *			\cr
......$dobjval			\tab NUMERIC		\tab *(NOT IN $int)	\cr
......$pobjbound		\tab NUMERIC		\tab *($int ONLY)       \cr
......$maxinfeas		\tab LIST		\tab *			\cr
........$pbound			\tab NUMERIC		\tab *			\cr
........$peq	                \tab NUMERIC            \tab *                  \cr
........$pcone	                \tab NUMERIC            \tab *(NOT IN $bas)     \cr
........$dbound	                \tab NUMERIC            \tab *(NOT IN $int)     \cr
........$deq	                \tab NUMERIC            \tab *(NOT IN $int)     \cr
........$dcone	                \tab NUMERIC            \tab *(NOT IN $int/$bas)\cr
........$int	                \tab NUMERIC            \tab *($int ONLY)       \cr
..$iinfo/$dinfo			\tab LIST		\tab *             	\cr
....$<MSK_INFO>			\tab NUMERIC		\tab *			\cr
}
\tabular{l}{*Starred items must be requested using an option.\cr}
}

The result is a named list containing the response of the MOSEK optimization
library. A response code of zero is the signal of success.

Depending on the specified solver, one or more solutions may be returned. The 
interior-point solution \code{itr}, the basic (corner point) solution 
\code{bas}, and the integer solution \code{int}.

The problem status \code{prosta} in all solutions shows the feasibility of your
problem description. All solutions are described by a solution status 
\code{solsta} (e.g. optimal) along with the variable and constraint activities.
All activities will further have a bound key that specify their value in 
relation to the declared bounds.

Dual variables are returned for all defined bounds wherever possible. Integer 
solutions \code{int} does not have any dual variables as such definitions 
would not make sense. Basic (corner point) solutions \code{bas} would never be 
returned if the problem had conic constraints, and does not define \code{snx}.

Setting option \code{soldetail} larger than 1 extracts \code{pobjval}, 
\code{pobjval} and \code{pobjbound}. Larger than 2 extracts \code{maxinfeas}. 
Setting option \code{getinfo} to \code{TRUE} extracts \code{iinfo} and 
\code{dinfo}.
\tabular{lll}{
  r			\tab Result \cr
  .$response		\tab Response from the MOSEK Optimization Library \cr
  ..$code		\tab ID-code of response \cr
  ..$msg		\tab Human-readable message \cr
  .$sol			\tab All solutions identified \cr
  ..$itr/$bas/$int	\tab Solution description  \cr
  ...$solsta		\tab Solution status  \cr
  ...$prosta		\tab Problem status  \cr
  ...$skc		\tab Linear constraint status keys  \cr
  ...$skx		\tab Variable bound status keys  \cr
  ...$skn		\tab Conic constraint status keys  \cr
  ...$xc		\tab Constraint activities  \cr
  ...$xx		\tab Variable activities  \cr
  ...$slc		\tab Dual variable for constraint lower bounds  \cr
  ...$suc		\tab Dual variable for constraint upper bounds  \cr
  ...$slx		\tab Dual variable for variable lower bounds  \cr
  ...$sux		\tab Dual variable for variable lower bounds  \cr
  ...$snx		\tab Dual variable of conic constraints \cr
  ...$pobjval           \tab Primal objective value \cr
  ...$dobjval           \tab Dual objective value \cr
  ...$pobjbound         \tab Best primal objective bound from relaxations \cr
  ...$maxinfeas         \tab Maximal solution infeasibilities \cr
  ....$pbound           \tab Primal inequality constraints \cr
  ....$peq              \tab Primal equality constraints \cr
  ....$pcone            \tab Primal cone constraints \cr
  ....$dbound           \tab Dual inequality constraints \cr
  ....$deq              \tab Dual equality constraints \cr
  ....$dcone            \tab Dual cone constraints \cr
  ....$int              \tab Integer variables \cr
  .$iinfo/$dinfo	\tab MOSEK information list \cr
  ..$<MSK_INFO>		\tab Value of any <MSK_INFO> \cr
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
                 buc = c(30,Inf,25))
 lo1$bx <- rbind(blx = c(0,0,0,0),
                 bux = c(Inf,10,Inf,Inf))
 r <- mosek(lo1, list( soldetail = 1 ))
}
\seealso{
\code{\link{mosek_version}}
\code{\link{mosek_clean}}
}
