\name{mosek_lptoprob}
\alias{mosek_lptoprob}
\title{Construct problem from a linear program}
\usage{
mosek_lptoprob(f,A,b,Aeq,beq,lb,ub)
}
\description{
  Construct a problem description from the following linear program:

  \tabular{ll}{  
    minimize: \tab f'x \cr
    subject to: \cr
    \tab A x   <= b \cr
    \tab Aeq x  = beq \cr
    with bounds: \cr
    \tab lb <= x <= ub
  }

  The result of this function is compatible with the problem description of the \code{mosek} function.
}
\arguments{
\item{f}{Objective coefficients (size n)}
\item{A}{Constraint inequality matrix (size mA x n)}
\item{b}{Constraint inequality upper bounds (size mA)}
\item{Aeq}{Constraint equality matrix (size mEQ x n)}
\item{beq}{Constraint equality fixed values (size mEQ)}
\item{lb}{Variable lower bounds (size n)}
\item{ub}{Variable upper bounds (size n)}
}
\examples{
 # Define a linear program
 f <- c(0,-5,0)
 A <- Matrix(c( 4, 3, 0,
               -2,-1, 0,
                0, 2,-1), nrow=3, byrow=TRUE, sparse=TRUE)
 b <- c(8,-2,0)
 Aeq <- NA; 
 beq <- NA;
 lb <- rep(-Inf, 3);
 ub <- rep(Inf, 3);

 # Construct and solve problem
 prob <- mosek_lptoprob(f, A, b, Aeq, beq, lb, ub);
 r <- mosek(prob);

 # Objective value is
 print(prob$c \%*\% r$sol$bas$xx);
}
\seealso{
\code{\link{mosek}}
\code{\link{mosek_qptoprob}}
}
