SRC = $(wildcard *.c)
BINDIR = $(RIOTBOARD)/$(BOARD)/bin/
OBJ = $(SRC:%.c=$(BINDIR)%.o)
DEP = $(SRC:%.c=$(BINDIR)%.d)

INCLUDES += -I$(RIOTBASE)/sys/include/ -I$(RIOTBASE)/drivers/cc110x_ng/include/ -I$(RIOTBOARD)/$(BOARD)/include

$(BINDIR)avsextrem_base.a: $(OBJ)
	$(AR) rcs $(BINDIR)${ARCH} $(OBJ)

# pull in dependency info for *existing* .o files
-include $(OBJ:.o=.d)

# compile and generate dependency info
$(BINDIR)%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(BOARDINCLUDE) $(PROJECTINCLUDE) $(CPUINCLUDE) -c $*.c -o $(BINDIR)$*.o
	$(CC) $(CFLAGS) $(INCLUDES) $(BOARDINCLUDE) $(PROJECTINCLUDE) $(CPUINCLUDE) -MM $*.c > $(BINDIR)$*.d
	@printf "$(BINDIR)" | cat - $(BINDIR)$*.d > /tmp/riot_out && mv /tmp/riot_out $(BINDIR)$*.d

# remove compilation products

clean:
	rm -f $(BINDIR)$(MODULE).a $(OBJ) $(DEP) $(ASMOBJ)

