// Dr. Bafflegab's Vector Base Amplitude Panner
// https://github.com/drbafflegab/vbap
// SPDX-License-Identifier: MIT

#ifndef DRB_VBAP_H
#define DRB_VBAP_H

#if defined(__cplusplus)
#  include <cstddef>
#  include <cstdint>
#else
#  include <stdbool.h>
#  include <stddef.h>
#  include <stdint.h>
#endif

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

//
// Retrieves the semantic version of the library: MAJOR.MINOR.PATCH.
//
// Parameters:
//
// - `major`: Optional, nullable pointer to receive the major version.
// - `minor`: Optional, nullable pointer to receive the minor version.
// - `patch`: Optional, nullable pointer to receive the patch version.
//
// For each parameter, the corresponding version component is written only if
// the pointer is non-null. Null pointers are ignored and left untouched.
//
// Assumed preconditions (not validated; violation causes undefined behaviour):
//
// - `major`, `minor`, and `patch` are distinct pointers.
//
DRB_VBAP_API void drb_vbap_version
    (
        int32_t * DRB_VBAP_RESTRICT major,
        int32_t * DRB_VBAP_RESTRICT minor,
        int32_t * DRB_VBAP_RESTRICT patch
    );

//
// Error code indicating why construction of a VBAP instance failed.
//
typedef int32_t DrB_VBAP_Error;

//
// Values for `DrB_VBAP_Error`:
//
// - `drb_vbap_error_null_pointer`: A required pointer is null.
// - `drb_vbap_error_misaligned_pointer`: A provided pointer is misaligned.
// - `drb_vbap_error_invalid_resolution`: The resolution is out of bounds.
// - `drb_vbap_error_invalid_speaker_steps`: The speaker steps are invalid.
// - `drb_vbap_error_invalid_speaker_count`: The speaker count is out of bounds.
//
enum
{
    drb_vbap_error_null_pointer = 1,
    drb_vbap_error_misaligned_pointer = 2,
    drb_vbap_error_invalid_resolution = 3,
    drb_vbap_error_invalid_speaker_steps = 4,
    drb_vbap_error_invalid_speaker_count = 5
};

//
// Converts an error code into a human-readable string with static storage.
//
// Parameters:
//
// - `error`: The error code to convert.
//
// Returns a pointer to a null-terminated string describing the error.
// For unrecognized codes, "unknown error" is returned. The string has static
// storage duration and must not be modified or freed. Never returns null.
//
// This function is thread-safe and does not allocate.
//
DRB_VBAP_API char const * drb_vbap_error_string
    (
        DrB_VBAP_Error error
    );

//
// Maximum resolution in a setup.
//
#define DRB_VBAP_MAX_RESOLUTION 65536

//
// Maximum speaker count in a setup.
//
#define DRB_VBAP_MAX_SPEAKER_COUNT 256

//
// Minimum and maximum spans in degrees.
//
#define DRB_VBAP_MIN_SPAN 5
#define DRB_VBAP_MAX_SPAN 175

//
// Struct describing the speaker layout used for constructing the VBAP instance.
//
// The `resolution` property divides the unit circle into evenly spaced angular
// grid steps. Step 0 corresponds to 90° (x = 0, y = 1). Steps increase
// counter-clockwise in sizes of 360°/`resolution`. Speaker positions index into
// this grid using integer indices (steps). All speaker indices must be within
// [0, `resolution` - 1], unique, and sorted in strict ascending numeric order.
//
// Use the following formula to convert a step to an angle:
//
//     θ(`step`) = `step` / `resolution` × 360° + 90°.
//
// For example, for a resolution of 8 and the speaker array [1, 3, 5, 7], the
// speakers will be positioned at the following angles (135°, 225°, 315°, 45°):
//
//                 |
//      spk. 1     |     spk. 4
//         \       |       /
//           \     |     /
//             \   |   /
//               \ | /
//    ------------ * ------------
//               / | \
//             /   |   \
//           /     |     \
//         /       |       \
//      spk. 2     |     spk. 3
//
// Note that index 7 maps to 45°, which appears after 315° due to the
// 360° wrap-around of the angle mapping; the input still uses ascending step
// order, not geometric (circular) order.
//
// Members:
//
// - `resolution`: Number of angular divisions in the panning grid.
// - `speaker_steps`: Pointer to array of speakes.
// - `sectors`: Pointer to array of sectors.
// - `count`: Number of speakers/spans in the layout.
//
// Checked preconditions (causes failure on violation):
//
// - 1 <= `resolution` <= `DRB_VBAP_MAXIMUM_RESOLUTION`.
// - All `speaker_steps` are in-range, unique, and sorted in ascending order.
// - For all `speakers->divergence`, `0.0` <= `divergence` <= `1.0`.
// - 1 <= `count` <= `DRB_VBAP_MAXIMUM_SPEAKER_COUNT`.
//
// Assumed preconditions (not validated):
//
// - `speaker_steps` contains at least `count` speakers.
// - `sectors` contains at least `count` sectors.
//
typedef struct
{
    int32_t resolution;
    int32_t const * speaker_steps;
    int32_t count;
}
DrB_VBAP_Layout;

