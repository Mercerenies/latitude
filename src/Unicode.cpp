//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details


#include "Unicode.hpp"
#include "pl_Unidata.h"

UniChar::UniChar(long cp)
    : codepoint(cp) {}

UniChar::operator std::string() const {
    long value = codePoint();
    std::string result;
    int n = 0;
    if (value <= 0x7F) {
        result += (char)value;
        n = 1;
    } else if (value <= 0x7FF) {
        result += (char)((value >> 6) | 0xC0);
        n = 2;
    } else if (value <= 0xFFFF) {
        result += (char)((value >> 12) | 0xE0);
        n = 3;
    } else if (value <= 0x10FFFF) {
        result += (char)((value >> 18) | 0xF0);
        n = 4;
    }
    for (int i = n - 1; i > 0; i--) {
        int shift = (i - 1) * 6;
        result += (char)(((value >> shift) & 0x3F) | 0x80);
    }
    return result;
}

long UniChar::codePoint() const noexcept {
    return codepoint;
}

uni_class_t UniChar::genCat() const {
    return get_class(codepoint);
}

UniChar UniChar::toUpper() const {
    return UniChar(to_upper(codepoint));
}

UniChar UniChar::toLower() const {
    return UniChar(to_lower(codepoint));
}

UniChar UniChar::toTitle() const {
    return UniChar(to_title(codepoint));
}

long uniOrd(UniChar ch) {
    return ch.codePoint();
}

UniChar uniChr(long cp) {
    return UniChar(cp);
}

boost::optional<UniChar> charAt(std::string str, long i) {
    if ((i < 0) || ((unsigned long)i >= str.length()))
        return boost::none;
    bool valid = true;
    long result = 0L;
    auto full_len = str.length();
    unsigned char c = static_cast<unsigned char>(str[i]);
    unsigned long n = 0;
    if ((c & 0x80) == 0x00) {
        n = 1;
        result = c & 0x7F;
    } else if ((c & 0xE0) == 0xC0) {
        n = 2;
        result = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        n = 3;
        result = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        n = 4;
        result = c & 0x07;
    } else {
        valid = false;
    }
    if (n + i > full_len)
        valid = false;
    if (valid) {
        for (unsigned long j = i + 1; j < i + n; j++) {
            unsigned char c1 = static_cast<unsigned char>(str[j]);
            if ((c1 & 0xC0) == 0x80) {
                result <<= 6;
                result += c1 & 0x3F;
            } else {
                valid = false;
            }
        }
    }
    if (valid)
        return UniChar(result);
    else
        return boost::none;
}

boost::optional<long> nextCharPos(std::string str, long i) {
    if ((i < 0) || ((unsigned long)i >= str.length()))
        return boost::none;
    // Based on the algorithm at http://stackoverflow.com/a/4063258/2288659
    bool valid = true;
    auto full_len = str.length();
    unsigned char c = static_cast<unsigned char>(str[i]);
    unsigned long n = 0;
    if ((c & 0x80) == 0x00)
        n = 1;
    else if ((c & 0xE0) == 0xC0)
        n = 2;
    else if ((c & 0xF0) == 0xE0)
        n = 3;
    else if ((c & 0xF8) == 0xF0)
        n = 4;
    else
        valid = false;
    if (n + i > full_len)
        valid = false;
    if (valid) {
        for (unsigned long j = i + 1; j < i + n; j++) {
            unsigned char c1 = static_cast<unsigned char>(str[j]);
            if ((c1 & 0xC0) != 0x80)
                valid = false;
        }
    }
    if (valid)
        return static_cast<signed long>(i + n);
    else
        return boost::none;
}
