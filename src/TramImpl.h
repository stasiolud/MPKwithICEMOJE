//
// Created by szymo on 27.04.2025.
//

#ifndef TRAMIMPL_H
#define TRAMIMPL_H
#include <Ice/Ice.h>
#include "SIP.h"
#include <string>
#include <vector>
#include <thread>
#include <chrono>

class TramImpl : public SIP::Tram {
public:
    TramImpl(const std::string& stockNumber, SIP::MPKPrx mpk);
    virtual ~TramImpl();

    // Interface implementation
    virtual SIP::TramStopPrx getLocation() override;
    virtual SIP::LinePrx getLine() override;
    virtual void setLine(SIP::LinePrx line) override;
    virtual SIP::StopList getNextStops(int howMany) override;
    virtual void RegisterPassenger(SIP::PassengerPrx p) override;
    virtual void UnregisterPassenger(SIP::PassengerPrx p) override;
    virtual std::string getStockNumber() override;

    // Additional methods
    void startSimulation();
    void stopSimulation();

private:
    std::string stockNumber_;
    SIP::MPKPrx mpk_;
    SIP::LinePrx line_;
    SIP::TramStopPrx currentStop_;
    int currentStopIndex_;
    SIP::StopList routeStops_;
    std::vector<SIP::PassengerPrx> passengers_;

    bool simulationRunning_;
    std::thread simulationThread_;
    Ice::Mutex mutex_;

    void simulationLoop();
    void moveToNextStop();
    void notifyPassengers();
};

class TramApp : public Ice::Application {
public:
    TramApp();
    virtual int run(int argc, char* argv[]) override;

private:
    std::string stockNumber_;
    std::string depoName_;
    std::string lineName_;
};
#endif //TRAMIMPL_H
