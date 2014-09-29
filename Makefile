

CC = g++
WFLAGS = -Wall                                 \
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
         -Wsuggest-attribute=format            \
         #-Wsuggest-attribute=noreturn

CCFLAGS = -std=c++11
LDFLAGS = -lircclient

TARGET = respublica


all: $(TARGET)

$(TARGET): main.o respub.o bot.o
	$(CC) -o $@ $(CCFLAGS) $(WFLAGS) $^ $(LDFLAGS)


main.o: main.cpp bot.h respub.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

respub.o: respub.cpp respub.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

bot.o: bot.cpp bot.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<


clean:
	rm -f *.o respublica
