#include "includes.h"

Animations g_anims;

void Animations::AnimationInfo_t::UpdateAnimations( LagComp::LagRecord_t* pRecord, LagComp::LagRecord_t* pPreviousRecord ) {
	if( !pPreviousRecord ) {
		pRecord->m_vecVelocity = m_pEntity->m_vecVelocity( );

		c_resolver::get()->ResolveAngles( m_pEntity, pRecord );

		pRecord->Apply( m_pEntity );

		g_anims.UpdatePlayer( m_pEntity );

		return;
	}

	vec3_t vecVelocity = m_pEntity->m_vecVelocity( );

	m_pEntity->SetAnimLayers( pRecord->m_pLayers );
	m_pEntity->SetAbsOrigin( pRecord->m_vecOrigin );
	m_pEntity->SetAbsAngles( pRecord->m_angAbsAngles );
	m_pEntity->m_vecVelocity( ) = pPreviousRecord->m_vecVelocity;

	pRecord->m_vecVelocity = vecVelocity;

	pRecord->m_bDidShot = pRecord->m_flLastShotTime > pPreviousRecord->m_flSimulationTime && pRecord->m_flLastShotTime <= pRecord->m_flSimulationTime;

	vec3_t vecPreviousOrigin = pPreviousRecord->m_vecOrigin;
	int fPreviousFlags = pPreviousRecord->m_fFlags;

	for( int i = 0; i < pRecord->m_iChoked; ++i ) {
		const float flSimulationTime = pPreviousRecord->m_flSimulationTime + game::TICKS_TO_TIME( i + 1 );
		const float flLerp = 1.f - ( pRecord->m_flSimulationTime - flSimulationTime ) / ( pRecord->m_flSimulationTime - pPreviousRecord->m_flSimulationTime );

		if( !pRecord->m_bDidShot ) {
			auto vecEyeAngles = math::Interpolate( pPreviousRecord->m_angEyeAngles, pRecord->m_angEyeAngles, flLerp );
			m_pEntity->m_angEyeAngles( ).y = vecEyeAngles.y;
		}

		m_pEntity->m_flDuckAmount( ) = math::Interpolate( pPreviousRecord->m_flDuck, pRecord->m_flDuck, flLerp );

		if( pRecord->m_iChoked - 1 == i ) {
			m_pEntity->m_vecVelocity( ) = vecVelocity;
			m_pEntity->m_fFlags( ) = pRecord->m_fFlags;
		}
		else {
			g_game_movement.Extrapolate( m_pEntity, vecPreviousOrigin, m_pEntity->m_vecVelocity( ), m_pEntity->m_fFlags( ), fPreviousFlags & FL_ONGROUND );
			fPreviousFlags = m_pEntity->m_fFlags( );

			//pRecord->m_vecVelocity = ( pRecord->m_vecOrigin - pPreviousRecord->m_vecOrigin ) * ( 1.f / game::TICKS_TO_TIME( pRecord->m_iChoked ) );
		}

		if( pRecord->m_bDidShot ) {
			/* NOTE FROM ALPHA:
			* This is to attempt resolving players' onshot desync, using the last angle they were at before they shot
			* It's good enough, but I will leave another (alternative) concept that could work when attempting to resolve on-shot desync
			* I haven't tested it at all, but the logic is there and in theory it should work fine (it's missing nullptr checks, obviously, since it's a concept)
			*
			* float flPseudoFireYaw = Math::NormalizeYaw( Math::CalculateAngle( m_pPlayer->GetBonePos( 8 ), m_pPlayer->GetBonePos( 0 ) ).y );
			* float flLeftFireYawDelta = fabsf( Math::NormalizeYaw( flPseudoFireYaw - ( pRecord->m_angEyeAngles.y + 58.f ) ) );
			* float flRightFireYawDelta = fabsf( Math::NormalizeYaw( flPseudoFireYaw - ( pRecord->m_angEyeAngles.y - 58.f ) ) );
			*
			* pRecord->m_pState->m_flGoalFeetYaw = pRecord->m_pState->m_flEyeYaw + ( flLeftFireYawDelta > flRightFireYawDelta ? -58.f : 58.f );
			*/

			m_pEntity->m_angEyeAngles( ) = pPreviousRecord->m_angLastReliableAngle;

			if( pRecord->m_flLastShotTime <= flSimulationTime ) {
				m_pEntity->m_angEyeAngles( ) = pRecord->m_angEyeAngles;
			}
		}

		// correct shot desync.
		if (pRecord->m_bDidShot) {
			m_pEntity->m_angEyeAngles() = pRecord->m_angLastReliableAngle;

			if (pRecord->m_flLastShotTime <= flSimulationTime) {
				m_pEntity->m_angEyeAngles() = pRecord->m_angEyeAngles;
			}
		}

		// fix feet spin.
		m_pEntity->m_PlayerAnimState()->m_flFeetWeight = 0.f;

		const float flBackupSimtime = m_pEntity->m_flSimulationTime( );

		m_pEntity->m_flSimulationTime( ) = flSimulationTime;

		c_resolver::get()->ResolveAngles( m_pEntity, pRecord );

		g_anims.UpdatePlayer( m_pEntity );

		m_pEntity->m_flSimulationTime( ) = flBackupSimtime;
	}
}

