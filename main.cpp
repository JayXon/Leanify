#include "main.h"

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
    char mbs[256] = { 0 };
    WideCharToMultiByte(CP_ACP, 0, file_path, -1, mbs, sizeof(mbs), nullptr, nullptr);
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
    if (!getenv("PROMPT"))
    {
        system("pause");
    }
#endif // _WIN32
}



void PrintInfo()
{
    std::cerr << "Leanify\t" << VERSION << std::endl << std::endl;
    std::cerr << "Usage: Leanify [options] paths" << std::endl;
    std::cerr << "  -i iteration\tMore iterations means slower but better result. Default: 15." << std::endl;
    std::cerr << "  -f\t\tFast mode, no recompression." << std::endl;
    std::cerr << "  -q\t\tQuiet mode, no output." << std::endl;
    std::cerr << "  -v\t\tVerbose output." << std::endl;
    PauseIfNotTerminal();
}


#ifdef _WIN32
int main()
{
    int argc;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
#else
int main(int argc, char const *argv[])
{
#endif // _WIN32

    is_fast = false;
    is_verbose = false;
    iterations = 15;
    level = 0;

    int i;
    for (i = 1; i < argc && argv[i][0] == L'-'; i++)
    {
        bool need_plus1 = false;
        for (int j = 1; argv[i][j]; j++)
        {
            switch (argv[i][j])
            {
            case 'f':
            case 'F':
                is_fast = true;
                break;
            case 'i':
            case 'I':
                if (i < argc - 1)
                {
#ifdef _WIN32
                    iterations = wcstol(argv[i + 1], nullptr, 10);
#else
                    iterations = strtol(argv[i + 1], nullptr, 10);
#endif // _WIN32
                    // strtol will return 0 on fail
                    if (iterations == 0)
                    {
                        std::cerr << "There should be a positive number after -i option." << std::endl;
                        PrintInfo();
                        return 1;
                    }
                    need_plus1 = true;
                }
                break;
            case 'q':
            case 'Q':
                std::cout.setstate(std::ios::failbit);
                is_verbose = false;
                break;
            case 'v':
            case 'V':
                std::cout.clear();
                is_verbose = true;
                break;
            default:
                PrintInfo();
                return 1;
            }
        }
        if (need_plus1)
        {
            i++;
        }
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

    } while (++i < argc);


    PauseIfNotTerminal();

    return 0;
}