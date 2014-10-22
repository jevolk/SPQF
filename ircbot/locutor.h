/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Locutor
{
  public:
	enum Method
	{
		NOTICE,
		PRIVMSG,
		ACTION,
	};

	enum MethodEx
	{
		NONE,
		CMSG,                                           // automatic when:  user << chan << ""
		WALLCHOPS,
		WALLVOICE,
	};

	static constexpr struct flush_t {} flush {};        // Stream is flushed (sent) to channel
	static const Method DEFAULT_METHOD = NOTICE;
	static const MethodEx DEFAULT_METHODEX = NONE;

  private:
	Sess *sess;
	std::string target;                                 // Target entity name
	std::ostringstream sendq;                           // Stream buffer for stream operators
	Method meth;                                        // Stream state for current method
	MethodEx methex;                                    // Stream state for extension to method
	colors::FG fg;                                      // Stream state for foreground color

  public:
	auto &get_sess() const                              { return *sess;                              }
	auto &get_opts() const                              { return get_sess().get_opts();              }
	auto &get_target() const                            { return target;                             }
	auto &get_sendq() const                             { return sendq;                              }
	auto &get_meth() const                              { return meth;                               }
	auto &get_methex() const                            { return methex;                             }
	auto has_sendq() const                              { return sendq.str().empty();                }

  protected:
	auto &get_sess()                                    { return *sess;                              }
	auto &get_sendq()                                   { return sendq;                              }
	void set_target(const std::string &target)          { this->target = target;                     }

  public:
	// [SEND] string interface
	void privmsg(const std::string &msg);
	void notice(const std::string &msg);
	void me(const std::string &msg);
	void clear_sendq();

	// [SEND] stream interface                          // Defaults back to DEFAULT_METHOD after flush
	virtual Locutor &operator<<(const flush_t f);       // Flush stream to endpoint
	Locutor &operator<<(const Method &method);          // Set method for this message
	Locutor &operator<<(const MethodEx &methodex);      // Set method extension for this message
	Locutor &operator<<(const colors::FG &fg);          // Insert foreground color
	Locutor &operator<<(const colors::BG &fg);          // Insert background color
	Locutor &operator<<(const colors::Mode &mode);      // Color controls
	template<class T> Locutor &operator<<(const T &t);  // Append data to sendq stream

	// [SEND] Controls / Utils
	void mode(const std::string &mode);                 // Raw mode command
	void whois();                                       // Sends whois query
	void mode();                                        // Sends mode query

	Locutor(Sess &sess, const std::string &target);
	Locutor(Locutor &&other) noexcept;                  // Must be defined due to bug in gnu libstdc++ for now
	Locutor(const Locutor &other);                      // ^
	Locutor &operator=(Locutor &&other) & noexcept;     // ^
	virtual ~Locutor() = default;
};


inline
Locutor::Locutor(Sess &sess,
                 const std::string &target):
sess(&sess),
target(target),
meth(DEFAULT_METHOD),
methex(DEFAULT_METHODEX),
fg(colors::FG::BLACK)
{
	sendq.exceptions(std::ios_base::badbit|std::ios_base::failbit);
}


inline
Locutor::Locutor(Locutor &&o)
noexcept:
sess(std::move(o.sess)),
target(std::move(o.target)),
sendq(o.sendq.str()),                                   // GNU libstdc++ oversight requires this
meth(std::move(o.meth)),
methex(std::move(o.methex)),
fg(std::move(o.fg))
{
	sendq.exceptions(std::ios_base::badbit|std::ios_base::failbit);
	sendq.setstate(o.sendq.rdstate());
	sendq.seekp(0,std::ios_base::end);
}


inline
Locutor::Locutor(const Locutor &o):
sess(o.sess),
target(o.target),
sendq(o.sendq.str()),
meth(o.meth),
methex(o.methex),
fg(o.fg)
{
	sendq.exceptions(std::ios_base::badbit|std::ios_base::failbit);
	sendq.setstate(o.sendq.rdstate());
	sendq.seekp(0,std::ios_base::end);
}


inline
Locutor &Locutor::operator=(Locutor &&o)
& noexcept
{
	sess = std::move(o.sess);
	target = std::move(o.target);
	sendq.str(o.sendq.str());
	sendq.setstate(o.sendq.rdstate());
	sendq.seekp(0,std::ios_base::end);
	meth = std::move(o.meth);
	methex = std::move(o.methex);
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
Locutor &Locutor::operator<<(const MethodEx &methex)
{
	this->methex = methex;
	return *this;
}


inline
Locutor &Locutor::operator<<(const flush_t f)
{
	const scope reset_stream([&]
	{
		clear_sendq();
		meth = DEFAULT_METHOD;
		methex = DEFAULT_METHODEX;
	});

	switch(meth)
	{
		case ACTION:     me(sendq.str());             break;
		case NOTICE:     notice(sendq.str());         break;
		case PRIVMSG:    privmsg(sendq.str());        break;
		default:
			throw Exception("Unsupported locution method");
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
void Locutor::notice(const std::string &text)
{
	Sess &sess = get_sess();
	const auto toks = tokens(text,"\n");

	switch(methex)
	{
		case CMSG:
		{
			const auto &chan = toks.at(0);
			for(auto it = toks.begin()+1; it != toks.end(); ++it)
				sess.quote("CNOTICE %s %s :%s",
				           get_target().c_str(),
				           chan.c_str(),
				           it->c_str());
			break;
		}

		case WALLCHOPS:
		case WALLVOICE:
		{
			for(const std::string &token : toks)
				sess.quote("NOTICE %c%s :%s",
				           methex == WALLCHOPS? '@' : '+',
				           get_target().c_str(),
				           token.c_str());
			break;
		}

		case NONE:
		default:
		{
			for(const std::string &token : toks)
				sess.call(irc_cmd_notice,get_target().c_str(),token.c_str());

			break;
		}
	}
}


inline
void Locutor::privmsg(const std::string &text)
{
	Sess &sess = get_sess();
	const auto toks = tokens(text,"\n");

	switch(methex)
	{
		case CMSG:
		{
			const auto &chan = toks.at(0);
			for(auto it = toks.begin()+1; it != toks.end(); ++it)
				sess.quote("CPRIVMSG %s %s :%s",
				           get_target().c_str(),
				           chan.c_str(),
				           it->c_str());
			break;
		}

		case WALLCHOPS:
		case WALLVOICE:
		{
			for(const std::string &token : toks)
				sess.quote("PRIVMSG %c%s :%s",
				           methex == WALLCHOPS? '@' : '+',
				           get_target().c_str(),
				           token.c_str());
			break;
		}

		case NONE:
		default:
		{
			for(const std::string &token : toks)
				sess.call(irc_cmd_msg,get_target().c_str(),token.c_str());

			break;
		}
	}
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
