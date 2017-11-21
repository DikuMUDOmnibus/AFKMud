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
 *                       Rune/Gem socketing module                          *
 *                 Inspired by the system used in Diablo 2                  *
 *             Also contains the random treasure creation code              *
 ****************************************************************************/

#define RUNE_FILE SYSTEM_DIR "runes.dat"
#define RUNEWORD_FILE SYSTEM_DIR "runeword.dat"

typedef enum
{
   RUNE_COMMON, RUNE_RARE, RUNE_ULTRARARE
} rune_rarities;

typedef struct rune_data RUNE_DATA;
typedef struct runeword_data RWORD_DATA;

extern RUNE_DATA *first_rune;
extern RUNE_DATA *last_rune;
extern RWORD_DATA *first_rword;
extern RWORD_DATA *last_rword;

/* Materials for Armor Generator and Weapon Generator */
struct armorgenM
{
   int material;  /* Type of material */
   char *name; /* Descriptive name */
   float weight;  /* Modification to weight */
   int ac;  /* Modification to armor class */
   int wd;  /* Modification to weapon damage */
   float cost; /* Modification to item value or cost */
   int mlevel; /* Minimum mob level before this material will drop */
};

/* Armor types for Armor Generator */
struct armorgenT
{
   int type;   /* Armor type */
   char *name; /* Descriptive name */
   float weight;  /* Base weight */
   int ac;  /* Base armor class */
   float cost; /* Base value or cost */
   char *flags;   /* Default flag set */
};

/* Weapon types for Weapon Generator */
struct weaponT
{
   int type;   /* Weapon type */
   char *name; /* Descriptive name */
   int wd;  /* Base damage */
   float weight;  /* Base weight */
   float cost; /* Base cost/value */
   int skill;  /* Skill type */
   int damage; /* Damage type */
   char *flags;   /* Default flag set */
};

struct rune_data
{
   RUNE_DATA *next;
   RUNE_DATA *prev;
   char *name;
   int rarity; /* Common, Rare, Ultrarare */
   int stat1[2];  /* The stat to modify goes in the first spot, modifier value in the second. */
   int stat2[2];  /* Stat1 is for weapons, Stat2 is for armors */
};

struct runeword_data
{
   RWORD_DATA *next;
   RWORD_DATA *prev;
   char *name; /* The runeword name */
   short type;   /* Weapon(1) or Armor(0) ? */
   char *rune1;   /* 1st required rune */
   char *rune2;   /* 2nd required rune */
   char *rune3;   /* 3rd required rune - NULL if not required */
   int stat1[2];  /* Affects the runeword transfers to the item */
   int stat2[2];
   int stat3[2];
   int stat4[2];
};

extern const struct weaponT weapon_type[];
extern const struct armorgenT armor_type[];
extern const struct armorgenM armor_materials[];
extern char *const weapon_quality[];
/* Refer to the tables in treasure.c to see what these affect. */
#define TMAT_MAX 14
#define TATP_MAX 17
#define TWTP_MAX 13
#define TQUAL_MAX 5
