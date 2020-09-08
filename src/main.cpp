#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <thread>
#include <fstream>
#include <iostream>

#include "HCNetSDK.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <base64.h>
#include <util.h>
#include "ws.h"
#include "connection.h"
#include "env.h"
#include "httplib/httplib.h"

void CALLBACK MessageCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser);
void server(struct mg_connection *nc, int ev, void *ev_data);

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("access controller coming...");

//    //
//    std::string AS_CLIENT = AS_BACKEND_WS_CLIENT;
//    auto wsclient = wsConn::getInstance(AS_CLIENT, true);
//    auto backendAddr = getBackendWSServer();
//    spdlog::debug("connecting to backend {}", backendAddr);
//    wsclient->connect(backendAddr);
//
//    //
//    auto machineClient = wsConn::getInstance(AS_MACHINE_WS_CLIENT, true);
//    machineClient->connect(getMachineWSServer());
//
//    //
//    auto wssconn = wsConn::getInstance(AS_SCREEN_WS_SERVER, true);
//    wssconn->bindAndServe("8085", server);

    auto acSrc = std::string(getAccessAddr());
    auto acUsername = std::string(getAccessUsername());
    auto acPassword = std::string(getAccessPassword());
    connection hik_ac(acSrc.c_str(), acUsername.c_str(), acPassword.c_str());

    spdlog::debug("hik sdk {} connected", hik_ac.isLogin() ? "is" : "NOT");

    while(1) {
        if(hik_ac.isLogin()) {
            hik_ac.setAlarm(MessageCallback);
            break;

//        auto newcard = std::make_shared<card>();
//        newcard->setCardNo("1593768284");
//        newcard->setCardType(1);
//        newcard->setName("新用户233");
//        newcard->setEmployeeId(std::stol("1593768284"));
//        time_t timep;
//        time(&timep); /*获得time_t结构的时间，UTC时间*/
//        auto p = gmtime(&timep);
//        auto newe = *p;
//        newe.tm_year+=100;
//        newcard->setBeginTime(*p);
//        newcard->setEndTime(newe);
//        if (!hik_ac.doSetCard(*newcard)) {
//            spdlog::info("do set card err: {} when set face", NET_DVR_GetLastError);
//            return 0;
//        }
//        hik_ac.doSetFace("1001", "/home/srt/froggggg.jpeg");
        } else {
            spdlog::error("hik ac connect error");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            hik_ac.doConnect();
        }

    }

    httplib::Server svr;
    svr.Get("/capture",  [](const httplib::Request& req, httplib::Response& res) {
        auto capSrc = std::string(getCaptureAddr());
        auto capUsername = std::string(getCaptureUsername());
        auto capPassword = std::string(getCapturePassword());
        connection hik_cap(capSrc.c_str(), capUsername.c_str(), capPassword.c_str());
        hik_cap.doConnect();

        std::vector<unsigned char> buffer(static_cast<size_t>(512 * 1024));
        auto picLength = hik_cap.doCapture(reinterpret_cast<char *>(buffer.data()), buffer.size());
        if (picLength == 0) {
            res.body = "error";
            res.status = 401;
            return;
        }

        std::vector<unsigned char> base64buffer(static_cast<size_t>(1024 * 1024));
        mg_base64_encode(buffer.data(), picLength, reinterpret_cast<char *>(base64buffer.data()));


        res.set_content(std::string(reinterpret_cast<char *>(base64buffer.data()), strlen(reinterpret_cast<char *>(base64buffer.data()))), "text/plain");
    });

    svr.Put("/face", [&](const httplib::Request& req, httplib::Response& res) {

        auto dom = json::parse(req.body);
        auto picture = dom["pictureInfo"];
        auto userName = dom["userName"];
        auto cardNo = dom["identityNo"];
//
//        auto acSrc = std::string(getAccessAddr());
//        auto acUsername = std::string(getAccessUsername());
//        auto acPassword = std::string(getAccessPassword());
//        connection hik_c(acSrc.c_str(), acUsername.c_str(), acPassword.c_str());
//        hik_c.doConnect();
        if (!hik_ac.isLogin()) {
            spdlog::error("access not connected");
            res.body = "error";
            return;
        }

        auto cards = hik_ac.doGetCards();
        std::map<long, bool> employeeSet;

        auto cardNoExist = false;
        for (auto &card : cards) {
            spdlog::debug("exist employeeid: {}", card.getEmployeeId());
            employeeSet.insert(std::make_pair(card.getEmployeeId(), true));

            auto itCardNo = std::stol(card.getCardNo());
            auto setCardNo = std::stol(cardNo.get<std::string>());
            if (itCardNo == setCardNo) {
                cardNoExist = true;
            }
        }

        long nextEmployeeId = 10001;
        for (long i = nextEmployeeId;; i++) {
            if (employeeSet.find(i) == employeeSet.end()) {
                spdlog::debug("next employee id found: {}", i);
                nextEmployeeId = i;
                break;
            }
        }

        spdlog::debug("hik sdk {} connected", hik_ac.isLogin() ? "is" : "NOT");
//        if(!hik_ac.isLogin()) {
//            spdlog::error("access connect failed");
//            res.body = "error";
//            return;
//        }
        if (!cardNoExist) {
            // 不存在, 先下发此cardNo
            spdlog::debug("set card {}", cardNo.get<std::string>());
            auto newcard = std::make_shared<card>();
            newcard->setCardNo(cardNo.get<std::string>());
            newcard->setCardType(1);
            newcard->setName(userName.get<std::string>());
            newcard->setEmployeeId(nextEmployeeId);
            time_t timep;
            time(&timep); /*获得time_t结构的时间，UTC时间*/
            auto p = gmtime(&timep);
            auto newe = *p;
            newe.tm_year+=10;
            newcard->setBeginTime(*p);
            newcard->setEndTime(newe);
            if (!hik_ac.doSetCard(*newcard)) {
                spdlog::info("do set card err: {} when set face", NET_DVR_GetLastError);
                res.body = "error";
                return;
            }

        }
        // 再下发人脸
        auto base64 = picture.get<std::string>();
        if(base64.empty()) {
            spdlog::debug("noface");
            res.body = "ok";
            return;
        }
        auto picBinary = base64_decode(base64);

        hik_ac.doSetFace(cardNo.get<std::string>(), picBinary.c_str(), picBinary.length());
        res.body = "ok";
    });

    svr.Delete("/face", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hik_ac.isLogin()) {
            spdlog::error("access not connected");
            res.body = "error";
            return;
        }

        auto cardNo = std::string("");
        auto cardNoItr = req.params.find("cardNo");
        while(cardNoItr != req.params.end()) {
            cardNo = cardNoItr->second;
            break;
        }
        if (!cardNo.empty()) {
            if(hik_ac.doRemoveFaces(cardNo)) {
                res.body = "ok";
            } else {
                res.body = "error";
            }
        } else {
            res.body = "error";
        }
   });

    svr.Delete("/all", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hik_ac.isLogin()) {
            spdlog::error("access not connected");
            res.body = "error";
            return;
        }
        if(hik_ac.DoClearAll()) {
            res.body = "ok";
        } else {
            res.body = "error";
        }

    });

    svr.listen("0.0.0.0", 8081);

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
//                    auto wsconn = wsConn::getInstance(AS_BACKEND_WS_CLIENT, true);
//                    if (!wsconn->isConnected()) {
//                        auto backendAddr = getBackendWSServer();
//                        wsconn->connect(backendAddr);
//                        if (!wsconn->isConnected()) {
//                            // todo: 后端连不上,告知前端
//
//                            spdlog::error("can't connect to backend");
//                            break;
//                        }
//                    }
                    std::string accessorCardNo((const char*)struAlarmInfo.struAcsEventInfo.byCardNo);
                    auto ctl = std::string(getMachineHTTPServer());
                    httplib::Client cli(ctl.c_str());

                    auto uri = "/login?cardno=" + accessorCardNo;
                    auto res = cli.Get(uri.c_str());
                    if (res && res->status == 200) {
                            spdlog::debug("login:", res->body);
                    } else {
                        auto err = res.error();
                        spdlog::error("login error: {}", err);
                    }
//                    auto citizen = wsConn::getCitizen();
//                    wsConn::lock();
//                    if (citizen->getCardNo() != accessorCardNo && citizen->getStatus() != PERSON_EXIT) {
//                        spdlog::info("occupied by {}", citizen->getName());
//                        wsConn::unlock();
//                        break;
//                    }
//                    wsConn::unlock();
//
//                    json data;
//                    data["cardNo"] = std::string((const char*)struAlarmInfo.struAcsEventInfo.byCardNo);
//                    data["deviceNo"] = getDeviceNo();
//                    json msg;
//                    msg["topic"] = "userIdentity";
//                    msg["data"] = data;
//                    wsconn->send(msg.dump());

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
            auto wss = wsConn::getInstance(AS_SCREEN_WS_SERVER, true);
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
                auto wss = wsConn::getInstance(AS_SCREEN_WS_SERVER, true);
                wss->erase_connection(nc);
                spdlog::info("client {} disconnected", addr);
            }
            break;
        }
    }
}
