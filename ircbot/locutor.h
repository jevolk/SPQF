/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


static thread_local int locution_meth_idx;

class Locutor
{
	Sess &sess;
	std::string target;                                 // Target entity name
	std::ostringstream sendq;                           // State for stream operators

  public:
	static constexpr struct flush_t {} flush {};        // Stream is flushed (sent) to channel

	enum Method
	{
		PRIVMSG,
		NOTICE,
		ACTION,
	};

	const Sess &get_sess() const                        { return sess;                               }
	const std::string &get_target() const               { return target;                             }
	const std::ostringstream &get_sendq() const         { return sendq;                              }

  protected:
	Sess &get_sess()                                    { return sess;                               }
	std::ostringstream &get_sendq()                     { return sendq;                              }
	void set_target(const std::string &target)          { this->target = target;                     }

  public:
	// [SEND] Raw interface                             // Should attempt to specify in a subclasses
	void mode(const std::string &mode);                 // Raw mode command

	// [SEND] Controls / Utils
	void whois();                                       // Sends whois query
	void mode();                                        // Sends mode query

	// [SEND] string interface
	void notice(const std::string &msg);
	void msg(const std::string &msg);
	void me(const std::string &msg);

	// [SEND] stream interface                          // Defaults back to MSG after every flush
	Locutor &operator<<(const flush_t f);               // Flush stream to channel
	Locutor &operator<<(const Method &method);          // Set method for this message
	template<class T> Locutor &operator<<(const T &t);  // Append data to sendq stream

	Locutor(Sess &sess, const std::string &target);
	Locutor(const Locutor &) = delete;
	Locutor &operator=(const Locutor &) = delete;
	virtual ~Locutor() = default;
};


inline
Locutor::Locutor(Sess &sess,
                 const std::string &target):
sess(sess),
target(target)
{
	locution_meth_idx = std::ios_base::xalloc();

}


template<class T>
Locutor &Locutor::operator<<(const T &t)
{
	sendq << t;
	return *this;
}


inline
Locutor &Locutor::operator<<(const flush_t f)
{
	switch(Method(sendq.iword(locution_meth_idx)))
	{
		case ACTION:    me(sendq.str());         break;
		case NOTICE:    notice(sendq.str());     break;
		case PRIVMSG:
		default:        msg(sendq.str());        break;
	}

	sendq.str(std::string());
	sendq.iword(locution_meth_idx) = PRIVMSG;    // reset stream to default
	return *this;
}


inline
Locutor &Locutor::operator<<(const Method &meth)
{
	sendq.iword(locution_meth_idx) = meth;
	return *this;
}


inline
void Locutor::me(const std::string &text)
{
	using delim = boost::char_separator<char>;
	static const delim sep("\n");

	const boost::tokenizer<delim> toks(text,sep);
	for(const std::string &token : toks)
		sess.call(irc_cmd_me,get_target().c_str(),token.c_str());
}


inline
void Locutor::msg(const std::string &text)
{
	using delim = boost::char_separator<char>;
	static const delim sep("\n");

	const boost::tokenizer<delim> toks(text,sep);
	for(const std::string &token : toks)
		sess.call(irc_cmd_msg,get_target().c_str(),token.c_str());
}


inline
void Locutor::notice(const std::string &text)
{
	using delim = boost::char_separator<char>;
	static const delim sep("\n");

	const boost::tokenizer<delim> toks(text,sep);
	for(const std::string &token : toks)
		sess.call(irc_cmd_notice,get_target().c_str(),token.c_str());
}


inline
void Locutor::mode()
{
	//NOTE: libircclient irc_cmd_channel_mode is just: %s %s
	sess.call(irc_cmd_channel_mode,get_target().c_str(),nullptr);
}


inline
void Locutor::whois()
{
	sess.call(irc_cmd_whois,get_target().c_str());
}


inline
void Locutor::mode(const std::string &str)
{
	//NOTE: libircclient irc_cmd_channel_mode is just: %s %s
	sess.call(irc_cmd_channel_mode,get_target().c_str(),str.c_str());
}
