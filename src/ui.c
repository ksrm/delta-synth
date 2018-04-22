#include "ui.h"
#include "main.h"
#include "midi.h"
#include "notes.h"
#include "audio.h"
#include "board.h"
#include "synth.h"
#include "stm32f4xx_ll_tim.h"
#include <math.h>

#define COL_RED     0x00F8
#define COL_GREEN   0xE007
#define COL_BLUE    0x1F00
#define COL_WHITE   0x18C6
#define CRED rgb(192,10,10)
#define CGRN rgb(10,192,10)
#define CBLU rgb(10,10,192)
#define CWHT rgb(192,192,192)
#define CGREY 0x8210

#define TWOPI 6.2831853f

typedef enum {
    PAGE_OSC,
    PAGE_ENV,
    PAGE_FILTER,
    PAGE_FX,
    PAGE_LFO,
    PAGE_SEQ,
    NUM_PAGES
} UIPage;

typedef struct {
    UIPage page;
    int selected_osc;
} UIState;
UIState ui;

typedef struct {
    int wave;
    int folding;
    int duty;
    int gain;
    int detune;
} OscSettings;

typedef struct {
    int attack;
    int decay;
    int release;
    int sustain;
    int cutoff;
    int resonance;
    int env_mod;
    OscSettings osc[NUM_OSCILLATOR];
    int lfo_rate;
    int lfo_amount;
    int fx_damping;
    int fx_amount;
} InputSettings;
InputSettings input;

unsigned int seq_idx;
float seq_note_input;




bool redraw = true;
void draw_screen(void);


#define ADD_DELTA_CLAMPED(x, y) {(x) += (y); if ((x) > 127) (x) = 127; if ((x) < 0) (x) = 0;}
#define ADD_DELTA_WRAPPED(x, y) {(x) += (y); if ((x) > 127) (x) -= 127; if ((x) < 0) (x) += 127;}
#define MAP_ENCODER(enc, var, block) if(encoders[enc].delta) { ADD_DELTA_CLAMPED(var, encoders[enc].delta); block; };


