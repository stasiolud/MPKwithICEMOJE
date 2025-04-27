//
// Created by szymo on 27.04.2025.
//

#include "TramImpl.h"
#include <iostream>
#include <algorithm>

TramImpl::TramImpl(const std::string& stockNumber, SIP::MPKPrx mpk)
    : stockNumber_(stockNumber), mpk_(mpk), currentStopIndex_(0), simulationRunning_(false) {}

TramImpl::~TramImpl() {
    stopSimulation();
}

SIP::TramStopPrx TramImpl::getLocation() {
    Ice::Mutex::Lock lock(mutex_);
    return currentStop_;
}

SIP::LinePrx TramImpl::getLine() {
    Ice::Mutex::Lock lock(mutex_);
    return line_;
}

void TramImpl::setLine(SIP::LinePrx line) {
    Ice::Mutex::Lock lock(mutex_);
    line_ = line;

    // Get stops for this line
    if (line_) {
        routeStops_ = line_->getStops();
        if (!routeStops_.empty()) {
            currentStopIndex_ = 0;
            currentStop_ = routeStops_[0].stop;
        }
    }
}

SIP::StopList TramImpl::getNextStops(int howMany) {
    Ice::Mutex::Lock lock(mutex_);

    SIP::StopList result;
    if (routeStops_.empty() || currentStopIndex_ >= routeStops_.size()) {
        return result;
    }

    int maxStops = std::min(howMany, static_cast<int>(routeStops_.size() - currentStopIndex_));
    for (int i = 0; i < maxStops; ++i) {
        int index = (currentStopIndex_ + i) % routeStops_.size();
        result.push_back(routeStops_[index]);
    }

    return result;
}

void TramImpl::RegisterPassenger(SIP::PassengerPrx p) {
    Ice::Mutex::Lock lock(mutex_);

    auto it = std::find_if(passengers_.begin(), passengers_.end(),
        [&p](const SIP::PassengerPrx& passenger) {
            return Ice::proxyIdentityCompare(p, passenger);
        });

    if (it == passengers_.end()) {
        passengers_.push_back(p);

        // Send current tram info to the passenger
        try {
            p->updateTramInfo(SIP::TramPrx::uncheckedCast(this), getNextStops(5));
        }
        catch (const Ice::Exception& e) {
            std::cerr << "Error notifying passenger: " << e << std::endl;
        }
    }
}

void TramImpl::UnregisterPassenger(SIP::PassengerPrx p) {
    Ice::Mutex::Lock lock(mutex_);

    auto it = std::find_if(passengers_.begin(), passengers_.end(),
        [&p](const SIP::PassengerPrx& passenger) {
            return Ice::proxyIdentityCompare(p, passenger);
        });

    if (it != passengers_.end()) {
        passengers_.erase(it);
    }
}

std::string TramImpl::getStockNumber() {
    return stockNumber_;
}

void TramImpl::startSimulation() {
    Ice::Mutex::Lock lock(mutex_);

    if (!simulationRunning_ && line_ && !routeStops_.empty()) {
        simulationRunning_ = true;
        simulationThread_ = std::thread(&TramImpl::simulationLoop, this);
    }
}

void TramImpl::stopSimulation() {
    {
        Ice::Mutex::Lock lock(mutex_);
        simulationRunning_ = false;
    }

    if (simulationThread_.joinable()) {
        simulationThread_.join();
    }
}

void TramImpl::simulationLoop() {
    while (true) {
        {
            Ice::Mutex::Lock lock(mutex_);
            if (!simulationRunning_) {
                break;
            }

            moveToNextStop();
            notifyPassengers();
        }

        // Sleep for some time to simulate tram movement
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void TramImpl::moveToNextStop() {
    if (routeStops_.empty()) {
        return;
    }

    // Move to the next stop
    currentStopIndex_ = (currentStopIndex_ + 1) % routeStops_.size();
    currentStop_ = routeStops_[currentStopIndex_].stop;

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&time_t_now);

    SIP::Time arrivalTime;
    arrivalTime.hour = localTime->tm_hour;
    arrivalTime.minute = localTime->tm_min;

    // Update current stop with tram info
    try {
        currentStop_->UpdateTramInfo(SIP::TramPrx::uncheckedCast(this), arrivalTime);
        std::cout << "Tram " << stockNumber_ << " arrived at "
                  << currentStop_->getName() << " at "
                  << arrivalTime.hour << ":" << arrivalTime.minute << std::endl;
    }
    catch (const Ice::Exception& e) {
        std::cerr << "Error updating tram info at stop: " << e << std::endl;
    }
}

void TramImpl::notifyPassengers() {
    for (const auto& passenger : passengers_) {
        try {
            passenger->updateTramInfo(SIP::TramPrx::uncheckedCast(this), getNextStops(5));
        }
        catch (const Ice::Exception& e) {
            std::cerr << "Error notifying passenger: " << e << std::endl;
        }
    }
}

// TramApp implementation
TramApp::TramApp() {}

int TramApp::run(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <stock-number> <depo-name> <line-name>" << std::endl;
        return 1;
    }

    stockNumber_ = argv[1];
    depoName_ = argv[2];
    lineName_ = argv[3];

    try {
        // Connect to the MPK service
        Ice::ObjectPrx base = communicator()->stringToProxy("MPK:default -p 10000");
        SIP::MPKPrx mpk = SIP::MPKPrx::checkedCast(base);
        if (!mpk) {
            std::cerr << "Invalid MPK proxy" << std::endl;
            return 1;
        }

        // Create tram object
        TramImpl* tramImpl = new TramImpl(stockNumber_, mpk);
        Ice::ObjectPrx tramBase = communicator()->addWithUUID(tramImpl);
        SIP::TramPrx tram = SIP::TramPrx::uncheckedCast(tramBase);

        // Get the depo and register the tram
        SIP::DepoPrx depo = mpk->getDepo(depoName_);
        if (!depo) {
            std::cerr << "Depo " << depoName_ << " not found" << std::endl;
            return 1;
        }

        depo->TramOnline(tram);

        // Get the line and register the tram with it
        SIP::LineList lines = mpk->getLines();
        SIP::LinePrx line;

        for (const auto& l : lines) {
            if (l->getName() == lineName_) {
                line = l;
                break;
            }
        }

        if (!line) {
            std::cerr << "Line " << lineName_ << " not found" << std::endl;
            return 1;
        }

        line->registerTram(tram);

        // Start the simulation
        tramImpl->startSimulation();

        // Run the event loop
        communicator()->waitForShutdown();

        // Clean up before exit
        tramImpl->stopSimulation();
        depo->TramOffline(tram);
        line->unregisterTram(tram);

        return 0;
    }
    catch (const Ice::Exception& e) {
        std::cerr << e << std::endl;
        return 1;
    }
}

int main(int argc, char* argv[]) {
    TramApp app;
    return app.main(argc, argv);
}