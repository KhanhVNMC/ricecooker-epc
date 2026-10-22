#ifndef BUZZERCTL_H
#define BUZZERCTL_H
#include <stdint.h>

typedef struct {
    uint16_t freq_hz; // 0 = shutup
    uint16_t duration_ms;
} buzzer_note_t;

void buzzer_play_melody(const buzzer_note_t* track, uint16_t track_size);
void buzzer_stop_melody();

void buzzer_sys_init(void);
void task_buzzer_update_thread(void *args);

#endif //BUZZERCTL_H