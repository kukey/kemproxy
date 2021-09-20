#ifndef NC_MONITOR_H
#define NC_MONITOR_H

#include <nc_core.h>

void monitor_init();
void monitor_deinit(struct context *ctx);
int mointor_is_empty();

rstatus_t add_to_monitor(struct conn *c);
void del_from_monitor(struct conn *c);
rstatus_t make_monitor(struct context *ctx, struct conn *c, struct msg *m);

#endif