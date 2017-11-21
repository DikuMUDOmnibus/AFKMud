/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine, and Adjani.    *
 * All Rights Reserved.                                                     *
 * Registered with the United States Copyright Office. TX 5-877-286         *
 *                                                                          *
 * External contributions from Xorith, Quixadhal, Zarius, and many others.  *
 *                                                                          *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                         MUD Specific Definitions                         *
 ****************************************************************************/

/* These definitions can be safely changed without fear of being overwritten by future patches.
 * This does not guarantee however that compatibility will be maintained if you change
 * or remove too much of this stuff. Best to keep the names and only change the values they
 * represent if you want to spare yourself the hassle. Samson 3-14-2004
 */

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_LAYERS            8  /* maximum clothing layers */
#define MAX_NEST              100   /* maximum container nesting */
#define MAX_REXITS            20 /* Maximum exits allowed in 1 room */
#define MAX_SKILL             500   /* Raised during 1.4a patch */
#define SPELL_SILENT_MARKER   "silent" /* No OK. or Failed. */
#define MAX_CLASS             20
#define MAX_NPC_CLASS         26
#define MAX_RACE              20 /*  added 6 for new race code */
#define MAX_NPC_RACE          162   /* Good God almighty! If a race your looking for isn't available, you have problems!!! */
#define MAX_BEACONS           10
#define MAX_SAYHISTORY        25
#define MAX_TELLHISTORY       25

extern int MAX_PC_RACE;
extern int MAX_PC_CLASS;

#define MAX_LEVEL			115   /* Raised from 65 by Teklord */
#define MAX_CPD			4  /* Maximum council power level difference */
#define MAX_HERB			20

#define MAX_DISEASE           20
#define MAX_PERSONAL          5  /* Maximum personal skills */
#define MAX_WHERE_NAME        29 /* See act_info.c for the text messages */

#define LEVEL_SUPREME		   MAX_LEVEL
#define LEVEL_ADMIN		   (MAX_LEVEL - 1)
#define LEVEL_KL			   (MAX_LEVEL - 2)
#define LEVEL_IMPLEMENTOR	   (MAX_LEVEL - 3)
#define LEVEL_SUB_IMPLEM	   (MAX_LEVEL - 4)
#define LEVEL_ASCENDANT		   (MAX_LEVEL - 5)
#define LEVEL_GREATER		   (MAX_LEVEL - 6)
#define LEVEL_GOD		  	   (MAX_LEVEL - 7)
#define LEVEL_LESSER		   (MAX_LEVEL - 8)
#define LEVEL_TRUEIMM		   (MAX_LEVEL - 9)
#define LEVEL_DEMI		   (MAX_LEVEL - 10)
#define LEVEL_SAVIOR		   (MAX_LEVEL - 11)
#define LEVEL_CREATOR		   (MAX_LEVEL - 12)
#define LEVEL_ACOLYTE		   (MAX_LEVEL - 13)
#define LEVEL_IMMORTAL		   (MAX_LEVEL - 14)
#define LEVEL_AVATAR		   (MAX_LEVEL - 15)
#define LEVEL_HERO		   LEVEL_AVATAR

#define LEVEL_LOG		    	   LEVEL_LESSER
#define LEVEL_HIGOD		   LEVEL_GOD

/* Realm Globals */
/* --- Xorith -- */
#define REALM_IMP          5
#define REALM_HEAD_BUILDER 4
#define REALM_HEAD_CODER   3
#define REALM_CODE         2
#define REALM_BUILD        1

#ifdef MULTIPORT
/* Port definitions for various commands - Samson 8-22-98 */
/* Now only works if Multiport support is enabled at compile time - Samson 7-12-02 */
#define CODEPORT  9700
#define BUILDPORT 9600
#define MAINPORT  9500
#endif

/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 */
#define AREA_CONVERT_DIR "../areaconvert/"   /* Directory for manually converting areas */
#define PLAYER_DIR	"../player/"   /* Player files      */
#define GOD_DIR		"../gods/"  /* God Info Dir      */
#define BUILD_DIR       "../building/" /* Online building save dir */
#define SYSTEM_DIR	"../system/"   /* Main system files */
#define PROG_DIR		"../mudprogs/" /* MUDProg files     */
#define CORPSE_DIR	"../corpses/"  /* Corpses        */
#define CLASS_DIR		"../classes/"  /* Classes        */
#define RACE_DIR        "../races/"
#define MOTD_DIR        "../motd/"
#define COLOR_DIR       "../color/"
#define BOARD_DIR       "../boards/"   /* Board directory */
#define HOTBOOT_DIR     "../hotboot/"   /* For storing objects across hotboots */
#define MAP_DIR         "../maps/"

