#ifndef GRACE_EVENTS_H
#define GRACE_EVENTS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum EventType {
    EVENT_TIMER,
    EVENT_END
} EventType;

struct event {
    EventType type;
    uint64_t data;
};

// Space for 255 events and a dedicated empty event to disambiguate empty/full buffers
#define EVENT_QUEUE_SIZE 256

struct event_queue {
    struct event events[EVENT_QUEUE_SIZE];
    uint64_t head;    // next event to read
    uint64_t tail;    // next place to write an event
    uint64_t dropped; // number of events that could not be added to the queue
};

typedef enum PollResult {
    POLL_SUCCESS,
    POLL_FAILURE
} PollResult;

bool event_queue_empty(void);
bool event_queue_full(void);
void push_event(struct event e);
PollResult pop_event(struct event*);
void wait_event(struct event*);

#endif
