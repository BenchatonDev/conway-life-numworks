#include "eadk/eadkpp.h"
#include "eadk/eadk_vars.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cstdio>

/**************************************
 * Configuration
 **************************************/
#define MAX_ACTIVE_CELLS   1024
#define MAX_CANDIDATES     (MAX_ACTIVE_CELLS * 9)
#define MAX_BRUSH_CELLS    128

/**************************************
 * Data Structures.
 **************************************/
typedef struct {
    int16_t x;
    int16_t y;
} Cell;

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t count;
} Candidate;

typedef struct {
    int16_t offsetX;
    int16_t offsetY;
    int zoom;
} Viewport;

typedef struct {
    int16_t cells[MAX_BRUSH_CELLS][2];
    uint16_t count;
} Brush;

/**************************************
 * Global Variables.
 **************************************/
static bool running = true;
static bool paused = true;
static bool speed_ctrl_mode = false;
static bool secondary_ctrl_layer = false;
int speed_multiplier;
Brush brushes[10];
int plrY, plrX, plrS;
int offsetX, offsetY, zoom;

EADK::Color cell_color = EADK::Color(0xFFFFFF);
EADK::Color plr_color = EADK::Color(0x4D4DFF);

/**************************************
 * Input Variables.
 **************************************/
bool backspace_held = false;
bool toolbox_held = false;
bool exe_held = false;
bool shift_held = false;
bool shifted = false;

/**************************************
 * Global Simulation State.
 **************************************/
static Cell activeCells[MAX_ACTIVE_CELLS];
static uint16_t activeCount = 0;

static Viewport viewport = {0, 0, 1};

/**************************************
 * Helper Functions.
 **************************************/
bool is_cell_alive(int16_t x, int16_t y) {
    for (uint16_t i = 0; i < activeCount; i++) {
        if (activeCells[i].x == x && activeCells[i].y == y)
            return true;
    }
    return false;
}

void place_cell(int16_t x, int16_t y) {
    if (!is_cell_alive(x, y) && activeCount < MAX_ACTIVE_CELLS) {
        activeCells[activeCount].x = x;
        activeCells[activeCount].y = y;
        activeCount++;
    }
}

void kill_cell(int16_t x, int16_t y) {
    for (uint16_t i = 0; i < activeCount; i++) {
        if (activeCells[i].x == x && activeCells[i].y == y) {
            activeCells[i] = activeCells[activeCount - 1];
            activeCount--;
            break;
        }
    }
}

int find_candidate(Candidate *candidates, uint16_t candidateCount, int16_t x, int16_t y) {
    for (uint16_t i = 0; i < candidateCount; i++) {
        if (candidates[i].x == x && candidates[i].y == y)
            return i;
    }
    return -1;
}

void simulate_step(void) {
    Candidate candidates[MAX_CANDIDATES];
    uint16_t candidateCount = 0;
    
    const int8_t offsets[8][2] = {
        {-1, -1}, { 0, -1}, { 1, -1},
        {-1,  0},           { 1,  0},
        {-1,  1}, { 0,  1}, { 1,  1}
    };
    
    for (uint16_t i = 0; i < activeCount; i++) {
        int16_t cx = activeCells[i].x;
        int16_t cy = activeCells[i].y;
        
        if (find_candidate(candidates, candidateCount, cx, cy) < 0) {
            if (candidateCount < MAX_CANDIDATES) {
                candidates[candidateCount].x = cx;
                candidates[candidateCount].y = cy;
                candidates[candidateCount].count = 0;
                candidateCount++;
            }
        }
        
        for (uint8_t j = 0; j < 8; j++) {
            int16_t nx = cx + offsets[j][0];
            int16_t ny = cy + offsets[j][1];
            int idx = find_candidate(candidates, candidateCount, nx, ny);
            if (idx < 0) {
                if (candidateCount < MAX_CANDIDATES) {
                    candidates[candidateCount].x = nx;
                    candidates[candidateCount].y = ny;
                    candidates[candidateCount].count = 1;
                    candidateCount++;
                }
            } else {
                if (candidates[idx].count < 8)
                    candidates[idx].count++;
            }
        }
    }
    
    Cell newActiveCells[MAX_ACTIVE_CELLS];
    uint16_t newActiveCount = 0;
    
    for (uint16_t i = 0; i < candidateCount; i++) {
        bool alive = is_cell_alive(candidates[i].x, candidates[i].y);
        uint8_t ncount = candidates[i].count;
        if (alive) {
            if (ncount == 2 || ncount == 3) {
                if (newActiveCount < MAX_ACTIVE_CELLS) {
                    newActiveCells[newActiveCount].x = candidates[i].x;
                    newActiveCells[newActiveCount].y = candidates[i].y;
                    newActiveCount++;
                }
            }
        } else {
            if (ncount == 3) {
                if (newActiveCount < MAX_ACTIVE_CELLS) {
                    newActiveCells[newActiveCount].x = candidates[i].x;
                    newActiveCells[newActiveCount].y = candidates[i].y;
                    newActiveCount++;
                }
            }
        }
    }
    
    memcpy(activeCells, newActiveCells, newActiveCount * sizeof(Cell));
    activeCount = newActiveCount;
}

