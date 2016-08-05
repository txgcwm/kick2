
#ifndef __NETWORK_HELPER_H__
#define __NETWORK_HELPER_H__

#include <map>
#include <string>

struct NicInfo
{
    std::string NicName;
    std::string Ip;
    std::string Mac;
};

class NetworkHelper
{
public:
    static int GetAllNicInfos(std::map<std::string, NicInfo> &nicInfos);
    static std::string GetIpAddrByNic(std::string nicName);
};


#endif
