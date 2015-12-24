# SENATVS POPVLVS QVE FREENODVS
###### The Senate & The People of Freenode - Democratic Channel Management System

**SPQF** is a robust **threshold-operator** replacing the need for individual operators to always agree, be present, or even exist.
The primary design parameter is the resistance to sybil, botnets, spam and various other noise and abuse seen regularly on IRC.
A versatile configuration can be customized for applications ranging from whitelisting users that may have
full operator access, to automatically analyzing an entire channel for nominating and enfranchising users.

The wide range of applications posits **SPQF** as the essential channel management solution regardless of your community's authority structure.
It facilitates agreement allowing a very large, diverse set of operators to maintain coverage
of a channel without typical friction leading to op-wars, arguments and group-think, etc. **SPQF** yields greater consistency and curbs
abuse seen by your users. It samples from a larger set of opinions, masks outliers and errors, and foremost: legitimizes decisions
by presenting consensus rather than having one ad hoc operator appearing arbitrary.


Join **irc.freenode.net/#SPQF** for feedback, suggestions and help.
Every community has different ideas for exactly what they want out of this system, and the complexity of the configuration reflects this;
do not hesitate to consult us if you're planning a deployment in your channel.
We are open to feature requests, but discourage features that undermine democratic
ideals beyond those available already (don't request features that make your vote worth more
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
* **Invex** - Add/Remove users from the channel +I list.
* **Topic** - Change the topic of the channel.
* **Flags** - Direct flags interface to ChanServ.
* **Opine** - Opinion polls that have no effects.

#### Additional types
* **Config** - Vote to change the voting configuration itself.
* **Import** - Vote to copy the voting configuration of another channel.
* **Civis** - Configurable automated vote to enfranchise new users.
* **Censure** - Configurable vote to disenfranchise users.
* **Staff** - Configurable vote to add/remove custom flags for operators.
* **Trial** - Auto vote to confirm an operator's manual action (else reverses it).
* **Appeal** - Auto vote to reverse an operator's manual action.


*(Any type can be disabled in the configuration)*



![vote list screenshot](http://i.imgur.com/xf24L54.png)
![vote info screenshot](http://i.imgur.com/NI5cxns.png)


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
    * Ability for prejudicial effects once a quorum is met, reversed if vote fails.


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
* Active vote list detailing open issues for a channel.
* Searchable database-records of all votes, ballots and outcomes.
* Expiration management to undo effects of votes after some time.
* Increment/Decrement effects-time based on number of yea and nay votes.
* Vote-time options to set any config variable for a single vote.
* Automatic appeals/trials regarding unilateral operator actions.
* Prevent an operator from manually acting after losing a related vote.
* Background jobs to nominate new eligible voters.
* Defense suite against other operators kick/banning or removing the bot.
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
* **--cloaked** &nbsp; If you have a hostmask/cloak, this option will ensure it is applied before joining channels.
* **--owner** &nbsp; NickServ account of the user who can type */me proves* in #freenode for bot cloaks. Also has configuration maintenance access.
* **--prefix** &nbsp; Prefix for commands to not conflict with other bots.
* **--invite** &nbsp; Allows the bot to be invited by request.
* **--dbdir** &nbsp; The directory of the primary LevelDB database. *RESOURCE LOCK* error will result if two bots share this.
* **--logdir** &nbsp; The directory where logfiles for channels will be stored. *Multiple bots in the same channel (by name) using the same logdir will cause double-appends to the same log file, skewing statistical analyses.*


#### Configuration

**All votes are disabled by default and must be enabled. You must read this.**
Users with flags +F or +f (or --owner) can modify the configuration without using a vote by privmsg:

	/msg <botnick> config <#channel> [path.to.doc [= [value]]]

The configuration is stored as a JSON document tree.
You don't have to worry about any actual JSON, just that keys are paths in a document tree separated by period characters;
examples below will make this clear.
When using the config command, without an equals sign, the document is fetched only.
Assigning a value after an equals sign writes to the document.
Omitting a value after the equals sign deletes a document.
The following command fetches my channel's *config* document:

	/msg SPQF config #spqf config

In fact, the configuration is the document named *config* inside your full channel document in the database.
Thus the following example command fetches the full channel document which may contain other stored data (it does not contain votes):

	/msg SPQF config #spqf

Within the *config* document the most relevant sub-document right now is *vote* and the following example will fetch that:

	/msg SPQF config #spqf config.vote

The *vote* document is made up of two levels. Items in the top level *config.vote* are applied to all vote types.
Each vote type has its own document within *config.vote*, and all items within that document are applied for that vote type only.

**The first thing you must do to begin using SPQF is the following** (swapping out your own bot and channel name):

	/msg SPQF config #spqf config.vote.opine.enable = 1

What we have done here is enabled the *opine* vote only.
Now in channel try the command:

	!vote opine test

An opinion vote should start with default settings.
Some of the essential defaults will be saved back to the configuration after the vote starts.
You can view the state of the configuration now in channel or PM, respectively:

	!vote config
	/msg SPQF config #spqf config
	/msg SPQF config #spqf config.vote
	/msg SPQF config #spqf config.vote.opine

Two essential variables that should have appeared in the top-level *config.vote* document are *for* and *duration*.
*duration* sets the time the voting motion is open. *for* sets how long the effects of the vote are applied, depending on the type.
For the opinion vote just enabled the effects are that it is available for viewing by using the !opinion command.
Assuming the test vote passed, type the following in channel:

	!opinion

At this time the enfranchisement permissions for who is allowed to vote are wide-open by default.
This is where things may become complicated, but can remain elegantly simple if you start with the following:

	/msg SPQF config #spqf config.vote.enfranchise.mode = v

This allows only users with voice (+v) to vote, applying to all vote types.
While *enfranchise* is not a vote type, the family of variables has its own document,
and each vote type can also have its own *enfranchise* document.
To configure the *opine* vote's enfranchisement specifically, taking priority over the global:

	/msg SPQF config #spqf config.vote.opine.enfranchise.mode = v

With this simple configuration you can now fall back on other aspects of IRC to modify what is now shared-state for voting access.
This includes using the existing ChanServ access lists (where ~200 people can have +V) or even using your own bot to conduct log analyses
more complex than what SPQF offers, by toggling +v on users.
