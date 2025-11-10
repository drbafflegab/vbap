#include "drb-vbap.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdalign.h>
#include <string.h>

// -------------------------------------------------------------------------- //

#if defined(DRB_CACHE_LINE_SIZE)
enum { cache_line_size = DRB_CACHE_LINE_SIZE };
#else
enum { cache_line_size = alignof(max_align_t) };
#endif

static_assert((alignof(max_align_t) & (alignof(max_align_t) - 1)) == 0, "");
static_assert(cache_line_size >= alignof(max_align_t), "");
static_assert((cache_line_size & (cache_line_size - 1)) == 0, "");
static_assert(cache_line_size % alignof(max_align_t) == 0, "");

// Checks that the alignment of `pointer` >= `alignof(max_align_t)`.
static inline bool aligned (void const * const pointer)
{
    uintptr_t const delta = (-(uintptr_t)pointer) & (alignof(max_align_t) - 1);

    return pointer != NULL && delta == 0;
}

// Align an arbitrary pointer up to the next cache line.
static inline void * alignup_pointer (void * const pointer)
{
    uintptr_t const delta = (-(uintptr_t)pointer) & (cache_line_size - 1);

    return (unsigned char *)pointer + delta;
}

// Rounds `size` up to nearest cache-line multiple.
static inline size_t alignup_size (size_t const size)
{
    return (size + cache_line_size - 1) & ~(cache_line_size - 1);
}

// Simple bump allocator: return current and advance by cache-line-rounded size.
static inline void * alloc (unsigned char * * const pointer, size_t const size)
{
    void * const memory = *pointer;

    *pointer += alignup_size(size);

    return memory;
}

// -------------------------------------------------------------------------- //

static float const pi = 3.1415927f;

// -------------------------------------------------------------------------- //

static inline float step_to_angle
    (
        int_fast32_t const step,
        int_fast32_t const resolution
    )
{
    return (float)step * (2.0f * pi) / (float)resolution;
}

static inline int_fast32_t angle_to_step
    (
        float const angle,
        int_fast32_t const resolution
    )
{
    int_fast32_t const step = floorf((angle) * (float)resolution / (2.0f * pi));

    return step < 0 ? step + resolution : step;
}

static inline bool contains
    (
        int_fast32_t const lower_bound,
        int_fast32_t const upper_bound,
        int_fast32_t const step,
        int_fast32_t const resolution
    )
{
    if (lower_bound < upper_bound)
    {
        return lower_bound <= step && step < upper_bound;
    }
    else
    {
        bool const cond_a = 0 <= step && step < upper_bound;
        bool const cond_b = lower_bound <= step && step < resolution;

        return cond_a || cond_b;
    }
}

static inline void unpack_pair
    (
        int_fast32_t const pair,
        int_fast32_t const speaker_count,
        int32_t base [2]
    )
{
    base[0] = (pair == 0 ? speaker_count : pair) - 1;
    base[1] = pair;
}

// -------------------------------------------------------------------------- //

extern void drb_vbap_version
    (
        int32_t * restrict const major,
        int32_t * restrict const minor,
        int32_t * restrict const patch
    )
{
    if (major != NULL) { *major = DRB_VBAP_VERSION_MAJOR; }
    if (minor != NULL) { *minor = DRB_VBAP_VERSION_MINOR; }
    if (patch != NULL) { *patch = DRB_VBAP_VERSION_PATCH; }
}

extern char const * drb_vbap_error_string
    (
        DrB_VBAP_Error const error
    )
{
    switch (error)
    {
        case drb_vbap_error_null_pointer: return "null pointer";
        case drb_vbap_error_misaligned_pointer: return "misaligned pointer";
        case drb_vbap_error_invalid_layout: return "invalid layout";
    }

    return "unknown";
}

static DrB_VBAP_Layout const surround_3 [1] =
{
    {
        .resolution = 36,
        .speaker_steps = (int32_t []){ 3, 18, 33 },
        .speaker_count = 3
    }
};

static DrB_VBAP_Layout const surround_5 [1] =
{
    {
        .resolution = 36,
        .speaker_steps = (int32_t []){ 0, 3, 11, 25, 33 },
        .speaker_count = 5
    }
};

