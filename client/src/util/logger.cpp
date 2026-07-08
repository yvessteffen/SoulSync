#include "util/logger.h"

#include <fstream>
#include <mutex>
#include <iostream>

static std::mutex logMutex;
static std::ofstream logFile("log.txt");

void Log(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(logMutex);

    logFile << msg << std::endl;
    logFile.flush();

    std::cout << msg << std::endl;
}