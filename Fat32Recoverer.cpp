#include "Fat32Recoverer.h"

Fat32Recoverer::Fat32Recoverer(const std::string_view path)
try
{
  device.readDevice(path);
}
catch (const std::runtime_error &)
{
  throw;
}

bool Fat32Recoverer::entryisDir(const FAT32Entry &entry)
{
  return (entry.attributes & 0x10) == 0x10;
}

bool Fat32Recoverer::entryisFile(const FAT32Entry &entry)
{
  return !entryisDir(entry) && ((entry.attributes & 0x08) != 0x08);
}

void Fat32Recoverer::readDevice(const std::string_view path)
{
  device.readDevice(path);
}

std::string Fat32Recoverer::convertEntryNameToString(const FAT32Entry &entry)
{
  if (entry.name[0] == 0x0)
    return "";

  std::string entryName{};

  if (entry.name[0] == 0xE5)
    entryName += '_';
  else
    entryName += static_cast<char>(entry.name[0]);

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
      if (entryisDir(device.getDeletedEntries()[i]))
        std::cout << " (directory)\n";
      else
        std::cout << " (file)\n";
    }
  }
}

void Fat32Recoverer::recoverDeletedEntry(const std::size_t index, const std::string_view outputDir)
{
  if (entryisDir(device.getDeletedEntries()[index - 1]))
    recoverDeletedDir(device.getDeletedEntries()[index - 1], outputDir);
  if (entryisFile(device.getDeletedEntries()[index - 1]))
    recoverDeletedFile(device.getDeletedEntries()[index - 1], outputDir);
}

void Fat32Recoverer::recoverDeletedFile(const FAT32Entry &entry, const std::string_view outputDir)
{
  try
  {
    if (!entryisFile(entry))
      throw std::runtime_error{"Entry is not a file"};

    if (device.getDeletedEntries().empty())
      throw std::runtime_error{"No deleted entry to recover file"};

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
      throw std::runtime_error{"Error creating file when recovering"};
    file.write(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(fileData.size()));
  }
  catch (const std::runtime_error &)
  {
    throw;
  }
  catch (...)
  {
    throw std::runtime_error{"Error recovering deleted file"};
  }
}

void Fat32Recoverer::recoverDeletedDir(const FAT32Entry &entry, const std::string_view outputDir)
{
  try
  {
    if (!entryisDir(entry))
      throw std::runtime_error{"Entry is not a directory"};

    if (device.getDeletedEntries().empty())
      throw std::runtime_error{"No deleted entry to recover directory"};

    std::string dirName{convertEntryNameToString(entry)};
    uint32_t currentCluster{(static_cast<uint32_t>(entry.firstClusterHigh) << 16) | entry.firstClusterLow};
    std::vector<FAT32Entry> dirEntries{};
    std::vector<uint32_t> dirClusters{};
    while (currentCluster >= 0x2 && currentCluster < 0x0FFFFFF8)
    {
      std::vector<FAT32Entry> clusterEntries{device.readClusterEntries(currentCluster)};
      for (const auto &dirEntry : clusterEntries)
      {
        if (dirEntry.name[0] == '.' && (dirEntry.name[1] == ' ' || (dirEntry.name[1] == '.' && dirEntry.name[2] == ' '))) // "." and ".." entry
          continue;

        if (dirEntry.name[0] == 0x0 || (!entryisFile(dirEntry) && !entryisDir(dirEntry)))
          continue;
          
        dirEntries.push_back(dirEntry);
      }

      currentCluster = device.getFatTable()[currentCluster];
    }

    std::filesystem::path currentPath{outputDir};
    std::filesystem::path newOutputDir{currentPath / dirName};
    if (!std::filesystem::create_directory(newOutputDir))
      throw std::runtime_error{"Failed to create directory when recovering"};

    for (const auto &dirEntry : dirEntries)
    {
      if (entryisDir(dirEntry))
        recoverDeletedDir(dirEntry, newOutputDir.string());
      else if (entryisFile(dirEntry))
        recoverDeletedFile(dirEntry, newOutputDir.string());
    }
  }
  catch (const std::runtime_error &)
  {
    throw;
  }
  catch (...)
  {
    throw std::runtime_error{"Error recovering deleted directory"};
  }
}
