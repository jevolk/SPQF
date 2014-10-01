/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


static thread_local int locution_type_idx;

class Locutor
{
  protected:
	Sess &sess;

  private:
	std::string target;                                     // Target entity name
	std::ostringstream sendq;                               // State for stream operators

  public:
	static constexpr struct flush_t {} endl {};             // Stream is flushed (sent) to channel

	enum Type
	{
		ME,
		MSG,
		NOTICE
	};

	const Sess &get_sess() const                            { return sess;                       }
	const std::string &get_target() const                   { return target;                     }
	const std::ostringstream &get_sendq() const             { return sendq;                      }

  protected:
	Sess &get_sess()                                        { return sess;                       }
	std::ostringstream &get_sendq()                         { return sendq;                      }
	void set_target(const std::string &target)              { this->target = target;             }

	// [SEND] Raw interface for subclasses
	void mode(const std::string &mode);                     // Raw mode command

  public:
	// [SEND] Controls / Utils
	void whois();                                           // Sends whois query
	void mode();                                            // Sends mode query

	// [SEND] string interface
	void notice(const std::string &msg);
	void msg(const std::string &msg);
	void me(const std::string &msg);

	// [SEND] stream interface
	Locutor &operator<<(const flush_t f);                   // Flush stream to channel
	Locutor &operator<<(const Type &type);                  // Set Locution type
	template<class T> Locutor &operator<<(const T &t);      // Append to sendq stream

	Locutor(Sess &sess, const std::string &target);
};


inline
Locutor::Locutor(Sess &sess,
                 const std::string &target):
sess(sess),
target(target)
{
	locution_type_idx = std::ios_base::xalloc();

}


template<class T>
Locutor &Locutor::operator<<(const T &t)
{
	sendq << t;
	return *this;
}


inline
Locutor &Locutor::operator<<(const Type &type)
{
	sendq.iword(locution_type_idx) = type;
	return *this;
}


inline
Locutor &Locutor::operator<<(const flush_t f)
{
	switch(Type(sendq.iword(locution_type_idx)))
	{
		case ME:        me(sendq.str());         break;
		case NOTICE:    notice(sendq.str());     break;
		case MSG:
		default:        msg(sendq.str());        break;
	}

	sendq.str(std::string());
	return *this;
}


inline
void Locutor::me(const std::string &msg)
{
	sess.call(irc_cmd_me,get_target().c_str(),msg.c_str());
}


inline
void Locutor::msg(const std::string &text)
{
	sess.call(irc_cmd_msg,get_target().c_str(),text.c_str());
}


inline
void Locutor::notice(const std::string &text)
{
	sess.call(irc_cmd_notice,get_target().c_str(),text.c_str());
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
