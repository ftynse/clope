#include <osl/osl.h>
#include <osl/extensions/loop.h>

#include <candl/candl.h>
#include <candl/label_mapping.h>

#include <clay/beta.h>
#include <clay/array.h>
#include <clay/list.h>
#include <clay/util.h>

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

static int clope_compare_ints(const void *e1, const void *e2) {
  return *((int *)e1) - *((int *)e2);
}

static void clope_generate_osl_loop_f(osl_scop_p uscop,
                                      clay_list_p (*analyzer)(osl_scop_p)) {
  if (!uscop || !analyzer)
    return;

  // FIXME: assumes candl_scop_usr_init(uscop) was called before, verify
  osl_scop_p scop = candl_scop_remove_unions(uscop);
  candl_scop_usr_init(scop);
  candl_label_mapping_p label_mapping = candl_scop_label_mapping(scop);

  clay_list_p parallelLoopBetas = analyzer(scop);
  clay_list_p allLoopBetas = clope_all_loop_betas(scop);
  clope_index_match_p mapping = clope_bulid_index_match(scop, allLoopBetas);
  clope_index_match_annotate(mapping, parallelLoopBetas);
  osl_loop_p loops = clope_generate_osl_loop_(scop, mapping);

  // Remap loop extension
  for (osl_loop_p loop = loops; loop != NULL; loop = loop->next) {
    clay_array_p stmt_ids = clay_array_malloc();
    // for each statement mentioned in the loop,
    //   use the label mapping to change it to the original one, avoid duplicates
    for (int i = 0; i < loop->nb_stmts; i++) {
      int original = candl_label_mapping_find_original(label_mapping,
                                                       loop->stmt_ids[i] - 1) + 1;
      if (!clay_array_contains(stmt_ids, original)) {
        clay_array_add(stmt_ids, original);
      }
    }
    loop->nb_stmts = stmt_ids->size;
    loop->stmt_ids = (int *) realloc(loop->stmt_ids, stmt_ids->size * sizeof(int));
    memcpy(loop->stmt_ids, stmt_ids->data, stmt_ids->size * sizeof(int));
    clay_array_free(stmt_ids);

    // keep stmt ids sorted to facilitate later search
    qsort(loop->stmt_ids, loop->nb_stmts, sizeof(int), &clope_compare_ints);

  }

  // find loops with identical iterator names and statement sets (order not important),
  // keep only first one and remove the others while selecting the most conservative flag
  for (osl_loop_p loop = loops; loop != NULL; loop = loop->next) {
    if (loop->nb_stmts == -1) // skip flag
      continue;
    for (osl_loop_p other_loop = loop->next; other_loop != NULL;
         other_loop = other_loop->next) {
      if (other_loop->nb_stmts == -1)
        continue;
      if (strcmp(loop->iter, other_loop->iter) == 0 &&
          loop->nb_stmts == other_loop->nb_stmts &&
          memcmp(loop->stmt_ids, other_loop->stmt_ids, loop->nb_stmts * sizeof(int)) == 0) {
        // what to do with private vars?

        loop->directive = loop->directive & other_loop->directive;
        other_loop->nb_stmts = -1; // set skip flag
      }
    }
  }
  for (osl_loop_p loop = loops; loop != NULL && loop->next != NULL;
       loop = loop->next) {  // first loop is never skipped
    if (loop->next->nb_stmts == -1)
      loop->next = loop->next->next;
  }


  osl_generic_p loopsGeneric = osl_generic_shell(osl_loop_clone(loops), osl_loop_interface());
  osl_generic_remove(&uscop->extension, OSL_URI_LOOP);
  osl_generic_add(&uscop->extension, loopsGeneric);

  candl_label_mapping_free(label_mapping);
  candl_scop_usr_cleanup(scop);
  osl_scop_free(scop);

  clope_index_match_destroy(mapping);
  clay_list_free(allLoopBetas);
  clay_list_free(parallelLoopBetas);
}

void clope_generate_osl_loop(osl_scop_p scop) {
  clope_generate_osl_loop_f(scop, &clope_parallel_loop_betas);
}

void clope_generate_osl_loop_scheduled(osl_scop_p scop) {
  clope_generate_osl_loop_f(scop, &clope_scheduled_parallel_loop_betas);
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
  candl_options_p options;
  clay_list_p parallelLoopBetas;
  osl_dependence_p dependences;

  dependences = (osl_dependence_p) osl_generic_lookup(scop->extension, OSL_URI_DEPENDENCE);
  if (dependences) {
    parallelLoopBetas = clope_parallel_loop_betas_(scop, dependences);
  } else {
    options = candl_options_malloc();
    options->fullcheck = 1;
    dependences = candl_dependence(scop, options);
    if (dependences)
      candl_dependence_init_fields(scop, dependences);
    parallelLoopBetas = clope_parallel_loop_betas_(scop, dependences);
    candl_options_free(options);
  }

  return parallelLoopBetas;
}

