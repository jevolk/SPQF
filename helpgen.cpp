/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include <string>
#include <sstream>
#include <iostream>

#include "help.h"


int main(void)
{
	std::stringstream ss;
	ss << help_vote << std::endl << std::endl << std::endl;

	ss << "!vote help config:" << std::endl << std::endl;
	ss << help_vote_config << std::endl << std::endl << std::endl;

	ss << "!vote help kick:" << std::endl << std::endl;
	ss << help_vote_kick << std::endl << std::endl << std::endl;

	ss << "!vote help mode:" << std::endl << std::endl;
	ss << help_vote_mode << std::endl << std::endl << std::endl;

	std::cout << ss.str() << std::endl;
}
