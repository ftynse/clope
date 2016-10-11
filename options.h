#include <stdio.h>

struct clope_options {
  /**
   * Compute dependences before (0) or after (1) scheduling.  Deafult = 1.
   */
  int scheduled;

  /**
   * Use conservative approximation for generating parallel loops using
   * osl_loop extension (1) or reject unions (0).  Default = 0.
   */
  int conservative;

  /**
   * Filename for the output
   */
  char *output_filename;
};

typedef struct clope_options   clope_options_t;
typedef struct clope_options * clope_options_p;

clope_options_p clope_options_malloc();
void clope_options_free(clope_options_p);

void clope_options_pprint(FILE *, clope_options_p);
clope_options_p clope_options_parse(int *, char **);
void clope_options_usage(FILE *, int, char **);