clay_list_p clope_parallel_loop_betas_(osl_scop_p scop, osl_dependence_p dependences) {
  // Parallelism condition of a given loop = all dependences where either source
  // or target statement, or both, have beta-prefix of this loop are not carried
  // by this loop.
  osl_dependence_p dependence;
  clay_list_p allLoopBetas, nonparallelLoopBetas, parallelLoopBetas;

  nonparallelLoopBetas = clay_list_malloc();
  parallelLoopBetas = clay_list_malloc();

  allLoopBetas = clope_all_loop_betas(scop);

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

  for (int i = 0; i < allLoopBetas->size; i++) {
    if (!clay_list_contains(nonparallelLoopBetas, allLoopBetas->data[i]))
      clay_list_add(parallelLoopBetas, clay_array_clone(allLoopBetas->data[i]));
  }
  clay_list_free(allLoopBetas);
  clay_list_free(nonparallelLoopBetas);
  return parallelLoopBetas;
}

static inline clay_array_p clope_applied_extract_orig_beta(clay_array_p full_beta) {
  int i;
  clay_array_p beta = clay_array_malloc();
  int original_size = (full_beta->size + 1) / 3;
  for (i = 2 * original_size - 1; i < full_beta->size; i++) {
    clay_array_add(beta, full_beta->data[i]);
  }
  return beta;
}

clay_list_p clope_all_loop_betas_applied(osl_scop_p scop) {
  osl_statement_p stmt;
  clay_list_p all_betas = clay_list_malloc();

  for (stmt = scop->statement; stmt != NULL; stmt = stmt->next) {
    clay_array_p beta;
    clay_array_p full_beta = clay_beta_extract(stmt->scattering);
    if (!full_beta)
      continue;

    beta = clope_applied_extract_orig_beta(full_beta);
    clay_array_free(full_beta);

    if (beta->size <= 1)
      continue;
    clay_array_remove_last(beta);
    while (beta->size > 0) {
      if (!clay_list_contains(all_betas, beta)) {
        clay_list_add(all_betas, clay_array_clone(beta));
      }
      clay_array_remove_last(beta);
    }

    clay_array_free(beta);
  }
  return all_betas;
}

clay_list_p clope_parallel_loop_betas_applied_(osl_scop_p scop, osl_dependence_p dependences) {
  // Parallelism condition of a given loop = all dependences where either source
  // or target statement, or both, have beta-prefix of this loop are not carried
  // by this loop.
  osl_dependence_p dependence;
  clay_list_p parallelLoopOrigBetas, nonparallelLoopOrigBetas, allLoopOrigBetas;

  parallelLoopOrigBetas = clay_list_malloc();
  nonparallelLoopOrigBetas = clay_list_malloc();

  allLoopOrigBetas = clope_all_loop_betas_applied(scop);

  // If a dependence between the statement in the loop is carried by that loop,
  // the loop is not parallel.
  for (dependence = dependences; dependence != NULL; dependence = dependence->next) {
    int i;
    clay_array_p sourceBeta, targetBeta, sourceOrigBeta, targetOrigBeta;

    sourceBeta = clay_beta_extract(dependence->stmt_source_ptr->scattering);
    targetBeta = clay_beta_extract(dependence->stmt_target_ptr->scattering);

    sourceOrigBeta = clope_applied_extract_orig_beta(sourceBeta);
    targetOrigBeta = clope_applied_extract_orig_beta(targetBeta);

    int matching = matchingLength(sourceOrigBeta, targetOrigBeta);
    for (i = 1; i < 2*matching + 1; i += 2) {
      // The problem here is that sourceBeta is identical for all statements, a series of zeros.
      // I guess it should be shomehow spaced original beta instead to reflect different loops and check for carried
      // >Current version works, not sure why exactly (with all-zero beta prefixes, the carrying
      // >loop is the same for statements that were in different loops before, but, probably, is_loop_carried just cares
      // >the depth of the loop and not the actual betas).
      // >>but we integrated the old beta-dimensions in the domain, making loop separation clear with identity schedule,
      // >>therefore, no need for separating with schedule betas (and schedules are ignored in dependence computation
      // >>anyways)
      // What if we put the old beta [b1,b2,b3] as new [0,b1,0,b2,0,b3], looks more correct and properly separates stuff

      clay_array_p loop = clope_clay_sub_array(sourceBeta, i + 1); // sourceBeta and targetBeta are identical up to matching.
      int index = clope_loop_index_from_beta(scop, loop);
      int carried = 1;
      if (index != -1) // If index not found, assume carried.
        carried = candl_dependence_is_loop_carried(dependence, index);
      if (carried) {
        clay_array_p sourceLoop = clope_clay_sub_array(sourceOrigBeta, (i - 1) / 2 + 1);
        clay_array_p targetLoop = clope_clay_sub_array(targetOrigBeta, (i - 1) / 2 + 1);
        assert(clay_array_equal(sourceLoop, targetLoop) &&
               "Betas identifying the loop that carries dependence differ.");
        if (!clay_list_contains(nonparallelLoopOrigBetas, sourceLoop)) {
          clay_list_add(nonparallelLoopOrigBetas, sourceLoop);
        }
      }
    }
    clay_array_free(sourceBeta);
    clay_array_free(targetBeta);
    clay_array_free(sourceOrigBeta);
    clay_array_free(targetOrigBeta);
  }

  for (int i = 0; i < allLoopOrigBetas->size; i++) {
    if (!clay_list_contains(nonparallelLoopOrigBetas, allLoopOrigBetas->data[i]))
      clay_list_add(parallelLoopOrigBetas, clay_array_clone(allLoopOrigBetas->data[i]));
  }

  clay_list_free(allLoopOrigBetas);
  clay_list_free(nonparallelLoopOrigBetas);
  return parallelLoopOrigBetas;
}

