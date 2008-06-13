.SECONDARY:

# Commands
CP          := cp -f
ECHO        := echo
MKDIR       := mkdir -p
PWD         := pwd
DEL         := rm -f
CXX         := g++
Q           := $(if $(VERBOSE),,@)

# Packages
APACHE_CPPFLAGS := -I/usr/include/apache2 \
                   -I/usr/include/apr-1.0

BOOST_LDLIBS      := -lboost_iostreams
BOOST_TEST_LDLIBS := -lboost_unit_test_framework
MYSQL_LDLIBS      := -lmysqlclient
XERCES_LDLIBS     := -lxerces-c


# Directories
BASE_DIR    := $(shell pwd)
BUILD_DIR   := build
BIN_DIR     := bin
LIB_DIR     := $(BASE_DIR)/lib

# Global flags
CPPFLAGS    += -D_REENTRANT
CPPFLAGS    += -D_XOPEN_SOURCE
CPPFLAGS    += -Imod_osm
CPPFLAGS    += $(APACHE_CPPFLAGS)

CXXFLAGS    += -Wall -Werror
#CXXFLAGS    += -O2
CXXFLAGS    += -fexceptions -finline-functions
CXXFLAGS    += -g


# Default rule
default : all

# mod_osm shared object
MODOSM_DIR     		:= mod_osm
MODOSM_TARGET  		:= $(LIB_DIR)/mod_osm.so
MODOSM_SOURCES 		:= $(wildcard $(MODOSM_DIR)/*.cpp)
MODOSM_OBJECTS 		:= $(MODOSM_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
UNIT_TEST_TARGET	:= $(BIN_DIR)/unittests
ALL_TARGETS			+= $(MODOSM_TARGET)
ALL_TARGETS			+= $(UNIT_TEST_TARGET)
-include $(MODOSM_OBJECTS:%.o=%.d)


$(MODOSM_TARGET) : LDLIBS  := $(BOOST_LDLIBS) $(MYSQL_LDLIBS)
$(MODOSM_TARGET) : LDFLAGS := -shared -fPIC
$(MODOSM_TARGET) : $(MODOSM_OBJECTS)
	$(Q)$(ECHO) " [LINK] $(@F)"
	$(Q)$(MKDIR) $(@D)
	$(Q)$(LINK.cpp) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(UNIT_TEST_TARGET)	: LDLIBS  := $(BOOST_LDLIBS) $(MYSQL_LDLIBS) $(XERCES_LDLIBS)
$(UNIT_TEST_TARGET)	: LDFLAGS := -fPIC
$(UNIT_TEST_TARGET)	: build/mod_osm/xml_reader.o build/mod_osm/osm_data.o testing/unittests.cpp
	$(Q)$(ECHO)	" [LINK] $(@F)"
	$(Q)$(MKDIR) $(@D)
	$(Q)$(LINK.cpp) $^ $(LDLIBS) $(OUTPUT_OPTION)

# Common compile rule
$(BUILD_DIR)/%.o : %.cpp
	$(Q)$(ECHO) " [CC]   $(<F)"
	$(Q)$(MKDIR) $(@D)
	$(Q)$(COMPILE.cpp) $< -MD $(OUTPUT_OPTION)

.PHONY: default all clean

all : $(ALL_TARGETS)

clean :
	$(Q)$(DEL) -r $(BUILD_DIR)