void Animations::UpdatePlayer( Player* pEntity ) {
	static bool& invalidate_bone_cache = **reinterpret_cast< bool** >( pattern::find( c_csgo::get()->m_client_dll, XOR( "C6 05 ? ? ? ? ? 89 47 70" ) ).add( 2 ).as<uint32_t>( ) );

	const float frametime = c_csgo::get()->m_globals->m_frametime;
	const float curtime = c_csgo::get()->m_globals->m_curtime;
	const auto old_flags = pEntity->m_fFlags();

	CCSGOPlayerAnimState* const state = pEntity->m_PlayerAnimState( );
	if( !state ) {
		return;
	}

	if (state->m_iLastClientSideAnimationUpdateFramecount == c_csgo::get()->m_globals->m_frame) {
		state->m_iLastClientSideAnimationUpdateFramecount -= 1;
	}

	c_csgo::get()->m_globals->m_frametime = c_csgo::get()->m_globals->m_interval;
	c_csgo::get()->m_globals->m_curtime = pEntity->m_flSimulationTime( );

	// skip call to C_BaseEntity::CalcAbsoluteVelocity
	pEntity->m_iEFlags( ) &= ~0x1000; // EFL_DIRTY_ABSVELOCITY

	pEntity->m_vecAbsVelocity() = pEntity->m_vecVelocity();

	const bool backup_invalidate_bone_cache = invalidate_bone_cache;

	pEntity->m_bClientSideAnimation( ) = true;
	pEntity->UpdateClientSideAnimation( );
	pEntity->m_bClientSideAnimation( ) = false;

	pEntity->InvalidatePhysicsRecursive( ANGLES_CHANGED );
	pEntity->InvalidatePhysicsRecursive( ANIMATION_CHANGED );
	pEntity->InvalidatePhysicsRecursive( SEQUENCE_CHANGED );

	invalidate_bone_cache = backup_invalidate_bone_cache;

	c_csgo::get()->m_globals->m_frametime = frametime;
	c_csgo::get()->m_globals->m_curtime = curtime;

	pEntity->m_fFlags() = old_flags;
}

