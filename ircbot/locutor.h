/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Locutor : public Stream
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

	static const Method DEFAULT_METHOD = NOTICE;
	static const MethodEx DEFAULT_METHODEX = NONE;

  private:
	Sess *sess;
	Method meth;                                        // Stream state for current method
	MethodEx methex;                                    // Stream state for extension to method
	colors::FG fg;                                      // Stream state for foreground color
	std::string target;

  public:
	auto &get_sess() const                              { return *sess;                              }
	auto &get_opts() const                              { return get_sess().get_opts();              }
	auto &get_meth() const                              { return meth;                               }
	auto &get_methex() const                            { return methex;                             }
	auto &get_target() const                            { return target;                             }

  protected:
	auto &get_sess()                                    { return *sess;                              }
	void privmsg();
	void notice();
	void me();

  public:
	void set_target(const std::string &target)          { this->target = target;                     }

	// [SEND] stream interface                          // Defaults back to DEFAULT_METHOD after flush
	Locutor &operator<<(const flush_t) override;
	Locutor &operator<<(const Method &method);          // Set method for this message
	Locutor &operator<<(const MethodEx &methodex);      // Set method extension for this message
	Locutor &operator<<(const colors::Mode &mode);      // Color controls
	Locutor &operator<<(const colors::FG &fg);          // Insert foreground color
	Locutor &operator<<(const colors::BG &fg);          // Insert background color
	template<class T> Locutor &operator<<(const T &t)   { Stream::operator<<<T>(t);   return *this;  }

	// [SEND] Controls / Utils
	void mode(const std::string &mode);                 // Raw mode command
	void whois();                                       // Sends whois query
	void mode();                                        // Sends mode query

	Locutor(Sess &sess, const std::string &target);
	virtual ~Locutor() = default;
};


inline
Locutor::Locutor(Sess &sess,
                 const std::string &target):
sess(&sess),
meth(DEFAULT_METHOD),
methex(DEFAULT_METHODEX),
fg(colors::FG::BLACK),
target(target)
{

}


inline
Locutor &Locutor::operator<<(const colors::FG &fg)
{
	auto &out = *this;
	out << "\x03" << std::setfill('0') << std::setw(2) << int(fg);
	this->fg = fg;
	return *this;
}


inline
Locutor &Locutor::operator<<(const colors::BG &bg)
{
	auto &out = *this;
	out << "\x03" << std::setfill('0') << std::setw(2) << int(fg);
	out << "," << std::setfill('0') << std::setw(2) << int(bg);
	return *this;
}


inline
Locutor &Locutor::operator<<(const colors::Mode &mode)
{
	auto &out = *this;
	out << (unsigned char)mode;
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
Locutor &Locutor::operator<<(const flush_t)
{
	const scope reset([&]
	{
		clear();
		meth = DEFAULT_METHOD;
		methex = DEFAULT_METHODEX;
	});

	switch(meth)
	{
		case ACTION:     me();             break;
		case NOTICE:     notice();         break;
		case PRIVMSG:    privmsg();        break;
		default:
			throw Exception("Unsupported locution method");
	}

	return *this;
}


inline
void Locutor::me()
{
	Quote out(get_sess());
	for(const auto &token : tokens(packetize(get_str()),"\n"))
		out << "ACTION " << get_target() << " :" << token << flush;
}


inline
void Locutor::notice()
{
	Quote out(get_sess());
	const auto toks = tokens(packetize(get_str()),"\n");

	switch(methex)
	{
		case CMSG:
		{
			const auto &chan = toks.at(0);
			for(auto it = toks.begin()+1; it != toks.end(); ++it)
				out << "CNOTICE"
				    << " "  << get_target()
				    << " "  << chan
				    << " :" << *it
				    << flush;
			break;
		}

		case WALLCHOPS:
		case WALLVOICE:
		{
			const auto prefix = methex == WALLCHOPS? '@' : '+';
			for(const auto &token : toks)
				out << "NOTICE"
				    << " "  << prefix << get_target()
				    << " :" << token
				    << flush;
			break;
		}

		case NONE:
		default:
		{
			for(const auto &token : toks)
				out << "NOTICE"
				    << " "  << get_target()
				    << " :" << token
				    << flush;
			break;
		}
	}
}


inline
void Locutor::privmsg()
{
	Quote out(get_sess());
	const auto toks = tokens(packetize(get_str()),"\n");

	switch(methex)
	{
		case CMSG:
		{
			const auto &chan = toks.at(0);
			for(auto it = toks.begin()+1; it != toks.end(); ++it)
				out << "CPRIVMSG"
				    << " "  << get_target()
				    << " "  << chan
				    << " :" << *it
				    << flush;
			break;
		}

		case WALLCHOPS:
		case WALLVOICE:
		{
			const auto prefix = methex == WALLCHOPS? '@' : '+';
			for(const auto &token : toks)
				out << "PRIVMSG"
				    << " "  << prefix << get_target()
				    << " :" << token
				    << flush;
			break;
		}

		case NONE:
		default:
		{
			for(const auto &token : toks)
				out << "PRIVMSG"
				    << " "  << get_target()
				    << " :" << token
				    << flush;
			break;
		}
	}
}


inline
void Locutor::mode()
{
	Quote out(get_sess(),"MODE");
	out << get_target() << flush;
}


inline
void Locutor::whois()
{
	Quote out(get_sess(),"WHOIS");
	out << get_target() << flush;
}


inline
void Locutor::mode(const std::string &str)
{
	Quote out(get_sess(),"MODE");
	out << get_target() << " " << str << flush;
}