static DrB_VBAP_Layout const surround_7 [1] =
{
    {
        .resolution = 36,
        .speaker_steps = (int32_t []){ 0, 3, 11, 15, 21, 25, 33 },
        .speaker_count = 7
    }
};

static DrB_VBAP_Layout const quadrophonic [1] =
{
    {
        .resolution = 8,
        .speaker_steps = (int32_t []){ 1, 3, 5, 7 },
        .speaker_count = 4
    }
};

static DrB_VBAP_Layout const hexagonal [1] =
{
    {
        .resolution = 6,
        .speaker_steps = (int32_t []){ 0, 1, 2, 3, 4, 5 },
        .speaker_count = 6
    }
};

static DrB_VBAP_Layout const octophonic [1] =
{
    {
        .resolution = 8,
        .speaker_steps = (int32_t []){ 0, 1, 2, 3, 4, 5, 6, 7 },
        .speaker_count = 8
    }
};

extern DrB_VBAP_Layout const * drb_vbap_builtin_layout
    (
        char const * const tag
    )
{
    if (tag == NULL) { return NULL; }

    if (strcmp(tag, DRB_VBAP_LAYOUT_TAG_SURROUND_3) == 0) { return surround_3; }
    if (strcmp(tag, DRB_VBAP_LAYOUT_TAG_SURROUND_5) == 0) { return surround_5; }
    if (strcmp(tag, DRB_VBAP_LAYOUT_TAG_SURROUND_7) == 0) { return surround_7; }
    if (strcmp(tag, DRB_VBAP_LAYOUT_TAG_QUADROPHONIC) == 0) { return quadrophonic; }
    if (strcmp(tag, DRB_VBAP_LAYOUT_TAG_HEXAGONAL) == 0) { return hexagonal; }
    if (strcmp(tag, DRB_VBAP_LAYOUT_TAG_OCTOPHONIC) == 0) { return octophonic; }

    return NULL;
}

typedef struct Bucket { uint8_t speaker_pair; } Bucket;
typedef struct Matrix { float a00, a01, a10, a11; } Matrix;

struct DrB_VBAP
{
    int32_t resolution;
    int32_t speaker_count;
    Bucket const * buckets;
    Matrix const * matrices;
};

extern size_t drb_vbap_alignment
    (
        void
    )
{
    return alignof(max_align_t);
}

extern size_t drb_vbap_size
    (
        DrB_VBAP_Layout const * const layout
    )
{
    if (1 > layout->resolution || layout->resolution > DRB_VBAP_MAX_RESOLUTION)
    {
        return 0;
    }

    if (2 > layout->speaker_count || layout->speaker_count > DRB_VBAP_MAX_SPEAKER_COUNT)
    {
        return 0;
    }

    size_t size = cache_line_size - alignof(max_align_t);

    size += alignup_size(sizeof(DrB_VBAP));
    size += alignup_size(sizeof(Bucket) * layout->resolution);
    size += alignup_size(sizeof(Matrix) * layout->speaker_count);

    return size;
}

