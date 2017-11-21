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
 *                          Oasis OLC Module                                *
 ****************************************************************************/

/****************************************************************************
 * ResortMUD 4.0 Beta by Ntanel, Garinan, Badastaz, Josh, Digifuzz, Senir,  *
 * Kratas, Scion, Shogar and Tagith.  Special thanks to Thoric, Nivek,      *
 * Altrag, Arlorn, Justice, Samson, Dace, HyperEye and Yakkov.              *
 ****************************************************************************
 * Copyright (C) 1996 - 2001 Haslage Net Electronics: MudWorld              *
 * of Lorain, Ohio - ALL RIGHTS RESERVED                                    *
 * The text and pictures of this publication, or any part thereof, may not  *
 * be reproduced or transmitted in any form or by any means, electronic or  *
 * mechanical, includes photocopying, recording, storage in a information   *
 * retrieval system, or otherwise, without the prior written or e-mail      *
 * consent from the publisher.                                              *
 ****************************************************************************
 * GREETING must mention ResortMUD programmers and the help file named      *
 * CREDITS must remain completely intact as listed in the SMAUG license.    *
 ****************************************************************************/

/**************************************************************************\
 *     OasisOLC II for Smaug 1.40 written by Evan Cortens(Tagith)         *
 *   Based on OasisOLC for CircleMUD3.0bpl9 written by Harvey Gilpin      *
 **************************************************************************
 *                         Defines, structs, etc.. v1.0                   *
\**************************************************************************/

typedef enum
{
   WEP_BAREHAND, WEP_SWORD, WEP_DAGGER, WEP_WHIP, WEP_TALON, WEP_MACE,
   WEP_ARCHERY, WEP_BLOWGUN, WEP_SLING, WEP_AXE, WEP_SPEAR, WEP_STAFF,
   WEP_MAX
} weapon_types;

typedef enum
{
   PROJ_BOLT, PROJ_ARROW, PROJ_DART, PROJ_STONE, PROJ_MAX
} projectile_types;

typedef enum
{
   GEAR_NONE, GEAR_BED, GEAR_MISC, GEAR_FIRE, GEAR_MAX
} campgear_types;

typedef enum
{
   ORE_NONE, ORE_IRON, ORE_GOLD, ORE_SILVER, ORE_ADAM, ORE_MITH, ORE_BLACK,
   ORE_TITANIUM, ORE_STEEL, ORE_BRONZE, ORE_DWARVEN, ORE_ELVEN, ORE_MAX
} ore_types;

typedef enum
{
   SEX_NEUTRAL, SEX_MALE, SEX_FEMALE, SEX_HERMAPHRODYTE, SEX_MAX
} sex_types;

typedef enum
{
   POS_DEAD, POS_MORTAL, POS_INCAP, POS_STUNNED, POS_SLEEPING,
   POS_RESTING, POS_SITTING, POS_BERSERK, POS_AGGRESSIVE, POS_FIGHTING,
   POS_DEFENSIVE, POS_EVASIVE, POS_STANDING, POS_MOUNTED,
   POS_SHOVE, POS_DRAG, POS_MAX
} positions;

/*
 * Directions.
 * Used in #ROOMS.
 */
typedef enum
{
   DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_UP, DIR_DOWN,
   DIR_NORTHEAST, DIR_NORTHWEST, DIR_SOUTHEAST, DIR_SOUTHWEST, DIR_SOMEWHERE
} dir_types;

#define MAX_DIR			DIR_SOUTHWEST  /* max for normal walking */
#define DIR_PORTAL		DIR_SOMEWHERE  /* portal direction    */

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
/* Don't forget to add the flag to build.c!!! */
/* current # of flags: 54 */
typedef enum
{
   ACT_IS_NPC, ACT_SENTINEL, ACT_SCAVENGER, ACT_INNKEEPER, ACT_BANKER,
   ACT_AGGRESSIVE, ACT_STAY_AREA, ACT_WIMPY, ACT_PET, ACT_AUTONOMOUS,
   ACT_PRACTICE, ACT_IMMORTAL, ACT_DEADLY, ACT_POLYSELF, ACT_META_AGGR,
   ACT_GUARDIAN, ACT_BOARDED, ACT_NOWANDER, ACT_MOUNTABLE, ACT_MOUNTED,
   ACT_SCHOLAR, ACT_SECRETIVE, ACT_HARDHAT, ACT_MOBINVIS, ACT_NOASSIST,
   ACT_ILLUSION, ACT_PACIFIST, ACT_NOATTACK, ACT_ANNOYING, ACT_AUCTION,
   ACT_PROTOTYPE,
   ACT_MAGE, ACT_WARRIOR, ACT_CLERIC, ACT_ROGUE, ACT_DRUID, ACT_MONK,
   ACT_PALADIN, ACT_RANGER, ACT_NECROMANCER, ACT_ANTIPALADIN, ACT_HUGE,
   ACT_GREET, ACT_TEACHER, ACT_ONMAP, ACT_SMITH, ACT_GUILDAUC, ACT_GUILDBANK,
   ACT_GUILDVENDOR, ACT_GUILDREPAIR, ACT_GUILDFORGE, ACT_IDMOB,
   ACT_GUILDIDMOB, MAX_ACT_FLAG
} mob_flags;

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 *
 * hold and flaming are yet uncoded
 */
