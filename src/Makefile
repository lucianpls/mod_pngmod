MODULE = mod_pngmod
C_SRC = $(MODULE).cpp
FILES = $(C_SRC)

DEFINES = -DLINUX -D_REENTRANT -D_GNU_SOURCE $(DEBUG)

# Create the ../Makefile.lcl, which should define
# DEST, the folder where the apache module should reside
# EXTRA_INCLUDES, the gcc include commands for httpd, apr headers
# LIBTOOL, DEBUG ...

MAKEOPT ?= Makefile.lcl
include $(MAKEOPT)
TARGET = .libs/$(MODULE).so

# Can't use apxs to build c++ modules
# The options used here might depend on how apache was built
$(TARGET)       :       $(FILES)
	$(LIBTOOL) --silent --mode=compile g++ -prefer-pic -O2 -Wall $(DEFINES) $(EXTRA_INCLUDES) -I $(EXP_INCLUDEDIR) -pthread -c -o $(MODULE).lo $(C_SRC) && touch $(MODULE).slo
	$(LIBTOOL) --silent --mode=link g++ -o $(MODULE).la -rpath $(LIBEXECDIR) -module -avoid-version $(MODULE).lo

install : $(TARGET)
	$(SUDO) $(CP) $(TARGET) $(DEST)

clean   :
	$(RM) -r .libs *.{o,lo,slo,la}
