#pragma once

typedef struct GlobalStatus
{
  int enable;
  int mode;
} GlobalStatus_t;

void init_ctrltask();
