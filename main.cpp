#include <string>
#include <vector>
#include "fat32.h"


int main()
{
  Fat32Device device{"/dev/nvme1n1p4"};
  device.printBootSectorInfo();
  device.findDeletedEntries();
  device.printDeletedEntriesConsole();
  return 0;
}
