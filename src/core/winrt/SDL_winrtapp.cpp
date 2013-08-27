﻿
#include <functional>
#include <string>
#include <sstream>

#include "ppltasks.h"

extern "C" {
#include "SDL_assert.h"
#include "SDL_events.h"
#include "SDL_hints.h"
#include "SDL_log.h"
#include "SDL_main.h"
#include "SDL_stdinc.h"
#include "SDL_render.h"
#include "../../video/SDL_sysvideo.h"
//#include "../../SDL_hints_c.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_windowevents_c.h"
#include "../../render/SDL_sysrender.h"
}

#include "../../video/winrt/SDL_winrtevents_c.h"
#include "../../video/winrt/SDL_winrtvideo.h"
#include "SDL_winrtapp.h"

using namespace concurrency;
using namespace std;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Devices::Input;
using namespace Windows::Graphics::Display;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;

// Compile-time debugging options:
// To enable, uncomment; to disable, comment them out.
//#define LOG_POINTER_EVENTS 1
//#define LOG_WINDOW_EVENTS 1
//#define LOG_ORIENTATION_EVENTS 1


// HACK, DLudwig: The C-style main() will get loaded via the app's
// WinRT-styled main(), which is part of SDLmain_for_WinRT.cpp.
// This seems wrong on some level, but does seem to work.
typedef int (*SDL_WinRT_MainFunction)(int, char **);
static SDL_WinRT_MainFunction SDL_WinRT_main = nullptr;

// HACK, DLudwig: record a reference to the global, Windows RT 'app'/view.
// SDL/WinRT will use this throughout its code.
//
// TODO, WinRT: consider replacing SDL_WinRTGlobalApp with something
// non-global, such as something created inside
// SDL_InitSubSystem(SDL_INIT_VIDEO), or something inside
// SDL_CreateWindow().
SDL_WinRTApp ^ SDL_WinRTGlobalApp = nullptr;

ref class SDLApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};

IFrameworkView^ SDLApplicationSource::CreateView()
{
    // TODO, WinRT: see if this function (CreateView) can ever get called
    // more than once.  For now, just prevent it from ever assigning
    // SDL_WinRTGlobalApp more than once.
    SDL_assert(!SDL_WinRTGlobalApp);
    SDL_WinRTApp ^ app = ref new SDL_WinRTApp();
    if (!SDL_WinRTGlobalApp)
    {
        SDL_WinRTGlobalApp = app;
    }
    return app;
}

__declspec(dllexport) int SDL_WinRT_RunApplication(SDL_WinRT_MainFunction mainFunction)
{
    SDL_WinRT_main = mainFunction;
    auto direct3DApplicationSource = ref new SDLApplicationSource();
    CoreApplication::Run(direct3DApplicationSource);
    return 0;
}

static void WINRT_SetDisplayOrientationsPreference(void *userdata, const char *name, const char *oldValue, const char *newValue)
{
    SDL_assert(SDL_strcmp(name, SDL_HINT_ORIENTATIONS) == 0);

    // Start with no orientation flags, then add each in as they're parsed
    // from newValue.
    unsigned int orientationFlags = 0;
    if (newValue) {
        std::istringstream tokenizer(newValue);
        while (!tokenizer.eof()) {
            std::string orientationName;
            std::getline(tokenizer, orientationName, ' ');
            if (orientationName == "LandscapeLeft") {
                orientationFlags |= (unsigned int) DisplayOrientations::LandscapeFlipped;
            } else if (orientationName == "LandscapeRight") {
                orientationFlags |= (unsigned int) DisplayOrientations::Landscape;
            } else if (orientationName == "Portrait") {
                orientationFlags |= (unsigned int) DisplayOrientations::Portrait;
            } else if (orientationName == "PortraitUpsideDown") {
                orientationFlags |= (unsigned int) DisplayOrientations::PortraitFlipped;
            }
        }
    }

    // If no valid orientation flags were specified, use a reasonable set of defaults:
    if (!orientationFlags) {
        // TODO, WinRT: consider seeing if an app's default orientation flags can be found out via some API call(s).
        orientationFlags = (unsigned int) ( \
            DisplayOrientations::Landscape |
            DisplayOrientations::LandscapeFlipped |
            DisplayOrientations::Portrait |
            DisplayOrientations::PortraitFlipped);
    }

    // Set the orientation/rotation preferences.  Please note that this does
    // not constitute a 100%-certain lock of a given set of possible
    // orientations.  According to Microsoft's documentation on Windows RT [1]
    // when a device is not capable of being rotated, Windows may ignore
    // the orientation preferences, and stick to what the device is capable of
    // displaying.
    //
    // [1] Documentation on the 'InitialRotationPreference' setting for a
    // Windows app's manifest file describes how some orientation/rotation
    // preferences may be ignored.  See
    // http://msdn.microsoft.com/en-us/library/windows/apps/hh700343.aspx
    // for details.  Microsoft's "Display orientation sample" also gives an
    // outline of how Windows treats device rotation
    // (http://code.msdn.microsoft.com/Display-Orientation-Sample-19a58e93).
    DisplayProperties::AutoRotationPreferences = (DisplayOrientations) orientationFlags;
}

