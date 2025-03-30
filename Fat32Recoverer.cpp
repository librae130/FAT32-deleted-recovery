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
  return entry.attributes == 0x0F;
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
    std::vector<FAT32Entry> cachedDeletedEntries{}; // For caching a vector of long file name entries, and main file/directory entry at the back.
    for (const auto &entry : device.getRootEntries())
    {
      // If the entry is a long file name,
      // just cache it until encountering a file/directory because
      // long file name entries are always at the front of the
      // entry they support.
      if (entryisLongFileName(entry))
      {
        cachedDeletedEntries.push_back(entry);
        continue;
      }
      // If the entry is a file/directory,
      // we found the long file name entries and their main entry,
      // so we push it to deletedEntries member and reset the cache.
      else if (entryisDir(entry) || entryisFile(entry))
      {
        if (entryisDeleted(entry))
        {
          cachedDeletedEntries.push_back(entry);
          deletedEntries.push_back(cachedDeletedEntries);
        }
        cachedDeletedEntries.clear();
      }
    }
  }
  catch (...)
  {
    throw std ::runtime_error{"Error reading deleted entries"};
  }
}

std::string Fat32Recoverer::getEntryNameAscii(const std::vector<FAT32Entry> &entry)
{
  try
  {
    if (entry.empty())
      return "";

    if (!entryisDir(entry.back()) && !entryisFile(entry.back()))
      throw std::runtime_error{"Not a valid chain of entries."};

    // Multiple entries exist mean there are long file name entries followed by the main entry they support.
    if (entry.size() > 1)
    {
      std::string longFileName{};

      for (long long i{static_cast<long long>(entry.size()) - 2}; i >= 0; --i)
      {
        if (!entryisLongFileName(entry[static_cast<std::size_t>(i)]))
          throw std::runtime_error{"Not a valid chain of entries."};

          // Read first 5 UTF-16 characters.
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

        // Read next 6 UTF-16 characters.
        for (std::size_t j{0}; j < 12; j += 2)
        {
          char16_t character{static_cast<char16_t>(*((uint8_t *)(&entry[static_cast<std::size_t>(i)]) + 14 + j) | (*((uint8_t *)(&entry[static_cast<std::size_t>(i)]) + 14 + j + 1) << 8))};
          if (character > 0 && character <= 127)
            longFileName += static_cast<char>(character);
          else if (character != 0xFFFF && character != 0)
            longFileName += '?';
        }

        // Read final 2 UTF-16 characters.
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

    // Fallback to using short file name if there is no long file name entry.
    std::string shortName{};

    // Check for starting deleted marker
    if (entry.back().name[0] == 0xE5)
      shortName += '_';
    else
      shortName += static_cast<char>(entry.back().name[0]);

    // Read first 8 ASCII characters for name.
    for (std::size_t j{1}; j < 8; ++j)
    {
      if (entry.back().name[j] == ' ')
        break;
      if (std::isprint(entry.back().name[j]))
        shortName += static_cast<char>(entry.back().name[j]);
    }

    // Read final 3 ASCII characters for extension.
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
  catch (const std::runtime_error &)
  {
    throw;
  }
}

void Fat32Recoverer::printDeletedEntriesConsole()
{
  try
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
      std::string entryName{getEntryNameAscii(entries)};

      std::cout << count << ". " << entryName;
      if (entryisDir(entries.back()))
        std::cout << " (directory)\n";
      else if (entryisFile(entries.back()))
        std::cout << " (file)\n";
      else
        throw std::runtime_error{"Invalid type of entry when printing deleted entries"};
      ++count;
    }
  }
  catch (const std::runtime_error &)
  {
    throw;
  }
}

void Fat32Recoverer::recoverDeletedEntry(const std::size_t index, const std::string_view outputDir)
{
  try
  {
    if (index >= deletedEntries.size())
      throw std::runtime_error{"Out of bound when acessing deleted entries"};

    if (entryisDir(deletedEntries[index].back()))
      recoverDeletedDir(deletedEntries[index], outputDir);
    else
      recoverDeletedFile(deletedEntries[index], outputDir);
  }
  catch (const std::runtime_error &)
  {
    throw;
  }
}

void Fat32Recoverer::writeFileToPath(const std::vector<uint8_t> &fileData, const std::string outputDir)
{
  try
  {
    std::ofstream file(outputDir, std::ios::binary | std::ios::out | std::ios::trunc);

    if (!file)
      throw std::runtime_error{"Failed to open output file for writing"};

    std::size_t chunkSize{4096}; // Write in chunks of 4096 bytes to prevent error from big files.
    std::size_t bytesLeft{fileData.size()};
    std::size_t position{0};

    while (bytesLeft > 0)
    {
      std::size_t bytesToWrite{std::min(chunkSize, bytesLeft)}; // Accounts for when bytesLeft < chunkSize to prevent writting out-of-bound.

      if (!file.write(reinterpret_cast<const char *>(&fileData[position]), static_cast<std::streamsize>(bytesToWrite)))
        throw std::runtime_error{"Error writing data to file"};

      position += bytesToWrite;
      bytesLeft -= bytesToWrite;
    }

    file.flush();

    if (!file)
      throw std::runtime_error{"Error occurred during file write operation"};

    file.close();

    if (!std::filesystem::exists(outputDir) || std::filesystem::file_size(outputDir) != fileData.size())
      throw std::runtime_error{"File verification failed after writing"};
  }
  catch (const std::exception &)
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

    std::vector<uint8_t> fileData{};
    std::string fileName{getEntryNameAscii(entry)};

    uint32_t currentCluster{(static_cast<uint32_t>(entry.back().firstClusterHigh) << 16) | entry.back().firstClusterLow}; // Initilized with the file's starting cluster.
    uint32_t bytesPerCluster{static_cast<uint32_t>(device.getBootSector()->bytesPerSector) * static_cast<uint32_t>(device.getBootSector()->sectorsPerCluster)};
    uint32_t remainingSize{entry.back().size};

    // This loop reads data (contents) of the file following its cluster chain in FAT table,
    // then append the data to fileData vector for writing later.
    // 0x0FFFFFF8 to 0x0FFFFFFF marks the end of the cluster chain, whereas cluster's numbering starts at #0x2.
    while (remainingSize > 0 && currentCluster >= 0x2 && currentCluster < 0x0FFFFFF8)
    {
      std::vector<uint8_t> clusterData{device.readClusterData(currentCluster)};
      uint32_t bytesToInsert{std::min(remainingSize, bytesPerCluster)};

      if (clusterData.size() != bytesPerCluster)
        throw std::runtime_error{"Incomplete cluster read"};

      fileData.insert(fileData.end(), clusterData.begin(), clusterData.begin() + bytesToInsert);
      remainingSize -= bytesToInsert;
      currentCluster = device.getFatTable()[currentCluster];
    }

    // Create the file path by appending its name to the output directory.
    std::filesystem::path currentPath{outputDir};
    std::filesystem::path newOutputPath{currentPath / fileName};

    writeFileToPath(fileData, newOutputPath.string());
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

    std::string dirName{getEntryNameAscii(entry)};
    uint32_t currentCluster{(static_cast<uint32_t>(entry.back().firstClusterHigh) << 16) | entry.back().firstClusterLow}; // Initilized with the folder's starting cluster.
    std::vector<FAT32Entry> dirEntries{};

    // This loop reads all entries of the folder following its cluster chain in FAT table,
    // then append the data to fileData vector for writing later.
    // 0x0FFFFFF8 to 0x0FFFFFFF marks the end of the cluster chain, whereas cluster's numbering starts at #0x2.
    while (currentCluster >= 0x2 && currentCluster < 0x0FFFFFF8)
    {
      std::vector<FAT32Entry> clusterEntries{device.readClusterEntries(currentCluster)};
      for (const auto &dirEntry : clusterEntries)
      {
        // Skip parent folder and the folder itself.
        if (dirEntry.name[0] == '.' && (dirEntry.name[1] == ' ' || (dirEntry.name[1] == '.' && dirEntry.name[2] == ' '))) // "." and ".." entry
          continue;

        // Skip empty entries.
        if (dirEntry.name[0] == 0x0)
          continue;

        // Failsafe check for skipping anything that is not a file/directory/long file name entry.
        if (!entryisFile(dirEntry) && !entryisDir(dirEntry) && !entryisLongFileName(dirEntry))
          continue;

        dirEntries.push_back(dirEntry);
      }

      currentCluster = device.getFatTable()[currentCluster];
    }

    // Create the folder itself by appending to output path.
    std::filesystem::path currentPath{outputDir};
    std::filesystem::path newOutputDir{currentPath / dirName};
    if (!std::filesystem::create_directory(newOutputDir))
      throw std::runtime_error{"Failed to create directory when recovering"};

    std::vector<FAT32Entry> cachedDirEntries{}; // For caching a vector of long file name entries, and main file/directory entry at the back.
    for (const auto &entry : device.getRootEntries())
    {
      // If the entry is a long file name,
      // just cache it until encountering a file/directory because
      // long file name entries are always at the front of the
      // entry they support.
      if (entryisLongFileName(entry))
      {
        cachedDirEntries.push_back(entry);
        continue;
      }
      // If the entry is a file/directory,
      // we found the long file name entries and their main entry,
      // so we call the apropriate method for recovering them.
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