bool Animations::GenerateMatrix( Player* pEntity, BoneArray* pBoneToWorldOut, int boneMask, float currentTime )
{
	if( !pEntity || !pEntity->IsPlayer( ) || !pEntity->alive( ) )
		return false;

	CStudioHdr* pStudioHdr = pEntity->GetModelPtr( );
	if( !pStudioHdr ) {
		return false;
	}

	// get ptr to bone accessor.
	CBoneAccessor* accessor = &pEntity->m_BoneAccessor( );
	if( !accessor )
		return false;

	// store origial output matrix.
	// likely cachedbonedata.
	BoneArray* backup_matrix = accessor->m_pBones;
	if( !backup_matrix )
		return false;

	vec3_t vecAbsOrigin = pEntity->GetAbsOrigin( );
	ang_t vecAbsAngles = pEntity->GetAbsAngles( );

	vec3_t vecBackupAbsOrigin = pEntity->GetAbsOrigin( );
	ang_t vecBackupAbsAngles = pEntity->GetAbsAngles( );

	matrix3x4a_t parentTransform;
	math::AngleMatrix( vecAbsAngles, vecAbsOrigin, parentTransform );

	pEntity->m_fEffects( ) |= 8;

	pEntity->SetAbsOrigin( vecAbsOrigin );
	pEntity->SetAbsAngles( vecAbsAngles );

	vec3_t pos[ 128 ];
	__declspec( align( 16 ) ) quaternion_t     q[ 128 ];

	g_anims.m_bEnablePVS = true;

	uint32_t fBackupOcclusionFlags = pEntity->m_iOcclusionFlags( );

	pEntity->m_iOcclusionFlags( ) |= 0xA; // skipp call to accumulatelayers in standardblendingrules

	pEntity->StandardBlendingRules( pStudioHdr, pos, q, currentTime, boneMask );

	pEntity->m_iOcclusionFlags( ) = fBackupOcclusionFlags; // standardblendingrules was called now restore niggaaaa

	accessor->m_pBones = pBoneToWorldOut;

	uint8_t computed[ 0x100 ];
	std::memset( computed, 0, 0x100 );

	pEntity->BuildTransformations( pStudioHdr, pos, q, parentTransform, boneMask, computed );

	accessor->m_pBones = backup_matrix;

	pEntity->SetAbsOrigin( vecBackupAbsOrigin );
	pEntity->SetAbsAngles( vecBackupAbsAngles );

	g_anims.m_bEnablePVS = false;

	return true;
}

void Animations::UpdateLocalAnimations( ) {
	if( !c_client::get()->m_cmd || !c_client::get()->m_processing )
		return;

	CCSGOPlayerAnimState* state = c_client::get()->m_local->m_PlayerAnimState( );
	if( !state ) {
		return;
	}

	if( !c_csgo::get()->m_cl ) {
		return;
	}

	// allow re-animating in the same frame.
	if( state->m_iLastClientSideAnimationUpdateFramecount == c_csgo::get()->m_globals->m_frame ) {
		state->m_iLastClientSideAnimationUpdateFramecount -= 1;
	}

	// update anim update delta as server build.
	state->m_flAnimUpdateDelta = math::Max( 0.0f, c_csgo::get()->m_globals->m_curtime - state->m_flLastClientSideAnimationUpdateTime ); // negative values possible when clocks on client and server go out of sync..

	if( c_client::get()->m_animate ) {
		// get layers.
		c_client::get()->m_local->GetAnimLayers( c_client::get()->m_real_layers );

		// allow the game to update animations this tick.
		c_client::get()->m_update = true;

		// update animations.
		game::UpdateAnimationState( state, c_client::get()->m_angle );
		c_client::get()->m_local->UpdateClientSideAnimation( );

		// disallow the game from updating animations this tick.
		c_client::get()->m_update = false;

		// save data when our choke cycle resets.
		if( !c_csgo::get()->m_cl->m_choked_commands ) {
			c_client::get()->m_rotation.y = state->m_flGoalFeetYaw;
			c_client::get()->m_local->GetPoseParameters( c_client::get()->m_real_poses );
		}

		// remove model sway.
		c_client::get()->m_real_layers[ 12 ].m_weight = 0.f;

		// make sure to only animate once per tick.
		c_client::get()->m_animate = false;
	}

	// update our layers and poses with the saved ones.
	c_client::get()->m_local->SetAnimLayers( c_client::get()->m_real_layers );
	c_client::get()->m_local->SetPoseParameters( c_client::get()->m_real_poses );

	// update our real rotation.
	c_client::get()->m_local->SetAbsAngles( c_client::get()->m_rotation );

	// build bones.
	GenerateMatrix( c_client::get()->m_local, c_client::get()->m_real_matrix, 0x7FF00, c_client::get()->m_local->m_flSimulationTime( ) );
}

Animations::AnimationInfo_t* Animations::GetAnimationInfo( Player* pEntity ) {
	auto pInfo = Animations::m_ulAnimationInfo.find( pEntity->GetRefEHandle( ) );
	if( pInfo == Animations::m_ulAnimationInfo.end( ) ) {
		return nullptr;
	}

	return &pInfo->second;
}