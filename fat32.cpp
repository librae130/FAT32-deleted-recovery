#include "fat32.h"

void Fat32Device::printBootSectorInfo()
{
  std::cout << "OEM Name: " << std::string(bootSector->oemName, bootSector->oemName + sizeof(bootSector->oemName)) << '\n';
  std::cout << "Bytes Per Sector: " << bootSector->bytesPerSector << '\n';
  std::cout << "Sectors Per Cluster: " << static_cast<int>(bootSector->sectorsPerCluster) << '\n';
  std::cout << "Root Dir Start Cluster: " << bootSector->rootDirStartCluster << '\n';
  std::cout << "FAT Count: " << static_cast<int>(bootSector->fatCount) << '\n';
  std::cout << "Total Sectors: " << bootSector->sectorTotal << '\n';
  std::cout << "Sectors Per FAT: " << bootSector->sectorsPerFat << '\n';
  std::cout << "Volume ID: " << bootSector->volumeId << '\n';
  std::cout << "Volume Label: " << std::string(bootSector->volumeLabel, bootSector->volumeLabel + sizeof(bootSector->volumeLabel)) << '\n';
}

Fat32Device::Fat32Device(const std::string_view path)
    : devicePath{path}
{
  read(devicePath);
}

Fat32Device::~Fat32Device()
{
  if (device.is_open())
    device.close();
}

bool Fat32Device::isFat32()
{
  if (std::string_view(reinterpret_cast<char *>(bootSector->fatName)) != "FAT32   ")
  {
    std::cerr << "Not a FAT32 filesystem" << std::endl;
    return false;
  }

  return true;
}

bool Fat32Device::read(const std::string_view path)
{
  devicePath = path;
  device.open(devicePath, std::ios::binary | std::ios::in);
  if (!device.is_open())
  {
    std::cerr << "Failed to open: " << devicePath << std::endl;
    return false;
  }
  readDeviceBootSector();

  if (!isFat32())
    return false;

  readDeviceFatTable();
  return true;
}
bool Fat32Device::readDeviceBootSector()
{
  bootSector = std::make_unique<FAT32BootSector>();
  device.seekg(0);
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

bool Fat32Device::readDeviceFatTable()
{
  uint32_t fatTableSize{bootSector->sectorsPerFat * static_cast<uint32_t>(bootSector->bytesPerSector)};
  uint32_t fatOffset{static_cast<uint32_t>(bootSector->reservedSectorCount) * static_cast<uint32_t>(bootSector->bytesPerSector)};
  fatTable.resize(fatTableSize / sizeof(uint32_t));
  device.seekg(fatOffset);
  device.read(reinterpret_cast<char *>(fatTable.data()), fatTableSize);

  if (!device)
  {
    std::cerr << "Failed when reading fat table" << std::endl;
    return false;
  }

  return true;
}

const std::vector<FAT32DirectoryEntry> &Fat32Device::getDeletedEntries()
{
  return deletedEntries;
}

void Fat32Device::printDeletedEntriesConsole()
{
  if (deletedEntries.empty())
  {
    std::cout << "No deleted entry is found\n";
    return;
  }

  std::cout << "Deleted entries:\n";
  for (std::size_t i{0}; i < deletedEntries.size(); ++i)
  {
    std::string entryName{"?"};
    bool isDirectory{(deletedEntries[i].attributes & 0x10) != 0};

    for (std::size_t j{1}; j < 8; ++j)
    {
      if (deletedEntries[i].name[j] == ' ')
        break;
      if (std::isprint(deletedEntries[i].name[j]))
        entryName += static_cast<char>(deletedEntries[i].name[j]);
    }

    bool hasExtension{false};
    for (std::size_t j{8}; j < 11; ++j)
    {
      if (deletedEntries[i].name[j] != ' ')
      {
        if (!hasExtension)
        {
          entryName += '.';
          hasExtension = true;
        }
        if (std::isprint(deletedEntries[i].name[j]))
          entryName += static_cast<char>(deletedEntries[i].name[j]);
      }
    }

    if (!entryName.empty())
    {
      std::cout << i + 1 << ". " << entryName;
      if (isDirectory)
        std::cout << " (directory)\n";
      else
        std::cout << " (file)\n";
    }
  }
}

void Fat32Device::findDeletedEntries()
{
  deletedEntries.clear();

  uint32_t firstDataSector{static_cast<uint32_t>(bootSector->reservedSectorCount) + (static_cast<uint32_t>(bootSector->fatCount) * bootSector->sectorsPerFat)};
  uint32_t currentCluster{bootSector->rootDirStartCluster};
  uint32_t bytesPerCluster{static_cast<uint32_t>(bootSector->bytesPerSector) * static_cast<uint32_t>(bootSector->sectorsPerCluster)};
  uint8_t bytesPerEntry{32};
  std::vector<FAT32DirectoryEntry> entries(bytesPerCluster / static_cast<uint32_t>(bytesPerEntry));

  while (currentCluster >= 0x2 && currentCluster < 0x0FFFFFF8)
  {
    if (currentCluster == 0x0FFFFFF7)
    {
      currentCluster = fatTable[currentCluster];
      continue;
    }

    uint32_t currentSector{firstDataSector + (currentCluster - 2) * static_cast<uint32_t>(bootSector->sectorsPerCluster)};
    uint32_t byteOffset{currentSector * static_cast<uint32_t>(bootSector->bytesPerSector)};

    device.seekg(byteOffset);
    device.read(reinterpret_cast<char *>(entries.data()), bytesPerCluster);

    for (const auto &entry : entries)
    {
      if (entry.name[0] == 0xE5)
      {
        if ((entry.attributes & 0x0F) == 0x0F)
          continue;
        deletedEntries.push_back(entry);
      }
    }
    entries.clear();
    currentCluster = fatTable[currentCluster];
  }
}
