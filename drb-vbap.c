#include "drb-vbap.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

static float const pi = 3.1415926535f;

static inline float flush_denormals (float const x)
{
    return fabsf(x) < FLT_MIN ? 0.0f : x;
}

// Wraps `angle` in [0; 2 pi].
static inline float wrap_two_pi (float const angle)
{
    float const wrapped = fmodf(angle, 2.0f * pi);

    return wrapped < 0.0f ? wrapped + 2.0f * pi : wrapped;
}

static inline float division_to_angle (int const division, int const resolution)
{
    return (float)division * (2.0f * pi) / (float)resolution;
}

static inline int angle_to_span (float const angle, int const resolution)
{
    return (int)floorf(angle * (float)resolution / (2.0f * pi));
}

static inline bool contains
    (
        int const division,
        int const last_division,
        int const next_division,
        int const resolution
    )
{
    if (last_division < next_division)
    {
        return last_division <= division && division < next_division;
    }
    else
    {
        bool const cond_a = 0 <= division && division < next_division;
        bool const cond_b = last_division <= division && division < resolution;

        return cond_a || cond_b;
    }
}

static inline void unpack_speaker_pair
    (
        int const speaker_pair,
        int const speaker_count,
        int speakers [2]
    )
{
    speakers[0] = (speaker_pair == 0 ? speaker_count : speaker_pair) - 1;
    speakers[1] = speaker_pair;
}

enum { cache_line_size = 64 };

typedef struct Bucket { unsigned char speaker_pair; } Bucket;
typedef struct Matrix { float a00, a01, a10, a11; } Matrix;

struct DrB_VBAP_2D
{
    int resolution;
    int speaker_count;
    Bucket const * buckets;
    Matrix const * matrices;
};

// Rounds size up to the nearest multiple of `cache_line_size`.
static size_t alignup (size_t const size)
{
    return (size + cache_line_size - 1) & -cache_line_size;
}

// Allocates aligned memory from a preallocated block.
static void * alloc (unsigned char * * const pointer, size_t const size)
{
    void * const block = *pointer;

    *pointer += alignup(size);

    return block;
}

enum { max_resolution = 3600, max_speaker_count = 64 };

extern size_t drb_vbap_2d_size
    (
        int const resolution,
        int const speaker_count
    )
{
    if (resolution < 2 || resolution > max_resolution)
    {
        return 1;
    }

    if (speaker_count < 2 || speaker_count > max_speaker_count)
    {
        return 1;
    }

    size_t size = 0;

    size += alignup(sizeof(DrB_VBAP_2D));
    size += alignup(sizeof(Bucket) * resolution);
    size += alignup(sizeof(Matrix) * speaker_count);

    return size;
}

extern DrB_VBAP_2D * drb_vbap_2d_construct
    (
        void * const memory,
        int const resolution,
        int const speaker_positions [],
        int const speaker_count,
        DrB_VBAP_2D_Error * const error
    )
{
    if (resolution < 2 || resolution >= max_resolution)
    {
        if (error != NULL)
        {
            *error = drb_vbap_2d_error_invalid_resolution;
        }

        return NULL;
    }

    if (speaker_count < 2 || speaker_count >= max_speaker_count)
    {
        if (error != NULL)
        {
            *error = drb_vbap_2d_error_invalid_speaker_count;
        }

        return NULL;
    }

    int previous_position = -1;

    for (int speaker_index = 0; speaker_index < speaker_count; speaker_index++)
    {
        int const position = speaker_positions[speaker_index];

        if (position <= previous_position || position >= resolution)
        {
            if (error != NULL)
            {
                *error = drb_vbap_2d_error_invalid_speaker_positions;
            }

            return NULL;
        }

        previous_position = position;
    }

    unsigned char * pointer = memory;

    DrB_VBAP_2D * const vbap = alloc(&pointer, sizeof(DrB_VBAP_2D));
    Bucket * const buckets = alloc(&pointer, sizeof(Bucket) * resolution);
    Matrix * const matrices = alloc(&pointer, sizeof(Matrix) * speaker_count);

    for (int division = 0, speaker_pair = 0; division < resolution; division++)
    {
        int speakers [2];

        unpack_speaker_pair(speaker_pair, speaker_count, speakers);

        int const last_division = speaker_positions[speakers[0]];
        int const next_division = speaker_positions[speakers[1]];

        if (!contains(division, last_division, next_division, resolution))
        {
            speaker_pair = (speaker_pair + 1) % speaker_count;
        }

        buckets[division].speaker_pair = (unsigned char)speaker_pair;
    }

    for (int speaker_pair = 0; speaker_pair < speaker_count; speaker_pair++)
    {
        int speakers [2];

        unpack_speaker_pair(speaker_pair, speaker_count, speakers);

        int const last_division = speaker_positions[speakers[0]];
        int const next_division = speaker_positions[speakers[1]];

        float const last_angle = division_to_angle(last_division, resolution);
        float const next_angle = division_to_angle(next_division, resolution);

        float const a00 = cosf(last_angle), a01 = cosf(next_angle);
        float const a10 = sinf(last_angle), a11 = sinf(next_angle);

        float const det = 1.0f / (a00 * a11 - a01 * a10);

        matrices[speaker_pair].a00 = +a11 * det;
        matrices[speaker_pair].a01 = -a01 * det;
        matrices[speaker_pair].a10 = -a10 * det;
        matrices[speaker_pair].a11 = +a00 * det;
    }

    vbap->resolution = resolution;
    vbap->speaker_count = speaker_count;
    vbap->buckets = buckets;
    vbap->matrices = matrices;

    return vbap;
}

extern void drb_vbap_2d_compute_gains
    (
        DrB_VBAP_2D const * const vbap,
        float const source_angles [],
        int const source_count,
        float gains []
    )
{
    memset(gains, 0, source_count * vbap->speaker_count * sizeof(float));

    for (int source = 0; source < source_count; source++)
    {
        float const source_angle = wrap_two_pi(source_angles[source]);

        float const source_x = cosf(source_angle);
        float const source_y = sinf(source_angle);

        int const span = angle_to_span(source_angle, vbap->resolution);

        assert(0 <= span && span < vbap->resolution);

        int const speaker_pair = vbap->buckets[span].speaker_pair;

        assert(0 <= speaker_pair && speaker_pair < vbap->speaker_count);

        int speakers [2];

        unpack_speaker_pair(speaker_pair, vbap->speaker_count, speakers);

        float const a00 = vbap->matrices[speaker_pair].a00;
        float const a01 = vbap->matrices[speaker_pair].a01;
        float const a10 = vbap->matrices[speaker_pair].a10;
        float const a11 = vbap->matrices[speaker_pair].a11;

        float const gain_a = source_x * a00 + source_y * a01;
        float const gain_b = source_x * a10 + source_y * a11;

        float const scale = 1.0f / sqrtf(gain_a * gain_a + gain_b * gain_b);

        float const gain_a_normalized = flush_denormals(gain_a * scale);
        float const gain_b_normalized = flush_denormals(gain_b * scale);

        gains[source * vbap->speaker_count + speakers[0]] = gain_a_normalized;
        gains[source * vbap->speaker_count + speakers[1]] = gain_b_normalized;
    }
}
