//
// Created by srt on 2020/6/23.
//

#ifndef AC_CONNECTION_H
#define AC_CONNECTION_H

#include <vector>
#include <string>

#include <card.h>
#include <face.h>

#include <spdlog/spdlog.h>

class connection {
public:
    //deprecated
    static bool threaded;

    static void initLibrary(){
        char cryptoPath[2048] = {0};
        sprintf(cryptoPath, "./libcrypto.so");
        bool cfgInit = NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_LIBEAY_PATH, cryptoPath);
        spdlog::debug("init lib libcrypto.so {}", cfgInit ? "success" : "fail");

        char sslPath[2048] = {0};
        sprintf(sslPath, "./libssl.so");
        cfgInit = NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SSLEAY_PATH, sslPath);
        spdlog::debug("init lib libssl.so {}", cfgInit ? "success" : "fail");

        NET_DVR_LOCAL_SDK_PATH struComPath = {0};
        sprintf(struComPath.sPath, "./"); //HCNetSDKCom文件夹所在的路径
        cfgInit = NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SDK_PATH, &struComPath);
        spdlog::debug("init lib HCNetSDKCom {}", cfgInit ? "success" : "fail");

    };

    connection(const char* srcIp,const char* username,const char* password);
    bool isLogin() const;

    static DWORD getLastError();

    bool doCapture(const char* saveDst) const;
    int doCapture(char *dst, int length) const;

    std::vector<card> doGetCards() const;
    bool doSetCard(const card &newcard);
    bool doRemoveCard(std::string cardNo);

    bool doSetFace(const std::string &cardNo,const std::string &facePath);
    bool doGetFaces(const std::string &cardNo) const;
    bool doRemoveFaces(const std::string &cardNo);

    bool setAlarm(MSGCallBack callBack);
    bool unsetAlarm();
    bool getAlarmEvents();

    virtual ~connection();

private:
    std::string srcIp, userName, password;
    long lUserID;
    bool isLogined;

    //deviceInfo
    long channelStart;
    long channelNums;

    std::string serialNumber;

    long alarmHandler;

};


#endif //AC_CONNECTION_H