/**************************************
 * Viewport / Zoom Functions.
 **************************************/
void set_viewport(int16_t offsetX, int16_t offsetY, float zoom) {
    viewport.offsetX = offsetX;
    viewport.offsetY = offsetY;
    viewport.zoom = zoom;
}

void sim_to_view(int16_t sim_x, int16_t sim_y, int *view_x, int *view_y) {
    *view_x = (int)((sim_x - viewport.offsetX) * viewport.zoom);
    *view_y = (int)((sim_y - viewport.offsetY) * viewport.zoom);
}

void view_to_sim(int view_x, int view_y, int16_t *sim_x, int16_t *sim_y) {
    *sim_x = (int16_t)(view_x / viewport.zoom + viewport.offsetX);
    *sim_y = (int16_t)(view_y / viewport.zoom + viewport.offsetY);
}

/**************************************
 * Brush Functions.
 **************************************/
void init_brush(Brush *b) {
    b->count = 0;
}

void brush_add_cell(Brush *b, int16_t dx, int16_t dy) {
    if (b->count < MAX_BRUSH_CELLS) {
        b->cells[b->count][0] = dx;
        b->cells[b->count][1] = dy;
        b->count++;
    }
}

void brush_from_plr(Brush *b) {
    for (int16_t i = 0; i < plrS; i++) {
        for (int16_t j = 0; j < plrS; j++) {
            if (is_cell_alive(i + plrX, plrY + j)) {
                brush_add_cell(b, i, j);
            }
        }
    };
}

void apply_brush(Brush *b, int16_t x, int16_t y) {
    for (uint16_t i = 0; i < b->count; i++) {
        place_cell(x + b->cells[i][0], y + b->cells[i][1]);
    }
}

/**************************************
 * Initialization.
 **************************************/
void init_simulation(void) {
    activeCount = 0;
    for (int i = 0; i < 10; i++) { init_brush(&brushes[i]); }
    memset(activeCells, 0, sizeof(activeCells));
}

/**************************************
 * Input Handling.
 **************************************/
