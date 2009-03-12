/* -*- c-file-style: "gnu" -*- */

#include <groonga.h>

void groonga_test_fixtures_warmup(void);
void groonga_test_fixtures_cooldown(void);

void
groonga_test_fixtures_warmup(void)
{
  grn_init();
}

void
groonga_test_fixtures_cooldown(void)
{
  grn_fin();
}
