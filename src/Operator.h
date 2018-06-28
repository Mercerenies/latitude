//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef OPERATOR_H
#define OPERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int BOOL;

BOOL isOperator(char* id);

BOOL isOperatorChar(long cp);

#ifdef __cplusplus
}
#endif

#endif // OPERATOR_H
