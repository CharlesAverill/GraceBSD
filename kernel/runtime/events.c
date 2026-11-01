#include "events.h"

#include <stddef.h>

#include "interrupts.h"

static struct event_queue eq;

bool event_queue_empty(void) {
    return eq.head == eq.tail;
}

bool event_queue_full(void) {
    return (eq.tail + 1) % EVENT_QUEUE_SIZE == eq.head;
}

uint64_t event_queue_length(void) {
    return (eq.tail + EVENT_QUEUE_SIZE - eq.head) % EVENT_QUEUE_SIZE;
}

uint64_t event_queue_dropped(void) {
    return eq.dropped;
}

void push_event(struct event e) {
    if (event_queue_full()) {
        eq.dropped++;
        return;
    }

    eq.events[eq.tail] = e;
    eq.tail            = (eq.tail + 1) % EVENT_QUEUE_SIZE;
}

static PollResult pop_event_internal(struct event* e) {
    if (event_queue_empty())
        return POLL_FAILURE;

    *e      = eq.events[eq.head];
    eq.head = (eq.head + 1) % EVENT_QUEUE_SIZE;

    return POLL_SUCCESS;
}

PollResult pop_event(struct event* e) {
    disable_interrupts();

    PollResult r = pop_event_internal(e);

    enable_interrupts();

    return r;
}

void wait_event(struct event* e) {
    for (;;) {
        disable_interrupts();

        if (pop_event_internal(e) == POLL_SUCCESS) {
            enable_interrupts();
            return;
        }

        __asm__ volatile("sti; hlt" ::: "memory");
    }
}
