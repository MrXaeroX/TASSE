# ----------------------------------
# Makefile for TASSE (Linux)
# Builds GUI version
# ----------------------------------
# Compiler and user-configurable settings
MK_CC:=g++

# Auto configuration
MK_OUTPUT=qtasse
ifeq ($(MK_PROFILE),)
MK_PROFILE:=Release
endif
ifeq ($(MK_BITS),)
MK_BITS:=$(shell getconf LONG_BIT)
endif
MK_DEFS:=-I../../../src_main/shared -I../../../src_main/tasse -D_QTASSE
ifeq ($(MK_BITS),64)
MK_MFLAG:=-m64
MK_DEFS:=$(MK_DEFS) -D_M_X64
MK_PLATFORM:=x64
else
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
MK_STRIP:=strip -s
else
MK_DEFS:=$(MK_DEFS) -D_DEBUG
MK_OPT:=-g 
MK_LADD:=-g
MK_STRIP:=@:
endif

ifeq ($(MK_PLATFORM),x64)
MK_QTBIN:=/home/xaerox/Qt/lib/4.8.7-64/bin/
MK_QTINC:=/home/xaerox/Qt/lib/4.8.7-64/include
else
MK_QTBIN:=/home/xaerox/Qt/lib/4.8.7-32/bin/
MK_QTINC:=/home/xaerox/Qt/lib/4.8.7-32/include
endif
MK_MOC=$(MK_QTBIN)moc
MK_UIC=$(MK_QTBIN)uic
MK_RCC=$(MK_QTBIN)rcc
MK_DEFS+= -I$(MK_QTINC)

MK_CFLAGS:=$(MK_MFLAG) $(MK_DEFS) -Wall -Wextra --std=c++11 -mtune=generic $(MK_OPT)
MK_LFLAGS:=$(MK_MFLAG) $(MK_LADD) -Wl,-rpath,.
MK_DYNLIB:=libQtCore.so libQtGui.so libQtOpenGL.so
MK_ADDLIB:=$(MK_DYNLIB) -ldl -lstdc++ -static-libgcc -lm -lpthread

MK_LIBDIR:=../lib_$(MK_PLATFORM)/
MK_COPY_LIBS:=$(MK_DYNLIB:%=cp $(MK_LIBDIR)% .;)
MK_CLEAN_LIBS:=$(MK_DYNLIB:%=rm -f %;)

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

MK_SRCDIR_GUI:=../../../src_main/tasse-gui/
MK_SRCLIST_GUI:= \
	console.cpp \
	main.cpp \
	resource.cpp \
	window.cpp
	
MK_SRCDIR_MOC:=$(MK_SRCDIR_GUI)
MK_SRCLIST_MOC:= \
	window.h

MK_SRCLIST_ALL:=$(MK_SRCLIST_ALL:%=$(MK_SRCDIR_ALL)%)
MK_SRCLIST_GUI:=$(MK_SRCLIST_GUI:%=$(MK_SRCDIR_GUI)%)
MK_SRCLIST_MOC:=$(MK_SRCLIST_MOC:%=$(MK_SRCDIR_MOC)%)

# Output files
MK_OBJDIR:=../../../temp/tasse-gui/linux/Output-$(MK_PROFILE)-$(MK_PLATFORM)/
MK_OBJDIR_ALL:=$(MK_OBJDIR)all/
MK_OBJLIST_ALL:=$(MK_SRCLIST_ALL:$(MK_SRCDIR_ALL)%.cpp=$(MK_OBJDIR_ALL)%.o)
MK_OBJDIR_GUI:=$(MK_OBJDIR)gui/
MK_OBJLIST_GUI:=$(MK_SRCLIST_GUI:$(MK_SRCDIR_GUI)%.cpp=$(MK_OBJDIR_GUI)%.o)
MK_OBJDIR_MOC:=$(MK_OBJDIR)moc/
MK_OBJLIST_MOC:=$(MK_SRCLIST_MOC:$(MK_SRCDIR_MOC)%.h=$(MK_OBJDIR_MOC)moc_%.o)

MK_INSTALLDIR:=../../../output/tasse_$(MK_PLATFORM)/

# Build rules
$(MK_SRCDIR_MOC)moc_%.cpp: $(MK_SRCDIR_MOC)%.h
	@echo "Compiling: $<"
	-@$(MK_MOC) "$<" -o "$@"

$(MK_OBJDIR_MOC)moc_%.o: $(MK_SRCDIR_MOC)moc_%.cpp
	@echo "Compiling: $<"
	-@$(MK_CC) $(MK_CFLAGS) -o "$@" -c "$<"


$(MK_OBJDIR_ALL)%.o: $(MK_SRCDIR_ALL)%.cpp
	@echo "Compiling: $<"
	-@$(MK_CC) $(MK_CFLAGS) -o "$@" -c "$<"
	
$(MK_OBJDIR_GUI)%.o: $(MK_SRCDIR_GUI)%.cpp
	@echo "Compiling: $<"
	-@$(MK_CC) $(MK_CFLAGS) -o "$@" -c "$<"

$(MK_OBJDIR)$(MK_OUTPUT): neat resources $(MK_OBJLIST_MOC) $(MK_OBJLIST_ALL) $(MK_OBJLIST_GUI)
	@echo Linking $(MK_OUTPUT)...
	-@$(MK_CC) $(MK_LFLAGS) -o $@ $(MK_OBJLIST_ALL) $(MK_OBJLIST_GUI) $(MK_OBJLIST_MOC) $(MK_ADDLIB)
	-@$(MK_STRIP) $@
	
resources:
	-@echo Compiling resources...
	-@$(MK_RCC) $(MK_SRCDIR_GUI)resource.qrc -o $(MK_SRCDIR_GUI)resource.cpp

neat:
	-@if [ ! -d "$(MK_OBJDIR_ALL)" ]; then mkdir -p "$(MK_OBJDIR_ALL)"; fi
	-@if [ ! -d "$(MK_OBJDIR_GUI)" ]; then mkdir -p "$(MK_OBJDIR_GUI)"; fi
	-@if [ ! -d "$(MK_OBJDIR_MOC)" ]; then mkdir -p "$(MK_OBJDIR_MOC)"; fi
	-@$(MK_COPY_LIBS)

clean:
	-@rm -f "$(MK_INSTALLDIR)$(MK_OUTPUT)"
	-@if [ -d "$(MK_OBJDIR)" ]; then rm -rf "$(MK_OBJDIR)"; fi
	-@$(MK_CLEAN_LIBS)
	
install: $(MK_OBJDIR)$(MK_OUTPUT)
	-@cp "$(MK_OBJDIR)$(MK_OUTPUT)" "$(MK_INSTALLDIR)$(MK_OUTPUT)"

