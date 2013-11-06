CC=gcc
CXX=g++-4.6 -Werror
CFLAGS=
EXECUTABLES=pair-wise-dtw
INCLUDE=-I include/
CPPFLAGS= -std=c++0x -Wall -fstrict-aliasing $(CFLAGS) $(INCLUDE)
 
.PHONY: debug all o3 example
all: $(EXECUTABLES) ctags

o3: CFLAGS+=-O3
o3: all
debug: CFLAGS+=-g -DDEBUG
debug: all

vpath %.h include/
vpath %.cpp src/
vpath %.cu src/

SOURCES=fast_dtw.cpp
OBJ=$(addprefix obj/,$(SOURCES:.cpp=.o))

pair-wise-dtw: pair-wise-dtw.cpp $(OBJ)
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^
ctags:
	@ctags -R *

obj/%.o: %.cpp
	$(CXX) $(CPPFLAGS) -o $@ -c $<

obj/%.d: %.cpp
	@$(CXX) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,obj/\1.o $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

-include $(addprefix obj/,$(subst .cpp,.d,$(SOURCES)))

.PHONY:
clean:
	rm -rf $(EXECUTABLES) obj/* tags