/* Current # of bits: 60 */
/* Don't forget to add the flag to build.c, and the affect_bit_name function in handler.c */
typedef enum
{
   AFF_NONE, AFF_BLIND, AFF_INVISIBLE, AFF_DETECT_EVIL, AFF_DETECT_INVIS,
   AFF_DETECT_MAGIC, AFF_DETECT_HIDDEN, AFF_HOLD, AFF_SANCTUARY,
   AFF_FAERIE_FIRE, AFF_INFRARED, AFF_CURSE, AFF_SPY, AFF_POISON,
   AFF_PROTECT, AFF_PARALYSIS, AFF_SNEAK, AFF_HIDE, AFF_SLEEP, AFF_CHARM,
   AFF_FLYING, AFF_ACIDMIST, AFF_FLOATING, AFF_TRUESIGHT, AFF_DETECTTRAPS,
   AFF_SCRYING, AFF_FIRESHIELD, AFF_SHOCKSHIELD, AFF_VENOMSHIELD, AFF_ICESHIELD,
   AFF_WIZARDEYE, AFF_BERSERK, AFF_AQUA_BREATH, AFF_RECURRINGSPELL,
   AFF_CONTAGIOUS, AFF_BLADEBARRIER, AFF_SILENCE, AFF_ANIMAL_INVIS,
   AFF_HEAT_STUFF, AFF_LIFE_PROT, AFF_DRAGON_RIDE, AFF_GROWTH, AFF_TREE_TRAVEL,
   AFF_TRAVELLING, AFF_TELEPATHY, AFF_ETHEREAL, AFF_PASS_DOOR, AFF_QUIV,
   AFF_FLAMING, AFF_HASTE, AFF_SLOW, AFF_ELVENSONG, AFF_BLADESONG,
   AFF_REVERIE, AFF_TENACITY, AFF_DEATHSONG, AFF_POSSESS, AFF_NOTRACK, AFF_ENLIGHTEN,
   AFF_TREETALK, AFF_SPAMGUARD, AFF_BASH, MAX_AFFECTED_BY
} affected_by_types;

/*
 * Item types.
 * Used in #OBJECTS.
 */
/* Don't forget to add the flag to build.c!!! */
/* also don't forget to add new item types to the find_oftype function in afk.c */
/* Current # of types: 68 */
typedef enum
{
   ITEM_NONE, ITEM_LIGHT, ITEM_SCROLL, ITEM_WAND, ITEM_STAFF, ITEM_WEAPON,
   ITEM_unused1, ITEM_unused2, ITEM_TREASURE, ITEM_ARMOR, ITEM_POTION,
   ITEM_CLOTHING, ITEM_FURNITURE, ITEM_TRASH, ITEM_unused4, ITEM_CONTAINER,
   ITEM_unused5, ITEM_DRINK_CON, ITEM_KEY, ITEM_FOOD, ITEM_MONEY, ITEM_PEN,
   ITEM_BOAT, ITEM_CORPSE_NPC, ITEM_CORPSE_PC, ITEM_FOUNTAIN, ITEM_PILL,
   ITEM_BLOOD, ITEM_BLOODSTAIN, ITEM_SCRAPS, ITEM_PIPE, ITEM_HERB_CON,
   ITEM_HERB, ITEM_INCENSE, ITEM_FIRE, ITEM_BOOK, ITEM_SWITCH, ITEM_LEVER,
   ITEM_PULLCHAIN, ITEM_BUTTON, ITEM_DIAL, ITEM_RUNE, ITEM_RUNEPOUCH,
   ITEM_MATCH, ITEM_TRAP, ITEM_MAP, ITEM_PORTAL, ITEM_PAPER,
   ITEM_TINDER, ITEM_LOCKPICK, ITEM_SPIKE, ITEM_DISEASE, ITEM_OIL, ITEM_FUEL,
   ITEM_PIECE, ITEM_TREE, ITEM_MISSILE_WEAPON, ITEM_PROJECTILE, ITEM_QUIVER,
   ITEM_SHOVEL, ITEM_SALVE, ITEM_COOK, ITEM_KEYRING, ITEM_ODOR, ITEM_CAMPGEAR,
   ITEM_DRINK_MIX, ITEM_INSTRUMENT, ITEM_ORE, MAX_ITEM_TYPE
} item_types;

