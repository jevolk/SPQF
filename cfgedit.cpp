/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


// libircbot irc::bot::
#include "ircbot/bot.h"
using namespace irc::bot;


static
void wait_anykey()
{
	std::cout << "Press any key to continue or ctrl-c to quit..." << std::endl;
	std::cin.get();
}


int main(int argc, char **argv) try
{
	Opts opts;
	const int nargs(opts.parse({argv+1,argv+argc}));

	if(argc - nargs < 2)
	{
		std::cerr << "Usage: " << argv[0] << " [--dbdir=db --db=ircbot] <dbkey | *> [key] [= [val...]]\n"
		          << "\t- dbkey is the channel or account name, i.e \"#SPQF\" or foobar.\n"
		          << "\t- key is the fully qualified JSON key, i.e config.vote.duration\n"
		          << "\t- Omitting value after = is a deletion of this key.\n"
		          << "\t- Omitting key prints the whole document.\n"
		          << "\t- Using \"*\" as dbkey prints the whole database.\n"
		          << "\t- Multiple vals become a JSON array.\n"
		          << "\t- Multiple vals in quotes become a single string.\n";
		return -1;
	}

	std::cout << "Using configuration: " << std::endl << opts << std::endl << std::endl;

	const std::string db      { opts.count("db")? opts["db"] : "ircbot"  };
	const std::string dockey  { tolower(argv[nargs+1])                   };
	const std::string key     { argc > nargs+2? argv[nargs+2] : ""       };
	const std::string eq      { argc > nargs+3? argv[nargs+3] : ""       };

	const std::vector<std::string> vals
	{
		argv + (argc > nargs + 4? nargs + 4 : 0),
		argv + (argc > nargs + 4? nargs + argc : 0)
	};

	std::cout << "db    [" << db       << "]" << std::endl;
	std::cout << "doc   [" << dockey   << "]" << std::endl;
	std::cout << "key   [" << key      << "]" << std::endl;

	for(const auto &val : vals)
		std::cout << "val    [" << val << "]" << std::endl;

	if(!eq.empty() && eq != "=")
		throw Exception("Invalid syntax (missing equal sign)");

	// Open database
	Adb adb(opts["dbdir"] + "/" + db);
	irc::bot::adb = &adb;

	// Read Iteration
	if(dockey == "*")
	{
		std::for_each(adb.begin(),adb.end(),[]
		(const auto &kv)
		{
			std::cout << "[" << kv.first << "] => " << std::endl;
			std::cout << kv.second << std::endl << std::endl;
		});

		return 0;
	}

	// Creation warning
	if(!adb.exists(dockey))
	{
		std::cout << "Document not found in database." << std::endl;
		if(key.empty())
			return -1;

		std::cout << "It will be created." << std::endl;
		wait_anykey();
	}

	// Deletion warning
	if(!eq.empty() && vals.empty())
	{
		std::cout << "DELETING THE KEY [" << key << "] INSIDE [" << dockey << "]" << std::endl;
		wait_anykey();
	}

	Acct acct(&dockey);

	// Read whole document
	if(key.empty() && eq.empty() && vals.empty())
	{
		const Adoc doc(acct.get());
		std::cout << doc << std::endl;
		return 0;
	}

	// Read document by key
	if(eq.empty() && vals.empty())
	{
		const Adoc doc(acct.get(key));
		std::cout << doc << std::endl;
		return 0;
	}

	// Delete document
	if(vals.empty())
	{
		Adoc doc(acct.get());
		if(!doc.remove(key))
			throw Exception("Failed to remove key: not found");

		acct.set(doc);
		std::cout << "Removed." << std::endl;
		return 0;
	}

	// Overwrite whole document (disabled)
	if(key.empty())
		throw Exception("Specify a key");

	// Set key to value
	if(vals.size() == 1)
	{
		acct.set_val(key,vals.at(0));
		std::cout << "Done." << std::endl;
		return 0;
	}

	// Set key to array
	acct.set_val(key,vals.begin(),vals.end());
	return 0;
}
catch(const Exception &e)
{
	std::cerr << "Error: " << e << std::endl;
	return -1;
}
