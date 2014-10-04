/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include "bot.h"


Bot::Bot(const Ident &ident,
         irc_callbacks_t &cbs)
try:
adb(ident["dbdir"]),
sess(ident,cbs),
chans(adb,sess),
users(adb,sess)
{
	irc_set_ctx(sess,this);

}
catch(const Exception &e)
{
	std::cerr << "Bot::Bot(): " << e << std::endl;
}


Bot::~Bot(void)
noexcept try
{
	sess.disconn();
}
catch(const Exception &e)
{
	std::cerr << "Bot::~Bot(): " << e << std::endl;
	return;
}


void Bot::operator()(const char *const &event,
                     const char *const &origin,
                     const char **const &params,
                     const size_t &count)
{
	const Msg msg(0,origin,params,count);

	switch(hash(event))
	{
		case hash("CTCP_ACTION"):      handle_ctcp_act(msg);            break;
		case hash("CTCP_REP"):         handle_ctcp_rep(msg);            break;
		case hash("CTCP_REQ"):         handle_ctcp_req(msg);            break;
		case hash("ACCOUNT"):          handle_account(msg);             break;
		case hash("INVITE"):           handle_invite(msg);              break;
		case hash("CHANNEL_NOTICE"):   handle_cnotice(msg);             break;
		case hash("NOTICE"):           handle_notice(msg);              break;
		case hash("PRIVMSG"):          handle_privmsg(msg);             break;
		case hash("CHANNEL"):          handle_chanmsg(msg);             break;
		case hash("KICK"):             handle_kick(msg);                break;
		case hash("TOPIC"):            handle_topic(msg);               break;
		case hash("UMODE"):            handle_umode(msg);               break;
		case hash("MODE"):             handle_mode(msg);                break;
		case hash("PART"):             handle_part(msg);                break;
		case hash("JOIN"):             handle_join(msg);                break;
		case hash("QUIT"):             handle_quit(msg);                break;
		case hash("NICK"):             handle_nick(msg);                break;
		case hash("CAP"):              handle_cap(msg);                 break;
		case hash("CONNECT"):          handle_conn(msg);                break;
		default:                       handle_unhandled(msg,event);     break;
	}
}


void Bot::operator()(const uint32_t &event,
                     const char *const &origin,
                     const char **const &params,
                     const size_t &count)
{
	const Msg msg(event,origin,params,count);

	switch(event)
	{
		case LIBIRC_RFC_RPL_WELCOME:              handle_welcome(msg);                 break;
		case LIBIRC_RFC_RPL_YOURHOST:             handle_yourhost(msg);                break;
		case LIBIRC_RFC_RPL_CREATED:              handle_created(msg);                 break;
		case LIBIRC_RFC_RPL_MYINFO:               handle_myinfo(msg);                  break;
		case LIBIRC_RFC_RPL_BOUNCE:               handle_bounce(msg);                  break;

		case LIBIRC_RFC_RPL_NAMREPLY:             handle_namreply(msg);                break;
		case LIBIRC_RFC_RPL_ENDOFNAMES:           handle_endofnames(msg);              break;
		case LIBIRC_RFC_RPL_UMODEIS:              handle_umodeis(msg);                 break;
		case LIBIRC_RFC_RPL_AWAY:                 handle_away(msg);                    break;
		case LIBIRC_RFC_RPL_WHOREPLY:             handle_whoreply(msg);                break;
		case 354     /* RPL_WHOSPCRPL */:         handle_whospecial(msg);              break;
		case LIBIRC_RFC_RPL_WHOISUSER:            handle_whoisuser(msg);               break;
		case LIBIRC_RFC_RPL_WHOISIDLE:            handle_whoisidle(msg);               break;
		case LIBIRC_RFC_RPL_WHOISSERVER:          handle_whoisserver(msg);             break;
		case 671     /* RPL_WHOISSECURE */:       handle_whoissecure(msg);             break;
		case 330     /* RPL_WHOISACCOUNT */:      handle_whoisaccount(msg);            break;
		case LIBIRC_RFC_RPL_ENDOFWHOIS:           handle_endofwhois(msg);              break;
		case LIBIRC_RFC_RPL_WHOWASUSER:           handle_whowasuser(msg);              break;
		case LIBIRC_RFC_RPL_CHANNELMODEIS:        handle_channelmodeis(msg);           break;
		case LIBIRC_RFC_RPL_TOPIC:                handle_topic(msg);                   break;
		case 333     /* RPL_TOPICWHOTIME */:      handle_topicwhotime(msg);            break;
		case 329     /* RPL_CREATIONTIME */:      handle_creationtime(msg);            break;
		case LIBIRC_RFC_RPL_BANLIST:              handle_banlist(msg);                 break;
		case 728     /* RPL_QUIETLIST */:         handle_quietlist(msg);               break;

		case LIBIRC_RFC_ERR_CHANOPRIVSNEEDED:     handle_chanoprivsneeded(msg);        break;
		case LIBIRC_RFC_ERR_ERRONEUSNICKNAME:     handle_erroneusnickname(msg);        break;
		case LIBIRC_RFC_ERR_BANNEDFROMCHAN:       handle_bannedfromchan(msg);          break;
		default:                                  handle_unhandled(msg);               break;
	}

}


