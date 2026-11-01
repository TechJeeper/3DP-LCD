#include "face.h"
#include "settings.h"

#define FRAME_MS 120
#define FAN_FRAMES 4
#define PANEL_PAD 8

static Window *s_window;
static Layer *s_layer;
static AppTimer *s_timer;

static int s_hour;
static int s_minute;
static int s_second;
static int s_month;   // 1..12
static int s_mday;    // 1..31
static int s_wday;    // 0=Sun
static int s_fan_frame;

static GBitmap *s_bmp_nozzle;
static GBitmap *s_bmp_bed;
static GBitmap *s_bmp_fan[FAN_FRAMES];

static GColor color_bg(void) {
  return settings_get()->bg;
}

static GColor color_panel(void) {
  return settings_get()->panel;
}

static GColor color_fg(void) {
  return settings_get()->fg;
}

static GColor color_dim(void) {
  return settings_get()->accent;
}

static GColor color_bar(void) {
  return settings_get()->bar;
}

static int display_hour(int hour) {
  uint8_t mode = settings_get()->hour_mode;
  bool use_12h = (mode == 1) || (mode == 0 && !clock_is_24h_style());
  if (mode == 2) {
    use_12h = false;
  }
  if (use_12h) {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
  }
  return hour;
}

// Fraction of the calendar day elapsed, in percent 0..100.
static int day_progress_pct(void) {
  int mins = s_hour * 60 + s_minute;
  int pct = (mins * 100) / (24 * 60);
  if (pct < 0) {
    return 0;
  }
  if (pct > 100) {
    return 100;
  }
  return pct;
}

static void draw_panel_frame(GContext *ctx, GRect panel) {
  graphics_context_set_fill_color(ctx, color_panel());
  graphics_fill_rect(ctx, panel, 4, GCornersAll);

  graphics_context_set_stroke_color(ctx, color_dim());
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_round_rect(ctx, panel, 4);

  graphics_context_set_stroke_color(ctx, color_fg());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx,
      GPoint(panel.origin.x + 3, panel.origin.y + 2),
      GPoint(panel.origin.x + panel.size.w - 4, panel.origin.y + 2));
}

static void draw_icon(GContext *ctx, GBitmap *bmp, GPoint origin) {
  if (!bmp) {
    return;
  }
#if defined(PBL_COLOR)
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
#endif
  graphics_draw_bitmap_in_rect(ctx, bmp,
      GRect(origin.x, origin.y, gbitmap_get_bounds(bmp).size.w, gbitmap_get_bounds(bmp).size.h));
}

static void draw_progress_bar(GContext *ctx, GRect bar, int pct) {
  graphics_context_set_stroke_color(ctx, color_fg());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, bar);

  int inner_w = bar.size.w - 4;
  int fill = (inner_w * pct) / 100;
  if (fill < 0) {
    fill = 0;
  }
  if (fill > inner_w) {
    fill = inner_w;
  }
  if (fill > 0) {
    graphics_context_set_fill_color(ctx, color_bar());
    graphics_fill_rect(ctx,
        GRect(bar.origin.x + 2, bar.origin.y + 2, fill, bar.size.h - 4), 0, GCornerNone);
  }
}

static const char *weekday_name(int wday) {
  static const char *names[7] = {
    "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"
  };
  if (wday < 0 || wday > 6) {
    return "TODAY";
  }
  return names[wday];
}

