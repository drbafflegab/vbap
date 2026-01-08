# Frequently Asked Questions

## What is VBAP and when should I use it?

**Vector Base Amplitude Panning (VBAP)** is a spatial audio technique that positions virtual sound sources across multiple loudspeakers by computing per-speaker gains. Use VBAP when you need:

- **Multi-speaker audio**: Game engines, art installations, planetariums, or any setup with 3+ speakers arranged in a circle around the listener.
- **Lightweight spatialization**: VBAP is computationally cheaper than HRTF convolution and scales well to many simultaneous sources.
- **Real-time performance**: Suitable for embedded systems, mobile devices, and performance-critical applications.

VBAP is *not* for headphone audio (use binaural/HRTF instead) or simple stereo panning (standard pan laws suffice).

### How does VBAP differ from binaural/HRTF audio?

- **VBAP** is for **loudspeaker** setups (3+ speakers in a room). It computes gains to distribute sound across speakers, creating the illusion of directional sound in physical space.
- **HRTF** (Head-Related Transfer Function) is for **headphones**. It uses convolution with head-related impulse responses to simulate 3D sound within the listener's head.

**Key differences:**
- VBAP is computationally cheaper (simple gain calculation vs. convolution).
- VBAP requires actual loudspeakers; HRTF works with any headphones.
- VBAP doesn't require head-tracking; HRTF benefits greatly from it.

## Why only 2D? Can this library do 3D or height speakers?

This library implements **2D VBAP**, which operates on the horizontal plane (azimuth only). It does *not* support elevation/height speakers (e.g., Dolby Atmos ceiling speakers).

**Why 2D?**
- Many use cases don't need elevation: game audio, DJ booths, art installations, and most music playback.
- 2D VBAP is simpler, faster, and covers the majority of spatial audio needs.
- Pulkki's original 3D VBAP algorithm exists, but implementing it would significantly increase complexity.

If you need 3D/height support, consider other libraries or implement Pulkki's 3D extension yourself.

### What are the speaker configuration requirements?

- **Minimum:** 3 speakers
- **Maximum:** 256 speakers
- **Arrangement:** Speakers must form a ring around the listener in the horizontal plane (azimuth).
- **Spacing constraints:**
  - Minimum span between adjacent speakers: **5°**
  - Maximum span between adjacent speakers: **175°**

These constraints ensure VBAP can always find a valid speaker pair for any source direction. Layouts violating these rules will fail validation during construction.

## Is this fast enough for real-time audio?

**Yes!** The library is designed for real-time use:

- **O(1) complexity per source**: Gain computation uses a pre-computed lookup table. No loops over speakers.
- **No allocations**: Memory is allocated once at construction. `drb_vbap_gain_matrix()` does zero allocations.
- **Batch processing**: Process many sources in one call for better cache performance.
- **Battle-tested**: Already used in production by [MojoAL](https://github.com/icculus/MojoAL) and [Simple DirectMedia Layer](https://www.libsdl.org).

**Performance example:** On modern hardware, you can easily spatialize 64+ sources per audio callback with negligible CPU usage.

## What coordinate system does this use?

The library uses a **2D Cartesian coordinate system** with the listener at the origin:

- **Origin:** Listener position `(0, 0)`
- **+x axis:** Points forward (0° azimuth)
- **+y axis:** Points left (90° azimuth)
- **Rotation:** Counter-clockwise is positive

**Converting angles to coordinates:**
```c
float angle_radians = angle_degrees * (3.14159f / 180.0f);
float x = cosf(angle_radians);
float y = sinf(angle_radians);
```

**Example angles:**
- 0° (front): `(1, 0)`
- 90° (left): `(0, 1)`
- 180° (back): `(-1, 0)`
- -90° or 270° (right): `(0, -1)`

Note: Source magnitude doesn't matter—VBAP only uses direction. Both `(1, 0)` and `(5, 0)` produce identical results.

### What speaker layouts are included?

The library includes 6 built-in layouts accessible via `drb_vbap_builtin_layout()`:

| Layout Tag | Speakers | Description |
|------------|----------|-------------|
| `DRB_VBAP_LAYOUT_TAG_SURROUND_5` | 5 | ITU-R BS.775-3 standard: L (+30°), C (0°), R (-30°), Ls (+110°), Rs (-110°) |
| `DRB_VBAP_LAYOUT_TAG_SURROUND_7` | 7 | 7.1 layout: adds Lb (+150°), Rb (-150°) |
| `DRB_VBAP_LAYOUT_TAG_QUADROPHONIC` | 4 | Square: ±45°, ±135° |
| `DRB_VBAP_LAYOUT_TAG_HEXAPHONIC` | 6 | Hexagon: 0°, ±60°, ±120°, 180° |
| `DRB_VBAP_LAYOUT_TAG_OCTOPHONIC` | 8 | Octagon: 0°, ±45°, ±90°, ±135°, 180° |
| `DRB_VBAP_LAYOUT_TAG_DODECAPHONIC` | 12 | 12 speakers evenly spaced (30° intervals) |

**Note:** These layouts *do not* include the LFE/subwoofer channel. VBAP is for directional panning; subwoofers handle non-directional low-frequency content separately.

## Can I define custom speaker layouts?

**Yes!** Create a `DrB_VBAP_Layout` structure:

```c
// Example: 3 speakers at 0°, 120°, 240° (triangle)
int32_t steps [] = { 0, 4, 8 };  // 0/12 = 0°, 4/12 = 120°, 8/12 = 240°

DrB_VBAP_Layout custom_layout =
{
    .resolution = 12,           // Grid resolution (360° / 12 = 30° per step)
    .speaker_steps = steps,     // Speaker positions as grid indices
    .speaker_count = 3          // Number of speakers
};
```

**Requirements:**
- `resolution` must be in `[1, 65536]`
- `speaker_count` must be in `[3, 256]`
- `speaker_steps` must be sorted in ascending order with no duplicates
- Adjacent speaker spans must be in `[5°, 175°]`

See [`drb-vbap.h`](drb-vbap.h) for full `DrB_VBAP_Layout` documentation.

## What happens if my source is at (0, 0)?

**Undefined behavior.** Passing a zero vector `(0, 0)` causes division by zero in the gain calculation. The implementation *does not* check for this condition (for performance reasons).

**Your responsibility:** Guard against zero vectors before calling `drb_vbap_gain_matrix()`:

```c
float x = source_x;
float y = source_y;
float magnitude = sqrtf(x * x + y * y);

if (magnitude < 1e-6f)
{
    // Handle zero vector: use a default direction or skip this source
    x = 1.0f;
    y = 0.0f;
}

// Now safe to use (x, y)
```

Similarly, avoid `NaN` or `±Infinity` values—these also produce undefined behavior.

## How do I report bugs or request features?

- **Bugs & Features:** Open an issue on [GitHub](https://github.com/drbafflegab/vbap/issues)
- **Private inquiries:** Email [drbafflegab@protonmail.com](mailto:drbafflegab@protonmail.com)

**When reporting bugs, please include:**
- Minimal code to reproduce the issue
- Your platform (OS, compiler, CPU architecture)
- Expected vs. actual behavior

Contributions are welcome! See the [Contributing](#contributing) section.