/*
 * Extra flags.
 * Used in #OBJECTS. Rearranged for better compatibility with Shard areas - Samson
 */
/* Don't forget to add the flag to o_flags in build.c and handler.c!!! */
/* Current # of flags: 65 */
typedef enum
{
   ITEM_GLOW, ITEM_HUM, ITEM_METAL, ITEM_MINERAL, ITEM_ORGANIC, ITEM_INVIS, ITEM_MAGIC,
   ITEM_NODROP, ITEM_BLESS, ITEM_ANTI_GOOD, ITEM_ANTI_EVIL, ITEM_ANTI_NEUTRAL,
   ITEM_ANTI_CLERIC, ITEM_ANTI_MAGE, ITEM_ANTI_ROGUE, ITEM_ANTI_WARRIOR,
   ITEM_INVENTORY, ITEM_NOREMOVE, ITEM_TWOHAND, ITEM_EVIL, ITEM_DONATION,
   ITEM_CLANOBJECT, ITEM_CLANCORPSE, ITEM_ANTI_BARD, ITEM_HIDDEN,
   ITEM_ANTI_DRUID, ITEM_POISONED, ITEM_COVERING, ITEM_DEATHROT, ITEM_BURIED,
   ITEM_PROTOTYPE, ITEM_NOLOCATE, ITEM_GROUNDROT, ITEM_ANTI_MONK, ITEM_LOYAL,
   ITEM_BRITTLE, ITEM_RESISTANT, ITEM_IMMUNE, ITEM_ANTI_MEN, ITEM_ANTI_WOMEN,
   ITEM_ANTI_NEUTER, ITEM_ANTI_HERMA, ITEM_ANTI_SUN, ITEM_ANTI_RANGER, ITEM_ANTI_PALADIN,
   ITEM_ANTI_NECRO, ITEM_ANTI_APAL, ITEM_ONLY_CLERIC, ITEM_ONLY_MAGE,
   ITEM_ONLY_ROGUE, ITEM_ONLY_WARRIOR, ITEM_ONLY_BARD, ITEM_ONLY_DRUID,
   ITEM_ONLY_MONK, ITEM_ONLY_RANGER, ITEM_ONLY_PALADIN, ITEM_ONLY_NECRO,
   ITEM_ONLY_APAL, ITEM_AUCTION, ITEM_ONMAP, ITEM_PERSONAL, ITEM_LODGED,
   ITEM_SINDHAE, ITEM_MUSTMOUNT, ITEM_NOAUCTION, MAX_ITEM_FLAG
} item_extra_flags;

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE			BV00
#define ITEM_WEAR_FINGER	BV01
#define ITEM_WEAR_NECK		BV02
#define ITEM_WEAR_BODY		BV03
#define ITEM_WEAR_HEAD		BV04
#define ITEM_WEAR_LEGS		BV05
#define ITEM_WEAR_FEET		BV06
#define ITEM_WEAR_HANDS		BV07
#define ITEM_WEAR_ARMS		BV08
#define ITEM_WEAR_SHIELD	BV09
#define ITEM_WEAR_ABOUT		BV10
#define ITEM_WEAR_WAIST		BV11
#define ITEM_WEAR_WRIST		BV12
#define ITEM_WIELD		BV13
#define ITEM_HOLD			BV14
#define ITEM_DUAL_WIELD		BV15
#define ITEM_WEAR_EARS		BV16
#define ITEM_WEAR_EYES		BV17
#define ITEM_MISSILE_WIELD	BV18
#define ITEM_WEAR_BACK		BV19
#define ITEM_WEAR_FACE		BV20
#define ITEM_WEAR_ANKLE		BV21
#define ITEM_LODGE_RIB		BV22
#define ITEM_LODGE_ARM		BV23
#define ITEM_LODGE_LEG		BV24
#define MAX_WEAR_FLAG		24

/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
typedef enum
{
   WEAR_NONE = -1, WEAR_LIGHT = 0, WEAR_FINGER_L, WEAR_FINGER_R, WEAR_NECK_1,
   WEAR_NECK_2, WEAR_BODY, WEAR_HEAD, WEAR_LEGS, WEAR_FEET, WEAR_HANDS,
   WEAR_ARMS, WEAR_SHIELD, WEAR_ABOUT, WEAR_WAIST, WEAR_WRIST_L, WEAR_WRIST_R,
   WEAR_WIELD, WEAR_HOLD, WEAR_DUAL_WIELD, WEAR_EARS, WEAR_EYES,
   WEAR_MISSILE_WIELD, WEAR_BACK, WEAR_FACE, WEAR_ANKLE_L, WEAR_ANKLE_R,
   WEAR_LODGE_RIB, WEAR_LODGE_ARM, WEAR_LODGE_LEG, MAX_WEAR
} wear_locations;

