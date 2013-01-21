#ifndef Utils_h
#define Utils_h

#include <cstdlib>
#include <vector>
#include <string>
using namespace std;

#include <StringUtils.h>
using namespace mlpl;

#include <execinfo.h>

typedef vector<string> CommandLineArg;

string makeDemangledStackTraceLines(char **stackTraceLines, int num);

#define TRMSG(msg, fmt, ...) \
do { \
  void *trace[128]; \
  int n = backtrace(trace, sizeof(trace) / sizeof(trace[0])); \
  char **symbols = backtrace_symbols(trace, n); \
  msg = StringUtils::sprintf("<%s:%d> " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
  msg += makeDemangledStackTraceLines(symbols, n); \
  free(symbols); \
} while (0)

#endif // Utils_h

