#ifndef DRB_VBAP_H
#define DRB_VBAP_H

#include <stddef.h> // For `size_t`.

#if defined(__cplusplus)
extern "C" {
#endif

// Semantic version for the library.
static struct
{
    short major, minor, patch;
}
const drb_vbap_version = { 0, 0, 1 };

// Opaque structure representing a 2-D VBAP instance.
typedef struct DrB_VBAP_2D DrB_VBAP_2D;

// The memory alignment required for constructing a 2-D VBAP instance.
enum { drb_vbap_alignment = 64 };

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
// The `memory` pointer must meet the alignment and size requirements provided
// by `drb_vbap_alignment` and `drb_vbap_2d_size`.
//
// Parameters:
//
// - `memory`: Pointer to preallocated memory for the VBAP instance.
// - `resolution`: Number of angular divisions in the panning grid.
// - `speaker_positions`: Array of indices on the panning grid, one per speaker.
// - `speaker_count`: Number of speakers in the setup.
//
// Returns: A pointer to a `DrB_VBAP_2D` sharing the same address as `memory`.
extern DrB_VBAP_2D * drb_vbap_2d_construct
    (
        void * memory,
        int resolution,
        int const speaker_positions [],
        int speaker_count
    );

// Computes the per-speaker gain for a list of source positions.
//
// The angles in the `source_array` must be specified in radians.
//
// Gains are stored in row-major order; each row corresponds to a source, and
// each column corresponds to a speaker. The size of `gains` must be at least
// `source_count * speaker_count`.
//
// Parameters:
//
// - `vbap`: Pointer to a 2-D VBAP instance.
// - `source_angles`: Array of angles specifying the positions of the sources.
// - `source_count`: Number of sources.
// - `gains`: Output array to store the computed gains.
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