#define AREA_LIST		"area.lst"  /* List of areas     */
#define GOD_LIST		"gods.lst"  /* List of gods      */
#define CLASS_LIST	"class.lst" /* List of classes   */
#define RACE_LIST		"race.lst"  /* List of races     */
#define SHUTDOWN_FILE	"shutdown.txt" /* For 'shutdown' */
#define IMM_HOST_FILE   SYSTEM_DIR "immortal.host" /* For stoping hackers */
#define ANSITITLE_FILE	SYSTEM_DIR "mudtitle.ans"
#define BOOTLOG_FILE	SYSTEM_DIR "boot.txt"   /* Boot up error file  */
#define PBUG_FILE		SYSTEM_DIR "pbugs.txt"  /* For 'bug' command   */
#define IDEA_FILE		SYSTEM_DIR "ideas.txt"  /* For 'idea'       */
#define TYPO_FILE		SYSTEM_DIR "typos.txt"  /* For 'typo'       */
#define FIXED_FILE	SYSTEM_DIR "fixed.txt"  /* For 'fixed' command */
#define LOG_FILE		SYSTEM_DIR "log.txt" /* For talking in logged rooms */
#define MOBLOG_FILE	SYSTEM_DIR "moblog.txt" /* For mplog messages  */
#define WIZLIST_FILE	SYSTEM_DIR "WIZLIST" /* Wizlist       */
#define WEBWHO_FILE	SYSTEM_DIR "WEBWHO"  /* WWW Who output file */
#define SKILL_FILE	SYSTEM_DIR "skills.dat" /* Skill table         */
#define HERB_FILE		SYSTEM_DIR "herbs.dat"  /* Herb table       */
#define NAMEGEN_FILE	SYSTEM_DIR "namegen.txt"   /* Used for the name generator */
#define TONGUE_FILE	SYSTEM_DIR "tongues.dat"   /* Tongue tables    */
#define SOCIAL_FILE	SYSTEM_DIR "socials.dat"   /* Socials       */
#define COMMAND_FILE	SYSTEM_DIR "commands.dat"  /* Commands      */
#define WEBWIZ_FILE     SYSTEM_DIR "WEBWIZ"
#define MOTD_FILE       MOTD_DIR "motd.dat"
#define IMOTD_FILE      MOTD_DIR "imotd.dat"
#define SPEC_MOTD       MOTD_DIR "specmotd.dat" /* Special MOTD - cannot be ignored on login */

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
/* Added animate dead mobs - Whir - 8/29/98 */
#define MOB_VNUM_SUPERMOB                 11402
#define MOB_VNUM_ANIMATED_CORPSE          11403
#define MOB_VNUM_ANIMATED_SKELETON        11404
#define MOB_VNUM_ANIMATED_ZOMBIE          11405
#define MOB_VNUM_ANIMATED_GHOUL           11406
#define MOB_VNUM_ANIMATED_CRYPT_THING     11407
#define MOB_VNUM_ANIMATED_MUMMY           11408
#define MOB_VNUM_ANIMATED_GHOST           11409
#define MOB_VNUM_ANIMATED_DEATH_KNIGHT    11410
#define MOB_VNUM_ANIMATED_DRACOLICH       11411

