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
#include "connection.h"
#include "env.h"
#include "httplib/httplib.h"

void CALLBACK MessageCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser);

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::trace);
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
        if(hik_ac.isLogin() &&  hik_ac.setAlarm(MessageCallback)) {
            spdlog::info("access login and set alarm ok");
            break;
        } else {
            spdlog::error("hik ac connect error");
            if (NET_DVR_PASSWORD_ERROR == NET_DVR_GetLastError()) {
                spdlog::error("password not match, please restart service....");
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!hik_ac.isLogin()) {
                hik_ac.doConnect();
            }
        }

    }

    httplib::Server svr;

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
                spdlog::info("do set card err: {} when set face", NET_DVR_GetLastError());
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

        if (hik_ac.doSetFace(cardNo.get<std::string>(), picBinary.c_str(), picBinary.length())) {
            res.body = "ok";
        } else {
            res.body = "error";
        }

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

    svr.Get("/card", [&](const httplib::Request& req, httplib::Response& res) {
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
            auto card = hik_ac.doGetCard(cardNo);
            if (card) {
                res.body = card->String();
            }
        } else {
            res.body = "error";
        }
    });

    svr.Get("/cards", [&](const httplib::Request& req, httplib::Response& res){
        if (!hik_ac.isLogin()) {
            spdlog::error("access not connected");
            res.body = "error";
            return;
        }
        auto cards = hik_ac.doGetCards();
        for (auto &&card : cards) {
            res.body += (card.String() + "\n");
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

    if (lCommand != COMM_ALARM_ACS) {
        return;
    }

    NET_DVR_ACS_ALARM_INFO struAlarmInfo;
    memcpy(&struAlarmInfo, pAlarmInfo, sizeof(NET_DVR_ACS_ALARM_INFO));
    spdlog::debug("cardNo {} major {:x} minor {:x}",  struAlarmInfo.struAcsEventInfo.byCardNo, struAlarmInfo.dwMajor, struAlarmInfo.dwMinor);
    switch (struAlarmInfo.dwMajor) {
        case MAJOR_EVENT: {
            switch (struAlarmInfo.dwMinor) {
                case MINOR_LEGAL_CARD_PASS:
                case MINOR_INVALID_CARD:
                case MINOR_FACE_VERIFY_PASS:{
                    // make a login request
                    std::string accessorCardNo((const char*)struAlarmInfo.struAcsEventInfo.byCardNo);
                    auto ctl = std::string(getMachineHTTPServer());
                    httplib::Client cli(ctl.c_str());

                    auto uri = "/login?cardno=" + accessorCardNo + "&line=" + getLineNumber();
                    auto res = cli.Get(uri.c_str());
                    if (res && res->status == 200) {
                        spdlog::debug("login:", res->body);
                    } else {
                        auto err = res.error();
                        spdlog::error("login error: {}", err);
                    }
                    if (struAlarmInfo.dwPicDataLen > 0) {


                        char buff[1024];
                        snprintf(buff, sizeof(buff), "/tmp/%ld_fail.jpg", std::time(nullptr));


                        std::ofstream pic;
                        pic.open(buff, std::ios::binary | std::fstream::trunc | std::fstream::in | std::fstream::out);
                        if (!pic.is_open()) {
                            std::cout << "failed to open" << std::endl;
                            break;
                        }
                        pic << struAlarmInfo.pPicData;
                        pic.close();

                    } else {
                        spdlog::debug("no image when verify passed");
                    }
                    break;
                }
                case MINOR_FACE_VERIFY_FAIL:{
                    if (struAlarmInfo.dwPicDataLen > 0) {


                        char buff[1024];
                        snprintf(buff, sizeof(buff), "/tmp/%ld_fail.jpg", std::time(nullptr));


                        std::ofstream pic;
                        pic.open(buff, std::ios::binary | std::fstream::trunc | std::fstream::in | std::fstream::out);
                        if (!pic.is_open()) {
                            std::cout << "failed to open" << std::endl;
                            break;
                        }
                        pic << struAlarmInfo.pPicData;
                        pic.close();

                    } else {
                        spdlog::debug("no image when verify failed");
                    }

                    break;
                default:
                    break;
                }
            }
        }
            break;
        case MAJOR_OPERATION: {

            break;
        }
        default:
            break;
    }
}
