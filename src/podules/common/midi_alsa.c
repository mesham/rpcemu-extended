/*
  RPCEmu - An Acorn system emulator

  ALSA MIDI backend.

  A real host-MIDI pathway for MIDI podules, ported from Arculator 2.2 (Sarah
  Walker). Enumerates ALSA raw-MIDI devices, opens the configured in/out
  devices, sends MIDI bytes to the host, and reads host input on a thread.
  Replaces midi_stub.c on platforms with ALSA.
 */

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "podule_api.h"
#include "midi.h"

typedef struct midi_t {
	int pos, len;
	uint32_t command;
	int insysex;
	uint8_t sysex_data[1024 + 2];

	snd_rawmidi_t *out_device;
	snd_rawmidi_t *in_device;

	pthread_t in_thread;
	int in_thread_started;
	volatile int in_thread_term;

	void (*receive)(void *p, uint8_t val);
	void (*log)(const char *format, ...);

	void *p;
} midi_t;

#define MAX_MIDI_DEVICES 50
static struct {
	int card;
	int device;
	int sub;
	int is_input;
	int is_output;
	char name[64];
} midi_devices[MAX_MIDI_DEVICES];

static int midi_device_count = 0;
static int midi_queried = 0;

static void
midi_query(void)
{
	int status;
	int card = -1;

	midi_queried = 1;

	if ((status = snd_card_next(&card)) < 0 || card < 0) {
		return; /* no cards */
	}

	while (card >= 0) {
		char *shortname;

		if (snd_card_get_name(card, &shortname) >= 0) {
			snd_ctl_t *ctl;
			char name[32];

			snprintf(name, sizeof(name), "hw:%i", card);

			if (snd_ctl_open(&ctl, name, 0) >= 0) {
				int device = -1;

				do {
					status = snd_ctl_rawmidi_next_device(ctl, &device);
					if (status >= 0 && device != -1) {
						snd_rawmidi_info_t *info;
						int sub_nr, sub;

						snd_rawmidi_info_alloca(&info);
						snd_rawmidi_info_set_device(info, device);
						snd_ctl_rawmidi_info(ctl, info);
						sub_nr = snd_rawmidi_info_get_subdevices_count(info);

						for (sub = 0; sub < sub_nr; sub++) {
							snd_rawmidi_info_set_subdevice(info, sub);
							if (snd_ctl_rawmidi_info(ctl, info) != 0) {
								continue;
							}

							midi_devices[midi_device_count].card = card;
							midi_devices[midi_device_count].device = device;
							midi_devices[midi_device_count].sub = sub;
							snprintf(midi_devices[midi_device_count].name,
							         sizeof(midi_devices[0].name),
							         "%s (%i:%i:%i)", shortname, card, device, sub);

							snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
							if (snd_ctl_rawmidi_info(ctl, info) == 0) {
								midi_devices[midi_device_count].is_output = 1;
							}
							snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
							if (snd_ctl_rawmidi_info(ctl, info) == 0) {
								midi_devices[midi_device_count].is_input = 1;
							}

							midi_device_count++;
							if (midi_device_count >= MAX_MIDI_DEVICES) {
								snd_ctl_close(ctl);
								return;
							}
						}
					}
				} while (device >= 0);

				snd_ctl_close(ctl);
			}
		}

		if (snd_card_next(&card) < 0) {
			break;
		}
	}
}

static void *
midi_in_callback(void *p)
{
	midi_t *midi = p;

	while (!midi->in_thread_term) {
		uint8_t data;
		int status = snd_rawmidi_read(midi->in_device, &data, 1);

		if (status >= 0) {
			midi->receive(midi->p, data);
		} else {
			usleep(5000); /* 5ms */
		}
	}
	return NULL;
}

