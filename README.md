# PowerEQ Master

**PowerEQ Master** is a production-grade Android audio processing application featuring a high-performance, modular C++ DSP engine. It provides professional-level equalization and spatial audio effects with a focus on real-time safety and low-latency performance.

---

## 🚀 Key Features

- **12-Band Parametric Equalizer**: Fully customizable EQ with support for 7 filter types:
  - Peak (Bell)
  - Low Shelf / High Shelf
  - Low Pass / High Pass
  - Band Pass
  - Notch
- **3D Surround Engine**: Custom C++ spatializer featuring:
  - **Stereo Widening**: Advanced Mid/Side (M/S) soundstage manipulation.
  - **Crossfeed**: Simulation of speaker-like listening for headphones to reduce ear fatigue.
- **Interactive EQ Graph**: Real-time visual feedback using native BiQuad mathematics to ensure perfect visual-to-audio synchronization.
- **Low Latency**: Powered by **Google Oboe**, utilizing the AAudio/OpenSL ES paths for pro-audio performance.
- **Preset System**: Save and restore custom audio profiles with real-time auto-saving and factory reset capabilities.

---

## 🛠 Architectural Highlights (RP-001)

This project follows a strict **Real-Time Safe** modular architecture:

1.  **Modular DSP Pipeline**: Processing stages (Preamp, HPF, EQ, Spatializer) are encapsulated in independent modules managed by a pre-compiled execution plan.
2.  **Thread Isolation**: All audio processing occurs on a high-priority audio thread. Thread safety is guaranteed via a **Lock-Free SPSC (Single-Producer Single-Consumer) Queue** for parameter updates.
3.  **Deterministic Execution**: The engine uses a pre-allocated **Buffer Arena** and O(1) **Module Registry** for parameter routing, ensuring no heap allocations or locks occur during the audio callback.
4.  **Native C++ Core**: The heavy lifting is done in C++17, exposed to a modern **Jetpack Compose** UI via a shallow JNI bridge.

---

## 📁 Project Structure

- `app/src/main/cpp/`: Native C++ DSP Engine, Filter math, and Oboe implementation.
- `app/src/main/java/.../dsp/`: Kotlin DSP configuration layer and parameter management.
- `app/src/main/java/.../ui/`: Reactive UI built with Jetpack Compose.
- `docs/architecture/`: In-depth Architectural Decision Records (ADRs) and design specs.

---

## 🛠 Build Requirements

- Android Studio Koala+
- NDK 27.0.12077973
- CMake 3.22.1
- JDK 17+

---

## 📄 Documentation

- [Real-Time Safety Contract](REALTIME_SAFETY.md)
- [Architecture Summary](ArchitectureSummary.md)
- [RP-001 Foundation Overview](docs/architecture/RP001_OVERVIEW.md)
