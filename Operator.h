#ifndef OPERATOR_H
#define OPERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int BOOL;

BOOL is_operator(char* id);

BOOL is_operator_char(long cp);

#ifdef __cplusplus
}
#endif

#endif // OPERATOR_H
