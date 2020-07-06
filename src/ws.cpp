#include <ws.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <env.h>
#include <util.h>
#include <connection.h>
#include <nlohmann/json.hpp>

#include <message.h>
using json = nlohmann::json;

void* process_proc(void* p_server)
{
    auto conn = (wsConn*)p_server;

    while(!conn->done)
    {
        mg_mgr_poll(conn->GetMgr(), 100);
    }
    conn->connected = 0;
    spdlog::debug("client {} thread exit", conn->getInstanceName());
    return NULL;
}
std::mutex wsConn::mtx;
void wsConn::connect(const std::string &url) {
    if (this->connected == 2) {
        spdlog::debug("connecting");
        return;
    }
    this->connected = 2;
    this->url = url;
    this->mgr = new(mg_mgr);
    mg_connection* nc;

    mg_mgr_init(this->mgr, NULL);
    if (this->instanceName == AS_BACKEND_WS_CLIENT) {
        nc = mg_connect_ws(this->mgr, wsConn::backend_ev_handler, url.c_str(), nullptr, nullptr);
    } else if (this->instanceName == AS_MACHINE_WS_CLIENT) {
        nc = mg_connect_ws(this->mgr, wsConn::machine_ev_handler, url.c_str(), nullptr, nullptr);
    } else {
        spdlog::error("wrong ws client type {}", this->instanceName);
        this->connected = 0;
        return;
    }

    if (nc == NULL) {
        this->connected = 0;
        spdlog::error("Invalid address");
        return;
    }
    this->nc = nc;


    this->done = 0;
    mg_start_thread(process_proc, this);

//    while(!wsConn::done) {
//        mg_mgr_poll(this->mgr, 100);
//    }

    spdlog::debug("websocket {} connect done, connected: {}", this->instanceName,  this->connected);
}

bool wsConn::isConnected() {
    return this->connected;
}


