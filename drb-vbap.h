// Dr. Bafflegab's Vector Base Amplitude Panner
// https://github.com/drbafflegab/vbap
// SPDX-License-Identifier: MIT

// Minimal, dependency-free C17 library for vector base amplitude panning. Com-
// putes gains for batches of 2D source positions in a Cartesian listener frame.
// Has optional SSE2/NEON paths chosen at compile time with a scalar fallback.
// All functions are thread-safe, real-time-safe, and free from dynamic mallocs.

#ifndef DRB_VBAP_H
#define DRB_VBAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DRB_VBAP_VERSION_MAJOR 0
#define DRB_VBAP_VERSION_MINOR 1
#define DRB_VBAP_VERSION_PATCH 0

#if !defined(DRB_VBAP_API)
#  define DRB_VBAP_API extern
#endif

#ifdef __cplusplus
#  define DRB_VBAP_RESTRICT
#else
#  define DRB_VBAP_RESTRICT restrict
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// `drb_vbap_version(major, minor, patch)`:
//
// Retrieves the semantic version of the library: MAJOR.MINOR.PATCH.
//
// Parameters:
//
// - `major`: Optional pointer to receive the major version.
// - `minor`: Optional pointer to receive the minor version.
// - `patch`: Optional pointer to receive the patch version.
//
// For each parameter, the corresponding version component is written only if
// the pointer is non-null. Null pointers are ignored.
//
// Assumed preconditions (not checked; violation causes undefined behaviour):
//
// - `major`, `minor`, and `patch` are distinct pointers.

DRB_VBAP_API void drb_vbap_version
    (
        int32_t * DRB_VBAP_RESTRICT major,
        int32_t * DRB_VBAP_RESTRICT minor,
        int32_t * DRB_VBAP_RESTRICT patch
    );

// `DrB_VBAP_Error`:
//
// Error code indicating why construction of a VBAP instance failed.

typedef int32_t DrB_VBAP_Error;

// Values for `DrB_VBAP_Error`:
//
// - `drb_vbap_error_null_pointer`: A required pointer is null.
// - `drb_vbap_error_misaligned_pointer`: A provided pointer is misaligned.
// - `drb_vbap_error_invalid_layout`: The layout is invalid.

enum
{
    drb_vbap_error_null_pointer = 1,
    drb_vbap_error_misaligned_pointer = 2,
    drb_vbap_error_invalid_layout = 3
};

// `drb_vbap_error_string(error)`:
//
// Converts an error code into a human-readable string with static storage.
//
// Parameters:
//
// - `error`: The error code to convert.
//
// Returns a pointer to a null-terminated string describing the error. For
// unrecognized codes, "unknown" is returned. The string has static storage
// duration and must not be modified or freed. Never returns null.

DRB_VBAP_API char const * drb_vbap_error_string
    (
        DrB_VBAP_Error error
    );

// `DRB_VBAP_MAX_RESOLUTION`:
//
// Maximum resolution in a setup.

#define DRB_VBAP_MAX_RESOLUTION 65536

// `DRB_VBAP_MAX_SPEAKER_COUNT`:
//
// Maximum speaker count in a setup.

#define DRB_VBAP_MAX_SPEAKER_COUNT 256

// `DRB_VBAP_MIN_SPAN`/`DRB_VBAP_MAX_SPAN`:
//
// Minimum and maximum span between two speakers in degrees.

#define DRB_VBAP_MIN_SPAN 5
#define DRB_VBAP_MAX_SPAN 175

