#include "Fat32Recoverer.h"

Fat32Recoverer::Fat32Recoverer(const std::string_view path)
try
{
  device.readDevice(path);
  readDeletedEntries();
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

bool Fat32Recoverer::entryisLongFileName(const FAT32Entry &entry)
{
  return (entry.attributes & 0x0F) == 0x0F;
}

bool Fat32Recoverer::entryisDeleted(const FAT32Entry &entry)
{
  return entry.name[0] == 0xE5;
}

void Fat32Recoverer::readDevice(const std::string_view path)
{
  try
  {
    device.readDevice(path);
    readDeletedEntries();
  }
  catch (const std::runtime_error &)
  {
    throw;
  }
}

void Fat32Recoverer ::readDeletedEntries()
{
  try
  {
    std::vector<FAT32Entry> cachedDeletedEntries{};
    for (const auto &entry : device.getEntries())
    {
      if (entryisLongFileName(entry))
      {
        cachedDeletedEntries.push_back(entry);
        continue;
      }

      if (entryisDir(entry) || entryisFile(entry))
      {
        if (entryisDeleted(entry))
        {
          cachedDeletedEntries.push_back(entry);
          deletedEntries.push_back(cachedDeletedEntries);
        }
        else
          cachedDeletedEntries.clear();
      }
    }
  }
  catch (...)
  {
    throw std ::runtime_error{"Error reading deleted entries"};
  }
}

std::string Fat32Recoverer::getEntryName(const std::vector<FAT32Entry> &entry)
{
  if (entry.empty())
    return "";

  if (entry.size() > 1)
  {
    std::string longFileName{};

    for (long long i{static_cast<long long>(entry.size()) - 2}; i >= 0; --i)
    {
      if ((entry[static_cast<std::size_t>(i)].attributes & 0x0F) != 0x0F)
        continue;

      for (std::size_t j{1}; j < 11; j += 2)
      {
        if (entry[static_cast<std::size_t>(i)].name[j] == 0 && entry[static_cast<std::size_t>(i)].name[j + 1] == 0)
          break;
        char16_t character{static_cast<char16_t>(entry[static_cast<std::size_t>(i)].name[j] | (entry[static_cast<std::size_t>(i)].name[j + 1] << 8))};
        if (character > 0 && character <= 127)
          longFileName += static_cast<char>(character);
        else if (character != 0xFFFF && character != 0)
          longFileName += '?';
      }

      for (std::size_t j{0}; j < 12; j += 2)
      {
        char16_t character{static_cast<char16_t>(*((uint8_t *)(&entry[static_cast<std::size_t>(i)]) + 14 + j) | (*((uint8_t *)(&entry[static_cast<std::size_t>(i)]) + 14 + j + 1) << 8))};
        if (character > 0 && character <= 127)
          longFileName += static_cast<char>(character);
        else if (character != 0xFFFF && character != 0)
          longFileName += '?';
      }

      for (std::size_t j{0}; j < 4; j += 2)
      {
        char16_t character{static_cast<char16_t>(*((uint8_t *)(&entry[static_cast<std::size_t>(i)]) + 28 + j) | (*((uint8_t *)(&entry[static_cast<std::size_t>(i)]) + 28 + j + 1) << 8))};
        if (character > 0 && character <= 127)
          longFileName += static_cast<char>(character);
        else if (character != 0xFFFF && character != 0)
          longFileName += '?';
      }
    }
    return longFileName;
  }

  std::string shortName{};

  if (entry.back().name[0] == 0xE5)
    shortName += '_';
  else
    shortName += static_cast<char>(entry.back().name[0]);

  for (std::size_t j{1}; j < 8; ++j)
  {
    if (entry.back().name[j] == ' ')
      break;
    if (std::isprint(entry.back().name[j]))
      shortName += static_cast<char>(entry.back().name[j]);
  }

  bool hasExtension{false};
  for (std::size_t j{8}; j < 11; ++j)
  {
    if (entry.back().name[j] != ' ')
    {
      if (!hasExtension)
      {
        shortName += '.';
        hasExtension = true;
      }
      if (std::isprint(entry.back().name[j]))
        shortName += static_cast<char>(entry.back().name[j]);
    }
  }
  return shortName;
}

void Fat32Recoverer::printDeletedEntriesConsole()
{
  if (deletedEntries.empty())
  {
    std::cout << "No deleted entry is found\n";
    return;
  }

  std::cout << "Deleted entries:\n";
  std::size_t count{1};
  for (const auto &entries : deletedEntries)
  {
    std::string entryName{getEntryName(entries)};

    std::cout << count << ". " << entryName;
    if (entryisDir(entries.back()))
      std::cout << " (directory)\n";
    else
      std::cout << " (file)\n";
    ++count;
  }
}

void Fat32Recoverer::recoverDeletedEntry(const std::size_t index, const std::string_view outputDir)
{
  try
  {
    if (index <= 0 || index > deletedEntries.size())
      throw std::runtime_error{"Out of bound when acessing deleted entries"};

    if (entryisDir(deletedEntries[index - 1].back()))
      recoverDeletedDir(deletedEntries[index], outputDir);
    else
      recoverDeletedFile(deletedEntries[index], outputDir);
  }
  catch (const std::runtime_error &)
  {
    throw;
  }
}

void Fat32Recoverer::recoverDeletedFile(const std::vector<FAT32Entry> &entry, const std::string_view outputDir)
{
  try
  {
    if (!entryisFile(entry.back()))
      throw std::runtime_error{"Entry is not a file"};

    if (deletedEntries.empty())
      throw std::runtime_error{"No deleted entry to recover file"};

    std::vector<uint8_t> fileData;
    std::string fileName{getEntryName(entry)};

    uint32_t currentCluster{(static_cast<uint32_t>(entry.back().firstClusterHigh) << 16) | entry.back().firstClusterLow};
    uint32_t bytesPerCluster{static_cast<uint32_t>(device.getBootSector()->bytesPerSector) * static_cast<uint32_t>(device.getBootSector()->sectorsPerCluster)};
    uint32_t remainingSize{entry.back().size};

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

void Fat32Recoverer::recoverDeletedDir(const std::vector<FAT32Entry> &entry, const std::string_view outputDir)
{
  try
  {
    if (!entryisDir(entry.back()))
      throw std::runtime_error{"Entry is not a directory"};

    if (deletedEntries.empty())
      throw std::runtime_error{"No deleted entry to recover directory"};

    std::string dirName{getEntryName(entry)};
    uint32_t currentCluster{(static_cast<uint32_t>(entry.back().firstClusterHigh) << 16) | entry.back().firstClusterLow};
    std::vector<FAT32Entry> dirEntries{};

    while (currentCluster >= 0x2 && currentCluster < 0x0FFFFFF8)
    {
      std::vector<FAT32Entry> clusterEntries{device.readClusterEntries(currentCluster)};
      for (const auto &dirEntry : clusterEntries)
      {
        if (dirEntry.name[0] == '.' && (dirEntry.name[1] == ' ' || (dirEntry.name[1] == '.' && dirEntry.name[2] == ' '))) // "." and ".." entry
          continue;

        if (dirEntry.name[0] == 0x0)
          continue;

        if (!entryisFile(dirEntry) || !entryisDir(dirEntry) || !entryisLongFileName(dirEntry))
          continue;

        dirEntries.push_back(dirEntry);
      }

      currentCluster = device.getFatTable()[currentCluster];
    }

    std::filesystem::path currentPath{outputDir};
    std::filesystem::path newOutputDir{currentPath / dirName};
    if (!std::filesystem::create_directory(newOutputDir))
      throw std::runtime_error{"Failed to create directory when recovering"};

    std::vector<FAT32Entry> cachedDirEntries{};
    for (const auto &entry : device.getEntries())
    {
      if (entryisLongFileName(entry))
      {
        cachedDirEntries.push_back(entry);
        continue;
      }
      else if (entryisDir(entry))
      {
        cachedDirEntries.push_back(entry);
        recoverDeletedDir(cachedDirEntries, newOutputDir.string());
        cachedDirEntries.clear();
      }
      else
      {
        cachedDirEntries.push_back(entry);
        recoverDeletedFile(cachedDirEntries, newOutputDir.string());
        cachedDirEntries.clear();
      }
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
