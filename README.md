# Dr. Bafflegab's Vector Base Amplitude Panner (VBAP)

Dr. Bafflegab's VBAP is an open-source library implementing 2-D Vector Base Amplitude Panning (VBAP), a technique for spatializing audio sources across a speaker array using gain interpolation. VBAP is a cornerstone of many spatial audio systems, enabling precise positioning of sound sources in virtual environments.

## Features

- *Lightweight*: Minimal library with low memory usage per instance.
- *Real-time*: Safe to use in a real-time audio processing context.
- *Portable*: Written in modern, portable C17 for cross-platform compatibility.
- *Efficient*: Optimized gain computation for 2D speaker setups.

## Getting Started

To integrate the library into your project, copy the following files into your source tree:

- [`drb-vbap.h`](drb-vbap.h)
- [`drb-vbap.c`](drb-vbap.c)

The header file exposes a struct and a few functions:

- `DrB_VBAP_2D`: Opaque structure representing a 2-D VBAP instance.
- `drb_vbap_2d_size`: Computes the memory size required to construct a 2-D VBAP instance.
- `drb_vbap_2d_construct`: Constructs a new 2-D VBAP instance.
- `drb_vbap_2d_compute_gains`: Computes the per-speaker gain for a list of source positions.

See [`example.c`](example.c) for a detailed example.

To run the example:

- Build the example binary using `make example`.
- Execute the example with `./example`.

## Testing

Basic tests are included to ensure correctness.

To run the tests:

- Build the test binary using `make tests`.
- Execute the tests with `./tests`.

## Contributing

Contributions are welcome! Feel free to submit issues, feature requests, or pull requests to the repository.

## License

The project is licensed under the [CC0 1.0 UNIVERSAL](https://creativecommons.org/publicdomain/zero/1.0/). See [LICENSE.txt](LICENSE.txt).
