# Project plan: zynq-jetson-stream

Milestones are checked every 2 weeks, starting Wednesday, 4 November 2026 (week 6).
Each milestone lists what must be working and demonstrable by that date.

| Week | Date | Milestone |
|------|------|-----------|
| 6  | Wed 4 Nov 2026  | Setup and first transfer |
| 8  | Wed 18 Nov 2026 | Complete baseline (all P7 requirements, plain TCP) |
| 10 | Wed 2 Dec 2026  | Display, CPU reference, measurements, custom UDP protocol |
| 12 | Wed 16 Dec 2026 | Streaming pipeline (improvement) |
| 14 | TBD (after winter break, check the calendar) | Final measurements, report, demo |

---

## Week 6: Setup and first transfer (Wed 4 Nov)

**Goal:** both boards run, talk to each other, and a file goes from the Zynq SD card to the Jetson.

- [ ] Private GitHub repo created, first commit pushed
- [ ] Decide the Zynq board (ZYBO or PYNQ) and how it runs: Linux (PetaLinux / PYNQ image) or bare-metal
- [ ] Linux booting on the Zynq from the SD card; SD card contents readable
- [ ] Jetson set up: CUDA toolkit works, a trivial kernel compiles with `nvcc` and runs
- [ ] Ethernet link: static IPs on both boards, `ping` works in both directions
- [ ] Cross-compiling (or compiling on-board) C code for the Zynq works
- [ ] First TCP transfer: Zynq reads the text file from SD and sends it, Jetson receives and prints it
- [ ] Message format drafted in `src/common/` (header with type, size; text vs. image)

**Demo:** text file sent Zynq → Jetson over TCP.

---

## Week 8: Complete baseline (Wed 18 Nov)

**Goal:** the whole P7 flow works end to end with plain TCP. This is the safety net: everything required for the base grade exists.

- [ ] Zynq sends both the image and the text file; Jetson replies with both results
- [ ] Text processing on Jetson: lowercase → uppercase
- [ ] Image loading/saving in a simple format (e.g. PPM/PGM or raw RGB) to avoid external libraries
- [ ] CUDA kernels (own implementations):
  - [ ] grayscale conversion
  - [ ] Gaussian blur
  - [ ] Sobel edge detection
  - [ ] binarization (threshold)
- [ ] Processed image and text sent back to the Zynq; text saved to SD / printed in terminal
- [ ] Processed image saved to SD on the Zynq (temporary, until the display works)
- [ ] Transfer validation: size check + checksum (e.g. CRC32) on both sides
- [ ] First rough timings: transfer time, kernel time
- [ ] **Present the improvement idea (streaming pipeline over custom UDP) to the teacher for approval**

**Demo:** image + text round trip, processed result visible as a file.

---

## Week 10: Display, CPU reference, measurements, custom protocol (Wed 2 Dec)

**Goal:** finish every remaining base requirement and start the improvement.

- [ ] Display the result image on the Zynq (HDMI / LCD / other available video interface, as agreed with the teacher)
- [ ] CPU reference implementations of all four image operations on the Jetson
- [ ] Output validation: compare GPU output with CPU output pixel by pixel (max difference / number of different pixels)
- [ ] Measurement setup: repeated runs, several image sizes (e.g. 640×480, 1280×720, 1920×1080, 4K), average and spread
- [ ] Separate timings: Zynq → Jetson transfer, host ↔ device copies, kernel time, Jetson → Zynq transfer, total
- [ ] Speedup CPU vs. GPU (kernel only and including memory copies)
- [ ] Custom reliable protocol over UDP (if approved):
  - [ ] packet header: sequence number, type, length, checksum
  - [ ] ACKs and retransmission on timeout
  - [ ] sliding window
  - [ ] whole-image transfer works over it and matches the TCP version byte for byte

**Demo:** image shown on the Zynq's display; table of CPU vs. GPU times.

---

## Week 12: Streaming pipeline (Wed 16 Dec)

**Goal:** the improvement works and shows a measurable benefit.

- [ ] Image split into strips of rows, each strip sent as soon as it is read
- [ ] Halo rows: each strip carries the extra border rows needed by Gaussian and Sobel
- [ ] Jetson processes each strip as soon as it arrives (CUDA streams), overlapping network and GPU work
- [ ] Results streamed back strip by strip and reassembled on the Zynq
- [ ] Output identical to the non-streaming version (validation)
- [ ] Packet loss test: drop packets on purpose and confirm retransmission still gives a correct result
- [ ] Optional extras, if time allows:
  - [ ] kernel fusion (grayscale + blur + Sobel in one kernel)
  - [ ] zero-copy / pinned memory on the Jetson (shared CPU/GPU RAM)
  - [ ] bit-packed binary output (1 bit per pixel) for the return transfer
- [ ] Comparison: total latency of the baseline (TCP, send all → process all → send back) vs. streaming pipeline

**Demo:** same image, baseline vs. streaming, with the latency difference.

---

## Week 14: Final measurements, report and demo (date TBD)

**Goal:** everything measured, documented and ready to present.

- [ ] Final measurement runs for all configurations and image sizes
- [ ] Charts: transfer time, processing time, total time, speedup
- [ ] Technical report: architecture, protocol design, CUDA kernels, implementation, tests, results and analysis
- [ ] README updated with build and run instructions for both boards
- [ ] Demo script rehearsed (boot both boards, run baseline, run streaming, show results)
- [ ] Code cleaned up, final commit

**Deliverables:** working demo, technical report, private repository with regular commits.

---

## Grading checklist (from the assignment)

| Requirement | Where it is covered |
|---|---|
| Working implementation | Weeks 6 to 8 |
| Effective GPU use (own CUDA kernels) | Week 8 |
| Result validation | Weeks 8 to 10 |
| Software reference implementation (CPU) | Week 10 |
| Optimization through parallelism / pipeline / memory | Week 12 |
| Execution time measurement (transfer + processing, compute + total) | Weeks 10 to 14 |
| Speedup calculation | Weeks 10 to 14 |
| Technical report and demo | Week 14 |
