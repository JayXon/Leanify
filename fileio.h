#ifndef FILEIO_H_
#define FILEIO_H_

#include <cstddef>

#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif  // UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif  // _UNICODE
#include <Windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif  // _WIN32

class File {
 public:
#ifdef _WIN32
  explicit File(const wchar_t* filepath);
#else
  explicit File(const char* filepath);
#endif  // _WIN32

  ~File() {
    if (fp_ != nullptr)
      UnMapFile(size_);
  }

  // Disallow copy and assign.
  File(const File&) = delete;
  File& operator=(const File&) = delete;

  void* GetFilePointer() const {
    return fp_;
  }

  unsigned int GetSize() const {
    return size_;
  }

  bool IsOK() const {
    return fp_ != nullptr;
  }

  void UnMapFile(size_t new_size);

 private:
#ifdef _WIN32
  HANDLE hFile_ = INVALID_HANDLE_VALUE;
  HANDLE hMap_ = INVALID_HANDLE_VALUE;
#else
  int fd_ = -1;
#endif  // _WIN32
  void* fp_ = nullptr;
  size_t size_ = 0;
};

#ifdef _WIN32
void TraversePath(const wchar_t* dir, int Callback(const wchar_t* file_path));
#else
void TraversePath(const char* dir, int Callback(const char* file_path, const struct stat* sb, int typeflag));
#endif  // _WIN32

#endif  // FILEIO_H_
