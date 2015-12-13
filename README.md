
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


### Vote Types
* **Kick** - Kick users from the channel.
* **Ban** - Ban/Unban users from the channel.
* **Quiet** - Quiet/Unquiet users in the channel.
* **Voice** - Voice/Devoice users in the channel.
* **Op** - Op/Deop users in the channel.
* **Mode** - Direct mode interface to the channel.
* **Invite** - Invite users to the channel.
* **Exempt** - Add/Remove users from the channel +e list.
* **Topic** - Change the topic of the channel.
* **Flags** - Direct flags interface to ChanServ.
* **Opine** - Opinion polls that have no effects.

#### Additional types
* **Config** - Vote to change the voting configuration itself.
* **Import** - Vote to copy the voting configuration of another channel.
* **Civis** - Configurable vote to enfranchise new users.
* **Censure** - Configurable vote to disenfranchise users.
* **Staff** - Configurable vote to add/remove flags for operators.
* **Appeal** - Appeal an op's extrademocratic ban/quiet/mode, etc.


*(Any type can be disabled in the configuration)*


----


### Ruleset Features


* **General**
	* The duration of a voting motion.
	* The duration of effects passed by a vote.
	* The duration each yea or nay vote adds or subtracts from the effects, respectively.
	* Effects are reserved asynchronously when they expire.
	* All outputs to public channel and/or users in private configurable.


* **Limits**
    * Maximum number of votes at any given time.
    * Maximum number of votes per account.
    * Minimum time an issue can be raised again after being voted down.
    * Minimum time an issue can be raised again after failing without a quorum.


* **Quorum**
    * Minimum required ballot submissions for a valid vote.
    * Minimum yes ballot submissions for a valid vote.
    * The percentage that constitutes a passing majority.
    * Turnout percentage of your channel that must submit a ballot for a valid vote.
    * Ability for a vote to end early if quorum requirements met.


* **Enfranchisement**
    * The minimum amount of time a user has to spend in a channel to begin voting at all.
    * The minimum number of lines a user has to say before being able to vote at all.
	* Granting voting power to those only with a current channel mode (+o/+v).
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
* Active vote listing detailing open issues for a channel.
* Searchable database-records of all votes, ballots and outcomes.
* Expiration management to undo effects of votes after some time.
* Increment/Decrement effects-time based on number of yea and nay votes.
* Vote-time options to set any config variable for a single vote.
* Automatic appeals to unilateral operator actions.
* Background jobs to nominate new eligible voters.
* Prevention of other operators kick/banning or removing the bot.
* Manual configuration override for channel +F or bot --owner.


#### Upcoming Features

* SSL support.
* Internationalization / Localization / Multi-Language support.
* Additional statistical analyses for channels.

----


## Installation


#### Requirements

* GNU C++ Compiler **4.9**+ &nbsp; *(last tested: 5.2.1-16)*
	* GNU libstdc++ **4.9**+
	* GCC Locales **4.9**+
* Boost &nbsp; *(last tested: 1.58)*
	* ASIO
    * Tokenizer
    * Lexical Cast
    * Property Tree, JSON Parser
* LevelDB &nbsp; *(tested: 1.17)*
* STLdb adapter (submodule)
	* Note: You must use the version pinned by the submodule. **Do not git-pull latest**.


#### Compilation

	git fetch --tags
	git submodule update --init --recursive
	git submodule foreach --recursive git fetch --tags
	make

*(If linking error occurs, type make again)*
*(Fetching tags is necessary for a complete version string)*


#### Execution

	./spqf

A usage message, and a list of command line options and their default arguments will print to console.

	./spqf --host=chat.freenode.net --nick=DemoBot --invite=true --join="##democracy"

Connects to Freenode with the nick *DemoBot*, joining *##democracy*, and allowing */invite* to other
channels.


###### Important Command Line Arguments
* **--nick** &nbsp; (*required*) &nbsp; Nickname for the bot to use.
* **--host** &nbsp; Hostname of the IRC server.
* **--join** &nbsp; Channels to join on connect *(multiple --join allowed)*.
* **--ns-acct** &nbsp; NickServ account name to identify with.
* **--ns-pass** &nbsp; NickServ account password to identify with.
* **--owner** &nbsp; NickServ account of the user who can type */me proves* in #freenode for bot cloaks. Also has configuration maintenance access the same as founder-override.
* **--prefix** &nbsp; Prefix for commands to not conflict with other bots.
* **--invite** &nbsp; Allows the bot to be invited by request.
* **--dbdir** &nbsp; The directory of the primary LevelDB database. *RESOURCE LOCK* error will result if two bots share this.
* **--logdir** &nbsp; The directory where logfiles for channels will be stored. *Multiple bots in the same channel (by name) using the same logdir will cause double-appends to the same log file, skewing statistical analyses.*