SDL_WinRTApp::SDL_WinRTApp() :
    m_windowClosed(false),
    m_windowVisible(true),
    m_sdlWindowData(NULL),
    m_sdlVideoDevice(NULL)
{
}

void SDL_WinRTApp::Initialize(CoreApplicationView^ applicationView)
{
    applicationView->Activated +=
        ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &SDL_WinRTApp::OnActivated);

    CoreApplication::Suspending +=
        ref new EventHandler<SuspendingEventArgs^>(this, &SDL_WinRTApp::OnSuspending);

    CoreApplication::Resuming +=
        ref new EventHandler<Platform::Object^>(this, &SDL_WinRTApp::OnResuming);

    DisplayProperties::OrientationChanged +=
        ref new DisplayPropertiesEventHandler(this, &SDL_WinRTApp::OnOrientationChanged);

    // Register the hint, SDL_HINT_ORIENTATIONS, with SDL.  This needs to be
    // done before the hint's callback is registered (as of Feb 22, 2013),
    // otherwise the hint callback won't get registered.
    //
    // WinRT, TODO: see if an app's default orientation can be found out via WinRT API(s), then set the initial value of SDL_HINT_ORIENTATIONS accordingly.
    //SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight Portrait PortraitUpsideDown");   // DavidL: this is no longer needed (for SDL_AddHintCallback)
    SDL_AddHintCallback(SDL_HINT_ORIENTATIONS, WINRT_SetDisplayOrientationsPreference, NULL);
}

void SDL_WinRTApp::OnOrientationChanged(Object^ sender)
{
#if LOG_ORIENTATION_EVENTS==1
    CoreWindow^ window = CoreWindow::GetForCurrentThread();
    if (window) {
        SDL_Log("%s, current orientation=%d, native orientation=%d, auto rot. pref=%d, CoreWindow Size={%f,%f}\n",
            __FUNCTION__,
            (int)DisplayProperties::CurrentOrientation,
            (int)DisplayProperties::NativeOrientation,
            (int)DisplayProperties::AutoRotationPreferences,
            window->Bounds.Width,
            window->Bounds.Height);
    } else {
        SDL_Log("%s, current orientation=%d, native orientation=%d, auto rot. pref=%d\n",
            __FUNCTION__,
            (int)DisplayProperties::CurrentOrientation,
            (int)DisplayProperties::NativeOrientation,
            (int)DisplayProperties::AutoRotationPreferences);
    }
#endif
}

void SDL_WinRTApp::SetWindow(CoreWindow^ window)
{
#if LOG_WINDOW_EVENTS==1
    SDL_Log("%s, current orientation=%d, native orientation=%d, auto rot. pref=%d, window Size={%f,%f}\n",
        __FUNCTION__,
        (int)DisplayProperties::CurrentOrientation,
        (int)DisplayProperties::NativeOrientation,
        (int)DisplayProperties::AutoRotationPreferences,
        window->Bounds.Width,
        window->Bounds.Height);
#endif

    window->SizeChanged += 
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &SDL_WinRTApp::OnWindowSizeChanged);

    window->VisibilityChanged +=
        ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &SDL_WinRTApp::OnVisibilityChanged);

    window->Closed += 
        ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &SDL_WinRTApp::OnWindowClosed);

#if WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP
    window->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);
#endif

    window->PointerPressed +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerPressed);

    window->PointerReleased +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerReleased);

    window->PointerWheelChanged +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerWheelChanged);

    window->PointerMoved +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerMoved);

#if WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP
    // Retrieves relative-only mouse movements:
    Windows::Devices::Input::MouseDevice::GetForCurrentView()->MouseMoved +=
        ref new TypedEventHandler<MouseDevice^, MouseEventArgs^>(this, &SDL_WinRTApp::OnMouseMoved);
#endif

    window->KeyDown +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &SDL_WinRTApp::OnKeyDown);

    window->KeyUp +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &SDL_WinRTApp::OnKeyUp);
}

void SDL_WinRTApp::Load(Platform::String^ entryPoint)
{
}

void SDL_WinRTApp::Run()
{
    SDL_SetMainReady();
    if (SDL_WinRT_main)
    {
        // TODO, WinRT: pass the C-style main() a reasonably realistic
        // representation of command line arguments.
        int argc = 0;
        char **argv = NULL;
        SDL_WinRT_main(argc, argv);
    }
}