void Bot::run()
{
	sess.call(irc_run);              // Loops forever here

	// see: handle_quit()
	std::cout << "Worker exit clean." << std::endl;
}


void Bot::handle_conn(const Msg &msg)
{
	log_handle(msg,"CONNECT");

	Sess &sess = get_sess();
	sess.cap_ls();
	sess.cap_req("account-notify");
	sess.cap_req("extended-join");
	sess.cap_end();

	const Ident &id = get_sess().get_ident();
	for(const auto &chan : id.autojoin)
		join(chan);
}


void Bot::handle_welcome(const Msg &msg)
{
	log_handle(msg,"WELCOME");

}


void Bot::handle_yourhost(const Msg &msg)
{
	log_handle(msg,"YOURHOST");

}


void Bot::handle_created(const Msg &msg)
{
	log_handle(msg,"CREATED");

}


void Bot::handle_myinfo(const Msg &msg)
{
	using namespace fmt::MYINFO;

	log_handle(msg,"MYINFO");

	Sess &sess = get_sess();
	Server &server = sess.server;

	server.name = msg[SERVNAME];
	server.vers = msg[VERSION];
	server.user_modes = msg[USERMODS];
	server.chan_modes = msg[CHANMODS];
	server.chan_pmodes = msg[CHANPARM];
}


void Bot::handle_bounce(const Msg &msg)
{
	log_handle(msg,"BOUNCE");

	Sess &sess = get_sess();
	Server &server = sess.server;

	const std::string &self = msg[0];
	for(size_t i = 1; i < msg.num_params(); i++)
	{
		const std::string &opt = msg[i];
		const size_t sep = opt.find('=');
		const std::string key = opt.substr(0,sep);
		const std::string val = sep == std::string::npos? "" : opt.substr(sep+1);

		if(!std::all_of(key.begin(),key.end(),::isupper))
			continue;

		server.cfg.emplace(key,val);
	}
}


void Bot::handle_cap(const Msg &msg)
{
	using namespace fmt::CAP;

	log_handle(msg,"CAP");

	Sess &sess = get_sess();
	Server &server = sess.server;

	using delim = boost::char_separator<char>;
	static const delim sep(" ");
	const boost::tokenizer<delim> caps(msg[CAPLIST],sep);

	switch(hash(msg[COMMAND]))
	{
		case hash("LS"):
			server.caps.insert(caps.begin(),caps.end());
			break;

		case hash("ACK"):
			sess.caps.insert(caps.begin(),caps.end());
			break;

		case hash("LIST"):
			sess.caps.clear();
			sess.caps.insert(caps.begin(),caps.end());
			break;

		case hash("NAK"):
			std::cerr << "UNSUPPORTED CAPABILITIES: " << msg[CAPLIST] << std::endl;
			break;

		default:
			throw Exception("Unhandled CAP response type");
	}
}


