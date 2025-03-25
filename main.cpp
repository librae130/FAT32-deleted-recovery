#include "fat32.h"

int main()
{
  std::cout << "- Enter FAT32 device's path: ";
  std::string inputPath{};
  std::cin >> inputPath;
  Fat32Device device{"/dev/nvme1n1p4"};
  device.printBootSectorInfo();
  device.findDeletedEntries();
  device.printDeletedEntriesConsole();
  std::cout << "- Enter the entry number to recover: ";
  unsigned long int num{};
  std::cin >> num;
  if (num > device.getDeletedEntries().size() || num <= 0)
    return 0;
  else
  {
    std::cout << "+ Writing to output path on current partition can render some deleted files/folders unrecoverable.\n";
    std::cout << "- Enter output path: ";
    std::string outputPath{};
    std::cin >> outputPath;
    device.recoverDeletedFolder(device.getDeletedEntries()[num - 1], outputPath);
  }
  return 0;
}

