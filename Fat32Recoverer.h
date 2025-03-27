#pragma once
#include "Fat32.h"

class Fat32Recoverer
{
private:
  Fat32Device device{};

  std::string convertEntryNameToString(const FAT32Entry &entry);
  std::vector<uint8_t> readClusterData(const uint32_t cluster);

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
  void recoverDeletedEntry(const std::size_t index, const std::string_view outputDir);
  void recoverDeletedFile(const FAT32Entry &entry, const std::string_view outputDir);
  void recoverDeletedDir(const FAT32Entry &entry, const std::string_view outputDir);
};
