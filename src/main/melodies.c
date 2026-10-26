#include "melodies.h"
#include "include/cmath.h"

#define RETURN_MELODY(melody) {*out_melody_size = arrsize(melody); return melody;}

// DISCLAIMER: the music content in this file is generated from MIDIs, not by hand

static const buzzer_note_t WESTMINSTER_CHIME_NOTES[] = {
    { NOTE_GS5, 440 }, { NOTE_REST, 60 }, { NOTE_E5, 440 }, { NOTE_REST, 60 }, { NOTE_FS5, 440 }, { NOTE_REST, 60 },
    { NOTE_B4, 940 }, { NOTE_REST, 560 }, { NOTE_B4, 440 }, { NOTE_REST, 60 }, { NOTE_FS5, 440 }, { NOTE_REST, 60 },
    { NOTE_GS5, 440 }, { NOTE_REST, 60 }, { NOTE_E5, 940 },
};

static const buzzer_note_t FUR_ELISE_NOTES[] = {
    { NOTE_E6, 140 }, { NOTE_REST, 20 }, { NOTE_DS6, 140 }, { NOTE_REST, 20 }, { NOTE_E6, 140 }, { NOTE_REST, 20 },
    { NOTE_DS6, 140 }, { NOTE_REST, 20 }, { NOTE_E6, 140 }, { NOTE_REST, 20 }, { NOTE_B5, 140 }, { NOTE_REST, 20 },
    { NOTE_D6, 140 }, { NOTE_REST, 20 }, { NOTE_C6, 140 }, { NOTE_REST, 20 }, { NOTE_A5, 640 },
};

static const buzzer_note_t UN_OWEN_NOTES[] = {
    { NOTE_B4, 200 }, { NOTE_REST, 200 }, { NOTE_FS5, 200 }, { NOTE_REST, 200 }, { NOTE_CS5, 200 }, { NOTE_REST, 200 },
    { NOTE_FS5, 200 }, { NOTE_REST, 200 }, { NOTE_D5, 200 }, { NOTE_REST, 200 }, { NOTE_E5, 200 }, { NOTE_FS5, 200 },
    { NOTE_E5, 200 }, { NOTE_REST, 200 }, { NOTE_GS5, 200 }, { NOTE_REST, 200 }, { NOTE_B5, 200 }, { NOTE_FS5, 200 },
    { NOTE_CS6, 200 }, { NOTE_D6, 100 }, { NOTE_REST, 100 }, { NOTE_CS6, 200 }, { NOTE_D6, 100 }, { NOTE_CS6, 100 },
    { NOTE_B5, 200 }, { NOTE_A5, 100 }, { NOTE_REST, 100 }, { NOTE_FS5, 200 }, { NOTE_A5, 100 }, { NOTE_REST, 100 },
    { NOTE_E5, 200 }, { NOTE_FS5, 100 }, { NOTE_REST, 100 }, { NOTE_D5, 400 },
};

static const buzzer_note_t CLICK_NOTES[] = {{ NOTE_C7, 30 }};

static const buzzer_note_t REJECT_NOTES[] = {
    { NOTE_E5, 70 }, { NOTE_REST, 30 }, { NOTE_C5, 120 }
};

static const buzzer_note_t COOK_START_NOTES[] = {
    { NOTE_G5, 100 }, { NOTE_REST, 20 }, { NOTE_C6, 100 }, { NOTE_REST, 20 },
    { NOTE_E6, 100 }, { NOTE_REST, 20 }, { NOTE_G6, 260 }
};

static const buzzer_note_t DELAYED_START_NOTES[] = {
    { NOTE_G6, 100 }, { NOTE_REST, 20 }, { NOTE_E6, 100 }, { NOTE_REST, 20 },
    { NOTE_C6, 320 }
};

static const buzzer_note_t ERROR_NOTES[] = {
    { NOTE_A6, 150 }, { NOTE_REST, 100 },
    { NOTE_A6, 150 }, { NOTE_REST, 700 }
}; // looped

void melodies_play_one(const melody_id_t melody, const bool loop_melody) {
    uint16_t note_len;
    buzzer_play_melody(melodies_get_one(melody, &note_len), note_len, loop_melody);
}

const buzzer_note_t* melodies_get_one(
    const melody_id_t melody,
    uint16_t *out_melody_size
) {
    switch (melody) {
        case MELODY_BTN_ACCEPTED: RETURN_MELODY(CLICK_NOTES);
        case MELODY_BTN_REJECTED: RETURN_MELODY(REJECT_NOTES);
        case MELODY_COOK_START: RETURN_MELODY(COOK_START_NOTES);
        case MELODY_DELAYED_START: RETURN_MELODY(DELAYED_START_NOTES);
        case MELODY_SYSTEM_ERROR: RETURN_MELODY(ERROR_NOTES);
        case MELODY_COMPLETION_OWEN: RETURN_MELODY(UN_OWEN_NOTES);
        case MELODY_COMPLETION_ELISE: RETURN_MELODY(FUR_ELISE_NOTES);
        case MELODY_COMPLETION_DEFAULT: RETURN_MELODY(WESTMINSTER_CHIME_NOTES);
    }
    *out_melody_size = 0;
    return nullptr;
}
