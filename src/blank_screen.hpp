/**
 * Disable/Restore "blank screen" through the X11 core screensaver and X11 DPMS.
 *
 * BlankScreen bs;
 * bs.disable();
 * bs.restore();
 */

#ifndef BLANK_SCREEN_HPP
#define BLANK_SCREEN_HPP

#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <string.h>
#include <errno.h>

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
#define LOG_BLANKS
#ifdef LOG_BLANKS
#  define LOGBLANKS(msg) (LOG_BASE("[BlankScreen]", msg))
#else
#  define LOGBLANKS(msg)
#endif

// ***************************************************************************
// *************************************************************** BlankScreen
// ***************************************************************************
struct BlankScreen
{
    // ************************************************ BlankScreen::constructor
    BlankScreen()
        : _is_initialized(false)
    {
        // Connection to Xerver to get the display
        display = XOpenDisplay( NULL ); // NULL : get $DISPLAY env varialble

        // Get and store ScreenSaver status
        XGetScreenSaver(display, &timeout_return, &interval_return,
                        &prefer_blanking, &allow_exposure);

        if (!DPMSInfo(display, &power_level, &dpms_state)) {
            std::cerr << "__BlankScreen ::WARNING:: " << strerror(errno) << std::endl;
        }
        else {
            _is_initialized = true;
            LOGBLANKS( "__is_initialized" );
        }
    }
    ~BlankScreen()
    {
        if (_is_initialized) {
            XCloseDisplay(display);
        }
    }

    // *************************************************** BlankScreen::str_info
    std::string str_info() const
    {
        int timeout, interval, blanking, exposure;
        CARD16 level;
        BOOL state;

        // Get and store ScreenSaver status
        XGetScreenSaver(display, &timeout, &interval,
                        &blanking, &exposure);

        if (!DPMSInfo(display, &level, &state)) {
            std::cerr << "__BlankScreen::str_info ::WARNING:: " << strerror(errno) << std::endl;
        }
        else {
            std::stringstream info;
            info << "BlankScreen timeout=" << timeout;

            if (state == true) info << ", DMPS Enabled";
            else info << ", DPMS Disabled";

            return info.str();
        }
        return "BlankScreen no info";
    }

    // **************************************************** BlankScreen::disable
    void disable()
    {
        if (_is_initialized) {
            DPMSDisable(display);
            XSetScreenSaver(display, 0 /* => disable*/, interval_return,
                            prefer_blanking, allow_exposure);
            XFlush(display);

            LOGBLANKS( "__disabled" );
        }
        else {
            LOGBLANKS( "__NOT disabled as NOT _is_initialized" );
        }
    }
    // ***************************************************** BlankScreen::enable
    void enable()
    {
        if (_is_initialized) {
            DPMSEnable(display);
            XSetScreenSaver(display, 600 /* => enable*/, interval_return,
                            prefer_blanking, allow_exposure);
            XFlush(display);

            LOGBLANKS( "__enabled" );
        }
        else {
            LOGBLANKS( "__NOT enabled as NOT _is_initialized" );
        }
    }
    // **************************************************** BlankScreen::restore
    void restore()
    {
        if (_is_initialized) {

            if (dpms_state) {
                DPMSEnable(display);
            }
            else {
                DPMSDisable(display);
            }

            XSetScreenSaver(display, -1 /*=> restore default*/, interval_return,
                            prefer_blanking, allow_exposure);

            LOGBLANKS( "__restored" );
        }
        else {
            LOGBLANKS( "__NOT restored as NOT _is_initialized" );
        }
    }

    // ************************************************** BlankScreen::attributs
    // can restore only if _is_initialized
    bool _is_initialized;

    // X11 Display
    Display *display;

    // ScreenSaver status
    int timeout_return, interval_return, prefer_blanking, allow_exposure;

    // DPMS status
    CARD16 power_level;
    BOOL dpms_state;

}; // BlankScreen
// ********************************************************* BlankScreen - END

#endif // BLANK_SCREEN_HPP
