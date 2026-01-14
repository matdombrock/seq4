#include "wasm4.h"
#include <math.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

#define COLS 16
#define ROWS 16

enum MODES { MODE_F1, MODE_F2, MODE_F3 };

const u8 smiley[] = {
    0b11000011, 0b10000001, 0b00100100, 0b00100100,
    0b00000000, 0b00100100, 0b10011001, 0b11000011,
};

static u32 PAL_OG[] = {0x001105, 0x506655, 0xA0FFA5, 0xB0FFB5};

struct Pos {
  u16 x;
  u16 y;
};

struct State {
  struct Pos cursor_pos;
  u8 input_released;
  u32 frame;
  enum MODES mode;
  struct Pos f1_pos;
  struct Pos f2_pos;
  struct Pos f3_pos;
};

static struct State state = {
    .cursor_pos = {0, 0},
    .input_released = 1,
    .frame = 0,
    .mode = 0,
    .f1_pos = {0, 0},
};

void set_palette(u32 pal[4]) {
  PALETTE[0] = pal[0];
  PALETTE[1] = pal[1];
  PALETTE[2] = pal[2];
  PALETTE[3] = pal[3];
}

void input() {
  u8 gamepad = *GAMEPAD1;
  if (state.input_released) {
    if (gamepad & BUTTON_UP) {
      if (state.cursor_pos.y > 0) {
       state.cursor_pos.y -= 1;
      }
      else {
       state.cursor_pos.y = ROWS - 1;
      }
    }
    if (gamepad & BUTTON_DOWN) {
      if (state.cursor_pos.y < ROWS - 1) {
        state.cursor_pos.y += 1;
      }
      else {
        state.cursor_pos.y = 0;
      }
    }
    if (gamepad & BUTTON_LEFT) {
      if (state.cursor_pos.x > 0) {
        state.cursor_pos.x -= 1;
      }
      else {
        state.cursor_pos.x = COLS - 1;
      }
    }
    if (gamepad & BUTTON_RIGHT) {
      if (state.cursor_pos.x < COLS - 1) {
        state.cursor_pos.x += 1;
      }
      else {
        state.cursor_pos.x = 0;
      }
    }
    if (gamepad & BUTTON_1) {
      state.mode = (state.mode + 1) % 3;
      switch (state.mode) {
      case MODE_F1:
        state.cursor_pos.x = state.f1_pos.x;
        state.cursor_pos.y = state.f1_pos.y;
        break;
      case MODE_F2:
        state.cursor_pos.x = state.f2_pos.x;
        state.cursor_pos.y = state.f2_pos.y;
        break;
      case MODE_F3:
        state.cursor_pos.x = state.f3_pos.x;
        state.cursor_pos.y = state.f3_pos.y;
        break;
      }
    }
    state.input_released = 0;
  }
  if (!(gamepad &
        (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT | BUTTON_1))) {
    state.input_released = 1;
  }
}

struct Pos cur_to_screen(struct Pos cur) {
  u8 cell_width = 160 / COLS;
  u8 cell_height = 160 / ROWS;
  struct Pos screen_pos = {
      .x = cur.x * cell_width,
      .y = cur.y * cell_height,
  };
  return screen_pos;
}

float pos_to_float(u16 pos, u16 max) { return pos / (float)max; }

// u32 freq_map(u16 x, u16 y) {
//   float f = powf(2.0f, pos_to_float(y, ROWS) * 5.0f);
//   f += 120;
//   float lfo_depth = pos_to_float(x, COLS) * 10.0f;
//   f += sinf((float)state.frame / 1000.0f) * lfo_depth;
//   return (u32)f;
// }

u32 freq_map(u16 x, u16 y) {
  float f = y * COLS + x;
  f *= 4;
  // f /= 3.33f;
  f += 32;
  return (u32)f;
}

void update() {
  state.frame += 1;
  input();

  set_palette(PAL_OG);
  *DRAW_COLORS = 2;
  u8 vsplit = 160 / COLS;
  for (u8 i = 0; i < COLS; i++) {
    vline(i * vsplit, 0, 160);
  }
  u8 hsplit = 160 / ROWS;
  for (u8 i = 0; i < ROWS; i++) {
    hline(0, i * hsplit, 160);
  }

  struct Pos cpos = cur_to_screen(state.cursor_pos);
  u8 cheight = 160 / ROWS;
  u8 cwidth = 160 / COLS;
  rect(cpos.x, cpos.y, cheight, cwidth);
  *DRAW_COLORS = 3;
  char *mode_text = "";
  switch (state.mode) {
  case MODE_F1:
    mode_text = "F1";
    break;
  case MODE_F2:
    mode_text = "F2";
    break;
  case MODE_F3:
    mode_text = "F3";
    break;
  }
  text(mode_text, cpos.x, cpos.y);

  switch (state.mode) {
  case MODE_F1:
    state.f1_pos.y = state.cursor_pos.y;
    state.f1_pos.x = state.cursor_pos.x;
    break;
  case MODE_F2:
    state.f2_pos.y = state.cursor_pos.y;
    state.f2_pos.x = state.cursor_pos.x;
    break;
  case MODE_F3:
    state.f3_pos.y = state.cursor_pos.y;
    state.f3_pos.x = state.cursor_pos.x;
    break;
  }

  tone((u32)freq_map(state.f1_pos.x, state.f1_pos.y), 1, 64, TONE_PULSE1);
  tone((u32)freq_map(state.f2_pos.x, state.f2_pos.y), 1, 64, TONE_PULSE2);
  tone(2 * (u32)freq_map(state.f3_pos.x, state.f3_pos.y), 1, 128, TONE_TRIANGLE);
}
