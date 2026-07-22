/*
  RPCEmu - An Acorn system emulator

  Copyright (C) Sarah Walker

  Part of the podule subsystem, derived from Arculator 2.2 by Sarah Walker
  (https://b-em.bbcmicro.com/arculator/), and distributed under the GNU GPL v2.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "podule_api.h"

void joystick_init(podule_t *podule, const podule_callbacks_t *podule_callbacks);
void joystick_close(void);
void joystick_poll_host(void);

typedef struct plat_joystick_t
{
	char name[64];

	int a[8];
	int b[32];
	int p[4];

	struct
	{
		char name[32];
		int id;
	} axis[8];

	struct
	{
		char name[32];
		int id;
	} button[32];

	struct
	{
		char name[32];
		int id;
	} pov[4];

	int nr_axes;
	int nr_buttons;
	int nr_povs;
} plat_joystick_t;

#define MAX_PLAT_JOYSTICKS 8

extern plat_joystick_t plat_joystick_state[MAX_PLAT_JOYSTICKS];
extern int joysticks_present;

#define POV_X 0x80000000
#define POV_Y 0x40000000

typedef struct joystick_t
{
	int axis[8];
	int button[32];
	int pov[4];

	int plat_joystick_nr;
	int axis_mapping[8];
	int button_mapping[32];
	int pov_mapping[4][2];
} joystick_t;

#define MAX_JOYSTICKS 4
extern joystick_t joystick_state[MAX_JOYSTICKS];

#define JOYSTICK_PRESENT(n) (joystick_state[n].plat_joystick_nr != 0)


extern int joystick_get_max_joysticks(void);
extern int joystick_get_axis_count(void);
extern int joystick_get_button_count(void);
extern int joystick_get_pov_count(void);

podule_config_selection_t *joystick_devices_config(const podule_callbacks_t *podule_callbacks);

extern podule_config_selection_t joystick_button_config_selection[33];
extern podule_config_selection_t joystick_axis_config_selection[13];

void joystick_update_buttons_config(int joystick_device);
void joystick_update_axes_config(int joy_device);
