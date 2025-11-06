#ifndef ALIVECHECKTIMER_H
#define ALIVECHECKTIMER_H

#include <thread>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <functional>

namespace doip {

using CloseConnectionCallback = std::function<void()>;

class AliveCheckTimer {
public:
    void setTimer(uint16_t seconds);
    void startTimer();
    void resetTimer();

    // Getters
    bool isDisabled() const { return disabled; }
    bool isActive() const { return active; }
    bool hasTimeout() const { return timeout; }
    CloseConnectionCallback getCloseConnectionHandler() const { return closeConnectionHandler; }

    // Setters
    void setDisabled(bool value) { disabled = value; }
    void setActive(bool value) { active = value; }
    void setTimeout(bool value) { timeout = value; }
    void setCloseConnectionHandler(const CloseConnectionCallback& handler) { closeConnectionHandler = handler; }

    ~AliveCheckTimer();

private:
    bool disabled = false;
    bool active = false;
    bool timeout = false;
    CloseConnectionCallback closeConnectionHandler;

    std::vector<std::thread> timerThreads;
    void waitForResponse();
    clock_t startTime{};
    uint16_t maxSeconds{};
};

} // namespace doip

#endif