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
    std::cout << "- Enter index to recover: ";
    std::size_t idx{};
    std::cin >> idx;

    std::cout << "+ Writing to output path on current partition can render some deleted files/folders unrecoverable.\n";
    std::cout << "- Enter output directory: ";
    std::string outputPath{};
    std::cin >> outputPath;

    recoverer.recoverDeletedEntry(idx, outputPath);

    std::cout << "- Succesfully recovered";

    return 0;
  }
  catch (const std::runtime_error& error)
  {
    std::cerr << error.what() << std::endl;
    return 1;
  }
  catch (...)
  {
    std::cerr << "Unknown error ocurred" << std::endl;
    return 1;
  }
}
