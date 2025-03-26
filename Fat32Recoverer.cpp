#include "Fat32Recoverer.h"

Fat32Recoverer::Fat32Recoverer(const std::string_view path)
{
  device.readDevice(path);
}

bool Fat32Recoverer::rootEntryisDir(const FAT32Entry &entry)
{
  return (entry.attributes & 0x10) != 0;
}

bool Fat32Recoverer::rootEntryisFile(const FAT32Entry &entry)
{
  return !rootEntryisDir(entry) && ((entry.attributes & 0x08) == 0);
}

bool Fat32Recoverer::readDevice(const std::string_view path)
{
  device.readDevice(path);
}

std::string Fat32Recoverer::convertEntryNameToString(const FAT32Entry &entry)
{
  std::string entryName{"_"};
  for (std::size_t j{1}; j < 8; ++j)
  {
    if (entry.name[j] == ' ')
      break;
    if (std::isprint(entry.name[j]))
      entryName += static_cast<char>(entry.name[j]);
  }

  bool hasExtension{false};
  for (std::size_t j{8}; j < 11; ++j)
  {
    if (entry.name[j] != ' ')
    {
      if (!hasExtension)
      {
        entryName += '.';
        hasExtension = true;
      }
      if (std::isprint(entry.name[j]))
        entryName += static_cast<char>(entry.name[j]);
    }
  }
  return entryName;
}

void Fat32Recoverer::printDeletedEntriesConsole()
{
  if (device.getDeletedEntries().empty())
  {
    std::cout << "No deleted entry is found\n";
    return;
  }

  std::cout << "Deleted entries:\n";
  for (std::size_t i{0}; i < device.getDeletedEntries().size(); ++i)
  {
    std::string entryName{convertEntryNameToString(device.getDeletedEntries()[i])};
    if (!entryName.empty())
    {
      std::cout << i + 1 << ". " << entryName;
      if (rootEntryisDir(device.getDeletedEntries()[i]))
        std::cout << " (directory)\n";
      else
        std::cout << " (file)\n";
    }
  }
}

void Fat32Recoverer::recoverDeletedFile(const FAT32Entry &entry, const std::string_view outputDir)
{
  if (rootEntryisDir(entry))
    return;

  if (device.getDeletedEntries().empty())
  {
    std::cerr << "No deleted entries to recover" << std::endl;
    return;
  }

  std::vector<uint8_t> fileData;
  std::string fileName{convertEntryNameToString(entry)};

  uint32_t currentCluster{(static_cast<uint32_t>(entry.firstClusterHigh) << 16) | entry.firstClusterLow};
  uint32_t bytesPerCluster{static_cast<uint32_t>(device.getBootSector()->bytesPerSector) * static_cast<uint32_t>(device.getBootSector()->sectorsPerCluster)};
  uint32_t remainingSize{entry.size};

  while (remainingSize > 0 && currentCluster >= 0x2 && currentCluster < 0x0FFFFFF8)
  {
    std::vector<uint8_t> clusterData{device.readClusterData(currentCluster)};
    uint32_t bytesToInsert{std::min(remainingSize, bytesPerCluster)};

    fileData.insert(fileData.end(), clusterData.begin(), clusterData.begin() + bytesToInsert);
    remainingSize -= bytesToInsert;
    currentCluster = device.getFatTable()[currentCluster];
  }

  std::filesystem::path currentPath{outputDir};
  std::filesystem::path newOutputPath{currentPath / fileName};
  std::fstream file(newOutputPath.string(), std::ios::binary | std::ios::out);
  if (!file)
  {
    return;
  }
  file.write(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(fileData.size()));
}

void Fat32Recoverer::recoverDeletedFolder(const FAT32Entry &entry, const std::string_view outputDir)
{
  if (!rootEntryisDir(entry))
    return;

  if (device.getDeletedEntries().empty())
  {
    std::cerr << "No deleted entries to recover" << std::endl;
    return;
  }

  std::string dirName{convertEntryNameToString(entry)};
  uint32_t currentCluster{(static_cast<uint32_t>(entry.firstClusterHigh) << 16) | entry.firstClusterLow};
  uint32_t bytesPerCluster{static_cast<uint32_t>(device.getBootSector()->bytesPerSector) * static_cast<uint32_t>(device.getBootSector()->sectorsPerCluster)};
  uint8_t bytesPerEntry{32};
  std::vector<FAT32Entry> dirEntries{};
  while (currentCluster >= 0x2 && currentCluster < 0x0FFFFFF8)
  {
    if (currentCluster == 0x0FFFFFF7)
    {
      currentCluster = device.getFatTable()[currentCluster];
      continue;
    }

    std::vector<FAT32Entry> clusterEntries{device.readClusterEntries(currentCluster)};
    for (const auto &dirEntry : clusterEntries)
    {
      if (dirEntry.name[0] == 0xE5)
      {
        if ((dirEntry.attributes & 0x0F) != 0x0F)
          dirEntries.push_back(dirEntry);
      }
    }

    currentCluster = device.getFatTable()[currentCluster];
  }

  std::filesystem::path currentPath{outputDir};
  std::filesystem::path newOutputDir{currentPath / dirName};
  if (std::filesystem::create_directory(newOutputDir))
  {
    std::cout << "Directory created successfully.\n";
  }
  else
  {
    std::cerr << "Directory already exists or failed to create.\n";
  }

  for (const auto &dirEntry : dirEntries)
  {
    if (rootEntryisDir(dirEntry))
      recoverDeletedFolder(dirEntry, newOutputDir.string());
    else if (rootEntryisFile(dirEntry))
      recoverDeletedFile(dirEntry, newOutputDir.string());
  }
}
