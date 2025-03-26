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
  
  bool readDevice(const std::string_view path);
  void printDeletedEntriesConsole();
  void findDeletedEntries();
  bool rootEntryisDir(const FAT32Entry &entry);
  bool rootEntryisFile(const FAT32Entry &entry);
  void recoverDeletedFile(const FAT32Entry &entry, const std::string_view outputDir);
  void recoverDeletedFolder(const FAT32Entry &entry, const std::string_view outputDir);
};
