#include "PassengerImpl.h"
#include <iostream>
#include <sstream>
#include <algorithm>

PassengerImpl::PassengerImpl(const std::string& name) : name_(name) {}

void PassengerImpl::updateTramInfo(SIP::TramPrx tram, const SIP::StopList& stops) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string tramNumber = tram->getStockNumber();
    tramInfos_[tramNumber] = stops;

    std::cout << "\nReceived update for tram " << tramNumber << std::endl;
}
void PassengerImpl::updateStopInfo(SIP::StopPrx stop, const SIP::TramList& trams) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string stopName = stop->getName();
    stopInfos_[stopName] = trams;

    std::cout << "\nReceived update for stop " << stopName << std::endl;
}

void PassengerImpl::watchStop(SIP::TramStopPrx stop) {
    std::string stopName = stop->getName();

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = std::find_if(watchedStops_.begin(), watchedStops_.end(),
            [&stop](const SIP::TramStopPrx& s) {
                return Ice::proxyIdentityCompare(s, stop);
            });

        if (it == watchedStops_.end()) {
            watchedStops_.push_back(stop);
            std::cout << "Now watching stop: " << stopName << std::endl;
        }
    }

    try {
        stop->RegisterPassenger(SIP::PassengerPrx::uncheckedCast(this));
    }
    catch (const Ice::Exception& e) {
        std::cerr << "Error registering with stop " << stopName << ": " << e << std::endl;
    }
}

void PassengerImpl::unwatchStop(SIP::TramStopPrx stop) {
    std::string stopName = stop->getName();

    try {
        stop->UnregisterPassenger(SIP::PassengerPrx::uncheckedCast(this));
    }
    catch (const Ice::Exception& e) {
        std::cerr << "Error unregistering from stop " << stopName << ": " << e << std::endl;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(watchedStops_.begin(), watchedStops_.end(),
        [&stop](const SIP::TramStopPrx& s) {
            return Ice::proxyIdentityCompare(s, stop);
        });

    if (it != watchedStops_.end()) {
        watchedStops_.erase(it);
        std::cout << "Stopped watching stop: " << stopName << std::endl;
    }
}

void PassengerImpl::watchTram(SIP::TramPrx tram) {
    std::string tramNumber = tram->getStockNumber();

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = std::find_if(watchedTrams_.begin(), watchedTrams_.end(),
            [&tram](const SIP::TramPrx& t) {
                return Ice::proxyIdentityCompare(t, tram);
            });

        if (it == watchedTrams_.end()) {
            watchedTrams_.push_back(tram);
            std::cout << "Now watching tram: " << tramNumber << std::endl;
        }
    }

    try {
        tram->RegisterPassenger(SIP::PassengerPrx::uncheckedCast(this));
    }
    catch (const Ice::Exception& e) {
        std::cerr << "Error registering with tram " << tramNumber << ": " << e << std::endl;
    }
}

void PassengerImpl::unwatchTram(SIP::TramPrx tram) {
    std::string tramNumber = tram->getStockNumber();

    try {
        tram->UnregisterPassenger(SIP::PassengerPrx::uncheckedCast(this));
    }
    catch (const Ice::Exception& e) {
        std::cerr << "Error unregistering from tram " << tramNumber << ": " << e << std::endl;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(watchedTrams_.begin(), watchedTrams_.end(),
        [&tram](const SIP::TramPrx& t) {
            return Ice::proxyIdentityCompare(t, tram);
        });

    if (it != watchedTrams_.end()) {
        watchedTrams_.erase(it);
        std::cout << "Stopped watching tram: " << tramNumber << std::endl;
    }
}

void PassengerImpl::displayTramInfo() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::cout << "\n===== Tram Information =====\n";

    if (tramInfos_.empty()) {
        std::cout << "No tram information available.\n";
        return;
    }

    for (const auto& pair : tramInfos_) {
        std::cout << "Tram: " << pair.first << std::endl;
        std::cout << "Next stops:\n";

        for (const auto& stopInfo : pair.second) {
            std::cout << "  " << stopInfo.stop->getName()
                      << " at " << stopInfo.time.hour << ":"
                      << (stopInfo.time.minute < 10 ? "0" : "") << stopInfo.time.minute << std::endl;
        }

        std::cout << std::endl;
    }
}