/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
typedef enum
{
   APPLY_NONE, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_WIS, APPLY_CON,
   APPLY_SEX, APPLY_CLASS, APPLY_LEVEL, APPLY_AGE, APPLY_HEIGHT, APPLY_WEIGHT,
   APPLY_MANA, APPLY_HIT, APPLY_MOVE, APPLY_GOLD, APPLY_EXP, APPLY_AC,
   APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_POISON, APPLY_SAVING_ROD,
   APPLY_SAVING_PARA, APPLY_SAVING_BREATH, APPLY_SAVING_SPELL, APPLY_CHA,
   APPLY_AFFECT, APPLY_RESISTANT, APPLY_IMMUNE, APPLY_SUSCEPTIBLE,
   APPLY_WEAPONSPELL, APPLY_LCK, APPLY_BACKSTAB, APPLY_PICK, APPLY_TRACK,
   APPLY_STEAL, APPLY_SNEAK, APPLY_HIDE, APPLY_PALM, APPLY_DETRAP, APPLY_DODGE,
   APPLY_SF, APPLY_SCAN, APPLY_GOUGE, APPLY_SEARCH, APPLY_MOUNT, APPLY_DISARM,
   APPLY_KICK, APPLY_PARRY, APPLY_BASH, APPLY_STUN, APPLY_PUNCH, APPLY_CLIMB,
   APPLY_GRIP, APPLY_SCRIBE, APPLY_BREW, APPLY_WEARSPELL, APPLY_REMOVESPELL,
   APPLY_unused, APPLY_MENTALSTATE, APPLY_STRIPSN, APPLY_REMOVE, APPLY_DIG,
   APPLY_FULL, APPLY_THIRST, APPLY_DRUNK, APPLY_HIT_REGEN,
   APPLY_MANA_REGEN, APPLY_MOVE_REGEN, APPLY_ANTIMAGIC,
   APPLY_ROOMFLAG, APPLY_SECTORTYPE, APPLY_ROOMLIGHT, APPLY_TELEVNUM,
   APPLY_TELEDELAY, APPLY_COOK, APPLY_RECURRINGSPELL, APPLY_RACE, APPLY_HITNDAM,
   APPLY_SAVING_ALL, APPLY_EAT_SPELL, APPLY_RACE_SLAYER, APPLY_ALIGN_SLAYER,
   APPLY_CONTAGIOUS, APPLY_EXT_AFFECT, APPLY_ODOR, APPLY_PEEK, APPLY_ABSORB,
   APPLY_ATTACKS, APPLY_EXTRAGOLD, APPLY_ALLSTATS, MAX_APPLY_TYPE
} apply_types;

#define REVERSE_APPLY		   1000

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE		   BV00
#define CONT_PICKPROOF		   BV01
#define CONT_CLOSED		   BV02
#define CONT_LOCKED		   BV03
#define CONT_EATKEY		   BV04

#define MAX_CONT_FLAG 5 /* This needs to be equal to the number of container flags for the OLC menu editor */

/* Magic flags - extra extra_flags for objects that are used in spells */
#define ITEM_RETURNING		BV00
#define ITEM_BACKSTABBER  	BV01
#define ITEM_BANE		BV02
#define ITEM_MAGIC_LOYAL	BV03
#define ITEM_HASTE		BV04
#define ITEM_DRAIN		BV05
#define ITEM_LIGHTNING_BLADE  	BV06
#define MAX_MFLAG 6

/* Lever/dial/switch/button/pullchain flags */
#define TRIG_UP			BV00
#define TRIG_UNLOCK		BV01
#define TRIG_LOCK			BV02
#define TRIG_D_NORTH		BV03
#define TRIG_D_SOUTH		BV04
#define TRIG_D_EAST		BV05
#define TRIG_D_WEST		BV06
#define TRIG_D_UP			BV07
#define TRIG_D_DOWN		BV08
#define TRIG_DOOR			BV09
#define TRIG_CONTAINER		BV10
#define TRIG_OPEN			BV11
#define TRIG_CLOSE		BV12
#define TRIG_PASSAGE		BV13
#define TRIG_OLOAD		BV14
#define TRIG_MLOAD		BV15
#define TRIG_TELEPORT		BV16
#define TRIG_TELEPORTALL	BV17
#define TRIG_TELEPORTPLUS	BV18
#define TRIG_DEATH		BV19
#define TRIG_CAST			BV20
#define TRIG_FAKEBLADE		BV21
#define TRIG_RAND4		BV22
#define TRIG_RAND6		BV23
#define TRIG_TRAPDOOR		BV24
#define TRIG_ANOTHEROOM		BV25
#define TRIG_USEDIAL		BV26
#define TRIG_ABSOLUTEVNUM	BV27
#define TRIG_SHOWROOMDESC	BV28
#define TRIG_AUTORETURN		BV29

