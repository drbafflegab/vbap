# Dr. Bafflegab's Vector Base Amplitude Panner (VBAP)

[![CI](https://github.com/drbafflegab/vbap/actions/workflows/ci.yml/badge.svg?branch=0.1.0)](https://github.com/drbafflegab/vbap/actions/workflows/ci.yml)

Dr. Bafflegab’s VBAP is a minimalistic, dependency‑free C17 implementation of 2‑D **Vector Base Amplitude Panning** (VBAP). It spatializes one or more audio sources across an arbitrary loudspeaker ring by computing per‑speaker gains, and is designed for real-time audio use, such as game engines, embedded DSP, and educational projects.

## Features

- **Mathematically sound**: VBAP panning for arbitrary 2‑D loudspeaker layouts.
- **Fast**: One‑time construction; **O(1)** work per source at render time.
- **Minimal, portable C17**: One header + one source, no third‑party dependencies.
- **Embed‑friendly**: Opaque handle, no hidden allocations, C/C++ compatible.
- **Public domain (CC0)**: Free for commercial, open‑source, and academic use.

## What is VBAP?

*Vector Base Amplitude Panning* (Pulkki, 1997) positions a virtual sound by driving the *nearest two* loudspeakers (in 2‑D) with gains chosen so the vector sum of the loudspeaker directions points toward the target. Compared to HRTF convolution, VBAP is lightweight and scales to many simultaneous sources while preserving directional cues on loudspeaker setups.

### References

- Pulkki, V. (1997). *Virtual Sound Source Positioning Using Vector Base Amplitude Panning*. **JAES**, 45(6), 456–466.  
- Pulkki, V. (1998). *Creating Auditory Displays with Multiple Loudspeakers using VBAP*. **ICAD**.

## Example

```c
#include "drb-vbap.h"

#include <stdio.h>
#include <stdlib.h>

extern int main (void)
{
    static int const resolution = 36; // 10° steps.
    static int const speaker_positions [] = { 0, 3, 11, 25, 33 }; // 0°, 30°, ….
    static int const speaker_count = sizeof(speakers) / sizeof(speakers[0]);

    void * const memory = malloc(drb_vbap_2d_size(resolution, speaker_count));

    DrB_VBAP_2D_Error error;

    DrB_VBAP_2D * const vbap =
     drb_vbap_2d_construct(memory, resolution, speakers, speaker_count, &error);

    if (vbap == NULL)
    {
        fprintf(stderr, "VBAP construction failed: error code %d\n", error);

        return EXIT_FAILURE;
    }

    float gains [5];

    // Pan a single source around on the unit circle in 5° steps.
    for (int angle = 0; angle <= 72; angle++)
    {
        float const sources [1] = { (float)angle * 2.0f * 3.14159265 / 72.0f };

        drb_vbap_2d_compute_gains(vbap, sources, 1, gains);

        printf("%3d° :", angle);

        for (int s = 0; s < speaker_count; s++) { printf(" %6.3f", gains[s]); }

        printf("\n");
    }

    free(memory);

    return EXIT_SUCCESS;
}
```

## Getting Started

To integrate the library, copy [`drb-vbap.h`](drb-vbap.h) and [`drb-vbap.c`](drb-vbap.c) into your project and compile them with your C17 toolchain. The header file exposes a few functions:

- `drb_vbap_2d_size(…)`: Returns the number of bytes you must provide to construct a VBAP instance.
- `drb_vbap_2d_construct(…)`: Initializes a VBAP instance in user-provided memory. Returns `NULL` on error.
- `drb_vbap_2d_compute_gains(…)`: Computes per-speaker gains for one or more source angles.

See [`drb-vbap.h`](./drb-vbap.h) for the full API documentation.

## Building & Testing

You need a [C17](https://en.wikipedia.org/wiki/C17_(C_standard_revision))‑capable compiler (GCC/Clang/MSVC) and [CMake](https://cmake.org). From within your cloned repo:

```bash
cmake -B build
cmake --build build
ctest --test-dir build
```

## Used by

- [MojoAL](https://github.com/icculus/MojoAL)
- [Simple DirectMedia Layer](https://www.libsdl.org)

Know another project? [Email me](mailto:drbafflegab@protonmail.com).

## Contributing

Contributions are welcome! Feel free to submit issues, feature requests, or pull requests.

## License

The project is released under the **[CC0 1.0 UNIVERSAL](https://creativecommons.org/publicdomain/zero/1.0/)** license. This work is dedicated to the public domain. You can use, modify, and distribute it freely, without restriction. See [LICENSE.txt](LICENSE.txt) for details.
