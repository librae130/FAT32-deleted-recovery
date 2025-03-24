#include <iostream>
#include "fat32.h"

int main()
{
  FAT32Reader reader{"/dev/nvme1n1p4"};
  reader.printBootSectorInfo();
  reader.listRootDirectoryDeletedEntries();
  return 0;
}
