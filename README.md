
# SENATVS POPVLVS QVE FREENODVS
###### The Senate & The People of Freenode - Democratic Channel Management System

**SPQF** is a robust democratic channel management system with the goal of 
replacing the need for channel operators to be present, or even exist. 
The primary design parameter is the resistance to sybil from botnets and 
spam, in addition to various other noise and abuse seen regularly on IRC.
A versatile configuration can be customized to statistically analyze a channel and
its participants, ensuring any action by the bot reflects the true and fair
intent of the channel's mission and community.

Join **irc.freenode.net/#SPQF** for feedback, suggestions and help.
We are open to feature requests, but discourage features that undermine democratic
ideals beyond those available already (Don't request features that make your vote worth more
than others, etc). We actively seek reports of how attempts are made to abuse the bot
or conduct takeovers.


----


### Vote Modules
* **Kick** - Kick users from the channel.
* **Quiet** - Quiet users in the channel.
* **Ban** - Ban users from the channel.
* **Invite** - Invite users to the channel.
* **Topic** - Change the topic of the channel.
* **Mode** - Direct mode interface to the channel.
* **Opine** - Opinion polls that have no effects.
* **Config** - Vote to change the voting configuration itself.


*(Any module can be disabled in the configuration)*


----


### Ruleset Features
* **Basic voting**
    * Maximum number of votes at any given time.
    * Maximum number of votes per account.
    * Minimum required ballot submissions for a valid vote.
    * Minimum yes ballot submissions for a valid vote.
    * The percentage that constitutes a passing majority.
    * Turnout percentage of your channel that must submit a ballot for a valid vote.
    * The time limit for votes.


* **Enfranchisement**
    * The minimum amount of time a user has to spend in a channel to begin voting at all.
    * The minimum number of lines a user has to say before being able to vote at all.
    * Reads ChanServ access flags potentially allowing only those with certain flags to vote.


* **Qualification**
    * The minimum amount of time a user must be actively chatting to vote in this round.
    * The minimum number of lines that constitutes activity to vote in this round.
    * Reads ChanServ access flags allowing those with certain flags to always be able to vote.


* **Veto Powers**
    * Reads ChanServ access flags allowing those with certain flags to have veto powers.
    * Gaining veto powers by toggling your channel mode (+o/+v) and then voting no.
    * Minimum number of vetoes required for an effective veto.
    * Immediate ending after a veto, or continue to take the roll until the time runs out.


* **Speaker Powers**
    * Reads ChanServ access flags allowing only those with certain flags to create votes.
    * Granting vote creation power by toggling a channel mode (+o/+v).


*see: help/config for detailed information.*


### Other Features


* Only recognizes users logged into NickServ.
* Ability to cast secret ballots via private message.
* Ability to /invite the bot into your channel.
* Prevention of votes that affect the bot itself.
* Offline configuration/database editing tool.
* Automatic NickServ ghost and regain on (re)connect.
* Vote modules can have independent configurations.
* Database-records of votes, ballots and outcomes.


#### Upcoming Features

* SSL support.
* Internationalization / Localization / Multi-Language support. 
* Additional statistical analyses for channels.

----


## Installation


#### Requirements

* GNU C++ Compiler **4.9** &nbsp; *(tested: 4.9.1)*
* GNU libstdc++ **4.9** 
* GCC Locales **4.9**
* Boost &nbsp; *(tested: 1.54)*
    * Tokenizer
    * Lexical Cast
    * Property Tree, JSON Parser
* LevelDB &nbsp; *(tested: 1.17)*
* libircclient **1.8** &nbsp; *(tested: 1.8)*
* STLdb adapter (submodule)


#### Compilation

> git submodule update --init stldb/
> make

*(If linking error occurs, type make again)*


#### Execution

> ./spqf

A usage message, and a list of command line options and their default arguments will print to console.

> ./spqf --host=chat.freenode.net --nick=DemoBot --invite=true --join="##democracy"

Connects to Freenode with the nick *DemoBot*, joining *##democracy*, and allowing */invite* to other
channels.


###### Important Command Line Arguments
* **--nick** &nbsp; (*required*) &nbsp; Nickname for the bot to use.
* **--host** &nbsp; Hostname of the IRC server.
* **--join** &nbsp; Channels to join on connect *(multiple --join allowed)*.
* **--ns-acct** &nbsp; NickServ account name to identify with.
* **--ns-pass** &nbsp; NickServ account password to identify with.
* **--owner** &nbsp; NickServ account of the user who can type */me proves* in #freenode for bot cloaks. 
* **--prefix** &nbsp; Prefix for commands to not conflict with other bots.
* **--invite** &nbsp; Allows the bot to be invited by request.
* **--dbdir** &nbsp; The directory of the primary LevelDB database. *RESOURCE LOCK* error will result if two bots share this.
* **--logdir** &nbsp; The directory where logfiles for channels will be stored. *Multiple bots in the same channel (by name) using the same logdir will cause double-appends to the same log file, skewing statistical analyses.*
