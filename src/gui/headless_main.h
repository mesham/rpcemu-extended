#ifndef HEADLESS_MAIN_H
#define HEADLESS_MAIN_H

/*
 * Console entry points for headless operation.
 *
 * These deliberately avoid ALL wxWidgets/GTK code: they are reached from main()
 * before any wxApp is constructed, so gtk_init() is never called and they work
 * on a system with no X11/Wayland display (or no GUI libraries' display
 * connection) at all. Each returns a process exit code.
 */
void HeadlessPrintUsage(const char *argv0);
int HeadlessListMachines(void);
int RunHeadless(const char *machine_name); /* machine_name may be NULL */

#endif