void ui_update(void) {

    bool evt = read_buttons();
    if (evt) {
        if (buttons[BUTTON_ENVELOPE] == BTN_PRESSED) {
            ui.page++;
            ui.page %= NUM_PAGES;
            redraw = true;
        }
        if (buttons[BUTTON_FILTER] == BTN_PRESSED) {
            ui.page = PAGE_SEQ;
            redraw = true;
        }
        if (buttons[BUTTON_OSC] == BTN_PRESSED) {
            //cfgnew.seq_play = !cfgnew.seq_play;
            if (ui.page != PAGE_OSC) {
                ui.page = PAGE_OSC;
                ui.selected_osc = 0;
            } else {
                ui.selected_osc++;
                ui.selected_osc %= NUM_OSCILLATOR;
            }
            redraw = true;
        }
    }

    evt = read_encoders();
    if (evt) {
        switch (ui.page) {
        case PAGE_ENV:

            // Attack
            MAP_ENCODER(ENC_RED, input.attack, {
                cfgnew.attack_time = (float)input.attack/127 + 0.001f;
                cfgnew.attack_rate = 1.0f/(cfgnew.attack_time * SAMPLE_RATE);
            });
            // Decay
            MAP_ENCODER(ENC_GREEN, input.decay, {
                float value = (float)input.decay/127;
                value = value*value*value;
                cfgnew.decay_time = value * 5;
                cfgnew.decay_rate = 1.0 - exp(cfgnew.env_curve / (cfgnew.decay_time * SAMPLE_RATE));
            });
            // Sustain
            MAP_ENCODER(ENC_BLUE, input.sustain, {
                float value = (float)input.sustain/127;
                cfgnew.sustain_level = value;
            });
            // Release
            MAP_ENCODER(ENC_WHITE, input.release, {
                float value = (float)input.release/127;
                value = value*value*value;
                cfgnew.release_time = value * 5;
                cfgnew.release_rate = 1.0 - exp(cfgnew.env_curve / (cfgnew.release_time * SAMPLE_RATE));
            });

            redraw = true;
            break;

        case PAGE_FILTER:

            // Cutoff
            if (encoders[ENC_RED].delta) {
                ADD_DELTA_CLAMPED(input.cutoff, encoders[ENC_RED].delta);
                cfgnew.cutoff = 10000.0f * (float)input.cutoff/127;
            }
            // Resonance
            if (encoders[ENC_GREEN].delta) {
                ADD_DELTA_CLAMPED(input.resonance, encoders[ENC_GREEN].delta);
                cfgnew.resonance = 3.99f * (float)input.resonance/127;
            }
            // Envelope modulation
            if (encoders[ENC_BLUE].delta) {
                ADD_DELTA_CLAMPED(input.env_mod, encoders[ENC_BLUE].delta);
                cfgnew.env_mod = 5000.0f * (float)input.env_mod/127;
            }

            redraw = true;
            break;

        case PAGE_OSC:
            // Waveform selector
            if (encoders[ENC_RED].delta) {
                ADD_DELTA_WRAPPED(input.osc[ui.selected_osc].wave, encoders[ENC_RED].delta);
                Wave w = input.osc[ui.selected_osc].wave / 16;
                w %= NUM_WAVE;
                cfgnew.osc[ui.selected_osc].waveform = w;
            }

            if (encoders[ENC_GREEN].delta) {
                ADD_DELTA_CLAMPED(input.osc[ui.selected_osc].folding, encoders[ENC_GREEN].delta);
                cfgnew.osc[ui.selected_osc].folding = 2.0f * (float)input.osc[ui.selected_osc].folding/127;
            }
            // Detune
            if (encoders[ENC_BLUE].delta) {
                ADD_DELTA_CLAMPED(input.osc[ui.selected_osc].detune, encoders[ENC_BLUE].delta);
                cfgnew.osc[ui.selected_osc].detune = 1.0f + 0.1f * (float)input.osc[ui.selected_osc].detune/127;
            }
            // Gain
            if (encoders[ENC_WHITE].delta) {
                ADD_DELTA_CLAMPED(input.osc[ui.selected_osc].gain, encoders[ENC_WHITE].delta);
                cfgnew.osc[ui.selected_osc].gain = (float)input.osc[ui.selected_osc].gain/127;
            }



            redraw = true;
            break;

        case PAGE_FX:
            if (encoders[ENC_GREEN].delta) {
                ADD_DELTA_CLAMPED(input.fx_damping, encoders[ENC_GREEN].delta);
                cfgnew.fx_damping = (float)input.fx_damping/127;
            }

            if (encoders[ENC_BLUE].delta) {
                ADD_DELTA_CLAMPED(input.fx_amount, encoders[ENC_BLUE].delta);
                float a = -1.0f / logf(1.0f - 0.3f);
                float b = 127.0f / (logf(1.0f - 0.98f) * a + 1.0f);
                float fb = 1.0f - expf(((float)input.fx_amount - b) / (a*b));
                cfgnew.fx_combg = fb;
            }            
            redraw = true;
            break;

        case PAGE_LFO:
            if (encoders[ENC_RED].delta) {
                ADD_DELTA_CLAMPED(input.lfo_rate, encoders[ENC_RED].delta);
                cfgnew.lfo_rate = (float)(input.lfo_rate+1) / (6 * SAMPLE_RATE);
            }
            if (encoders[ENC_GREEN].delta) {
                ADD_DELTA_CLAMPED(input.lfo_amount, encoders[ENC_GREEN].delta);
                cfgnew.lfo_amount = (float)input.lfo_amount/127;
            }            
            redraw = true;
            break;

        case PAGE_SEQ:
            if (encoders[ENC_RED].delta) {
                seq_idx += encoders[ENC_RED].delta;
                seq_idx %= NUM_SEQ_NOTES;
            }
            redraw = true;
            break;

        default:
            break;
        }
    }

    if (cfgnew.seq_play && seq_note_input != 0.0f) {
        seq.note[seq_idx] = seq_note_input;
    }

    // Redraw if required
    if (redraw) {
        draw_screen();
        display_draw();
    }

    HAL_Delay(10);

}

inline float fast_sin(float arg) {
    size_t idx = (size_t)(SINE_TABLE_SIZE * arg/TWOPI) + 1;
    return sine_table[idx] / 32768.0f;
}
inline float fast_cos(float arg) {
    size_t idx = (size_t)(SINE_TABLE_SIZE * arg/TWOPI) + 1;
    idx += SINE_TABLE_SIZE >> 2;
    idx %= SINE_TABLE_SIZE;
    return sine_table[idx] / 32768.0f;
}

void draw_gauge(uint16_t x, uint16_t y, float amount, uint16_t colour) {

    int radius = 16;
    int thickness = 23-radius;

    float start = TWOPI * 0.1f;
    float end = TWOPI * 0.9f;

    float angle = start + (end-start) * amount;

    int segments = 284; // Tune this so there are no gaps
    float step = TWOPI / segments;
    float xf = x + 0.5f;
    float yf = y + 0.5f;
    
    for (float a=start; a<end; a+=step) {
        float sina = -fast_sin(a);
        float cosa = fast_cos(a);
        float rad1 = radius;
        float rad2 = radius + thickness;
        uint16_t col = (a < angle) ? colour : CGREY;
        draw_line(xf + rad1*sina, yf + rad1*cosa, xf + rad2*sina, yf + rad2*cosa, col);
    }    

}

