#include "includes.h"

#define NET_FRAMES_BACKUP 64 // must be power of 2. 
#define NET_FRAMES_MASK ( NET_FRAMES_BACKUP - 1 )

int Hooks::SendDatagram(void* data) {
	int backup1 = c_csgo::get()->m_net->m_in_rel_state;
	int backup2 = c_csgo::get()->m_net->m_in_seq;

	int ret = g_hooks.m_net_channel.GetOldMethod< SendDatagram_t >(INetChannel::SENDDATAGRAM)(this, data);

	c_csgo::get()->m_net->m_in_rel_state = backup1;
	c_csgo::get()->m_net->m_in_seq = backup2;

	return ret;
}

void Hooks::ProcessPacket(void* packet, bool header) {
	g_hooks.m_net_channel.GetOldMethod< ProcessPacket_t >(INetChannel::PROCESSPACKET)(this, packet, header);

	c_client::get()->UpdateIncomingSequences();

	// get this from CL_FireEvents string "Failed to execute event for classId" in engine.dll
	for (CEventInfo* it{ c_csgo::get()->m_cl->m_events }; it != nullptr; it = it->m_next) {
		if (!it->m_class_id)
			continue;

		// set all delays to instant.
		it->m_fire_delay = 0.f;
	}

	// game events are actually fired in OnRenderStart which is WAY later after they are received
	// effective delay by lerp time, now we call them right after theyre received (all receive proxies are invoked without delay).
	c_csgo::get()->m_engine->FireEvents();
}