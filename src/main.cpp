#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <thread>
#include <fstream>
#include <iostream>

#include "HCNetSDK.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "ws.h"
#include "connection.h"
#include "env.h"

void CALLBACK MessageCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser);

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("access controller coming...");

    auto acSrc = std::string(getAccessAddr());
    auto acUsername = std::string(getAccessUsername());
    auto acPassword = std::string(getAccessPassword());
    connection hik_c(acSrc.c_str(), acUsername.c_str(), acPassword.c_str());

    spdlog::debug("hik sdk {} connected", hik_c.isLogin() ? "is" : "NOT");
    if(hik_c.isLogin()) {
//        hik_c.doCapture("./here.jpg");
//        auto cards = hik_c.doGetCards();
//        for (auto it = cards.begin(); it < cards.end(); it++) {
//            std::cout << it->String() << std::endl;
//        }
//        hik_c.doGetFaces(std::string("1001"));
//        c.doSetFace("1001", "./faces/srt1.jpeg");
//        c.doRemoveFaces(std::string("1001"));
//        hik_c.setAlarm(MessageCallback);

//        time_t timep;
//        time(&timep); /*获得time_t结构的时间，UTC时间*/
//        auto p = gmtime(&timep);
//        spdlog::info("year: {}, mon: {}, mday: {}, hour: {}, min: {}, sec: {}",
//                    p->tm_year, p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
//
//
//            auto newcard = new card;
//            newcard->setCardType(1);
//            newcard->setCardNo(std::string("0736558942"));
//            newcard->setName("abc");
//            newcard->setEmployeeId(90001);
//            auto newe = *p;
//            newe.tm_year++;
//            newcard->setBeginTime(*p);
//            newcard->setEndTime(newe);
//            if (!hik_c.doSetCard(*newcard)) {
//                spdlog::info("do set card err: ", NET_DVR_GetLastError);
//            }
    }

    auto wsconn = wsConn::get_instance();
    auto wsServerAddr = std::string(getWebsocketServer());
    wsconn->connect(wsServerAddr);

    std::this_thread::sleep_for(std::chrono::hours(1));

    return 0;
}

void CALLBACK MessageCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser)
{
    spdlog::debug("command 0x{0:x}", lCommand);
    //以下代码仅供参考，实际应用中不建议在该回调函数中直接处理数据保存文件
    //例如可以使用消息的方式(PostMessage)在消息响应函数里进行处理


    switch(lCommand)
    {
        case COMM_ALARM_ACS:
            NET_DVR_ACS_ALARM_INFO struAlarmInfo;
            memcpy(&struAlarmInfo, pAlarmInfo, sizeof(NET_DVR_ACS_ALARM_INFO));
            spdlog::debug("cardNo {} major {:x} minor {:x}",  struAlarmInfo.struAcsEventInfo.byCardNo, struAlarmInfo.dwMajor, struAlarmInfo.dwMinor);
                if(struAlarmInfo.dwMajor == MAJOR_EVENT &&
                (struAlarmInfo.dwMinor == MINOR_LEGAL_CARD_PASS
                || struAlarmInfo.dwMinor == MINOR_INVALID_CARD
                || struAlarmInfo.dwMinor == MINOR_FACE_VERIFY_PASS)) {
                    auto wsconn = wsConn::get_instance();

                    json j;
                    j["cardNo"] = (char*)struAlarmInfo.struAcsEventInfo.byCardNo;
                    j["deviceNo"] = "sid";

                    auto capSrc = std::string(getCaptureAddr());
                    auto capUsername = std::string(getCaptureUsername());
                    auto capPassword = std::string(getCapturePassword());
                    connection cap(capSrc.c_str(), capUsername.c_str(), capPassword.c_str());

                    char pic[102400];
                    char dst[204800];

                    int picLength = cap.doCapture(pic, 102400);
                    spdlog::debug("raw picture length: {}", picLength);
                    mg_base64_encode(reinterpret_cast<const unsigned char *>(pic), picLength, dst);

                    wsconn->send(dst);

                }
            break;
        default:
            break;
    }
}