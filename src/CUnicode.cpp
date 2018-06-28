//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details


#include "CUnicode.h"

static constexpr int   ONE_BOUND = 1 <<  7;
static constexpr int   TWO_BOUND = 1 << 11;
static constexpr int THREE_BOUND = 1 << 16;
static constexpr int  FOUR_BOUND = 1 << 21;

char* charEncode(char* out, unsigned long codepoint) {
    if (codepoint < ONE_BOUND) {
        out[0] = codepoint;
        return out + 1;
    } else if (codepoint < TWO_BOUND) {
        out[0] = (codepoint >> 6) | 0xC0;
        out[1] = (codepoint & 0x3F) | 0x80;
        return out + 2;
    } else if (codepoint < THREE_BOUND) {
        out[0] = (codepoint >> 12) | 0xE0;
        out[1] = ((codepoint >> 6) & 0x3F) | 0x80;
        out[2] = (codepoint & 0x3F) | 0x80;
        return out + 3;
    } else if (codepoint < FOUR_BOUND) {
        out[0] = (codepoint >> 18) | 0xF0;
        out[1] = ((codepoint >> 12) & 0x3F) | 0x80;
        out[2] = ((codepoint >> 6) & 0x3F) | 0x80;
        out[3] = (codepoint & 0x3F) | 0x80;
        return out + 4;
    } else {
        return nullptr;
    }
}
