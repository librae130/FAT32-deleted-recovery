#include "fat32.h"
#include <iostream>
#include <iomanip>
#include <cstring>

FAT32Reader::FAT32Reader(const std::string_view path)
    : devicePath(path)
{
  device.open(devicePath, std::ios::binary | std::ios::in);
  read(devicePath);
}

FAT32Reader::~FAT32Reader()
{
  if (device.is_open())
    device.close();
}

bool FAT32Reader::validate()
{
  if (std::string_view(reinterpret_cast<char *>(bootSector->fatName)) != "FAT32   ")
  {
    std::cerr << "Not a FAT32 filesystem" << std::endl;
    return false;
  }

  return true;
}

bool FAT32Reader::read(const std::string_view path)
{
  device.open(devicePath, std::ios::binary | std::ios::in);
  if (!device.is_open())
  {
    std::cerr << "Failed to open: " << devicePath << std::endl;
    return false;
  }
  devicePath = path;
  readDeviceBootSector();
  validate();
  readDeviceFatTable();
}
bool FAT32Reader::readDeviceBootSector()
{
  bootSector = std::make_unique<FAT32BootSector>();

  device.read(reinterpret_cast<char *>(bootSector->jumpBoot), sizeof(bootSector->jumpBoot));
  device.read(reinterpret_cast<char *>(bootSector->oemName), sizeof(bootSector->oemName));
  device.read(reinterpret_cast<char *>(&bootSector->bytesPerSector), sizeof(bootSector->bytesPerSector));
  device.read(reinterpret_cast<char *>(&bootSector->sectorsPerCluster), sizeof(bootSector->sectorsPerCluster));
  device.read(reinterpret_cast<char *>(&bootSector->reservedSectorCount), sizeof(bootSector->reservedSectorCount));
  device.read(reinterpret_cast<char *>(&bootSector->fatCount), sizeof(bootSector->fatCount));
  device.read(reinterpret_cast<char *>(&bootSector->rootEntryCount), sizeof(bootSector->rootEntryCount));
  device.read(reinterpret_cast<char *>(&bootSector->sectorCount), sizeof(bootSector->sectorCount));
  device.read(reinterpret_cast<char *>(&bootSector->mediaDescriptor), sizeof(bootSector->mediaDescriptor));
  device.read(reinterpret_cast<char *>(&bootSector->sectorsPerFatUnused), sizeof(bootSector->sectorsPerFatUnused));
  device.read(reinterpret_cast<char *>(&bootSector->sectorPerTrack), sizeof(bootSector->sectorPerTrack));
  device.read(reinterpret_cast<char *>(&bootSector->headCount), sizeof(bootSector->headCount));
  device.read(reinterpret_cast<char *>(&bootSector->hiddenSectorCount), sizeof(bootSector->hiddenSectorCount));
  device.read(reinterpret_cast<char *>(&bootSector->sectorTotal), sizeof(bootSector->sectorTotal));
  device.read(reinterpret_cast<char *>(&bootSector->sectorsPerFat), sizeof(bootSector->sectorsPerFat));
  device.read(reinterpret_cast<char *>(&bootSector->flags), sizeof(bootSector->flags));
  device.read(reinterpret_cast<char *>(&bootSector->driveVersion), sizeof(bootSector->driveVersion));
  device.read(reinterpret_cast<char *>(&bootSector->rootDirStartCluster), sizeof(bootSector->rootDirStartCluster));
  device.read(reinterpret_cast<char *>(&bootSector->fsInfoSector), sizeof(bootSector->fsInfoSector));
  device.read(reinterpret_cast<char *>(&bootSector->backupBootSector), sizeof(bootSector->backupBootSector));
  device.read(reinterpret_cast<char *>(bootSector->reserved), sizeof(bootSector->reserved));
  device.read(reinterpret_cast<char *>(&bootSector->driveNumber), sizeof(bootSector->driveNumber));
  device.read(reinterpret_cast<char *>(&bootSector->unused), sizeof(bootSector->unused));
  device.read(reinterpret_cast<char *>(&bootSector->bootSignature), sizeof(bootSector->bootSignature));
  device.read(reinterpret_cast<char *>(&bootSector->volumeId), sizeof(bootSector->volumeId));
  device.read(reinterpret_cast<char *>(bootSector->volumeLabel), sizeof(bootSector->volumeLabel));
  device.read(reinterpret_cast<char *>(bootSector->fatName), sizeof(bootSector->fatName));
  device.read(reinterpret_cast<char *>(bootSector->executableCode), sizeof(bootSector->executableCode));
  device.read(reinterpret_cast<char *>(bootSector->bootRecordSignature), sizeof(bootSector->bootRecordSignature));

  if (!device)
  {
    std::cerr << "Failed when reading boot sector" << std::endl;
    return false;
  }

  return true;
}

bool FAT32Reader::readDeviceFatTable()
{
  uint32_t fatTableSize{bootSector->sectorsPerFat * bootSector->bytesPerSector};
  fatTable.resize(fatTableSize / sizeof(uint32_t));
  device.seekg(bootSector->reservedSectorCount * bootSector->bytesPerSector);
  device.read(reinterpret_cast<char *>(fatTable.data()), fatTableSize);

  if (!device)
  {
    std::cerr << "Failed when reading fat table" << std::endl;
    return false;
  }

  return true;
}

void FAT32Reader::listRootDirectoryDeletedEntries()
{
  uint32_t currentCluster = bootSector->rootDirStartCluster;
  uint32_t bytesPerCluster = bootSector->bytesPerSector * bootSector->sectorsPerCluster;
  std::vector<FAT32DirectoryEntry> entries(bytesPerCluster / sizeof(FAT32DirectoryEntry));

  while (currentCluster >= 0x2 && currentCluster < 0x0FFFFFF8)
  {
    if (currentCluster == 0x0FFFFFF7)
    {
      currentCluster = fatTable[currentCluster];
      continue;
    }

    device.seekg(currentCluster * bytesPerCluster);
    device.read(reinterpret_cast<char *>(entries.data()), bytesPerCluster);

    for (const auto &entry : entries)
    {
      if (entry.name[0] == 0xE5)
      {
        std::string name(reinterpret_cast<const char *>(entry.name + 1), 10);
        std::cout << "Deleted File: " << name << std::endl;
      }
    }

    currentCluster = fatTable[currentCluster];
  }
}
