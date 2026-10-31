#ifndef GRACE_TSS_H
#define GRACE_TSS_H

#include <stdint.h>

struct __attribute__((packed)) tss {
    uint32_t reserved0;
    uint64_t rsp[3]; // RSP0-RSP2
    uint64_t reserved1;
    uint64_t ist[7]; // IST1-IST7
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb; // I/O permission bitmap offset
};

#endif