// `DrB_VBAP_Layout`:
//
// Struct describing the speaker layout used for constructing the VBAP instance.
//
// The library uses a 2D Cartesian coordinate system, with the listener sitting
// at the origin looking towards `+x`, and with `+y` and `-y` pointing towards
// the left and right directions, respectively.
//
// The `resolution` property divides the unit circle into evenly spaced angular
// grid steps. Step `0` corresponds to `0°` (`x = 1, y = 0`). Steps increase
// counter-clockwise in sizes of `360°`/`resolution`. Speaker positions index
// into this grid using integer indices (steps). All speaker indices must be
// within `[0, resolution - 1]`, unique, and sorted in strict ascending order.
//
// Use the following formula to convert a step to an angle:
//
//            step
//     θ = ---------- × 360°.
//         resolution
//
// For example, for a resolution of `8` and the speaker array `[1, 3, 5, 7]`,
// the speakers will be positioned at `45°`, `135°`, `225°`, and `315°` angles:
//
//                 y
//
//     2nd spk.    ^     1st spk.
//         \       |       /
//           \     |     /
//             \   |   /
//               \ | /
//    ------------ * -----------> x (front)
//               / | \
//             /   |   \
//           /     |     \
//         /       |       \
//     3rd spk.    |     4th spk.
//
// Members:
//
// - `resolution`: Number of steps in the panning grid.
// - `speaker_steps`: Pointer to an array of speaker steps.
// - `speaker_count`: Number of speakers in the layout.
//
// Required conditions for a layout to be valid:
//
// - `resolution` is positive and bounded by `DRB_VBAP_MAX_RESOLUTION`.
// - All speaker steps are in-range, unique, and sorted in ascending order.
// - The span between any adjacent speakers is within the min/max span limit.
// - `speaker_count` is positive and bounded by `DRB_VBAP_MAX_SPEAKER_COUNT`.
// - `speaker_steps` contains at least `speaker_count` speakers. (Not checked.)

typedef struct
{
    int32_t resolution;
    int32_t const * speaker_steps;
    int32_t speaker_count;
}
DrB_VBAP_Layout;

// `drb_vbap_builtin_layout(tag)`:
//
// Returns a pointer to a built-in speaker layout described by `tag`.
//
// The returned `DrB_VBAP_Layout` and its `speaker_steps` array have static
// storage duration. They must not be modified or freed by the caller. The
// pointer remains valid for the lifetime of the process.
//
// Recognised tags are the `DRB_VBAP_LAYOUT_TAG_*` constants below. Tags are
// case-sensitive. If `tag` is null or not recognised, this function returns
// null.

DRB_VBAP_API DrB_VBAP_Layout const * drb_vbap_builtin_layout
    (
        char const * tag
    );

// `DRB_VBAP_LAYOUT_TAG_*`:
//
// Built-in layout tags accepted by `drb_vbap_builtin_layout`:
//
// - "surround-5": L / C / R / Ls / Rs.
// - "surround-7": L / C / R / Ls / Rs / Lb / Rb.
// - "quadrophonic": `4` speakers equally spaced at `±45°`, `±135°`.
// - "hexagonal": `6` speakers equally spaced at `0°`, `±60°`, `±120°`, `180°`.
// - "octophonic": `8` speakers equally spaced at 0°, ±45°, ±90°, ±135°, 180°.
// - "dodecaphonic": `12` speakers equally spaced  at 0°, ±30°, ±60°, ..., 180°.
//
// The surround layouts use conventional loudspeaker names and angles:
//
// - `L`: Left speaker, positioned at +30°.
// - `R`: Right speaker, positioned at -30°.
// - `C`: Centre speaker, positioned at 0°.
// - `B`: Back speaker, positioned at 180°.
// - `Ls`: Left surround speaker, positioned at +110°.
// - `Rs`: Right surround speaker, positioned at -110°.
// - `Lb`: Left back speaker, positioned at +150°.
// - `Rb`: Right back speaker, positioned at -150°.
//
// Angles are in degrees, 0° points to +x, and counter-clockwise is positive.
//
// The 5‑channel angles follow ITU‑R BS.775‑3 where applicable. 7.1 layouts are
// not explicitly defined in BS.775, but the rear loudspeaker angles follow
// common industry practice (e.g., Dolby/CEDIA/CTA guidance). The LFE/Subwoofer
// channel is not included as VBAP operates only on programme channels.

#define DRB_VBAP_LAYOUT_TAG_SURROUND_5 "surround-5"
#define DRB_VBAP_LAYOUT_TAG_SURROUND_7 "surround-7"
#define DRB_VBAP_LAYOUT_TAG_QUADROPHONIC "quadrophonic"
#define DRB_VBAP_LAYOUT_TAG_HEXAPHONIC "hexaphonic"
#define DRB_VBAP_LAYOUT_TAG_OCTOPHONIC "octophonic"
#define DRB_VBAP_LAYOUT_TAG_DODECAPHONIC "dodecaphonic"

// `DrB_VBAP`:
//
// Opaque structure representing a VBAP instance.
//
// Instances are immutable after successful construction.

typedef struct DrB_VBAP DrB_VBAP;