#define MOB_VNUM_CITYGUARD                11074 /* Replaced original cityguard - Samson 3-26-98 */
#define MOB_VNUM_GATE                     11029 /* Gate spell servitor daemon - Samson 3-26-98 */
#define MOB_VNUM_CREEPINGDOOM             11412 /* Creeping Doom mob - Samson 9-27-98 */
#define MOB_VNUM_WARMOUNT                 11414 /* Paladin warmount - Samson 10-13-98 */
#define MOB_DOPPLEGANGER                  11413 /* Doppleganger base mob - Samson 10-11-99 */
#define MOB_VNUM_WARMOUNTTWO              11007 /* Antipaladin warmount - Samson 4-2-00 */
#define MOB_VNUM_WARMOUNTTHREE            11008 /* Paladin flying warmount - Samson 4-2-00 */
#define MOB_VNUM_WARMOUNTFOUR             11009 /* Antipaladin flying warmount - Samson 4-2-00 */

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_DUMMYOBJ           11000
#define OBJ_VNUM_FIREPIT            11001 /* Campfires turn into these when dead - Samson 8-10-98 */
#define OBJ_VNUM_CAMPFIRE           11002 /* Campfire that loads when a player camps - Samson 8-10-98 */
#define OBJ_VNUM_FIRESEED           11007 /* Fireseed object for spell_fireseed - Samson 10-13-98 */
#define OBJ_VNUM_MONEY_ONE          11401
#define OBJ_VNUM_MONEY_SOME         11402
#define OBJ_VNUM_CORPSE_NPC         11403
#define OBJ_VNUM_CORPSE_PC          11404
#define OBJ_VNUM_TREASURE           11044
#define OBJ_VNUM_RUNE               11045
#define OBJ_VNUM_SEVERED_HEAD       11405
#define OBJ_VNUM_TORN_HEART         11406
#define OBJ_VNUM_SLICED_ARM         11407
#define OBJ_VNUM_SLICED_LEG         11408
#define OBJ_VNUM_SPILLED_GUTS       11409
#define OBJ_VNUM_BRAINS             11435
#define OBJ_VNUM_HANDS              11436
#define OBJ_VNUM_FOOT               11437
#define OBJ_VNUM_FINGERS            11438
#define OBJ_VNUM_EAR                11439
#define OBJ_VNUM_EYE                11440
#define OBJ_VNUM_TONGUE             11441
#define OBJ_VNUM_EYESTALK           11442
#define OBJ_VNUM_TENTACLE           11443
#define OBJ_VNUM_FINS               11444
#define OBJ_VNUM_WINGS              11445
#define OBJ_VNUM_TAIL               11446
#define OBJ_VNUM_SCALES             11447
#define OBJ_VNUM_TUSKS              11448
#define OBJ_VNUM_HORNS              11449
#define OBJ_VNUM_CLAWS              11450
#define OBJ_VNUM_FEATHERS           11462
#define OBJ_VNUM_FORELEG            11463
#define OBJ_VNUM_PAWS               11464
#define OBJ_VNUM_HOOVES             11465
#define OBJ_VNUM_BEAK               11466
#define OBJ_VNUM_SHARPSCALE         11467
#define OBJ_VNUM_HAUNCHES           11468
#define OBJ_VNUM_FANGS              11469
#define OBJ_VNUM_BLOOD              11410
#define OBJ_VNUM_BLOODSTAIN         11411
#define OBJ_VNUM_SCRAPS             11412
#define OBJ_VNUM_MUSHROOM           11413
#define OBJ_VNUM_LIGHT_BALL         11414
#define OBJ_VNUM_SPRING             11415
#define OBJ_VNUM_SLICE              11417
#define OBJ_VNUM_SHOPPING_BAG       11418
#define OBJ_VNUM_FIRE               11423
#define OBJ_VNUM_TRAP               11424
#define OBJ_VNUM_PORTAL             11425
#define OBJ_VNUM_BLACK_POWDER       11426
#define OBJ_VNUM_SCROLL_SCRIBING    11427
#define OBJ_VNUM_FLASK_BREWING      11428
#define OBJ_VNUM_NOTE               11429
#define OBJ_VNUM_WAND_CHARGING      11432
#define OBJ_VNUM_TAN_JACKET         11368
#define OBJ_VNUM_TAN_BOOTS          11369
#define OBJ_VNUM_TAN_GLOVES         11370
#define OBJ_VNUM_TAN_LEGGINGS       11371
#define OBJ_VNUM_TAN_SLEEVES        11372
#define OBJ_VNUM_TAN_HELMET         11373
#define OBJ_VNUM_TAN_BAG            11374
#define OBJ_VNUM_TAN_CLOAK          11375
#define OBJ_VNUM_TAN_BELT           11376
#define OBJ_VNUM_TAN_COLLAR         11377
#define OBJ_VNUM_TAN_WATERSKIN      11378
#define OBJ_VNUM_TAN_QUIVER         11379
#define OBJ_VNUM_TAN_WHIP           11380
#define OBJ_VNUM_TAN_SHIELD         11381

