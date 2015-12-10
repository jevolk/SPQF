/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


// libircbot irc::bot::
#include "ircbot/bot.h"
using namespace irc::bot;


int main(int argc, char **argv) try
{
	Opts opts;
	const int nargs = opts.parse({argv+1,argv+argc});

	if(argc - nargs < 2)
	{
		printf("Usage: %s [--dbdir=db --db=ircbot] <dbkey | *> [key] [= [val]]\n",argv[0]);
		printf("\t- dbkey is the channel or account name, i.e \"#SPQF\" or foobar\n");
		printf("\t- key is the fully qualified JSON key, i.e config.vote.duration\n");
		printf("\t- omitting value after = is a deletion of this key.\n");
		printf("\t- omitting key prints the whole document.\n");
		printf("\t- using \"*\" as dbkey prints the whole database.\n");
		return -1;
	}

	std::cout << "NOTE : Using configuration: " << std::endl;
	std::cout << opts << std::endl;
	std::cout << std::endl;

	const std::string dockey = tolower(argv[nargs+1]);
	const std::string key = argc > nargs+2? argv[nargs+2] : "";
	const std::string eq = argc > nargs+3? argv[nargs+3] : "";
	const std::string val = argc > nargs+4? argv[nargs+4] : "";

	printf("doc[%s] key[%s] val[%s]\n",dockey.c_str(),key.c_str(),val.c_str());

	if(!eq.empty() && eq != "=")
	{
		printf("Invalid syntax (missing equal sign)\n");
		return -1;
	}

	if(!eq.empty() && val.empty())
	{
		printf("DELETING THE KEY [%s] INSIDE [%s]\n",key.c_str(),dockey.c_str());
		printf("Press any key to continue or ctrl-c to quit...\n");
		std::cin.get();
	}

	const auto db = opts.count("db")? opts["db"] : "ircbot";
	Adb adb(opts["dbdir"] + "/" + db);

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

	if(!adb.exists(dockey))
	{
		printf("Document not found in database.\n");
		if(key.empty())
			return -1;

		printf("It will be created.\n");
		printf("Press any key to continue or ctrl-c to quit...\n");
		std::cin.get();
	}

	Acct acct(adb,&dockey);

	if(key.empty() && eq.empty() && val.empty())
	{
		Adoc doc = acct.get();
		std::cout << doc << std::endl;
		return 0;
	}

	if(eq.empty() && val.empty())
	{
		Adoc doc = acct.get();
		std::cout << key << " => [" << doc[key] << "]" << std::endl;
		return 0;
	}

	if(val.empty())
	{
		Adoc doc = acct.get();
		if(!doc.remove(key))
			throw Exception("Failed to remove key: not found");

		acct.set(doc);
		printf("Removed.\n");
		return 0;
	}

	if(key.empty())
	{
		printf("OVERWRITNG THE DOCUMENT [%s]\n",dockey.c_str());
		printf("Press any key to continue or ctrl-c to quit...\n");
		std::cin.get();

		acct.set(Adoc(val));
		printf("Done.\n");
		return 0;
	}

	acct.set_val(key,val);
	printf("Done.\n");
	return 0;
}
catch(const Exception &e)
{
	std::cerr << "Exception: " << e << std::endl;
	return -1;
}
