#ifndef GRN_EXPR_H
#define GRN_EXPR_H

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

#ifdef __cplusplus
}
#endif

#endif /* GRN_EXPR_H */
