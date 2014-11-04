
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
         -Wsuggest-attribute=format

VERSTR = $(shell git describe --tags)
CCFLAGS += -std=c++14 -Istldb -DSPQF_VERSION=\"$(VERSTR)\" -fstack-protector
LDFLAGS = -lircbot -lleveldb -lboost_system -lpthread
LIBPATH = -Lircbot/

LIBRARIES = libircbot
TARGETS = spqf cfgedit


all:  $(LIBRARIES) $(TARGETS)

clean:
	$(MAKE) -C ircbot clean
	rm -f *.o $(TARGETS)


libircbot:
	$(MAKE) -C ircbot

spqf: main.o respub.o voting.o votes.o praetor.o vote.o
	$(CC) -o $@ $(CCFLAGS) $(WFLAGS) $(LIBPATH) $^ $(LDFLAGS)

cfgedit: cfgedit.o
	$(CC) -o $@ $(CCFLAGS) $(WFLAGS) $(LIBPATH) $^ $(LDFLAGS)


main.o: main.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

respub.o: respub.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

voting.o: voting.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

votes.o: votes.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

praetor.o: praetor.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

vote.o: vote.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<


cfgedit.o: cfgedit.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<
