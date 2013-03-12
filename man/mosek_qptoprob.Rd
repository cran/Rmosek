\name{mosek_qptoprob}
\alias{mosek_qptoprob}
\title{Construct problem from a quadratic program}
\usage{
mosek_qptoprob(F,f,A,b,Aeq,beq,lb,ub)
}
\description{
  Construct a conic problem description from the following quadratic program:

  \tabular{ll}{  
    minimize: \tab f'x + 0.5x'(F'F)x \cr
    subject to: \cr
    \tab A x   <= b \cr
    \tab Aeq x  = beq \cr
    with bounds: \cr
    \tab lb <= x <= ub
  }

  Given that F is not known, but Q = F'F on the other hand is, we can estimate 
  F from Q by the Cholesky Decomposition: F = Matrix::chol(Q). The result of 
  the \code{mosek_qptoprob} function is compatible with the problem description 
  of the \code{mosek} function. 

  Note that problems with a quadratic objective can also be formulated without
  cones, using the field 'qobj' in the problem description. This is documented 
  in the userguide.
}
\arguments{
\item{F}{Objective quadratic coefficient matrix (size mF x n)}
\item{f}{Objective linear coefficient vector (size n)}
\item{A}{Constraint inequality matrix (size mA x n)}
\item{b}{Constraint inequality upper bounds (size mA)}
\item{Aeq}{Constraint equality matrix (size mEQ x n)}
\item{beq}{Constraint equality fixed values (size mEQ)}
\item{lb}{Variable lower bounds (size n)}
\item{ub}{Variable upper bounds (size n)}
}
\examples{
 # Define a quadratic program
 F <- Diagonal(3)
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
 prob <- mosek_qptoprob(F, f, A, b, Aeq, beq, lb, ub);
 r <- mosek(prob);

 # Objective value is
 print(prob$c \%*\% r$sol$itr$xx);
}
\seealso{
\code{\link{mosek}}
\code{\link{mosek_lptoprob}}
}
