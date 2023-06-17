#include "includes.h"

NetData g_netdata{};;

void NetData::store( ) {
    int          tickbase;
    StoredData_t *data;

	if( !c_client::get()->m_processing ) {
		reset( );
		return;
	}

    tickbase = c_client::get()->m_local->m_nTickBase( );

    // get current record and store data.
    data = &m_data[ tickbase % MULTIPLAYER_BACKUP ];
    
    data->m_tickbase    = tickbase;
    data->m_punch       = c_client::get()->m_local->m_aimPunchAngle( );
    data->m_punch_vel   = c_client::get()->m_local->m_aimPunchAngleVel( );
	data->m_view_offset = c_client::get()->m_local->m_vecViewOffset( );
}

void NetData::apply( ) {
    int          tickbase;
    StoredData_t *data;
    ang_t        punch_delta, punch_vel_delta;
	vec3_t       view_delta;

    if( !c_client::get()->m_processing ) {
		reset( );
		return;
	}

    tickbase = c_client::get()->m_local->m_nTickBase( );
    
    // get current record and validate.
    data = &m_data[ tickbase % MULTIPLAYER_BACKUP ];

    if( c_client::get()->m_local->m_nTickBase( ) != data->m_tickbase )
    	return;
    
    // get deltas.
    // note - dex;  before, when you stop shooting, punch values would sit around 0.03125 and then goto 0 next update.
    //              with this fix applied, values slowly decay under 0.03125.
    punch_delta     = c_client::get()->m_local->m_aimPunchAngle( ) - data->m_punch;
    punch_vel_delta = c_client::get()->m_local->m_aimPunchAngleVel( ) - data->m_punch_vel;
	view_delta      = c_client::get()->m_local->m_vecViewOffset( ) - data->m_view_offset;
    
    // set data.
    if( std::abs( punch_delta.x ) < 0.03125f &&
    	std::abs( punch_delta.y ) < 0.03125f &&
    	std::abs( punch_delta.z ) < 0.03125f )
    	c_client::get()->m_local->m_aimPunchAngle( ) = data->m_punch;
    
    if( std::abs( punch_vel_delta.x ) < 0.03125f &&
    	std::abs( punch_vel_delta.y ) < 0.03125f &&
    	std::abs( punch_vel_delta.z ) < 0.03125f )
    	c_client::get()->m_local->m_aimPunchAngleVel( ) = data->m_punch_vel;

	if( std::abs( view_delta.x ) < 0.03125f &&
		std::abs( view_delta.y ) < 0.03125f &&
		std::abs( view_delta.z ) < 0.03125f )
		c_client::get()->m_local->m_vecViewOffset( ) = data->m_view_offset;
}

void NetData::reset( ) {
	m_data.fill( StoredData_t( ) );
}