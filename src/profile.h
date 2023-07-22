#pragma once

typedef struct {
  char *name;
  u32 *samples;
} Metric;

void PrintMetrics(Metric *metrics);
