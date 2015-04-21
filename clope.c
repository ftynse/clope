#include <osl/osl.h>
#include <osl/extensions/loop.h>

#include <candl/candl.h>

#include <clay/beta.h>
#include <clay/array.h>
#include <clay/list.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clope.h"

char *clope_strdup(const char *const str) {
  if (!str)
    return NULL;
  size_t length = strlen(str);
  char *clone = (char *) malloc(length + 1);
  strncpy(clone, str, length);
  return clone;
}

char *clope_strcat(char *str, const char *const extra) {
  size_t str_length = strlen(str);
  size_t extra_length = strlen(extra);
  str = (char *) realloc(str, str_length + extra_length + 1);
  assert (str != 0);
  strncpy(str + str_length, extra, extra_length);
  return str;
}

clope_index_match_p clope_index_match_create() {
  clope_index_match_p match = (clope_index_match_p) malloc(sizeof(clope_index_match_t));
  match->index = -1;
  match->beta = NULL;
  match->loop = NULL;
  match->next = NULL;
  return match;
}

void clope_index_match_destroy(clope_index_match_p match) {
  clope_index_match_p m = match;
  while (m != NULL) {
    m = match->next;
    if (match->beta)
      clay_array_free(match->beta);
    if (match->loop) {
      match->loop->next = NULL;
      osl_loop_free(match->loop);
    }
    free(match);
    match = m;
  }
}

clope_index_match_p clope_index_match_append_sorted(clope_index_match_p head, clope_index_match_p element) {
  clope_index_match_p original_head = head;

  if (!head) {
    return element;
  }
  if (!(head->next)) {
    if (head->index < element->index) {
      head->next = element;
      return head;
    } else {
      element->next = head;
      return element;
    }
  }
  while (head->next != NULL && head->next->index < element->index) {
    head = head->next;
  }
  element->next = head->next;
  head->next = element;
  return original_head;
}

clope_index_match_p clope_index_match_find_index(int index, clope_index_match_p head) {
  for ( ; head != NULL; head = head->next) {
    if (head->index == index)
      return head;
  }
  return NULL;
}

clope_index_match_p clope_index_match_find_beta(clay_array_p beta, clope_index_match_p head) {
  for ( ; head != NULL; head = head->next) {
    if (clay_array_equal(beta, head->beta))
      return head;
  }
  return NULL;
}

int clope_characterize_beta(osl_scop_p scop, clay_array_p beta, osl_statement_p *stmt) {
  osl_statement_p statement;
  clay_array_p current_beta;
  candl_statement_usr_p stmt_usr;
  int matching;

  if (stmt)
    *stmt = NULL;
  for (statement = scop->statement; statement != NULL; statement = statement->next) {
    current_beta = clay_beta_extract(statement->scattering);
    matching = matchingLength(current_beta, beta);
    if (matching != beta->size)
      continue;
    if (matching == current_beta->size) // not a loop
      return -1;

    stmt_usr = (candl_statement_usr_p) statement->usr;
    if (stmt)
      *stmt = statement;
    return stmt_usr->index[beta->size - 1];
  }

  return -1;
}

int clope_statement_number(osl_scop_p scop, clay_array_p beta) {
  osl_statement_p statement = scop->statement;
  clay_array_p current_beta;
  int length;
  int statement_number = 0;

  for ( ; statement != NULL; statement = statement->next) {
    current_beta = clay_beta_extract(statement->scattering);
    length = matchingLength(beta, current_beta);
    if (length == beta->size) {
      if (length == current_beta->size) {
        return 1; // This is a statement, we must have onlyo one statement with a given beta.
      }
      statement_number++;
    }
  }
  return statement_number;
}

clay_array_p clope_statement_ids(osl_scop_p scop, clay_array_p beta) {
  clay_array_p statement_ids = clay_array_malloc();
  osl_statement_p statement = scop->statement;
  clay_array_p current_beta;
  int length;
  candl_statement_usr_p stmt_usr;

  for ( ; statement != NULL; statement = statement->next) {
    current_beta = clay_beta_extract(statement->scattering);
    length = matchingLength(beta, current_beta);
    if (length == beta->size) {
      stmt_usr = (candl_statement_usr_p) statement->usr;
      clay_array_add(statement_ids, stmt_usr->label + 1); // XXX: assuming candl lables are equal to IDs CLooG expects.
      if (length == current_beta->size) {
        return statement_ids; // This is a statement.
      }
    }
  }
  return statement_ids;
}

