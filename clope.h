#ifndef CLOPE_H
#define CLOPE_H

#include <osl/osl.h>
#include <osl/extensions/loop.h>

#include <clay/array.h>
#include <clay/list.h>

/// Keeps a correspondance between an internal zero-based loop index (similar
/// to CLooG), a beta-vector and osl_loop structure.
struct clope_index_match {
  int index;
  clay_array_p beta;
  osl_loop_p loop;
  struct clope_index_match *next;
};
typedef struct clope_index_match  clope_index_match_t;
typedef struct clope_index_match *clope_index_match_p;

/*+********** index_match related functions *******************/
clope_index_match_p clope_index_match_create();
void                clope_index_match_destroy(clope_index_match_p);
clope_index_match_p clope_index_match_append_sorted(clope_index_match_p, clope_index_match_p);
clope_index_match_p clope_index_match_find_index(int, clope_index_match_p);
clope_index_match_p clope_index_match_find_beta(clay_array_p, clope_index_match_p);
clope_index_match_p clope_bulid_index_match(osl_scop_p, clay_list_p);
void                clope_index_match_annotate(clope_index_match_p, clay_list_p);

/*+********** internal index related functions ****************/
int          clope_characterize_beta(osl_scop_p, clay_array_p, osl_statement_p *);
int          clope_loop_index_from_beta(osl_scop_p, clay_array_p);
int          clope_statement_number(osl_scop_p, clay_array_p);
clay_array_p clope_statement_ids(osl_scop_p, clay_array_p);

/*+*********** beta-vector related functions ******************/
int          matchingLength(clay_array_p, clay_array_p);
clay_array_p clope_clay_sub_array(clay_array_p, int);
clay_list_p  clope_all_loop_betas(osl_scop_p);
clay_list_p  clope_parallel_loop_betas(osl_scop_p);
clay_list_p  clope_parallel_loop_betas_(osl_scop_p, osl_dependence_p);
clay_list_p  clope_scheduled_parallel_loop_betas(osl_scop_p);

/*+*********** high-level processing functions ****************/
osl_loop_p   clope_generate_osl_loop_(osl_scop_p, clope_index_match_p);
void         clope_generate_osl_loop(osl_scop_p);
void         clope_generate_osl_loop_scheduled(osl_scop_p);

/*+*********** general utility functions **********************/
char *clope_strdup(const char *const str);
char *clope_strcat(char *str, const char *const extra);

#endif

