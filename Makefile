######################################################################
#
#			Author:	Kyven Wu
#			Date:	09/15/2020
#
######################################################################

BIN=bin/
SRC=src/
INCLUDE_DIR=$(SRC)include/
FS_DIR=$(SRC)fs/
PENNFAT_DIR=$(SRC)pennfat/
PENNOS_DIR=$(SRC)pennos/
LOG_DIR=$(shell pwd)/log
LOGFILE=\"$(LOG_DIR)/log.txt\"

LOGFILE=\"$(shell pwd)/log/log.txt\"

PENNOS=penn-os
PENNFAT=pennfat

PROMPT='"$(PENNOS)> "'

# Remove -DNDEBUG during development if assert(3) is used
#
override CPPFLAGS += -DNDEBUG -DPENNOS=$(PENNOS) -DPENNFAT=$(PENNFAT)
override CPPFLAGS += -DNDEBUG -DPROMPT=$(PROMPT) -DLOGFILE=$(LOGFILE)

CC=clang

# Replace -O1 with -g for a debug version during development
#
CFLAGS=-Wall -Werror -g

# Add relevant files prefixes here
FS-FILES = fat file

PENNFAT-FILES = pennfat pennfathandler

PENNOS-FILES = handlejob iter job jobcontrol jobQueue \
			   kernel node queue scheduler shell \
			   token user_level_funcs filedescriptor

FS-FILES-IN = $(addsuffix .o, $(addprefix $(FS_DIR), $(FS-FILES)))

PENNFAT-FILES-IN  = $(addsuffix .o, $(addprefix $(PENNFAT_DIR), $(PENNFAT-FILES)))

PENNOS-FILES-IN  = $(addsuffix .o, $(addprefix $(PENNOS_DIR), $(PENNOS-FILES)))

# Compile all C source files in the current working directory and link with the
# parsejob.o job parser module.
all : $(BIN)/$(PENNOS) $(BIN)/$(PENNFAT) $(shell mkdir $(LOG_DIR)) $(shell mkdir $(BIN))

# Target for pennos binary
$(BIN)/$(PENNOS) :  $(FS-FILES-IN) $(PENNOS-FILES-IN)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDE_DIR)parsejob.o -lm

# Target for pennfat binary
$(BIN)/$(PENNFAT) : $(FS-FILES-IN) $(PENNFAT-FILES-IN)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDE_DIR)parsejob.o -lm

# Remove program binaries and all .o files except for parsejob.o
clean :
	rm -f $(BIN)$(PENNOS) $(BIN)$(PENNFAT)
	rm -f $(addsuffix .o, $(addprefix $(FS_DIR), $(FS-FILES)))
	rm -f $(addsuffix .o, $(addprefix $(PENNFAT_DIR), $(PENNFAT-FILES)))
	rm -f $(addsuffix .o, $(addprefix $(PENNOS_DIR), $(PENNOS-FILES)))