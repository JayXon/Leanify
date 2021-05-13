#include "library.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <locale>
#include <string>

// Using this library may require additional compiler/linker options.
// GNU implementation prior to 9.1 requires linking with -lstdc++fs and LLVM implementation
// prior to LLVM 9.0 requires linking with -lc++fs.
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <SHA1/sha1.hpp>

constexpr auto PATH_SEPARATOR = '/';

class Storage {
 public:
  virtual LibraryEntry* GetEntry(void* data, size_t dataSize, const char* tag) = 0;
  virtual const std::string& GetStorageName() = 0;
};

static Storage* LibraryStorage = NULL;

class FileEntry : public LibraryEntry {
 private:
  std::string _path;

 public:
  FileEntry(const std::string& path) : _path(path) {}

 public:
  virtual bool isExists() {
    return std::filesystem::exists(_path);
  }
  virtual void Save(void* data, size_t dataSize) {
    std::ofstream fout(_path, std::ios_base::binary);
    fout.write((const char*)data, dataSize);
    fout.close();
  }
  virtual size_t Load(void* data, size_t dataSize) {
    auto size = std::filesystem::file_size(_path);
    std::ifstream fin(_path, std::ios_base::binary);
    fin.read((char*)data, size);
    fin.close();
    return size;
  }
};

int dirExists(const char* path) {
  struct stat info;

  if (stat(path, &info) != 0)
    return 0;
  else if (info.st_mode & S_IFDIR)
    return 1;
  else
    return 0;
}

template <class I, class E, class S>
struct codecvt : std::codecvt<I, E, S> {
  ~codecvt() {}
};

std::string pathToString(const std::filesystem::path& path) {
  typedef codecvt<std::filesystem::path::value_type, char, std::mbstate_t> Codecvt;
  std::wstring_convert<Codecvt, std::filesystem::path::value_type> converter;
  return converter.to_bytes(std::filesystem::temp_directory_path());
}

class DirectoryStorage : public Storage {
 private:
  std::string _pathToDirectory;

 protected:
  LibraryEntry* CreateEntry(const std::string hash) {
    auto firstTwo = std::string(hash, 0, 2);

    auto directory = _pathToDirectory + PATH_SEPARATOR + firstTwo;
    std::filesystem::create_directories(directory);
    auto fullPath = directory + PATH_SEPARATOR + hash;
    return new FileEntry(fullPath);
  }

 public:
  DirectoryStorage(const std::string pathToDirectory) : _pathToDirectory(pathToDirectory) {
    if (_pathToDirectory == "*") {
      _pathToDirectory = pathToString(std::filesystem::temp_directory_path());
      _pathToDirectory += "leanify_library";
    }
    std::filesystem::create_directories(_pathToDirectory);
    if (!dirExists(_pathToDirectory.c_str()))
      throw std::runtime_error("Library directory not exists: " + _pathToDirectory);
  }

  LibraryEntry* GetEntry(void* data, size_t dataSize, const char* tag) {
    auto hash = sha1(tag);
    hash.add(data, dataSize);
    char hex[SHA1_HEX_SIZE];
    hash.print_hex(hex);
    return CreateEntry(hex);
  }
  const std::string& GetStorageName() {
    return _pathToDirectory;
  }
};

#ifdef _WIN32
void Library::Initialize(const std::wstring& library) {
  char mbs[MAX_PATH] = { 0 };
  WideCharToMultiByte(CP_ACP, 0, library.c_str(), -1, mbs, sizeof(mbs) - 1, nullptr, nullptr);
  LibraryStorage = new DirectoryStorage(mbs);
}
#else
void Library::Initialize(const std::string& library) {
  LibraryStorage = new DirectoryStorage(library);
}
#endif
LibraryEntry* Library::GetEntry(void* data, size_t dataSize, const char* tag) {
  return LibraryStorage ? LibraryStorage->GetEntry(data, dataSize, tag) : NULL;
}

const std::string& Library::GetStorageName() {
  static std::string none = "None";
  return LibraryStorage ? LibraryStorage->GetStorageName() : none;
}