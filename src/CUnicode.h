//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef CUNICODE_H
#define CUNICODE_H

#ifdef __cplusplus
extern "C" {
#endif

char* charEncode(char* out, unsigned long codepoint);

#ifdef __cplusplus
}
#endif

#endif // CUNICODE_H
