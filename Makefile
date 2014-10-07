
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
         #-Wsuggest-attribute=noreturn

CCFLAGS = -std=c++14
LDFLAGS = -lircbot -lircclient -lleveldb -lboost_locale -lpthread
LIBPATH = -Lircbot/

LIBRARIES = libircbot
TARGETS = spqf helpgen


all:  $(LIBRARIES) $(TARGETS)

clean:
	$(MAKE) -C ircbot clean
	rm -f *.o $(TARGETS)


libircbot:
	$(MAKE) -C ircbot

spqf: main.o respub.o voting.o help.o
	$(CC) -o $@ $(CCFLAGS) $(WFLAGS) $(LIBPATH) $^ $(LDFLAGS)

helpgen: helpgen.o help.o
	$(CC) -o $@ $(CCFLAGS) $^ $(WFLAGS)


main.o: main.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

respub.o: respub.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

voting.o: voting.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

help.o: help.cpp help.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

helpgen.o: helpgen.cpp help.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<
