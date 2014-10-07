/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


extern const char *const help_vote = \
"*** SENATVS POPVLVS QVE FREENODUS - The Senate and The People of Freenode ( #SPQF )\n"\
"*** Voting System:\n"\
"*** usage:    !vote    <command | issue>       : Initiate a vote for an issue or supply a command...\n"\
"***           help     [topic]                 : Show this help system.\n"\
"***           poll     [#ID]                   : Show the status of a vote, either by ID or the current active motion in the channel.\n"\
"***           cancel   [#ID]                   : Cancel your vote by ID or whatever you initiated; only if it has not been voted on.\n"\
"***           config.$                         : View the configuration of this channel.\n"\
"*** issue:    config.$ <= [value]>             : Vote on the voting configuration in this channel.\n"\
"*** issue:    <mode>   [target ...]            : Vote on setting modes for channel or target exactly as if you were an operator.\n"\
"*** issue:    kick     <nickname>              : Vote to kick a victim from the channel.\n"\
"*** issue:    invite   <nickname>              : Vote to invite a user to the channel.\n"\
"*** issue:    topic    <message>               : Vote to change the topic.\n"\
;

extern const char *const help_vote_config = \
"** Vote on changing various configuration settings.\n"\
"** The syntax for printing information is: <variable>\n"\
"** The syntax for initiating a vote to set: <variable> = <new value>\n"\
"** The syntax for initiating a vote to delete: <variable> =\n"\
"** Configuration variables are addressed as config.foo.bar... Voting configuration is config.vote.*"\
"** Example: !vote config.vote.duration = 10    : Set the duration of a vote to be 10 seconds.\n"\
"** Example: !vote config.vote.duration =       : Deletes this variable and all children.\n"\
;

extern const char *const help_vote_kick = \
"** Vote to kick user(s) from the channel.\n"\
"** The syntax for initiating a vote is: [target ...]\n"\
"** Example: !vote kick fred waldo             : Starts a single vote to kick both fred and waldo (all or none)."\
;

extern const char *const help_vote_mode = \
"** Vote on changing modes for a channel with operator syntax.\n"\
"** The syntax for initiating a vote is: <mode string> [target ...]\n"\
"** Example: !vote +q $a:foobar     : Quiets based on the account foobar.\n"\
"** Example: !vote +r               : Set the channel for registered users only.\n"\
"** Example: !vote +b <nickname>    : Kick-ban based on a nickname's $a: and/or *!*@host all in one.\n"\
"** Example: !vote +b <*!*@host>    : Set ban based *!*@host mask, and kick any matches currently in the channel.\n"\
;
