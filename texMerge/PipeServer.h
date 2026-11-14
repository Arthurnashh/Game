#pragma once

#include <projects/bredy.libs/StreamParser/StreamParser.tli>

#pragma warning (push)
#pragma warning (disable: 4355)

class PipeServer: public StreamParser<char,PipeServer>
{
  HANDLE _input;
  HANDLE _output;
  bool _console;
  void SendMessage(const char *prefix, const char *message, const char *postfix);
  bool _error;
public:
  PipeServer();
  PipeServer(HANDLE input, HANDLE output):StreamParser<char,PipeServer>(this,false),_input(input),_output(output),_console(false),_error(false) {}
  ~PipeServer(void);

  bool ReadStream(char *array, size_t toread, size_t &wasread);

  void SendOK(const char *message=NULL);
  void SendError(const char *reason=NULL);
  void SendLine(const char *line);
  void SendDot() {SendLine(".");}
  void SendMultiline(const char *text) {SendMessage(text,"\r\n.","\r\n");}
  void SendFormatted(const char *format, ...);

  void WriteStream(const void *data, size_t size);
  bool TestCommand(const char *command);
};
#pragma warning (pop)
