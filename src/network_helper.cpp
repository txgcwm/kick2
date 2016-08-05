
#include <cstdio>
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "network_helper.h"

int NetworkHelper::GetAllNicInfos(std::map<std::string, NicInfo> &nicInfos)
{
    int result = 0;

    do
    {
        struct ifaddrs *ifaddr;
        if (getifaddrs(&ifaddr) == -1)
        {
            perror("getifaddrs");
            result = -1;
            break;
        }

        for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
                continue;

            char host[NI_MAXHOST] = { 0 };
            int family = ifa->ifa_addr->sa_family;
            if (family == AF_INET)// || family == AF_INET6)
            {
                int s = getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if (s != 0)
                {
                    printf("getnameinfo() failed: %s\n", gai_strerror(s));
                    result = -1;
                    break;
                }
                std::string nicName = ifa->ifa_name;
                std::map<std::string, NicInfo>::iterator it;
                if ((it = nicInfos.find(nicName)) != nicInfos.end())
                {
                    it->second.Ip = host;
                }
                else
                {
                    NicInfo networkInfo;
                    networkInfo.NicName = nicName;
                    networkInfo.Ip = host;
                    nicInfos.insert(std::pair<std::string, NicInfo>(nicName, networkInfo));
                }
            }
            else if (family == AF_PACKET)
            {
                struct sockaddr_ll *s = (struct sockaddr_ll*)ifa->ifa_addr;
                char mac[INET6_ADDRSTRLEN];
                int i;
                int len = 0;
                for (i = 0; i < 6; i++)
                    len += sprintf(mac + len, "%02X%s", s->sll_addr[i], i < 5 ? ":" : "");

                std::string nicName = ifa->ifa_name;
                std::map<std::string, NicInfo>::iterator it;
                if ((it = nicInfos.find(nicName)) != nicInfos.end())
                {
                    it->second.Mac = mac;
                }
                else
                {
                    NicInfo networkInfo;
                    networkInfo.NicName = nicName;
                    networkInfo.Mac = mac;
                    nicInfos.insert(std::pair<std::string, NicInfo>(nicName, networkInfo));
                }
            }
        }
        freeifaddrs(ifaddr);
    } while (0);

    return result;
}

std::string NetworkHelper::GetIpAddrByNic(std::string nicName)
{
    if (nicName.empty() || nicName == "any")
        return "0.0.0.0";
    std::map<std::string, NicInfo> nicInfos;
    GetAllNicInfos(nicInfos);
    std::string local;
    if (nicInfos.find(nicName) != nicInfos.end())
        local = nicInfos[nicName].Ip;
    else
        local = "0.0.0.0";
    return local;
}
