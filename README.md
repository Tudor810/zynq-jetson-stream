# zynq-jetson-stream

SSC (Structura Sistemelor de Calcul) project, P7: Ethernet communication between a Zybo Z7-20 (Zynq-7020) and an NVIDIA Jetson Orin Nano, with image processing accelerated by custom CUDA kernels.

## What it does

1. The **Zybo** reads an image (PPM/PGM) and a text file from its SD card and sends them to the Jetson over Ethernet (TCP).
2. The **Jetson** converts the text to uppercase and processes the image with custom CUDA kernels: grayscale, Gaussian blur, Sobel edge detection and binarization.
3. The results go back to the **Zybo**, which shows the image on HDMI and prints or saves the text.
4. Transfer time and Jetson processing time are measured, and the CUDA kernels are compared with equivalent CPU implementations.

**Improvement (for the top grade):** variants are listed in `docs/plan.md` (faster transfer, faster kernels, extra functionality on the Jetson, or extra use of the results on the Zybo). The choice is pending the teacher's answer.

## Status

- [x] File I/O, PPM/PGM loading and saving
- [x] Message protocol with a fixed 32-byte header and CRC32
- [x] Client: sends the text and the image, receives and saves the results
- [ ] Server: moving to the new protocol (text uppercase, image echo)
- [ ] CUDA kernels and CPU reference implementations
- [ ] HDMI output on the Zybo (results are saved to a file for now)
- [ ] Measurements and report

## Layout

```
src/
  common/     code used by both programs
    errors      error codes and messages
    fileio      whole-file read/write (the SD card is a normal filesystem)
    image       in-memory image, PPM/PGM parse/load/save
    netio       sendn/recvn: send/receive exactly n bytes over TCP
    protocol    message header, CRC32, proto_send/proto_recv
  zynq/
    client.c    Zybo side: reads the files, sends them, receives the results
  jetson/
    server.c    Jetson side: receives, processes, replies
resources/    test inputs (test.ppm, test.txt)
docs/
  plan.md     milestone plan, weeks 6 to 14
Makefile
```

## Protocol

Every message is a 32-byte header followed by the payload. All header fields are big-endian.

| Offset | Size | Field            | Meaning                                       |
|--------|------|------------------|-----------------------------------------------|
| 0      | 4    | `magic`          | `0x5A4A5331` ("ZJS1")                          |
| 4      | 2    | `version`        | protocol version (1)                          |
| 6      | 2    | `type`           | TEXT, IMAGE, TEXT_RESULT, IMAGE_RESULT, ERROR |
| 8      | 4    | `payload_size`   | bytes after the header (max 64 MB)            |
| 12     | 4    | `width`          | image width (0 for text)                      |
| 16     | 4    | `height`         | image height (0 for text)                     |
| 20     | 4    | `channels`       | 1 = gray, 3 = RGB (0 for text)                |
| 24     | 4    | `crc32`          | CRC32 of the payload                          |
| 28     | 4    | `server_time_us` | server processing time, in replies            |

The receiver validates the header before allocating anything and checks the CRC32 after receiving the payload.

## Build

```
make          # both programs (laptop)
make zynq     # build/zynq_client   (on the Zybo)
make jetson   # build/jetson_server (on the Jetson)
make clean
```

## Run

Start the server first (it listens on port 5000):

```
./build/jetson_server
```

Then the client:

```
./build/zynq_client <image.ppm> <text.txt> <server ip> <port>
```

Example on one machine:

```
./build/zynq_client resources/test.ppm resources/test.txt 127.0.0.1 5000
```

To make a test image from any picture: `convert photo.jpg -resize 640x480 test.ppm` (ImageMagick).

## Hardware and software

- Zybo Z7-20, running Digilent's PetaLinux image (2022.1 BSP), booted from the SD card
- NVIDIA Jetson Orin Nano developer kit, JetPack 6 (L4T R36.3)
- Ethernet cable between the two boards (direct link, static IPs)