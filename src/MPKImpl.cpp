//
// Created by szymo on 27.04.2025.
//

#include "MPKImpl.h"
#include <algorithm>
#include <iostream>

// StopImpl implementation
StopImpl::StopImpl(const std::string& name) : name_(name) {}

std::string StopImpl::getName() {
    return name_;
}

// TramStopImpl implementation
TramStopImpl::TramStopImpl(const std::string& name) : name_(name) {}

std::string TramStopImpl::getName() {
    return name_;
}

SIP::TramList TramStopImpl::getNextTrams(int howMany) {
    Ice::Mutex::Lock lock(mutex_);

    SIP::TramList result;
    int count = std::min(howMany, static_cast<int>(tramInfos_.size()));

    // Sort by arrival time
    std::sort(tramInfos_.begin(), tramInfos_.end(),
        [](const SIP::TramInfo& a, const SIP::TramInfo& b) {
            if (a.time.hour == b.time.hour)
                return a.time.minute < b.time.minute;
            return a.time.hour < b.time.hour;
        });

    for (int i = 0; i < count; ++i) {
        result.push_back(tramInfos_[i]);
    }

    return result;
}

void TramStopImpl::RegisterPassenger(SIP::PassengerPrx p) {
    Ice::Mutex::Lock lock(mutex_);

    // Check if passenger is already registered
    auto it = std::find_if(passengers_.begin(), passengers_.end(),
        [&p](const SIP::PassengerPrx& passenger) {
            return Ice::proxyIdentityCompare(p, passenger);
        });

    if (it == passengers_.end()) {
        passengers_.push_back(p);

        // Send current tram info to the passenger
        try {
            p->updateStopInfo(SIP::StopPrx::uncheckedCast(this), getNextTrams(5));
        }
        catch (const Ice::Exception& e) {
            std::cerr << "Error notifying passenger: " << e << std::endl;
        }
    }
}

void TramStopImpl::UnregisterPassenger(SIP::PassengerPrx p) {
    Ice::Mutex::Lock lock(mutex_);

    auto it = std::find_if(passengers_.begin(), passengers_.end(),
        [&p](const SIP::PassengerPrx& passenger) {
            return Ice::proxyIdentityCompare(p, passenger);
        });

    if (it != passengers_.end()) {
        passengers_.erase(it);
    }
}

void TramStopImpl::UpdateTramInfo(SIP::TramPrx tram, SIP::Time time) {
    Ice::Mutex::Lock lock(mutex_);

    // Update or add tram info
    SIP::TramInfo info;
    info.time = time;
    info.tram = tram;

    auto it = std::find_if(tramInfos_.begin(), tramInfos_.end(),
        [&tram](const SIP::TramInfo& info) {
            return Ice::proxyIdentityCompare(info.tram, tram);
        });

    if (it != tramInfos_.end()) {
        *it = info;
    } else {
        tramInfos_.push_back(info);
    }

    // Notify all registered passengers
    for (const auto& passenger : passengers_) {
        try {
            passenger->updateStopInfo(SIP::StopPrx::uncheckedCast(this), getNextTrams(5));
        }
        catch (const Ice::Exception& e) {
            std::cerr << "Error notifying passenger: " << e << std::endl;
        }
    }
}

// LineImpl implementation
LineImpl::LineImpl(const std::string& name) : name_(name) {}

SIP::TramList LineImpl::getTrams() {
    Ice::Mutex::Lock lock(mutex_);

    // Convert trams list to TramList format required by interface
    SIP::TramList result;
    for (const auto& tram : trams_) {
        SIP::TramInfo info;
        info.tram = tram;
        info.time = {0, 0}; // Default time
        result.push_back(info);
    }

    return result;
}

SIP::StopList LineImpl::getStops() {
    Ice::Mutex::Lock lock(mutex_);
    return stops_;
}

void LineImpl::registerTram(SIP::TramPrx tram) {
    Ice::Mutex::Lock lock(mutex_);

    auto it = std::find_if(trams_.begin(), trams_.end(),
        [&tram](const SIP::TramPrx& t) {
            return Ice::proxyIdentityCompare(t, tram);
        });

    if (it == trams_.end()) {
        trams_.push_back(tram);
        try {
            tram->setLine(SIP::LinePrx::uncheckedCast(this));
        }
        catch (const Ice::Exception& e) {
            std::cerr << "Error setting line for tram: " << e << std::endl;
        }
    }
}

void LineImpl::unregisterTram(SIP::TramPrx tram) {
    Ice::Mutex::Lock lock(mutex_);

    auto it = std::find_if(trams_.begin(), trams_.end(),
        [&tram](const SIP::TramPrx& t) {
            return Ice::proxyIdentityCompare(t, tram);
        });

    if (it != trams_.end()) {
        trams_.erase(it);
    }
}

void LineImpl::setStops(const SIP::StopList& sl) {
    Ice::Mutex::Lock lock(mutex_);
    stops_ = sl;
}

std::string LineImpl::getName() {
    return name_;
}

