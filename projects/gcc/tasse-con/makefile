# ----------------------------------
# Makefile for TASSE (Linux)
# Builds console version
# ----------------------------------
# Compiler and user-configurable settings
MK_CC:=g++

# Auto configuration
ifeq ($(MK_PROFILE),)
MK_PROFILE:=Release
endif
ifeq ($(MK_BITS),)
MK_BITS:=$(shell getconf LONG_BIT)
endif
MK_DEFS:=-I../../../src_main/shared -I../../../src_main/tasse
ifeq ($(MK_BITS),64)
MK_OUTPUT=tasse64
MK_MFLAG:=-m64
MK_DEFS:=$(MK_DEFS) -D_M_X64
MK_PLATFORM:=x64
else
MK_OUTPUT:=tasse
MK_MFLAG:=-m32
MK_DEFS:=$(MK_DEFS) -D_M_IX86
MK_PLATFORM:=x86
endif
ifneq (,$(findstring WINDOWS,$(PATH)))
MK_DEFS:=$(MK_DEFS) -D_WIN32
else
ifneq (,$(findstring Windows,$(PATH)))
MK_DEFS:=$(MK_DEFS) -D_WIN32
else
MK_DEFS:=$(MK_DEFS) -D_LINUX
endif
endif
ifeq ($(MK_PROFILE),Release)
MK_DEFS:=$(MK_DEFS) -DNDEBUG
MK_OPT:=-O2 -fno-stack-protector -ffast-math -freciprocal-math -fno-finite-math-only -fno-strict-aliasing
MK_LADD:=
else
MK_DEFS:=$(MK_DEFS) -D_DEBUG
MK_OPT:=-g 
MK_LADD:=-g
endif

MK_CFLAGS:=$(MK_MFLAG) $(MK_DEFS) -Wall -Wextra --std=c++11 -mtune=generic $(MK_OPT)
MK_LFLAGS:=$(MK_MFLAG) $(MK_LADD)
MK_ADDLIB:=-ldl -lstdc++ -static-libgcc -lm -lpthread

# Source files
MK_SRCDIR_ALL:=../../../src_main/tasse/
MK_SRCLIST_ALL:= \
	cfgfile.cpp \
	hbonds.cpp \
	logfile.cpp \
	nature.cpp \
	threads.cpp \
	topology.cpp \
	utils.cpp

MK_SRCDIR_CON:=../../../src_main/tasse-con/
MK_SRCLIST_CON:= \
	console.cpp \
	main.cpp

MK_SRCLIST_ALL:=$(MK_SRCLIST_ALL:%=$(MK_SRCDIR_ALL)%)
MK_SRCLIST_CON:=$(MK_SRCLIST_CON:%=$(MK_SRCDIR_CON)%)

# Output files
MK_OBJDIR:=../../../temp/tasse-con/linux/Output-$(MK_PROFILE)-$(MK_PLATFORM)/
MK_OBJDIR_ALL:=$(MK_OBJDIR)all/
MK_OBJLIST_ALL:=$(MK_SRCLIST_ALL:$(MK_SRCDIR_ALL)%.cpp=$(MK_OBJDIR_ALL)%.o)
MK_OBJDIR_CON:=$(MK_OBJDIR)con/
MK_OBJLIST_CON:=$(MK_SRCLIST_CON:$(MK_SRCDIR_CON)%.cpp=$(MK_OBJDIR_CON)%.o)
MK_INSTALLDIR:=../../../bin/

# Build rules
$(MK_OBJDIR_ALL)%.o: $(MK_SRCDIR_ALL)%.cpp
	@echo "Compiling: $<"
	$(MK_CC) $(MK_CFLAGS) -o "$@" -c "$<"
	
$(MK_OBJDIR_CON)%.o: $(MK_SRCDIR_CON)%.cpp
	@echo "Compiling: $<"
	$(MK_CC) $(MK_CFLAGS) -o "$@" -c "$<"

$(MK_OBJDIR)$(MK_OUTPUT): neat $(MK_OBJLIST_ALL) $(MK_OBJLIST_CON)
	@echo Linking $(MK_OUTPUT)...
	$(MK_CC) $(MK_LFLAGS) -o $@ $(MK_OBJLIST_ALL) $(MK_OBJLIST_CON) $(MK_ADDLIB)
	strip -s $@

neat:
	-@if [ ! -d "$(MK_OBJDIR_ALL)" ]; then mkdir -p "$(MK_OBJDIR_ALL)"; fi
	-@if [ ! -d "$(MK_OBJDIR_CON)" ]; then mkdir -p "$(MK_OBJDIR_CON)"; fi

clean:
	-@rm -f "$(MK_INSTALLDIR)$(MK_OUTPUT)"
	-@if [ -d "$(MK_OBJDIR)" ]; then rm -rf "$(MK_OBJDIR)"; fi
	
install: $(MK_OBJDIR)$(MK_OUTPUT)
	-@cp "$(MK_OBJDIR)$(MK_OUTPUT)" "$(MK_INSTALLDIR)$(MK_OUTPUT)"
