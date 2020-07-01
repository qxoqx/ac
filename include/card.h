//
// Created by srt on 2020/6/23.
//
#include <string>
#include <chrono>

#ifndef AC_CARD_H
#define AC_CARD_H


class card {
    std::string cardNo;
    long employeeID{0};
    std::string name;
    std::string password = "123456";
    long cardType{1};
    std::tm endTime{};
    std::tm beginTime{};

public:
    card(){};
    const std::string &getCardNo() const;
    void setCardNo(const std::string &cardNo);
    const std::string &getName() const;
    void setName(const std::string &name);
    long getCardType() const;
    void setCardType(long cardType);
    const tm &getBeginTime() const;
    void setBeginTime(const tm &beginTime);
    const tm &getEndTime() const;
    void setEndTime(const tm &endTime);
    long getEmployeeId() const;
    void setEmployeeId(long employeeId);
    const std::string &getPassword() const;
    void setPassword(const std::string &password);

    std::string String();
};


#endif //AC_CARD_H
