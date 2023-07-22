#include "profile.h"

u32 MetricAvg(Metric *metric)
{
  u32 avg = 0;
  for (u32 i = 0; i < VecCount(metric->samples); i++) {
    avg += metric->samples[i];
  }
  return avg / VecCount(metric->samples);
}

void PrintMetrics(Metric *metrics)
{
  u32 max_len = 0;
  for (u32 i = 0; i < VecCount(metrics); i++) {
    u32 len = StrLen(metrics[i].name)+1;
    max_len = Max(len, max_len);
  }

  char *title = "╴Metrics╶";
  Print("┌─");
  Print(title);
  for (u32 i = 0; i < max_len + 6 - NumChars(title); i++) Print("─");
  Print("┐\n");

  for (u32 i = 0; i < VecCount(metrics); i++) {
    u32 avg = MetricAvg(&metrics[i]);
    if (avg == 0) continue;

    Print("│");
    PrintN(metrics[i].name, max_len);
    Print("│");
    PrintIntN(avg, 6, ' ');
    Print("│\n");
  }
  Print("└");
  for (u32 i = 0; i < max_len; i++) Print("─");
  Print("┴");
  for (u32 i = 0; i < 6; i++) Print("─");
  Print("┘\n");

}
