#pragma once
#include "Fat32.h"
#include "uchar.h"

// Class for reading a Fat32-formatted device/partition/disk/...
// and support recovering deleted files/directories as well.
class Fat32Recoverer
{
private:
  Fat32Device device{}; // Store read device/partition/disk/... here.

  // Cached deleted entries (file/directory) for later use.
  // 2D vector because each valid file/directory entry can have zero to multiple
  // long file name entries, so each valid entry will take up a vector
  // with long file name entries at the front and
  // the main file/directory entry at the back.
  std::vector<std::vector<FAT32Entry>> deletedEntries{}; 

  // Private method for retrieving a main entry's name from a set of entries,
  // with long file name entries at the front and main file/directory entry at the back.
  // Support both long file name and short file name (as fallback).
  // Do note that only ASCII characters are supported for now,
  // other characters from long file name is turned into '?',
  // and deleted marker of short file name turned into '_'.
  std::string getEntryNameAscii(const std::vector<FAT32Entry> &entry);

  // Private method for supporting writting a file to a path when recovering a file.
  void writeFileToPath(const std::vector<uint8_t> &fileData, const std::string outputDir);

  // Private method for recovering a specific type of entry (file/dirrectory).
  // Called by recoverDeletedEntry when the right type is determined.
  void recoverDeletedFile(const std::vector<FAT32Entry> &entry, const std::string_view outputDir);
  void recoverDeletedDir(const std::vector<FAT32Entry> &entry, const std::string_view outputDir);

public:
  // Default constructor.
  Fat32Recoverer() = default;

  // Take a device/partition/disk/...'s path to start reading immediately, also read deleted entries.
  Fat32Recoverer(const std::string_view path);

  // Disabled copy and move semantics.
  Fat32Recoverer(const Fat32Recoverer &) = delete;
  Fat32Recoverer &operator=(const Fat32Recoverer &) = delete;

  // Destructor.
  ~Fat32Recoverer() = default;

  // Public method for reading device/partition/disk/... as well as its deleted entries.
  // Used by constructor, but user can use this as well.
  void readDevice(const std::string_view path);

  // Public method for printing deleted entries to console.
  // Useful for console app UI.
  // List starts at #1 for index #0.
  void printDeletedEntriesConsole();

  bool entryisDir(const FAT32Entry &entry);
  bool entryisFile(const FAT32Entry &entry);
  bool entryisLongFileName(const FAT32Entry &entry);
  bool entryisDeleted(const FAT32Entry &entry);

  // Public method for reading deleted entries, each entry
  // can be accompanied by multiple long file name entries.
  // Made public for user to refresh deleted entries list in case
  // they recover file/directory to the same device/partition/disk/...,
  // which can overwrite data, including deleted entries.
  void readDeletedEntries();

  // Public method for recovering a desired deleted entry with its index
  // in deletedEntries member. If used with printDeletedEntriesConsole,
  // element #1 in list becomes 0 in index and so on.
  void recoverDeletedEntry(const std::size_t index, const std::string_view outputDir);
};
