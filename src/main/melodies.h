#ifndef MELODIES_H
#define MELODIES_H

#include "include/buzzerctl.h"

// notes
#define NOTE_REST   (0u)

#define NOTE_C3     (131u)
#define NOTE_CS3    (139u)
#define NOTE_D3     (147u)
#define NOTE_DS3    (156u)
#define NOTE_E3     (165u)
#define NOTE_F3     (175u)
#define NOTE_FS3    (185u)
#define NOTE_G3     (196u)
#define NOTE_GS3    (208u)
#define NOTE_A3     (220u)
#define NOTE_AS3    (233u)
#define NOTE_B3     (247u)

#define NOTE_C4     (262u)
#define NOTE_CS4    (277u)
#define NOTE_D4     (294u)
#define NOTE_DS4    (311u)
#define NOTE_E4     (330u)
#define NOTE_F4     (349u)
#define NOTE_FS4    (370u)
#define NOTE_G4     (392u)
#define NOTE_GS4    (415u)
#define NOTE_A4     (440u)
#define NOTE_AS4    (466u)
#define NOTE_B4     (494u)

#define NOTE_C5     (523u)
#define NOTE_CS5    (554u)
#define NOTE_D5     (587u)
#define NOTE_DS5    (622u)
#define NOTE_E5     (659u)
#define NOTE_F5     (698u)
#define NOTE_FS5    (740u)
#define NOTE_G5     (784u)
#define NOTE_GS5    (831u)
#define NOTE_A5     (880u)
#define NOTE_AS5    (932u)
#define NOTE_B5     (988u)

#define NOTE_C6     (1047u)
#define NOTE_CS6    (1109u)
#define NOTE_D6     (1175u)
#define NOTE_DS6    (1245u)
#define NOTE_E6     (1319u)
#define NOTE_F6     (1397u)
#define NOTE_FS6    (1480u)
#define NOTE_G6     (1568u)
#define NOTE_GS6    (1661u)
#define NOTE_A6     (1760u)
#define NOTE_AS6    (1865u)
#define NOTE_B6     (1976u)

#define NOTE_C7     (2093u)
#define NOTE_CS7    (2217u)
#define NOTE_D7     (2349u)
#define NOTE_DS7    (2489u)
#define NOTE_E7     (2637u)
#define NOTE_F7     (2794u)
#define NOTE_FS7    (2960u)
#define NOTE_G7     (3136u)
#define NOTE_GS7    (3322u)
#define NOTE_A7     (3520u)
#define NOTE_AS7    (3729u)
#define NOTE_B7     (3951u)

typedef enum : uint8_t {
    MELODY_KATYUSHA,
} melody_id_t;

const buzzer_note_t *get_melody(melody_id_t melody, uint16_t *out_melody_size);

#endif //MELODIES_H