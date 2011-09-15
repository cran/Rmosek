.onLoad =
function(libname, pkgname)
{
  dll <- library.dynam("rmosek", pkgname) 
 
  syms = getNativeSymbolInfo(c("mosek", "mosek_clean", "mosek_version", "mosek_read", "mosek_write"), dll)

  # Create symbols in this package
  env = environment(.onLoad)
  sapply(names(syms), function(id) assign( paste(id,"_sym",sep="") , syms[[id]], env))
}

mosek = 
function(problem, opts=list())
{
  if (nargs() < 1) {
    print(?mosek)
    stop("Invalid number of arguments")
  }

  # Force evaluation of mandatory objects
  problem;

  r <- try(.Call(mosek_sym, problem, opts), silent = TRUE)

  if (inherits(r, "try-error")) {
    .Call(mosek_clean_sym)
    stop(r)
  }

  return(r)
}

mosek_clean = 
function()
{
  .Call(mosek_clean_sym) 
  return(invisible(NULL))
}

mosek_version =
function()
{
  r <- .Call(mosek_version_sym)
  return(r)
}


mosek_read = 
function(modelfile, opts=list())
{
  if (nargs() < 1) {
    print(?mosek_read)
    stop("Invalid number of arguments")
  }
 
  # Force evaluation of mandatory objects
  modelfile;
 
  r <- try(.Call(mosek_read_sym, modelfile, opts), silent = TRUE)

  if (inherits(r, "try-error")) {
    .Call(mosek_clean_sym)
    stop(r)
  }

  return(r)
}

mosek_write = 
function(problem, modelfile, opts=list())
{
  if (nargs() < 2) {
    print(?mosek_read)
    stop("Invalid number of arguments")
  }

  # Force evaluation of mandatory objects
  problem; modelfile;

  r <- try(.Call(mosek_write_sym, problem, modelfile, opts), silent = TRUE)

  if (inherits(r, "try-error")) {
    .Call(mosek_clean_sym)
    stop(r)
  }

  return(r)
}
