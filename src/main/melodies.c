#include "melodies.h"
#include "include/cmath.h"

#define RETURN_MELODY(melody) {*out_melody_size = arrsize(melody); return melody;}

// converted from midi/kachiusa.mid (channels 3 intro, 0 vocal line, 8 solo), 120 BPM, one octave up
static const buzzer_note_t KATYUSHA_NOTES[] = {
    { NOTE_B5, 417 }, { NOTE_D6, 50 }, { NOTE_DS6, 83 }, { NOTE_E6, 467 }, { NOTE_REST, 33 }, // bar 1
    { NOTE_D6, 450 }, { NOTE_REST, 84 }, { NOTE_E6, 183 }, { NOTE_REST, 50 }, { NOTE_D6, 250 }, { NOTE_REST, 33 }, { NOTE_C6, 117 }, { NOTE_REST, 133 }, { NOTE_C6, 250 }, { NOTE_REST, 17 }, { NOTE_B5, 267 }, { NOTE_A5, 216 }, // bar 2
    { NOTE_AS5, 34 }, { NOTE_B5, 466 }, { NOTE_REST, 17 }, { NOTE_E5, 650 }, { NOTE_REST, 150 }, { NOTE_B5, 33 }, { NOTE_C6, 450 }, { NOTE_A5, 234 }, { NOTE_REST, 33 }, // bar 3
    { NOTE_B5, 817 }, { NOTE_G5, 216 }, { NOTE_GS5, 50 }, { NOTE_A5, 167 }, { NOTE_REST, 83 }, { NOTE_A5, 284 }, { NOTE_G5, 250 }, // bar 4
    { NOTE_FS5, 266 }, { NOTE_E5, 500 }, { NOTE_REST, 17 }, { NOTE_E6, 133 }, { NOTE_REST, 384 }, { NOTE_E5, 733 }, { NOTE_REST, 33 }, // bar 5
    { NOTE_FS5, 234 }, { NOTE_REST, 50 }, { NOTE_G5, 716 }, { NOTE_REST, 50 }, { NOTE_E5, 250 }, { NOTE_REST, 17 }, { NOTE_G5, 133 }, { NOTE_REST, 117 }, { NOTE_G5, 217 }, { NOTE_REST, 50 }, { NOTE_FS5, 200 }, { NOTE_REST, 50 }, // bar 6
    { NOTE_E5, 250 }, { NOTE_REST, 33 }, { NOTE_FS5, 467 }, { NOTE_REST, 33 }, { NOTE_B4, 533 }, { NOTE_REST, 17 }, { NOTE_FS5, 683 }, { NOTE_REST, 50 }, // bar 7
    { NOTE_G5, 217 }, { NOTE_REST, 67 }, { NOTE_A5, 733 }, { NOTE_REST, 50 }, { NOTE_FS5, 233 }, { NOTE_REST, 34 }, { NOTE_A5, 150 }, { NOTE_REST, 100 }, { NOTE_A5, 216 }, { NOTE_REST, 50 }, // bar 8
    { NOTE_G5, 250 }, { NOTE_REST, 17 }, { NOTE_FS5, 217 }, { NOTE_REST, 33 }, { NOTE_E5, 933 }, { NOTE_REST, 134 }, { NOTE_B5, 483 }, { NOTE_REST, 33 }, // bar 9
    { NOTE_E6, 450 }, { NOTE_REST, 50 }, { NOTE_D6, 434 }, { NOTE_REST, 100 }, { NOTE_E6, 166 }, { NOTE_REST, 67 }, { NOTE_D6, 233 }, { NOTE_REST, 34 }, { NOTE_C6, 133 }, { NOTE_REST, 117 }, // bar 10
    { NOTE_C6, 233 }, { NOTE_REST, 50 }, { NOTE_B5, 217 }, { NOTE_REST, 33 }, { NOTE_A5, 267 }, { NOTE_REST, 16 }, { NOTE_B5, 484 }, { NOTE_REST, 16 }, { NOTE_E5, 600 }, { NOTE_REST, 184 }, // bar 11
    { NOTE_C6, 466 }, { NOTE_REST, 34 }, { NOTE_A5, 233 }, { NOTE_REST, 50 }, { NOTE_B5, 700 }, { NOTE_REST, 50 }, { NOTE_G5, 217 }, { NOTE_REST, 50 }, { NOTE_A5, 150 }, { NOTE_REST, 116 }, // bar 12
    { NOTE_A5, 250 }, { NOTE_REST, 34 }, { NOTE_G5, 233 }, { NOTE_FS5, 283 }, { NOTE_REST, 17 }, { NOTE_E5, 917 }, { NOTE_REST, 116 }, // bar 13
    { NOTE_B5, 484 }, { NOTE_REST, 33 }, { NOTE_E6, 450 }, { NOTE_REST, 67 }, { NOTE_D6, 433 }, { NOTE_REST, 83 }, { NOTE_E6, 184 }, { NOTE_REST, 50 }, { NOTE_D6, 216 }, { NOTE_REST, 67 }, // bar 14
    { NOTE_C6, 133 }, { NOTE_REST, 117 }, { NOTE_C6, 250 }, { NOTE_REST, 33 }, { NOTE_B5, 250 }, { NOTE_A5, 250 }, { NOTE_REST, 17 }, { NOTE_B5, 500 }, { NOTE_REST, 17 }, { NOTE_E5, 583 }, { NOTE_REST, 217 }, // bar 15
    { NOTE_C6, 450 }, { NOTE_REST, 50 }, { NOTE_A5, 216 }, { NOTE_REST, 50 }, { NOTE_B5, 734 }, { NOTE_REST, 16 }, { NOTE_G5, 250 }, { NOTE_REST, 34 }, // bar 16
    { NOTE_FS5, 233 }, { NOTE_REST, 17 }, { NOTE_B4, 233 }, { NOTE_REST, 33 }, { NOTE_G5, 250 }, { NOTE_FS5, 250 }, { NOTE_REST, 34 }, { NOTE_E5, 866 }, { NOTE_REST, 184 }, // bar 17
    { NOTE_E5, 733 }, { NOTE_REST, 33 }, { NOTE_FS5, 234 }, { NOTE_REST, 50 }, { NOTE_G5, 716 }, { NOTE_REST, 50 }, // bar 18
    { NOTE_E5, 250 }, { NOTE_REST, 17 }, { NOTE_G5, 133 }, { NOTE_REST, 117 }, { NOTE_G5, 217 }, { NOTE_REST, 50 }, { NOTE_FS5, 200 }, { NOTE_REST, 33 }, { NOTE_E5, 267 }, { NOTE_REST, 33 }, { NOTE_FS5, 467 }, { NOTE_REST, 33 }, // bar 19
    { NOTE_B4, 533 }, { NOTE_REST, 17 }, { NOTE_FS5, 683 }, { NOTE_REST, 50 }, { NOTE_G5, 217 }, { NOTE_REST, 67 }, { NOTE_A5, 733 }, { NOTE_REST, 50 }, // bar 20
    { NOTE_FS5, 217 }, { NOTE_REST, 50 }, { NOTE_A5, 150 }, { NOTE_REST, 100 }, { NOTE_A5, 216 }, { NOTE_REST, 50 }, { NOTE_G5, 250 }, { NOTE_REST, 17 }, { NOTE_FS5, 217 }, { NOTE_REST, 33 }, { NOTE_E5, 933 }, { NOTE_REST, 134 }, // bar 21
    { NOTE_B5, 483 }, { NOTE_REST, 33 }, { NOTE_E6, 434 }, { NOTE_REST, 66 }, { NOTE_D6, 434 }, { NOTE_REST, 100 }, // bar 22
    { NOTE_E6, 166 }, { NOTE_REST, 67 }, { NOTE_D6, 217 }, { NOTE_REST, 50 }, { NOTE_C6, 133 }, { NOTE_REST, 117 }, { NOTE_C6, 216 }, { NOTE_REST, 67 }, { NOTE_B5, 217 }, { NOTE_REST, 33 }, { NOTE_A5, 267 }, { NOTE_REST, 16 }, { NOTE_B5, 484 }, { NOTE_REST, 16 }, // bar 23
    { NOTE_E5, 600 }, { NOTE_REST, 184 }, { NOTE_C6, 466 }, { NOTE_REST, 34 }, { NOTE_A5, 233 }, { NOTE_REST, 50 }, { NOTE_B5, 683 }, { NOTE_REST, 67 }, // bar 24
    { NOTE_G5, 217 }, { NOTE_REST, 50 }, { NOTE_A5, 150 }, { NOTE_REST, 116 }, { NOTE_A5, 250 }, { NOTE_REST, 34 }, { NOTE_G5, 233 }, { NOTE_FS5, 283 }, { NOTE_REST, 17 }, // bar 25
    { NOTE_E5, 917 }, { NOTE_REST, 116 }, { NOTE_B5, 484 }, { NOTE_REST, 33 }, { NOTE_E6, 450 }, { NOTE_REST, 67 }, // bar 26
    { NOTE_D6, 433 }, { NOTE_REST, 83 }, { NOTE_E6, 184 }, { NOTE_REST, 50 }, { NOTE_D6, 216 }, { NOTE_REST, 50 }, { NOTE_C6, 150 }, { NOTE_REST, 117 }, { NOTE_C6, 250 }, { NOTE_REST, 33 }, { NOTE_B5, 250 }, { NOTE_A5, 234 }, { NOTE_REST, 33 }, // bar 27
    { NOTE_B5, 500 }, { NOTE_REST, 17 }, { NOTE_E5, 583 }, { NOTE_REST, 217 }, { NOTE_C6, 433 }, { NOTE_REST, 67 }, { NOTE_A5, 216 }, { NOTE_REST, 50 }, // bar 28
    { NOTE_B5, 734 }, { NOTE_REST, 16 }, { NOTE_G5, 250 }, { NOTE_REST, 17 }, { NOTE_FS5, 250 }, { NOTE_REST, 17 }, { NOTE_B4, 233 }, { NOTE_REST, 17 }, { NOTE_G5, 266 }, // bar 29
    { NOTE_FS5, 250 }, { NOTE_REST, 34 }, { NOTE_E5, 866 }, { NOTE_REST, 184 }, { NOTE_GS5, 283 }, { NOTE_REST, 217 }, { NOTE_GS5, 216 }, { NOTE_REST, 67 }, // bar 30
    { NOTE_A5, 150 }, { NOTE_REST, 117 }, { NOTE_B5, 683 }, { NOTE_REST, 83 }, { NOTE_CS6, 167 }, { NOTE_REST, 100 }, { NOTE_B5, 117 }, { NOTE_REST, 150 }, { NOTE_B5, 166 }, { NOTE_REST, 100 }, { NOTE_CS6, 184 }, { NOTE_REST, 50 }, // bar 31
    { NOTE_DS6, 166 }, { NOTE_REST, 117 }, { NOTE_E6, 417 }, { NOTE_REST, 83 }, { NOTE_FS5, 617 }, { NOTE_REST, 233 }, { NOTE_FS5, 300 }, { NOTE_REST, 167 }, // bar 32
    { NOTE_A5, 166 }, { NOTE_REST, 100 }, { NOTE_DS6, 784 }, { NOTE_CS6, 250 }, { NOTE_B5, 100 }, { NOTE_REST, 166 }, { NOTE_B5, 217 }, { NOTE_REST, 67 }, // bar 33
    { NOTE_A5, 216 }, { NOTE_REST, 34 }, { NOTE_FS5, 200 }, { NOTE_REST, 50 }, { NOTE_GS5, 850 }, { NOTE_REST, 200 }, { NOTE_GS5, 300 }, { NOTE_REST, 183 }, // bar 34
    { NOTE_GS5, 167 }, { NOTE_REST, 100 }, { NOTE_A5, 150 }, { NOTE_REST, 133 }, { NOTE_B5, 700 }, { NOTE_REST, 50 }, { NOTE_CS6, 200 }, { NOTE_REST, 67 }, { NOTE_B5, 116 }, { NOTE_REST, 150 }, // bar 35
    { NOTE_B5, 234 }, { NOTE_REST, 50 }, { NOTE_CS6, 183 }, { NOTE_REST, 50 }, { NOTE_DS6, 150 }, { NOTE_REST, 133 }, { NOTE_E6, 434 }, { NOTE_REST, 83 }, { NOTE_FS5, 717 }, { NOTE_REST, 116 }, // bar 36
    { NOTE_FS5, 300 }, { NOTE_REST, 150 }, { NOTE_A5, 200 }, { NOTE_REST, 84 }, { NOTE_DS6, 716 }, { NOTE_REST, 50 }, { NOTE_CS6, 200 }, { NOTE_REST, 67 }, { NOTE_B5, 100 }, { NOTE_REST, 150 }, // bar 37
    { NOTE_B5, 150 }, { NOTE_REST, 117 }, { NOTE_CS6, 183 }, { NOTE_REST, 67 }, { NOTE_DS6, 150 }, { NOTE_REST, 133 }, { NOTE_E6, 767 }, { NOTE_REST, 266 }, // bar 38
    { NOTE_E5, 734 }, { NOTE_REST, 33 }, { NOTE_FS5, 233 }, { NOTE_REST, 50 }, { NOTE_G5, 717 }, { NOTE_REST, 50 }, { NOTE_E5, 250 }, // bar 39
    { NOTE_G5, 150 }, { NOTE_REST, 117 }, { NOTE_G5, 216 }, { NOTE_REST, 50 }, { NOTE_FS5, 200 }, { NOTE_REST, 34 }, { NOTE_E5, 266 }, { NOTE_REST, 34 }, { NOTE_FS5, 466 }, { NOTE_REST, 34 }, { NOTE_B4, 533 }, // bar 40
    { NOTE_FS5, 700 }, { NOTE_REST, 50 }, { NOTE_G5, 217 }, { NOTE_REST, 66 }, { NOTE_A5, 734 }, { NOTE_REST, 50 }, // bar 41
    { NOTE_FS5, 216 }, { NOTE_REST, 50 }, { NOTE_A5, 150 }, { NOTE_REST, 100 }, { NOTE_A5, 217 }, { NOTE_REST, 50 }, { NOTE_G5, 233 }, { NOTE_REST, 34 }, { NOTE_FS5, 216 }, { NOTE_REST, 34 }, { NOTE_E5, 916 }, { NOTE_REST, 134 }, // bar 42
    { NOTE_B5, 483 }, { NOTE_REST, 33 }, { NOTE_E6, 450 }, { NOTE_REST, 67 }, { NOTE_D6, 433 }, { NOTE_REST, 84 }, { NOTE_E6, 183 }, { NOTE_REST, 50 }, // bar 43
    { NOTE_D6, 233 }, { NOTE_REST, 50 }, { NOTE_C6, 134 }, { NOTE_REST, 116 }, { NOTE_C6, 217 }, { NOTE_REST, 67 }, { NOTE_B5, 216 }, { NOTE_REST, 34 }, { NOTE_A5, 266 }, { NOTE_REST, 17 }, { NOTE_B5, 483 }, { NOTE_REST, 17 }, // bar 44
    { NOTE_E5, 600 }, { NOTE_REST, 167 }, { NOTE_C6, 483 }, { NOTE_REST, 33 }, { NOTE_A5, 217 }, { NOTE_REST, 67 }, { NOTE_B5, 683 }, { NOTE_REST, 67 }, // bar 45
    { NOTE_G5, 216 }, { NOTE_REST, 50 }, { NOTE_A5, 150 }, { NOTE_REST, 117 }, { NOTE_A5, 250 }, { NOTE_REST, 33 }, { NOTE_G5, 234 }, { NOTE_FS5, 283 }, { NOTE_REST, 17 }, { NOTE_E5, 916 }, { NOTE_REST, 117 }, // bar 46
    { NOTE_B5, 483 }, { NOTE_REST, 34 }, { NOTE_E6, 450 }, { NOTE_REST, 66 }, { NOTE_D6, 434 }, { NOTE_REST, 83 }, // bar 47
    { NOTE_E6, 183 }, { NOTE_REST, 50 }, { NOTE_D6, 217 }, { NOTE_REST, 50 }, { NOTE_C6, 133 }, { NOTE_REST, 134 }, { NOTE_C6, 250 }, { NOTE_REST, 33 }, { NOTE_B5, 250 }, { NOTE_A5, 233 }, { NOTE_REST, 34 }, { NOTE_B5, 500 }, { NOTE_REST, 16 }, // bar 48
    { NOTE_E5, 567 }, { NOTE_REST, 233 }, { NOTE_C6, 434 }, { NOTE_REST, 66 }, { NOTE_A5, 217 }, { NOTE_REST, 50 }, { NOTE_B5, 733 }, { NOTE_REST, 17 }, // bar 49
    { NOTE_G5, 250 }, { NOTE_REST, 17 }, { NOTE_FS5, 250 }, { NOTE_REST, 16 }, { NOTE_B4, 234 }, { NOTE_REST, 16 }, { NOTE_G5, 250 }, { NOTE_REST, 17 }, { NOTE_FS5, 233 }, { NOTE_REST, 50 }, { NOTE_E5, 867 }, { NOTE_REST, 183 }, // bar 50
    { NOTE_E5, 734 }, { NOTE_REST, 33 }, { NOTE_FS5, 217 }, { NOTE_REST, 66 }, // bar 51
    { NOTE_G5, 717 }, { NOTE_REST, 50 }, { NOTE_E5, 250 }, { NOTE_G5, 150 }, { NOTE_REST, 117 }, { NOTE_G5, 216 }, { NOTE_REST, 50 }, { NOTE_FS5, 200 }, { NOTE_REST, 34 }, { NOTE_E5, 266 }, { NOTE_REST, 34 }, // bar 52
    { NOTE_FS5, 466 }, { NOTE_REST, 34 }, { NOTE_B4, 533 }, { NOTE_FS5, 700 }, { NOTE_REST, 50 }, { NOTE_G5, 217 }, { NOTE_REST, 66 }, // bar 53
    { NOTE_A5, 734 }, { NOTE_REST, 50 }, { NOTE_FS5, 216 }, { NOTE_REST, 50 }, { NOTE_A5, 150 }, { NOTE_REST, 100 }, { NOTE_A5, 217 }, { NOTE_REST, 50 }, { NOTE_G5, 233 }, { NOTE_REST, 17 }, // bar 54
    { NOTE_FS5, 233 }, { NOTE_REST, 34 }, { NOTE_E5, 916 }, { NOTE_REST, 134 }, { NOTE_B5, 483 }, { NOTE_REST, 33 }, { NOTE_E6, 450 }, { NOTE_REST, 67 }, // bar 55
    { NOTE_D6, 433 }, { NOTE_REST, 84 }, { NOTE_E6, 183 }, { NOTE_REST, 50 }, { NOTE_D6, 233 }, { NOTE_REST, 50 }, { NOTE_C6, 134 }, { NOTE_REST, 116 }, { NOTE_C6, 217 }, { NOTE_REST, 67 }, { NOTE_B5, 216 }, { NOTE_REST, 34 }, // bar 56
    { NOTE_A5, 266 }, { NOTE_REST, 17 }, { NOTE_B5, 483 }, { NOTE_REST, 17 }, { NOTE_E5, 600 }, { NOTE_REST, 167 }, { NOTE_C6, 483 }, { NOTE_REST, 33 }, // bar 57
    { NOTE_A5, 217 }, { NOTE_REST, 67 }, { NOTE_B5, 683 }, { NOTE_REST, 67 }, { NOTE_G5, 216 }, { NOTE_REST, 50 }, { NOTE_A5, 150 }, { NOTE_REST, 117 }, { NOTE_A5, 250 }, { NOTE_REST, 33 }, // bar 58
    { NOTE_G5, 234 }, { NOTE_FS5, 283 }, { NOTE_REST, 17 }, { NOTE_E5, 900 }, { NOTE_REST, 133 }, { NOTE_B5, 467 }, { NOTE_REST, 50 }, // bar 59
    { NOTE_E6, 450 }, { NOTE_REST, 66 }, { NOTE_D6, 434 }, { NOTE_REST, 83 }, { NOTE_E6, 167 }, { NOTE_REST, 66 }, { NOTE_D6, 217 }, { NOTE_REST, 50 }, { NOTE_C6, 133 }, { NOTE_REST, 134 }, // bar 60
    { NOTE_C6, 233 }, { NOTE_REST, 50 }, { NOTE_B5, 250 }, { NOTE_A5, 233 }, { NOTE_REST, 34 }, { NOTE_B5, 500 }, { NOTE_REST, 16 }, { NOTE_E5, 567 }, { NOTE_REST, 233 }, // bar 61
    { NOTE_C6, 434 }, { NOTE_REST, 66 }, { NOTE_A5, 217 }, { NOTE_REST, 50 }, { NOTE_B5, 733 }, { NOTE_REST, 17 }, { NOTE_G5, 250 }, { NOTE_REST, 17 }, { NOTE_FS5, 250 }, { NOTE_REST, 16 }, // bar 62
    { NOTE_B4, 234 }, { NOTE_REST, 16 }, { NOTE_G5, 250 }, { NOTE_REST, 17 }, { NOTE_FS5, 233 }, { NOTE_REST, 50 }, { NOTE_E5, 867 }, { NOTE_REST, 183 }, // bar 63
    { NOTE_GS5, 684 }, { NOTE_REST, 66 }, { NOTE_A5, 200 }, { NOTE_REST, 84 }, { NOTE_B5, 733 }, { NOTE_REST, 33 }, { NOTE_CS6, 217 }, { NOTE_REST, 50 }, // bar 64
    { NOTE_B5, 200 }, { NOTE_REST, 50 }, { NOTE_CS6, 217 }, { NOTE_REST, 33 }, { NOTE_DS6, 200 }, { NOTE_REST, 67 }, { NOTE_E6, 150 }, { NOTE_REST, 150 }, { NOTE_E6, 433 }, { NOTE_REST, 67 }, { NOTE_FS5, 650 }, { NOTE_REST, 166 }, // bar 65
    { NOTE_A5, 450 }, { NOTE_REST, 50 }, { NOTE_B5, 200 }, { NOTE_REST, 67 }, { NOTE_DS6, 733 }, { NOTE_REST, 67 }, // bar 66
    { NOTE_CS6, 133 }, { NOTE_REST, 100 }, { NOTE_B5, 184 }, { NOTE_REST, 66 }, { NOTE_CS6, 167 }, { NOTE_REST, 83 }, { NOTE_B5, 267 }, { NOTE_REST, 17 }, { NOTE_A5, 216 }, { NOTE_REST, 34 }, { NOTE_GS5, 833 }, { NOTE_REST, 217 }, // bar 67
    { NOTE_GS5, 683 }, { NOTE_REST, 83 }, { NOTE_A5, 217 }, { NOTE_REST, 50 }, { NOTE_B5, 750 }, { NOTE_REST, 67 }, // bar 68
    { NOTE_CS6, 183 }, { NOTE_REST, 67 }, { NOTE_B5, 216 }, { NOTE_REST, 17 }, { NOTE_CS6, 233 }, { NOTE_REST, 34 }, { NOTE_DS6, 183 }, { NOTE_REST, 67 }, { NOTE_E6, 133 }, { NOTE_REST, 133 }, { NOTE_E6, 467 }, { NOTE_REST, 50 }, // bar 69
    { NOTE_FS5, 600 }, { NOTE_REST, 217 }, { NOTE_A5, 433 }, { NOTE_REST, 50 }, { NOTE_B5, 33 }, { NOTE_CS6, 34 }, { NOTE_REST, 200 }, { NOTE_DS6, 733 }, { NOTE_REST, 33 }, // bar 70
    { NOTE_CS6, 134 }, { NOTE_REST, 133 }, { NOTE_B5, 250 }, { NOTE_CS6, 217 }, { NOTE_REST, 50 }, { NOTE_DS6, 216 }, { NOTE_REST, 50 }, { NOTE_E6, 17 }, { NOTE_DS6, 17 }, { NOTE_REST, 216 }, { NOTE_E6, 2650 }, { NOTE_REST, 250 }, // bar 71
};

const buzzer_note_t *get_melody(
    const melody_id_t melody,
    uint16_t *out_melody_size
) {
    switch (melody) {
        case MELODY_KATYUSHA: RETURN_MELODY(KATYUSHA_NOTES);
    }
    *out_melody_size = 0;
    return nullptr;
}
