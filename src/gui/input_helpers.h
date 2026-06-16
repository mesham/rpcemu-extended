#ifndef INPUT_HELPERS_H
#define INPUT_HELPERS_H

#include <wx/event.h>

extern "C" {
#include "keyboard.h"
}

unsigned InputNativeScancodeFromKeyEvent(const wxKeyEvent &event);
bool InputIsReleaseMouseCaptureKey(const wxKeyEvent &event);

#endif
