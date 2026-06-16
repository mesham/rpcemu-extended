/*
  RPCEmu - An Acorn system emulator

  Copyright (C) 2025 RPCEmu contributors

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

/*
 * Print job conversion using the Ghostscript / GhostPDL library API.
 *
 * When built with CONFIG_GHOSTPDL, .prn files are converted in-process via
 * gsapi (no external ps2pdf/gpcl6 executables). Link against libgpdl for
 * PostScript, PDF, PCL, and XPS input, or libgs for PostScript/PDF only.
 */

#ifndef PRINT_CONVERT_H
#define PRINT_CONVERT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PrintConvert_OK = 0,
	PrintConvert_Error = -1,
	PrintConvert_NotAvailable = -2,
	PrintConvert_Unsupported = -3
} PrintConvertResult;

typedef enum {
	PrintJobFormat_Unknown = 0,
	PrintJobFormat_PostScript,
	PrintJobFormat_PCL
} PrintJobFormat;

/**
 * Return non-zero if in-process conversion was compiled in and initialised.
 */
int print_convert_available(void);

/**
 * Detect the format of a captured print job file.
 */
PrintJobFormat print_convert_detect_format(const char *input_path);

/**
 * Convert a .prn file to PDF.
 *
 * @param input_path   Source print job path
 * @param output_path  Destination PDF path
 * @param errbuf       Optional buffer for an error message
 * @param errbuf_len   Size of errbuf
 */
PrintConvertResult print_convert_prn_to_pdf(const char *input_path,
                                            const char *output_path,
                                            char *errbuf,
                                            size_t errbuf_len);

/**
 * Derive a PDF output path from a .prn path (replace or append .pdf).
 */
void print_convert_prn_to_pdf_path(const char *input_path,
                                   char *output_path,
                                   size_t output_path_len);

#ifdef __cplusplus
}
#endif

#endif /* PRINT_CONVERT_H */
