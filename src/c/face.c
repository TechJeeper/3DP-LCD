#include "face.h"

#define FRAME_MS 120
#define FAN_FRAMES 4
#define PANEL_PAD 8
#define ROW_GAP 4

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
#if defined(PBL_COLOR)
  return GColorOxfordBlue;
#else
  return GColorBlack;
#endif
}

static GColor color_panel(void) {
#if defined(PBL_COLOR)
  return GColorDukeBlue;
#else
  return GColorBlack;
#endif
}

static GColor color_fg(void) {
  return GColorWhite;
}

static GColor color_dim(void) {
#if defined(PBL_COLOR)
  return GColorPictonBlue;
#else
  return GColorWhite;
#endif
}

static GColor color_bar(void) {
#if defined(PBL_COLOR)
  return GColorWhite;
#else
  return GColorWhite;
#endif
}

static int display_hour(int hour) {
  if (!clock_is_24h_style()) {
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

  // Bezel highlight — cheap “LCD glass” cue
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

static void draw_frame(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, color_bg());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // LCD panel sized to the watch, with a dark bezel margin outside.
  int margin = bounds.size.w >= 180 ? 10 : 6;
  GRect panel = GRect(margin, margin, bounds.size.w - margin * 2, bounds.size.h - margin * 2);
  draw_panel_frame(ctx, panel);

  GRect content = GRect(panel.origin.x + PANEL_PAD, panel.origin.y + PANEL_PAD,
                        panel.size.w - PANEL_PAD * 2, panel.size.h - PANEL_PAD * 2);

  int icon_s = 16;
  int row_h = bounds.size.h >= 200 ? 28 : 24;
  int fan_s = 16;
  int tall = bounds.size.h >= 200;
  int gap = tall ? 10 : 6;
  int y = content.origin.y + 2;
  int hour = display_hour(s_hour);
  int pct = day_progress_pct();

  // --- Row 1: Nozzle = hour ---
  draw_icon(ctx, s_bmp_nozzle, GPoint(content.origin.x, y + (row_h - icon_s) / 2));

  graphics_context_set_text_color(ctx, color_dim());
  graphics_draw_text(ctx, "Nozzle", fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(content.origin.x + icon_s + 4, y - 2, 56, 16),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  char nozzle_line[24];
  snprintf(nozzle_line, sizeof(nozzle_line), "%02d/%02d C", hour, hour);
  graphics_context_set_text_color(ctx, color_fg());
  graphics_draw_text(ctx, nozzle_line, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
      GRect(content.origin.x + icon_s + 4, y + 8, content.size.w - icon_s - fan_s - 12, 28),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Fan spins in the top-right corner of the LCD
  draw_icon(ctx, s_bmp_fan[s_fan_frame % FAN_FRAMES],
      GPoint(content.origin.x + content.size.w - fan_s, y + 2));

  y += row_h + gap;

  // --- Row 2: Bed = minute ---
  draw_icon(ctx, s_bmp_bed, GPoint(content.origin.x, y + (row_h - icon_s) / 2));

  graphics_context_set_text_color(ctx, color_dim());
  graphics_draw_text(ctx, "Bed", fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(content.origin.x + icon_s + 4, y - 2, 56, 16),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  char bed_line[24];
  snprintf(bed_line, sizeof(bed_line), "%02d/%02d C", s_minute, s_minute);
  graphics_context_set_text_color(ctx, color_fg());
  graphics_draw_text(ctx, bed_line, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
      GRect(content.origin.x + icon_s + 4, y + 8, content.size.w - icon_s - 8, 28),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Feedrate joke — always “100%” like an idle printer
  graphics_context_set_text_color(ctx, color_dim());
  graphics_draw_text(ctx, "F100%", fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(content.origin.x + content.size.w - 44, y + 12, 44, 16),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  y += row_h + gap;

  // --- Progress bar = day elapsed ---
  int bar_h = 14;
  int pct_w = 36;
  GRect bar = GRect(content.origin.x, y, content.size.w - pct_w - 4, bar_h);
  draw_progress_bar(ctx, bar, pct);

  char pct_line[8];
  snprintf(pct_line, sizeof(pct_line), "%d%%", pct);
  graphics_context_set_text_color(ctx, color_fg());
  graphics_draw_text(ctx, pct_line, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(bar.origin.x + bar.size.w + 4, y - 4, pct_w, 20),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  y += bar_h + gap;

  // --- XYZ: month / day / seconds (classic status-screen coords) ---
  char xyz_line[32];
  snprintf(xyz_line, sizeof(xyz_line), "X:%02d.00 Y:%02d.00 Z:%02d",
           s_month, s_mday, s_second);
  graphics_context_set_text_color(ctx, color_fg());
  graphics_draw_text(ctx, xyz_line, fonts_get_system_font(FONT_KEY_GOTHIC_18),
      GRect(content.origin.x, y - 2, content.size.w, 20),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  y += 20 + (tall ? 6 : 2);

  // --- Remaining until midnight (Marlin "R" time) ---
  int remain_mins = (24 * 60) - (s_hour * 60 + s_minute);
  if (remain_mins < 0) {
    remain_mins = 0;
  }
  char remain_line[24];
  snprintf(remain_line, sizeof(remain_line), "R %02d:%02d",
           remain_mins / 60, remain_mins % 60);
  graphics_context_set_text_color(ctx, color_dim());
  graphics_draw_text(ctx, remain_line, fonts_get_system_font(FONT_KEY_GOTHIC_18),
      GRect(content.origin.x, y - 2, content.size.w / 2, 20),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  const char *status = "Printing...";
  if (pct >= 100 || (s_hour == 23 && s_minute >= 59)) {
    status = "Finish";
  } else if (s_hour == 0 && s_minute < 1) {
    status = "Heating...";
  }
  graphics_context_set_text_color(ctx, color_fg());
  graphics_draw_text(ctx, status, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(content.origin.x + content.size.w / 2, y - 2, content.size.w / 2, 20),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  // --- Filename pinned to the bottom of the panel ---
  char file_line[28];
  snprintf(file_line, sizeof(file_line), "%s.GCO", weekday_name(s_wday));
  int file_y = content.origin.y + content.size.h - 22;
  if (file_y < y + 18) {
    file_y = y + 18;
  }
  graphics_context_set_text_color(ctx, color_dim());
  graphics_draw_text(ctx, file_line, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(content.origin.x, file_y, content.size.w, 22),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
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
  s_fan_frame = (s_fan_frame + 1) % FAN_FRAMES;
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
