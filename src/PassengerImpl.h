//
// Created by szymo on 27.04.2025.
//

#ifndef PASSENGERIMPL_H
#define PASSENGERIMPL_H
#include <Ice/Ice.h>
#include "../SIP.h"
#include <string>
#include <vector>
#include <mutex>

class PassengerImpl : public SIP::Passenger {
public:
    PassengerImpl(const std::string& name);
    virtual ~PassengerImpl() = default;

    // Interface implementation
    virtual void updateTramInfo(SIP::TramPrx tram, const SIP::StopList& stops) override;
    virtual void updateStopInfo(SIP::StopPrx stop, const SIP::TramList& trams) override;

    // Additional methods
    void watchStop(SIP::TramStopPrx stop);
    void unwatchStop(SIP::TramStopPrx stop);
    void watchTram(SIP::TramPrx tram);
    void unwatchTram(SIP::TramPrx tram);
    void displayTramInfo();
    void displayStopInfo();

private:
    std::string name_;
    std::map<std::string, SIP::StopList> tramInfos_;    // Map tram stock number to stop list
    std::map<std::string, SIP::TramList> stopInfos_;    // Map stop name to tram list
    std::vector<SIP::TramPrx> watchedTrams_;
    std::vector<SIP::TramStopPrx> watchedStops_;
    std::mutex mutex_;
};

class PassengerApp : public Ice::Application {
public:
    PassengerApp();
    virtual int run(int argc, char* argv[]) override;

private:
    void showHelp();
    void processCommands(PassengerImpl* passengerImpl, SIP::MPKPrx mpk);
};

#endif //PASSENGERIMPL_H