#define MAX_TRIGFLAG 30 /* Make equal to the number of trigger flags for OLC menu editor */

#define TELE_SHOWDESC		BV00
#define TELE_TRANSALL		BV01
#define TELE_TRANSALLPLUS	BV02

/*
 * Sitting/Standing/Sleeping/Sitting on/in/at Objects - Xerves
 * Used for furniture (value[2]) in the #OBJECTS Section
 */
#define SIT_ON     BV00
#define SIT_IN     BV01
#define SIT_AT     BV02

#define STAND_ON   BV03
#define STAND_IN   BV04
#define STAND_AT   BV05

#define SLEEP_ON   BV06
#define SLEEP_IN   BV07
#define SLEEP_AT   BV08

#define REST_ON     BV09
#define REST_IN     BV10
#define REST_AT     BV11

#define MAX_FURNFLAG 12

/*
 * Room flags.           Holy cow!  Talked about stripped away..
 * Used in #ROOMS.       Those merc guys know how to strip code down.
 *			 Lets put it all back... ;)
 */
/* Roomflags converted to Extended BV - Samson 8-11-98 */
/* NOTE: If this list grows to 65, raise BFS_MARK in track.c - Samson */
/* Don't forget to add the flag to build.c!!! */
/* Current # of flags: 41 */
typedef enum
{
   ROOM_DARK, ROOM_DEATH, ROOM_NO_MOB, ROOM_INDOORS, ROOM_SAFE, ROOM_NOCAMP,
   ROOM_NO_SUMMON, ROOM_NO_MAGIC, ROOM_TUNNEL, ROOM_PRIVATE, ROOM_SILENCE,
   ROOM_NOSUPPLICATE, ROOM_ARENA, ROOM_NOMISSILE, ROOM_NO_RECALL, ROOM_NO_PORTAL,
   ROOM_NO_ASTRAL, ROOM_NODROP, ROOM_CLANSTOREROOM, ROOM_TELEPORT, ROOM_TELESHOWDESC,
   ROOM_NOFLOOR, ROOM_SOLITARY, ROOM_PET_SHOP, ROOM_DONATION, ROOM_NODROPALL,
   ROOM_LOGSPEECH, ROOM_PROTOTYPE, ROOM_NOTELEPORT, ROOM_NOSCRY, ROOM_CAVE,
   ROOM_CAVERN, ROOM_NOBEACON, ROOM_AUCTION, ROOM_MAP, ROOM_FORGE, ROOM_GUILDINN,
   ROOM_ISOLATED, ROOM_WATCHTOWER, ROOM_NOQUIT, ROOM_TELENOFLY, ROOM_MAX
} room_flags;

/*
 * Exit flags.			EX_RES# are reserved for use by the
 * Used in #ROOMS.		SMAUG development team ( No RES flags left, sorry. - Samson )
 */
/* Converted to Extended BV - Samson 7-23-00 */
/* Don't forget to add the flag to build.c!!! */
/* Current # of flags: 35 */
typedef enum
{
   EX_ISDOOR, EX_CLOSED, EX_LOCKED, EX_SECRET, EX_SWIM, EX_PICKPROOF, EX_FLY,
   EX_CLIMB, EX_DIG, EX_EATKEY, EX_NOPASSDOOR, EX_HIDDEN, EX_PASSAGE, EX_PORTAL,
   EX_OVERLAND, EX_ASLIT, EX_xCLIMB, EX_xENTER, EX_xLEAVE, EX_xAUTO, EX_NOFLEE,
   EX_xSEARCHABLE, EX_BASHED, EX_BASHPROOF, EX_NOMOB, EX_WINDOW, EX_xLOOK,
   EX_ISBOLT, EX_BOLTED, EX_FORTIFIED, EX_HEAVY, EX_MEDIUM, EX_LIGHT, EX_CRUMBLING,
   EX_DESTROYED, MAX_EXFLAG
} exit_flags;

/*
 * Sector types.
 * Used in #ROOMS and on Overland maps.
 */
