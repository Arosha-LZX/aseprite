// Aseprite
// Copyright (C) 2019-2021  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cli/app_options.h"
#include "app/console.h"
#include "app/resource_finder.h"
#include "app/send_crash.h"
#include "base/exception.h"
#include "base/memory.h"
#include "base/memory_dump.h"
#include "base/system_console.h"
#include "os/error.h"
#include "os/system.h"

#include <clocale>
#include <cstdlib>
#include <ctime>
#include <iostream>

#ifdef _WIN32
  #include <windows.h>
#endif

namespace {

  // Memory leak detector wrapper
  class MemLeak {
  public:
#ifdef LAF_MEMLEAK
    MemLeak() { base_memleak_init(); }
    ~MemLeak() { base_memleak_exit(); }
#else
    MemLeak() { }
#endif
  };

#ifdef _WIN32
  // Successful calls to CoInitialize() (S_OK or S_FALSE) must match
  // the calls to CoUninitialize().
  // From: https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-couninitialize#remarks
  struct CoInit {
    HRESULT hr;
    CoInit() {
      hr = CoInitialize(nullptr);
    }
    ~CoInit() {
      if (hr == S_OK || hr == S_FALSE)
        CoUninitialize();
    }
  };
#endif

}

// Aseprite entry point. (Called from "os" library.)
int app_main(int argc, char* argv[])
{
  // Initialize the locale. Aseprite isn't ready to handle numeric
  // fields with other locales (e.g. we expect strings like "10.32" be
  // used in std::strtod(), not something like "10,32").
  std::setlocale(LC_ALL, "en-US");
  ASSERT(std::strtod("10.32", nullptr) == 10.32);

  // Initialize the random seed.
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

#ifdef _WIN32
  CoInit com;                   // To create COM objects
#endif

  try {
    base::MemoryDump memoryDump;
    MemLeak memleak;
    base::SystemConsole systemConsole;
    app::AppOptions options(argc, const_cast<const char**>(argv));
    os::SystemRef system(os::make_system());
    app::App app;

    // Change the memory dump filename to save on disk (.dmp
    // file). Note: Only useful on Windows.
    {
      const std::string fn = app::SendCrash::DefaultMemoryDumpFilename();
      if (!fn.empty())
        memoryDump.setFileName(fn);
    }

    const int code = app.initialize(options);

    if (options.startShell())
      systemConsole.prepareShell();

    app.run();

    // After starting the GUI, we'll always return 0, but in batch
    // mode we can return the error code.
    return (app.isGui() ? 0: code);
  }
  catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    os::error_message(e.what());
    return 1;
  }
}