void draw_screen(void) {

    char buf[32];
    char *wave;

    if (display_busy) return;

    draw_rect(0, 0, 128, 128, 0x0000);

    float load = 100 * ((float)loop_time / transfer_time);
    sprintf(buf, "%.1f", (double)load);
    draw_text(64,0, buf, 1, COL_WHITE);
   

    switch (ui.page) {
        case PAGE_ENV:
            draw_text(0,  0,   "Envelope",  1, COL_WHITE);

            draw_text_cen(32,  16,   "ATTACK",  1, CRED);
            draw_gauge(32, 52, input.attack / 127.0f, CRED);

            draw_text_cen(96, 16,   "DECAY",   1, CGRN);
            draw_gauge(96, 52, input.decay / 127.0f, CGRN);
            
            draw_text_cen(32,   116, "SUSTAIN", 1, CBLU);
            draw_gauge(32, 94, input.sustain / 127.0f, CBLU);
            
            draw_text_cen(96,  116, "RELEASE", 1, CWHT);
            draw_gauge(96, 94, input.release / 127.0f, CWHT);
            break;


        case PAGE_FILTER:
            draw_text(0,  0,   "Filter",  1, COL_WHITE);

            draw_text_cen(32,  16,   "CUTOFF",  1, CRED);
            draw_gauge(32, 52, input.cutoff / 127.0f, CRED);

            draw_text_cen(96, 16,   "RESONANCE",   1, CGRN);
            draw_gauge(96, 52, input.resonance / 127.0f, CGRN);

            draw_text_cen(32,   116, "ENV MOD", 1, CBLU);
            draw_gauge(32, 94, input.env_mod / 127.0f, CBLU);
            break;


        case PAGE_OSC:
            sprintf(buf, "Osc %d", ui.selected_osc);
            draw_text(0,  0,   buf,  1, COL_WHITE);

            draw_text_cen(32,  16,   "WAVEFORM",  1, CRED);
            if (cfg.osc[ui.selected_osc].waveform == WAVE_SAW) {
                wave = "Saw";
            } else if (cfg.osc[ui.selected_osc].waveform == WAVE_SQUARE) {
                wave = "Square";
                draw_text_cen(96, 16, "SYMMETRY", 1, CGRN);
                draw_gauge(96, 52, input.osc[ui.selected_osc].duty / 127.0f, CGRN);
            } else {
                wave = "Tri";
                draw_text_cen(96, 16, "FOLDING", 1, CGRN);
                draw_gauge(96, 52, input.osc[ui.selected_osc].folding / 127.0f, CGRN);                
            }
            draw_text_cen(32,  40, wave,  1, CRED);

            draw_text_cen(32,   116, "DETUNE", 1, CBLU);
            draw_gauge(32, 94, input.osc[ui.selected_osc].detune / 127.0f, CBLU);
            
            draw_text_cen(96,  116, "GAIN", 1, CWHT);
            draw_gauge(96, 94, input.osc[ui.selected_osc].gain / 127.0f, CWHT);
            break;


        case PAGE_FX:
            //draw_text_cen(32,  16,   "COMBS",  1, CRED);
            //sprintf(buf, "%d", input.fx_ncombs);
            //draw_text_cen(32,  40, buf,  1, CRED);

            draw_text_cen(96, 16, "DAMPING", 1, CGRN);
            draw_gauge(96, 52, input.fx_damping / 127.0f, CGRN);

            draw_text_cen(32,   116, "AMOUNT", 1, CBLU);
            draw_gauge(32, 94, input.fx_amount / 127.0f, CBLU);
            break;


        case PAGE_LFO:
            draw_text(0, 0, "LFO", 1, COL_WHITE);

            draw_text_cen(32,  16,   "RATE",  1, CRED);
            draw_gauge(32, 52, input.lfo_rate / 127.0f, CRED);

            draw_text_cen(96, 16, "AMOUNT", 1, CGRN);
            draw_gauge(96, 52, input.lfo_amount / 127.0f, CGRN);
            break;


        case PAGE_SEQ:
            draw_text(0, 0, "Seq", 1, COL_WHITE);
            sprintf(buf, "idx=%d", seq_idx);
            draw_text(0, 32, buf, 1, COL_RED);
            break;


        default:
            break;
    }

}