void Bot::handle_quit(const Msg &msg)
{
	using namespace fmt::QUIT;

	log_handle(msg,"QUIT");

	const std::string nick = msg.get_nick();
	const std::string &reason = msg[REASON];

	if(my_nick(nick))
	{
		//TODO: libbirc crashes if we don't exit (note: quit() was from a sighandler)
		exit(0);
		return;
	}

	Users &users = get_users();
	User &user = users.get(nick);

	Chans &chans = get_chans();
	chans.for_each([&user]
	(Chan &chan)
	{
		chan.del(user);
	});
}


void Bot::handle_nick(const Msg &msg)
{
	using namespace fmt::NICK;

	log_handle(msg,"NICK");

	const std::string old_nick = msg.get_nick();
	const std::string &new_nick = msg[NICKNAME];

	if(my_nick(old_nick))
	{
		Sess &sess = get_sess();
		sess.set_nick(new_nick);
	}

	Users &users = get_users();
	User &user = users.get(old_nick);

	Chans &chans = get_chans();
	chans.for_each([&user,&old_nick]
	(Chan &chan)
	{
		chan.rename(user,old_nick);
	});
}


void Bot::handle_account(const Msg &msg)
{
	using namespace fmt::ACCOUNT;

	log_handle(msg,"ACCOUNT");

	Users &users = get_users();
	User &user = users.add(msg.get_nick());
	user.set_acct(msg[ACCTNAME]);
}


void Bot::handle_join(const Msg &msg)
{
	using namespace fmt::JOIN;

	log_handle(msg,"JOIN");

	Sess &sess = get_sess();
	Chans &chans = get_chans();
	Users &users = get_users();

	User &user = users.add(msg.get_nick());
	if(sess.has_cap("extended-join"))
		user.set_acct(msg[ACCTNAME]);

	Chan &chan = chans.add(msg[CHANNAME]);
	chan.add(user);

	if(my_nick(msg.get_nick()))
	{
		chan.set_joined(true);
		chan.mode();
		chan.who();
		chan.banlist();
		chan.quietlist();
	}

	handle_join(msg,chan,user);
}


void Bot::handle_part(const Msg &msg)
{
	using namespace fmt::PART;

	log_handle(msg,"PART");

	Users &users = get_users();
	Chans &chans = get_chans();

	User &user = users.get(msg.get_nick());
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.del(user);

	if(my_nick(msg.get_nick()))
	{
		chans.del(chan);
		return;
	}

	const scope free([&]
	{
		if(user.num_chans() == 0)
			users.del(user);
	});

	handle_part(msg,chan,user);
}


void Bot::handle_mode(const Msg &msg)
{
	using namespace fmt::MODE;

	log_handle(msg,"MODE");

	// Our UMODE coming as MODE from the irclib
	if(msg.num_params() == 1)
	{
		handle_umode(msg);
		return;
	}

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);

	// Channel's own mode
	if(msg.num_params() < 3)
	{
		chan.delta_mode(msg[DELTASTR]);
		handle_mode(msg,chan);
		return;
	}

	// Channel mode apropos a user
	Users &users = get_users();
	const std::string sign(1,msg[DELTASTR].at(0));
	for(size_t d = 1, m = 2; m < msg.num_params(); d++, m++) try
	{
		// Associate each mode delta with its target
		const std::string delta = sign + msg[DELTASTR].at(d);
		const Mask &targ = msg[m];
		chan.delta_mode(delta,targ);          // Chan handles everything from here

		if(targ.get_form() == Mask::INVALID)  // Target is a straight nickname, we can call downstream
		{
			User &user = users.get(targ);
			handle_mode(msg,chan,user);
		}
	}
	catch(const std::exception &e)
	{
		std::cerr << "Mode update failed:"
		          << " chan: " << chan.get_name()
		          << " target[" << m << "]: " << msg[m]
		          << " (modestr: " << msg[DELTASTR] << ")"
		          << std::endl;
	}
}


