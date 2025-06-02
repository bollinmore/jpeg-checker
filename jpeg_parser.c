#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

bool read_two_bytes(FILE *file, uint8_t *b1, uint8_t *b2)
{
	return fread(b1, 1, 1, file) == 1 && fread(b2, 1, 1, file) == 1;
}

bool is_valid_jpeg_header(FILE *file)
{
	uint8_t soi[2], eoi[2];
	rewind(file);
	if (fread(soi, 1, 2, file) != 2 || soi[0] != 0xFF || soi[1] != 0xD8)
		return false;
	if (fseek(file, -2, SEEK_END) != 0 || fread(eoi, 1, 2, file) != 2)
		return false;
	return (eoi[0] == 0xFF && eoi[1] == 0xD9);
}

int sof_components = 0;
int adobe_color_transform = -1;

void detect_color_space_from_sof(FILE *file, long offset, uint8_t marker)
{
	uint8_t len_hi, len_lo;
	fread(&len_hi, 1, 1, file);
	fread(&len_lo, 1, 1, file);
	uint16_t length = (len_hi << 8) | len_lo;

	uint8_t precision, h_hi, h_lo, w_hi, w_lo, components;
	fread(&precision, 1, 1, file);
	fread(&h_hi, 1, 1, file);
	fread(&h_lo, 1, 1, file);
	fread(&w_hi, 1, 1, file);
	fread(&w_lo, 1, 1, file);
	fread(&components, 1, 1, file);

	sof_components = components;

	int height = (h_hi << 8) | h_lo;
	int width = (w_hi << 8) | w_lo;

	printf("[0x%04lX] SOF Marker: 0xFF%02X, %dx%d, Components: %d\n", offset - 9, marker, width, height, components);

	fseek(file, length - 8, SEEK_CUR);
}

void parse_app14_adobe(FILE *file, long offset, uint16_t length)
{
	char id[6] = {0};
	fread(id, 1, 5, file);
	if (strcmp(id, "Adobe") != 0)
	{
		fseek(file, length - 5 - 2, SEEK_CUR);
		return;
	}

	fseek(file, 7, SEEK_CUR);
	uint8_t color_transform;
	fread(&color_transform, 1, 1, file);
	adobe_color_transform = color_transform;

	printf("[0x%04lX] APP14 (Adobe) Marker: ColorTransform = %d -> ", offset, color_transform);
	if (color_transform == 0)
		printf("CMYK\n");
	else if (color_transform == 1)
		printf("YCbCr\n");
	else if (color_transform == 2)
		printf("YCCK\n");
	else
		printf("Unknown\n");

	fseek(file, length - 5 - 2 - 8, SEEK_CUR);
}

void parse_jpeg_segments(FILE *file)
{
	rewind(file);
	uint8_t b1, b2;
	long offset = 0;
	read_two_bytes(file, &b1, &b2);
	offset += 2;
	printf("[0x%04lX] Marker: SOI (0xFFD8)\n", 0L);

	while (read_two_bytes(file, &b1, &b2))
	{
		offset += 2;

		if (b2 >= 0xC0 && b2 <= 0xCF && b2 != 0xC4 && b2 != 0xC8 && b2 != 0xCC)
		{
			detect_color_space_from_sof(file, offset, b2);
			continue;
		}

		if ((b2 >= 0xD0 && b2 <= 0xD7) || b2 == 0x01)
		{
			printf("[0x%04lX] Marker: 0xFF%02X (No payload)\n", offset - 2, b2);
			continue;
		}

		if (b2 == 0xD9)
		{
			printf("[0x%04lX] Marker: EOI (0xFFD9)\n", offset - 2);
			break;
		}

		uint8_t len_hi, len_lo;
		if (!read_two_bytes(file, &len_hi, &len_lo))
			break;
		uint16_t length = (len_hi << 8) | len_lo;
		offset += 2;

		if (b2 == 0xEE)
		{
			parse_app14_adobe(file, offset - 4, length);
		}
		else
		{
			printf("[0x%04lX] Marker: 0xFF%02X, Length: %d\n", offset - 4, b2, length);
			fseek(file, length - 2, SEEK_CUR);
		}

		offset += (length - 2);
	}

	if (sof_components == 1)
	{
		printf("-> Final Color Space: Grayscale\n");
	}
	else if (sof_components == 3)
	{
		printf("-> Final Color Space: ");
		if (adobe_color_transform == 1)
			printf("YCbCr\n");
		else
			printf("Possibly RGB (no Adobe marker)\n");
	}
	else if (sof_components == 4)
	{
		printf("-> Final Color Space: ");
		if (adobe_color_transform == 2)
			printf("YCCK\n");
		else if (adobe_color_transform == 0)
			printf("CMYK\n");
		else
			printf("Unknown 4-component (no Adobe marker)\n");
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Usage: %s <image.jpg>\n", argv[0]);
		return 1;
	}

	FILE *file = fopen(argv[1], "rb");
	if (!file)
	{
		perror("Cannot open file");
		return 1;
	}

	if (!is_valid_jpeg_header(file))
	{
		printf("Not a valid JPEG file (missing SOI/EOI markers).\n");
		fclose(file);
		return 1;
	}

	parse_jpeg_segments(file);
	fclose(file);
	return 0;
}
