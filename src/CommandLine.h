#pragma once
#include <iostream>
#include <string>
#include "HelpDesk.h"

// Runs one command line against the desk and writes the result to out.
// Returns false when the user asks to quit.
bool runCommand(HelpDesk &desk, const std::string &line, std::ostream &out);

void printHelp(std::ostream &out);