void input(void) {
    EADK::Keyboard::State kbdState = EADK::Keyboard::scan();
    running = !kbdState.keyDown(EADK::Keyboard::Key::Home);

    // Option Toggles
    if (kbdState.keyDown(EADK::Keyboard::Key::Shift) && !speed_ctrl_mode) {
        if (!shift_held) {shifted = !shifted; plr_color = EADK::Color(0xD22730);}
        if (!shifted) {plr_color = EADK::Color(0x4D4DFF);}
        shift_held = true;
    } else {shift_held = false;}

    if (kbdState.keyDown(EADK::Keyboard::Key::Toolbox) && !shifted) {
        if (!toolbox_held) {speed_ctrl_mode = !speed_ctrl_mode; plr_color = EADK::Color(0x44D62C);}
        if (!speed_ctrl_mode) {plr_color = EADK::Color(0x4D4DFF);}
        toolbox_held = true;
    } else {toolbox_held = false;}

    if (kbdState.keyDown(EADK::Keyboard::Key::EXE)) {
        if (!exe_held) {secondary_ctrl_layer = !secondary_ctrl_layer;}
        exe_held = true;
    } else {exe_held = false;}
    
    if (kbdState.keyDown(EADK::Keyboard::Key::Backspace)) {
        if (!backspace_held) {paused = !paused;}
        backspace_held = true;
    } else {backspace_held = false;}

    // Zoom Management
    if (kbdState.keyDown(EADK::Keyboard::Key::Plus) && secondary_ctrl_layer && !speed_ctrl_mode) {
        zoom += 1;
        if (zoom > EADK::Screen::Width / 4) {zoom = EADK::Screen::Width / 4;}
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Minus) && secondary_ctrl_layer && !speed_ctrl_mode) {
        zoom -= 1;
        if (zoom < 1) {zoom = 1;}
    }

    //Speed Control
    if (kbdState.keyDown(EADK::Keyboard::Key::Plus) && speed_ctrl_mode) {
        speed_multiplier += 1;
        if (speed_multiplier > 20) {speed_multiplier = 20;}
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Minus) && speed_ctrl_mode) {
        speed_multiplier -= 1;
        if (speed_multiplier < 1) {speed_multiplier = 1;}
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Ans) && speed_ctrl_mode) {
        speed_multiplier = 1;
    }

    // Camera Movement
    if (kbdState.keyDown(EADK::Keyboard::Key::Up) && secondary_ctrl_layer) {
        offsetY -= 1 * speed_multiplier;
    }
    
    if (kbdState.keyDown(EADK::Keyboard::Key::Down) && secondary_ctrl_layer) {
        offsetY += 1 * speed_multiplier;
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Right) && secondary_ctrl_layer) {
        offsetX += 1 * speed_multiplier;
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Left) && secondary_ctrl_layer) {
        offsetX -= 1 * speed_multiplier;
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Ans) && !speed_ctrl_mode) {
        offsetX = offsetY = 0;
    }

    // Player Controls
    if (kbdState.keyDown(EADK::Keyboard::Key::Up) && !secondary_ctrl_layer) {
        plrY -= 1 * speed_multiplier;
    }
    
    if (kbdState.keyDown(EADK::Keyboard::Key::Down) && !secondary_ctrl_layer) {
        plrY += 1 * speed_multiplier;
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Right) && !secondary_ctrl_layer) {
        plrX += 1 * speed_multiplier;
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Left) && !secondary_ctrl_layer) {
        plrX -= 1 * speed_multiplier;
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Plus) && !secondary_ctrl_layer && !speed_ctrl_mode) {
        plrS += 1;
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Minus) && !secondary_ctrl_layer && !speed_ctrl_mode) {
        plrS -= 1;
        if (plrS < 1) {plrS = 1;}
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::OK) && !shifted) {
        for (int i = 0; i < plrS; i++) {
            for (int j = 0; j < plrS; j++) {
                place_cell(plrX + i, plrY + j);
            }
        };
    }

    if (kbdState.keyDown(EADK::Keyboard::Key::Back) && !shifted) {
        for (int i = 0; i < plrS; i++) {
            for (int j = 0; j < plrS; j++) {
                kill_cell(plrX + i, plrY + j);
            }
        };
    }

    // Brushes
    if (kbdState.keyDown(EADK::Keyboard::Key::Toolbox) && shifted) {
        if (kbdState.keyDown(EADK::Keyboard::Key::One)) {apply_brush(&brushes[1], plrX, plrY);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Two)) {apply_brush(&brushes[2], plrX, plrY);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Three)) {apply_brush(&brushes[3], plrX, plrY);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Four)) {apply_brush(&brushes[4], plrX, plrY);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Five)) {apply_brush(&brushes[5], plrX, plrY);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Six)) {apply_brush(&brushes[6], plrX, plrY);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Seven)) {apply_brush(&brushes[7], plrX, plrY);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Eight)) {apply_brush(&brushes[8], plrX, plrY);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Nine)) {apply_brush(&brushes[9], plrX, plrY);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Zero)) {apply_brush(&brushes[0], plrX, plrY);
        }
    }
    if (kbdState.keyDown(EADK::Keyboard::Key::Var) && shifted) {
        Brush tmp;
        init_brush(&tmp);
        if (kbdState.keyDown(EADK::Keyboard::Key::One)) {brushes[1] = tmp; brush_from_plr(&brushes[1]);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Two)) {brushes[2] = tmp; brush_from_plr(&brushes[2]);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Three)) {brushes[3] = tmp; brush_from_plr(&brushes[3]);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Four)) {brushes[4] = tmp; brush_from_plr(&brushes[4]);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Five)) {brushes[5] = tmp; brush_from_plr(&brushes[5]);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Six)) {brushes[6] = tmp; brush_from_plr(&brushes[6]);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Seven)) {brushes[7] = tmp; brush_from_plr(&brushes[7]);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Eight)) {brushes[8] = tmp; brush_from_plr(&brushes[8]);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Nine)) {brushes[9] = tmp; brush_from_plr(&brushes[9]);
        } if (kbdState.keyDown(EADK::Keyboard::Key::Zero)) {brushes[0] = tmp; brush_from_plr(&brushes[0]);
        }
    }

    // Player Lock
    int tmpX, tmpY;
    int16_t tmpX2, tmpY2;
    sim_to_view(plrX, plrY, &tmpX, &tmpY);
    view_to_sim(0, 0, &tmpX2, &tmpY2);
    if (plrS * zoom > EADK::Screen::Height) {
        if (zoom != 0) {plrS = EADK::Screen::Height / zoom;}
    }
    if (tmpX < 0) {
        plrX = tmpX2;
    } if (tmpY < 0) {
        plrY = tmpY2;
    }
    view_to_sim((EADK::Screen::Width - plrS * zoom), (EADK::Screen::Height - plrS * zoom), &tmpX2, &tmpY2);
    if ((tmpX + plrS * zoom) >= EADK::Screen::Width) {
        plrX = tmpX2;
    } if ((tmpY + plrS * zoom) >= EADK::Screen::Height) {
        plrY = tmpY2;
    }
}

