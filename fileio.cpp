#include "fileio.h"



// traverse directory and call Callback() for each file
#ifdef _WIN32
void TraverseDirectory(const wchar_t Dir[], int Callback(const wchar_t file_path[]))
{
    WIN32_FIND_DATA FindFileData;
    wchar_t DirSpec[MAX_PATH];
    //DWORD dwError;  
    lstrcpy(DirSpec, Dir);
    lstrcat(DirSpec, L"\\*");

    HANDLE hFind = FindFirstFile(DirSpec, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
        return;
    }

    while (FindNextFile(hFind, &FindFileData) != 0)
    {
        if (FindFileData.cFileName[0] == '.')
        {
            if (FindFileData.cFileName[1] == 0 ||               // "."
                lstrcmp(FindFileData.cFileName + 1, L".") == 0) // ".."
            {
                continue;
            }
        }

        wchar_t DirAdd[MAX_PATH];
        lstrcpy(DirAdd, Dir);
        lstrcat(DirAdd, L"\\");
        lstrcat(DirAdd, FindFileData.cFileName);

        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // directory
            TraverseDirectory(DirAdd, Callback);
        }
        else
        {
            // file
            Callback(DirAdd);
        }
    }

    FindClose(hFind);
}
#else
void TraverseDirectory(const char Dir[], int Callback(const char file_path[], const struct stat *sb, int typeflag))
{
    if (ftw(Dir, Callback, 16))
    {
        perror("ftw");
    }
}
#endif // _WIN32


#ifdef _WIN32
bool IsDirectory(const wchar_t path[])
{
    DWORD fa = GetFileAttributes(path);
    if (fa == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    return (fa & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
#else
bool IsDirectory(const char path[])
{
    struct stat sb;
    if (!stat(path, &sb))
    {
        return (sb.st_mode & S_IFDIR) != 0;
    }
    return false;
}
#endif // _WIN32


#ifdef _WIN32
File::File(const wchar_t *filepath)
{
    fp = nullptr;
    hFile = CreateFile(filepath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_WRITE_THROUGH, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintErrorMessage("Open file error!");
        size = 0;
        return;
    }
    size = GetFileSize(hFile, NULL);
    hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (hMap == INVALID_HANDLE_VALUE)
    {
        PrintErrorMessage("Map file error!");
        return;
    }
    fp = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
}


void File::UnMapFile(size_t new_size)
{
    if (new_size < size)
    {
        if (!FlushViewOfFile(fp, 0))
        {
            PrintErrorMessage("Write file error!");
        }
    }
    if (!UnmapViewOfFile(fp))
    {
        PrintErrorMessage("UnmapViewOfFile error!");
    }
    CloseHandle(hMap);
    if (new_size)
    {
        SetFilePointer(hFile, new_size, NULL, FILE_BEGIN);
        if (!SetEndOfFile(hFile))
        {
            PrintErrorMessage("SetEndOfFile error!");
        }
    }
    CloseHandle(hFile);
}
void File::PrintErrorMessage(const char *msg)
{
    char *error_msg = NULL;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0, (char *)&error_msg, 0, NULL);
    std::cerr << msg << std::endl;
    if (error_msg)
    {
        std::cerr << error_msg << std::endl;
        LocalFree(error_msg);
    }
}
#else
File::File(const char *filepath)
{
    fp = nullptr;
    fd = open(filepath, O_RDWR);

    if(fd == -1)
    {
        perror("Open file error");
        return;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        perror("fstat");
        return;
    }
    size = sb.st_size;

    // map the file into memory
    fp = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fp == MAP_FAILED)
    {
        perror("Map file error");
        fp = nullptr;
    }
}


void File::UnMapFile(size_t new_size)
{
    if (munmap(fp, size) == -1)
    {
        perror("munmap");
    }
    if (new_size)
    {
        if (ftruncate(fd, new_size) == -1)
        {
            perror("ftruncate");
        }
    }

    close(fd);
}


#endif // _WIN32