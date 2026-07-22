/*
  RPCEmu - An Acorn system emulator

  Copyright (C) 2025 Andy Timmins

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

#include <stdio.h>
#include <string.h>

#include "rpcemu.h"
#include "print_convert.h"

static void
print_convert_set_error(char *errbuf, size_t errbuf_len, const char *message)
{
	if (errbuf == NULL || errbuf_len == 0 || message == NULL) {
		return;
	}

	strncpy(errbuf, message, errbuf_len - 1);
	errbuf[errbuf_len - 1] = '\0';
}

#ifdef CONFIG_GHOSTPDL

#include <ghostscript/iapi.h>
#include <ghostscript/ierrors.h>

int
print_convert_available(void)
{
	return 1;
}

PrintJobFormat
print_convert_detect_format(const char *input_path)
{
	FILE *f;
	unsigned char header[8];
	size_t nread;

	if (input_path == NULL || input_path[0] == '\0') {
		return PrintJobFormat_Unknown;
	}

	f = fopen(input_path, "rb");
	if (f == NULL) {
		return PrintJobFormat_Unknown;
	}

	nread = fread(header, 1, sizeof(header), f);
	fclose(f);

	if (nread >= 4 && memcmp(header, "%!PS", 4) == 0) {
		return PrintJobFormat_PostScript;
	}
	if (nread >= 2 && header[0] == 0x1b && header[1] == 'E') {
		return PrintJobFormat_PCL;
	}
	if (nread >= 4 && memcmp(header, "@PJL", 4) == 0) {
		return PrintJobFormat_PCL;
	}

	return PrintJobFormat_Unknown;
}

void
print_convert_prn_to_pdf_path(const char *input_path,
                              char *output_path,
                              size_t output_path_len)
{
	size_t input_len;
	const char *dot;

	if (input_path == NULL || output_path == NULL || output_path_len == 0) {
		return;
	}

	input_len = strlen(input_path);
	if (input_len >= output_path_len) {
		output_path[0] = '\0';
		return;
	}

	strncpy(output_path, input_path, output_path_len - 1);
	output_path[output_path_len - 1] = '\0';

	dot = strrchr(output_path, '.');
	if (dot != NULL) {
		const size_t prefix_len = (size_t)(dot - output_path);
		if (prefix_len + 4 < output_path_len) {
			strcpy(output_path + prefix_len, ".pdf");
			return;
		}
	}

	if (input_len + 4 < output_path_len) {
		strcat(output_path, ".pdf");
	}
}

PrintConvertResult
print_convert_prn_to_pdf(const char *input_path,
                         const char *output_path,
                         char *errbuf,
                         size_t errbuf_len)
{
	void *instance = NULL;
	char output_arg[1024];
	char *argv[8];
	const PrintJobFormat format = print_convert_detect_format(input_path);
	int argc = 0;
	int code;

	if (input_path == NULL || output_path == NULL) {
		print_convert_set_error(errbuf, errbuf_len, "Missing input or output path");
		return PrintConvert_Error;
	}

#ifndef CONFIG_GHOSTPDL_PCL
	if (format == PrintJobFormat_PCL) {
		print_convert_set_error(errbuf, errbuf_len,
		                        "PCL print jobs require GhostPDL (libgpdl); "
		                        "use a PostScript printer driver in RISC OS");
		return PrintConvert_Unsupported;
	}
#endif

	snprintf(output_arg, sizeof(output_arg), "-sOutputFile=%s", output_path);

	argv[argc++] = "rpcemu";
	argv[argc++] = "-dBATCH";
	argv[argc++] = "-dNOPAUSE";
	argv[argc++] = "-dSAFER";
	argv[argc++] = "-sDEVICE=pdfwrite";
	argv[argc++] = output_arg;
	argv[argc++] = (char *)input_path;

	code = gsapi_new_instance(&instance, NULL);
	if (code < 0) {
		print_convert_set_error(errbuf, errbuf_len,
		                        "Failed to create Ghostscript instance");
		return PrintConvert_Error;
	}

	code = gsapi_init_with_args(instance, argc, argv);
	if (code != 0 && code != gs_error_Quit) {
		print_convert_set_error(errbuf, errbuf_len,
		                        "Ghostscript failed to convert print job");
		gsapi_exit(instance);
		gsapi_delete_instance(instance);
		return PrintConvert_Error;
	}

	gsapi_exit(instance);
	gsapi_delete_instance(instance);

	rpclog("PrintConvert: Wrote PDF '%s' from '%s'\n", output_path, input_path);
	return PrintConvert_OK;
}

#else /* CONFIG_GHOSTPDL */

int
print_convert_available(void)
{
	return 0;
}

PrintJobFormat
print_convert_detect_format(const char *input_path)
{
	(void)input_path;
	return PrintJobFormat_Unknown;
}

void
print_convert_prn_to_pdf_path(const char *input_path,
                              char *output_path,
                              size_t output_path_len)
{
	(void)input_path;
	if (output_path != NULL && output_path_len > 0) {
		output_path[0] = '\0';
	}
}

PrintConvertResult
print_convert_prn_to_pdf(const char *input_path,
                         const char *output_path,
                         char *errbuf,
                         size_t errbuf_len)
{
	(void)input_path;
	(void)output_path;
	print_convert_set_error(errbuf, errbuf_len,
	                        "RPCEmu was built without GhostPDL support");
	return PrintConvert_NotAvailable;
}

#endif /* CONFIG_GHOSTPDL */
