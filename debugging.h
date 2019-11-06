#ifndef DEBUG_H
#define DEBUG_H
#include<cstdio>
/// \file
/// \brief Defines macros that make debugging easier.

/// Prints out the current line of text and name of the file that this macro is
/// in and on.
#define PRINTLINE printf("\r\n%s ,Line %d\r\n", __FILE__, __LINE__);

/// Prints the value of the integer and the line number of the print statement
#define PRINTINT(x) printf("\r\n Line %d int = %d\r\n", __LINE__, (x));

/// Prints the value of the integer and the line number of the print statement
#define PRINTFLOAT(x) printf("\r\n Line %d float = %f\r\n", __LINE__, (x));

/// Prints the value of the integer and the line number of the print statement
#define PRINTSTRING(x)                                                         \
    printf("\r\n Line %d string = %s\r\n", __LINE__, (x.c_str()));



#endif // DEBUG
