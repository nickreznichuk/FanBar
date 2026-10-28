// Read-only access to fan speeds (AppleSMC) and SoC die temperatures (IOHID).
#pragma once

int smc_open(void);
int fan_count(void);
// suffix: "Ac" actual, "Tg" target, "Mn" min, "Mx" max. Returns -1 on error.
double fan_value(int fan, const char *suffix);
// Hottest "tdie" sensor in °C, or -1 if none found.
double max_die_temp(void);