void Bot::handle_umode(const Msg &msg)
{
	using namespace fmt::UMODE;

	log_handle(msg,"UMODE");

	if(!my_nick(msg.get_nick()))
		throw Exception("Server sent us umode for a different nickname");

	Sess &sess = get_sess();
	sess.delta_mode(msg[DELTASTR]);
}


void Bot::handle_umodeis(const Msg &msg)
{
	using namespace fmt::UMODEIS;

	log_handle(msg,"UMODEIS");

	if(!my_nick(msg[NICKNAME]))
		throw Exception("Server sent us umodeis for a different nickname");

	Sess &sess = get_sess();
	sess.delta_mode(msg[DELTASTR]);
}


void Bot::handle_away(const Msg &msg)
{
	using namespace fmt::AWAY;

	log_handle(msg,"AWAY");

	Users &users = get_users();
	User &user = users.get(msg[NICKNAME]);
	user.set_away(true);
}


void Bot::handle_channelmodeis(const Msg &msg)
{
	using namespace fmt::CHANNELMODEIS;

	log_handle(msg,"CHANNELMODEIS");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.delta_mode(msg[DELTASTR]);
}


void Bot::handle_chanoprivsneeded(const Msg &msg)
{
	using namespace fmt::CHANOPRIVSNEEDED;

	log_handle(msg,"CHANOPRIVSNEEDED");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan << Chan::NOTICE << "I need to be +o to do that. (" << msg[REASON] << ")" << Chan::flush;
}


void Bot::handle_creationtime(const Msg &msg)
{
	using namespace fmt::CREATIONTIME;

	log_handle(msg,"CREATION TIME");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.set_creation(msg.get<time_t>(TIME));
}


void Bot::handle_topicwhotime(const Msg &msg)
{
	using namespace fmt::TOPICWHOTIME;

	log_handle(msg,"TOPIC WHO TIME");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	std::get<Chan::Topic::MASK>(chan.topic) = msg[MASK];
	std::get<Chan::Topic::TIME>(chan.topic) = msg.get<time_t>(TIME);
}


void Bot::handle_banlist(const Msg &msg)
{
	using namespace fmt::BANLIST;

	log_handle(msg,"BAN LIST");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.bans.add(msg[BANMASK],msg[OPERATOR],msg.get<time_t>(TIME));
}


void Bot::handle_quietlist(const Msg &msg)
{
	using namespace fmt::QUIETLIST;

	log_handle(msg,"728 LIST");

	if(msg[MODECODE] != "q")
	{
		std::cout << "Received non 'q' mode for 728" << std::endl;
		return;
	}

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.quiets.add(msg[BANMASK],msg[OPERATOR],msg.get<time_t>(TIME));
}


void Bot::handle_topic(const Msg &msg)
{
	using namespace fmt::TOPIC;

	log_handle(msg,"TOPIC");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	std::get<Chan::Topic::TEXT>(chan.topic) = msg[TEXT];
}


void Bot::handle_kick(const Msg &msg)
{
	using namespace fmt::KICK;

	log_handle(msg,"KICK");

	const std::string kicker = msg.get_nick();
	const std::string &kickee = msg.num_params() > 1? msg[KICK::TARGET] : kicker;

	Chans &chans = get_chans();

	if(my_nick(kickee))
	{
		chans.del(msg[CHANNAME]);
		chans.join(msg[CHANNAME]);
		return;
	}

	Users &users = get_users();

	User &user = users.get(kickee);
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.del(user);

	const scope free([&]
	{
		if(user.num_chans() == 0)
			users.del(user);
	});

	handle_kick(msg,chan,user);
}


