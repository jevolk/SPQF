###############################################################################
#
# SPQF Makefile
#
#	Author:
#		COPYRIGHT (C) Jason Volk 2016
#
# This file is free and available under the GNU GPL terms specified in COPYING.
#
# The production build process should work by simply typing `make`. libircbot
# is built recursively and may require a second `make` command to complete
# linking.
#
# For developers, export SPQF_DEVELOPER=1 which adjusts the CCFLAGS for non-
# production debug builds.
#
###############################################################################


###############################################################################
#
# SPQF options
#

SPQF_CC       := g++
SPQF_VERSTR   := $(shell git describe --tags)
SPQF_CCFLAGS  := -std=c++14 -Iircbot/stldb -DSPQF_VERSION=\"$(SPQF_VERSTR)\" -fstack-protector
SPQF_LDFLAGS  += -fuse-ld=gold -Wl,--no-gnu-unique -Lircbot/
SPQF_WFLAGS   += -pedantic                             \
                 -Wall                                 \
                 -Wextra                               \
                 -Wcomment                             \
                 -Waddress                             \
                 -Winit-self                           \
                 -Wuninitialized                       \
                 -Wunreachable-code                    \
                 -Wvolatile-register-var               \
                 -Wvariadic-macros                     \
                 -Woverloaded-virtual                  \
                 -Wpointer-arith                       \
                 -Wlogical-op                          \
                 -Wcast-align                          \
                 -Wcast-qual                           \
                 -Wstrict-aliasing=2                   \
                 -Wstrict-overflow                     \
                 -Wwrite-strings                       \
                 -Wformat-y2k                          \
                 -Wformat-security                     \
                 -Wformat-nonliteral                   \
                 -Wfloat-equal                         \
                 -Wdisabled-optimization               \
                 -Wno-missing-field-initializers       \
                 -Wmissing-format-attribute            \
                 -Wno-unused-parameter                 \
                 -Wno-unused-label                     \
                 -Wno-unused-variable                  \
                 -Wsuggest-attribute=format



###############################################################################
#
# Composition of SPQF options and user environment options
#


ifdef SPQF_DEVELOPER
	SPQF_CCFLAGS += -ggdb -O0
	export IRCBOT_DEVELOPER := 1
else
	SPQF_CCFLAGS += -DNDEBUG -D_FORTIFY_SOURCE=1 -O3
endif


SPQF_CCFLAGS += $(SPQF_WFLAGS) $(CCFLAGS)
SPQF_LDFLAGS += $(LDFLAGS)



###############################################################################
#
# Final build targets composition
#

SPQF_LIBRARIES := libircbot
SPQF_TARGETS := spqf respub cfgedit


all:  $(SPQF_LIBRARIES) $(SPQF_TARGETS)

clean:
	$(MAKE) -C ircbot clean
	rm -f *.o *.so $(SPQF_TARGETS)

libircbot:
	$(MAKE) -C ircbot


respub: respub.o voting.o votes.o praetor.o vdb.o vote.o log.o
	$(SPQF_CC) -o $@.so $(SPQF_CCFLAGS) -shared $(SPQF_LDFLAGS) $^

spqf: spqf.o
	$(SPQF_CC) -o $@ $(SPQF_CCFLAGS) -rdynamic $(SPQF_LDFLAGS) $^ -lircbot -lleveldb -lboost_system -lpthread -ldl

cfgedit: cfgedit.o
	$(SPQF_CC) -o $@ $(SPQF_CCFLAGS) $(SPQF_LDFLAGS) $^ -lircbot -lleveldb -lboost_system -lpthread


spqf.o: spqf.cpp
	$(SPQF_CC) -c -o $@ $(SPQF_CCFLAGS) $<

respub.o: respub.cpp *.h
	$(SPQF_CC) -c -o $@ $(SPQF_CCFLAGS) -fPIC $<

voting.o: voting.cpp *.h
	$(SPQF_CC) -c -o $@ $(SPQF_CCFLAGS) -fPIC $<

votes.o: votes.cpp *.h
	$(SPQF_CC) -c -o $@ $(SPQF_CCFLAGS) -fPIC $<

praetor.o: praetor.cpp *.h
	$(SPQF_CC) -c -o $@ $(SPQF_CCFLAGS) -fPIC $<

vdb.o: vdb.cpp *.h
	$(SPQF_CC) -c -o $@ $(SPQF_CCFLAGS) -fPIC $<

vote.o: vote.cpp *.h
	$(SPQF_CC) -c -o $@ $(SPQF_CCFLAGS) -fPIC $<

log.o: log.cpp *.h
	$(SPQF_CC) -c -o $@ $(SPQF_CCFLAGS) -fPIC $<

cfgedit.o: cfgedit.cpp *.h
	$(SPQF_CC) -c -o $@ $(SPQF_CCFLAGS) $<