void *
midi_init(void *p, void (*receive)(void *p, uint8_t val),
          void (*log)(const char *format, ...),
          const podule_callbacks_t *podule_callbacks, podule_t *podule)
{
	midi_t *midi;
	char portname[32];
	const char *device;
	int midi_in_dev_nr = -1, midi_out_dev_nr = -1;

	if (!midi_queried) {
		midi_query();
	}

	midi = malloc(sizeof(midi_t));
	if (midi == NULL) {
		return NULL;
	}
	memset(midi, 0, sizeof(midi_t));

	device = podule_callbacks->config_get_string(podule, "midi_in_device", "-1");
	sscanf(device, "%i", &midi_in_dev_nr);
	device = podule_callbacks->config_get_string(podule, "midi_out_device", "-1");
	sscanf(device, "%i", &midi_out_dev_nr);
	if (log) {
		log("midi_init: in=%i out=%i (%i devices)\n",
		    midi_in_dev_nr, midi_out_dev_nr, midi_device_count);
	}

	midi->p = p;
	midi->receive = receive;
	midi->log = log;

	if (midi_out_dev_nr >= 0 && midi_out_dev_nr < midi_device_count) {
		snprintf(portname, sizeof(portname), "hw:%i,%i,%i",
		         midi_devices[midi_out_dev_nr].card,
		         midi_devices[midi_out_dev_nr].device,
		         midi_devices[midi_out_dev_nr].sub);
		if (log) {
			log("Opening MIDI out port %s\n", portname);
		}
		if (snd_rawmidi_open(NULL, &midi->out_device, portname, SND_RAWMIDI_SYNC) < 0) {
			midi->out_device = NULL;
			if (log) {
				log("Failed to open MIDI out device\n");
			}
		}
	}

	if (midi_in_dev_nr >= 0 && midi_in_dev_nr < midi_device_count) {
		snprintf(portname, sizeof(portname), "hw:%i,%i,%i",
		         midi_devices[midi_in_dev_nr].card,
		         midi_devices[midi_in_dev_nr].device,
		         midi_devices[midi_in_dev_nr].sub);
		if (log) {
			log("Opening MIDI in port %s\n", portname);
		}
		if (snd_rawmidi_open(&midi->in_device, NULL, portname, SND_RAWMIDI_NONBLOCK) < 0) {
			midi->in_device = NULL;
			if (log) {
				log("Failed to open MIDI in device\n");
			}
		} else {
			midi->in_thread_started = 1;
			pthread_create(&midi->in_thread, NULL, midi_in_callback, midi);
		}
	}

	return midi;
}

void
midi_close(void *p)
{
	midi_t *midi = p;

	if (midi == NULL) {
		return;
	}

	if (midi->in_thread_started) {
		midi->in_thread_term = 1;
		pthread_join(midi->in_thread, NULL);
	}
	if (midi->in_device != NULL) {
		snd_rawmidi_close(midi->in_device);
	}
	if (midi->out_device != NULL) {
		snd_rawmidi_close(midi->out_device);
	}
	free(midi);
}

static const int midi_lengths[8] = {3, 3, 3, 3, 2, 2, 3, 1};

void
midi_write(void *p, uint8_t val)
{
	midi_t *midi = p;

	if (midi == NULL || midi->out_device == NULL) {
		return;
	}

	if ((val & 0x80) && !(val == 0xf7 && midi->insysex)) {
		midi->pos = 0;
		midi->len = midi_lengths[(val >> 4) & 7];
		midi->command = 0;
		if (val == 0xf0) {
			midi->insysex = 1;
		}
	}

	if (midi->insysex) {
		midi->sysex_data[midi->pos++] = val;
		if (val == 0xf7 || midi->pos >= 1024 + 2) {
			snd_rawmidi_write(midi->out_device, midi->sysex_data, midi->pos);
			midi->insysex = 0;
		}
		return;
	}

	if (midi->len) {
		if (midi->pos > midi->len) {
			/* Repeated (running-status) command */
			midi->pos = 1;
			midi->command &= 0xff;
		}
		midi->command |= (val << (midi->pos * 8));
		midi->pos++;
		if (midi->pos == midi->len) {
			snd_rawmidi_write(midi->out_device, &midi->command, midi->len);
		}
	}
}

/* Build a CONFIG_SELECTION list of host MIDI devices (filtered by direction).
   Entry 0 is "None" (value -1); the list is terminated by an empty description. */
static podule_config_selection_t *
build_device_list(int want_output)
{
	podule_config_selection_t *sel, *sel_p;
	char *text = malloc(65536);
	int c;

	if (!midi_queried) {
		midi_query();
	}
	if (text == NULL) {
		return NULL;
	}

	sel = malloc(sizeof(podule_config_selection_t) * (midi_device_count + 2));
	if (sel == NULL) {
		free(text);
		return NULL;
	}
	sel_p = sel;

	strcpy(text, "None");
	sel_p->description = text;
	sel_p->value = -1;
	sel_p++;
	text += strlen(text) + 1;

	for (c = 0; c < midi_device_count; c++) {
		if ((want_output && midi_devices[c].is_output) ||
		    (!want_output && midi_devices[c].is_input)) {
			strcpy(text, midi_devices[c].name);
			sel_p->description = text;
			sel_p->value = c;
			sel_p++;
			text += strlen(text) + 1;
		}
	}

	strcpy(text, "");
	sel_p->description = text;

	return sel;
}

podule_config_selection_t *
midi_out_devices_config(void)
{
	return build_device_list(1);
}

podule_config_selection_t *
midi_in_devices_config(void)
{
	return build_device_list(0);
}
