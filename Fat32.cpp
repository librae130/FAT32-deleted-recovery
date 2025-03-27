#include "Fat32.h"

Fat32Device::Fat32Device(const std::string_view path)
try
    : devicePath{path}
{
  readDevice(devicePath);
}
catch (const std::runtime_error&)
{
  throw;
}

Fat32Device::~Fat32Device()
{
  if (device.is_open())
    device.close();
}

bool Fat32Device::isFat32()
{
  if (std::string_view(reinterpret_cast<char *>(bootSector->fatName)) != "FAT32   ")
    return false;

  return true;
}

void Fat32Device::readDevice(const std::string_view path)
{
  try
  {
    devicePath = path;
    device.open(devicePath, std::ios::binary | std::ios::in);
    if (!device.is_open())
      throw std::runtime_error{"Failed to open device"};
    readDeviceBootSector();

    if (!isFat32())
      throw std::runtime_error{"Path is not a FAT32 device"};

    readDeviceFatTable();
    readDeletedEntries();
  }
  catch (const std::runtime_error&)
  {
    throw;
  }
}

void Fat32Device::readDeviceBootSector()
{
  try
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
      throw std::runtime_error{"Error reading boot sector"};
  }
  catch (const std::runtime_error&)
  {
    throw;
  }
  catch (...)
  {
    throw std::runtime_error{"Error reading boot sector"};
  }
}

void Fat32Device::readDeviceFatTable()
{
  try
  {
    uint32_t fatTableSize{bootSector->sectorsPerFat * static_cast<uint32_t>(bootSector->bytesPerSector)};
    uint32_t fatOffset{static_cast<uint32_t>(bootSector->reservedSectorCount) * static_cast<uint32_t>(bootSector->bytesPerSector)};
    fatTable.resize(fatTableSize / sizeof(uint32_t));
    device.seekg(fatOffset);
    device.read(reinterpret_cast<char *>(fatTable.data()), fatTableSize);

    if (!device)
      throw std::runtime_error{"Error reading fat table"};
  }
  catch (const std::runtime_error&)
  {
    throw;
  }
}

std::vector<uint8_t> Fat32Device::readClusterData(const uint32_t cluster)
{
  try
  {
    uint32_t firstDataSector{bootSector->reservedSectorCount + (bootSector->fatCount * bootSector->sectorsPerFat)};
    uint32_t bytesPerCluster{static_cast<uint32_t>(bootSector->bytesPerSector) * static_cast<uint32_t>(bootSector->sectorsPerCluster)};
    uint32_t sectorNumber{firstDataSector + (cluster - 2) * bootSector->sectorsPerCluster};
    uint32_t byteOffset{sectorNumber * bootSector->bytesPerSector};

    std::vector<uint8_t> clusterData(bytesPerCluster);
    device.seekg(byteOffset);
    device.read(reinterpret_cast<char *>(clusterData.data()), bytesPerCluster);

    if (!device)
      throw std::runtime_error{"Error reading cluster data"};

    return clusterData;
  }
  catch (const std::runtime_error&)
  {
    throw;
  }
}
std::vector<FAT32Entry> Fat32Device::readClusterEntries(const uint32_t cluster)
{
  try
  {
    uint32_t firstDataSector{bootSector->reservedSectorCount + (bootSector->fatCount * bootSector->sectorsPerFat)};
    uint32_t bytesPerCluster{static_cast<uint32_t>(bootSector->bytesPerSector) * static_cast<uint32_t>(bootSector->sectorsPerCluster)};
    uint32_t sectorNumber{firstDataSector + (cluster - 2) * bootSector->sectorsPerCluster};
    uint32_t byteOffset{sectorNumber * bootSector->bytesPerSector};
    uint8_t bytesPerEntry{32};

    std::vector<FAT32Entry> clusterEntries(bytesPerCluster / bytesPerEntry);
    device.seekg(byteOffset);
    device.read(reinterpret_cast<char *>(clusterEntries.data()), bytesPerCluster);

    if (!device)
      throw std::runtime_error{"Error reading cluster entries"};

    return clusterEntries;
  }
  catch (const std::runtime_error&)
  {
    throw;
  }
}

const std::vector<FAT32Entry> &Fat32Device::readDeletedEntries()
{
  try
  {
    deletedEntries.clear();

    uint32_t firstDataSector{static_cast<uint32_t>(bootSector->reservedSectorCount) + (static_cast<uint32_t>(bootSector->fatCount) * bootSector->sectorsPerFat)};
    uint32_t currentCluster{bootSector->rootDirStartCluster};
    uint32_t bytesPerCluster{static_cast<uint32_t>(bootSector->bytesPerSector) * static_cast<uint32_t>(bootSector->sectorsPerCluster)};
    uint8_t bytesPerEntry{32};
    std::vector<FAT32Entry> entries(bytesPerCluster / static_cast<uint32_t>(bytesPerEntry));

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

      if (!device)
        throw std::runtime_error{"Error reading a deleted entry"};

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
    return deletedEntries;
  }
  catch (const std::runtime_error&)
  {
    throw;
  }
  catch (...)
  {
    throw std::runtime_error{"Error reading a deleted entry"};
  }
}
