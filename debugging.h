#ifndef DEBUG_H
#define DEBUG_H
//===-- debug.h - Debugging macros -----------------===//
//
// Part of the IAC energy monitoring project
//
//===---------------------------------------------------------------------===//
/// \file
/// Defines macros that make debugging easier.

/// Prints out the current line of text and name of the file that this macro is
/// in and on.
#define PRINTLINE printf("\r\nLine: %d in file:  %sr\n",  __LINE__ ,__FILE__);
#define PRINTINT(x) printf("\r\n Line %d int = %d\r\n", __LINE__,(x));
#define PRINTFLOAT(x) printf("\r\n Line %d float = %f\r\n", __LINE__,(x));
#define PRINTSTRING(x) printf("\r\n Line %d string = %s\r\n", __LINE__,(x.c_str()));

// function to determine free amount of memory
#include <stdio.h>
#include <stdlib.h>

#define FREEMEM_CELL 100
void printAvailableBytes(void);
struct elem { /* Definition of a structure that is FREEMEM_CELL bytes  in size.) */
    struct elem *next;
    char dummy[FREEMEM_CELL-2];
};

#endif //DEBUG  