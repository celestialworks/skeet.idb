#include "includes.h"

bool c_bones::setup( Player* player, BoneArray* out, LagRecord* record ) {
	// if the record isnt setup yet.
	if( !record->m_setup ) {
		// run setupbones rebuilt.
		if( !BuildBones( player, 0x7FF00, record->m_bones, record ) )
			return false;

		// we have setup this record bones.
		record->m_setup = true;
	}

	// record is setup.
	if( out && record->m_setup )
		std::memcpy( out, record->m_bones, sizeof( BoneArray ) * 128 );

	return true;
}

bool c_bones::BuildBones( Player* target, int mask, BoneArray* out, LagRecord* record ) {
	//quaternion_t     q[ 128 ];
	float            backup_poses[ 24 ];
	C_AnimationLayer backup_layers[ 13 ];

	if (!target || !target->IsPlayer() || !target->alive())
		return false;

	// get hdr.
	CStudioHdr* hdr = target->GetModelPtr( );
	if( !hdr )
		return false;

	// get ptr to bone accessor.
	CBoneAccessor* accessor = &target->m_BoneAccessor( );
	if( !accessor )
		return false;

	// store origial output matrix.
	// likely cachedbonedata.
	BoneArray* backup_matrix = accessor->m_pBones;
	if( !backup_matrix )
		return false;

	// backup original.
	vec3_t vecAbsOrigin = target->GetAbsOrigin();
	ang_t vecAbsAngles = target->GetAbsAngles();

	vec3_t vecBackupAbsOrigin = target->GetAbsOrigin();
	ang_t vecBackupAbsAngles = target->GetAbsAngles();

	// compute transform from raw data.
	matrix3x4_t transform;
	math::AngleMatrix( vecAbsAngles, vecAbsOrigin, transform );

	// set non interpolated data.
	target->AddEffect( EF_NOINTERP );
	target->SetAbsOrigin( record->m_pred_origin );
	target->SetAbsAngles( record->m_abs_ang );

	vec3_t pos[128];
	__declspec(align(16)) quaternion_t     q[128];

	// force game to call AccumulateLayers - pvs fix.
	m_running = true;

	// set bone array for write.
	accessor->m_pBones = out;

	// compute and build bones.
	target->StandardBlendingRules( hdr, pos, q, record->m_pred_time, mask );

	uint8_t computed[ 0x100 ];
	std::memset( computed, 0, 0x100 );
	target->BuildTransformations( hdr, pos, q, transform, mask, computed );

	// restore old matrix.
	accessor->m_pBones = backup_matrix;

	// restore original interpolated entity data.
	target->SetAbsOrigin( vecBackupAbsOrigin );
	target->SetAbsAngles( vecBackupAbsAngles );

	// revert to old game behavior.
	m_running = false;

	return true;
}