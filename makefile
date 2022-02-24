
# makefile gcc (da usare con gcc make, non watcom wmake)

BASE=../..

# 16 bit realmode
AFLAGS=-ml
CFLAGS=-ml -3 -fp3 -oxh
AS=wasm
CC=wcc

## 32 bit protected mode w/dos32x
#AFLAGS=-mf -3
#CFLAGS+=-mf -3 -fp3 -oxh
#AS=wasm
#CC=wcc386



all :
	@test -z "$$WATCOM" && \
	(echo lanciare ; echo . ~/owsetenv.sh ) || \
	echo specificare un target


# core=auto non va con i dos extenders:
# usare core=simple|normal
define DOSBOX_CONF
[cpu]
core=normal
#core=simple
#core=auto
cycles=max
#cycles=10000
[render]  
scaler=tv3x
aspect=false
#fullscreen=false
[autoexec]
MOUNT C . 
C:   
MAIN.EXE 2>@1 > LOG
TYPE LOG
endef
export DOSBOX_CONF

dosbox.conf : makefile
	echo "$$DOSBOX_CONF" > $@

box : dosbox.conf main.exe
	dosbox

emu : main.exe
	dosemu $^

vb : vb.iso
	virtualbox --startvm FreeDOS --dvd `readlink -f vb.iso`


define AUTOEXEC_BAT
MAIN.EXE 2>@1 > C:\LOG
TYPE C:\LOG
endef
export AUTOEXEC_BAT

autoexec.bat : makefile
	echo "$$AUTOEXEC_BAT" > $@

vb.iso : autoexec.bat main.exe
	mkisofs -f -o $@ $^
	isoinfo -l -i $@

dbg : dosbox.conf main.exe
	xterm -geometry 80x60+0+0 -fa fixed -fs 12 -e dosbox-dbg






OBS += hash.o

# wlink richiede l'obj con dentro la main a sinistra
# visto che non è specificato come OBS venga incrementata
# passando da un'altra var si assicura il main a sinistra
ALLOBS += main.o $(OBS)

COMMA=,
SPACE=$(subst ,, )

main.exe : makefile $(ALLOBS) 
	wlink sys dos file $(subst $(SPACE),$(COMMA),$(ALLOBS))


















.SUFFIXES :
.EXTENSIONS : 
.SUFFIXES : .o .c .cc .cpp .asm

.c.o : .AUTODEPEND
		$(CC) $(CFLAGS) $<
		
.cpp.o : .AUTODEPEND
		$(CC) $(CFLAGS) $<
		
.cc.o : .AUTODEPEND
		$(CC) $(CFLAGS) $<
		
.asm.o : .AUTODEPEND
		$(AS) $(AFLAGS) $<
		
clean cl :
	rm -f *.exe *.o *.err LOG dosbox.conf


.PHONY : prereq all clean cl box emu dbg vb