void SDL_WinRTApp::PumpEvents()
{
    if (!m_windowClosed)
    {
        if (m_windowVisible)
        {
            CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
        }
        else
        {
            CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
        }
    }
}

void SDL_WinRTApp::Uninitialize()
{
}

void SDL_WinRTApp::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
#if LOG_WINDOW_EVENTS==1
    SDL_Log("%s, size={%f,%f}, current orientation=%d, native orientation=%d, auto rot. pref=%d, m_sdlWindowData?=%s\n",
        __FUNCTION__,
        args->Size.Width, args->Size.Height,
        (int)DisplayProperties::CurrentOrientation,
        (int)DisplayProperties::NativeOrientation,
        (int)DisplayProperties::AutoRotationPreferences,
        (m_sdlWindowData ? "yes" : "no"));
#endif

    if (m_sdlWindowData) {
        // Make the new window size be the one true fullscreen mode.
        // This change was initially done, in part, to allow the Direct3D 11.1
        // renderer to receive window-resize events as a device rotates.
        // Before, rotating a device from landscape, to portrait, and then
        // back to landscape would cause the Direct3D 11.1 swap buffer to
        // not get resized appropriately.  SDL would, on the rotation from
        // landscape to portrait, re-resize the SDL window to it's initial
        // size (landscape).  On the subsequent rotation, SDL would drop the
        // window-resize event as it appeared the SDL window didn't change
        // size, and the Direct3D 11.1 renderer wouldn't resize its swap
        // chain.
        SDL_DisplayMode resizedDisplayMode = CalcCurrentDisplayMode();
        m_sdlVideoDevice->displays[0].current_mode = resizedDisplayMode;
        m_sdlVideoDevice->displays[0].desktop_mode = resizedDisplayMode;
        m_sdlVideoDevice->displays[0].display_modes[0] = resizedDisplayMode;

        // Send the window-resize event to the rest of SDL, and to apps:
        const int windowWidth = (int) ceil(args->Size.Width);
        const int windowHeight = (int) ceil(args->Size.Height);
        SDL_SendWindowEvent(
            m_sdlWindowData->sdlWindow,
            SDL_WINDOWEVENT_RESIZED,
            windowWidth,
            windowHeight);
    }
}

void SDL_WinRTApp::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
#if LOG_WINDOW_EVENTS==1
    SDL_Log("%s, visible?=%s, m_sdlWindowData?=%s\n",
        __FUNCTION__,
        (args->Visible ? "yes" : "no"),
        (m_sdlWindowData ? "yes" : "no"));
#endif

    m_windowVisible = args->Visible;
    if (m_sdlWindowData) {
        SDL_bool wasSDLWindowSurfaceValid = m_sdlWindowData->sdlWindow->surface_valid;

        if (args->Visible) {
            SDL_SendWindowEvent(m_sdlWindowData->sdlWindow, SDL_WINDOWEVENT_SHOWN, 0, 0);
        } else {
            SDL_SendWindowEvent(m_sdlWindowData->sdlWindow, SDL_WINDOWEVENT_HIDDEN, 0, 0);
        }

        // HACK: Prevent SDL's window-hide handling code, which currently
        // triggers a fake window resize (possibly erronously), from
        // marking the SDL window's surface as invalid.
        //
        // A better solution to this probably involves figuring out if the
        // fake window resize can be prevented.
        m_sdlWindowData->sdlWindow->surface_valid = wasSDLWindowSurfaceValid;
    }
}

void SDL_WinRTApp::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
#if LOG_WINDOW_EVENTS==1
    SDL_Log("%s\n", __FUNCTION__);
#endif
    m_windowClosed = true;
}

void SDL_WinRTApp::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
    SDL_Window * window = (m_sdlWindowData ? m_sdlWindowData->sdlWindow : nullptr);
    WINRT_ProcessPointerPressedEvent(window, args);
}

void SDL_WinRTApp::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
    SDL_Window * window = (m_sdlWindowData ? m_sdlWindowData->sdlWindow : nullptr);
    WINRT_ProcessPointerReleasedEvent(window, args);
}

void SDL_WinRTApp::OnPointerWheelChanged(CoreWindow^ sender, PointerEventArgs^ args)
{
    SDL_Window * window = (m_sdlWindowData ? m_sdlWindowData->sdlWindow : nullptr);
    WINRT_ProcessPointerWheelChangedEvent(window, args);
}

void SDL_WinRTApp::OnMouseMoved(MouseDevice^ mouseDevice, MouseEventArgs^ args)
{
    SDL_Window * window = (m_sdlWindowData ? m_sdlWindowData->sdlWindow : nullptr);
    WINRT_ProcessMouseMovedEvent(window, args);
}