void Bot::handle_chanmsg(const Msg &msg)
{
	using namespace fmt::CHANMSG;

	log_handle(msg,"CHANMSG");

	Chans &chans = get_chans();
	Users &users = get_users();

	User &user = users.get(msg.get_nick());
	Chan &chan = chans.get(msg[CHANNAME]);
	handle_chanmsg(msg,chan,user);
}


void Bot::handle_cnotice(const Msg &msg)
{
	using namespace fmt::CNOTICE;

	log_handle(msg,"CHANNEL NOTICE");

	Chans &chans = get_chans();
	Users &users = get_users();

	Chan &chan = chans.get(msg[CHANNAME]);
	User &user = users.get(msg.get_nick());
	handle_cnotice(msg,chan,user);
}


void Bot::handle_privmsg(const Msg &msg)
{
	log_handle(msg,"PRIVMSG");

	Users &users = get_users();
	User &user = users.get(msg.get_nick());
	handle_privmsg(msg,user);
}


void Bot::handle_notice(const Msg &msg)
{
	using namespace fmt::NOTICE;

	log_handle(msg,"NOTICE");

	if(msg.from_server())
		return;

	Users &users = get_users();
	User &user = users.get(msg[NICKNAME]);
	handle_notice(msg,user);
}


void Bot::handle_invite(const Msg &msg)
{
	using namespace fmt::INVITE;

	log_handle(msg,"INVITE");

	// Temp hack to throttle invites to 10 minutes
	static const time_t limit = 600;
	static time_t throttle = time(NULL);
	if(time(NULL) - throttle < limit)
		return;

	Chans &chans = get_chans();
	chans.join(msg[CHANNAME]);
	throttle = time(NULL);
}


void Bot::handle_ctcp_req(const Msg &msg)
{
	log_handle(msg,"CTCP REQ");
}


void Bot::handle_ctcp_rep(const Msg &msg)
{
	log_handle(msg,"CTCP REP");
}


void Bot::handle_ctcp_act(const Msg &msg)
{
	log_handle(msg,"CTCP ACT");
}


void Bot::handle_namreply(const Msg &msg)
{
	using namespace fmt::NAMREPLY;

	log_handle(msg,"NAM REPLY");

	if(!my_nick(msg[NICKNAME]))
		throw Exception("Server replied to the wrong nickname");

	Users &users = get_users();
	Chans &chans = get_chans();
	Chan &chan = chans.add(msg[CHANNAME]);

	using delim = boost::char_separator<char>;
	const delim sep(" ");
	const boost::tokenizer<delim> tokens(msg[NAMELIST],sep);
	for(const auto &nick: tokens)
	{
		const char f = Chan::nick_flag(nick);
		const char m = Chan::flag2mode(f);
		const std::string &rawnick = f? nick.substr(1) : nick;
		const std::string &modestr = m? std::string(1,m) : "";

		User &user = users.add(rawnick);
		chan.add(user,modestr);
	}
}


void Bot::handle_endofnames(const Msg &msg)
{
	log_handle(msg,"END OF NAMES");
}


void Bot::handle_bannedfromchan(const Msg &msg)
{
	log_handle(msg,"BANNED FROM CHAN");
}


void Bot::handle_erroneusnickname(const Msg &msg)
{
	log_handle(msg,"ERRONEOUS NICKNAME");
	quit();
}


void Bot::handle_whoreply(const Msg &msg)
{
	log_handle(msg,"WHO REPLY");

	const std::string &selfserv = msg.get_nick();
	const std::string &self = msg[WHOREPLY::SELFNAME];
	const std::string &chan = msg[WHOREPLY::CHANNAME];
	const std::string &user = msg[WHOREPLY::USERNAME];
	const std::string &host = msg[WHOREPLY::HOSTNAME];
	const std::string &serv = msg[WHOREPLY::SERVNAME];
	const std::string &nick = msg[WHOREPLY::NICKNAME];
	const std::string &flag = msg[WHOREPLY::FLAGS];
	const std::string &addl = msg[WHOREPLY::ADDL];

}


