## ----------------------------------------------------------------------------------
##
## Makefile for the Windows platform
##
## ----------------------------------------------------------------------------------

## ---------------------------------------------------------------
## Compiler and linker...
##
CXX := cl
LD := $(CXX)
LIBRARIAN := lib

## ---------------------------------------------------------------
## SOURCES, INCLUDES, LIBS, EXTLIBS and DEFINES are set by specific makefiles...
_SOURCES := $(SOURCES)																		# Source files for a given module are referenced from the module directory
_OBJS := $(patsubst %.cpp, objs/%.o, $(SOURCES))											# List of object files
_INCLUDES := $(addprefix -I, $(INCLUDES)) -I$(TUCUXI_ROOT)\src								# Include directories are referenced from Tucuxi's root directory
_LIBS := $(foreach _LIB, $(LIBS), $(TUCUXI_ROOT)\bin\$(_LIB).lib) $(EXTLIBS)				# Libs are rerefenced by their name only 
_DEFINES := $(addprefix -D, $(DEFINES))

## ---------------------------------------------------------------
## Build flags.
##
CCFLAG :=  -Os -c -Oy -EHsc -nologo $(_INCLUDES) $(_DEFINES)
ASFLAG := -W $(_INCLUDES)
LDFLAG_APP := -nologo -MT -Fmobjs\$(NAME).map
LDFLAG_DLL := -nologo -LD -MD -Fmobjs\$(NAME).map

## ---------------------------------------------------------------
## Rules for construction of a static library
##
ifeq ($(TYPE), LIB)
all : $(_OBJS)
	$(LIBRARIAN) /NOLOGO /VERBOSE /OUT:objs\$(NAME).lib $(_OBJS) $(_LIBS) 
	copy /Y /V objs\$(NAME).lib $(TUCUXI_ROOT)\bin

clean:
	del /Q objs\*
	del /Q $(TUCUXI_ROOT)\bin\$(NAME).lib
endif

## ---------------------------------------------------------------
## Rules for construction of a dynamic library
##
ifeq ($(TYPE), DLL)
all : $(_OBJS)
	$(LD) $(LDFLAG_DLL) -Feobjs\$(NAME).dll $(_OBJS) $(_LIBS) 
	copy /Y /V objs\$(NAME).dll $(TUCUXI_ROOT)\bin

clean:
	del /Q objs\*
	del /Q $(TUCUXI_ROOT)\bin\$(NAME).dll
endif

## ---------------------------------------------------------------
## Rules for construction of a c++ application
##
ifeq ($(TYPE), APP)
all : $(_OBJS)
	$(LD) $(LDFLAG_APP) -Feobjs\$(NAME).exe $(_OBJS) $(_LIBS) 
	copy /Y /V objs\$(NAME).exe $(TUCUXI_ROOT)\bin

clean:
	del /Q objs\*
	del /Q $(TUCUXI_ROOT)\bin\$(NAME).exe
endif

## ---------------------------------------------------------------
## Rules for construction of a c++ unit test application
##
ifeq ($(TYPE), TEST)
all : $(_OBJS)
	$(LD) $(LDFLAG_APP) -Feobjs\$(NAME)-Test.exe $(_OBJS) $(_LIBS) 
	copy /Y /V objs\$(NAME)-test.exe $(TUCUXI_ROOT)\bin

clean:
	del /Q objs\*
	del /Q $(TUCUXI_ROOT)\bin\$(NAME)-test.exe
endif

## ---------------------------------------------------------------
## Generic rules
##
objs/%.o: %.cpp
	if not exist objs mkdir objs
	$(CXX) -Foobjs/$*.o $< $(CCFLAG)

