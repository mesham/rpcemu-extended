#ifndef MACHINE_INSPECTOR_WINDOW_H
#define MACHINE_INSPECTOR_WINDOW_H

#include <cstdint>

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>

#include "emulator_host.h"
#include "machine_snapshot.h"

class MachineInspectorWindow : public wxFrame {
public:
	explicit MachineInspectorWindow(wxWindow *parent, EmulatorHost &emulator);

	void ShowAndRaise();

private:
	enum {
		ID_AUTO_REFRESH = wxID_HIGHEST + 1,
		ID_REFRESH_NOW,
		ID_DISASM_GO,
		ID_DISASM_FOLLOW_PC,
		ID_MEMORY_GO,
		ID_MEMORY_REFRESH,
		ID_RUN,
		ID_PAUSE,
		ID_STEP,
		ID_BREAKPOINT_ADD,
		ID_BREAKPOINT_REMOVE,
		ID_WATCHPOINT_ADD,
		ID_WATCHPOINT_REMOVE,
		ID_TRACE_CONFIG,
		ID_TRACE_CLEAR,
	};

	void BuildUi();
	void ApplyMonoFont(wxWindow *window);

	void OnTimer(wxTimerEvent &event);
	void OnRefreshNow(wxCommandEvent &event);
	void OnAutoRefresh(wxCommandEvent &event);
	void OnDisasmGo(wxCommandEvent &event);
	void OnDisasmFollowPc(wxCommandEvent &event);
	void OnMemoryGo(wxCommandEvent &event);
	void OnMemoryRefresh(wxCommandEvent &event);
	void OnRun(wxCommandEvent &event);
	void OnPause(wxCommandEvent &event);
	void OnStep(wxCommandEvent &event);
	void OnAddBreakpoint(wxCommandEvent &event);
	void OnRemoveBreakpoint(wxCommandEvent &event);
	void OnAddWatchpoint(wxCommandEvent &event);
	void OnRemoveWatchpoint(wxCommandEvent &event);
	void OnBreakpointSelection(wxCommandEvent &event);
	void OnWatchpointSelection(wxCommandEvent &event);
	void OnTraceConfigChanged(wxCommandEvent &event);
	void OnTraceClear(wxCommandEvent &event);

	void RefreshSnapshot();
	void ApplySnapshot(const MachineSnapshot &snapshot);
	void RefreshDisassembly(uint32_t address);
	void RefreshMemoryView(uint32_t address);

	wxString FormatRegisters(const MachineSnapshot &snapshot) const;
	wxString FormatPeripheralSummary(const MachineSnapshot &snapshot) const;
	wxString MakeSummary(const MachineSnapshot &snapshot) const;
	void UpdateDebuggerUi(const MachineSnapshot &snapshot);
	void PopulateBreakpointList(const MachineSnapshot &snapshot);
	void PopulateWatchpointList(const MachineSnapshot &snapshot);
	void ApplyTraceConfig();
	void DrainTraceEvents();

	uint32_t ParseAddress(const wxString &text, bool *ok) const;

	EmulatorHost &emulator_;
	wxTimer refresh_timer_{this};

	wxStaticText *summary_label_ = nullptr;
	wxCheckBox *auto_refresh_checkbox_ = nullptr;

	wxTextCtrl *cpu_view_ = nullptr;
	wxTextCtrl *disasm_view_ = nullptr;
	wxTextCtrl *memory_view_ = nullptr;
	wxTextCtrl *peripheral_view_ = nullptr;

	wxTextCtrl *disasm_address_input_ = nullptr;
	wxCheckBox *disasm_follow_pc_checkbox_ = nullptr;

	wxTextCtrl *memory_address_input_ = nullptr;
	wxSpinCtrl *memory_bytes_spin_ = nullptr;

	wxStaticText *debug_status_label_ = nullptr;
	wxStaticText *debug_hit_label_ = nullptr;
	wxButton *run_button_ = nullptr;
	wxButton *pause_button_ = nullptr;
	wxButton *step_button_ = nullptr;
	wxListBox *breakpoint_list_ = nullptr;
	wxTextCtrl *breakpoint_input_ = nullptr;
	wxButton *breakpoint_remove_button_ = nullptr;
	wxListBox *watchpoint_list_ = nullptr;
	wxTextCtrl *watchpoint_address_input_ = nullptr;
	wxChoice *watchpoint_size_choice_ = nullptr;
	wxCheckBox *watchpoint_read_checkbox_ = nullptr;
	wxCheckBox *watchpoint_write_checkbox_ = nullptr;
	wxCheckBox *watchpoint_log_only_checkbox_ = nullptr;
	wxButton *watchpoint_remove_button_ = nullptr;

	wxCheckBox *trap_undefined_checkbox_ = nullptr;
	wxCheckBox *trap_prefetch_checkbox_ = nullptr;
	wxCheckBox *trap_data_abort_checkbox_ = nullptr;
	wxCheckBox *log_exceptions_checkbox_ = nullptr;
	wxCheckBox *swi_trace_checkbox_ = nullptr;
	wxCheckBox *swi_halt_checkbox_ = nullptr;
	wxTextCtrl *swi_filter_min_input_ = nullptr;
	wxTextCtrl *swi_filter_max_input_ = nullptr;
	wxTextCtrl *trace_view_ = nullptr;
	wxCheckBox *trace_autoscroll_checkbox_ = nullptr;
	wxStaticText *trace_dropped_label_ = nullptr;
	uint32_t trace_dropped_total_ = 0;

	uint32_t disasm_current_address_ = 0;
	uint32_t memory_current_address_ = 0;
	MachineSnapshot last_snapshot_{};

	wxDECLARE_EVENT_TABLE();
};

#endif
