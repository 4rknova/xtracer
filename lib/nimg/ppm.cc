#include <string>
#include <cctype>
#include <cstring>
#include <cstdio>
#include "color.h"
#include "ppm.h"

namespace nimg {
	namespace io {
		namespace load {
/*
	FORMAT: PPM
	TYPE: BINARY

	HEADER:
	1. magic number: P3 (asciii / plain) or P6 (raw)
	2. Whitespace (blanks, TABs, CRs, LFs).
	3. A width in ASCII decimal.
	4. Whitespace.
	5. A height in ASCII decimal.
	6. Whitespace (blanks, TABs, CRs, LFs).
	7. The maximum color value in ASCII decimal.
	   Must be less than 65536 and more than zero.
	9. A single whitespace character (usually a newline).

	Pixel data then follows.

	If the PPM magic identifier is "P6" then the image data is stored in byte format,
	one byte per colour component (r,g,b). Comments can only occur before the last
	field of the header and only one byte may appear after the last header field,
	normally a carriage return or line feed. "P6" image files are obviously smaller
	than P3 and much faster to read. Note that P6 PPM files can only be used for single
	byte colours. Comment start with the character '#' and continue until a new line character.

	NOTE:
	Only the P6 type with maximum value of 255 is currently supported.
*/

int ppm_raw(const char *filename, Pixmap &fb)
{
	if (!filename) return 1;

	FILE* fp = fopen(filename, "rb");

	if (fp == NULL) {
		fb.init(0, 0);
		return 5;
	}

	// Read the header.
	int c = 0;
	std::string header_token[4];

	for (size_t tcount = 0; tcount < 4; tcount++) {
		for (;;) {
			while (isspace(c = getc(fp)));

			if (c != '#')
				break;

			do {
				c = getc(fp);
			} while (c != '\n' && c != EOF);

			if (c == EOF)
				break;
		}

		if (c != EOF) {
			do {
				header_token[tcount].append(1, c);
				c = getc(fp);
			} while (!isspace(c) && c != '#' && c != EOF);

			if (c == '#')
				ungetc(c, fp);
		}
	}

	if (header_token[0].compare("P6")) {
		fclose(fp);
		return 3;
	}

	int nx, ny, pm;
	if (sscanf(header_token[1].c_str(), "%d", &nx) != 1 ||
		sscanf(header_token[2].c_str(), "%d", &ny) != 1 ||
		sscanf(header_token[3].c_str(), "%d", &pm) != 1) {
		fclose(fp);
		return 3;
	}

	if (pm != 255) {
		fclose(fp);
		return 3;
	}

	if (fb.init(nx, ny)) {
		fclose(fp);
		return 5;
	}

	// Read the pixel data.
	for (size_t j = 0; j < fb.height(); j++) {
		for (size_t i = 0; i < fb.width(); i++) {
			unsigned char p[3];

			if (fread(p, 1, 3, fp) != 3)
			{
				fclose(fp);
				return 4;
			}

			ColorRGBAf &pixel = fb.pixel(i, j);

			pixel.r((float)p[0] / 255.0f);
			pixel.g((float)p[1] / 255.0f);
			pixel.b((float)p[2] / 255.0f);
			pixel.a(1.0f);
		}
	}

	fclose(fp);
	return 0;
}

		} /* namespace load */

		namespace save {

int ppm_raw(const char *filename, const Pixmap &fb)
{
	if (!filename)
		return 1;

	if (!fb.width() || !fb.height())
		return 3;

	FILE *fp = fopen(filename, "wb");

	if (fp == NULL)
		return 2;

	// Write the header.
	fprintf(fp, "P6\n%lu %lu\n255\n", fb.width(), fb.height());

	// Write the pixel data.
	for (size_t j = 0; j < fb.height(); j++) {
		for (size_t i = 0; i < fb.width(); i++) {
			const ColorRGBAf &pixel = fb.pixel_ro(i, j);

			// Do some basic tone mapping.
			float fmax = 0.0f;
			fmax = pixel.r() > fmax ? pixel.r() : fmax;
			fmax = pixel.g() > fmax ? pixel.g() : fmax;
			fmax = pixel.b() > fmax ? pixel.b() : fmax;

			float scale = 1.f;

			if (fmax > 1.f) {
				scale = 1.f / fmax;
			}

			// Write the pixel.
			fputc((char)(pixel.r() * scale * 255.0f), fp);
			fputc((char)(pixel.g() * scale * 255.0f), fp);
			fputc((char)(pixel.b() * scale * 255.0f), fp);

			// Check for errors
			if (ferror(fp)) {
				fclose(fp);
				return 2;
			}
		}
	}

	fclose(fp);

	return 0;
}

		} /* namespace save */
	} /* namespace io */
} /* namespace nimg */
