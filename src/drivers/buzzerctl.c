#include "include/xthreads.h"
#include "include/rchal.h"
#include "include/buzzerctl.h"

static const xthread_handle_t *thread_handle;
static const xthread_mutex_t  *mutex;

static const buzzer_note_t *current_track = nullptr;
static uint16_t current_track_size = 0;
static uint16_t current_note_index = 0;

static bool is_playable() {
    return current_track != nullptr && current_track_size != 0 && current_note_index < current_track_size;
}

void buzzer_play_melody(const buzzer_note_t* track, const uint16_t track_size) {
    xthreads_mutex_acquire(mutex);
    current_track = track;
    current_track_size = track_size;
    current_note_index = 0;
    if (is_playable()) {
        // wake the buzzer thread up
        xthreads_notify(thread_handle);
    }
    xthreads_mutex_release(mutex);
}

void buzzer_stop_melody() {
    buzzer_play_melody(nullptr, 0);
}

void task_buzzer_update_thread(void *args) {
    mutex = xthreads_mutex_new();
    thread_handle = xthreads_current();
    while (1) {
        xthreads_mutex_acquire(mutex);
        if (!is_playable()) {
            rc_buzzer_set_tone(0); // shutup
            xthreads_mutex_release(mutex);
            // sleep until buzzer_set_melody poke your ass
            xthreads_wait(XTHREAD_WAIT_ALL);
            continue;
        }
        // track ALWAYS point to static/malloced memory
        const buzzer_note_t *current_note = &current_track[current_note_index++];
        xthreads_mutex_release(mutex);

        // play the note & sleep (the note still plays)
        rc_buzzer_set_tone(current_note->freq_hz);
        xthreads_sleep(current_note->duration_ms);
    }
}