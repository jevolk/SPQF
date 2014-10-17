/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


namespace colors
{
	enum Mode
	{
		OFF           = 0x0f,
		BOLD          = 0x02,
		COLOR         = 0x03,
		ITALIC        = 0x09,
		STRIKE        = 0x13,
		UNDER         = 0x15,
		UNDER2        = 0x1f,
		REVERSE       = 0x16,
	};

	enum class FG
	{
		WHITE,    BLACK,      BLUE,      GREEN,
		LRED,     RED,        MAGENTA,   ORANGE,
		YELLOW,   LGREEN,     CYAN,      LCYAN,
		LBLUE,    LMAGENTA,   GRAY,      LGRAY
	};

	enum class BG
	{
		LGRAY_BLINK,     BLACK,           BLUE,          GREEN,
		RED,             RED_BLINK,       MAGENTA,       ORANGE,
		ORANGE_BLINK,    GREEN_BLINK,     CYAN,          CYAN_BLINK,
		BLUE_BLINK,      MAGENTA_BLINK,   BLACK_BLINK,   LGRAY,
	};
}


class Locutor
{
  public:
	enum Method
	{
		NOTICE,
		PRIVMSG,
		ACTION,
	};

	static constexpr struct flush_t {} flush {};        // Stream is flushed (sent) to channel
	static const Method DEFAULT_METHOD = NOTICE;

  private:
	Sess *sess;
	std::string target;                                 // Target entity name
	std::ostringstream sendq;                           // Stream buffer for stream operators
	Method meth;                                        // Stream state for current method
	colors::FG fg;                                      // Stream state for foreground color

  public:
	auto &get_sess() const                              { return *sess;                              }
	auto &get_target() const                            { return target;                             }
	auto &get_sendq() const                             { return sendq;                              }

  protected:
	auto &get_sess()                                    { return *sess;                              }
	auto &get_sendq()                                   { return sendq;                              }
	void set_target(const std::string &target)          { this->target = target;                     }
	void clear_sendq();

	// [SEND] Raw interface                             // Should attempt to specify in a subclasses

  public:
	// [SEND] Controls / Utils
	void mode(const std::string &mode);                 // Raw mode command
	void whois();                                       // Sends whois query
	void mode();                                        // Sends mode query

	// [SEND] string interface
	void notice(const std::string &msg);
	void msg(const std::string &msg);
	void me(const std::string &msg);

	// [SEND] stream interface                          // Defaults back to DEFAULT_METHOD after flush
	virtual Locutor &operator<<(const flush_t f);       // Flush stream to endpoint
	Locutor &operator<<(const Method &method);          // Set method for this message
	Locutor &operator<<(const colors::FG &fg);          // Insert foreground color
	Locutor &operator<<(const colors::BG &fg);          // Insert background color
	Locutor &operator<<(const colors::Mode &mode);      // Color controls
	template<class T> Locutor &operator<<(const T &t);  // Append data to sendq stream

	Locutor(Sess &sess, const std::string &target);
	Locutor(Locutor &&other);                           // Must be defined due to bug in gnu libstdc++ for now
	Locutor &operator=(Locutor &&other) &;              // ^
	virtual ~Locutor() = default;
};


inline
Locutor::Locutor(Sess &sess,
                 const std::string &target):
sess(&sess),
target(target),
meth(DEFAULT_METHOD),
fg(colors::FG::BLACK)
{

}


inline
Locutor::Locutor(Locutor &&o):
sess(std::move(o.sess)),
target(std::move(o.target)),
sendq(o.sendq.str()),                                   // GNU libstdc++ oversight requires this
meth(std::move(o.meth)),
fg(std::move(o.fg))
{

}


inline
Locutor &Locutor::operator=(Locutor &&o) &
{
	sess = std::move(o.sess);
	target = std::move(o.target);
	sendq.str(o.sendq.str());                           // GNU libstdc++ oversight requires this
	meth = std::move(o.meth);
	fg = std::move(o.fg);
	return *this;
}


template<class T>
Locutor &Locutor::operator<<(const T &t)
{
	sendq << t;
	return *this;
}


inline
Locutor &Locutor::operator<<(const colors::FG &fg)
{
	sendq << "\x03" << std::setfill('0') << std::setw(2) << int(fg);
	this->fg = fg;
	return *this;
}


inline
Locutor &Locutor::operator<<(const colors::BG &bg)
{
	sendq << "\x03" << std::setfill('0') << std::setw(2) << int(fg);
	sendq << "," << std::setfill('0') << std::setw(2) << int(bg);
	return *this;
}


inline
Locutor &Locutor::operator<<(const colors::Mode &mode)
{
	sendq << (unsigned char)mode;
	return *this;
}


inline
Locutor &Locutor::operator<<(const Method &meth)
{
	this->meth = meth;
	return *this;
}


inline
Locutor &Locutor::operator<<(const flush_t f)
{
	const scope reset_stream([&]
	{
		clear_sendq();
		meth = DEFAULT_METHOD;
	});

	switch(meth)
	{
		case ACTION:    me(sendq.str());         break;
		case NOTICE:    notice(sendq.str());     break;
		case PRIVMSG:
		default:        msg(sendq.str());        break;
	}

	return *this;
}


inline
void Locutor::me(const std::string &text)
{
	Sess &sess = get_sess();
	for(const std::string &token : tokens(text,"\n"))
		sess.call(irc_cmd_me,get_target().c_str(),token.c_str());
}


inline
void Locutor::msg(const std::string &text)
{
	Sess &sess = get_sess();
	for(const std::string &token : tokens(text,"\n"))
		sess.call(irc_cmd_msg,get_target().c_str(),token.c_str());
}


inline
void Locutor::notice(const std::string &text)
{
	Sess &sess = get_sess();
	for(const std::string &token : tokens(text,"\n"))
		sess.call(irc_cmd_notice,get_target().c_str(),token.c_str());
}


inline
void Locutor::mode()
{
	//NOTE: libircclient irc_cmd_channel_mode is just: %s %s
	Sess &sess = get_sess();
	sess.call(irc_cmd_channel_mode,get_target().c_str(),nullptr);
}


inline
void Locutor::whois()
{
	Sess &sess = get_sess();
	sess.call(irc_cmd_whois,get_target().c_str());
}


inline
void Locutor::mode(const std::string &str)
{
	//NOTE: libircclient irc_cmd_channel_mode is just: %s %s
	Sess &sess = get_sess();
	sess.call(irc_cmd_channel_mode,get_target().c_str(),str.c_str());
}


inline
void Locutor::clear_sendq()
{
	sendq.clear();
	sendq.str(std::string());
}
