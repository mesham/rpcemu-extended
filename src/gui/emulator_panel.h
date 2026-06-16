#ifndef EMULATOR_PANEL_H
#define EMULATOR_PANEL_H

#include <chrono>

#include <wx/wx.h>

#include "emulator_host.h"
#include "host_types.h"

class EmulatorPanel : public wxPanel {
public:
	EmulatorPanel(wxWindow *parent, EmulatorHost &emulator);

	void ApplyVideoUpdate(const VideoUpdate &update);
	void HandleMoveHostMouse(const MouseMoveUpdate &update);
	void ReleaseMouseCapture();
	void UpdateMouseCursor();
	void FocusPanel();
	void SetFullScreen(bool full_screen);
	void SetIntegerScaling(bool integer_scaling);
	bool SaveScreenshot(const wxString &path);

private:
	void OnPaint(wxPaintEvent &event);
	void OnSize(wxSizeEvent &event);
	void OnMouseMove(wxMouseEvent &event);
	void OnMouseDown(wxMouseEvent &event);
	void OnMouseUp(wxMouseEvent &event);
	void OnMouseDoubleClick(wxMouseEvent &event);
	void ForwardMousePress(const wxMouseEvent &event);
	void OnMouseWheel(wxMouseEvent &event);
	void OnEnterWindow(wxMouseEvent &event);
	void OnLeaveWindow(wxMouseEvent &event);

	void CalculateScaling();
	void ResizeToHostDisplay();
	void SyncMousePosition(int x, int y);
	bool IsMouseOverPanel() const;
	bool ShouldSuppressHostMouseWarp() const;
	int MapClickButton(const wxMouseEvent &event) const;
	wxPoint HostPointToPanel(int host_x, int host_y) const;
	wxPoint PanelPointToHost(int x, int y) const;
	wxPoint CaptureCentre() const;
	void MarkUserPointerActivity();

	EmulatorHost &emulator_;
	wxImage display_image_;
	int image_width_ = 640;
	int image_height_ = 480;
	int double_size_ = 0;
	int host_xsize_ = 640;
	int host_ysize_ = 480;
	int scaled_x_ = 640;
	int scaled_y_ = 480;
	int offset_x_ = 0;
	int offset_y_ = 0;
	int last_mouse_x_ = -1;
	int last_mouse_y_ = -1;
	int last_press_button_ = 0;
	std::chrono::steady_clock::time_point last_press_time_{};
	bool integer_scaling_ = false;
	bool full_screen_ = false;
	std::chrono::steady_clock::time_point user_pointer_until_{};

	wxDECLARE_EVENT_TABLE();
};

#endif
