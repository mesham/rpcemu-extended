/*
  RPCEmu - An Acorn system emulator

  Podule configuration store - see podule_config.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "podule_config.h"

#define MAX_ENTRIES 128
#define SECTION_LEN 40
#define NAME_LEN    32
#define VALUE_LEN   256

typedef struct {
	char section[SECTION_LEN];
	char name[NAME_LEN];
	char value[VALUE_LEN];
	int used;
} podule_cfg_entry;

static podule_cfg_entry entries[MAX_ENTRIES];
static int entry_count;

static char slot_names[PODULE_CONFIG_SLOTS][PODULE_CONFIG_NAME_LEN];

void
podule_cfg_reset(void)
{
	memset(entries, 0, sizeof(entries));
	entry_count = 0;
	memset(slot_names, 0, sizeof(slot_names));
}

void
podule_cfg_set_slot(int idx, const char *short_name)
{
	if (idx < 0 || idx >= PODULE_CONFIG_SLOTS) {
		return;
	}
	if (short_name == NULL) {
		short_name = "";
	}
	snprintf(slot_names[idx], PODULE_CONFIG_NAME_LEN, "%s", short_name);
}

const char *
podule_cfg_get_slot(int idx)
{
	if (idx < 0 || idx >= PODULE_CONFIG_SLOTS) {
		return "";
	}
	return slot_names[idx];
}

static podule_cfg_entry *
find_entry(const char *section, const char *name)
{
	int i;

	for (i = 0; i < entry_count; i++) {
		if (entries[i].used &&
		    strcmp(entries[i].section, section) == 0 &&
		    strcmp(entries[i].name, name) == 0) {
			return &entries[i];
		}
	}
	return NULL;
}

const char *
podule_cfg_get_string(const char *section, const char *name, const char *def)
{
	podule_cfg_entry *e = find_entry(section, name);

	return e ? e->value : def;
}

int
podule_cfg_get_int(const char *section, const char *name, int def)
{
	podule_cfg_entry *e = find_entry(section, name);

	return e ? atoi(e->value) : def;
}

void
podule_cfg_set_string(const char *section, const char *name, const char *val)
{
	podule_cfg_entry *e;

	if (val == NULL) {
		val = "";
	}

	e = find_entry(section, name);
	if (e == NULL) {
		if (entry_count >= MAX_ENTRIES) {
			return;
		}
		e = &entries[entry_count++];
		e->used = 1;
		snprintf(e->section, SECTION_LEN, "%s", section);
		snprintf(e->name, NAME_LEN, "%s", name);
	}
	snprintf(e->value, VALUE_LEN, "%s", val);
}

void
podule_cfg_set_int(const char *section, const char *name, int val)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%d", val);
	podule_cfg_set_string(section, name, buf);
}

int
podule_cfg_entry_count(void)
{
	return entry_count;
}

int
podule_cfg_get_entry(int idx, const char **section, const char **name, const char **value)
{
	if (idx < 0 || idx >= entry_count || !entries[idx].used) {
		return 0;
	}
	*section = entries[idx].section;
	*name = entries[idx].name;
	*value = entries[idx].value;
	return 1;
}
