//
// Created by srt on 2020/7/3.
//

#ifndef AC_MESSAGE_H
#define AC_MESSAGE_H

#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace fe {
    struct userT {
        std::string id = "";
        std::string username = "";
        int pointTotal = 0;
    };

    struct rubbishT {
        int type = 0;
        int weight = 0;
        int point = 0;
    };

    struct frontendMsg {
        std::string operationType;
        std::vector<rubbishT> rubbishList;
        userT user;
    };

    void to_json(json& j, const frontendMsg& m) {
        j = json{{"operationType", m.operationType},
                 {"user", {{"id", m.user.id}, {"username", m.user.username}, {"pointTotal", m.user.pointTotal}} }
        };
        auto rubbishListObj = json::array();
        for (auto &rubbish : m.rubbishList) {
            json rubbishObj = {
                    {"type", rubbish.type},
                    {"weight", rubbish.weight},
                    {"point", rubbish.point},
            };
            rubbishListObj.push_back(rubbishObj);
        }
        j["rubbishList"] = rubbishListObj;
    }

//    void from_json(const json& j, frontendMsg& m) {
//        j.at("operationType").get_to(m.operationType);
//    }
}



#endif //AC_MESSAGE_H
