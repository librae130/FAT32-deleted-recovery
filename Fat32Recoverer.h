#pragma once
#include "Fat32.h"
#include "uchar.h"

class Fat32Recoverer
{
private:
  Fat32Device device{};

  std::vector<std::vector<FAT32Entry>> deletedEntries{};

  std::string getEntryName(const std::vector<FAT32Entry> &entry);

public:
  Fat32Recoverer() = default;
  Fat32Recoverer(const Fat32Recoverer &) = delete;
  Fat32Recoverer &operator=(const Fat32Recoverer &) = delete;
  Fat32Recoverer(const std::string_view path);
  ~Fat32Recoverer() = default;

  void readDevice(const std::string_view path);
  void printDeletedEntriesConsole();
  void findDeletedEntries();
  bool entryisDir(const FAT32Entry &entry);
  bool entryisFile(const FAT32Entry &entry);
  bool entryisLongFileName(const FAT32Entry &entry);
  bool entryisDeleted(const FAT32Entry &entry);
  void readDeletedEntries();
  void recoverDeletedEntry(const std::size_t index, const std::string_view outputDir);
  void recoverDeletedFile(const std::vector<FAT32Entry> &entry, const std::string_view outputDir);
  void recoverDeletedDir(const std::vector<FAT32Entry> &entry, const std::string_view outputDir);
};
