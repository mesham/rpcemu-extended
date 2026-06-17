#include "headless_bridge.h"

#include <cstdio>
#include <cstdlib>

/*
 * Video is delivered to VNC clients straight from rpcemu_video_update(); the
 * bridge has nothing to draw. Likewise there is no host cursor to move, no NAT
 * dialog to update, no debugger UI, and no window title to retitle.
 */
void HeadlessBridge::PostVideoUpdate(VideoUpdate /*update*/)
{
}

void HeadlessBridge::PostMoveHostMouse(const MouseMoveUpdate & /*update*/)
{
}

void HeadlessBridge::PostNatRule(PortForwardRule /*rule*/)
{
}

void HeadlessBridge::PostDebuggerStateChanged()
{
}

void HeadlessBridge::PostMachineSwitched(const std::string & /*machine_name*/)
{
}

/*
 * error()/fatal() in emulator_host.cpp already print the message to stderr
 * before calling the bridge, so the non-fatal hooks need only avoid crashing.
 */
void HeadlessBridge::PostError(const std::string & /*message*/)
{
}

void HeadlessBridge::ShowError(const std::string & /*message*/)
{
}

/*
 * fatal() invokes one of these and then spins forever expecting the GUI to put
 * up a dialog. In headless mode there is no UI, so terminate the process
 * instead of hanging.
 */
void HeadlessBridge::PostFatal(const std::string & /*message*/)
{
	fflush(stdout);
	fflush(stderr);
	std::_Exit(EXIT_FAILURE);
}

void HeadlessBridge::ShowFatal(const std::string &message)
{
	PostFatal(message);
}
