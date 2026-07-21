#ifndef HEADLESS_BRIDGE_H
#define HEADLESS_BRIDGE_H

#include "gui_bridge.h"

/*
 * GuiBridge implementation for --headless mode.
 *
 * There is no wxWidgets window and no GUI event loop. Video reaches clients
 * through the built-in VNC server, which is fed directly from
 * rpcemu_video_update() (independently of the bridge), so the video and mouse
 * callbacks here are deliberate no-ops. Errors are reported on stderr; a fatal
 * error terminates the process (there is no dialog to show and nobody to
 * dismiss it).
 */
class HeadlessBridge : public GuiBridge {
public:
	bool IsGuiThread() const override { return false; }

	void PostVideoUpdate(VideoUpdate update) override;
	void PostError(const std::string &message) override;
	void PostFatal(const std::string &message) override;
	void PostMoveHostMouse(const MouseMoveUpdate &update) override;

	void ShowError(const std::string &message) override;
	void ShowFatal(const std::string &message) override;

	void PostNatRule(PortForwardRule rule) override;
	void PostDebuggerStateChanged() override;
	void PostMachineSwitched(const std::string &machine_name) override;
	void PostQuit() override;
};

#endif
