#include "settings.h"
#include "face.h"
#include <stdlib.h>

#define SETTINGS_KEY 1

static FaceSettings s_settings;

static void defaults(void) {
#if defined(PBL_COLOR)
  s_settings.bg = GColorOxfordBlue;
  s_settings.panel = GColorDukeBlue;
  s_settings.fg = GColorWhite;
  s_settings.accent = GColorPictonBlue;
  s_settings.bar = GColorWhite;
#else
  s_settings.bg = GColorBlack;
  s_settings.panel = GColorBlack;
  s_settings.fg = GColorWhite;
  s_settings.accent = GColorWhite;
  s_settings.bar = GColorWhite;
#endif
  s_settings.hour_mode = 0;
  s_settings.show_weekday = true;
  s_settings.show_xyz = true;
  s_settings.show_remaining = true;
  s_settings.show_progress = true;
  s_settings.fan_spin = true;
  s_settings.show_feedrate = true;
}

static void save(void) {
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static void load(void) {
  defaults();
  persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static int read_int(DictionaryIterator *iter, uint32_t key, int fallback) {
  Tuple *t = dict_find(iter, key);
  if (!t) {
    return fallback;
  }
  if (t->type == TUPLE_CSTRING) {
    return atoi(t->value->cstring);
  }
  return (int)t->value->int32;
}

static bool read_bool(DictionaryIterator *iter, uint32_t key, bool fallback) {
  Tuple *t = dict_find(iter, key);
  if (!t) {
    return fallback;
  }
  return t->value->int32 == 1;
}

static void read_color(DictionaryIterator *iter, uint32_t key, GColor *out) {
  Tuple *t = dict_find(iter, key);
  if (!t) {
    return;
  }
  *out = GColorFromHEX(t->value->int32);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  read_color(iter, MESSAGE_KEY_BackgroundColor, &s_settings.bg);
  read_color(iter, MESSAGE_KEY_PanelColor, &s_settings.panel);
  read_color(iter, MESSAGE_KEY_TextColor, &s_settings.fg);
  read_color(iter, MESSAGE_KEY_AccentColor, &s_settings.accent);
  read_color(iter, MESSAGE_KEY_BarColor, &s_settings.bar);

  s_settings.hour_mode = (uint8_t)read_int(iter, MESSAGE_KEY_HourMode, s_settings.hour_mode);
  s_settings.show_weekday = read_bool(iter, MESSAGE_KEY_ShowWeekday, s_settings.show_weekday);
  s_settings.show_xyz = read_bool(iter, MESSAGE_KEY_ShowXYZ, s_settings.show_xyz);
  s_settings.show_remaining = read_bool(iter, MESSAGE_KEY_ShowRemaining, s_settings.show_remaining);
  s_settings.show_progress = read_bool(iter, MESSAGE_KEY_ShowProgress, s_settings.show_progress);
  s_settings.fan_spin = read_bool(iter, MESSAGE_KEY_FanSpin, s_settings.fan_spin);
  s_settings.show_feedrate = read_bool(iter, MESSAGE_KEY_ShowFeedrate, s_settings.show_feedrate);

  save();
  face_settings_changed();
}

const FaceSettings *settings_get(void) {
  return &s_settings;
}

void settings_init(void) {
  load();
  app_message_register_inbox_received(inbox_received);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void settings_deinit(void) {
  // nothing persistent beyond what save() already wrote
}
