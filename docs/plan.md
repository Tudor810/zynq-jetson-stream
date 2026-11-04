# Project plan: zynq-jetson-stream

| Week | Date | Milestone |
|------|------|-----------|
| 6  | Wed 4 Nov 2026  | GitHub, plan, project decision |
| 8  | Wed 18 Nov 2026 | Zybo ↔ Jetson communication over Ethernet (TCP), text in terminal, image on HDMI |
| 10 | Wed 2 Dec 2026  | CUDA processing on the Jetson and integration with week 8 (baseline complete) |
| 12 | Wed 16 Dec 2026 | Improvement (variants) |
| 14 | Wed 6 Jan 2027| Final measurements, report, demo |

---

## Week 6: GitHub, plan, project decision (Wed 4 Nov)

**Goal:** project skeleton and plan, key decisions made.

- [x] Private GitHub repo created, first commit pushed
- [x] Project layout created (`src/zynq/`, `src/jetson/`, `src/common/`, `docs/`)
- [x] Milestone plan written (`docs/plan.md`)
- [x] Project decision: P7 (Zybo Z7-20 ↔ Jetson Orin Nano)

---

## Week 8: Communication Zybo → Jetson → Zybo (Wed 18 Nov)

**Goal:** both boards run and exchange data over Ethernet with plain TCP. The Jetson only echoes the data back unchanged (no CUDA yet), which validates the whole path before processing is added. The text goes to the terminal and the image goes to the display.

**Setup**
- [ ] Zybo: Linux booting from the SD card (PetaLinux image from Digilent's BSP), SD card contents readable
- [ ] Zybo: how code is built (cross-compile on PC)
- [ ] Jetson set up: CUDA toolkit works, a trivial kernel compiles with `nvcc` and runs
- [ ] Ethernet link: static IPs on both boards, `ping` works in both directions

**Communication (TCP)**
- [ ] Message format in `src/common/` (header with type, size; text vs. image)
- [ ] Zybo reads the text file and the image from the SD card and sends them
- [ ] Jetson receives both and sends them back unchanged (echo)
- [ ] Text shown in the terminal on the Zybo
- [ ] Transfer validation: size check + checksum (e.g. CRC32) on both sides
- [ ] Image loading/saving in a simple format (PPM/PGM or raw RGB), no external libraries

**Display**
- [ ] Image shown through HDMI on the Zybo (needs fabric logic: VDMA + RGB-to-DVI, plus a Linux driver or device tree entry)
- [ ] **HDMI deadline: start of week 9.** If there is no image on screen by then, switch to the fallback: a small SPI LCD on the PS-connected Pmod, or saving the result to a file for now


**Demo:** text and image sent Zybo → Jetson → Zybo over TCP; text in the terminal; image on HDMI (or the agreed fallback).

---

## Week 10: CUDA processing and integration (Wed 2 Dec)

**Goal:** the whole P7 flow works end to end, validated and measured.

**Processing on the Jetson**
- [ ] Text: lowercase → uppercase
- [ ] CUDA kernels (own implementations):
  - [ ] grayscale conversion
  - [ ] Gaussian blur
  - [ ] Sobel edge detection
  - [ ] binarization (threshold)
- [ ] Processed image and text sent back to the Zybo
- [ ] Zybo shows the processed image and prints or saves the modified text (SD card or terminal)

**CPU reference and validation**
- [ ] CPU implementations of all four image operations on the Jetson
- [ ] Output validation: compare GPU output with CPU output pixel by pixel (max difference / number of different pixels)

**Measurements**
- [ ] Measurement setup: repeated runs, several image sizes (e.g. 640×480, 1280×720, 1920×1080, 4K), average and spread
- [ ] Separate timings: Jetson processing (host ↔ device copies, kernel time), Zybo round trip, and transfer estimated as round trip minus Jetson processing time (the two boards' clocks are not synchronized, so no cross-board timestamps)
- [ ] Speedup CPU vs. GPU (kernel only and including memory copies)

**Demo:** image + text round trip with processing, result shown on the Zybo, table of CPU vs. GPU times.

**Fallback:** if time runs out, the later measurement runs can finish in the first days of week 12.

---

## Week 12: Improvement, variants (Wed 16 Dec)

**Goal:** one or more improvements that work and can be shown. 

**Rules for any variant**
- Each improvement can be switched on or off (config flag), so the report can show baseline, each improvement alone, and all together
- Every new CUDA kernel gets a CPU reference and a validation against it
- Keep the Zybo side simple: results must also be shown in the terminal or as files, not only on HDMI

**Variant A: more speed in the transfer**
- [ ] Output bit-packed (1 bit per pixel) for the binarized image
- [ ] Jumbo frames (MTU 9000) and larger socket buffers, if the Zybo Ethernet driver supports them
- [ ] Streaming pipeline: image split into strips of rows, each strip sent as soon as it is read, with halo rows for Gaussian and Sobel, CUDA streams on the Jetson, results streamed back and reassembled; first over TCP, then compared with the baseline
- [ ] Custom reliable protocol over UDP, built in stages: header (sequence number, type, length, checksum) → stop-and-wait with ACKs and timeout retransmission → sliding window 
- [ ] Packet loss test: drop packets on purpose and check the result is still correct

**Variant B: faster CUDA kernels**
- [ ] Separable Gaussian blur (two 1D passes)
- [ ] Shared memory tiling for blur and Sobel
- [ ] Kernel fusion (grayscale + blur + Sobel + threshold)
- [ ] Pinned / zero-copy memory (the Orin Nano shares RAM between CPU and GPU)

**Variant C: extra functionality on the Jetson**
- [ ] Full Canny (non-maximum suppression and double-threshold hysteresis on top of blur and Sobel)
- [ ] Histogram equalization (histogram with `atomicAdd`, then remap)
- [ ] Morphological operations (erosion, dilation) on the binarized image
- [ ] Object counting via connected components on the binarized image (the Jetson returns a number as well as an image)
- [ ] Text statistics (letter frequency, word count) computed on the GPU
- [ ] Optional, more ambitious: median filter, Harris corners, Hough lines

**Variant D: using the returned results on the Zybo**
- [ ] Configurable pipeline: Zybo buttons/switches pick the operations and parameters (threshold, sigma), sent to the Jetson in a control message
- [ ] Image sequence from the SD card with motion detection (frame difference with a threshold), shown as a short sequence
- [ ] Overlay: the Zybo draws the detected edges or objects over the original image

**Demo:** the chosen improvements shown against the baseline, with the difference in times or in functionality.

---

## Week 14: Final measurements, report and demo (date TBD)

**Goal:** everything measured, documented and ready to present. 

- [ ] Final measurement runs for all configurations and image sizes (baseline, each improvement alone, all together)
- [ ] Charts: transfer time, processing time, total time, speedup
- [ ] Technical report: architecture, protocol design, CUDA kernels, implementation, tests, results and analysis 
- [ ] README updated with build and run instructions for both boards
- [ ] Code cleaned up, final commit

**Deliverables:** working demo, technical report, private repository with regular commits.