/* Current number of types: 34 */
typedef enum
{
   SECT_INDOORS, SECT_CITY, SECT_FIELD, SECT_FOREST, SECT_HILLS, SECT_MOUNTAIN,
   SECT_WATER_SWIM, SECT_WATER_NOSWIM, SECT_AIR, SECT_UNDERWATER, SECT_DESERT,
   SECT_RIVER, SECT_OCEANFLOOR, SECT_UNDERGROUND, SECT_JUNGLE, SECT_SWAMP,
   SECT_TUNDRA, SECT_ICE, SECT_OCEAN, SECT_LAVA, SECT_SHORE, SECT_TREE, SECT_STONE,
   SECT_QUICKSAND, SECT_WALL, SECT_GLACIER, SECT_EXIT, SECT_TRAIL, SECT_BLANDS,
   SECT_GRASSLAND, SECT_SCRUB, SECT_BARREN, SECT_BRIDGE, SECT_ROAD, SECT_LANDING, SECT_MAX
} sector_types;

/*
 * Resistant Immune Susceptible flags
 * Now also supporting absorb flags - Samson 3-16-00 
 */
enum risa_flags
{
   RIS_NONE, RIS_FIRE, RIS_COLD, RIS_ELECTRICITY, RIS_ENERGY, RIS_BLUNT, RIS_PIERCE, RIS_SLASH,
   RIS_ACID, RIS_POISON, RIS_DRAIN, RIS_SLEEP, RIS_CHARM, RIS_HOLD, RIS_NONMAGIC,
   RIS_PLUS1, RIS_PLUS2, RIS_PLUS3, RIS_PLUS4, RIS_PLUS5, RIS_PLUS6, RIS_MAGIC,
   RIS_PARALYSIS, RIS_GOOD, RIS_EVIL, RIS_HACK, RIS_LASH, MAX_RIS_FLAG
};

/* 
 * Attack types
 */
typedef enum
{
   ATCK_BITE, ATCK_CLAWS, ATCK_TAIL, ATCK_STING, ATCK_PUNCH, ATCK_KICK,
   ATCK_TRIP, ATCK_BASH, ATCK_STUN, ATCK_GOUGE, ATCK_BACKSTAB, ATCK_AGE,
   ATCK_DRAIN, ATCK_FIREBREATH, ATCK_FROSTBREATH, ATCK_ACIDBREATH,
   ATCK_LIGHTNBREATH, ATCK_GASBREATH, ATCK_POISON, ATCK_NASTYPOISON, ATCK_GAZE,
   ATCK_BLINDNESS, ATCK_CAUSESERIOUS, ATCK_EARTHQUAKE, ATCK_CAUSECRITICAL,
   ATCK_CURSE, ATCK_FLAMESTRIKE, ATCK_HARM, ATCK_FIREBALL, ATCK_COLORSPRAY,
   ATCK_WEAKEN, ATCK_SPIRALBLAST, MAX_ATTACK_TYPE
} attack_types;

/* Removed redundant defenses that were covered in affectflags - Samson 5-12-99 */
/*
 * Defense types
 */
typedef enum
{
   DFND_PARRY, DFND_DODGE, DFND_HEAL, DFND_CURELIGHT, DFND_CURESERIOUS,
   DFND_CURECRITICAL, DFND_DISPELMAGIC, DFND_DISPELEVIL, DFND_SANCTUARY,
   DFND_UNUSED1, DFND_UNUSED2, DFND_SHIELD, DFND_BLESS, DFND_STONESKIN,
   DFND_TELEPORT, DFND_UNUSED3, DFND_UNUSED4, DFND_UNUSED5, DFND_UNUSED6,
   DFND_DISARM, DFND_UNUSED7, DFND_GRIP, DFND_TRUESIGHT, MAX_DEFENSE_TYPE
} defense_types;

/*
 * Body parts
 */
#define PART_HEAD		  	  BV00
#define PART_ARMS		  	  BV01
#define PART_LEGS		  	  BV02
#define PART_HEART		  BV03
#define PART_BRAINS		  BV04
#define PART_GUTS		  	  BV05
#define PART_HANDS		  BV06
#define PART_FEET		  	  BV07
#define PART_FINGERS		  BV08
#define PART_EAR		  	  BV09
#define PART_EYE		 	  BV10
#define PART_LONG_TONGUE	  BV11
#define PART_EYESTALKS		  BV12
#define PART_TENTACLES		  BV13
#define PART_FINS		  	  BV14
#define PART_WINGS		  BV15
#define PART_TAIL			  BV16
#define PART_SCALES		  BV17
/* for combat */
#define PART_CLAWS		  BV18
#define PART_FANGS		  BV19
#define PART_HORNS		  BV20
#define PART_TUSKS		  BV21
#define PART_TAILATTACK		  BV22
#define PART_SHARPSCALES	  BV23
#define PART_BEAK			  BV24

#define PART_HAUNCH		  BV25
#define PART_HOOVES		  BV26
#define PART_PAWS			  BV27
#define PART_FORELEGS		  BV28
#define PART_FEATHERS		  BV29

