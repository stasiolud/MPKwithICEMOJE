//
// Created by szymo on 27.04.2025.
//

#ifndef MPKIMPL_H
#define MPKIMPL_H

#include <Ice/Ice.h>
#include "../SIP.h"
#include <map>
#include <vector>
#include <string>
#include <memory>

class StopImpl : public SIP::Stop {
public:
    StopImpl(const std::string& name);
    virtual std::string getName() override;

private:
    std::string name_;
};

class TramStopImpl : public SIP::TramStop {
public:
    TramStopImpl(const std::string& name);
    virtual std::string getName() override;
    virtual SIP::TramList getNextTrams(int howMany) override;
    virtual void RegisterPassenger(SIP::PassengerPrx p) override;
    virtual void UnregisterPassenger(SIP::PassengerPrx p) override;
    virtual void UpdateTramInfo(SIP::TramPrx tram, SIP::Time time) override;

private:
    std::string name_;
    std::vector<SIP::TramInfo> tramInfos_;
    std::vector<SIP::PassengerPrx> passengers_;
    Ice::Mutex mutex_;
};

class LineImpl : public SIP::Line {
public:
    LineImpl(const std::string& name);
    virtual SIP::TramList getTrams() override;
    virtual SIP::StopList getStops() override;
    virtual void registerTram(SIP::TramPrx tram) override;
    virtual void unregisterTram(SIP::TramPrx tram) override;
    virtual void setStops(const SIP::StopList& sl) override;
    virtual std::string getName() override;

private:
    std::string name_;
    SIP::StopList stops_;
    std::vector<SIP::TramPrx> trams_;
    Ice::Mutex mutex_;
};

class DepoImpl : public SIP::Depo {
public:
    DepoImpl(const std::string& name);
    virtual void TramOnline(SIP::TramPrx t) override;
    virtual void TramOffline(SIP::TramPrx t) override;
    virtual std::string getName() override;

private:
    std::string name_;
    std::vector<SIP::TramPrx> trams_;
    Ice::Mutex mutex_;
};

class LineFactoryImpl : public SIP::LineFactory {
public:
    virtual SIP::LinePrx createLine(const std::string& name) override;
    virtual double getLoad() override;

private:
    int createdLinesCount_ = 0;
};

class StopFactoryImpl : public SIP::StopFactory {
public:
    virtual SIP::StopPrx createStop(const std::string& name) override;
    virtual double getLoad() override;

private:
    int createdStopsCount_ = 0;
};

class MPKImpl : public SIP::MPK {
public:
    MPKImpl();

    virtual SIP::TramStopPrx getTramStop(const std::string& name) override;
    virtual void registerDepo(SIP::DepoPrx depo) override;
    virtual void unregisterDepo(SIP::DepoPrx depo) override;
    virtual SIP::DepoPrx getDepo(const std::string& name) override;
    virtual SIP::DepoList getDepos() override;
    virtual SIP::LineList getLines() override;
    virtual void registerLineFactory(SIP::LineFactoryPrx lf) override;
    virtual void unregisterLineFactory(SIP::LineFactoryPrx lf) override;
    virtual void registerStopFactory(SIP::StopFactoryPrx lf) override;
    virtual void unregisterStopFactory(SIP::StopFactoryPrx lf) override;

private:
    std::map<std::string, SIP::TramStopPrx> tramStops_;
    std::map<std::string, SIP::DepoPrx> depos_;
    std::vector<SIP::LineFactoryPrx> lineFactories_;
    std::vector<SIP::StopFactoryPrx> stopFactories_;
    std::vector<SIP::LinePrx> lines_;
    Ice::Mutex mutex_;
};

#endif //MPKIMPL_H