/**************************************
 * Rendering.
 **************************************/
void draw(void) {
    int tmpX, tmpY;
    set_viewport(offsetX, offsetY, zoom);

    EADK::Display::pushRectUniform(EADK::Screen::Rect, 0x000000);
    for (uint16_t i = 0; i < activeCount; i++) {
        sim_to_view(activeCells[i].x, activeCells[i].y, &tmpX, &tmpY);
        if ((tmpX < 0 || tmpY < 0 || tmpX >= EADK::Screen::Width || tmpY >= EADK::Screen::Height)) {continue;}

        EADK::Display::pushRectUniform(EADK::Rect(tmpX, tmpY, zoom, zoom), cell_color);
    }

    sim_to_view(plrX, plrY, &tmpX, &tmpY);
    EADK::Display::pushRectUniform(EADK::Rect(tmpX, tmpY, 1, plrS * zoom), plr_color);
    EADK::Display::pushRectUniform(EADK::Rect(tmpX, tmpY, plrS * zoom, 1), plr_color);
    EADK::Display::pushRectUniform(EADK::Rect(tmpX + plrS * zoom - 1, tmpY, 1, plrS * zoom), plr_color);
    EADK::Display::pushRectUniform(EADK::Rect(tmpX, tmpY + plrS * zoom - 1, plrS * zoom, 1), plr_color);
}

int main(void) {
    EADK::Display::pushRectUniform(EADK::Screen::Rect, 0x000000);

    zoom = 10;
    speed_multiplier = 1;
    plrS = 1;

    init_simulation();
    while (running) {
        if (!paused) {simulate_step();}
        input();
        draw();
        EADK::Timing::msleep(100);
    }
    return 0;
}