/* Academy eq */
#define OBJ_VNUM_SCHOOL_BANNER      11478
#define OBJ_VNUM_NEWBIE_GUIDE       11479

/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO             11401
#define ROOM_VNUM_POLY              11402
#define ROOM_VNUM_CHAT              11406 /* Parlour of the Immortals */
#define ROOM_NOAUTH_START           102   /* Pregame Entry, auth system off */
#define ROOM_AUTH_START	     100 /* Pregame Entry, auth system on */
#define ROOM_VNUM_TEMPLE            11407 /* Green Dragon Inn - Main continent recall point */
#define ROOM_VNUM_ALTAR             11407 /* Bywater Temple - Where one goes to die */
#define ROOM_VNUM_ELETAR_RECALL     11408 /* Eletar continent recall point */
#define ROOM_VNUM_ELETAR_DEATH      11408 /* Eletar continent death recall */
#define ROOM_VNUM_ALATIA_RECALL     11409 /* Alatia continent recall point */
#define ROOM_VNUM_ALATIA_DEATH      11409 /* Alatia continent death recall */
#define ROOM_VNUM_ASTRAL_DEATH      11407 /* Astral Plane, death sends you to Bywater */
#define ROOM_VNUM_PAST_RECALL       25301 /* Distant past recall point */


#define ROOM_VNUM_RENTUPDATE        11402 /* Vnum used for checking rent on players - Samson 1-24-00 */
#define ROOM_VNUM_HELL              11405 /* Vnum for Hell - Samson */
#define ROOM_VNUM_DONATION          11410 /* First donation room - Samson 2-6-98 */
#define ROOM_VNUM_REDEEM            11411 /* Sindhae prize redemption start room - Samson 6-2-00 */
#define ROOM_VNUM_ENDREDEEM         11412 /* Sindhae prize redemption ending room - Samson 6-2-00 */

/* New continent and plane support - Samson 3-28-98
 * Name of continent or plane is followed by the recall and death zone.
 * Area continent flags for continent and plane system, revised format - Samson 8-8-98
 */
typedef enum
{
   ACON_ALSHEROK, ACON_ELETAR, ACON_ALATIA, ACON_UNUSED1, ACON_UNUSED2,
   ACON_UNUSED3, ACON_UNUSED4, ACON_UNUSED5, ACON_ASTRAL, ACON_PAST,
   ACON_IMMORTAL, ACON_VARSIS, ACON_MAX
} acon_types;

/* ACON_ALSHEROK	Alsherok continent, Bywater */
/* ACON_ELETAR	Eletar continent, Dragon Gate Village */
/* ACON_ALATIA	Alatia continent, Venetorium Colony */
/* ACON_ASTRAL	Astral Plane, Astral entry point - death = Bywater */
/* ACON_PAST      The distant past, Entry to Land of the Lost - death = Bywater */
/* ACON_IMMORTAL  Immortal zone, no recall for mortals */
/* ACON_VARSIS    The Immortal challenge area */

/* the races */
/* If you add a new race to this table, make sure you update update_aris in handler.c as well */
typedef enum
{
   RACE_HUMAN, RACE_HIGH_ELF, RACE_DWARF, RACE_HALFLING, RACE_PIXIE,
   RACE_HALF_OGRE, RACE_HALF_ORC, RACE_HALF_TROLL, RACE_HALF_ELF, RACE_GITH,
   RACE_MINOTAUR, RACE_DUERGAR, RACE_CENTAUR, RACE_IGUANADON,
   RACE_GNOME, RACE_DROW, RACE_WILD_ELF, RACE_INSECTOID, RACE_SAHUAGIN, RACE_19
} race_types;