void PassengerImpl::displayStopInfo() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::cout << "\n===== Stop Information =====\n";

    if (stopInfos_.empty()) {
        std::cout << "No stop information available.\n";
        return;
    }

    for (const auto& pair : stopInfos_) {
        std::cout << "Stop: " << pair.first << std::endl;
        std::cout << "Expected trams:\n";

        for (const auto& tramInfo : pair.second) {
            std::cout << "  Tram " << tramInfo.tram->getStockNumber()
                      << " at " << tramInfo.time.hour << ":"
                      << (tramInfo.time.minute < 10 ? "0" : "") << tramInfo.time.minute << std::endl;
        }

        std::cout << std::endl;
    }
}

// PassengerApp implementation
PassengerApp::PassengerApp() {}

void PassengerApp::showHelp() {
    std::cout << "\nAvailable commands:\n"
              << "  help - Show this help message\n"
              << "  watch stop <name> - Start watching a tram stop\n"
              << "  unwatch stop <name> - Stop watching a tram stop\n"
              << "  watch tram <number> - Start watching a tram\n"
              << "  unwatch tram <number> - Stop watching a tram\n"
              << "  list stops - List all stops in the system\n"
              << "  list trams [line] - List all trams (optionally for a specific line)\n"
              << "  show stops - Display information about watched stops\n"
              << "  show trams - Display information about watched trams\n"
              << "  exit - Exit the application\n";
}