void wsConn::backend_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    (void) nc;
    auto c = wsConn::getInstance(AS_BACKEND_WS_CLIENT, false);
    switch (ev) {
        case MG_EV_CONNECT: {
            int status = *((int *) ev_data);
            if (status != 0) {
                spdlog::error("backend ws connect error: {}", status);
            }
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
            auto *hm = (struct http_message *) ev_data;
            if (hm->resp_code == 101) {
                spdlog::info("backend websocket connected");
                c->connected = 1;

//                char msg[1024];
//                strcpy(msg, "connected");
//                mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, msg, strlen(msg));

            } else {
                spdlog::error("ws connected failed, http code: {}", hm->resp_code);
                /* Connection will be closed after this. */
            }
            break;
        }
        case MG_EV_POLL: {
//            char msg[500];
//            int n = 0;
//
//            fd_set read_set, write_set, err_set;
//            struct timeval timeout = {0, 0};
//            FD_ZERO(&read_set);
//            FD_ZERO(&write_set);
//            FD_ZERO(&err_set);
//            FD_SET(0 /* stdin */, &read_set);
//            if (select(1, &read_set, &write_set, &err_set, &timeout) == 1) {
//                n = read(0, msg, sizeof(msg));
//            }
//            if (n <= 0) break;
//            while (msg[n - 1] == '\r' || msg[n - 1] == '\n') n--;
//            mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, msg, n);
            break;
        }
        case MG_EV_WEBSOCKET_FRAME: {
            struct websocket_message *wm = (struct websocket_message *) ev_data;
            std::string msg((const char*)wm->data, wm->size);
            spdlog::info("receive from backend {}", msg);


            try {
                auto down = json::parse(msg);
                auto topic = down["topic"];
                if (topic == nullptr) {
                    break;
                }
                auto topicString = topic.get<std::string>();
                spdlog::debug("topic: {}", topicString);
                auto wssconn = wsConn::getInstance(AS_SCREEN_WS_SERVER, true);
                if (topicString == "face") {
                    auto data = down["data"];
                    if (data == nullptr) {
                        break;
                    }
                    auto dataDOM = json::parse(data.get<std::string>());

                    auto picture = dataDOM["pictureInfo"];
                    auto userName = dataDOM["userName"];
                    auto cardNo = dataDOM["identityNo"];
                    if (picture == nullptr || userName == nullptr || cardNo == nullptr ||
                        !picture.is_string() || !userName.is_string() || !cardNo.is_string()) {
                        spdlog::error("loss of field or wrong type");
                        break;
                    }

                    auto acSrc = std::string(getAccessAddr());
                    auto acUsername = std::string(getAccessUsername());
                    auto acPassword = std::string(getAccessPassword());
                    connection hik_c(acSrc.c_str(), acUsername.c_str(), acPassword.c_str());

                    auto cards = hik_c.doGetCards();
                    std::map<long, bool> employeeSet;

                    auto cardNoExist = false;
                    for (auto &card : cards) {
                        employeeSet.insert(std::make_pair(card.getEmployeeId(), true));

                        auto itCardNo = std::stol(card.getCardNo());
                        auto setCardNo = std::stol(cardNo.get<std::string>());
                        if (itCardNo == setCardNo) {
                            cardNoExist = true;
                        }
                    }

                    long nextEmployeeId = 0;
                    for (long i = 0;; i++) {
                        if (employeeSet.find(i) != employeeSet.end()) {
                            spdlog::debug("next employee id found: {}", nextEmployeeId);
                            nextEmployeeId = i;
                            break;
                        }
                    }

                    spdlog::debug("hik sdk {} connected", hik_c.isLogin() ? "is" : "NOT");
                    if(!hik_c.isLogin()) {
                        spdlog::error("access connect failed");
                        break;
                    }
                    if (!cardNoExist) {
                        // 不存在, 先下发此cardNo
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
                        if (!hik_c.doSetCard(*newcard)) {
                            spdlog::info("do set card err: {} when set face", NET_DVR_GetLastError);
                            break;
                        }

                    }
                    // 再下发人脸
                    auto base64 = picture.get<std::string>();
                    char face[1024 * 1024] = {0};
                    auto picLength = mg_base64_decode((const unsigned char*)base64.c_str(), base64.length(), face);
                    hik_c.doSetFace(cardNo.get<std::string>(), face, picLength);
                } else if (topicString == "ic") {
                    // do nothing now
                } else if (topicString == "userIdentity") {
                    auto code = down["code"];
                    auto msg = down["msg"];
                    if (code == nullptr || msg == nullptr) {
                        spdlog::error("userIdentity response less code/msg");
                        break;
                    }

                    auto codeInt = code.get<int>();

                    switch (codeInt) {
                        case IDENTITY_SUCCESS: {
                            auto msgDOM = json::parse(msg.get<std::string>());
                            auto userName = msgDOM["userName"];
                            auto point = msgDOM["point"];
                            auto cardNo = msgDOM["cardNo"];
                            if (userName == nullptr || point == nullptr || cardNo == nullptr) {
                                spdlog::error("msg response less userName/point");
                                break;
                            }

                            auto citizen = wsConn::getCitizen();
                            wsConn::lock();
                            citizen->setCardNo(cardNo.get<std::string>());
                            citizen->setName(userName.get<std::string>());
                            citizen->setPoint(point.get<int>());
                            citizen->setTimestampNow();
                            citizen->setStatus(PERSON_LOGIN);
                            wsConn::unlock();

                            std::thread p([](std::string cardNo){
                                auto citizenPtr = wsConn::getCitizen();
                                wsConn::lock();
                                if (citizenPtr->getCardNo() != cardNo || citizenPtr->getStatus() == PERSON_EXIT) {
                                    wsConn::unlock();
                                    return;
                                }

                                auto machineClient = wsConn::getInstance(AS_MACHINE_WS_CLIENT, true);
                                spdlog::debug("machine is connected: {}", machineClient->isConnected());
                                if (machineClient->isConnected()) {
                                    // 告诉驱动开始扔垃圾
                                    json msg;
                                    msg["topic"] = MACHINE_START_DROP;
                                    machineClient->send(msg.dump());

                                    // 等居民扔垃圾
                                    citizenPtr->setStatus(PERSON_DROPPING);
                                    std::this_thread::sleep_for(std::chrono::seconds (5));
                                    wsConn::unlock();

                                    // 告诉驱动扔垃圾完成
                                    citizenPtr = wsConn::getCitizen();
                                    wsConn::lock();
                                    if (citizenPtr->getCardNo() != cardNo || citizenPtr->getStatus() == PERSON_EXIT) {
                                        wsConn::unlock();
                                        return;
                                    }
                                    citizenPtr->setStatus(PERSON_WAITING);
                                    wsConn::unlock();

                                    msg["topic"] = MACHINE_FINISH_DROP;
                                    machineClient->send(msg.dump());
                                } else {
                                    wsConn::unlock();
                                    spdlog::error("machine connection error");
                                }

                                std::this_thread::sleep_for(std::chrono::seconds (80));
                                const auto nowTime = std::chrono::system_clock::now();
                                auto nowTimeUnix = std::chrono::duration_cast<std::chrono::seconds>(
                                        nowTime.time_since_epoch()).count();
                                spdlog::debug("nowTimeUnix: {}", nowTimeUnix);

                                citizenPtr = wsConn::getCitizen();
                                spdlog::debug("timestamp: {} citizencard: {} cardNo: {} status: {}",
                                        citizenPtr->getTimestamp(), citizenPtr->getCardNo(), cardNo, citizenPtr->getStatus());
                                wsConn::lock();
                                if (citizenPtr->getCardNo() != cardNo || citizenPtr->getStatus() == PERSON_EXIT) {
                                    wsConn::unlock();
                                    return;
                                } else if (nowTimeUnix - citizenPtr->getTimestamp() > 75) {
                                    spdlog::info("{} timeout", cardNo);
                                    auto wsServer = wsConn::getInstance(AS_SCREEN_WS_SERVER, true);

                                    json msgObj;
                                    fe::frontendMsg m;
                                    m.user.id = cardNo;
                                    m.user.username = citizenPtr->getName();
                                    m.user.pointTotal = citizenPtr->getPoint();
                                    m.operationType = "logout";

                                    fe::to_json(msgObj, m);

                                    auto msgStr = msgObj.dump();
                                    wsServer->sendToAll(msgStr.c_str());
                                    wsConn::unlock();
                                    return;
                                }

                            }, cardNo.get<std::string>());
                            p.detach();

                            fe::frontendMsg m;
                            m.user.id = cardNo.get<std::string>();
                            m.user.username = userName.get<std::string>();
                            m.user.pointTotal = point.get<int>();
                            m.operationType = "login";
                            json notifyObj;
                            fe::to_json(notifyObj, m);

                            auto msgStr = notifyObj.dump();
                            wssconn->sendToAll(msgStr.c_str());

                            break;
                        }
                        case IDENTITY_NOT_FOUND:
                        case IDENTITY_WRONG_STATION: {
                            fe::frontendMsg m;
                            m.operationType = "logerror";
                            json notifyObj;
                            fe::to_json(notifyObj, m);
                            auto msgStr = notifyObj.dump();
                            wssconn->sendToAll(msgStr.c_str());
                            break;
                        }
                        case IDENTITY_NOT_TIME: {
                            fe::frontendMsg m;
                            m.operationType = "outdate";
                            json notifyObj;
                            fe::to_json(notifyObj, m);
                            auto msgStr = notifyObj.dump();
                            wssconn->sendToAll(msgStr.c_str());
                            break;
                        }
                        default: {
                            break;
                        }
                    }

                } else if (topicString == "garbageInfo"){
                    auto msg = down["msg"];
                    auto code = down["code"];
                    if (msg == nullptr || code == nullptr || code.get<int>() != 200) {
                        spdlog::error("userIdentity response less msg");
                        break;
                    }
                    auto msgDOM = json::parse(msg.get<std::string>());
                    auto isPass = msgDOM["passed"];
                    auto pointTotal = msgDOM["pointTotal"];
                    auto thePoints = msgDOM["thePoints"];
                    auto othersWeight = msgDOM["othersWeight"];
                    auto perishableWeight = msgDOM["perishableWeight"];
                    auto cardNo = msgDOM["cardNo"];
                    if (pointTotal == nullptr || thePoints == nullptr || othersWeight == nullptr || perishableWeight == nullptr || cardNo == nullptr) {
                        spdlog::error("userIdentity response less pointTotal/thePoints");
                        break;
                    }
                    auto citizen = wsConn::getCitizen();
                    wsConn::lock();
                    if (citizen->getCardNo() != cardNo || citizen->getStatus() == PERSON_EXIT) {
                        spdlog::error("wrong status");
                        wsConn::unlock();
                        return;
                    }

                    if (thePoints.get<int>() > 0) {
                        citizen->setPoint(citizen->getPoint() + thePoints.get<int>());
                    }
//                    citizen->setTimestampNow();

                    auto isDroppingWetGarbage = isPass != nullptr;
                    fe::frontendMsg m;
                    if (isPass == nullptr) {
                        // 未投递易腐垃圾
                        fe::rubbishT other;
                        other.type = OTHER_GARBAGE;
                        other.point = 0;
                        other.weight = othersWeight.get<int>();
                        m.rubbishList.emplace_back(other);
                    }  else {
                        if (isPass.get<bool>()){
                            fe::rubbishT wet;
                            wet.type = WET_GARBAGE;
                            wet.point = thePoints.get<int>();
                            wet.weight = perishableWeight.get<int>();
                            m.rubbishList.emplace_back(wet);

                            fe::rubbishT other;
                            other.type = OTHER_GARBAGE;
                            other.point = 0;
                            other.weight = othersWeight.get<int>();
                            m.rubbishList.emplace_back(other);
                        } else {
                            fe::rubbishT other;
                            other.type = OTHER_GARBAGE;
                            other.point = 0;
                            other.weight = othersWeight.get<int>() + perishableWeight.get<int>();
                            m.rubbishList.emplace_back(other);
                        }
                    }

                    m.user.id = citizen->getCardNo();
                    m.user.username = citizen->getName();
                    m.user.pointTotal = citizen->getPoint();
                    m.operationType = "drop";
                    wsConn::unlock();

                    {
                        auto machineClient = wsConn::getInstance(AS_MACHINE_WS_CLIENT, true);
                        json MachineMsgObj;
                        json PayloadObj;
                        PayloadObj["is_take_photo"] = isDroppingWetGarbage;
                        PayloadObj["is_perishable_waste_right"] = isDroppingWetGarbage ? isPass.get<bool>() : false;
                        MachineMsgObj["payload"] = PayloadObj;
                        MachineMsgObj["error"] = "";
                        MachineMsgObj["topic"] = MACHINE_PHOTO_RESULT;

                        machineClient->send(MachineMsgObj.dump());
                    }
                    json notifyObj;
                    fe::to_json(notifyObj, m);
                    auto msgStr = notifyObj.dump();
                    wssconn->sendToAll(msgStr.c_str());
                } else {
                    spdlog::warn("wrong topic");
                }
            } catch (json::exception &exception) {
                spdlog::error("catch json exception: {}", exception.what());
            }


            break;
        }
        case MG_EV_CLOSE: {
            if (c->isConnected())
                spdlog::info("websocket {} disconnected", "backend");
            c->connected = false;
            c->done = 1;
            break;
        }
    }
}

