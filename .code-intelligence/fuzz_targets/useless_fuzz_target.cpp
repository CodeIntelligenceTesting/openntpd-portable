#include <stdint.h>
#include <stddef.h>

extern "C" {
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <err.h>
#include "ntpd.h"
}

void ntp_sighdlr(int sig);

// extern "C" int FUZZ_INIT_WITH_ARGS(int *argc, char ***argv) {
extern "C" int FUZZ_INIT() {

  // Add global setup code here - called once before fuzzing starts

  return 0;
}

extern "C" int FUZZ(const int *Data, size_t Size) {

  // process fuzzer input (*Data) and setup local objects necessary to call the function under test

  ntp_sighdlr(Data[0]);

  // reset state and free all locally allocated resources

  return 0;
}