void PassengerApp::processCommands(PassengerImpl* passengerImpl, SIP::MPKPrx mpk) {
    std::string line;

    showHelp();

    while (std::cout << "\nEnter command: " && std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "help") {
            showHelp();
        }
        else if (command == "exit") {
            break;
        }
        else if (command == "watch") {
            std::string type;
            iss >> type;

            if (type == "stop") {
                std::string name;
                iss >> name;

                try {
                    SIP::TramStopPrx stop = mpk->getTramStop(name);
                    passengerImpl->watchStop(stop);
                }
                catch (const Ice::Exception& e) {
                    std::cerr << "Error: " << e << std::endl;
                }
            }
            else if (type == "tram") {
                std::string number;
                iss >> number;

                try {
                    // Find the tram with the given number
                    SIP::LineList lines = mpk->getLines();
                    SIP::TramPrx foundTram;

                    for (const auto& line : lines) {
                        SIP::TramList trams = line->getTrams();
                        for (const auto& tramInfo : trams) {
                            try {
                                if (tramInfo.tram->getStockNumber() == number) {
                                    foundTram = tramInfo.tram;
                                    break;
                                }
                            }
                            catch (const Ice::Exception& e) {
                                // Skip unresponsive trams
                            }
                        }

                        if (foundTram) {
                            break;
                        }
                    }

                    if (foundTram) {
                        passengerImpl->watchTram(foundTram);
                    }
                    else {
                        std::cout << "Tram " << number << " not found." << std::endl;
                    }
                }
                catch (const Ice::Exception& e) {
                    std::cerr << "Error: " << e << std::endl;
                }
            }
            else {
                std::cout << "Unknown watch type. Use 'watch stop <name>' or 'watch tram <number>'." << std::endl;
            }
        }
        else if (command == "unwatch") {
            std::string type;
            iss >> type;

            if (type == "stop") {
                std::string name;
                iss >> name;

                try {
                    SIP::TramStopPrx stop = mpk->getTramStop(name);
                    passengerImpl->unwatchStop(stop);
                }
                catch (const Ice::Exception& e) {
                    std::cerr << "Error: " << e << std::endl;
                }
            }
            else if (type == "tram") {
                std::string number;
                iss >> number;

                try {
                    // Find the tram with the given number
                    SIP::LineList lines = mpk->getLines();
                    SIP::TramPrx foundTram;

                    for (const auto& line : lines) {
                        SIP::TramList trams = line->getTrams();
                        for (const auto& tramInfo : trams) {
                            try {
                                if (tramInfo.tram->getStockNumber() == number) {
                                    foundTram = tramInfo.tram;
                                    break;
                                }
                            }
                            catch (const Ice::Exception& e) {
                                // Skip unresponsive trams
                            }
                        }

                        if (foundTram) {
                            break;
                        }
                    }

                    if (foundTram) {
                        passengerImpl->unwatchTram(foundTram);
                    }
                    else {
                        std::cout << "Tram " << number << " not found." << std::endl;
                    }
                }
                catch (const Ice::Exception& e) {
                    std::cerr << "Error: " << e << std::endl;
                }
            }
            else {
                std::cout << "Unknown unwatch type. Use 'unwatch stop <name>' or 'unwatch tram <number>'." << std::endl;
            }
        }
        else if (command == "list") {
            std::string type;
            iss >> type;

            if (type == "stops") {
                try {
                    SIP::LineList lines = mpk->getLines();
                    std::set<std::string> stopNames;

                    for (const auto& line : lines) {
                        SIP::StopList stops = line->getStops();
                        for (const auto& stopInfo : stops) {
                            try {
                                stopNames.insert(stopInfo.stop->getName());
                            }
                            catch (const Ice::Exception& e) {
                                // Skip unresponsive stops
                            }
                        }
                    }

                    std::cout << "\nAvailable stops:" << std::endl;
                    for (const auto& name : stopNames) {
                        std::cout << "  " << name << std::endl;
                    }
                }
                catch (const Ice::Exception& e) {
                    std::cerr << "Error: " << e << std::endl;
                }
            }
            else if (type == "trams") {
                std::string lineName;
                iss >> lineName;

                try {
                    SIP::LineList lines = mpk->getLines();

                    std::cout << "\nAvailable trams:" << std::endl;
                    for (const auto& line : lines) {
                        std::string currentLineName = line->getName();

                        if (!lineName.empty() && lineName != currentLineName) {
                            continue;
                        }

                        std::cout << "Line " << currentLineName << ":" << std::endl;

                        SIP::TramList trams = line->getTrams();
                        if (trams.empty()) {
                            std::cout << "  No trams on this line." << std::endl;
                            continue;
                        }

                        for (const auto& tramInfo : trams) {
                            try {
                                std::cout << "  Tram " << tramInfo.tram->getStockNumber() << std::endl;
                            }
                            catch (const Ice::Exception& e) {
                                // Skip unresponsive trams
                            }
                        }
                    }
                }
                catch (const Ice::Exception& e) {
                    std::cerr << "Error: " << e << std::endl;
                }
            }
            else {
                std::cout << "Unknown list type. Use 'list stops' or 'list trams [line]'." << std::endl;
            }
        }
        else if (command == "show") {
            std::string type;
            iss >> type;

            if (type == "stops") {
                passengerImpl->displayStopInfo();
            }
            else if (type == "trams") {
                passengerImpl->displayTramInfo();
            }
            else {
                std::cout << "Unknown show type. Use 'show stops' or 'show trams'." << std::endl;
            }
        }
        else {
            std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
        }
    }
}

int PassengerApp::run(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <passenger-name>" << std::endl;
        return 1;
    }

    std::string passengerName = argv[1];

    try {
        // Connect to the MPK service
        Ice::ObjectPrx base = communicator()->stringToProxy("MPK:default -p 10000");
        SIP::MPKPrx mpk = SIP::MPKPrx::checkedCast(base);
        if (!mpk) {
            std::cerr << "Invalid MPK proxy" << std::endl;
            return 1;
        }

        // Create passenger object
        PassengerImpl* passengerImpl = new PassengerImpl(passengerName);
        Ice::ObjectPrx passengerBase = communicator()->addWithUUID(passengerImpl);
        SIP::PassengerPrx passenger = SIP::PassengerPrx::uncheckedCast(passengerBase);

        // Start the adapter
        Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("Passenger");
        adapter->add(passengerImpl, Ice::stringToIdentity("passenger"));
        adapter->activate();

        std::cout << "Passenger " << passengerName << " is online." << std::endl;

        // Process user commands
        processCommands(passengerImpl, mpk);

        return 0;
    }
    catch (const Ice::Exception& e) {
        std::cerr << e << std::endl;
        return 1;
    }
}

int main(int argc, char* argv[]) {
    PassengerApp app;
    return app.main(argc, argv);
}