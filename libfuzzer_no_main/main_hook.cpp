#include <unistd.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <chrono>
#include <iostream>
#include <thread>

#include <dlfcn.h>

static int (*gRealMain)(int, char **, char **) = nullptr;

// Our fuzz target
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size);
// Provided by libFuzzer
extern "C" int LLVMFuzzerRunDriver(int *argc, char ***argv,
                  int (*UserCb)(const uint8_t *Data, size_t Size));

// remove arg that start with x from argv
void remove_arg(int &argc, char **argv, const char* x) {
  std::string str;
  int len = std::strlen(x);

  for (int i = 1; i < argc; i++) {
    str = std::string (argv [i]);
    if (str.rfind(x, 0) == 0) {
      for (int j = i; j < argc; j++) {
        argv [j] = argv [j + 1];
      }
      argc--;
      break;
    }
  }
}

void write_name(int argc, char** argv) {
    char filename[64];
    sprintf(filename, "/tmp/ntpd.%d.pid", (int)getpid());
    FILE *f = fopen(filename, "w+");
    assert(f);
    fprintf(f, "args=[\n");
    for(int i=0; i<argc; ++i) {
      fprintf(f, "\"");
      for(char *a=argv[i]; *a; ++a) {
        switch(*a) {
          case '\"':
          case '\'':
            fprintf(f, "\\%c", *a);
            break;
          case '\n':
            fprintf(f, "\\n", *a);
            break;
          case '\r':
            fprintf(f, "\\r", *a);
            break;
          default:
            fprintf(f, "%c", *a);
        }
      }
      if(i+1 < argc) {
        fprintf(f, "\",\n");
      } else {
        fprintf(f, "\"");
      }
    }
    fprintf(f, "]\n");
    fclose(f);
}

int our_main(int argc, char** argv, char** envp) {
    write_name(argc, argv);
  
    // cifuzz or libfuzzer check for arguments in argv that conflict with
    // args that ntpd expects via argv. Args that are set from cifuzz or
    // for libfuzzer will be saved in orig_argc and orig_argv
    int orig_argc = argc;
    char** orig_argv = new char*[argc];
    for (int i = 0; i < argc; i++) {
        orig_argv[i] = new char[strlen(argv[i]) + 1];
        strcpy(orig_argv[i], argv[i]);
        // helpful to debug unknown args set by cifuzz or for target 
        // application child process
        // std::cout << std::string(orig_argv[i]) << std::endl;
    }

    // remove all args that might be set by cifuzz 
    remove_arg(argc, argv, "-max_total_time=");
    remove_arg(argc, argv, "/openntpd-portable/.cifuzz-corpus");
    remove_arg(argc, argv, "-artifact_prefix=");
    remove_arg(argc, argv, "-merge=");
    remove_arg(argc, argv, "/tmp");
    remove_arg(argc, argv, "/tmp");
    remove_arg(argc, argv, "-runs=");
    remove_arg(argc, argv, "-merge_control_file=");
    remove_arg(argc, argv, "-merge_inner=");
    // remove_arg(argc, argv, "");

    if(getenv("FUZZER_RUNNING")) {

        return gRealMain(argc, argv, envp);

    } else {
        putenv("FUZZER_RUNNING=1");

        std::thread real_main([&]() {
            gRealMain(argc, argv, envp);
        });

        // uncomment during debugging
        //std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        std::cout << "[premain] Starting fuzzer..." << std::endl;
        return LLVMFuzzerRunDriver(&orig_argc, &orig_argv, LLVMFuzzerTestOneInput);
    }
    
}

__attribute__((visibility("default"))) __attribute__((unused)) extern "C" int
__libc_start_main(int (*main)(int, char **, char **), int argc, char **argv,
                  int (*init)(int, char **, char **), void (*fini)(void),
                  void (*rtld_fini)(void), void *stack_end) {
  gRealMain = main;
  decltype(&__libc_start_main) real_start_main =
      reinterpret_cast<decltype(&__libc_start_main)>(
          dlsym(RTLD_NEXT, "__libc_start_main"));
  return real_start_main(our_main, argc, argv, init,
                         fini, rtld_fini, stack_end);
}
