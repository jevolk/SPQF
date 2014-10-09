# 
#  COPYRIGHT 2014 (C) Jason Volk
#  COPYRIGHT 2014 (C) Svetlana Tkachenko
#
#  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
#

TARGET = libircbot.a

CC = g++
CCFLAGS += -std=c++11
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
         -Wsuggest-attribute=format            \
         #-Wsuggest-attribute=noreturn


all: $(TARGET)


$(TARGET): irclib.o bot.o
	ar rc $@ $^
	ranlib $@

irclib.o: irclib.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

bot.o: bot.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<


clean:
	rm -f *.o *.a
