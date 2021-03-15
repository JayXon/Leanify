#include "main.h"

#include <climits>
#include <cstring>
#include <iomanip>
#include <iostream>

#ifndef _WIN32
#include <ftw.h>
#endif

#include <taskflow/taskflow.hpp>

#include "fileio.h"
#include "formats/jpeg.h"
#include "formats/png.h"
#include "formats/zip.h"
#include "leanify.h"
#include "version.h"


using std::cerr;
using std::cout;
using std::endl;
using std::string;

template <typename T>
std::string ToString(const T a_value, const int n = 2) {
  std::ostringstream out;
  out.precision(n);
  out << std::fixed << a_value;
  return out.str();
}


const std::string BuildSize(size_t size) {
  if (size < 1024)
    return std::to_string(size) + " B";
  else if (size < 1024 * 1024)
    return ToString(size / 1024.0) + " KB";
  else
    return ToString(size / 1024.0 / 1024.0) + " MB";
}


#ifdef _WIN32
int ProcessFile(const std::wstring& file_path) {
  char mbs[MAX_PATH] = { 0 };
  WideCharToMultiByte(CP_ACP, 0, file_path.c_str(), -1, mbs, sizeof(mbs) - 1, nullptr, nullptr);
  string filename(mbs);
#else
int ProcessFile(const std::string& file_path) {
  const std::string& filename = file_path;
#endif  // _WIN32
  

  if (!parallel_processing)
    cout << "Processing: " << filename << endl;

  File input_file(file_path.c_str());

  if (input_file.IsOK()) {
    size_t original_size = input_file.GetSize();

    size_t new_size = LeanifyFile(input_file.GetFilePointer(), original_size, 0, filename);

    std::string log;
    if (parallel_processing)
      log = "Processed: " + filename + "\n";

    log += 
        BuildSize(original_size) + 
        " -> " + 
        BuildSize(new_size) +
        "\tLeanified: " + 
        BuildSize(original_size - new_size) + 
        " ("  + 
        ToString(100 - 100.0 * new_size / original_size) + 
        "%)";

    cout << log << endl;

    input_file.UnMapFile(new_size);
  }

  return 0;
}



void PauseIfNotTerminal() {
// pause if Leanify is not started in terminal
// so that user can see the output instead of just a flash of a black box
#ifdef _WIN32
  if (is_pause)
    system("pause");
#endif  // _WIN32
}


void PrintInfo() {
  cerr << "Leanify\t" << VERSION_STR << endl << endl;
  cerr << "Usage: leanify [options] paths\n"
          "  -i, --iteration <iteration>   More iterations may produce better result, but\n"
          "                                  use more time, default is 15.\n"
          "  -d, --max_depth <max depth>   Maximum recursive depth, unlimited by default.\n"
          "                                  Set to 1 will disable recursive minifying.\n"
          "  -f, --fastmode                Fast mode, no recompression.\n"
          "  -q, --quiet                   No output to stdout.\n"
          "  -v, --verbose                 Verbose output.\n"
          "  -p, --parallel                Distribute all tasks to all CPUs.\n"
          "  --keep-exif                   Do not remove Exif.\n"
          "  --keep-icc                    Do not remove ICC profile.\n"
          "\n"
          "JPEG options:\n"
          "  --jpeg-keep-all               Do not remove any metadata or comments in JPEG.\n"
          "  --jpeg-arithmetic             Use arithmetic coding for JPEG.\n"
          "\n"
          "ZIP options:\n"
          "  --zip-deflate                 Try deflate even if not compressed originally.\n";

  PauseIfNotTerminal();
}

tf::Taskflow taskflow;

#ifdef _WIN32
int EnqueueProcessFileTask(const wchar_t* file_path) {
  std::wstring* filePath = new std::wstring(file_path);
#else
// written like this in order to be callback function of ftw()
int EnqueueProcessFileTask(const char* file_path, const struct stat* sb = nullptr, int typeflag = FTW_F) {
  if (typeflag != FTW_F)
    return 0;
  std::string* filePath = new std::string(file_path);
#endif  // _WIN32

  auto task = [filePath]() { 
      ProcessFile(*filePath); 
      delete filePath;
  };
  taskflow.emplace(task);
  return 0;
}


#ifdef _WIN32
int main() {
  int argc;
  wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
#else
int main(int argc, char** argv) {
#endif  // _WIN32

  is_fast = false;
  is_verbose = false;
  iterations = 15;
  depth = 1;
  max_depth = INT_MAX;

#ifdef _WIN32
  is_pause = !getenv("PROMPT");
#endif  // _WIN32

  int i;
  for (i = 1; i < argc && argv[i][0] == L'-'; i++) {
#ifdef _WIN32
    // do not pause if any options are given
    is_pause = false;
#endif  // _WIN32
    int num_optargs = 0;
    for (int j = 1; argv[i][j]; j++) {
      switch (argv[i][j]) {
        case 'f':
          is_fast = true;
          break;
        case 'i':
          if (i < argc - 1) {
            iterations = STRTOL(argv[i + ++num_optargs], nullptr, 10);
            // strtol will return 0 on fail
            if (iterations == 0) {
              cerr << "There should be a positive number after -i option." << endl;
              PrintInfo();
              return 1;
            }
          }
          break;
        case 'd':
          if (i < argc - 1) {
            max_depth = STRTOL(argv[i + ++num_optargs], nullptr, 10);
            // strtol will return 0 on fail
            if (max_depth == 0) {
              cerr << "There should be a positive number after -d option." << endl;
              PrintInfo();
              return 1;
            }
          }
          break;
        case 'q':
          cout.setstate(std::ios::failbit);
          is_verbose = false;
          break;
        case 'v':
          cout.clear();
          is_verbose = true;
          break;
        case 'p':
          parallel_processing = true;
          break;
        case '-':
          if (STRCMP(argv[i] + j + 1, "fastmode") == 0) {
            j += 7;
            argv[i][j + 1] = 'f';
          } else if (STRCMP(argv[i] + j + 1, "iteration") == 0) {
            j += 8;
            argv[i][j + 1] = 'i';
          } else if (STRCMP(argv[i] + j + 1, "max_depth") == 0) {
            j += 8;
            argv[i][j + 1] = 'd';
          } else if (STRCMP(argv[i] + j + 1, "quiet") == 0) {
            j += 4;
            argv[i][j + 1] = 'q';
          } else if (STRCMP(argv[i] + j + 1, "verbose") == 0) {
            j += 6;
            argv[i][j + 1] = 'v';
          } else if (STRCMP(argv[i] + j + 1, "parallel") == 0) {
            j += 7;
            argv[i][j + 1] = 'p';
          } else if (STRCMP(argv[i] + j + 1, "keep-exif") == 0) {
            j += 9;
            Jpeg::keep_exif_ = true;
          } else if (STRCMP(argv[i] + j + 1, "keep-icc") == 0) {
            j += 8;
            Jpeg::keep_icc_profile_ = true;
            Png::keep_icc_profile_ = true;
          } else if (STRCMP(argv[i] + j + 1, "jpeg-keep-all") == 0) {
            j += 13;
            Jpeg::keep_all_metadata_ = true;
          } else if (STRCMP(argv[i] + j + 1, "jpeg-arithmetic") == 0) {
            j += 15;
            Jpeg::force_arithmetic_coding_ = true;
          } else if (STRCMP(argv[i] + j + 1, "zip-deflate") == 0) {
            j += 11;
            Zip::force_deflate_ = true;
          } else {
#ifdef _WIN32
            char mbs[64] = { 0 };
            WideCharToMultiByte(CP_ACP, 0, argv[i] + j + 1, -1, mbs, sizeof(mbs) - 1, nullptr, nullptr);
            cerr << "Unknown option: " << mbs << endl;
#else
            cerr << "Unknown option: " << argv[i] + j + 1 << endl;
#endif  // _WIN32
            PrintInfo();
            return 1;
          }
          break;
        default:
          cerr << "Unknown option: " << (char)argv[i][j] << endl;
          PrintInfo();
          return 1;
      }
    }
    i += num_optargs;
  }

  if (parallel_processing && is_verbose) {
    cerr << "Verbose logs not supported in parallel mode." << endl;
    PrintInfo();
    return 1;
  }

  if (i == argc) {
    cerr << "No file path provided." << endl;
    PrintInfo();
    return 1;
  }

  cout << std::fixed;
  cout.precision(2);

  // support multiple input file
  do {
    TraversePath(argv[i], EnqueueProcessFileTask);
  } while (++i < argc);

  size_t parallel_tasks = 1;
  if (parallel_processing)
    parallel_tasks = std::thread::hardware_concurrency();
  
  tf::Executor executor(parallel_tasks);
  executor.run(taskflow).wait(); 
  
  PauseIfNotTerminal();

  return 0;
}
