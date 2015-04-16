#include <osl/osl.h>

#include <candl/candl.h>

#include <clay/beta.h>
#include <clay/array.h>
#include <clay/list.h>

#include <stdlib.h>
#include <stdio.h>

int matchingLength(clay_array_p first, clay_array_p second) {
  int i, matching = 0;
  int minsize = first->size < second->size ? first->size : second->size;
  for (i = 0; i < minsize; i++) {
    if (first->data[i] == second->data[i])
      matching++;
    else
      break;
  }
  return matching;
}

clay_array_p clope_clay_sub_array(clay_array_p array, int length) {
  int i;
  clay_array_p sub;

  if (length < 0)
    return NULL;
  sub = clay_array_malloc();
  for (i = 0; i < length; i++) {
    clay_array_add(sub, array->data[i]);
  }
  return sub;
}

clay_list_p clope_all_loop_betas(osl_scop_p scop) {
  osl_statement_p statement;
  osl_relation_p scattering;
  clay_list_p allLoopBetas = clay_list_malloc();

  // Find all unique beta-loops.
  for (statement = scop->statement; statement != NULL; statement = statement->next) {
    for (scattering = statement->scattering; scattering != NULL; scattering = scattering->next) {
      clay_array_p beta = clay_beta_extract(scattering);
      clay_array_remove_last(beta);
      while (beta->size > 0) {
        if (!clay_list_contains(allLoopBetas, beta))
          clay_list_add(allLoopBetas, clay_array_clone(beta));
        clay_array_remove_last(beta);
      }
      clay_array_free(beta);
    }
  }

  return allLoopBetas;
}

int clope_loop_index_from_beta(osl_scop_p scop, clay_array_p beta) {
  osl_statement_p statement;
  clay_array_p current_beta;
  candl_statement_usr_p stmt_usr;
  int matching;

  for (statement = scop->statement; statement != NULL; statement = statement->next) {
    current_beta = clay_beta_extract(statement->scattering);
    matching = matchingLength(current_beta, beta);
    if (matching != beta->size)
      continue;
    if (matching == current_beta->size) // not a loop
      return -1;

    stmt_usr = (candl_statement_usr_p) statement->usr;
    return stmt_usr->index[beta->size - 1];
  }

  return -1;
}

clay_list_p clope_parallel_loop_betas(osl_scop_p scop) {
  // Parallelism condition of a given loop = all dependences where either source
  // or target statement, or both, have beta-prefix of this loop are not carried
  // by this loop.
  osl_dependence_p dependence, dependences;
  clay_list_p allLoopBetas, nonparallelLoopBetas, parallelLoopBetas;
  candl_options_p options;

  nonparallelLoopBetas = clay_list_malloc();
  parallelLoopBetas = clay_list_malloc();

  allLoopBetas = clope_all_loop_betas(scop);

  options = candl_options_malloc();
  options->fullcheck = 1;
  candl_scop_usr_init(scop);
  dependences = candl_dependence(scop, options);

  if (dependences)
    candl_dependence_init_fields(scop, dependences);
  // If a dependence between the statement in the loop is carried by that loop,
  // the loop is not parallel.
  for (dependence = dependences; dependence != NULL; dependence = dependence->next) {
    int i;
    clay_array_p sourceBeta, targetBeta;

    sourceBeta = clay_beta_extract(dependence->stmt_source_ptr->scattering);
    targetBeta = clay_beta_extract(dependence->stmt_target_ptr->scattering);

    int matching = matchingLength(sourceBeta, targetBeta);
    for (i = 0; i < matching; i++) {
      clay_array_p loop = clope_clay_sub_array(sourceBeta, i + 1); // sourceBeta and targetBeta are identical up to matching.
      int index = clope_loop_index_from_beta(scop, loop);
      int carried = 1;
      if (index != -1) // If index not found, assume carried.
        carried = candl_dependence_is_loop_carried(dependence, index);
      if (carried) {

        if (!clay_list_contains(nonparallelLoopBetas, loop)) {
          clay_list_add(nonparallelLoopBetas, loop);
        }
      }
    }
    clay_array_free(sourceBeta);
    clay_array_free(targetBeta);
  }
  candl_scop_usr_cleanup(scop);
  candl_options_free(options);

  for (int i = 0; i < allLoopBetas->size; i++) {
    if (!clay_list_contains(nonparallelLoopBetas, allLoopBetas->data[i]))
      clay_list_add(parallelLoopBetas, clay_array_clone(allLoopBetas->data[i]));
  }
  clay_list_free(allLoopBetas);
  clay_list_free(nonparallelLoopBetas);
  return parallelLoopBetas;
}

int main() {
  int i;
  osl_scop_p scop = osl_scop_read(stdin);
  clay_list_p allLoopBetas = clope_all_loop_betas(scop);
  clay_list_p parallelLoopBetas = clope_parallel_loop_betas(scop);

  printf("## PARALLELISM ANALYSIS ##\n");
  for (i = 0; i < allLoopBetas->size; i++) {
    clay_array_print(stdout, allLoopBetas->data[i], 0);
    if (clay_list_contains(parallelLoopBetas, allLoopBetas->data[i]))
      printf(" || parallel");
    printf("\n");
  }

  osl_scop_free(scop);
  clay_list_free(allLoopBetas);
  clay_list_free(parallelLoopBetas);
}
