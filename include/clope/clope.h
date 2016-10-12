#ifndef CLOPE_H
#define CLOPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <osl/osl.h>
#include <osl/extensions/loop.h>

#include <clay/array.h>
#include <clay/list.h>

/*+*********** beta-vector related functions ******************/
clay_list_p  clope_all_loop_betas(osl_scop_p);
clay_list_p  clope_parallel_loop_betas(osl_scop_p);
clay_list_p  clope_scheduled_parallel_loop_betas(osl_scop_p);

/*+*********** high-level processing functions ****************/
void         clope_generate_osl_loop(osl_scop_p);
void         clope_generate_osl_loop_scheduled(osl_scop_p);

/*+*********** general utility functions **********************/
char *clope_strdup(const char *const str);
char *clope_strcat(char *str, const char *const extra);

#ifdef __cplusplus
}
#endif

#endif

