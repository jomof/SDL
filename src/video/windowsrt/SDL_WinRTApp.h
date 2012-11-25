﻿#pragma once

#include "SDLmain_WinRT_common.h"
#include "SDL_winrtvideo.h"
#include "SDL_winrtrenderer.h"
#include <vector>

using namespace Windows::UI::Core;

ref class SDL_WinRTApp sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
	SDL_WinRTApp();
	
	// IFrameworkView Methods.
	virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
	virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
	virtual void Load(Platform::String^ entryPoint);
	virtual void Run();
	virtual void Uninitialize();

internal:
    // SDL-specific methods
    SDL_DisplayMode GetMainDisplayMode();
    void PumpEvents();
    const SDL_WindowData * GetSDLWindowData() const;
    bool HasSDLWindowData() const;
    void SetSDLWindowData(const SDL_WindowData * windowData);
    void UpdateWindowFramebuffer(SDL_Surface * surface, SDL_Rect * rects, int numrects);
    void ResizeMainTexture(int w, int h);

protected:
	// Event Handlers.
	void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
	void OnLogicalDpiChanged(Platform::Object^ sender);
	void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
	void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
	void OnResuming(Platform::Object^ sender, Platform::Object^ args);
	void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
	void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
	void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
    void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
    void OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);

private:
	SDL_winrtrenderer^ m_renderer;
	bool m_windowClosed;
	bool m_windowVisible;
    const SDL_WindowData* m_sdlWindowData;
};

ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};
