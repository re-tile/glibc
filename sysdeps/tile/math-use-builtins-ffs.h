/* TILE-Gx has a hardware count-trailing-zeros (ctz), so GCC lowers
   __builtin_ffs/__builtin_ffsll inline with no libcall.  Route the generic
   string/ffs.c and string/ffsll.c through the builtins instead of carrying an
   arch override (the old sysdeps/tile/ffsll.c tripped -Winfinite-recursion).  */
#define USE_FFS_BUILTIN   1
#define USE_FFSLL_BUILTIN 1