//
// TODO: write description.
//
#define DRB_VBAP_LAYOUT_TAG_SURROUND_2 "surround-2" // L/R
#define DRB_VBAP_LAYOUT_TAG_SURROUND_3 "surround-3" // L/C/R
#define DRB_VBAP_LAYOUT_TAG_SURROUND_5 "surround-5" // L/C/R/Ls/Rs
#define DRB_VBAP_LAYOUT_TAG_SURROUND_7 "surround-7" // L/C/R/Ls/Rs/Lb/Rb

//
// TODO: write description.
//
DRB_VBAP_API DrB_VBAP_Layout const * drb_vbap_builtin_layout
    (
        char const * const tag
    );

//
// Opaque structure representing a VBAP instance.
//
// Instances are immutable after successful construction.
//
typedef struct DrB_VBAP DrB_VBAP;

//
// Returns the minimum required alignment to construct a VBAP instance.
//
// The return value is hardcoded to the alignment of `max_align_t`. Use it if
// you use custom allocators. On modern platforms, `malloc` and `new` satisfy
// this alignment requirement.
//
// This function is thread-safe and does not allocate.
//
DRB_VBAP_API size_t drb_vbap_alignment
    (
        void
    );

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
//
// This function is thread-safe and does not allocate.
//
DRB_VBAP_API size_t drb_vbap_size
    (
        DrB_VBAP_Layout const * layout
    );

//
// Constructs a new VBAP instance.
//
// The `memory` pointer must meet the size requirement returned by
// `drb_vbap_size`, and the alignment of `max_align_t`. Memory can be allocated
// with `malloc`, `new`, or a custom allocator.
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
// - `memory` and `layout` are non-null.
// - `memory` is aligned to `max_align_t`.
// - `layout` is a valid speaker layout.
//
// This function is thread-safe and does not allocate.
//
DRB_VBAP_API DrB_VBAP const * drb_vbap_construct
    (
        void * memory,
        DrB_VBAP_Layout const * layout,
        DrB_VBAP_Error * error
    );

//
// Computes the per-speaker gains for a list of source positions.
//
// Source positions are treated as directions; the radial magnitude does not
// affect panning. Behavior is undefined for the zero vector (0,0) or non-finite
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
// is one and the others are zero. To avoid performance penalties, any subnormal
// (denormal) gain values are flushed to zero before they are written.
//
// Parameters:
//
// - `vbap`: Pointer to a VBAP instance.
// - `source_positions`: Interleaved array of x/y coordinates for the sources.
// - `source_count`: Number of sources.
// - `gains`: Output array to receive the computed gains.
//
// Assumed preconditions (not validated, violation causes undefined behaviour):
//
// - `vbap`, `source_positions`, and `gains` are non-null.
// - `source_positions` has space for at least `source_count` × 2 floats.
// - `gains` has space for at least `source_count` × `speaker_count` floats.
// - 0 <= `source_count`. (`source_count` == 0 is a no-op.)
// - `source_positions` and `gains` do not overlap in memory.
//
// Time complexity: O(`source_count`).
//
// This function is safe to call from a real-time audio thread (no internal
// allocations or locks). It is fully re-entrant and may be called concurrently
// from multiple threads on the same instance, as long as the `gains` arrays
// passed to the function are disjoint.
//
DRB_VBAP_API void drb_vbap_process
    (
        DrB_VBAP const * vbap,
        float const * DRB_VBAP_RESTRICT source_positions,
        float * DRB_VBAP_RESTRICT speaker_gains,
        int32_t source_count
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // DRB_VBAP_H
