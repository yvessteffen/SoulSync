#pragma once

#include <unordered_map>
#include <string>
#include <SDL3/SDL.h>
#include <Network/ReceiveState.hpp>
#include <Network/StreamState.hpp>
#include <UI/ManagedWindow.hpp>

using SenderId = std::string;

enum class WindowId
{
    Emulator,
    Stream1,
    Stream2
};

class WindowManager
{
public:
    bool create(WindowId id, const std::string& title);
    void destroy(WindowId id);
    void destroyAll();

    bool registerSender(const SenderId& id);
    
    StreamState& getOrCreateStream(const SenderId& id);
    StreamState* getStream(const SenderId& id);
    
    ManagedWindow* getWindowBySender(const SenderId& id);
    ManagedWindow* get(WindowId id);

private:
    static constexpr int width = 240;
    static constexpr int height = 160;

    std::unordered_map<WindowId, ManagedWindow> windows;
    std::unordered_map<SenderId, WindowId> senderToWindow;
    std::unordered_map<WindowId, StreamState> streams;
};