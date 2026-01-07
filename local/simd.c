// Vectorized drb_vbap_process with SSE2/NEON lanes (4 at a time).
// Fallback to scalar when SIMD is unavailable or for the tail.

#include <assert.h>
#include <string.h>
#include <math.h>

#if defined(__SSE2__)
  #include <emmintrin.h>
  #include <xmmintrin.h> // rsqrt
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
  #include <arm_neon.h>
#endif

extern void drb_vbap_gain_matrix
    (
        DrB_VBAP const * const vbap,
        float const * restrict const source_positions,
        float * restrict const speaker_gains,
        int32_t const source_count
    )
{
    int32_t const sc = vbap->speaker_count;

    // Zero the entire output once (cheaper than per-row memset).
    memset(speaker_gains, 0, (size_t)source_count * (size_t)sc * sizeof(float));

    int32_t s = 0;

#if defined(__SSE2__) || defined(__ARM_NEON) || defined(__ARM_NEON__)
    for (; s + 3 < source_count; s += 4)
    {
        // Per-lane scalars we still need (pair/base indices and row offsets).
        int32_t base0[4], base1[4];
        int32_t rowoff[4];
        float   x[4], y[4];

        // Gather per-source inputs, pairs, and matrices.
        Matrix const *m[4];

        for (int i = 0; i < 4; ++i)
        {
            int32_t const idx = s + i;

            float sx = source_positions[idx * 2 + 0];
            float sy = source_positions[idx * 2 + 1];
            x[i] = sx; y[i] = sy;

            // Pair selection (scalar): atan2f + bucket lookup.
            float const ang = atan2f(sy, sx);
            int_fast32_t const step = angle_to_step(ang, vbap->resolution);
            assert(0 <= step && step < vbap->resolution);

            int_fast32_t const pair = vbap->buckets[step].speaker_pair;
            assert(0 <= pair && pair < sc);

            m[i] = &vbap->matrices[pair];

            int32_t b[2];
            unpack_pair(pair, sc, b);
            base0[i]  = b[0];
            base1[i]  = b[1];
            rowoff[i] = idx * sc;
        }

        // SIMD compute: a = x*a00 + y*a01;  b = x*a10 + y*a11;  s = 1/sqrt(a^2+b^2)
    #if defined(__SSE2__)
        __m128 vx   = _mm_set_ps(x[3], x[2], x[1], x[0]);
        __m128 vy   = _mm_set_ps(y[3], y[2], y[1], y[0]);

        __m128 a00v = _mm_set_ps(m[3]->a00, m[2]->a00, m[1]->a00, m[0]->a00);
        __m128 a01v = _mm_set_ps(m[3]->a01, m[2]->a01, m[1]->a01, m[0]->a01);
        __m128 a10v = _mm_set_ps(m[3]->a10, m[2]->a10, m[1]->a10, m[0]->a10);
        __m128 a11v = _mm_set_ps(m[3]->a11, m[2]->a11, m[1]->a11, m[0]->a11);

        __m128 va = _mm_add_ps(_mm_mul_ps(vx, a00v), _mm_mul_ps(vy, a01v));
        __m128 vb = _mm_add_ps(_mm_mul_ps(vx, a10v), _mm_mul_ps(vy, a11v));

        __m128 len2 = _mm_add_ps(_mm_mul_ps(va, va), _mm_mul_ps(vb, vb));

        // rsqrt with one NR refinement: y = y*(1.5 - 0.5*x*y*y)
        __m128 y0   = _mm_rsqrt_ps(len2);
        __m128 half = _mm_set1_ps(0.5f);
        __m128 onep5= _mm_set1_ps(1.5f);
        __m128 y2   = _mm_mul_ps(y0, y0);
        __m128 term = _mm_sub_ps(onep5, _mm_mul_ps(half, _mm_mul_ps(len2, y2)));
        __m128 invl = _mm_mul_ps(y0, term);

        __m128 g0 = _mm_mul_ps(va, invl);
        __m128 g1 = _mm_mul_ps(vb, invl);

        // Scatter per lane
        float gout0[4], gout1[4];
        _mm_storeu_ps(gout0, g0);
        _mm_storeu_ps(gout1, g1);

    #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
        float32x4_t vx   = { x[0], x[1], x[2], x[3] };
        float32x4_t vy   = { y[0], y[1], y[2], y[3] };

        float32x4_t a00v = { m[0]->a00, m[1]->a00, m[2]->a00, m[3]->a00 };
        float32x4_t a01v = { m[0]->a01, m[1]->a01, m[2]->a01, m[3]->a01 };
        float32x4_t a10v = { m[0]->a10, m[1]->a10, m[2]->a10, m[3]->a10 };
        float32x4_t a11v = { m[0]->a11, m[1]->a11, m[2]->a11, m[3]->a11 };

        float32x4_t va = vmlaq_f32(vmulq_f32(vx, a00v), vy, a01v);
        float32x4_t vb = vmlaq_f32(vmulq_f32(vx, a10v), vy, a11v);

        float32x4_t len2 = vmlaq_f32(vmulq_f32(va, va), vb, vb);

        // vrsqrte + one NR refinement: y = y * (3 - x*y*y)/2
        float32x4_t y0   = vrsqrteq_f32(len2);
        float32x4_t y2   = vmulq_f32(y0, y0);
        float32x4_t three= vdupq_n_f32(3.0f);
        float32x4_t half = vdupq_n_f32(0.5f);
        float32x4_t term = vmlsq_f32(three, len2, y2); // (3 - x*y*y)
        float32x4_t invl = vmulq_f32(y0, vmulq_f32(term, half));

        float32x4_t g0v = vmulq_f32(va, invl);
        float32x4_t g1v = vmulq_f32(vb, invl);

        float gout0[4], gout1[4];
        vst1q_f32(gout0, g0v);
        vst1q_f32(gout1, g1v);
    #endif

        // Store to two speaker columns (clamp tiny negatives to 0)
        for (int i = 0; i < 4; ++i)
        {
            float ga = gout0[i];
            float gb = gout1[i];
            if (ga < 0.0f) ga = 0.0f;
            if (gb < 0.0f) gb = 0.0f;

            speaker_gains[rowoff[i] + base0[i]] = ga;
            speaker_gains[rowoff[i] + base1[i]] = gb;
        }
    }
#endif

    // Tail or non-SIMD fallback (scalar, same math)
    for (; s < source_count; ++s)
    {
        float *row = speaker_gains + (size_t)s * (size_t)sc; // already zeroed

        float const sx = source_positions[s * 2 + 0];
        float const sy = source_positions[s * 2 + 1];

        float const ang = atan2f(sy, sx);
        int_fast32_t const step = angle_to_step(ang, vbap->resolution);
        assert(0 <= step && step < vbap->resolution);

        int_fast32_t const pair = vbap->buckets[step].speaker_pair;
        assert(0 <= pair && pair < sc);

        Matrix const *matrix = &vbap->matrices[pair];

        float const a = sx * matrix->a00 + sy * matrix->a01;
        float const b = sx * matrix->a10 + sy * matrix->a11;
        float const invl = 1.0f / sqrtf(a * a + b * b);

        int32_t base[2];
        unpack_pair(pair, sc, base);

        float ga = a * invl;
        float gb = b * invl;
        if (ga < 0.0f) ga = 0.0f;
        if (gb < 0.0f) gb = 0.0f;

        row[base[0]] = ga;
        row[base[1]] = gb;
    }
}
