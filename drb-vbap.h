#ifndef DRB_VBAP_H
#define DRB_VBAP_H

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Semantic version for the library.
static struct
{
    short major, minor, patch;
}
const drb_vbap_version = { 0, 0, 4 };

// Opaque structure representing a 2-D VBAP instance.
//
// Instances are immutable after successful construction.
typedef struct DrB_VBAP_2D DrB_VBAP_2D;

// Error codes indicating why construction of a 2-D VBAP instance failed.
//
// Values:
//
// - `drb_vbap_2d_error_invalid_resolution`: The resolution is less than two or
//   exceeds the maximum limit of 3600.
// - `drb_vbap_2d_error_invalid_speaker_count`: The number of speakers is less
//   than two or exceeds the maximum supported count of 64.
// - `drb_vbap_2d_error_invalid_speaker_positions`: The speaker positions con-
//   tains out-of-range values, duplicates, or is not sorted in ascending order.
typedef enum
{
    drb_vbap_2d_error_invalid_resolution,
    drb_vbap_2d_error_invalid_speaker_count,
    drb_vbap_2d_error_invalid_speaker_positions
}
DrB_VBAP_2D_Error;

// Computes the memory size required to construct a 2-D VBAP instance.
//
// The maximum grid resolution is 3600 divisions and the maximum number of
// speakers is 64.
//
// Parameters:
//
// - `resolution`: Number of angular divisions in the panning grid.
// - `speaker_count`: Number of speakers in the setup.
//
// Returns: The required memory size in bytes or `0` if the parameters does not
// meet the maximum bounds.
extern size_t drb_vbap_2d_size
    (
        int resolution,
        int speaker_count
    );

// Constructs a new 2-D VBAP instance.
//
// The `memory` pointer must meet the size requirement returned by
// `drb_vbap_2d_size`, and the minimum alignment is `alignof(max_align_t)`.
// Memory can be allocated with `malloc` or a custom allocator.
//
// The `resolution` parameter divides the unit circle into evenly spaced
// angular grid points. Speaker positions index into this grid in
// counter-clockwise order, starting from the top of the unit circle
// (x = 0, y = 1). All speaker positions must be unique and sorted in
// ascending order.
//
// If `error` is non-NULL, the function writes an error code on failure.
// If `error` is NULL, errors are silently ignored.
//
// Parameters:
// - `memory`: Pointer to pre-allocated memory for the VBAP instance.
// - `resolution`: Number of angular divisions in the panning grid.
// - `speaker_positions`: Array of grid indices, one per speaker.
// - `speaker_count`: Number of speakers in the layout.
// - `error`: Optional pointer to receive an error code; may be NULL.
//
// Returns: A pointer to a VBAP instance on success, or NULL if the parameters
// are invalid (e.g., out-of-range, duplicate, or unsorted speaker positions).
extern DrB_VBAP_2D * drb_vbap_2d_construct
    (
        void * memory,
        int resolution,
        int const speaker_positions [],
        int speaker_count,
        DrB_VBAP_2D_Error * error
    );

// Computes the per-speaker gains for a list of source positions.
//
// Source angles must be specified in radians.
//
// Gains are linear, normalized for unit power, and stored in row-major layout
// with source-major order. Each row corresponds to a source, and each column
// corresponds to a speaker. The size of `gains` must be at least
// `source_count * speaker_count`.
//
// The `source_angles` array must not overlap (alias) with the `gains` array.
//
// This function is safe to call from a real-time audio thread. It is fully
// re-entrant and may be called concurrently from multiple threads on the same
// instance, as long as the `gains` arrays passed to the function are disjoint.
//
// Parameters:
//
// - `vbap`: Pointer to a 2-D VBAP instance.
// - `source_angles`: Array of angles specifying the positions of the sources.
// - `source_count`: Number of sources.
// - `gains`: Output array to receive the computed gains.
extern void drb_vbap_2d_compute_gains
    (
        DrB_VBAP_2D const * vbap,
        float const source_angles [],
        int source_count,
        float gains []
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // DRB_VBAP_H
