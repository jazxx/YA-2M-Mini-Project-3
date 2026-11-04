#include <iostream>
#include <string>
#include "HelpDesk.h"
#include "CommandLine.h"

int main()
{
    HelpDesk desk;
    std::string line;

    std::cout << "Help desk ready. Type 'help' for commands.\n";
    while (true)
    {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) // end of input (Ctrl+D, or end of a piped file)
            break;
        if (!runCommand(desk, line, std::cout))
            break;
    }
    std::cout << "bye\n";
    return 0;
}
