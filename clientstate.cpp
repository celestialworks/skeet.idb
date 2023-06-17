#include "includes.h"

void Hooks::PacketStart(int m_iIncomingSequence, int m_iOutgoingAcknowledged) {
	/*if (!c_client::get()->m_processing) {
		return g_hooks.m_client_state.GetOldMethod< PacketStart_t >(5)(this, m_iIncomingSequence, m_iOutgoingAcknowledged);
	}

	for (auto it = c_client::get()->m_cmds.begin(); it != c_client::get()->m_cmds.end(); it++) {
		if (*it == m_iOutgoingAcknowledged) {
			c_client::get()->m_cmds.erase(it);
			return g_hooks.m_client_state.GetOldMethod< PacketStart_t >(5)(this, m_iIncomingSequence, m_iOutgoingAcknowledged);
		}
	}*/
	g_hooks.m_client_state.GetOldMethod< PacketStart_t >(5)(this, m_iIncomingSequence, m_iOutgoingAcknowledged);
}

bool Hooks::TempEntities( void *msg ) {
	if( !c_client::get()->m_processing ) {
		return g_hooks.m_client_state.GetOldMethod< TempEntities_t >( 36 )( this, msg );
	}

	const bool ret = g_hooks.m_client_state.GetOldMethod< TempEntities_t >( 36 )( this, msg );

	CEventInfo *ei = c_csgo::get()->m_cl->m_events; 
	CEventInfo *next = nullptr;

	if( !ei ) {
		return ret;
	}

	do {
		next = *reinterpret_cast< CEventInfo ** >( reinterpret_cast< uintptr_t >( ei ) + 0x38 );

		uint16_t classID = ei->m_class_id - 1;

		auto m_pCreateEventFn = ei->m_client_class->m_pCreateEvent;
		if( !m_pCreateEventFn ) {
			continue;
		}

		void *pCE = m_pCreateEventFn( );
		if( !pCE ) {
			continue;
		}

		if( classID == 170 ){
			ei->m_fire_delay = 0.0f;
		}
		ei = next;
	} while( next != nullptr );

	return ret;
}