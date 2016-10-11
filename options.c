#include "options.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Allocates a new instance of \ref clope_options, initializes it with default
 * values and returns a pointer to it.
 * Returns \c NULL in case of memory allocation failure.
 * \return Pointer to the new instance of clope_options.
 */
clope_options_p clope_options_malloc() {
  clope_options_p options = (clope_options_p) malloc(sizeof(clope_options_t));
  if (!options)
    return options;
  options->scheduled = 1;
  options->conservative = 0;
  options->output_filename = NULL;
  return options;
}

/**
 * Deallocates memory used by an instance of \ref clope_options.
 * Safe with \c NULL pointers.
 * \param [in] options Pointer to the instance of \ref clope_options.
 */
void clope_options_free(clope_options_p options) {
  free(options->output_filename);
  free(options);
}

/**
 * Prints annonated options to the given file.
 * \param [in] f       Destination file.
 * \param [in] options Instance of \ref clope_options to print.
 */
void clope_options_pprint(FILE *f, clope_options_p options) {
  fprintf(f, "# Output filename\n%s\n",
          (options->output_filename == NULL ? "(nil)" :
                                              options->output_filename));
  fprintf(f, "# Compute dependences before (0) or after (1) scheduling.\n%d\n",
          options->scheduled);
  fprintf(f, "# Use conservative approximation for parallel loops given\n"
             "# union of scattering relations (1) or reject unisons (0).\n%d\n",
          options->conservative);
}

clope_options_p clope_options_parse(int *argc, char **argv) {
  int i;
  clope_options_p options = clope_options_malloc();
  
  for (i = 1; i < *argc; ++i) {
    if (strcmp(argv[i], "--conservative") == 0) {
      if (*argc <= i+1)
        return 0; //error!    

      if (strcmp(argv[i + 1], "0") == 0)
        options->conservative = 0;
      else if (strcmp(argv[i + 1], "1") == 0)
        options->conservative = 1;
      else
        return 0; //error!

      memmove(&argv[i], &argv[i+2], sizeof(char *) * (*argc - i - 2));
      *argc -= 2;
      i--;
      continue;
    }

    if (strcmp(argv[i], "--scheduled") == 0) {
      if (*argc <= i+1)
        return 0; //error!    

      if (strcmp(argv[i + 1], "0") == 0)
        options->scheduled = 0;
      else if (strcmp(argv[i + 1], "1") == 0)
        options->scheduled = 1;
      else
        return 0; //error!

      memmove(&argv[i], &argv[i+2], sizeof(char *) * (*argc - i - 2));
      *argc -= 2;
      i--;
      continue;
    }

    if (strcmp(argv[i], "--output") == 0 ||
        strcmp(argv[i], "-o") == 0) {
      if (*argc <= i+1)
        return 0; //error!

      options->output_filename = malloc(strlen(argv[i + 1]) + 1);
      strcpy(options->output_filename, argv[i + 1]);

      memmove(&argv[i], &argv[i+2], sizeof(char *) * (*argc - i - 2));
      *argc -= 2;
      i--;
      continue;
    }
  }
  return options;
}

void clope_options_usage(FILE *f, int argc, char **argv) {
  if (argc >= 1)
    fprintf(f, "Usage: %s [options] [filename]\n", argv[0]);
  else
    fprintf(f, "Usage ./clope [options] [filename]\n");

  fprintf(f,
"If filename is omitted, clope reads standard input.\n"
"\n"
"Options:\n"
"   --output        [name] Output filename.\n"
"   -o              [name] (default: print to standard output)\n"
"\n"
"   --scheduled     (0|1)  Compute dependences before (0) or after (1)\n"
"                          scheduling the domain using the scattering relation.\n"
"                          (default: 1)\n"
"\n"
"   --conservative  (0|1)  Use conservative approximation for parallel loops\n"
"                          if the scattering is a union of relations.\n"
"                          (default: 0)\n");

}