static inline void clope_pad_osl_relation(osl_relation_p relation, int nb_dimensions) {
  for (int i = 0; i < nb_dimensions; i++) {
    osl_relation_insert_blank_column(relation, 1 + relation->nb_output_dims);
  }
  relation->nb_input_dims += nb_dimensions;
}

static inline osl_relation_p clope_identity_scattering(int nb_dimensions, int nb_parameters) {
  osl_relation_p scattering = osl_relation_pmalloc(osl_util_get_precision(),
                                                   2 * nb_dimensions + 1,
                                                   2 + nb_dimensions * 2 + 1 + nb_dimensions + nb_parameters);
  for (int i = 0; i < 2 * nb_dimensions + 1; i++) {
    osl_int_set_si(scattering->precision,
                   &scattering->m[i][1 + i],
                   -1);
    if ((i % 2) == 1) {
      osl_int_set_si(scattering->precision,
                     &scattering->m[i][1 + 2 * nb_dimensions + 1 + i/2],
                     1);
    }
  }
  scattering->nb_input_dims = nb_dimensions;
  scattering->nb_output_dims = 2 * nb_dimensions + 1;
  scattering->nb_parameters = nb_parameters;
  scattering->nb_local_dims = 0;
  scattering->type = OSL_TYPE_SCATTERING;
  return scattering;
}

osl_relation_p clope_apply_scattering(osl_statement_p stmt) {
  osl_relation_p domain, scattering, result;

  result = NULL;
  for (domain = stmt->domain; domain != NULL; domain = domain->next) {
    for (scattering = stmt->scattering; scattering != NULL;
         scattering = scattering->next) {
      int i, scattering_local_index, domain_local_index;
      osl_relation_p result_union_part;

      // Stash old sizes as we are going to resize the relations.
      int domain_output_dims = domain->nb_output_dims;
      int domain_input_dims = domain->nb_input_dims;
      int domain_local_dims = domain->nb_local_dims;
      int domain_parameters = domain->nb_parameters;
      int scattering_output_dims = scattering->nb_output_dims;
      int scattering_input_dims = scattering->nb_input_dims;
      int scattering_local_dims = scattering->nb_local_dims;
      int scattering_parameters = scattering->nb_parameters;

      // Prepare a domain relation for concatenation with a scattering relation.
      // Add columns to accomodate local dimensions of the scattering relation (l's).
      // Local dimensions are intrinsic to each relation and should not overlap.
      // Prepend local dimensions to the domain and append to the scattering.
      osl_relation_p domain_part = osl_relation_nclone(domain, 1);
      osl_relation_p scattering_part = osl_relation_nclone(scattering, 1);

      domain_local_index = 1 + domain_output_dims + domain_input_dims;
      for (i = 0; i < scattering_local_dims; i++) {
        osl_relation_insert_blank_column(domain_part, domain_local_index);
      }
      // Add columns to accommodate output dimensions of the scattering relation (c's).
      for (i = 0; i < scattering_output_dims; i++) {
        osl_relation_insert_blank_column(domain_part, 1);
      }
      domain_part->nb_input_dims  = scattering_input_dims;
      domain_part->nb_output_dims = scattering_output_dims;
      domain_part->nb_local_dims  = domain_local_dims + scattering_local_dims;
      domain_part->type           = OSL_TYPE_SCATTERING;

      // Append local dimensions to the scatteirng.
      scattering_local_index = 1 + scattering_part->nb_input_dims
          + scattering_part->nb_output_dims + scattering_part->nb_local_dims;
      for (i = 0; i < domain_local_dims; i++) {
        osl_relation_insert_blank_column(scattering_part, scattering_local_index);
      }
      scattering_part->nb_local_dims += domain_local_dims;

      // Create a scattered domain relation.
      result_union_part = osl_relation_concat_constraints(domain_part, scattering_part);
      result_union_part->nb_output_dims = scattering_input_dims + scattering_output_dims;
      result_union_part->nb_input_dims = 0;
      result_union_part->nb_local_dims = domain_local_dims + scattering_local_dims;
      result_union_part->nb_parameters = domain_parameters;
      result_union_part->type = OSL_TYPE_DOMAIN;
      result_union_part->next = NULL;
      osl_relation_integrity_check(result_union_part, OSL_TYPE_DOMAIN,
                                   scattering_input_dims + scattering_output_dims,
                                   0, scattering_parameters);
      osl_relation_add(&result, result_union_part);
      osl_relation_free(domain_part);
      osl_relation_free(scattering_part);
    }
  }
  return result;
}