#define MAX_BPART 30

/*
 * Pipe flags
 */
#define PIPE_TAMPED	BV01
#define PIPE_LIT		BV02
#define PIPE_HOT		BV03
#define PIPE_DIRTY	BV04
#define PIPE_FILTHY	BV05
#define PIPE_GOINGOUT	BV06
#define PIPE_BURNT	BV07
#define PIPE_FULLOFASH	BV08

typedef enum
{
   TRAP_TYPE_POISON_GAS = 1, TRAP_TYPE_POISON_DART, TRAP_TYPE_POISON_NEEDLE,
   TRAP_TYPE_POISON_DAGGER, TRAP_TYPE_POISON_ARROW, TRAP_TYPE_BLINDNESS_GAS,
   TRAP_TYPE_SLEEPING_GAS, TRAP_TYPE_FLAME, TRAP_TYPE_EXPLOSION,
   TRAP_TYPE_ACID_SPRAY, TRAP_TYPE_ELECTRIC_SHOCK, TRAP_TYPE_BLADE,
   TRAP_TYPE_SEX_CHANGE
} traps;

#define MAX_TRAPTYPE		   TRAP_TYPE_SEX_CHANGE

#define TRAP_ROOM      		   BV00
#define TRAP_OBJ	      	   BV01
#define TRAP_ENTER_ROOM		   BV02
#define TRAP_LEAVE_ROOM		   BV03
#define TRAP_OPEN		   BV04
#define TRAP_CLOSE		   BV05
#define TRAP_GET		   BV06
#define TRAP_PUT		   BV07
#define TRAP_PICK		   BV08
#define TRAP_UNLOCK		   BV09
#define TRAP_N			   BV10
#define TRAP_S			   BV11
#define TRAP_E	      		   BV12
#define TRAP_W	      		   BV13
#define TRAP_U	      		   BV14
#define TRAP_D	      		   BV15
#define TRAP_EXAMINE		   BV16
#define TRAP_NE			   BV17
#define TRAP_NW			   BV18
#define TRAP_SE			   BV19
#define TRAP_SW			   BV20
#define TRAPFLAG_MAX          20

extern char *const log_flag[];   /* Used in cedit display and command saving */
extern char *const mag_flags[];  /* Used during bootup */
extern char *const npc_sex[SEX_MAX];
extern char *const npc_position[POS_MAX];
extern char *weapon_skills[WEP_MAX];   /* Used in spell_identify */
extern char *projectiles[PROJ_MAX]; /* For archery weapons */
extern char *const interfaces[]; /* Used in stat command */
extern char *const container_flags[];  /* Tagith */
extern char *const furniture_flags[];  /* Zarius */
extern char *const campgear[GEAR_MAX]; /* For OLC menus */
extern char *const ores[ORE_MAX];   /* For OLC menus */

/*. OLC structs .*/

typedef struct olc_data OLC_DATA;   /* Tagith */

struct olc_data
{
   MOB_INDEX_DATA *mob;
   ROOM_INDEX_DATA *room;
   OBJ_DATA *obj;
   AREA_DATA *area;
   struct shop_data *shop;
   EXTRA_DESCR_DATA *desc;
   AFFECT_DATA *paf;
   EXIT_DATA *xit;
   int mode;
   int zone_num;
   int number;
   int value;
   bool changed;
};

/*. Descriptor access macros .*/
#define OLC_MODE(d) 	((d)->olc->mode)  /* Parse input mode  */
#define OLC_NUM(d) 	((d)->olc->number)   /* Room/Obj VNUM  */
#define OLC_VNUM(d)	OLC_NUM(d)
#define OLC_VAL(d) 	((d)->olc->value) /* Scratch variable  */
#define OLC_OBJ(d)	(obj)
#define OLC_DESC(d) 	((d)->olc->desc)  /* Extra description */
#define OLC_AFF(d)	((d)->olc->paf)   /* Affect data       */
#define OLC_CHANGE(d)	((d)->olc->changed)  /* Changed flag      */
#define OLC_EXIT(d)     ((d)->olc->xit)   /* An Exit     */

#ifdef OLD_CIRCLE_STYLE
# define OLC_ROOM(d)    ((d)->olc->room)  /* Room structure       */
# define OLC_OBJ(d)     ((d)->olc->obj)   /* Object structure     */
# define OLC_MOB(d)     ((d)->olc->mob)   /* Mob structure        */
# define OLC_SHOP(d)    ((d)->olc->shop)  /* Shop structure       */
# define OLC_EXIT(d)	(OLC_ROOM(d)->dir_option[OLC_VAL(d)])
#endif