// `drb_vbap_alignment()`
//
// Returns the minimum required alignment to construct a VBAP instance.
//
// The return value is hardcoded to the alignment of `max_align_t`. Use it if
// you use custom allocators. On modern platforms, `malloc` and `new` satisfy
// this alignment requirement.

DRB_VBAP_API size_t drb_vbap_alignment
    (
        void
    );

// `drb_vbap_size(layout)`:
//
// Computes the memory size required to construct a VBAP instance.
//
// Parameters:
//
// - `layout`: Pointer to the layout used for constructing the VBAP instance.
//
// Returns the required memory size in bytes, or `0` if the layout is invalid.
//
// Checked preconditions (causes failure on violation):
//
// - `layout` points to a valid speaker layout.

DRB_VBAP_API size_t drb_vbap_size
    (
        DrB_VBAP_Layout const * layout
    );

// `drb_vbap_construct(memory, layout, error`):
//
// Constructs a new VBAP instance from a memory block.
//
// The `memory` pointer must meet the size requirement returned by
// `drb_vbap_size`, and the alignment of `drb_vbap_alignment`. Memory can be
// allocated with `malloc`, `new`, or a custom allocator.
//
// Parameters:
//
// - `memory`: Pointer to pre-allocated memory for the VBAP instance.
// - `layout`: Pointer to the layout used for constructing the VBAP instance.
// - `error`: Optional pointer to receive an error code; may be null.
//
// Returns a pointer to a VBAP instance on success and null on failure.
// If `error` is non-null, the function writes an error code on failure.
//
// Checked preconditions (causes failure on violation):
//
// - `memory` is non-null and aligned to `max_align_t`.
// - `layout` points to a valid speaker layout.
//
// Space complexity: `O(source_count × speaker_count)`.

DRB_VBAP_API DrB_VBAP const * drb_vbap_construct
    (
        void * memory,
        DrB_VBAP_Layout const * layout,
        DrB_VBAP_Error * error
    );

// `drb_vbap_gain_matrix(vbap, source_positions, speaker_gains, source_count)`
//
// Computes the per-speaker gains for a list of source positions.
//
// Source positions are treated as directions; the radial magnitude does not
// affect panning. Behaviour is undefined for the zero vector or non-finite
// values; the implementation does not check for these conditions. Callers
// should guard against these cases at the call site in real-time contexts.
//
// Gains are stored in row-major order; each row corresponds to a source and
// each column to a speaker:
//
// | source-1-gain-1, source-1-gain-2, source-1-gain-3, ..., source-1-gain-n |
// | source-2-gain-1, source-2-gain-2, source-2-gain-3, ..., source-2-gain-n |
// | source-3-gain-1, source-3-gain-2, source-3-gain-3, ..., source-3-gain-n |
// |        .                .                .        .            .        |
// |        .                .                .          .          .        |
// |        .                .                .            .        .        |
// | source-k-gain-1, source-k-gain-2, source-k-gain-3, ..., source-k-gain-n |
//
// Gains are linear, non-negative, and—within normal floating-point round-off—
// the sum of squares across all speakers equals one for any finite, non-zero
// source vector. If a source aligns exactly with a speaker, that speaker’s gain
// is one and the others are zero.
//
// Parameters:
//
// - `vbap`: Pointer to a VBAP instance.
// - `source_positions`: Interleaved array of x/y coordinates for the sources.
// - `source_count`: Number of sources.
// - `speaker_gains`: Output array to receive the computed gains.
//
// Assumed preconditions (not checked; violation causes undefined behaviour):
//
// - All pointers are non-null, distinct, and do not overlap in memory.
// - `source_positions` has space for `source_count × 2` floats.
// - `speaker_gains` has space for `source_count × speaker_count` floats.
// - `source_count` is nonnegative.
//
// Postconditions (guaranteed when the preconditions are met):
//
// - For each computed gain, `g`: `0 ≤ g ≤ 1` for all finite inputs.
// - For each computed gain row, `g_1 ... g_n: g_1² + ... + g_n² = 1 ± ε`.
//
// Time complexity: `O(source_count × speaker_count)`.

DRB_VBAP_API void drb_vbap_gain_matrix
    (
        DrB_VBAP const * vbap,
        float const * DRB_VBAP_RESTRICT source_positions,
        float * DRB_VBAP_RESTRICT gain_matrix,
        int32_t source_count
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // DRB_VBAP_H
