#ifndef FILEIO_H
#define FILEIO_H

#include <cstddef>

#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <Windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif // _WIN32


class File
{
public:
#ifdef _WIN32
    explicit File(const wchar_t *filepath);
#else
    explicit File(const char *filepath);
#endif // _WIN32

    void *GetFilePionter() const
    {
        return fp_;
    }

    unsigned int GetSize() const
    {
        return size_;
    }

    bool IsOK() const
    {
        return fp_ != nullptr;
    }

    void UnMapFile(size_t new_size);

private:
#ifdef _WIN32
    HANDLE hFile_, hMap_;
#else
    int fd_;
#endif // _WIN32
    void *fp_;
    size_t size_;
};


#ifdef _WIN32
void TraverseDirectory(const wchar_t Dir[], int Callback(const wchar_t file_path[]));
bool IsDirectory(const wchar_t path[]);
#else
void TraverseDirectory(const char Dir[], int Callback(const char file_path[], const struct stat *sb, int typeflag));
bool IsDirectory(const char path[]);
#endif // _WIN32


#endif