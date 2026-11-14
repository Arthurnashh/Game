#ifndef _CONSOLE_BASE_H
#define _CONSOLE_BASE_H

/**
note that when error is indicated via return value, wasError need not be set
*/

extern bool wasError; // use this to indicate error
/// can be used to override detection
extern bool ignoreError;

int consoleMain( int argc, const char *argv[] );

#endif
