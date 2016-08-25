
#ifndef __LOG_H__
#define __LOG_H__

#include "datetime.h"

#define SYSTEM_LOG(line)     std::cout << DateTime::NowMs() << "    " << line << std::endl;

#define DEBUG_LOG(line)     SYSTEM_LOG( "DEBUG    " << line );
#define INFO_LOG (line)     SYSTEM_LOG( "INFO     " << line );
#define ERROR_LOG(line)     SYSTEM_LOG( "ERROR    " << line );

//void PrintSystemLog(std::istringstream input);

#endif