void wsConn::machine_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    (void) nc;
    auto c = wsConn::getInstance(AS_MACHINE_WS_CLIENT, false);
    switch (ev) {
        case MG_EV_CONNECT: {
            int status = *((int *) ev_data);
            if (status != 0) {
                spdlog::error("ws connect error: {}", status);
            }
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
            auto *hm = (struct http_message *) ev_data;
            if (hm->resp_code == 101) {
                spdlog::info("machine websocket connected");
                c->connected = 1;
            } else {
                spdlog::error("ws connected failed, http code: {}", hm->resp_code);
                /* Connection will be closed after this. */
            }
            break;
        }
        case MG_EV_POLL: {
            break;
        }
        case MG_EV_WEBSOCKET_FRAME: {
            struct websocket_message *wm = (struct websocket_message *) ev_data;
            std::string msg((const char*)wm->data, wm->size);
            spdlog::info("receive from machine {}", msg);

            try {
                auto down = json::parse(msg);
                auto topic = down["topic"];
                auto payload = down["payload"];
                auto error = down["error"];
                if (topic == nullptr) {
                    spdlog::error("none topic msg");
                    break;
                }

                auto topicStr = topic.get<std::string>();
                if (topicStr == MACHINE_REPORT_WEIGHT) {
                    if (payload == nullptr) {
                        break;
                    }
                    auto hasWetGarbageObj = payload["perishable_waste_flag"];
                    auto WetGarbageWeightObj = payload["perishable_waste_weight"];
                    auto hasOtherGarbageObj = payload["not_perishable_waste_flag"];
                    auto OtherGarbageWeightObj = payload["not_perishable_waste_weight"];
                    if (hasWetGarbageObj == nullptr || WetGarbageWeightObj == nullptr || hasOtherGarbageObj == nullptr || OtherGarbageWeightObj == nullptr) {
                        spdlog::error("loss of field");
                        break;
                    }
                    bool hasWetGarbage = false;
                    auto WetGarbageWeight = 0;
                    bool hasOtherGarbage = false;
                    auto OtherGarbageWeight = 0;
                    hasWetGarbage = hasWetGarbageObj.get<bool>();
                    WetGarbageWeight = WetGarbageWeightObj.get<int32_t>();
                    hasOtherGarbage = hasOtherGarbageObj.get<bool>();
                    OtherGarbageWeight = OtherGarbageWeightObj.get<int32_t>();

                    std::vector<unsigned char> base64buffer(static_cast<size_t>(1024 * 1024));
                    if (hasWetGarbage) {
                        // 有湿垃圾,拍照
                        auto capSrc = std::string(getCaptureAddr());
                        auto capUsername = std::string(getCaptureUsername());
                        auto capPassword = std::string(getCapturePassword());
                        connection hik_cap(capSrc.c_str(), capUsername.c_str(), capPassword.c_str());

                        std::vector<unsigned char> buffer(static_cast<size_t>(512 * 1024));
                        auto picLength = hik_cap.doCapture(reinterpret_cast<char *>(buffer.data()), buffer.size());


                        mg_base64_encode(buffer.data(), picLength, reinterpret_cast<char *>(base64buffer.data()));


                    }
                    auto citizen = wsConn::getCitizen();
                    wsConn::lock();

                    const auto nowTime = std::chrono::system_clock::now();
                    auto nowTimeUnix = std::chrono::duration_cast<std::chrono::seconds>(
                            nowTime.time_since_epoch()).count();
                    // send to backend
                    json garbageInfoDataObj;
                    garbageInfoDataObj["msgId"] = nowTimeUnix;
                    garbageInfoDataObj["cardNo"] = citizen->getCardNo();
                    garbageInfoDataObj["deviceNo"] = getDeviceNo();
                    garbageInfoDataObj["perishableExist"] = hasWetGarbage;
                    garbageInfoDataObj["perishableWeight"] = WetGarbageWeight;
                    garbageInfoDataObj["othersExist"] = hasOtherGarbage;
                    garbageInfoDataObj["othersWeight"] = OtherGarbageWeight;
                    if (hasWetGarbage) {
                        garbageInfoDataObj["picture"] = std::string(reinterpret_cast<char *>(base64buffer.data()), strlen(reinterpret_cast<char *>(base64buffer.data())));
                    }
                    json msgObj;
                    msgObj["topic"] = "garbageInfo";
                    msgObj["data"] = garbageInfoDataObj;
//                    spdlog::debug("garbageInfo: {}", msgObj.dump());

                    auto backendClient = wsConn::getInstance(AS_BACKEND_WS_CLIENT, true);
                    backendClient->send(msgObj.dump());

                    wsConn::unlock();

                } else if (topicStr == MACHINE_REPORT_END) {
                    auto citizen = wsConn::getCitizen();
                    wsConn::lock();
                    if (citizen->getStatus() != PERSON_EXIT) {
                        citizen->setStatus(PERSON_EXIT);
                    }
                    fe::frontendMsg m;
                    m.user.id = citizen->getCardNo();
                    m.user.username = citizen->getName();
                    m.user.pointTotal = citizen->getPoint();
                    m.operationType = "logout";

                    wsConn::unlock();

                    auto wsServer = wsConn::getInstance(AS_SCREEN_WS_SERVER, true);
                    json msgObj;
                    fe::to_json(msgObj, m);
                    auto msgStr = msgObj.dump();
                    wsServer->sendToAll(msgStr.c_str());

                } else if (topicStr == MACHINE_REPORT_ERROR) {
                    //
                    auto citizen = wsConn::getCitizen();
                    wsConn::lock();
                    if (error.is_string()) {
                        auto errorStr = error.get<std::string>();
                        if (errorStr.empty()) {
                            auto runningStateObj = payload["running_state"];
                            auto equipmentStateObj = payload["equipment_state"];
                            if (runningStateObj == nullptr || equipmentStateObj == nullptr) {
                                wsConn::unlock();
                                spdlog::error("no field in {}", MACHINE_REPORT_ERROR);
                                break;
                            }

                            auto runningState = runningStateObj.get<int>();
                            auto equipmentState = equipmentStateObj.get<unsigned int>();
                            citizen->setMachineStatus(static_cast<MACHINE_STATUS>(runningState));
                        } else {
                            spdlog::error("machine err: {}", errorStr);
                            citizen->setMachineStatus(MACHINE_ERROR);
                        }

                    }

                    wsConn::unlock();
                } else if (topicStr == MACHINE_REPORT_SET_MODE) {
                    // 设备状态
                } else {
                    spdlog::error("unknown topic from machine");
                }



            } catch (std::exception &e) {
                spdlog::error("catch exception: {}", e.what());
            }

            break;
        }
        case MG_EV_CLOSE: {
            if (c->isConnected())
                spdlog::info("ws {} disconnected", "machine");
            c->connected = false;
            c->done = 1;
            break;
        }
    }
}

