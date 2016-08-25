
#include <iostream>

#include "log.h"

void PrintSystemLog(std::istream input)
{
    std::cout << DateTime::NowMs() << "    " << input.rdbuf() << std::endl;
}
