/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Callbacks : irc_callbacks_t
{
	template<class HandlerNumeric,
	         class HandlerNamed>
	Callbacks(HandlerNumeric&& handler_numeric, HandlerNamed&& handler_named)
	{
		event_numeric         = handler_numeric;
		event_unknown         = handler_named;
		event_connect         = handler_named;
		event_nick            = handler_named;
		event_quit            = handler_named;
		event_join            = handler_named;
		event_part            = handler_named;
		event_mode            = handler_named;
		event_umode           = handler_named;
		event_topic           = handler_named;
		event_kick            = handler_named;
		event_channel         = handler_named;
		event_privmsg         = handler_named;
		event_notice          = handler_named;
		event_channel_notice  = handler_named;
		event_invite          = handler_named;
		event_ctcp_req        = handler_named;
		event_ctcp_rep        = handler_named;
		event_ctcp_action     = handler_named;
		//event_dcc_chat_req    = &handler_dcc;
		//event_dcc_send_req    = &handler_dcc;
	}
};


extern Callbacks callbacks;  // irclib.cpp  (callback stack base)