void SDL_WinRTApp::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
    SDL_Window * window = (m_sdlWindowData ? m_sdlWindowData->sdlWindow : nullptr);
    WINRT_ProcessPointerMovedEvent(window, args);
}

void SDL_WinRTApp::OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
    WINRT_ProcessKeyDownEvent(args);
}

void SDL_WinRTApp::OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
    WINRT_ProcessKeyUpEvent(args);
}

void SDL_WinRTApp::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    CoreWindow::GetForCurrentThread()->Activate();
}

static int SDLCALL RemoveAppSuspendAndResumeEvents(void * userdata, SDL_Event * event)
{
    if (event->type == SDL_WINDOWEVENT)
    {
        switch (event->window.event)
        {
            case SDL_WINDOWEVENT_MINIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
                // Return 0 to indicate that the event should be removed from the
                // event queue:
                return 0;
            default:
                break;
        }
    }

    // Return 1 to indicate that the event should stay in the event queue:
    return 1;
}

void SDL_WinRTApp::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
    // Save app state asynchronously after requesting a deferral. Holding a deferral
    // indicates that the application is busy performing suspending operations. Be
    // aware that a deferral may not be held indefinitely. After about five seconds,
    // the app will be forced to exit.
    SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();
    create_task([this, deferral]()
    {
        // Send a window-minimized event immediately to observers.
        // CoreDispatcher::ProcessEvents, which is the backbone on which
        // SDL_WinRTApp::PumpEvents is built, will not return to its caller
        // once it sends out a suspend event.  Any events posted to SDL's
        // event queue won't get received until the WinRT app is resumed.
        // SDL_AddEventWatch() may be used to receive app-suspend events on
        // WinRT.
        //
        // In order to prevent app-suspend events from being received twice:
        // first via a callback passed to SDL_AddEventWatch, and second via
        // SDL's event queue, the event will be sent to SDL, then immediately
        // removed from the queue.
        if (m_sdlWindowData)
        {
            SDL_SendWindowEvent(m_sdlWindowData->sdlWindow, SDL_WINDOWEVENT_MINIMIZED, 0, 0);   // TODO: see if SDL_WINDOWEVENT_SIZE_CHANGED should be getting triggered here (it is, currently)
            SDL_FilterEvents(RemoveAppSuspendAndResumeEvents, 0);
        }
        deferral->Complete();
    });
}

void SDL_WinRTApp::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
    // Restore any data or state that was unloaded on suspend. By default, data
    // and state are persisted when resuming from suspend. Note that this event
    // does not occur if the app was previously terminated.
    if (m_sdlWindowData)
    {
        SDL_SendWindowEvent(m_sdlWindowData->sdlWindow, SDL_WINDOWEVENT_RESTORED, 0, 0);    // TODO: see if SDL_WINDOWEVENT_SIZE_CHANGED should be getting triggered here (it is, currently)

        // Remove the app-resume event from the queue, as is done with the
        // app-suspend event.
        //
        // TODO, WinRT: consider posting this event to the queue even though
        // its counterpart, the app-suspend event, effectively has to be
        // processed immediately.
        SDL_FilterEvents(RemoveAppSuspendAndResumeEvents, 0);
    }
}

SDL_DisplayMode SDL_WinRTApp::CalcCurrentDisplayMode()
{
    // Create an empty, zeroed-out display mode:
    SDL_DisplayMode mode;
    SDL_zero(mode);

    // Fill in most fields:
    mode.format = SDL_PIXELFORMAT_RGB888;
    mode.refresh_rate = 0;  // TODO, WinRT: see if refresh rate data is available, or relevant (for WinRT apps)
    mode.driverdata = NULL;

    // Calculate the display size given the window size, taking into account
    // the current display's DPI:
    const float currentDPI = Windows::Graphics::Display::DisplayProperties::LogicalDpi; 
    const float dipsPerInch = 96.0f;
    mode.w = (int) ((CoreWindow::GetForCurrentThread()->Bounds.Width * currentDPI) / dipsPerInch);
    mode.h = (int) ((CoreWindow::GetForCurrentThread()->Bounds.Height * currentDPI) / dipsPerInch);

    return mode;
}

const SDL_WindowData * SDL_WinRTApp::GetSDLWindowData() const
{
    return m_sdlWindowData;
}

bool SDL_WinRTApp::HasSDLWindowData() const
{
    return (m_sdlWindowData != NULL);
}

void SDL_WinRTApp::SetSDLWindowData(const SDL_WindowData * windowData)
{
    m_sdlWindowData = windowData;
}

void SDL_WinRTApp::SetSDLVideoDevice(const SDL_VideoDevice * videoDevice)
{
    m_sdlVideoDevice = videoDevice;
}
