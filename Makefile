
CC = g++
WFLAGS = -pedantic                             \
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

VERSTR = $(shell git describe --tags)
CCFLAGS += -std=c++14 -Iircbot/stldb -DSPQF_VERSION=\"$(VERSTR)\" -fstack-protector
LDFLAGS += -fuse-ld=gold -Wl,--no-gnu-unique
LIBPATH += -Lircbot/

LIBRARIES = libircbot
TARGETS = spqf respub cfgedit


all:  $(LIBRARIES) $(TARGETS)

clean:
	$(MAKE) -C ircbot clean
	rm -f *.o *.so $(TARGETS)


libircbot:
	$(MAKE) -C ircbot

respub: respub.o voting.o votes.o praetor.o vdb.o vote.o log.o
	$(CC) -o $@.so $(CCFLAGS) -shared $(WFLAGS) $(LIBPATH) $^ $(LDFLAGS)

spqf: spqf.o
	$(CC) -o $@ $(CCFLAGS) -rdynamic $(WFLAGS) $(LIBPATH) $^ -lircbot -lleveldb -lboost_system -lpthread -ldl

cfgedit: cfgedit.o
	$(CC) -o $@ $(CCFLAGS) $(WFLAGS) $(LIBPATH) $^ $(LDFLAGS) -lircbot -lleveldb -lboost_system -lpthread


spqf.o: spqf.cpp
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

respub.o: respub.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) -fPIC $(WFLAGS) $<

voting.o: voting.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) -fPIC $(WFLAGS) $<

votes.o: votes.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) -fPIC $(WFLAGS) $<

praetor.o: praetor.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) -fPIC $(WFLAGS) $<

vdb.o: vdb.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) -fPIC $(WFLAGS) $<

vote.o: vote.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) -fPIC $(WFLAGS) $<

log.o: log.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) -fPIC $(WFLAGS) $<

cfgedit.o: cfgedit.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<
