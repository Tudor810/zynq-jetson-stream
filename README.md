# zynq-jetson-stream

SSC (Structura Sistemelor de Calcul) project, P7: Ethernet communication between a Zynq board (ZYBO / PYNQ) and an NVIDIA Jetson Orin Nano, with image processing accelerated by custom CUDA kernels.

## What it does

1. **Zynq** reads an image and a text file from the SD card and sends them to the Jetson over Ethernet.
2. **Jetson** converts the text to uppercase and runs at least three image operations with custom CUDA kernels: grayscale, Gaussian blur, Sobel edge detection and binarization.
3. Results go back to the **Zynq**, which displays the image and saves the text to the SD card or prints it in a terminal.
4. Transfer time and Jetson processing time are measured, and the CUDA kernels are compared with equivalent CPU implementations.

**Planned improvement (pending teacher approval):** a streaming pipeline over a custom reliable protocol on UDP. The image is sent in strips of rows, so network transfer and GPU processing overlap.

## Layout

```
src/
  zynq/     Zynq side: SD card I/O, network client, display
  jetson/   Jetson side: network server, CUDA kernels, CPU reference
  common/   Shared protocol definitions (message format, headers)
docs/
  plan.md   Milestone plan, weeks 6 to 14
```

## Hardware

- Zynq: ZYBO / PYNQ (TBD)
- NVIDIA Jetson Orin Nano
- Ethernet cable (direct link or switch)
