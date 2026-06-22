/*
  RPCEmu - An Acorn system emulator

  Stub MIDI backend.

  Satisfies the podule MIDI interface (midi.h, from Arculator) so that MIDI
  podules build and run, but performs no real host MIDI I/O. Transmitted bytes
  are discarded and nothing is ever received. A real ALSA/CoreMIDI/Win-MM
  backend can replace this later without touching the devices that use it.
 */

#include <stdint.h>
#include <stdlib.h>

#include "midi.h"

/* Opaque handle returned to the device. Non-NULL so device NULL-checks pass;
   contents are unused by the stub. */
static int midi_stub_handle;

void *
midi_init(void *p, void (*receive)(void *p, uint8_t val),
          void (*log)(const char *format, ...),
          const podule_callbacks_t *podule_callbacks, podule_t *podule)
{
	(void) p;
	(void) receive;
	(void) log;
	(void) podule_callbacks;
	(void) podule;

	return &midi_stub_handle;
}

void
midi_close(void *p)
{
	(void) p;
}

void
midi_write(void *p, uint8_t val)
{
	(void) p;
	(void) val;
}

podule_config_selection_t *
midi_out_devices_config(void)
{
	return NULL;
}

podule_config_selection_t *
midi_in_devices_config(void)
{
	return NULL;
}
