#include "includes.h"

CMatchSessionOnlineHost* Hooks::GetMatchSession( ) {
	Stack       stack;
	Address     kv;
	std::string map;

	// ClientJob_EMsgGCCStrike15_v2_MatchmakingGC2ClientReserve::BYieldingRunJobFromMsg
	static Address BYieldingRunJobFromMsg{ pattern::find( c_csgo::get()->m_client_dll, XOR( "8B D8 85 DB 0F 84 ? ? ? ? 8B 13 8B CB 68 ? ? ? ? 68" ) ) };

	// NOTE;
	// string xref "game/mmqueue" above that string is a virtual call which is g_pMatchFramework->GetMatchSession( )
	// string xref "Matchmaking reservation confirmed: %llx/%s\n"

	if(c_config::get()->m["misc_notifications"][0] && stack.ReturnAddress( ) == BYieldingRunJobFromMsg ) {
		// note - nitro:
		// deref ebp = old stack -> old stack - 0xA8 -> keyvalue ptr -> deref keyvalue ptr + 0x24 = mapname

		kv = stack.next( ).local( 0xA8 ).get( );

		if( kv && kv.at( 0x24 ) ) {
			map = { kv.at( 0x24 ).as< char* >( ) };
			std::string out = tfm::format(XOR("%s\n"), map);
			c_event_logs::get()->add(out);
		}
	}

	// call og.
	return g_hooks.m_match_framework.GetOldMethod< GetMatchSession_t >( CMatchFramework::GETMATCHSESSION )( this );
}