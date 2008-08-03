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

BOOST_LDLIBS      := -lboost_iostreams -lboost_date_time -lboost_regex -lboost_system
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
CPPFLAGS    += -Iosm_core
CPPFLAGS    += $(APACHE_CPPFLAGS)

#CXXFLAGS    += -Wall -Werror
CXXFLAGS	+= -Wall
CXXFLAGS    += -O2
CXXFLAGS    += -fexceptions
# -finline-functions
CXXFLAGS    += -g


# Default rule
default : all

OSMCORE_DIR			:= osm_core
OSMCORE_TARGET		:= $(LIB_DIR)/osm_core.o
OSMCORE_SOURCES		:= $(wildcard $(OSMCORE_DIR)/*.cpp)
OSMCORE_OBJECTS		:= $(OSMCORE_SOURCES:%.cpp=$(BUILD_DIR)/%.o)

# mod_osm shared object
MODOSM_DIR     		:= mod_osm
MODOSM_TARGET  		:= $(LIB_DIR)/mod_osm.so
MODOSM_SOURCES 		:= $(wildcard $(MODOSM_DIR)/*.cpp)
MODOSM_OBJECTS 		:= $(MODOSM_SOURCES:%.cpp=$(BUILD_DIR)/%.o) $(LIB_DIR)/osm_core.o
UNIT_TEST_TARGET	:= $(BIN_DIR)/unittests
UE_TARGET		    := $(BIN_DIR)/userextractor
OSMCOMPARE_TARGET	:= $(BIN_DIR)/osmcompare
ROUTEAPP_TARGET		:= $(BIN_DIR)/routeapp
ALL_TARGETS			+= $(MODOSM_TARGET)
ALL_TARGETS			+= $(UNIT_TEST_TARGET)
ALL_TARGETS			+= $(UE_TARGET)
ALL_TARGETS			+= $(OSMCOMPARE_TARGET)
ALL_TARGETS			+= $(ROUTEAPP_TARGET)
-include $(MODOSM_OBJECTS:%.o=%.d)

$(OSMCORE_TARGET)	: $(OSMCORE_OBJECTS)
	$(Q)$(ECHO)	" [LINK] $(@F)"
	$(Q)$(MKDIR) $(@D)
	$(Q)$(LINK.cpp) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(MODOSM_TARGET) : LDLIBS  := $(BOOST_LDLIBS) $(MYSQL_LDLIBS) $(XERCES_LDLIBS) 
$(MODOSM_TARGET) : LDFLAGS := -shared -fPIC
$(MODOSM_TARGET) : $(MODOSM_OBJECTS)
	$(Q)$(ECHO) " [LINK] $(@F)"
	$(Q)$(MKDIR) $(@D)
	$(Q)$(LINK.cpp) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(UNIT_TEST_TARGET)	: LDLIBS  := $(BOOST_LDLIBS) $(MYSQL_LDLIBS) $(XERCES_LDLIBS)
$(UNIT_TEST_TARGET)	: LDFLAGS := -fPIC
$(UNIT_TEST_TARGET)	: testing/unittests.cpp $(OSMCORE_TARGET)
	$(Q)$(ECHO)	" [LINK] $(@F)"
	$(Q)$(MKDIR) $(@D)
	$(Q)$(LINK.cpp) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(UE_TARGET)	: LDLIBS  := $(BOOST_LDLIBS) $(MYSQL_LDLIBS) $(XERCES_LDLIBS)
$(UE_TARGET)	: LDFLAGS := -fPIC
$(UE_TARGET)	: testing/userextractor.cpp $(OSMCORE_TARGET)
	$(Q)$(ECHO)	" [LINK] $(@F)"
	$(Q)$(MKDIR) $(@D)
	$(Q)$(LINK.cpp) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(OSMCOMPARE_TARGET)	: LDLIBS  := $(BOOST_LDLIBS) $(MYSQL_LDLIBS) $(XERCES_LDLIBS)
$(OSMCOMPARE_TARGET)	: LDFLAGS := -fPIC
$(OSMCOMPARE_TARGET)	: testing/osm_xml_compare.cpp $(OSMCORE_TARGET)
	$(Q)$(ECHO)	" [LINK] $(@F)"
	$(Q)$(MKDIR) $(@D)
	$(Q)$(LINK.cpp) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(ROUTEAPP_TARGET)	: LDLIBS  := $(BOOST_LDLIBS) $(MYSQL_LDLIBS) $(XERCES_LDLIBS)
$(ROUTEAPP_TARGET)	: LDFLAGS := -fPIC
$(ROUTEAPP_TARGET)	: routeapp/routeapp.cpp $(OSMCORE_TARGET)
	$(Q)$(ECHO)	" [LINK] $(@F)"
	$(Q)$(MKDIR) $(@D)
	$(Q)$(LINK.cpp) $^ $(LDLIBS) $(OUTPUT_OPTION)


# Common compile rule
$(BUILD_DIR)/%.o : %.cpp
	$(Q)$(ECHO) " [CC]   $(<F)"
	$(Q)$(MKDIR) $(@D)
	$(Q)$(COMPILE.cpp) $< -MD $(OUTPUT_OPTION)

.PHONY: default all clean


install:
	cp $(MODOSM_TARGET) /usr/lib/apache2/modules
	apache2ctl stop
	apache2ctl start

all : $(ALL_TARGETS)

clean :
	$(Q)$(DEL) -r $(BUILD_DIR)
