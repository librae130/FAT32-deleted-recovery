#pragma once
#include <cstdint>
#include <string_view>
#include <vector>
#include <fstream>
#include <iostream>

// FAT32 Boot Sector structure
struct FAT32BootSector
{
  uint8_t jumpBoot[3];
  uint8_t oemName[8];
  uint16_t bytesPerSector;
  uint8_t sectorsPerCluster;
  uint16_t reservedSectorCount;
  uint8_t fatCount;
  uint16_t rootEntryCount;
  uint16_t sectorCount;
  uint8_t mediaDescriptor;
  uint16_t sectorsPerFatUnused;
  uint16_t sectorPerTrack;
  uint16_t headCount;
  uint32_t hiddenSectorCount;
  uint32_t sectorTotal;
  uint32_t sectorsPerFat;
  uint16_t flags;
  uint16_t driveVersion;
  uint32_t rootDirStartCluster;
  uint16_t fsInfoSector;
  uint16_t backupBootSector;
  uint8_t reserved[12];
  uint8_t driveNumber;
  uint8_t unused;
  uint8_t bootSignature;
  uint32_t volumeId;
  uint8_t volumeLabel[11];
  uint8_t fatName[8];
  uint32_t executableCode[13];
  uint8_t bootRecordSignature[2];
};

// FAT32 Directory Entry structure
struct FAT32DirectoryEntry
{
  uint8_t name[11];
  uint8_t attributes;
  uint8_t reserved;
  uint8_t creationTime[3];
  uint16_t creationDate;
  uint16_t lastAccessedDate;
  uint16_t firstClusterHigh;
  uint16_t lastWrittenTime;
  uint16_t lastWrittenDate;
  uint16_t firstClusterLow;
  uint32_t fileSize;
};

class FAT32Reader
{
private:
  std::string devicePath;
  std::fstream device;

  std::unique_ptr<FAT32BootSector> bootSector;
  std::vector<uint32_t> fatTable;

  bool readDeviceBootSector();
  bool readDeviceFatTable();
  bool validate();

public:
  FAT32Reader(const std::string_view path);
  ~FAT32Reader();

  bool read(const std::string_view path);
  void listRootDirectoryDeletedEntries();
  std::vector<uint8_t> read_file(const std::string &filename);
};
