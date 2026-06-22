/*
  RPCEmu - An Acorn system emulator

  Podule configuration store.

  A small generic key/value store for podules, plus per-slot podule selection.
  RPCEmu's main configuration is a fixed Config struct; podules instead need an
  open-ended store (arbitrary device-defined keys), so this provides one. The
  core holds the data in memory; the GUI layer (settings.cpp) persists it to the
  machine .cfg, exactly as it does for the fixed Config struct.
 */

#ifndef PODULE_CONFIG_H
#define PODULE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Number of user-configurable podule slots. The Risc PC expansion bus exposes
   eight expansion-card slots; configured podules are installed into the slots
   left free by the built-in extension-ROM and network podules. */
#define PODULE_CONFIG_SLOTS 8
#define PODULE_CONFIG_NAME_LEN 16

/* Slot assignment - which podule short_name occupies each configurable slot.
   Empty string means the slot is unused. */
void podule_cfg_set_slot(int idx, const char *short_name);
const char *podule_cfg_get_slot(int idx);

/* Generic key/value store. 'section' is conventionally "<short_name>.<slot>".
   Values are held as strings; get_int parses with atoi(). */
int  podule_cfg_get_int(const char *section, const char *name, int def);
const char *podule_cfg_get_string(const char *section, const char *name, const char *def);
void podule_cfg_set_int(const char *section, const char *name, int val);
void podule_cfg_set_string(const char *section, const char *name, const char *val);

/* Iteration over stored entries, for persistence by the GUI layer. Returns
   non-zero and fills the pointers for a valid index, zero otherwise. */
int  podule_cfg_entry_count(void);
int  podule_cfg_get_entry(int idx, const char **section, const char **name, const char **value);

/* Clear all slot assignments and stored entries (called before a config load). */
void podule_cfg_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* PODULE_CONFIG_H */
