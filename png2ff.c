/* See LICENSE file for copyright and license details. */
#include <arpa/inet.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <png.h>

static char *argv0;

void
pngerr(png_structp png_struct_p, png_const_charp msg)
{
	fprintf(stderr, "%s: libpng: %s\n", argv0, msg);
	exit(1);
}

int
main(int argc, char *argv[])
{
	png_structp png_struct_p;
	png_infop png_info_p;
	png_bytepp png_row_p;
	uint32_t width, height, png_row_len, tmp32, r, i;
	uint16_t tmp16;

	argv0 = argv[0], argc--, argv++;

	if (argc) {
		fprintf(stderr, "usage: %s\n", argv0);
		return 1;
	}

	/* load png */
	png_struct_p = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
	                                      pngerr, NULL);
	png_info_p = png_create_info_struct(png_struct_p);

	if (!png_struct_p || !png_info_p) {
		fprintf(stderr, "%s: failed to initialize libpng\n", argv0);
		return 1;
	}
	png_init_io(png_struct_p, stdin);
	if (png_get_valid(png_struct_p, png_info_p, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_struct_p);
	png_set_add_alpha(png_struct_p, 255*257, PNG_FILLER_AFTER);
	png_set_expand_gray_1_2_4_to_8(png_struct_p);
	png_set_gray_to_rgb(png_struct_p);
	png_set_packing(png_struct_p);
	png_read_png(png_struct_p, png_info_p, PNG_TRANSFORM_PACKING |
	             PNG_TRANSFORM_EXPAND, NULL);
	width = png_get_image_width(png_struct_p, png_info_p);
	height = png_get_image_height(png_struct_p, png_info_p);
	png_row_len = png_get_rowbytes(png_struct_p, png_info_p);
	png_row_p = png_get_rows(png_struct_p, png_info_p);

	/* write header */
	fputs("farbfeld", stdout);
	tmp32 = htonl(width);
	if (fwrite(&tmp32, sizeof(uint32_t), 1, stdout) != 1)
		goto writerr;
	tmp32 = htonl(height);
	if (fwrite(&tmp32, sizeof(uint32_t), 1, stdout) != 1)
		goto writerr;

	/* write data */
	switch(png_get_bit_depth(png_struct_p, png_info_p)) {
	case 8:
		for (r = 0; r < height; ++r) {
			for (i = 0; i < png_row_len; i++) {
				/* ((2^16-1) / 255) == 257 */
				tmp16 = htons(257 * png_row_p[r][i]);
				if (fwrite(&tmp16, sizeof(uint16_t), 1,
				           stdout) != 1)
					goto writerr;
			}
		}
		break;
	case 16:
		for (r = 0; r < height; ++r) {
			for (i = 0; i < png_row_len / 2; i++) {
				tmp16 = *((uint16_t *)(png_row_p[r] +
				          2 * i));
				if (fwrite(&tmp16, sizeof(uint16_t), 1,
				           stdout) != 1)
					goto writerr;
			}
		}
		break;
	default:
		fprintf(stderr, "%s: invalid bit-depth\n", argv0);
		return 1;
	}

	png_destroy_read_struct(&png_struct_p, &png_info_p, NULL);

	return 0;
writerr:
	fprintf(stderr, "%s: fwrite: ", argv0);
	perror(NULL);

	return 1;
}
