PLATFORM := $(shell uname -s)
CC      = gcc
PROF    = -O -g
NOCRYPT =
C_FLAGS =  -Wall $(PROF) $(DEBUG) $(NOCRYPT)
L_FLAGS =  $(PROF) $(DEBUG)
LIBS    :=
ifeq ($(PLATFORM), Linux)
LIBS    += -lcrypt
endif
O_FILES :=
O_FILES += act_comm.o
O_FILES += act_enter.o
O_FILES += act_info.o
O_FILES += act_move.o
O_FILES += act_obj.o
O_FILES += act_wiz.o
O_FILES += alias.o
O_FILES += ban.o
O_FILES += bit.o
O_FILES += comm.o
O_FILES += const.o
O_FILES += db.o
O_FILES += db2.o
O_FILES += effects.o
O_FILES += fight.o
O_FILES += flags.o
O_FILES += handler.o
O_FILES += healer.o
O_FILES += interp.o
O_FILES += note.o
O_FILES += lookup.o
O_FILES += magic.o
O_FILES += magic2.o
O_FILES += music.o
O_FILES += olc.o
O_FILES += olc_act.o
O_FILES += olc_save.o
O_FILES += recycle.o
O_FILES += save.o
O_FILES += scan.o
O_FILES += skills.o
O_FILES += special.o
O_FILES += tables.o
O_FILES += update.o
O_FILES += string.o
O_FILES += mem.o

rom: $(O_FILES)
	rm -f rom
	$(CC) $(L_FLAGS) -o rom $(O_FILES) $(LIBS)

.c.o:
	$(CC) -c $(C_FLAGS) $<


clean:
	rm -f *.o rom
