#pragma once
#include <pebble.h>

typedef struct {
  GColor bg;
  GColor panel;
  GColor fg;
  GColor accent;
  GColor bar;
  uint8_t hour_mode;  // 0=system, 1=12h, 2=24h
  bool show_weekday;
  bool show_xyz;
  bool show_remaining;
  bool show_progress;
  bool fan_spin;
  bool show_feedrate;
} FaceSettings;

void settings_init(void);
void settings_deinit(void);
const FaceSettings *settings_get(void);
