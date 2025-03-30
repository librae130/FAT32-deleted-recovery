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

// FAT32 Boot Sector structure.
struct FAT32BootSector
{
  uint8_t jumpBoot[3]{};
  uint8_t oemName[8]{};
  uint16_t bytesPerSector{};
  uint8_t sectorsPerCluster{};
  uint16_t reservedSectorCount{};
  uint8_t fatCount{};
  uint16_t rootEntryCount{};
  uint16_t sectorCount{};
  uint8_t mediaDescriptor{};
  uint16_t sectorsPerFatUnused{};
  uint16_t sectorPerTrack{};
  uint16_t headCount{};
  uint32_t hiddenSectorCount{};
  uint32_t sectorTotal{};
  uint32_t sectorsPerFat{};
  uint16_t flags{};
  uint16_t driveVersion{};
  uint32_t rootDirStartCluster{};
  uint16_t fsInfoSector{};
  uint16_t backupBootSector{};
  uint8_t reserved[12]{};
  uint8_t driveNumber{};
  uint8_t unused{};
  uint8_t bootSignature{};
  uint32_t volumeId{};
  uint8_t volumeLabel[11]{};
  uint8_t fatName[8]{};
  uint32_t executableCode[13]{};
  uint8_t bootRecordSignature[2]{};
};

// FAT32 Directory Entry structure, packed for easier reading, but slower.
#pragma pack(push, 1)
struct FAT32Entry
{
  uint8_t name[11]{};
  uint8_t attributes{};
  uint8_t reserved{};
  uint8_t creationTime[3]{};
  uint16_t creationDate{};
  uint16_t lastAccessedDate{};
  uint16_t firstClusterHigh{};
  uint16_t lastWrittenTime{};
  uint16_t lastWrittenDate{};
  uint16_t firstClusterLow{};
  uint32_t size{};
};
#pragma pack(pop)

// Class for reading a Fat32-formatted device/partition/disk/...
class Fat32Device
{
private:
  std::string devicePath{};
  std::fstream device{}; // Device/partition/disk/... is read as file stream

  // Member storing Boot Sector, FAT table and Entries read from device/partition/disk/...
  std::unique_ptr<FAT32BootSector> bootSector{nullptr};
  std::vector<uint32_t> fatTable{};
  std::vector<FAT32Entry> entries{};

  // Private methods for reading Boot Sector, FAT table and Root Entries of device/partition/disk/...
  void readBootSector();
  void readFatTable();
  void readRootEntries(); // Do note that this read ALL types of entry.

  // Private method checking if the read device/partition/disk/... is really FAT32-formatted,
  // used after reading Boot Sector.
  bool isFat32();

public:
  // Default constructor.
  Fat32Device() = default;

  // Take a device/partition/disk/...'s path to start reading Boot Sector, FAT Table and Entries immediately.
  Fat32Device(const std::string_view path);

  // Disabled copy and move semantics.
  Fat32Device(const Fat32Device &) = delete;
  Fat32Device &operator=(const Fat32Device &) = delete;

  // Constructor.
  ~Fat32Device();

  // Public getters for Boot Sector, FAT Table and Root Entries read from device/partition/disk/...
  const std::unique_ptr<FAT32BootSector> &getBootSector() { return bootSector; }
  const std::vector<uint32_t> &getFatTable() { return fatTable; }
  const std::vector<FAT32Entry> &getRootEntries() { return entries; }

  // Public method for reading Boot Sector, FAT Table and Entries device/partition/disk/...,
  // used by constructor, but user can use this as well.
  void readDevice(const std::string_view path);

  // Public method for reading and returning data from a cluster,
  // useful for getting data region's file's contents.
  // Do note that this does not check whether it's root directory or data clusters, maybe later...
  std::vector<uint8_t> readClusterData(const uint32_t cluster);

  // Public method for retrieving entries (4 bytes/entry) available in the cluster,
  // useful for getting entries from directories.
  // Do note that this does not check whether it's directory's clusters or data clusters, maybe later...
  std::vector<FAT32Entry> readClusterEntries(const uint32_t cluster);
};
