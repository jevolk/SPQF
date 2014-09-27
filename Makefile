

CC = g++
WFLAGS = -Wall                                 \
         -Wextra                               \
         -Wcomment                             \
         -Waddress                             \
         -Winvalid-pch                         \
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
         -Wsuggest-attribute=format            \
         #-Wsuggest-attribute=noreturn

CCFLAGS = -std=c++11
LDFLAGS = -lircclient

all: respublica TAGS

respublica: respublica.o
	$(CC) -o $@ $(CCFLAGS) $(WFLAGS) $< $(LDFLAGS)

%.o: %.cpp
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

TAGS: *.cpp

clean:
	rm -f *.o respublica
