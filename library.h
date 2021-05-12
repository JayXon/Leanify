#ifndef LIBRARY_H_
#define LIBRARY_H_

#include <string>

class LibraryEntry;

class Library 
{
 public:
#ifdef _WIN32
 static void Initialize(const std::wstring& library);
#else
 static void Initialize(const std::string& library);
#endif

 static LibraryEntry* GetEntry(void* data, size_t dataSize, const char* tag);
 static const std::string& GetStorageName();

};

class LibraryEntry 
{
 public:
  virtual bool isExists() = 0;
  virtual void Save(void* data, size_t dataSize) = 0;
  virtual size_t Load(void* data, size_t dataSize) = 0;
  virtual ~LibraryEntry() {}
};

#endif