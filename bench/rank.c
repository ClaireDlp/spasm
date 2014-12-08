#include <stdio.h>
#include <assert.h>
#include <getopt.h>

#include "spasm.h"

#ifdef SPASM_TIMING
extern int64 reach, scatter, data_shuffling;
#endif


int main(int argc, char **argv) {
  spasm_triplet *T;
  spasm *A, *U, *L;
  spasm_lu *LU;
  int r, n, m, *p, ch, prime, allow_transpose, sort_strategy, keep_L;
  double start_time, end_time;

  prime = 42013;
  sort_strategy = 1; // cheap pivots by default
  allow_transpose = 1;
  keep_L = 0;

  /* options descriptor */
  struct option longopts[6] = {
    { "sort-rows",    no_argument,       NULL,          's' },
    { "keep-rows", no_argument,       NULL,             'k' },
    { "no-transpose", no_argument,       NULL,          'a' },
    { "modulus",      required_argument, NULL,          'p' },
    { "keep-L" ,      no_argument,       NULL,          'l' },
    { NULL,           0,                 NULL,           0  }
  };

  while ((ch = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
    switch (ch) {
    case 'p':
      prime = atoi(optarg);
      break;
    case 'k':
      sort_strategy = 0;
      break;
    case 't':
      break;
    case 'a':
      allow_transpose = 0;
      break;
    case 's':
      sort_strategy = 2;
      break;
    case 'l':
      keep_L = 1;
      break;
    default:
      printf("Unknown option\n");
      exit(1);
    }
  }
  argc -= optind;
  argv += optind;

  T = spasm_load_sms(stdin, prime);
  printf("A : %d x %d with %d nnz (density = %.3f %%) -- loaded modulo %d\n", T->n, T->m, T->nz, 100.0 * T->nz / (1.0 * T->n * T->m), prime);
  if (allow_transpose && (T->n < T->m)) {
    printf("[rank] transposing matrix\n");
    spasm_triplet_transpose(T);
  }
  A = spasm_compress(T);
  spasm_triplet_free(T);

  start_time = spasm_wtime();

  switch(sort_strategy) {
  case 0:
    p = NULL;
    break;
  case 1:
    p = spasm_cheap_pivots(A);
    break;
  case 2:
    p = spasm_row_sort(A);
    break;
  }

  LU = spasm_LU(A, p, keep_L);
  end_time = spasm_wtime();
  printf("\n");

  U = LU->U;
  r = U->n;
  n = A->n;
  m = A->m;

  printf("LU factorisation (+ sort took) %.2f s\n", end_time - start_time);
  printf("U :  %d x %d with %d nnz (density = %.1f %%)\n", r, m, spasm_nnz(U), 100.0 * spasm_nnz(U) / (r*m - r*r/2));
  if (LU->L != NULL) {
    L = LU->L;
    printf("L :  %d x %d with %d nnz (density =%.1f %%)\n", L->n, r, spasm_nnz(L), 100.0 * spasm_nnz(L) / (r*n - r*r/2));
  }

#ifdef SPASM_TIMING
  printf("----------------------------------------\n");
  printf("reach   : %12" PRId64 "\n", reach);
  printf("scatter : %12" PRId64 "\n", scatter);
  printf("misc    : %12" PRId64 "\n", data_shuffling);
#endif
  printf("----------------------------------------\n");
  printf("rank of A = %d\n", U->n);
  spasm_free_LU(LU);


  spasm_csr_free(A);
  return 0;
}