/* NPC Races */
#define RACE_HALFBREED 20
#define RACE_REPTILE  21
#define RACE_SPECIAL  22
#define RACE_LYCANTH  23
#define RACE_DRAGON   24
#define RACE_UNDEAD   25
#define RACE_ORC      26
#define RACE_INSECT   27
#define RACE_ARACHNID 28
#define RACE_DINOSAUR 29
#define RACE_FISH     30
#define RACE_BIRD     31
#define RACE_GIANT    32   /* generic giant more specials down ---V */
#define RACE_PREDATOR 33
#define RACE_PARASITE 34
#define RACE_SLIME    35
#define RACE_DEMON    36
#define RACE_SNAKE    37
#define RACE_HERBIV   38
#define RACE_TREE     39
#define RACE_VEGGIE   40
#define RACE_ELEMENT  41
#define RACE_PLANAR   42
#define RACE_DEVIL    43
#define RACE_GHOST    44
#define RACE_GOBLIN   45
#define RACE_TROLL    46
#define RACE_VEGMAN   47
#define RACE_MFLAYER  48
#define RACE_PRIMATE  49
#define RACE_ENFAN    50
#define RACE_GOLEM    51
#define RACE_SKEXIE   52
#define RACE_TROGMAN  53
#define RACE_PATRYN   54
#define RACE_LABRAT   55
#define RACE_SARTAN   56
#define RACE_TYTAN    57
#define RACE_SMURF    58
#define RACE_ROO      59
#define RACE_HORSE    60
#define RACE_DRAAGDIM 61
#define RACE_ASTRAL   62
#define RACE_GOD      63

#define RACE_GIANT_HILL   64
#define RACE_GIANT_FROST  65
#define RACE_GIANT_FIRE   66
#define RACE_GIANT_CLOUD  67
#define RACE_GIANT_STORM  68
#define RACE_GIANT_STONE  69

#define RACE_DRAGON_RED    70
#define RACE_DRAGON_BLACK  71
#define RACE_DRAGON_GREEN  72
#define RACE_DRAGON_WHITE  73
#define RACE_DRAGON_BLUE   74
#define RACE_DRAGON_SILVER 75
#define RACE_DRAGON_GOLD   76
#define RACE_DRAGON_BRONZE 77
#define RACE_DRAGON_COPPER 78
#define RACE_DRAGON_BRASS  79

#define RACE_VAMPIRE		80
#define RACE_UNDEAD_VAMPIRE	80
#define RACE_LICH             81
#define RACE_UNDEAD_LICH	81
#define RACE_WIGHT		82
#define RACE_UNDEAD_WIGHT	82
#define RACE_GHAST		83
#define RACE_UNDEAD_GHAST	83
#define RACE_SPECTRE		84
#define RACE_UNDEAD_SPECTRE	84
#define RACE_ZOMBIE		85
#define RACE_UNDEAD_ZOMBIE	85
#define RACE_SKELETON		86
#define RACE_UNDEAD_SKELETON	86
#define RACE_GHOUL		87
#define RACE_UNDEAD_GHOUL	87

#define RACE_HALF_GIANT   88
#define RACE_DEEP_GNOME 89
#define RACE_GNOLL	90

#define RACE_GOLD_ELF	91
#define RACE_GOLD_ELVEN	91
#define RACE_SEA_ELF	92
#define RACE_SEA_ELVEN	92

/* 10-20-96 Admiral */
#define RACE_TIEFLING   93
#define RACE_AASIMAR    94
#define RACE_SOLAR      95
#define RACE_PLANITAR   96
#define RACE_UNDEAD_SHADOW  97
#define RACE_GIANT_SKELETON 98
#define RACE_NILBOG         99
#define RACE_HOUSERS        100
#define RACE_BAKU           101
#define RACE_BEASTLORD      102
#define RACE_DEVAS          103
#define RACE_POLARIS        104
#define RACE_DEMODAND       105
#define RACE_TARASQUE       106
#define RACE_DIETY          107
#define RACE_DAEMON         108
#define RACE_VAGABOND       109

