/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


// libircbot irc::bot::
#include "ircbot/bot.h"
using namespace irc::bot;

// SPQF
#include "log.h"
#include "vote.h"
#include "votes.h"
#include "vdb.h"


Vdb::Vdb(const std::string &dir):
Adb(dir)
{
}


const std::vector<std::string> Vdb::operators
{{
	"",     // empty operator checks if key exists (and is non-empty)
	"=",    // equality operator aliased to ==
	"==",   // case insensitive string equality
	"!=",   // case insensitive string inequality
	"<",    // integer less than
	">",    // integer greater than
	"<=",   // integer less than or equals
	">=",   // integer greater than or equals
}};


Vdb::Results Vdb::query(const Terms &terms,
                        const size_t &limit,
                        const bool &descending)
{
	Results ret;

	// TODO: Until stldb reverse iterators and propert sorting are fixed hack this for now...
	query(terms,0,cbegin(),cend(),ret);
	ret.sort();

	if(descending)
		ret.reverse();

	if(limit)
		ret.resize(std::min(limit,ret.size()));

	return ret;
}


bool Vdb::match(const Adoc &doc,
                const Term &term)
try
{
	const auto &key(std::get<0>(term));
	const auto &op(std::get<1>(term));
	const auto &val(std::get<2>(term));

	switch(hash(op))
	{
		case hash("="):
		case hash("=="):  return boost::iequals(doc[key],val);
		case hash("!="):  return !boost::iequals(doc[key],val);
		case hash("<="):  return doc.get<int64_t>(key) <= lex_cast<int64_t>(val);
		case hash(">="):  return doc.get<int64_t>(key) >= lex_cast<int64_t>(val);
		case hash("<"):   return doc.get<int64_t>(key) < lex_cast<int64_t>(val);
		case hash(">"):   return doc.get<int64_t>(key) > lex_cast<int64_t>(val);
		case hash(""):    return doc.has(key);
		default:          throw Assertive("Unrecognized query term operator");
	};
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("This comparison operator requires a number.");
}
catch(const boost::property_tree::ptree_bad_path &e)
{
	throw Exception("A key is not recognized: ") << e.what();
}
catch(const boost::property_tree::ptree_bad_data &e)
{
	throw Exception("A key can not be compared to the argument: ") << e.what();
}
catch(const boost::property_tree::ptree_error &e)
{
	throw Assertive(e.what());
}


std::string Vdb::get_type(const id_t &id)
{
	return get_value(id,"type");
}


std::string Vdb::get_value(const id_t &id,
                           const std::string &key)
{
	return Adb::get(std::nothrow,lex_cast(id))[key];
}


std::unique_ptr<Vote> Vdb::get(const id_t &id)
try
{
	switch(hash(get_type(id)))
	{
		case hash("config"):    return std::make_unique<vote::Config>(id,*this);
		case hash("mode"):      return std::make_unique<vote::Mode>(id,*this);
		case hash("appeal"):    return std::make_unique<vote::Appeal>(id,*this);
		case hash("trial"):     return std::make_unique<vote::Trial>(id,*this);
		case hash("kick"):      return std::make_unique<vote::Kick>(id,*this);
		case hash("invite"):    return std::make_unique<vote::Invite>(id,*this);
		case hash("topic"):     return std::make_unique<vote::Topic>(id,*this);
		case hash("opine"):     return std::make_unique<vote::Opine>(id,*this);
		case hash("quote"):     return std::make_unique<vote::Quote>(id,*this);
		case hash("exempt"):    return std::make_unique<vote::Exempt>(id,*this);
		case hash("unexempt"):  return std::make_unique<vote::UnExempt>(id,*this);
		case hash("op"):        return std::make_unique<vote::Op>(id,*this);
		case hash("deop"):      return std::make_unique<vote::DeOp>(id,*this);
		case hash("ban"):       return std::make_unique<vote::Ban>(id,*this);
		case hash("unban"):     return std::make_unique<vote::UnBan>(id,*this);
		case hash("quiet"):     return std::make_unique<vote::Quiet>(id,*this);
		case hash("unquiet"):   return std::make_unique<vote::UnQuiet>(id,*this);
		case hash("voice"):     return std::make_unique<vote::Voice>(id,*this);
		case hash("devoice"):   return std::make_unique<vote::DeVoice>(id,*this);
		case hash("flags"):     return std::make_unique<vote::Flags>(id,*this);
		case hash("civis"):     return std::make_unique<vote::Civis>(id,*this);
		case hash("censure"):   return std::make_unique<vote::Censure>(id,*this);
		case hash("import"):    return std::make_unique<vote::Import>(id,*this);
		default:                return std::make_unique<Vote>("",id,*this);
	}
}
catch(const Exception &e)
{
	if(exists(id))
		throw;

	throw Exception("Could not find a vote by that ID.");
}


bool Vdb::exists(const id_t &id)
const
{
	return Adb::exists(lex_cast(id));
}
