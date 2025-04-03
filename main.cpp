#include "Fat32Recoverer.h"

int main()
{
  try
  {
    std::cout << "- Enter device: ";
    std::string device{};
    std::cin >> device;

    Fat32Recoverer recoverer{device};

    recoverer.printDeletedEntriesConsole();
    std::cout << "+ Some corrupted (partly-overwritten) files/folders may appear in the list.\n";
    std::cout << "- Enter index to recover: ";
    std::size_t index{};
    std::cin >> index;

    std::cout << "+ Writing to output path on current partition can render some deleted files/folders unrecoverable.\n";
    std::cout << "- Enter output directory: ";
    std::string outputPath{};
    std::cin >> outputPath;

    recoverer.recoverDeletedEntry(index - 1, outputPath);
  }
  catch (const std::runtime_error &error)
  {
    std::cerr << error.what() << std::endl;

    do
    {
      std::cout << "\nPress enter to exit...\n";
    } while (std::cin.get() != '\n');

    return 1;
  }
  catch (...)
  {
    std::cerr << "Unknown error ocurred" << std::endl;
    
    do
    {
      std::cout << "\nPress enter to exit...\n";
    } while (std::cin.get() != '\n');

    return 1;
  }

  std::cout << "- Succesfully recovered.\n";

  do
  {
    std::cout << "\nPress enter to exit...\n";
  } while (std::cin.get() != '\n');

  return 0;
}
