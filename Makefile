#general
HOST_ARCH	:= $(shell uname -m)

ifneq ($(HOST_ARCH), armv7l)
CC			:= $(shell which arm-linux-gnueabihf-gcc)
else
CC			:= $(shell which gcc)
endif

RM			:= $(shell which rm) -rf
MKDIR		:= $(shell which mkdir) -p
MAKE		:= $(shell which make) --no-print-directory
OCTAVE		:= $(shell which octave)

#Paths
srcPath		:= src
libPath		:= lib
scrPath		:= script
incPath		:= inc
objPath		:= obj
binPath		:= bin

#Flags
wFlags 		:= -Wall -O3
# Archs 		:= -march=armv7-a -mfloat-abi=hard -mfpu=neon
Archs 		:= -march=armv7-a -mtune=cortex-a7 -ftree-vectorize -mhard-float -mfloat-abi=hard -mfpu=neon -ffast-math -mvectorize-with-neon-quad
LibsPath	:= -L/usr/lib/arm-linux-gnueabihf
Frameworks 	:= -lasound -lm -lpthread
Libs		:= -I$(libPath)
CFLAGS		:= $(wFlags) $(Archs) $(Libs)
LDFLAGS 	:= $(LibsPath) $(Frameworks)

# build version
include Makefile.buildver

#Project Name
Project		:= CFDBM
buildName	?=
buildPath	:= $(srcPath)/$(buildName)
buildPath	:= $(buildPath:%/=%)
buildFlags  := -D __BUILD__

#Debug
dbgFlags	:= -g -D __DEBUG__
debugPath	:= debug
verbose		?= context
MK_VERBOSE	?= no
Release		?= no

#targets
TARGETS		:= dbg$(Project)
ifeq ($(Release), yes)
TARGETS		:= $(Project)
endif

#verbose options: alloc, op&data, context, memory, print, graphics, all
ifeq ($(verbose), all)
  verbose += alloc op&data context memory print graphics
endif

ifneq (,$(findstring alloc , $(verbose)))
  dbgFlags += -D __DEBUG__MALLOC__
  dbgFlags += -D __DEBUG__FREE__
endif

ifneq (,$(findstring context , $(verbose)))
  dbgFlags += -D __DEBUG__CONTEXT__
  dbgFlags += -D __DEBUG__INLINE__
endif

ifneq (,$(findstring op&data , $(verbose)))
  dbgFlags += -D __DEBUG__INIT__
  dbgFlags += -D __DEBUG__OPERATION__
endif

ifneq (,$(findstring memory , $(verbose)))
  dbgFlags += -D __DEBUG__MEMORY__
endif

ifeq ($(verbose), graphics)
  dbgFlags += -D __DEBUG__OpenGL__
  dbgFlags += -D __DEBUG__GCONTEXT__
endif

ifneq (,$(findstring print , $(verbose)))
  dbgFlags += -D __DEBUG__PRINT__
endif

#Colors
BLACK	:= \033[0;30m
GRAY	:= \033[1;30m
RED	:= \033[0;31m
LRED	:= \033[1;31m
GREEN	:= \033[0;32m
LGREEN	:= \033[1;32m
BROWN	:= \033[0;33m
YELLOW	:= \033[1;33m
BLUE	:= \033[0;34m
LBLUE	:= \033[1;34m
PURPLE	:= \033[0;35m
LPURPLE	:= \033[1;35m
CYAN	:= \033[0;36m
LCYAN	:= \033[1;36m
LGRAY	:= \033[0;37m
WHITE	:= \033[1;37m
NOCOLOR := \033[0m

#common: auto rec scan
inc 	:= $(shell find $(incPath) -name '*.h')
src  	:= $(shell find $(buildPath) -name '*.c')
scr	:= $(shell find $(scrPath) -name '*.m')

obj	:= $(src:$(srcPath)/%c=$(objPath)/%o)
dobj	:= $(src:$(srcPath)/%c=$(debugPath)/%o)

ifeq ($(MK_VERBOSE), no)
	SHOW := @
else
	SHOW :=
endif

all: mrproper dep $(TARGETS)

#dep scripts
dep: depRes $(scr:%m=%srn)

tools/%: tools/%.c
	$(SHOW)$(MAKE) compile OBJ='no' out=$@ objects=$<

depRes:
	$(SHOW)echo "$(LRED)Resolving Dependecies...$(NOCOLOR)"
	$(SHOW)echo "$(LRED)Scripts found: $(GREEN)$(scr)$(NOCOLOR)"
	$(SHOW)$(RM) $(scrPath)/*.srn

$(scrPath)/%.srn: $(scrPath)/%.m
	$(SHOW)#$(OCTAVE) $< 2> $@

#main build
$(Project): $(obj)
	$(SHOW)$(MAKE) directory path=$(binPath)/Release
	$(SHOW)$(MAKE) compile OBJ='no' Flags="$(buildFlags)" out=$(binPath)/Release/$(Project) objects="$^"

dbg$(Project): $(dobj)
	$(SHOW)$(MAKE) directory path=$(binPath)/Debug
	$(SHOW)$(MAKE) compile OBJ='no' Flags="$(buildFlags) $(dbgFlags)" out=$(binPath)/Debug/$(Project) objects="$^"

$(objPath)/%.o: $(srcPath)/%.c
	$(SHOW)$(MAKE) directory path=$(dir $@)
	$(SHOW)$(MAKE) compile OBJ='yes' Flags="$(buildFlags)" out=$@ in=$<

#debug
$(debugPath)/%.o: $(srcPath)/%.c
	$(SHOW)$(MAKE) directory path=$(dir $@)
	$(SHOW)$(MAKE) compile OBJ='yes' Flags="$(dbgFlags)" out=$@ in=$< 2> $(basename $@).err

directory:
	$(SHOW)[ -d $(path) ] || echo "$(LRED)Creating $(LPURPLE)Path:$(GREEN) $(path)$(NOCOLOR)"; $(MKDIR) $(path)

compile:
	$(SHOW)if [ $(OBJ) = 'yes' ]; then \
		echo "$(LRED)Generating $(LPURPLE)Object $(LRED)file:$(GREEN) $(out)$(NOCOLOR)"; \
		$(CC) -I $(incPath) $(CFLAGS) $(Flags) -c -o $(out) $(in); \
	else \
		echo "$(LRED)Generating $(LPURPLE)Binary $(LRED)file:$(GREEN) $(out)$(NOCOLOR)"; \
		$(CC) -I $(incPath) $(LDFLAGS) $(CFLAGS) $(Flags) -o $(out) $(objects); \
	fi

mrproper:
	$(SHOW)echo "$(LRED)Cleaning...$(NOCOLOR)"
	$(SHOW)[ ! -d $(binPath)   ] || $(RM) $(binPath)/*
	$(SHOW)[ ! -d $(objPath)   ] || $(RM) $(objPath)/*
	$(SHOW)[ ! -d $(debugPath) ] || $(RM) $(debugPath)/*

clean:
	$(SHOW)echo "$(LRED)So clean!$(NOCOLOR)"
	$(SHOW)[ ! -d $(binPath)   ] || $(RM) $(binPath)
	$(SHOW)[ ! -d $(objPath)   ] || $(RM) $(objPath)
	$(SHOW)[ ! -d $(debugPath) ] || $(RM) $(debugPath)
