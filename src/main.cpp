#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <thread>
#include <fstream>
#include <iostream>

#include "HCNetSDK.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <util.h>
#include "ws.h"
#include "connection.h"
#include "env.h"

void CALLBACK MessageCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser);
void server(struct mg_connection *nc, int ev, void *ev_data);

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("access controller coming...");

    auto acSrc = std::string(getAccessAddr());
    auto acUsername = std::string(getAccessUsername());
    auto acPassword = std::string(getAccessPassword());
    connection hik_ac(acSrc.c_str(), acUsername.c_str(), acPassword.c_str());

    spdlog::debug("hik sdk {} connected", hik_ac.isLogin() ? "is" : "NOT");
    if(hik_ac.isLogin()) {
//        hik_ac.doCapture("./here.jpg");
//        auto cards = hik_ac.doGetCards();
//        for (auto it = cards.begin(); it < cards.end(); it++) {
//            std::cout << it->String() << std::endl;
//        }
//        hik_ac.doGetFaces(std::string("1001"));
//        c.doSetFace("1001", "./faces/srt1.jpeg");
//        c.doRemoveFaces(std::string("1001"));
        hik_ac.setAlarm(MessageCallback);

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
//            if (!hik_ac.doSetCard(*newcard)) {
//                spdlog::info("do set card err: ", NET_DVR_GetLastError);
//            }
    }

    auto wsclient = wsConn::getInstance(AS_BACKEND_WS_CLIENT);
    wsclient->connect("ws://192.168.99.144:8000/webSocket/100243536");

    auto wssconn = wsConn::getInstance(AS_SCREEN_WS_SERVER);
    wssconn->bindAndServe("8085", server);

//    std::ifstream fin("/home/srt/froggggg.jpeg", std::ios::binary);
//    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(fin), {});
//
//    char base64[409600]={0};
//    mg_base64_encode(buffer.data(), buffer.size(), base64);
//    spdlog::debug("pic: {}", base64);

    for (auto i = 0;i < 0;i++) {
        std::this_thread::sleep_for(std::chrono::seconds(10));

        spdlog::debug("someone coming");

        json data;
        data["cardNo"] = "6012345643";
        data["deviceNo"] = "100243536";
        json msg;
        msg["topic"] = "userIdentity";
        msg["data"] = data;

        wsclient->send(msg.dump());
        std::this_thread::sleep_for(std::chrono::seconds (10));

//        auto capSrc = std::string(getCaptureAddr());
//        auto capUsername = std::string(getCaptureUsername());
//        auto capPassword = std::string(getCapturePassword());
//        connection hik_cap(capSrc.c_str(), capUsername.c_str(), capPassword.c_str());
//        hik_cap.doCapture()

        std::ifstream fin("/home/srt/froggggg.jpeg", std::ios::binary);
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(fin), {});
        char base64[409600]={0};
        mg_base64_encode(buffer.data(), buffer.size(), base64);
        spdlog::debug("pic: {}", base64);

        json garbageData;
        garbageData["cardNo"] = "6012345643";
        garbageData["deviceNo"] = "100243536";
        garbageData["garbageType"] = WET_GARBAGE;
        garbageData["weight"] = 10.0;
        garbageData["picture"] = base64;
        const auto nowTime = std::chrono::system_clock::now();
        garbageData["msgId"] = std::to_string( std::chrono::duration_cast<std::chrono::seconds>(
                nowTime.time_since_epoch()).count());
        json garbageMsg;
        garbageMsg["topic"] = "garbageInfo";
        garbageMsg["data"] = garbageData;

        wsclient->send(garbageMsg.dump());

    }

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
                    auto wsconn = wsConn::getInstance(AS_BACKEND_WS_CLIENT);

                    json data;
                    data["cardNo"] = std::string((const char*)struAlarmInfo.struAcsEventInfo.byCardNo);
                    data["deviceNo"] = "100243536";
                    json msg;
                    msg["topic"] = "userIdentity";
                    msg["data"] = data;
                    wsconn->send(msg.dump());

//                    auto capSrc = std::string(getCaptureAddr());
//                    auto capUsername = std::string(getCaptureUsername());
//                    auto capPassword = std::string(getCapturePassword());
//                    connection cap(capSrc.c_str(), capUsername.c_str(), capPassword.c_str());
//
//                    char pic[102400];
//                    char dst[204800];
//
//                    int picLength = cap.doCapture(pic, 102400);
//                    spdlog::debug("raw picture length: {}", picLength);
//                    mg_base64_encode(reinterpret_cast<const unsigned char *>(pic), picLength, dst);
//
//                    wsconn->send(dst);

                }
            break;
        default:
            break;
    }
}

void server(struct mg_connection *nc, int ev, void *ev_data) {
    char buf[64];
    char addr[32];
    mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                        MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

    snprintf(buf, sizeof(buf), "%s", addr);
    switch (ev) {
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
            /* New websocket connection. Tell everybody. */
            auto wss = wsConn::getInstance(AS_SCREEN_WS_SERVER);
            wss->emplace_back_connection(nc);
            spdlog::info("client {} connected", addr);
            break;
        }
        case MG_EV_WEBSOCKET_FRAME: {
            struct websocket_message *wm = (struct websocket_message *) ev_data;
            /* New websocket message. Tell everybody. */
            struct mg_str d = {(char *) wm->data, wm->size};
            spdlog::debug("receive: {}(size: {}) from {}", d.p, wm->size, addr);

            break;
        }
//    case MG_EV_HTTP_REQUEST: {
//      mg_serve_http(nc, (struct http_message *) ev_data, s_http_server_opts);
//      break;
//    }
        case MG_EV_CLOSE: {
            /* Disconnect. Tell everybody. */
            if (nc->flags & MG_F_IS_WEBSOCKET) {
//                broadcast(nc, mg_mk_str("-- left"));
                auto wss = wsConn::getInstance(AS_SCREEN_WS_SERVER);
                wss->erase_connection(nc);
                spdlog::info("client {} disconnected", addr);
            }
            break;
        }
    }
}
