/*
  RPCEmu - An Acorn system emulator

  Podule plugin ABI.

  Adapted from Arculator 2.2 (Sarah Walker) so that podule device
  implementations written against Arculator's expansion-card API can be hosted
  by RPCEmu with little or no change. See docs and reference/arculator for the
  original.

  This header defines the contract between the emulator (host) and a podule
  device. A device provides a podule_header_t via podule_probe()/an internal
  probe function; the host calls into header->functions and the device calls
  back via podule_callbacks_t.
 */

#ifndef PODULE_API_H
#define PODULE_API_H

#include <stdint.h>

#define MAKE_PODULE_API_VERSION(major, minor) (((minor) << 16) | (major))

#define PODULE_API_VERSION_GET_MAJOR(version) ((version) & 0xffff)
#define PODULE_API_VERSION_GET_MINOR(version) ((version) >> 16)

/*v1.1 - add PODULE_FLAGS_NEXT and PODULE_FLAGS_NET.
  v1.0 - initial version.*/
#define PODULE_API_VERSION MAKE_PODULE_API_VERSION(1, 1)

struct podule_t;

/* Canonical podule address-space enum. podules.h aliases this as PoduleIoType
   for existing RPCEmu code. */
typedef enum podule_io_type
{
	/*Access is in IOC space*/
	PODULE_IO_TYPE_IOC = 0,
	/*Access is in MEMC space*/
	PODULE_IO_TYPE_MEMC,
	/*Access is in EASI space*/
	PODULE_IO_TYPE_EASI
} podule_io_type;

typedef struct podule_functions_t
{
	/*init() - Initialise podule instance. Returns zero on success, non-zero
	  on error. Any instance data should be stored in podule->p*/
	int (*init)(struct podule_t *podule);
	/*close() - Close podule instance. Responsible for freeing podule->p*/
	void (*close)(struct podule_t *podule);

	void (*write_b)(struct podule_t *podule, podule_io_type type, uint32_t addr, uint8_t val);
	void (*write_w)(struct podule_t *podule, podule_io_type type, uint32_t addr, uint16_t val);
	void (*write_l)(struct podule_t *podule, podule_io_type type, uint32_t addr, uint32_t val);
	uint8_t  (*read_b)(struct podule_t *podule, podule_io_type type, uint32_t addr);
	uint16_t (*read_w)(struct podule_t *podule, podule_io_type type, uint32_t addr);
	uint32_t (*read_l)(struct podule_t *podule, podule_io_type type, uint32_t addr);

	/*reset() - Reset podule*/
	void (*reset)(struct podule_t *podule);

	/*run() - Callback from podule timer. @timeslice_us is the time expired
	  since the last run() call. Returns delay (in us) to reload the timer
	  with; zero stops the timer.*/
	int (*run)(struct podule_t *podule, int timeslice_us);
} podule_functions_t;

#define CONFIG_STRING 0
#define CONFIG_INT 1
#define CONFIG_BINARY 2
#define CONFIG_SELECTION 3
#define CONFIG_SELECTION_STRING 4
#define CONFIG_BUTTON 5

/*Control can not be altered by user*/
#define CONFIG_FLAGS_DISABLED (1 << 0)
/*'name' entry should be prefixed with the prefix passed to config_open()*/
#define CONFIG_FLAGS_NAME_PREFIXED (1 << 1)

typedef struct podule_config_selection_t
{
	const char *description;
	int value;
	char *value_string;
} podule_config_selection_t;

typedef struct podule_config_item_t
{
	const char *name;
	const char *description;
	const int id;
	const int type;
	const int flags;

	const char *default_string;
	int default_int;
	podule_config_selection_t *selection;

	int (*function)(void *window_p, const struct podule_config_item_t *item, void *new_data);
} podule_config_item_t;

typedef struct podule_config_t
{
	const char *title;
	void (*init)(void *window_p);
	int (*close)(void *window_p);

	podule_config_item_t items[];
} podule_config_t;

/*Only one instance of this podule allowed per system*/
#define PODULE_FLAGS_UNIQUE (1 << 0)
/*Podule is 8-bit minipodule (A30x0/A4000) - Archimedes only*/
#define PODULE_FLAGS_8BIT (1 << 1)
/*Indicates another podule header is present after this one*/
#define PODULE_FLAGS_NEXT (1 << 2)
/*Podule is network interface expansion*/
#define PODULE_FLAGS_NET (1 << 3)

#define PODULE_FLAGS_VALID_VERSION_1_0  (PODULE_FLAGS_UNIQUE | PODULE_FLAGS_8BIT)
#define PODULE_FLAGS_VALID 		(PODULE_FLAGS_UNIQUE | PODULE_FLAGS_8BIT | PODULE_FLAGS_NEXT | PODULE_FLAGS_NET)

typedef struct podule_header_t
{
	const uint32_t version;
	const uint32_t flags;
	const char *short_name;
	const char *name;

	const podule_functions_t functions;

	podule_config_t *config;
} podule_header_t;

typedef struct podule_t
{
	/*Pointer to header for this podule*/
	const podule_header_t *header;
	/*Pointer to private instance data for this podule*/
	void *p;
} podule_t;

#define CONFIG_FILESEL_LOAD (0)
#define CONFIG_FILESEL_SAVE (1 << 0)

typedef struct podule_callbacks_t
{
	void (*set_irq)(podule_t *podule, int state);
	void (*set_fiq)(podule_t *podule, int state);

	void (*set_timer_delay_us)(podule_t *podule, int delay_us);
	int (*get_timer_remaining_us)(podule_t *podule);
	void (*stop_timer)(podule_t *podule);

	int (*config_get_int)(podule_t *podule, const char *name, int def);
	const char *(*config_get_string)(podule_t *podule, const char *name, const char *def);
	void (*config_set_int)(podule_t *podule, const char *name, int val);
	void (*config_set_string)(podule_t *podule, const char *name, char *val);

	int (*config_open)(void *window_p, podule_config_t *config, const char *prefix);
	int (*config_file_selector)(void *window_p, const char *title,
			const char *default_path, const char *default_fn,
			const char *default_ext, const char *wildcard,
			char *dest, int dest_len, int flags);
	void *(*config_get_current)(void *window_p, int id);
	void (*config_set_current)(void *window_p, int id, void *val);
} podule_callbacks_t;

/*Main entry point implemented by a dynamically-loaded podule. Internal podules
  expose an equivalent <name>_probe() function registered in podules.c.*/
const podule_header_t *podule_probe(const podule_callbacks_t *callbacks, char *path);

#endif /*PODULE_API_H*/