bool wsConn::send(const std::string &wsMessage) {
    if(!this->isConnected()) {
        spdlog::error("{} not connected", this->instanceName);
        return false;
    }
    spdlog::debug("send to {}: {}", this->instanceName, wsMessage);
    char msg[1024 * 1024] ={0};
    memcpy(msg, wsMessage.c_str(), wsMessage.size());
    mg_send_websocket_frame(this->nc, WEBSOCKET_OP_TEXT, msg, strlen(msg));
    return true;
}

mg_mgr *wsConn::GetMgr() {
    return this->mgr;
}

void wsConn::close() {
    wsConn::done = true;
    mg_mgr_free(this->mgr);
}

bool wsConn::bindAndServe(const char *port, MG_CB(mg_event_handler_t handler,
                                                  void *user_data)) {

    this->mgr = new(mg_mgr);
    mg_mgr_init(this->mgr, NULL);

    this->nc = mg_bind(this->mgr , port, handler);
    mg_set_protocol_http_websocket(this->nc);
    mg_start_thread(process_proc, this);

    return true;
}

void wsConn::emplace_back_connection(mg_connection *nc) {
    this->clientList.emplace_back(nc);
}

bool wsConn::erase_connection(mg_connection *c) {
    auto search = this->clientList.end();
    for(auto it = this->clientList.begin(); it < this->clientList.end(); it++) {
        if (*it == c) {
            search = it;
        }
    }
    if(search != this->clientList.end()) {
        spdlog::debug("ws connection found, delete");
        this->clientList.erase(search);
        return true;
    }

    spdlog::error("ws connection not found");
    return false;
}