/*. Add/Remove save list types	.*/
#define OLC_SAVE_ROOM			0
#define OLC_SAVE_OBJ			1
#define OLC_SAVE_ZONE			2
#define OLC_SAVE_MOB			3
#define OLC_SAVE_SHOP			4

/* Submodes of OEDIT connectedness */
#define OEDIT_MAIN_MENU              	1
#define OEDIT_EDIT_NAMELIST          	2
#define OEDIT_SHORTDESC              	3
#define OEDIT_LONGDESC               	4
#define OEDIT_ACTDESC                	5
#define OEDIT_TYPE                   	6
#define OEDIT_EXTRAS                 	7
#define OEDIT_WEAR                  	8
#define OEDIT_WEIGHT                	9
#define OEDIT_COST                  	10
#define OEDIT_COSTPERDAY            	11
#define OEDIT_TIMER                 	12
#define OEDIT_VALUE_0               	13
#define OEDIT_VALUE_1               	14
#define OEDIT_VALUE_2               	15
#define OEDIT_VALUE_3               	16
#define OEDIT_VALUE_4				17
#define OEDIT_VALUE_5				18
#define OEDIT_EXTRADESC_KEY         	19
#define OEDIT_TRAPFLAGS             20

#define OEDIT_EXTRADESC_DESCRIPTION 	22
#define OEDIT_EXTRADESC_MENU        	23
#define OEDIT_LEVEL                 	24
#define OEDIT_LAYERS			25
#define OEDIT_AFFECT_MENU		26
#define OEDIT_AFFECT_LOCATION		27
#define OEDIT_AFFECT_MODIFIER		28
#define OEDIT_AFFECT_REMOVE		29
#define OEDIT_AFFECT_RIS		30
#define OEDIT_EXTRADESC_CHOICE		31
#define OEDIT_EXTRADESC_DELETE		32
#define OEDIT_VALUE_6			33
#define OEDIT_VALUE_7			34
#define OEDIT_VALUE_8			35
#define OEDIT_VALUE_9			36
#define OEDIT_VALUE_10			37

/* Submodes of REDIT connectedness */
#define REDIT_MAIN_MENU 		1
#define REDIT_NAME 			2
#define REDIT_DESC 			3
#define REDIT_FLAGS 			4
#define REDIT_SECTOR 			5
#define REDIT_EXIT_MENU 		6

#define REDIT_EXIT_DIR              8
#define REDIT_EXIT_VNUM 		9
#define REDIT_EXIT_DESC 		10
#define REDIT_EXIT_KEYWORD 		11
#define REDIT_EXIT_KEY 			12
#define REDIT_EXIT_FLAGS 		13
#define REDIT_EXTRADESC_MENU 		14
#define REDIT_EXTRADESC_KEY 		15
#define REDIT_EXTRADESC_DESCRIPTION 16
#define REDIT_TUNNEL			17
#define REDIT_TELEDELAY			18
#define REDIT_TELEVNUM			19
#define REDIT_EXIT_EDIT			20
#define REDIT_EXIT_ADD			21
#define REDIT_EXIT_DELETE		22
#define REDIT_EXIT_ADD_VNUM		23
#define REDIT_EXTRADESC_DELETE	24
#define REDIT_EXTRADESC_CHOICE	25
#define REDIT_NDESC			26

/*. Submodes of MEDIT connectedness 	.*/
#define MEDIT_NPC_MAIN_MENU		0
#define MEDIT_NAME 			1
#define MEDIT_S_DESC			2
#define MEDIT_L_DESC			3
#define MEDIT_D_DESC			4
#define MEDIT_NPC_FLAGS			5
#define MEDIT_AFF_FLAGS			6

#define MEDIT_SEX				8
#define MEDIT_HITROLL			9
#define MEDIT_DAMROLL			10
#define MEDIT_DAMNUMDIE			11
#define MEDIT_DAMSIZEDIE		12
#define MEDIT_DAMPLUS			13
#define MEDIT_HITPLUS			14
#define MEDIT_AC				15
#define MEDIT_GOLD			16
#define MEDIT_POS				17
#define MEDIT_DEFPOS			18
#define MEDIT_ATTACK			19
#define MEDIT_DEFENSE			20
#define MEDIT_LEVEL			21
#define MEDIT_ALIGNMENT			22
#define MEDIT_THACO			23
#define MEDIT_EXP				24
#define MEDIT_SPEC			25
#define MEDIT_RESISTANT			26
#define MEDIT_IMMUNE			27
#define MEDIT_SUSCEPTIBLE		28
#define MEDIT_ABSORB			29
#define MEDIT_MENTALSTATE		30

#define MEDIT_PARTS			32
#define MEDIT_HITPOINT			33
#define MEDIT_MANA			34
#define MEDIT_MOVE			35
#define MEDIT_CLASS			36
#define MEDIT_RACE			37
