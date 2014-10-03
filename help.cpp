/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


extern const char *const help_vote = \
"*** SENATVS POPVLVS QVE FREENODUS - The Senate and The People of Freenode ( #SPQF )\n"\
"*** Voting System:\n"\
"*** usage:    !vote   <command | issue>       : Initiate a vote for an issue or supply a command...\n"\
"*** issue:    <mode>  [target ...]            : Vote on setting modes for channel or target (+b implies kick)\n"\
"*** issue:    kick    <target>                : Vote to kick a target from the channel\n"\
"*** issue:    config  <variable = value>      : Vote on the voting configuration in this channel\n"\
"*** command:  config  [variable ...]          : Show whole configuration or one or more variable\n"\
;

extern const char *const help_vote_config = \
"** Vote on changing various configuration settings.\n"\
"** The syntax for initiating a vote is: <variable> = <new value>\n"\
"** The alternative syntax for printing information is: <variable> [variable ...]\n"\
"** Example: !vote config vote_quorum_minimum = 10  : requires at least 10 participants in any vote\n"\
;

extern const char *const help_vote_kick = \
"** Vote to kick user(s) from the channel.\n"\
"** The syntax for initiating a vote is: [target ...]\n"\
"** Example: !vote kick fred waldo             : Starts a single vote to kick both fred and waldo"\
;

extern const char *const help_vote_mode = \
"** Vote on changing modes for a channel with operator syntax.\n"\
"** The syntax for initiating a vote is: <mode string> [target ...]\n"\
"** Example: !vote +q $a:foobar     : Quiets based on the account foobar\n"\
"** Example: !vote +b <nickname>    : Kick-ban based on a nickname's $a: and/or *!*@host all in one\n"\
"** Example: !vote +r               : Set the channel for registered users only\n"\
;
