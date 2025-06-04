# JPEG Parser

This repository contains a simple JPEG marker parser written in C.

## Building

Use `gcc` to compile the parser:

```bash
gcc jpeg_parser.c -o jpeg_parser
```

This will produce an executable named `jpeg_parser`.

## Usage

Run the parser by providing a path to a JPEG image. Example images are provided in the `images/` directory:

```bash
./jpeg_parser images/rgb.jpeg
./jpeg_parser images/cmyk.jpeg
```

The program prints out JPEG markers and attempts to infer the color space.