void wsConn::sendToAll(const char *msg) {
    spdlog::debug("send to frontend: {}", msg);
    for(auto &c : this->clientList) {
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, msg, strlen(msg));
    }
}

const std::string &wsConn::getInstanceName() const {
    return instanceName;
}

const std::string &wsConn::Citizen::getName() const {
    return name;
}

void wsConn::Citizen::setName(const std::string &name) {
    this->checkLock();
    Citizen::name = name;
}

const std::string &wsConn::Citizen::getCardNo() const {
    return cardNo;
}

void wsConn::Citizen::setCardNo(const std::string &cardNo) {
    this->checkLock();
    Citizen::cardNo = cardNo;
}

int wsConn::Citizen::getPoint() const {
    return point;
}

void wsConn::Citizen::setPoint(int point) {
    this->checkLock();
    Citizen::point = point;
}

json wsConn::Citizen::formJSON() {
    json j;
    j["username"] = this->getName();
    j["id"] = std::stol(this->getCardNo());
    j["pointTotal"] = this->getPoint();
    return json(j);
}

int wsConn::Citizen::getTimestamp() const {
    return timestamp;
}

void wsConn::Citizen::setTimestamp(int timestamp) {
    this->checkLock();
    Citizen::timestamp = timestamp;
}

void wsConn::Citizen::setTimestampNow() {
    this->checkLock();
    const auto nowTime = std::chrono::system_clock::now();
    Citizen::setTimestamp(std::chrono::duration_cast<std::chrono::seconds>(
            nowTime.time_since_epoch()).count());
}

PERSON_STATUS wsConn::Citizen::getStatus() const {
    return status;
}

void wsConn::Citizen::setStatus(PERSON_STATUS status) {
    this->checkLock();
    Citizen::status = status;
}

void wsConn::Citizen::checkLock()  {
//    if (mtx.try_lock()) {
//        throw std::runtime_error("using after unlock");
//    }
}

MACHINE_STATUS wsConn::Citizen::getMachineStatus() const {
    return machineStatus;
}

void wsConn::Citizen::setMachineStatus(MACHINE_STATUS machineStatus) {
    Citizen::machineStatus = machineStatus;
}
