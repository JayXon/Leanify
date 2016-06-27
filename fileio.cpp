#include "fileio.h"

#include <cstdio>
#include <iostream>

#ifndef _WIN32
#include <sys/mman.h>

#include <fcntl.h>
#include <ftw.h>
#include <unistd.h>
#endif

using std::cerr;
using std::endl;

// traverse directory and call Callback() for each file
#ifdef _WIN32
void TraverseDirectory(const wchar_t Dir[], int Callback(const wchar_t file_path[])) {
  WIN32_FIND_DATA FindFileData;
  wchar_t DirSpec[MAX_PATH];
  // DWORD dwError;
  lstrcpy(DirSpec, Dir);
  lstrcat(DirSpec, L"\\*");

  HANDLE hFind = FindFirstFile(DirSpec, &FindFileData);

  if (hFind == INVALID_HANDLE_VALUE) {
    FindClose(hFind);
    return;
  }

  while (FindNextFile(hFind, &FindFileData) != 0) {
    if (FindFileData.cFileName[0] == '.') {
      if (FindFileData.cFileName[1] == 0 ||                // "."
          lstrcmp(FindFileData.cFileName + 1, L".") == 0)  // ".."
        continue;
    }

    wchar_t DirAdd[MAX_PATH];
    lstrcpy(DirAdd, Dir);
    lstrcat(DirAdd, L"\\");
    lstrcat(DirAdd, FindFileData.cFileName);

    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      // directory
      TraverseDirectory(DirAdd, Callback);
    } else {
      // file
      Callback(DirAdd);
    }
  }

  FindClose(hFind);
}
#else
void TraverseDirectory(const char Dir[], int Callback(const char file_path[], const struct stat* sb, int typeflag)) {
  if (ftw(Dir, Callback, 16))
    perror("ftw");
}
#endif  // _WIN32

#ifdef _WIN32
bool IsDirectory(const wchar_t path[]) {
  DWORD fa = GetFileAttributes(path);
  if (fa == INVALID_FILE_ATTRIBUTES)
    return false;
  return (fa & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
#else
bool IsDirectory(const char path[]) {
  struct stat sb;
  if (!stat(path, &sb))
    return (sb.st_mode & S_IFDIR) != 0;
  return false;
}
#endif  // _WIN32

#ifdef _WIN32
namespace {

void PrintErrorMessage(const char* msg) {
  char* error_msg = nullptr;
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, GetLastError(), 0,
                 reinterpret_cast<char*>(&error_msg), 0, nullptr);
  cerr << msg << endl;
  if (error_msg) {
    cerr << error_msg << endl;
    LocalFree(error_msg);
  }
}

}  // namespace

File::File(const wchar_t* filepath) {
  fp_ = nullptr;
  hFile_ = CreateFile(filepath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_WRITE_THROUGH, nullptr);
  if (hFile_ == INVALID_HANDLE_VALUE) {
    PrintErrorMessage("Open file error!");
    size_ = 0;
    return;
  }
  size_ = GetFileSize(hFile_, nullptr);
  hMap_ = CreateFileMapping(hFile_, nullptr, PAGE_READWRITE, 0, 0, nullptr);
  if (hMap_ == INVALID_HANDLE_VALUE) {
    PrintErrorMessage("Map file error!");
    return;
  }
  fp_ = MapViewOfFile(hMap_, FILE_MAP_ALL_ACCESS, 0, 0, 0);
}

void File::UnMapFile(size_t new_size) {
  if (new_size < size_)
    if (!FlushViewOfFile(fp_, 0))
      PrintErrorMessage("Write file error!");

  if (!UnmapViewOfFile(fp_))
    PrintErrorMessage("UnmapViewOfFile error!");

  CloseHandle(hMap_);
  if (new_size) {
    SetFilePointer(hFile_, new_size, nullptr, FILE_BEGIN);
    if (!SetEndOfFile(hFile_))
      PrintErrorMessage("SetEndOfFile error!");
  }
  CloseHandle(hFile_);
  fp_ = nullptr;
}
#else
File::File(const char* filepath) {
  fp_ = nullptr;
  fd_ = open(filepath, O_RDWR);

  if (fd_ == -1) {
    perror("Open file error");
    return;
  }

  struct stat sb;
  if (fstat(fd_, &sb) == -1) {
    perror("fstat");
    return;
  }
  size_ = sb.st_size;

  // map the file into memory
  fp_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  if (fp_ == MAP_FAILED) {
    perror("Map file error");
    fp_ = nullptr;
  }
}

void File::UnMapFile(size_t new_size) {
  if (munmap(fp_, size_) == -1)
    perror("munmap");
  if (new_size)
    if (ftruncate(fd_, new_size) == -1)
      perror("ftruncate");

  close(fd_);
  fp_ = nullptr;
}

#endif  // _WIN32