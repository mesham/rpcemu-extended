#ifndef ABOUT_DIALOG_H
#define ABOUT_DIALOG_H

#include <wx/wx.h>

class AboutDialog : public wxDialog {
public:
	explicit AboutDialog(wxWindow *parent);

private:
	void BuildUi();
};

#endif