void Bot::handle_whospecial(const Msg &msg)
{
	log_handle(msg,"WHO SPECIAL");

	const std::string &server = msg.get_nick();
	const std::string &self = msg[0];
	const int recipe = msg.get<int>(1);

	switch(recipe)
	{
		case User::WHO_RECIPE:
		{
			const std::string &host = msg[2];
			const std::string &nick = msg[3];
			const std::string &acct = msg[4];
			//const time_t idle = msg.get<time_t>(4);

			Users &users = get_users();
			User &user = users.get(nick);

			user.set_host(host);
			user.set_acct(acct);
			//user.set_idle(idle);

			if(user.is_logged_in() && !user.has_acct())
				user.set_val("first_seen",time(NULL));

			break;
		}

		default:
			throw Exception("WHO SPECIAL recipe unrecognized");
	}
}


void Bot::handle_whoisuser(const Msg &msg)
{
	log_handle(msg,"WHOIS USER");

}


void Bot::handle_whoisidle(const Msg &msg)
try
{
	using namespace fmt::WHOISIDLE;

	log_handle(msg,"WHOIS IDLE");

	Users &users = get_users();
	User &user = users.get(msg[NICKNAME]);
	user.set_idle(msg.get<time_t>(SECONDS));
	user.set_signon(msg.get<time_t>(SIGNON));
}
catch(const Exception &e)
{
	std::cerr << "handle_whoisidle(): " << e << std::endl;
	return;
}


void Bot::handle_whoisserver(const Msg &msg)
{
	log_handle(msg,"WHOIS SERVER");

}


void Bot::handle_whoissecure(const Msg &msg)
try
{
	using namespace fmt::WHOISSECURE;

	log_handle(msg,"WHOIS SECURE");

	Users &users = get_users();
	User &user = users.get(msg[NICKNAME]);
	user.set_secure(true);
}
catch(const Exception &e)
{
	std::cerr << "handle_whoissecure(): " << e << std::endl;
	return;
}


void Bot::handle_whoisaccount(const Msg &msg)
try
{
	using namespace fmt::WHOISACCOUNT;

	log_handle(msg,"WHOIS ACCOUNT");

	Users &users = get_users();
	User &user = users.get(msg[NICKNAME]);
	user.set_acct(msg[ACCTNAME]);
}
catch(const Exception &e)
{
	std::cerr << "handle_whoisaccount(): " << e << std::endl;
	return;
}


void Bot::handle_whoischannels(const Msg &msg)
{
	log_handle(msg,"WHOIS CHANNELS");

}


void Bot::handle_endofwhois(const Msg &msg)
{
	log_handle(msg,"END OF WHOIS");

}


void Bot::handle_whowasuser(const Msg &msg)
{
	log_handle(msg,"WHOWAS USER");

}


void Bot::handle_nosuchnick(const Msg &msg)
{
	log_handle(msg,"NO SUCH NICK");

}


void Bot::handle_unhandled(const Msg &msg,
                           const std::string &name)
{
	log_handle(msg,name);
}


void Bot::handle_unhandled(const Msg &msg)
{
	log_handle(msg,msg.get_code());
}


void Bot::log_handle(const Msg &msg,
                     const uint32_t &code,
                     const std::string &remarks)
const
{
	std::cout << std::setw(15) << std::setfill(' ') << std::left << "";
	std::cout << msg;
	std::cout << " " << remarks;
	std::cout << std::endl;
}


void Bot::log_handle(const Msg &msg,
                     const std::string &name,
                     const std::string &remarks)
const
{
	std::cout << std::setw(15) << std::setfill(' ') << std::left << name;
	std::cout << msg;
	std::cout << " " << remarks;
	std::cout << std::endl;
}


std::ostream &operator<<(std::ostream &s,
                         const Bot &b)
{
	s << "Session: " << std::endl;
	s << b.sess << std::endl;
	s << b.chans << std::endl;
	s << b.users << std::endl;
	return s;
}