static inline void clope_update_beta_tail(osl_relation_p scattering, clay_array_p beta) {
  int i;
  clay_array_p current_beta = clay_beta_extract(scattering);
  assert(beta->size <= current_beta->size && "Can update only a tail of beta");
  for (i = 0; i < beta->size; i++) {
    current_beta->data[current_beta->size - beta->size + i] = beta->data[i];
  }
  clay_util_scattering_update_beta(scattering, current_beta);
}

clay_list_p clope_scheduled_parallel_loop_betas(osl_scop_p scop) {
  osl_scop_p applied_scop = osl_scop_clone(scop);
  osl_statement_p applied_stmt = applied_scop->statement;
  int stmt_id = 0;
  for (osl_statement_p stmt = scop->statement; stmt != NULL;
       stmt = stmt->next, applied_stmt = applied_stmt->next, stmt_id++) {
    // Insert scheduled dimensions in the domain.
    osl_relation_p applied_domain = clope_apply_scattering(stmt);
    osl_relation_free(applied_stmt->domain);
    applied_stmt->domain = applied_domain;

    for (osl_relation_p scattering = stmt->scattering; scattering != NULL;
         scattering = scattering->next) {
      assert(scattering->nb_output_dims == stmt->scattering->nb_output_dims &&
             "Cannot extend access relations given unequal dimensionality of scatterings");
      // This needs oslStmtReify before running the dependence analysis
    }
    clay_array_p beta = clay_beta_extract(applied_stmt->scattering);
    osl_relation_free(applied_stmt->scattering);
    applied_stmt->scattering = clope_identity_scattering(stmt->scattering->nb_output_dims + stmt->scattering->nb_input_dims,
                                                         stmt->scattering->nb_parameters);
    clope_update_beta_tail(applied_stmt->scattering, beta);

    osl_generic_remove(&applied_stmt->extension, OSL_URI_BODY);

    // Insert scheduled dimensions in the access relations.
    for (osl_relation_list_p access = applied_stmt->access; access != NULL;
         access = access->next) {
      for (osl_relation_p rel = access->elt; rel != NULL; rel = rel->next) {
        clope_pad_osl_relation(rel, stmt->scattering->nb_output_dims);
      }
    }
  }

  osl_generic_remove(&applied_scop->extension, OSL_URI_SCATNAMES);
  candl_options_p options = candl_options_malloc();
  options->fullcheck = 1;
  osl_dependence_p dependence = candl_dependence(applied_scop, options);
  candl_options_free(options);

  clay_list_p betas = clope_parallel_loop_betas_applied_(applied_scop, dependence);
  osl_scop_free(applied_scop);
  osl_dependence_free(dependence);
  return betas;
}

int main() {
  osl_scop_p scop = osl_scop_read(stdin);
  candl_scop_usr_init(scop);
  clope_generate_osl_loop_scheduled(scop);
  candl_scop_usr_cleanup(scop);
  osl_scop_print(stdout, scop);
  return 0;
}
