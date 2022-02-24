
# Makefile gcc (da usare con gcc make, non watcom wmake)

# 16 bit realmode
AFLAGS=-ml
CFLAGS=-ml -3
AS=wasm
CC=wcc

## 32 bit protected mode w/dos32x
#AFLAGS=-mf -3
#CFLAGS=-mf -3
#AS=wasm
#CC=wcc386

# optim
CFLAGS+=-fp3 -oxt

all :
	@test -z "$$WATCOM" && (echo lanciare ; echo . ~/owsetenv.sh ) || echo specificare un target

# dosbox.conf minimale
# core=auto non va con i dos extenders:
# usare core=simple/normal
define DOSBOX_CONF
[cpu]
cycles=max
core=auto
#core=simple
#core=normal
[render]  
scaler=tv3x
aspect=false
[autoexec]
MOUNT C . 
C:   
MAIN.EXE > LOG
TYPE LOG
#MAIN.EXE
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
main.exe > c:\log
type c:\log
endef
export AUTOEXEC_BAT

autoexec.bat : makefile
	echo "$$AUTOEXEC_BAT" > $@

vb.iso : autoexec.bat main.exe
	mkisofs -f -o $@ $^
	isoinfo -l -i $@

dbg : dosbox.conf main.exe
	xterm -geometry 80x60+0+0 -fa fixed -fs 12 -e dosbox-dbg


# per i moduli aggiuntivi
# creare il symlink alla directory

#OBS += keyboard.o
#CFLAGS += -i=keyboard
#keyboard.o : makefile keyboard/keyboard.c
#	$(CC) $(CFLAGS) keyboard/keyboard.c

#OBS += debug.o
#CFLAGS += -i=debug
#debug.o : makefile debug/debug.c
#	$(CC) $(CFLAGS) debug/debug.c

OBS += hash.o



ALLOBS = main.o $(OBS)

main.exe : makefile $(ALLOBS) $(INC)
	wlink sys dos file {$(ALLOBS)}
#	wlink sys dos32a file {$(ALLOBS)}





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
	rm -f *.exe *.o *.err LOG dosbox.conf *.iso *.bat


.PHONY : all clean cl box emu dbg vb


