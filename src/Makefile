#This is the AFKMud Makefile.
#This should be usable on pretty much any Linux system.
#Refer to your 'man gcc' for explanations of these options
#To compile in FreeBSD or OpenBSD, use the gmake compiler.

#Used for Alsherok stuff, uncommenting it should have no affect, though why would you want to?
#ALSHEROK = 1

#Change this to reflect whichever compiler you want to use.
CC      = g++

#Type of machine to compile for. Athlon works just as well on Duron too.
#The march option needs to match the general family of CPU.
#If you don't know what to set for these options, and your system administrator doesn't either, comment this line out
#MACHINE = -march=athlon-xp

#Uncomment if compiling in Windows under Cygwin
#WINDOZE = 1

# Uncomment the two lines below if compiling on a Solaris box
#SOLARIS_FLAG = -Dsun -DSYSV -Wno-char-subscripts
#SOLARIS_LINK = -lnsl -lsocket -lresolv

#Uncomment the line below if you are getting undefined references to dlsym, dlopen, and dlclose.
#Comment it out if you get errors about ldl not being found.
NEED_DL = -ldl

#Some systems need this for dynamic linking to work.
EXPORT_SYMBOLS = -export-dynamic

#Math functions - uncomment if you get errors or warnings about math functions.
MATH_LIB = -lm

#Intermud-3 - Comment out to disable I3 support in your code
#I3 = 1

#IMC2 - Comment out to disable IMC2 support
IMC = 1

#Internal Web Server - comment out to disable web server code.
#WEB = 1

#Multiport support. Comment out to disable this feature.
#MULTIPORT = 1

#Miscellaneous compiler options.
OPT_FLAG = -pipe -Os
DEBUG_FLAG = -g2
#PROF_FLAG = -pg

W_FLAGS = -Wall -Wextra -Wformat-security -Wshadow -Wpointer-arith -Wcast-align -Wcast-qual -Wredundant-decls

ifndef WINDOZE
   W_FLAGS := -Werror
endif

C_FLAGS = $(MACHINE) $(W_FLAGS) $(DEBUG_FLAG) $(OPT_FLAG) $(PROF_FLAG) $(SOLARIS_FLAG) $(EXPORT_SYMBOLS)
L_FLAGS = $(MACHINE) $(DEBUG_FLAG) $(OPT_FLAG) $(PROF_FLAG) $(EXPORT_SYMBOLS) $(SOLARIS_LINK) -lz $(MATH_LIB) $(NEED_DL)
#D_FLAGS : For the DNS Resolver process. No need in linking all the extra libs for this.
D_FLAGS = $(MACHINE) $(W_FLAGS) $(DEBUG_FLAG) $(OPT_FLAG) $(PROF_FLAG) $(SOLARIS_LINK)

C_FILES = act_comm.c act_info.c act_move.c act_obj.c act_wiz.c \
          archery.c areaconvert.c auction.c ban.c bits.c boards.c \
          build.c calendar.c channels.c clans.c color.c comm.c comments.c connhist.c \
          const.c db.c deity.c editor.c environment.c event.c event_handler.c features.c \
          fight.c finger.c handler.c hashstr.c help.c hotboot.c iafk.c idale.c imm_host.c \
          interface.c interp.c ismaug.c liquids.c magic.c md5.c \
          misc.c mspecial.c mudcfg.c mud_comm.c mud_prog.c \
          new_auth.c olcmob.c olcobj.c olcroom.c overland.c \
	  pfiles.c player.c polymorph.c rent.c renumber.c reset.c \
          save.c sha256.c ships.c shops.c skills.c skyship.c slay.c \
          tables.c track.c treasure.c update.c

ifdef WEB
   C_FILES := websvr.c $(C_FILES)
   C_FLAGS := $(C_FLAGS) -DWEBSVR
endif

ifdef IMC
   C_FILES := imc.c $(C_FILES)
   C_FLAGS := $(C_FLAGS) -DIMC
endif

ifdef I3
   C_FILES := i3.c $(C_FILES)
   C_FLAGS := $(C_FLAGS) -DI3
endif

ifdef MULTIPORT
   C_FILES := shell.c $(C_FILES)
   C_FLAGS := $(C_FLAGS) -DMULTIPORT
   #Sorry folks, this doesn't work in Cygwin
   ifndef WINDOZE
      C_FILES := mudmsg.c $(C_FILES)
   endif
endif

O_FILES := $(patsubst %.c,o/%.o,$(C_FILES))

H_FILES = $(wildcard *.h) 

ifdef WINDOZE
   AFKMUD = afkmud.exe
   RESOLVER = resolver.exe
else
   AFKMUD = afkmud
   RESOLVER = resolver
endif

all:
	$(MAKE) -s afkmud
	$(MAKE) -s resolver

# pull in dependency info for *existing* .o files
-include dependencies.d

afkmud: $(O_FILES)
	rm -f $(AFKMUD)
ifdef WINDOZE
	dlltool --export-all --output-def afkmud.def $(O_FILES)
	dlltool --dllname $(AFKMUD) --output-exp afkmud.exp --def afkmud.def
	$(CC) -o $(AFKMUD) $(O_FILES) afkmud.exp $(L_FLAGS)
else
	$(CC) -o $(AFKMUD) $(O_FILES) $(L_FLAGS)
endif
	echo "Generating dependency file ...";
	$(CC) -MM $(C_FLAGS) $(C_FILES) > dependencies.d
	perl -pi -e 's.^([a-z]).o/$$1.g' dependencies.d
	echo "Done compiling mud.";
	chmod g+w $(AFKMUD)
	chmod a+x $(AFKMUD)
	chmod g+w $(O_FILES)

resolver: resolver.o
	rm -f $(RESOLVER) 
	$(CC) $(D_FLAGS) -o $(RESOLVER) resolver.o
	echo "Done compiling resolver.";
	chmod g+w $(RESOLVER)
	chmod a+x $(RESOLVER)
	chmod g+w resolver.o

clean:  
	rm -f o/*.o $(AFKMUD) afkmud.def afkmud.exp core $(RESOLVER) resolver.o
	$(MAKE) all
        
purge:  
	rm -f o/*.o $(AFKMUD) afkmud.def afkmud.exp core $(RESOLVER) resolver.o dependencies.d

indent:
	indent -ts3 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -prs -bap -cbi0 -cli3 -bli0 -l125 -lp -i3 -cdb -c1 -cd1 -sc -pmt $(C_FILES)
	indent -ts3 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -prs -bap -cbi0 -cli3 -bli0 -l125 -lp -i3 -cdb -c1 -cd1 -sc -pmt $(H_FILES)

indentclean:
	rm *.c~ *.h~

o/%.o: %.c
	echo "  Compiling $@....";
	$(CC) -c $(C_FLAGS) $< -o $@

.c.o: mud.h
	$(CC) -c $(C_FLAGS) $<