extern DrB_VBAP const * drb_vbap_construct
    (
        void * const memory,
        DrB_VBAP_Layout const * const layout,
        DrB_VBAP_Error * const error
    )
{
    if (1 > layout->resolution || layout->resolution > DRB_VBAP_MAX_RESOLUTION)
    {
        if (error != NULL) { *error = drb_vbap_error_invalid_layout; }

        return NULL;
    }

    if (1 > layout->speaker_count || layout->speaker_count > DRB_VBAP_MAX_SPEAKER_COUNT)
    {
        if (error != NULL) { *error = drb_vbap_error_invalid_layout; }

        return NULL;
    }

    if (memory == NULL || layout->speaker_steps == NULL)
    {
        if (error != NULL) { *error = drb_vbap_error_null_pointer; }

        return NULL;
    }

    if (!aligned(memory))
    {
        if (error != NULL) { *error = drb_vbap_error_misaligned_pointer; }

        return NULL;
    }

    int_fast32_t previous_step = -1;

    for (int_fast32_t speaker = 0; speaker < layout->speaker_count; speaker++)
    {
        int_fast32_t const step = layout->speaker_steps[speaker];

        if (step <= previous_step || step >= layout->resolution)
        {
            if (error != NULL) { *error = drb_vbap_error_invalid_layout; }

            return NULL;
        }

        previous_step = step;
    }

    for (int_fast32_t pair = 0; pair < layout->speaker_count; pair++)
    {
        int32_t base [2];

        unpack_pair(pair, layout->speaker_count, base);

        float const span = fmodf
        (
            step_to_angle(layout->speaker_steps[base[1]], layout->resolution)
          -
            step_to_angle(layout->speaker_steps[base[0]], layout->resolution)
          +
            pi * 2.0f
          ,
            pi * 2.0f
        )
          *
            360.0f / (2.0f * pi);

        float const a = step_to_angle(layout->speaker_steps[base[0]], layout->resolution);
        float const b = step_to_angle(layout->speaker_steps[base[1]], layout->resolution);

        if (span < DRB_VBAP_MIN_SPAN || DRB_VBAP_MAX_SPAN < span)
        {
            if (error != NULL) { *error = drb_vbap_error_invalid_layout; }

            return NULL;
        }
    }

    unsigned char * pointer = alignup_pointer(memory);

    DrB_VBAP * const vbap = alloc(&pointer, sizeof(DrB_VBAP));
    Bucket * const buckets = alloc(&pointer, layout->resolution * sizeof(Bucket));
    Matrix * const matrices = alloc(&pointer, layout->speaker_count * sizeof(Matrix));

    for (int_fast32_t step = 0, pair = 0; step < layout->resolution; step++)
    {
        int32_t base [2];

        unpack_pair(pair, layout->speaker_count, base);

        int_fast32_t const lower_bound = layout->speaker_steps[base[0]];
        int_fast32_t const upper_bound = layout->speaker_steps[base[1]];

        if (!contains(lower_bound, upper_bound, step, layout->resolution))
        {
            pair = (pair + 1) % layout->speaker_count;
        }

        assert(0 <= pair && pair <= UINT8_MAX);

        buckets[step].speaker_pair = (uint8_t)pair;
    }

    for (int_fast32_t pair = 0; pair < layout->speaker_count; pair++)
    {
        int32_t base [2];

        unpack_pair(pair, layout->speaker_count, base);

        int_fast32_t const fst_speaker_step = layout->speaker_steps[base[0]];
        int_fast32_t const snd_speaker_step = layout->speaker_steps[base[1]];

        double const w0 = step_to_angle(fst_speaker_step, layout->resolution);
        double const w1 = step_to_angle(snd_speaker_step, layout->resolution);

        double const a00 = cos(w0), a01 = cos(w1);
        double const a10 = sin(w0), a11 = sin(w1);

        double const det = 1.0 / (a00 * a11 - a01 * a10);

        matrices[pair].a00 = (float)(+a11 * det);
        matrices[pair].a01 = (float)(-a01 * det);
        matrices[pair].a10 = (float)(-a10 * det);
        matrices[pair].a11 = (float)(+a00 * det);
    }

    vbap->resolution = layout->resolution;
    vbap->speaker_count = layout->speaker_count;
    vbap->buckets = buckets;
    vbap->matrices = matrices;

    return vbap;
}

extern void drb_vbap_process
    (
        DrB_VBAP const * const vbap,
        float const * restrict const source_positions,
        float * restrict const speaker_gains,
        int32_t const source_count
    )
{
    for (int_fast32_t source = 0; source < source_count; source++)
    {
        float * const row = speaker_gains + source * vbap->speaker_count;

        memset(row, 0, vbap->speaker_count * sizeof(float));

        float const source_x = source_positions[source * 2 + 0];
        float const source_y = source_positions[source * 2 + 1];

        float const angle = atan2f(source_y, source_x);

        int_fast32_t const step = angle_to_step(angle, vbap->resolution);

        assert(0 <= step && step < vbap->resolution);

        int_fast32_t const pair = vbap->buckets[step].speaker_pair;

        assert(0 <= pair && pair < vbap->speaker_count);

        Matrix const * const matrix = &vbap->matrices[pair];

        float const a = source_x * matrix->a00 + source_y * matrix->a01;
        float const b = source_x * matrix->a10 + source_y * matrix->a11;

        float const scale = 1.0f / sqrtf(a * a + b * b);

        int32_t base [2];

        unpack_pair(pair, vbap->speaker_count, base);

        row[base[0]] = a * scale;
        row[base[1]] = b * scale;
    }
}

// -------------------------------------------------------------------------- //