/* Imported races from old Alsherok code - Samson */
#define RACE_GARGOYLE       110
#define RACE_BEAR		    111
#define RACE_BAT            112
#define RACE_CAT            113
#define RACE_DOG            114
#define RACE_ANT            115
#define RACE_APE            116
#define RACE_BABOON         117
#define RACE_BEE            118
#define RACE_BEETLE         119
#define RACE_BOAR           120
#define RACE_BUGBEAR        121
#define RACE_FERRET         122
#define RACE_FLY            123
#define RACE_GELATIN        124
#define RACE_GORGON         125
#define RACE_HARPY          126
#define RACE_HOBGOBLIN      127
#define RACE_KOBOLD         128
#define RACE_LOCUST         129
#define RACE_MOLD           130
#define RACE_MULE           131
#define RACE_NEANDERTHAL    132
#define RACE_OOZE           133
#define RACE_RAT            134
#define RACE_RUSTMONSTER    135
#define RACE_SHAPESHIFTER   136
#define RACE_SHREW          137
#define RACE_SHRIEKER       138
#define RACE_STIRGE         139
#define RACE_THOUL          140
#define RACE_WOLF           141
#define RACE_WORM           142
#define RACE_BOVINE         143
#define RACE_CANINE         144
#define RACE_FELINE         145
#define RACE_PORCINE        146
#define RACE_MAMMAL         147
#define RACE_RODENT         148
#define RACE_AMPHIBIAN      149
#define RACE_CRUSTACEAN     150
#define RACE_SPIRIT         151
#define RACE_MAGICAL        152
#define RACE_ANIMAL         153
#define RACE_HUMANOID       154
#define RACE_MONSTER        155
#define RACE_UNUSED1        156
#define RACE_UNUSED2        157
#define RACE_UNUSED3        158
#define RACE_UNUSED4        159
#define RACE_UNUSED5        160
#define RACE_UNUSED6        161

#define CLASS_NONE	   -1 /* For skill/spells according to guild */

/* If you add a new class to this, make sure you update update_aris in handler.c as well */
typedef enum
{
   CLASS_MAGE, CLASS_CLERIC, CLASS_ROGUE, CLASS_WARRIOR, CLASS_NECROMANCER,
   CLASS_DRUID, CLASS_RANGER, CLASS_MONK, CLASS_AVAILABLE, CLASS_AVAILABLE2,
   CLASS_ANTIPALADIN, CLASS_PALADIN, CLASS_BARD
} class_types;

/*
 * Languages -- Altrag
 */
#define LANG_COMMON      BV00 /* Human base language */
#define LANG_ELVEN       BV01 /* Elven base language */
#define LANG_DWARVEN     BV02 /* Dwarven base language */
#define LANG_PIXIE       BV03 /* Pixie/Fairy base language */
#define LANG_OGRE        BV04 /* Ogre base language */
#define LANG_ORCISH      BV05 /* Orc base language */
#define LANG_TROLLISH    BV06 /* Troll base language */
#define LANG_RODENT      BV07 /* Small mammals */
#define LANG_INSECTOID   BV08 /* Insects */
#define LANG_MAMMAL      BV09 /* Larger mammals */
#define LANG_REPTILE     BV10 /* Small reptiles */
#define LANG_DRAGON      BV11 /* Large reptiles, Dragons */
#define LANG_SPIRITUAL   BV12 /* Necromancers or undeads/spectres */
#define LANG_MAGICAL     BV13 /* Spells maybe?  Magical creatures */
#define LANG_GOBLIN      BV14 /* Goblin base language */
#define LANG_GOD         BV15 /* Clerics possibly?  God creatures */
#define LANG_ANCIENT     BV16 /* Prelude to a glyph read skill? */
#define LANG_HALFLING    BV17 /* Halfling base language */
#define LANG_CLAN	       BV18 /* Clan language */
#define LANG_GITH		 BV19 /* Gith Language */
#define LANG_MINOTAUR	 BV20 /* Minotaur language - Samson 8-2-98 */
#define LANG_CENTAUR	 BV21 /* Centaur language - Samson 8-2-98 */
#define LANG_GNOME	 BV22 /* Gnome language - Samson 8-6-98 */
#define LANG_SAHUAGIN	 BV23 /* Sahuagin language - Samson 2-8-02 */
#define LANG_UNKNOWN        0 /* Anything that doesnt fit a category */
#define VALID_LANGS    ( LANG_COMMON | LANG_ELVEN | LANG_DWARVEN | LANG_PIXIE  \
		       | LANG_OGRE | LANG_ORCISH | LANG_TROLLISH | LANG_GOBLIN \
		       | LANG_HALFLING | LANG_GITH | LANG_MINOTAUR | LANG_CENTAUR | LANG_GNOME \
			 | LANG_REPTILE | LANG_INSECTOID | LANG_SAHUAGIN )
/* 24 Languages */