// DepoImpl implementation
DepoImpl::DepoImpl(const std::string& name) : name_(name) {}

void DepoImpl::TramOnline(SIP::TramPrx t) {
    Ice::Mutex::Lock lock(mutex_);

    auto it = std::find_if(trams_.begin(), trams_.end(),
        [&t](const SIP::TramPrx& tram) {
            return Ice::proxyIdentityCompare(t, tram);
        });

    if (it == trams_.end()) {
        trams_.push_back(t);
        std::cout << "Tram " << t->getStockNumber() << " online at depo " << name_ << std::endl;
    }
}

void DepoImpl::TramOffline(SIP::TramPrx t) {
    Ice::Mutex::Lock lock(mutex_);

    auto it = std::find_if(trams_.begin(), trams_.end(),
        [&t](const SIP::TramPrx& tram) {
            return Ice::proxyIdentityCompare(t, tram);
        });

    if (it != trams_.end()) {
        trams_.erase(it);
        std::cout << "Tram " << t->getStockNumber() << " offline at depo " << name_ << std::endl;
    }
}

std::string DepoImpl::getName() {
    return name_;
}

// LineFactoryImpl implementation
SIP::LinePrx LineFactoryImpl::createLine(const std::string& name) {
    LineImpl* lineImpl = new LineImpl(name);
    Ice::ObjectPrx base = Ice::Application::communicator()->addWithUUID(lineImpl);
    SIP::LinePrx line = SIP::LinePrx::uncheckedCast(base);
    createdLinesCount_++;
    return line;
}

double LineFactoryImpl::getLoad() {
    return createdLinesCount_;
}

// StopFactoryImpl implementation
SIP::StopPrx StopFactoryImpl::createStop(const std::string& name) {
    StopImpl* stopImpl = new StopImpl(name);
    Ice::ObjectPrx base = Ice::Application::communicator()->addWithUUID(stopImpl);
    SIP::StopPrx stop = SIP::StopPrx::uncheckedCast(base);
    createdStopsCount_++;
    return stop;
}

double StopFactoryImpl::getLoad() {
    return createdStopsCount_;
}

// MPKImpl implementation
MPKImpl::MPKImpl() {}

SIP::TramStopPrx MPKImpl::getTramStop(const std::string& name) {
    Ice::Mutex::Lock lock(mutex_);

    auto it = tramStops_.find(name);
    if (it != tramStops_.end()) {
        return it->second;
    }

    // Create new tram stop if not found
    TramStopImpl* stopImpl = new TramStopImpl(name);
    Ice::ObjectPrx base = Ice::Application::communicator()->addWithUUID(stopImpl);
    SIP::TramStopPrx stop = SIP::TramStopPrx::uncheckedCast(base);
    tramStops_[name] = stop;

    return stop;
}

void MPKImpl::registerDepo(SIP::DepoPrx depo) {
    Ice::Mutex::Lock lock(mutex_);
    depos_[depo->getName()] = depo;
}

void MPKImpl::unregisterDepo(SIP::DepoPrx depo) {
    Ice::Mutex::Lock lock(mutex_);
    depos_.erase(depo->getName());
}

SIP::DepoPrx MPKImpl::getDepo(const std::string& name) {
    Ice::Mutex::Lock lock(mutex_);
    auto it = depos_.find(name);
    return (it != depos_.end()) ? it->second : SIP::DepoPrx();
}

SIP::DepoList MPKImpl::getDepos() {
    Ice::Mutex::Lock lock(mutex_);

    SIP::DepoList result;
    for (const auto& pair : depos_) {
        SIP::DepoInfo info;
        info.name = pair.first;
        info.stop = pair.second;
        result.push_back(info);
    }

    return result;
}

SIP::LineList MPKImpl::getLines() {
    Ice::Mutex::Lock lock(mutex_);
    return lines_;
}

void MPKImpl::registerLineFactory(SIP::LineFactoryPrx lf) {
    Ice::Mutex::Lock lock(mutex_);
    lineFactories_.push_back(lf);
}

void MPKImpl::unregisterLineFactory(SIP::LineFactoryPrx lf) {
    Ice::Mutex::Lock lock(mutex_);

    auto it = std::find_if(lineFactories_.begin(), lineFactories_.end(),
        [&lf](const SIP::LineFactoryPrx& factory) {
            return Ice::proxyIdentityCompare(factory, lf);
        });

    if (it != lineFactories_.end()) {
        lineFactories_.erase(it);
    }
}

void MPKImpl::registerStopFactory(SIP::StopFactoryPrx sf) {
    Ice::Mutex::Lock lock(mutex_);
    stopFactories_.push_back(sf);
}

void MPKImpl::unregisterStopFactory(SIP::StopFactoryPrx sf) {
    Ice::Mutex::Lock lock(mutex_);

    auto it = std::find_if(stopFactories_.begin(), stopFactories_.end(),
        [&sf](const SIP::StopFactoryPrx& factory) {
            return Ice::proxyIdentityCompare(factory, sf);
        });

    if (it != stopFactories_.end()) {
        stopFactories_.erase(it);
    }
}