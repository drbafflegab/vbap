![radial graph](graphs/radial.svg)

*Polar plot for the surround-5 layout showing per-speaker gain (linear) as a function of source azimuth (degrees).*

# Dr. Bafflegab’s Vector Base Amplitude Panner (VBAP)

[![CI](https://github.com/drbafflegab/vbap/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/drbafflegab/vbap/actions/workflows/ci.yml)

Dr. Bafflegab’s VBAP is a minimal, dependency‑free C17 implementation of 2D **Vector Base Amplitude Panning** (VBAP). It spatializes one or more audio sources across an arbitrary loudspeaker ring by computing per‑speaker gains, and is designed for real-time audio use, such as game engines, embedded DSP, and educational projects.

## Features

- **Tiny & portable:** One header + one source; cross-platform; no third-party dependencies; C++ compatible.
- **Real-time & embed-friendly:** Thread-safe opaque handle; no dynamic allocations; deterministic.
- **Equal-power panning:** Constant-loudness VBAP panning for arbitrary 2D loudspeaker layouts.
- **Batched API:** One‑time construction; processes many sources in one go with constant work per source.
- **Robust test suite:** Deterministic fuzzing, property checks, reference-law verification, threshold edge cases, and sanity vectors.
- **MIT Licence:** Free for commercial, open-source, and academic use.

> Even though the library is targeted for C17, it doesn't rely on advanced features and can be modified to compile on a C99 compiler with minimal effort.

## Theory

*Vector Base Amplitude Panning* (Pulkki, 1997) positions a virtual sound by driving the *nearest two* loudspeakers (in 2D) with gains chosen so the vector sum of the loudspeaker directions points toward the target. Compared to HRTF convolution, VBAP is lightweight and scales to many simultaneous sources while preserving directional cues on loudspeaker setups.

The library uses a 2D Cartesian coordinate system, with the listener sitting at the origin looking towards $`+x`$, and with $`+y`$ and $`-y`$ pointing left and right, respectively. Given a 2D source at position $`(x, y)`$, only its direction matters: vectors are internally normalised and their magnitude is ignored (the zero vector $`(0, 0)`$ is invalid).

Azimuth is measured from $`+x`$, with positive angles rotating towards $`+y`$ (anticlockwise): left is $`+90^\circ`$, right is $`-90^\circ`$, and behind is $`\pm180^\circ`$.

### References

- Pulkki, V. (1997). *Virtual Sound Source Positioning Using Vector Base Amplitude Panning*. **JAES**, 45(6), 456–466.
- Pulkki, V. (1998). *Creating Auditory Displays with Multiple Loudspeakers using VBAP*. **ICAD**.

## Installation & Integration

To integrate the library, copy [`drb-vbap.h`](drb-vbap.h) and [`drb-vbap.c`](drb-vbap.c) into your project and compile them with your C17 toolchain. The header file exposes a few functions:

- `drb_vbap_size(…)`: Returns the number of bytes you must provide to construct a VBAP instance.
- `drb_vbap_construct(…)`: Initializes a VBAP instance in user-provided memory.
- `drb_vbap_gain_matrix(…)`: Computes per-speaker gains for batches of 2D source positions.

See [`drb-vbap.h`](drb-vbap.h) for the full API documentation.

## Example

This example demonstrates how to compute VBAP gains for a set of source positions spaced evenly around a circle, using the standard 5.0 surround layout (the LFE is excluded). The output shows each source angle and the corresponding gain (in dB) for each of the five speakers.

1. Include the VBAP header:

    ```c
    #include "drb-vbap.h"
    ```

2. Allocate arrays for source positions (x, y coordinates) and computed gains:

    ```c
    enum { source_count = 36 + 1, speaker_count = 5 };

    float source_positions [source_count * 2];
    float speaker_gains [source_count * speaker_count];
    ```

3. Retrieve the built-in surround-5 layout and allocate memory for the VBAP instance:

    ```c
    char const * const tag = DRB_VBAP_LAYOUT_TAG_SURROUND_5;

    DrB_VBAP_Layout const * const layout = drb_vbap_builtin_layout(tag);

    void * const memory = malloc(drb_vbap_size(layout));
    ```

4. Construct the VBAP instance with error checking:

    ```c
    DrB_VBAP_Error error;

    DrB_VBAP const * const vbap = drb_vbap_construct(memory, layout, &error);

    if (vbap == NULL)
    {
        fprintf(stderr, "Error: %s.\n", drb_vbap_error_string(error));

        return EXIT_FAILURE;
    }
    ```

5. Generate source positions: 37 points evenly distributed around a circle (from -180° to +180°):

    ```c
    for (int_fast32_t source = 0; source < source_count; source++)
    {
        float const theta = (float)source * 2.0f * pi / (float)(source_count-1);

        source_positions[source * 2 + 0] = cosf(theta - pi);
        source_positions[source * 2 + 1] = sinf(theta - pi);
    }
    ```

6. Compute per-speaker gains for all sources in one batch:

    ```c
    drb_vbap_gain_matrix(vbap, source_positions, speaker_gains, source_count);
    ```

7. Print the results: for each source angle, show the gain (in dB) for each speaker:

    ```c
    for (int_fast32_t source = 0; source < source_count; source++)
    {
        float const theta = (float)source * 360.0f / (float)(source_count - 1);

        printf("%+4.0f°:", theta - 180.0f);

        for (int_fast32_t speaker = 0; speaker < speaker_count; speaker++)
        {
            int_fast32_t const index = source * speaker_count + speaker;

            printf(" %+6.1f dB", 20.0f * log10f(speaker_gains[index]));
        }

        printf("\n");
    }
    ```

8. Clean up:

    ```c
    free(memory);
    ```

Graph of the example output:

![example graph](graphs/example.svg)

## Building and Running the Tests

You need a [C17](https://en.wikipedia.org/wiki/C17_(C_standard_revision))-capable compiler (GCC/Clang/MSVC) and [CMake](https://cmake.org). From within your cloned repository follow these steps:

1. Configure the project and generate build files in the subdirectory `build`:

    ```bash
    cmake -B build
    ```

2. Build the project:

    ```bash
    cmake --build build
    ```

3. Run the [tests](tests):

    ```bash
    ctest --test-dir build
    ```

## Used by

- [MojoAL](https://github.com/icculus/MojoAL)
- [Simple DirectMedia Layer](https://www.libsdl.org)

Know another project? [Email me](mailto:drbafflegab@protonmail.com).

## Contributing

Contributions are welcome! Feel free to submit issues, feature requests, or pull requests.

## Licence

The project is released under the **[MIT](https://opensource.org/license/mit)** licence. You can use, modify, and distribute it freely for any purpose, subject to the terms in [LICENSE](LICENSE).