static void draw_temp_row(GContext *ctx, GRect content, int y, GBitmap *icon,
                          const char *label, const char *value, const char *right,
                          GFont value_font, int value_h, int icon_s, int fan_s,
                          bool show_fan) {
  int text_x = content.origin.x + icon_s + 4;
  int text_w = content.size.w - icon_s - 4;
  if (show_fan) {
    text_w -= fan_s + 4;
  } else if (right) {
    text_w -= 44;
  }

  draw_icon(ctx, icon, GPoint(content.origin.x, y + 2));

  graphics_context_set_text_color(ctx, color_dim());
  graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(text_x, y - 1, text_w, 16),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  graphics_context_set_text_color(ctx, color_fg());
  graphics_draw_text(ctx, value, value_font,
      GRect(text_x, y + 12, text_w, value_h),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  if (show_fan) {
    draw_icon(ctx, s_bmp_fan[s_fan_frame % FAN_FRAMES],
        GPoint(content.origin.x + content.size.w - fan_s, y + 1));
  }
  if (right) {
    graphics_context_set_text_color(ctx, color_dim());
    graphics_draw_text(ctx, right, fonts_get_system_font(FONT_KEY_GOTHIC_14),
        GRect(content.origin.x + content.size.w - 44, y + 14, 44, 16),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
  }
}

static void draw_frame(Layer *layer, GContext *ctx) {
  const FaceSettings *cfg = settings_get();
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, color_bg());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int margin = bounds.size.w >= 180 ? 10 : 6;
  GRect panel = GRect(margin, margin, bounds.size.w - margin * 2, bounds.size.h - margin * 2);
  draw_panel_frame(ctx, panel);

  int pad = bounds.size.h < 180 ? 6 : PANEL_PAD;
  int pad_bottom = pad + 10;
  GRect content = GRect(panel.origin.x + pad, panel.origin.y + pad,
                        panel.size.w - pad * 2, panel.size.h - pad - pad_bottom);

  const int file_h = 22;
  const int file_gap = 4;
  bool reserve_file = cfg->show_weekday;
  int file_y = content.origin.y + content.size.h - file_h;
  int body_bottom = reserve_file ? (file_y - file_gap) : (content.origin.y + content.size.h);

  bool compact = content.size.h < 190;
  GFont value_font = fonts_get_system_font(
      compact ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_24_BOLD);
  int value_h = compact ? 20 : 28;
  int temp_row_h = 12 + value_h;
  int icon_s = 16;
  int fan_s = 16;
  int gap = compact ? 3 : 8;
  int y = content.origin.y;
  int hour = display_hour(s_hour);
  int pct = day_progress_pct();

  char nozzle_line[24];
  snprintf(nozzle_line, sizeof(nozzle_line), "%02d/%02d C", hour, hour);
  draw_temp_row(ctx, content, y, s_bmp_nozzle, "Nozzle", nozzle_line, NULL,
                value_font, value_h, icon_s, fan_s, true);
  y += temp_row_h + gap;

  char bed_line[24];
  snprintf(bed_line, sizeof(bed_line), "%02d/%02d C", s_minute, s_minute);
  const char *feed = cfg->show_feedrate ? "F100%" : NULL;
  draw_temp_row(ctx, content, y, s_bmp_bed, "Bed", bed_line, feed,
                value_font, value_h, icon_s, fan_s, false);
  y += temp_row_h + gap;

  if (cfg->show_progress) {
    int bar_h = compact ? 11 : 14;
    int pct_w = 36;
    GRect bar = GRect(content.origin.x, y, content.size.w - pct_w - 4, bar_h);
    draw_progress_bar(ctx, bar, pct);

    char pct_line[8];
    snprintf(pct_line, sizeof(pct_line), "%d%%", pct);
    graphics_context_set_text_color(ctx, color_fg());
    graphics_draw_text(ctx, pct_line, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
        GRect(bar.origin.x + bar.size.w + 4, y - 3, pct_w, 20),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

    y += bar_h + gap;
  }

  if (cfg->show_xyz && y + 16 <= body_bottom) {
    char xyz_line[32];
    snprintf(xyz_line, sizeof(xyz_line), "X:%02d.00 Y:%02d.00 Z:%02d",
             s_month, s_mday, s_second);
    graphics_context_set_text_color(ctx, color_fg());
    graphics_draw_text(ctx, xyz_line, fonts_get_system_font(FONT_KEY_GOTHIC_14),
        GRect(content.origin.x, y, content.size.w, 16),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    y += 15 + gap;
  }

  if (cfg->show_remaining && y + 16 <= body_bottom) {
    int remain_mins = (24 * 60) - (s_hour * 60 + s_minute);
    if (remain_mins < 0) {
      remain_mins = 0;
    }
    const char *status = "Printing";
    if (pct >= 100 || (s_hour == 23 && s_minute >= 59)) {
      status = "Finish";
    } else if (s_hour == 0 && s_minute < 1) {
      status = "Heating";
    }
    char status_line[36];
    snprintf(status_line, sizeof(status_line), "R %02d:%02d  %s",
             remain_mins / 60, remain_mins % 60, status);
    graphics_context_set_text_color(ctx, color_dim());
    graphics_draw_text(ctx, status_line, fonts_get_system_font(FONT_KEY_GOTHIC_14),
        GRect(content.origin.x, y, content.size.w, 16),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }

  if (cfg->show_weekday) {
    char file_line[28];
    snprintf(file_line, sizeof(file_line), "%s.GCO", weekday_name(s_wday));
    graphics_context_set_text_color(ctx, color_fg());
    graphics_draw_text(ctx, file_line, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
        GRect(content.origin.x, file_y, content.size.w, file_h),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
}

static void schedule_frame(void);

static void sync_clock(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_hour = t->tm_hour;
  s_minute = t->tm_min;
  s_second = t->tm_sec;
  s_month = t->tm_mon + 1;
  s_mday = t->tm_mday;
  s_wday = t->tm_wday;
}

static void on_frame(void *context) {
  s_timer = NULL;
  if (settings_get()->fan_spin) {
    s_fan_frame = (s_fan_frame + 1) % FAN_FRAMES;
  } else {
    s_fan_frame = 0;
  }
  sync_clock();
  layer_mark_dirty(s_layer);
  schedule_frame();
}

static void schedule_frame(void) {
  if (!s_timer) {
    s_timer = app_timer_register(FRAME_MS, on_frame, NULL);
  }
}

void face_tick(void) {
  sync_clock();
  layer_mark_dirty(s_layer);
}

void face_settings_changed(void) {
  if (s_window) {
    window_set_background_color(s_window, color_bg());
  }
  if (s_layer) {
    layer_mark_dirty(s_layer);
  }
}

void face_init(Window *window) {
  s_window = window;
  window_set_background_color(window, color_bg());

  sync_clock();
  s_fan_frame = 0;

  s_bmp_nozzle = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NOZZLE);
  s_bmp_bed = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BED);
  s_bmp_fan[0] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FAN_0);
  s_bmp_fan[1] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FAN_1);
  s_bmp_fan[2] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FAN_2);
  s_bmp_fan[3] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FAN_3);

  s_layer = layer_create(layer_get_bounds(window_get_root_layer(window)));
  layer_set_update_proc(s_layer, draw_frame);
  layer_add_child(window_get_root_layer(window), s_layer);

  schedule_frame();
}

void face_deinit(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  if (s_layer) {
    layer_destroy(s_layer);
    s_layer = NULL;
  }
  if (s_bmp_nozzle) {
    gbitmap_destroy(s_bmp_nozzle);
    s_bmp_nozzle = NULL;
  }
  if (s_bmp_bed) {
    gbitmap_destroy(s_bmp_bed);
    s_bmp_bed = NULL;
  }
  for (int i = 0; i < FAN_FRAMES; i++) {
    if (s_bmp_fan[i]) {
      gbitmap_destroy(s_bmp_fan[i]);
      s_bmp_fan[i] = NULL;
    }
  }
}
