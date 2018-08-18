#pragma once
#include "main.h"

#define SAMPLE_RATE 44100
#define ENV_OVERSHOOT 0.05f
#define MIN_ATTACK 0.005f;

#define SINE_TABLE_WIDTH 11 // bits
#define SINE_TABLE_SIZE (1<<SINE_TABLE_WIDTH)
extern int16_t sine_table[SINE_TABLE_SIZE];




#define MAX_ARP 5
#define NUM_VOICE 1
#define NUM_OSCILLATOR 2


typedef enum {
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvState;

typedef enum {
    WAVE_TRI,
    WAVE_SQUARE,
    WAVE_SAW,
    NUM_WAVE
} Wave;

typedef enum {
    ARP_OFF,
    ARP_UP,
    ARP_DOWN,
    ARP_UP_DOWN,
} ArpMode;




typedef struct {
    Wave waveform;
    float folding;
    float duty;
    float detune;
    float gain;
} Oscillator;

typedef struct {
    bool busy;          // config being updated, don't copy

    uint8_t volume;     // 0-100
    bool legato;

    // Sequencer
    ArpMode arp;        // arpeggio mode
    uint32_t arp_freqs[MAX_ARP]; // arp notes, normalised
    int tempo;          // bpm
    bool seq_play;

    // Oscillators
    float freq[NUM_VOICE];
    Oscillator osc[NUM_OSCILLATOR];
    float noise_gain;

    // ADSR 
    bool key[NUM_VOICE];           // key down?
    bool key_retrigger[NUM_VOICE];
    float attack_rate;
    float decay_rate;
    float sustain_level;
    float release_rate;
    float env_curve;    // linearity
    

    float attack_time;
    float decay_time;
    float release_time;

    // Filter
    float cutoff;       // fs
    float resonance;    // 0-4
    float env_mod;      // Hz at max env

    // LFO
    float lfo_rate;
    float lfo_amount;

    // FX
    float fx_damping;
    float fx_combg;

    // Bass drum
    float bass_pitch;
    float bass_click;
    float bass_punch;

} SynthConfig;

#define NUM_SEQ_NOTES 2
typedef struct {
    float note[NUM_SEQ_NOTES];

} SeqConfig;

extern SynthConfig cfg;
extern SynthConfig cfgnew;
extern SeqConfig seq;

extern uint32_t loop_time;
extern uint32_t transfer_time;

extern bool trig_bass;

void create_wave_tables(void);
void synth_start(void);


