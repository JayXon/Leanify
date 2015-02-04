#include "main.h"

#include <climits>
#include <cstring>
#include <iomanip>
#include <iostream>

#ifndef _WIN32
#include <ftw.h>
#endif

#include "leanify.h"
#include "fileio.h"
#include "version.h"


void PrintSize(size_t size)
{
    if (size < 1024)
    {
        std::cout << size << " B";
    }
    else if (size < 1024 * 1024)
    {
        std::cout << size / 1024.0 << " KB";
    }
    else
    {
        std::cout << size / 1024.0 / 1024.0 << " MB";
    }
}


#ifdef _WIN32
int ProcessFile(const wchar_t *file_path)
{
    char mbs[MAX_PATH] = { 0 };
    WideCharToMultiByte(CP_ACP, 0, file_path, -1, mbs, sizeof(mbs) - 1, nullptr, nullptr);
    std::cout << "Processing: " << mbs << std::endl;
#else
// written like this in order to be callback funtion of ftw()
int ProcessFile(const char file_path[], const struct stat *sb = nullptr, int typeflag = FTW_F)
{
    if (typeflag != FTW_F)
    {
        return 0;
    }
    std::cout << "Processing: " << file_path << std::endl;
#endif // _WIN32


    File input_file(file_path);

    if (input_file.IsOK())
    {
        size_t original_size = input_file.GetSize();

        size_t new_size = LeanifyFile(input_file.GetFilePionter(), original_size);

        PrintSize(original_size);
        std::cout << " -> ";
        PrintSize(new_size);
        std::cout << "\tLeanified: ";
        PrintSize(original_size - new_size);

        std::cout << " (" << 100 - 100.0 * new_size / original_size << "%)" << std::endl;

        input_file.UnMapFile(new_size);
    }

    return 0;
}


void PauseIfNotTerminal()
{
    // pause if Leanify is not started in terminal
    // so that user can see the output instead of just a flash of a black box
#ifdef _WIN32
    if (is_pause)
    {
        system("pause");
    }
#endif // _WIN32
}



void PrintInfo()
{
    std::cerr << "Leanify\t" << VERSION_STR << std::endl << std::endl;
    std::cerr <<
        "Usage: Leanify [options] paths\n"
        "  -i, --iteration <iteration>   More iterations produce better result, but\n"
        "                                  use more time, default is 15.\n"
        "  -d, --max_depth <max depth>   Maximum recursive depth, unlimited by default.\n"
        "                                  Set to 1 will disable recursive minifying.\n"
        "  -f, --fastmode                Fast mode, no recompression.\n"
        "  -q, --quiet                   No output to stdout.\n"
        "  -v, --verbose                 Verbose output." << std::endl;
    PauseIfNotTerminal();
}


#ifdef _WIN32
int main()
{
    int argc;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
#else
int main(int argc, char *argv[])
{
#endif // _WIN32

    is_fast = false;
    is_verbose = false;
    iterations = 15;
    depth = 1;
    max_depth = INT_MAX;

#ifdef _WIN32
    is_pause = !getenv("PROMPT");
#endif // _WIN32

    int i;
    for (i = 1; i < argc && argv[i][0] == L'-'; i++)
    {
#ifdef _WIN32
        // do not pause if any options are given
        is_pause = false;
#endif // _WIN32
        int num_optargs = 0;
        for (int j = 1; argv[i][j]; j++)
        {
            switch (argv[i][j])
            {
            case 'f':
                is_fast = true;
                break;
            case 'i':
                if (i < argc - 1)
                {
                    iterations = STRTOL(argv[i + ++num_optargs], nullptr, 10);
                    // strtol will return 0 on fail
                    if (iterations == 0)
                    {
                        std::cerr << "There should be a positive number after -i option." << std::endl;
                        PrintInfo();
                        return 1;
                    }
                }
                break;
            case 'd':
                if (i < argc - 1)
                {
                    max_depth = STRTOL(argv[i + ++num_optargs], nullptr, 10);
                    // strtol will return 0 on fail
                    if (max_depth == 0)
                    {
                        std::cerr << "There should be a positive number after -d option." << std::endl;
                        PrintInfo();
                        return 1;
                    }
                }
                break;
            case 'q':
                std::cout.setstate(std::ios::failbit);
                is_verbose = false;
                break;
            case 'v':
                std::cout.clear();
                is_verbose = true;
                break;
            case '-':
                if (STRCMP(argv[i] + j + 1, "fastmode") == 0)
                {
                    j += 7;
                    argv[i][j + 1] = 'f';
                }
                else if (STRCMP(argv[i] + j + 1, "iteration") == 0)
                {
                    j += 8;
                    argv[i][j + 1] = 'i';
                }
                else if (STRCMP(argv[i] + j + 1, "max_depth") == 0)
                {
                    j += 8;
                    argv[i][j + 1] = 'd';
                }
                else if (STRCMP(argv[i] + j + 1, "quiet") == 0)
                {
                    j += 4;
                    argv[i][j + 1] = 'q';
                }
                else if (STRCMP(argv[i] + j + 1, "verbose") == 0)
                {
                    j += 6;
                    argv[i][j + 1] = 'v';
                }
                else
                {
#ifdef _WIN32
                    char mbs[64] = { 0 };
                    WideCharToMultiByte(CP_ACP, 0, argv[i] + j + 1, -1, mbs, sizeof(mbs) - 1, nullptr, nullptr);
                    std::cerr << "Unknown option: " << mbs << std::endl;
#else
                    std::cerr << "Unknown option: " << argv[i] + j + 1 << std::endl;
#endif // _WIN32
                    PrintInfo();
                    return 1;
                }
                break;
            default:
                std::cerr << "Unknown option: " << (char)argv[i][j] << std::endl;
                PrintInfo();
                return 1;
            }
        }
        i += num_optargs;
    }

    if (i == argc)
    {
        std::cerr << "No file path provided." << std::endl;
        PrintInfo();
        return 1;
    }


    std::cout << std::fixed;
    std::cout.precision(2);

    // support multiple input file
    do
    {
        if (IsDirectory(argv[i]))
        {
            // directory
            TraverseDirectory(argv[i], ProcessFile);
        }
        else
        {
            // file
            ProcessFile(argv[i]);
        }

    }
    while (++i < argc);


    PauseIfNotTerminal();

    return 0;
}