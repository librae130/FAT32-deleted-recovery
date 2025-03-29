#pragma once
#include <cstdint>
#include <string_view>
#include <vector>
#include <fstream>
#include <iostream>
#include <memory>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

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

#pragma pack(push, 1)
// FAT32 Directory Entry structure
struct FAT32Entry
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
  uint32_t size;
};
#pragma pack(pop)

class Fat32Device
{
private:
  std::string devicePath{};
  std::fstream device{};

  std::unique_ptr<FAT32BootSector> bootSector{nullptr};
  std::vector<uint32_t> fatTable{};
  std::vector<FAT32Entry> deletedEntries{};

  void readBootSector();
  void readFatTable();
  bool isFat32();

public:
  Fat32Device() = default;
  Fat32Device(const Fat32Device &) = delete;
  Fat32Device &operator=(const Fat32Device &) = delete;
  Fat32Device(const std::string_view path);
  ~Fat32Device();

  const std::unique_ptr<FAT32BootSector> &getBootSector() { return bootSector; }
  const std::vector<uint32_t> &getFatTable() { return fatTable; }
  const std::vector<FAT32Entry> &getDeletedEntries() { return deletedEntries; }

  void readDevice(const std::string_view path);
  std::vector<uint8_t> readClusterData(const uint32_t cluster);
  std::vector<FAT32Entry> readClusterEntries(const uint32_t cluster);
  const std::vector<FAT32Entry>& readEntries();
  const std::vector<FAT32Entry>& readDeletedEntries();
};
