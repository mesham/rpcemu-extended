#ifndef GUI_BRIDGE_H
#define GUI_BRIDGE_H

#include <string>

extern "C" {
#include "rpcemu.h"
}

#include "host_types.h"

class GuiBridge {
public:
	virtual ~GuiBridge() = default;

	virtual bool IsGuiThread() const = 0;
	virtual void PostVideoUpdate(VideoUpdate update) = 0;
	virtual void PostError(const std::string &message) = 0;
	virtual void PostFatal(const std::string &message) = 0;
	virtual void PostMoveHostMouse(const MouseMoveUpdate &update) = 0;

	virtual void ShowError(const std::string &message) = 0;
	virtual void ShowFatal(const std::string &message) = 0;

	virtual void PostNatRule(PortForwardRule rule) = 0;
	virtual void PostDebuggerStateChanged() = 0;
	virtual void PostMachineSwitched(const std::string &machine_name) = 0;

	/* The core is asking the application to quit (e.g. guest soft power-off). */
	virtual void PostQuit() = 0;
};

#endif
