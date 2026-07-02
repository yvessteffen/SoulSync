#include <UI/WindowManager.hpp>
#include <SDL3/SDL.h>

bool WindowManager::create(WindowId id, const std::string& title)
{
    if (windows.find(id) != windows.end())
        return true;

    ManagedWindow w;

    w.window = SDL_CreateWindow(
        title.c_str(),
        width * 3,
        height * 3,
        SDL_WINDOW_RESIZABLE
    );

    if (!w.window)
        return false;

    w.renderer = SDL_CreateRenderer(w.window, nullptr);
    if (!w.renderer)
    {
        SDL_DestroyWindow(w.window);
        return false;
    }

    w.texture = SDL_CreateTexture(
        w.renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (!w.texture)
    {
        SDL_DestroyRenderer(w.renderer);
        SDL_DestroyWindow(w.window);
        return false;
    }

    SDL_SetTextureScaleMode(w.texture, SDL_SCALEMODE_NEAREST);

    windows.emplace(id, w);
    return true;
}

void WindowManager::destroyAll()
{
    for (auto& [id, w] : windows)
    {
        if (w.texture) SDL_DestroyTexture(w.texture);
        if (w.renderer) SDL_DestroyRenderer(w.renderer);
        if (w.window) SDL_DestroyWindow(w.window);
    }

    windows.clear();

    for (auto& [id, s] : streams)
        s.decoder.close();

    streams.clear();

    senderToWindow.clear();
}

bool WindowManager::registerSender(const SenderId& id)
{
    if (senderToWindow.find(id) != senderToWindow.end())
        return true;

    if (senderToWindow.size() >= 3)
        return false;

    WindowId target;

    if (windows.find(WindowId::Stream1) == windows.end())
        target = WindowId::Stream1;
    else if (windows.find(WindowId::Stream2) == windows.end())
        target = WindowId::Stream2;
    else
        return false;

    create(target, id + " Stream");

    senderToWindow[id] = target;

    StreamState s;
    s.decoder.open();
    streams[target] = std::move(s);

    return true;
}

StreamState* WindowManager::getStream(const SenderId& id)
{
    auto it = senderToWindow.find(id);
    if (it == senderToWindow.end())
        return nullptr;

    auto sit = streams.find(it->second);
    if (sit == streams.end())
        return nullptr;

    return &sit->second;
}

StreamState& WindowManager::getOrCreateStream(const SenderId& id)
{
    registerSender(id);

    WindowId wid = senderToWindow[id];
    return streams[wid];
}

ManagedWindow* WindowManager::get(WindowId id)
{
    auto it = windows.find(id);
    if (it == windows.end())
        return nullptr;

    return &it->second;
}

ManagedWindow* WindowManager::getWindowBySender(const SenderId& id)
{
    auto it = senderToWindow.find(id);
    if (it == senderToWindow.end())
        return nullptr;

    return get(it->second);
}