#include "options.h"
#include "index_match.h"

#include "clope/clope.h"

int main(int argc, char **argv) {
  FILE *input;
  FILE *output;
  osl_scop_p scop;

  clope_options_p options = clope_options_parse(&argc, argv);
  if (!options) {
    clope_options_usage(stderr, argc, argv);
    return 1;
  }
  if (argc > 2) {
    clope_options_usage(stderr, argc, argv);
    return 2;
  }

  // Standard input.
  if (argc == 1) {
    input = stdin;
  } else {
    if (!(input = fopen(argv[1], "r"))) {
      fprintf(stderr, "Could not open file %s\n", argv[1]);
      return 3;
    }
  }

  scop = osl_scop_read(input);
  if (input != stdin) {
    fclose(input);
  }
  if (!scop) {
    fprintf(stderr, "Could not read SCoP file %s\n", argv[1]);
    return 4;
  }

  if (clope_scop_has_unions(scop) &&
      !options->conservative) {
    fprintf(stderr, "Cannot generate loop extension for SCoP with union of\n"
                    "relations scattering without `--conservative 1'.\n");
    osl_scop_free(scop);
    return 5;
  }

  if (options->scheduled) {
    clope_generate_osl_loop_scheduled(scop);
  } else {
    clope_generate_osl_loop(scop);
  }

  if (!options->output_filename) {
    output = stdout;
  } else {
    if (!(output = fopen(options->output_filename, "w"))) {
      fprintf(stderr, "Could not open file %s for writing\n",
              options->output_filename);
      osl_scop_free(scop);
      return 6;
    }
  }

  osl_scop_print(output, scop);
  osl_scop_free(scop);

  if (output != stdout)
    fclose(output);

  return 0;
}

