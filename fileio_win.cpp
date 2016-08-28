#include "fileio.h"

#include <cstdio>
#include <iostream>

using std::cerr;
using std::endl;

// traverse directory and call Callback() for each file
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

bool IsDirectory(const wchar_t path[]) {
  DWORD fa = GetFileAttributes(path);
  if (fa == INVALID_FILE_ATTRIBUTES)
    return false;
  return (fa & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

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