clope_index_match_p clope_bulid_index_match(osl_scop_p scop, clay_list_p all_betas) {
  int i, index;
  clope_index_match_p match_list = NULL, current_match;
  osl_statement_p statement;
  clay_array_p statement_ids;
  osl_scatnames_p scatnames =
    (osl_scatnames_p) osl_generic_lookup(scop->extension, OSL_URI_SCATNAMES);
  osl_strings_p iterator_names;
  osl_names_p names = NULL;
  if (scatnames) {
    iterator_names = scatnames->names;
  } else {
    names = osl_scop_names(scop);
    iterator_names = names->scatt_dims;
  }

  for (i = 0; i < all_betas->size; i++) {
    index = clope_characterize_beta(scop, all_betas->data[i], &statement);
    current_match = clope_index_match_create();
    current_match->index = index;
    current_match->beta = clay_array_clone(all_betas->data[i]);
    current_match->loop = osl_loop_malloc();
    char *itername = NULL;
    if (osl_strings_size(iterator_names) >= current_match->beta->size) {
      itername = iterator_names->string[2 * current_match->beta->size - 1];
    }
    assert(itername != NULL);
    current_match->loop->iter = clope_strdup(itername);

    for (int j = 1; j < current_match->beta->size - 1; j += 2) {
      if (j == 1) {
        current_match->loop->private_vars = clope_strdup(iterator_names->string[j]);
      } else {
        current_match->loop->private_vars =
          clope_strcat(current_match->loop->private_vars,
                       iterator_names->string[i]);
      }
    }

    statement_ids = clope_statement_ids(scop, all_betas->data[i]);
    current_match->loop->nb_stmts = statement_ids->size;
    current_match->loop->stmt_ids = (int *) malloc(current_match->loop->nb_stmts * sizeof(int));
    memcpy(current_match->loop->stmt_ids, statement_ids->data, current_match->loop->nb_stmts * sizeof(int));
    clay_array_free(statement_ids);
    match_list = clope_index_match_append_sorted(match_list, current_match);
  }

  if (names)
    osl_names_free(names);
  return match_list;
}

void clope_index_match_annotate(clope_index_match_p head, clay_list_p parallel_betas) {
  for ( ; head != NULL; head = head->next) {
    if (clay_list_contains(parallel_betas, head->beta)) {
      head->loop->directive = OSL_LOOP_DIRECTIVE_PARALLEL;
    }
  }
}

osl_loop_p clope_generate_osl_loop_(osl_scop_p scop, clope_index_match_p mapping) {
  osl_loop_p loops, loop_ptr;

  if (!mapping || !scop)
    return NULL;

  loop_ptr = mapping->loop;
  loops = mapping->loop;
  
  mapping = mapping->next;
  for ( ; mapping != NULL; mapping = mapping->next) {
    loop_ptr->next = mapping->loop;
    loop_ptr = loop_ptr->next;
  }

  return loops;
}

void clope_generate_osl_loop(osl_scop_p scop) {
  if (!scop)
    return;

  clay_list_p parallelLoopBetas = clope_parallel_loop_betas(scop);
  clay_list_p allLoopBetas = clope_all_loop_betas(scop);
  clope_index_match_p mapping = clope_bulid_index_match(scop, allLoopBetas);
  clope_index_match_annotate(mapping, parallelLoopBetas);
  osl_loop_p loops = clope_generate_osl_loop_(scop, mapping);
  osl_generic_p loopsGeneric = osl_generic_shell(osl_loop_clone(loops), osl_loop_interface());
  osl_generic_add(&scop->extension, loopsGeneric);

  clope_index_match_destroy(mapping);
  clay_list_free(allLoopBetas);
  clay_list_free(parallelLoopBetas);
}


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
  return clope_characterize_beta(scop, beta, NULL);
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
  candl_scop_usr_init(scop);
  clope_generate_osl_loop(scop);
  candl_scop_usr_cleanup(scop);
  osl_scop_print(stdout, scop);
  return 0;
}
