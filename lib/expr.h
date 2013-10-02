#ifndef GRN_EXPR_H
#define GRN_EXPR_H

#include "db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCAN_ACCESSOR                  (0x01)
#define SCAN_PUSH                      (0x02)
#define SCAN_POP                       (0x04)
#define SCAN_PRE_CONST                 (0x08)

typedef enum {
  SCAN_START = 0,
  SCAN_VAR,
  SCAN_COL1,
  SCAN_COL2,
  SCAN_CONST
} scan_stat;

typedef struct _grn_scan_info scan_info;
typedef grn_bool (*grn_scan_info_each_arg_callback)(grn_ctx *ctx, grn_obj *obj, void *user_data);

scan_info *grn_scan_info_alloc(grn_ctx *ctx, int start);
void grn_scan_info_free(grn_ctx *ctx, scan_info *si);
void grn_scan_info_put_index(grn_ctx *ctx, scan_info *si, grn_obj *index,
                             uint32_t sid, int32_t weight);
grn_bool grn_scan_info_check_flags(scan_info *si, int flags);
void grn_scan_info_reset_flags(scan_info *si, int flags);
int grn_scan_info_get_flags(scan_info *si);
void grn_scan_info_set_flags(scan_info *si, int flags);
void grn_scan_info_unset_flags(scan_info *si, int flags);
grn_operator grn_scan_info_get_logical_op(scan_info *si);
void grn_scan_info_set_logical_op(scan_info *si, grn_operator logical_op);
grn_operator grn_scan_info_get_op(scan_info *si);
void grn_scan_info_set_op(scan_info *si, grn_operator op);
void grn_scan_info_set_end(scan_info *si, uint32_t end);
void grn_scan_info_set_query(scan_info *si, grn_obj *query);
grn_bool grn_scan_info_push_arg(scan_info *si, grn_obj *arg);
void grn_scan_info_each_arg(grn_ctx *ctx, scan_info *si,
                                grn_scan_info_each_arg_callback callback,
                                void *user_data);

int32_t grn_expr_code_get_weight(grn_ctx *ctx, grn_expr_code *ec);

#ifdef __cplusplus
}
#endif

#endif /* GRN_EXPR_H */
