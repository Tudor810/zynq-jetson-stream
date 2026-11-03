# Project plan: zynq-jetson-stream

Milestones are checked every 2 weeks, starting Wednesday, 4 November 2026 (week 6).
Each milestone lists what must be working and demonstrable by that date.

| Week | Date | Milestone |
|------|------|-----------|
| 6  | Wed 4 Nov 2026  | Repo and roadmap |
| 8  | Wed 18 Nov 2026 | Setup and connection only |
| 10 | Wed 2 Dec 2026  | Complete project (all P7 requirements, plain TCP) |
| 12 | Wed 16 Dec 2026 | Improvement: custom UDP protocol and streaming pipeline |
| 14 | TBD (after winter break, check the calendar) | Final measurements, report, demo |

---

## Week 6: Repo and roadmap (Wed 4 Nov)

**Goal:** project skeleton and plan exist, key hardware decisions made.

- [x] Private GitHub repo created, first commit pushed
- [x] Project layout created (`src/zynq/`, `src/jetson/`, `src/common/`, `docs/`)
- [x] Milestone plan written (`docs/plan.md`)
- [x] Decide the Zynq board (ZYBO or PYNQ) and how it runs: Linux (PetaLinux / PYNQ image) or bare-metal

**Demo:** repo and plan.

---

## Week 8: Setup and connection only (Wed 18 Nov)

**Goal:** both boards run, talk to each other, and a text file goes from the Zynq SD card to the Jetson. The display path is checked early because it is the riskiest base requirement.

- [ ] Linux booting on the Zynq from the SD card; SD card contents readable
- [ ] Jetson set up: CUDA toolkit works, a trivial kernel compiles with `nvcc` and runs
- [ ] Ethernet link: static IPs on both boards, `ping` works in both directions
- [ ] Cross-compiling (or compiling on-board) C code for the Zynq works
- [ ] First TCP transfer: Zynq reads the text file from SD and sends it, Jetson receives and prints it
- [ ] Message format drafted in `src/common/` (header with type, size; text vs. image)
- [ ] Prep for week 10: find out which video output the Zynq can use (HDMI / LCD / other) and show a test pattern on it
- [ ] **Present the improvement idea (streaming pipeline over custom UDP) to the teacher for approval**

**Demo:** text file sent Zynq → Jetson over TCP; test pattern on the Zynq display (or a clear status on what is blocking it).

---

## Week 10: Complete project, plain TCP (Wed 2 Dec)

**Goal:** the whole P7 flow works end to end with plain TCP, validated and measured. This is the safety net: everything required for the base grade exists.

Suggested order inside the week: (1) round trip with a dummy kernel, (2) real CUDA kernels, (3) display, (4) CPU reference and validation, (5) measurements. Put timers into the code while writing the round trip, not afterwards.

**Round trip and I/O**
- [ ] Zynq sends both the image and the text file; Jetson replies with both results
- [ ] Text processing on Jetson: lowercase → uppercase
- [ ] Image loading/saving in a simple format (e.g. PPM/PGM or raw RGB) to avoid external libraries
- [ ] Processed image and text sent back to the Zynq; text saved to SD / printed in terminal
- [ ] Transfer validation: size check + checksum (e.g. CRC32) on both sides

**CUDA kernels (own implementations)**
- [ ] grayscale conversion
- [ ] Gaussian blur
- [ ] Sobel edge detection
- [ ] binarization (threshold)

**Display**
- [ ] Display the result image on the Zynq (HDMI / LCD / other available video interface, as agreed with the teacher)

**CPU reference and validation**
- [ ] CPU reference implementations of all four image operations on the Jetson
- [ ] Output validation: compare GPU output with CPU output pixel by pixel (max difference / number of different pixels)

**Measurements**
- [ ] Measurement setup: repeated runs, several image sizes (e.g. 640×480, 1280×720, 1920×1080, 4K), average and spread
- [ ] Separate timings: Zynq → Jetson transfer, host ↔ device copies, kernel time, Jetson → Zynq transfer, total
- [ ] Speedup CPU vs. GPU (kernel only and including memory copies)

**Wrap-up**
- [ ] Tag the repo `baseline-complete`

**Demo:** image + text round trip, result shown on the Zynq display, table of CPU vs. GPU times.

**Fallback:** if time runs out, the CPU reference and measurements can finish in the first days of week 12, since they do not depend on the improvement.

---

## Week 12: Improvement, custom UDP and streaming (Wed 16 Dec)

**Goal:** the improvement works and shows a measurable benefit.

**Custom reliable protocol over UDP (build in stages)**
- [ ] packet header: sequence number, type, length, checksum
- [ ] stop-and-wait: ACKs and retransmission on timeout
- [ ] whole-image transfer works over it and matches the TCP version byte for byte
- [ ] sliding window (stretch goal; streaming with stop-and-wait already shows the overlap benefit)

**Streaming pipeline**
- [ ] Image split into strips of rows, each strip sent as soon as it is read
- [ ] Halo rows: each strip carries the extra border rows needed by Gaussian and Sobel
- [ ] Jetson processes each strip as soon as it arrives (CUDA streams), overlapping network and GPU work
- [ ] Results streamed back strip by strip and reassembled on the Zynq

**Validation and comparison**
- [ ] Output identical to the non-streaming version (validation)
- [ ] Packet loss test: drop packets on purpose and confirm retransmission still gives a correct result
- [ ] Comparison: total latency of the baseline (TCP, send all → process all → send back) vs. streaming pipeline

**Optional extras, only if time allows**
- [ ] kernel fusion (grayscale + blur + Sobel in one kernel)
- [ ] zero-copy / pinned memory on the Jetson (shared CPU/GPU RAM)
- [ ] bit-packed binary output (1 bit per pixel) for the return transfer

**Demo:** same image, baseline vs. streaming, with the latency difference.

---

## Week 14: Final measurements, report and demo (date TBD)

**Goal:** everything measured, documented and ready to present. If the date turns out to be late January, use the extra buffer for the optional extras from week 12.

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
| Working implementation | Weeks 8 to 10 |
| Effective GPU use (own CUDA kernels) | Week 10 |
| Result validation | Weeks 10 to 12 |
| Software reference implementation (CPU) | Week 10 (fallback: start of week 12) |
| Optimization through parallelism / pipeline / memory | Week 12 |
| Execution time measurement (transfer + processing, compute + total) | Weeks 10 to 14 |
| Speedup calculation | Weeks 10 to 14 |
| Technical report and demo | Week 14 |
