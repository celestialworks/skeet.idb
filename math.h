#pragma once

namespace math {
	// pi constants.
	static const float invtwopi = 0.1591549f;
	static const float twopi = 6.283185f;
	static const float threehalfpi = 4.7123889f;
	static const float pi = 3.141593f;
	static const float halfpi = 1.570796f;
	static const __m128 signmask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));

	static const __declspec(align(16)) float null[4] = { 0.f, 0.f, 0.f, 0.f };
	static const __declspec(align(16)) float _pi2[4] = { 1.5707963267948966192f, 1.5707963267948966192f, 1.5707963267948966192f, 1.5707963267948966192f };
	static const __declspec(align(16)) float _pi[4] = { 3.141592653589793238f, 3.141592653589793238f, 3.141592653589793238f, 3.141592653589793238f };


	typedef __declspec(align(16)) union {
		float f[4];
		__m128 v;
	} m128;

	inline __m128 sqrt_ps(const __m128 squared) {
		return _mm_sqrt_ps(squared);
	}

	__forceinline __m128 cos_52s_ps(const __m128 x)
	{
		const auto c1 = _mm_set1_ps(0.9999932946f);
		const auto c2 = _mm_set1_ps(-0.4999124376f);
		const auto c3 = _mm_set1_ps(0.0414877472f);
		const auto c4 = _mm_set1_ps(-0.0012712095f);
		const auto x2 = _mm_mul_ps(x, x);
		return _mm_add_ps(c1, _mm_mul_ps(x2, _mm_add_ps(c2, _mm_mul_ps(x2, _mm_add_ps(c3, _mm_mul_ps(c4, x2))))));
	}

	__forceinline void sincos_ps(__m128 angle, __m128 *sin, __m128 *cos) {
		const auto anglesign = _mm_or_ps(_mm_set1_ps(1.f), _mm_and_ps(signmask, angle));
		angle = _mm_andnot_ps(signmask, angle);
		angle = _mm_sub_ps(angle, _mm_mul_ps(_mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_mul_ps(angle, _mm_set1_ps(invtwopi)))), _mm_set1_ps(twopi)));

		auto cosangle = angle;
		cosangle = _mm_xor_ps(cosangle, _mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(halfpi)), _mm_xor_ps(cosangle, _mm_sub_ps(_mm_set1_ps(pi), angle))));
		cosangle = _mm_xor_ps(cosangle, _mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(pi)), signmask));
		cosangle = _mm_xor_ps(cosangle, _mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(threehalfpi)), _mm_xor_ps(cosangle, _mm_sub_ps(_mm_set1_ps(twopi), angle))));

		auto result = cos_52s_ps(cosangle);
		result = _mm_xor_ps(result, _mm_and_ps(_mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(halfpi)), _mm_cmplt_ps(angle, _mm_set1_ps(threehalfpi))), signmask));
		*cos = result;

		const auto sinmultiplier = _mm_mul_ps(anglesign, _mm_or_ps(_mm_set1_ps(1.f), _mm_and_ps(_mm_cmpgt_ps(angle, _mm_set1_ps(pi)), signmask)));
		*sin = _mm_mul_ps(sinmultiplier, sqrt_ps(_mm_sub_ps(_mm_set1_ps(1.f), _mm_mul_ps(result, result))));
	}


	// degrees to radians.
	__forceinline constexpr float deg_to_rad(float val) {
		return val * (pi / 180.f);
	}

	// radians to degrees.
	__forceinline constexpr float rad_to_deg(float val) {
		return val * (180.f / pi);
	}

	// angle mod ( shitty normalize ).
	__forceinline float AngleMod(float angle) {
		return (360.f / 65536) * ((int)(angle * (65536.f / 360.f)) & 65535);
	}

	vec3_t CalcAngle(const vec3_t& vecSource, const vec3_t& vecDestination);
	
	void AngleVectors_2(const vec3_t& angles, vec3_t* forward);

    void AngleMatrix( const ang_t& ang, const vec3_t& pos, matrix3x4_t& out );

	matrix3x4_t AngleMatrix(const ang_t angles);

    float SegmentToSegment(const vec3_t s1, const vec3_t s2, const vec3_t k1, const vec3_t k2);

	bool IntersectLineWithBB(vec3_t & start, vec3_t & end, vec3_t & min, vec3_t & max);

    // normalizes an angle.
    void NormalizeAngle( float &angle );

    ang_t CalculateAngle(vec3_t vecOrigin, vec3_t vecOther);

	ang_t vec_to_ang(const vec3_t & vecSource);

	vec3_t ang_to_vec(const ang_t & vecSource);

    __forceinline float NormalizedAngle( float angle ) {
        NormalizeAngle( angle );
        return angle;
    }

    float ApproachAngle( float target, float value, float speed );
    void  VectorAngles( const vec3_t& forward, ang_t& angles, vec3_t* up = nullptr );
    void  AngleVectors( const ang_t& angles, vec3_t* forward, vec3_t* right = nullptr, vec3_t* up = nullptr );
    float GetFOV( const ang_t &view_angles, const vec3_t &start, const vec3_t &end );
    void  VectorTransform( const vec3_t& in, const matrix3x4_t& matrix, vec3_t& out );
    void  VectorITransform( const vec3_t& in, const matrix3x4_t& matrix, vec3_t& out );
	vec3_t VectorRotate(const vec3_t & in1, const matrix3x4_t & in2);
	vec3_t VectorRotate(const vec3_t & in1, const ang_t & in2);
	void VectorIRotate(const vec3_t & in1, const matrix3x4_t & in2, vec3_t & out);
    void  MatrixAngles( const matrix3x4_t& matrix, ang_t& angles );
    void  MatrixCopy( const matrix3x4_t &in, matrix3x4_t &out );
    void  ConcatTransforms( const matrix3x4_t &in1, const matrix3x4_t &in2, matrix3x4_t &out );

    // computes the intersection of a ray with a box ( AABB ).
    bool IntersectRayWithBox( const vec3_t &start, const vec3_t &delta, const vec3_t &mins, const vec3_t &maxs, float tolerance, BoxTraceInfo_t *out_info );
    bool IntersectRayWithBox( const vec3_t &start, const vec3_t &delta, const vec3_t &mins, const vec3_t &maxs, float tolerance, CBaseTrace *out_tr, float *fraction_left_solid = nullptr );

    // computes the intersection of a ray with a oriented box ( OBB ).
    bool IntersectRayWithOBB( const vec3_t &start, const vec3_t &delta, const matrix3x4_t &obb_to_world, const vec3_t &mins, const vec3_t &maxs, float tolerance, CBaseTrace *out_tr );
    bool IntersectRayWithOBB( const vec3_t &start, const vec3_t &delta, const vec3_t &box_origin, const ang_t &box_rotation, const vec3_t &mins, const vec3_t &maxs, float tolerance, CBaseTrace *out_tr );

    // returns whether or not there was an intersection of a sphere against an infinitely extending ray. 
    // returns the two intersection points.
    bool IntersectInfiniteRayWithSphere( const vec3_t &start, const vec3_t &delta, const vec3_t &sphere_center, float radius, float *out_t1, float *out_t2 );

    // returns whether or not there was an intersection, also returns the two intersection points ( clamped 0.f to 1.f. ).
    // note: the point of closest approach can be found at the average t value.
    bool IntersectRayWithSphere( const vec3_t &start, const vec3_t &delta, const vec3_t &sphere_center, float radius, float *out_t1, float *out_t2 );

	vec3_t Interpolate( const vec3_t from, const vec3_t to, const float percent );

    template < typename t >
    __forceinline void clamp( t& n, const t& lower, const t& upper ) {
        n = std::max( lower, std::min( n, upper ) );
    }
}