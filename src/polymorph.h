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
 *                          Shaddai's Polymorph                             *
 ****************************************************************************/

#define MORPH_FILE      "morph.dat" /* For morph data */

typedef struct char_morph CHAR_MORPH;
typedef struct morph_data MORPH_DATA;

extern MORPH_DATA *morph_start;
extern MORPH_DATA *morph_end;

int do_morph_char( CHAR_DATA * ch, MORPH_DATA * morph );
void do_unmorph_char( CHAR_DATA * ch );

#define MORPHPERS(ch, looker, from) ( can_see( (looker), (ch), (from) ) ? (ch)->morph->morph->short_desc : "Someone" )

/*
 * Structure for a morph -- Shaddai
 */
/*
 *  Morph structs.
 */

#define ONLY_PKILL  	1
#define ONLY_PEACEFULL  2

struct char_morph
{
   MORPH_DATA *morph;
   EXT_BV affected_by;  /* New affected_by added */
   EXT_BV no_affected_by;  /* Prevents affects from being added */
   EXT_BV no_immune; /* Prevents Immunities */
   EXT_BV no_resistant; /* Prevents resistances */
   EXT_BV no_suscept;   /* Prevents Susceptibilities */
   EXT_BV resistant; /* Resistances added */
   EXT_BV suscept;   /* Suscepts added */
   EXT_BV immune; /* Immunities added */
   EXT_BV absorb; /* Absorbs added */
   int timer;  /* How much time is left */
   short ac;
   short cha;
   short con;
   short damroll;
   short dex;
   short dodge;
   short hit;
   short hitroll;
   short inte;
   short lck;
   short mana;
   short move;
   short parry;
   short saving_breath;
   short saving_para_petri;
   short saving_poison_death;
   short saving_spell_staff;
   short saving_wand;
   short str;
   short tumble;
   short wis;
   bool cast_allowed;   /* Casting allowed whilst morphed */
};

struct morph_data
{
   MORPH_DATA *next; /* Next morph file */
   MORPH_DATA *prev; /* Previous morph file */
   char *damroll;
   char *deity;
   char *description;
   char *help; /* What player sees for info on morph */
   char *hit;  /* Hitpoints added */
   char *hitroll;
   char *key_words;  /* Keywords added to your name */
   char *long_desc;  /* New long_desc for player */
   char *mana; /* Mana added not for vamps */
   char *morph_other;   /* What others see when you morph */
   char *morph_self; /* What you see when you morph */
   char *move; /* Move added */
   char *name; /* Name used to polymorph into this */
   char *short_desc; /* New short desc for player */
   char *no_skills;  /* Prevented Skills */
   char *skills;
   char *unmorph_other; /* What others see when you unmorph */
   char *unmorph_self;  /* What you see when you unmorph */
   EXT_BV affected_by;  /* New affected_by added */
   int Class;  /* Classes not allowed to use this */
   int defpos; /* Default position */
   EXT_BV no_affected_by;  /* Prevents affects from being added */
   EXT_BV no_immune; /* Prevents Immunities */
   EXT_BV no_resistant; /* Prevents resistances */
   EXT_BV no_suscept;   /* Prevents Susceptibilities */
   EXT_BV immune; /* Immunities added */
   int obj[3]; /* Object needed to morph you */
   int race;   /* Races not allowed to use this */
   EXT_BV resistant; /* Resistances added */
   EXT_BV suscept;   /* Suscepts added */
   EXT_BV absorb; /* Absorbs added - Samson 3-16-00 */
   int timer;  /* Timer for how long it lasts */
   int used;   /* How many times has this morph been used */
   int vnum;   /* Unique identifier */
   short ac;
   short cha; /* Amount Cha gained/Lost */
   short con; /* Amount of Con gained/Lost */
   short dayfrom;   /* Starting Day you can morph into this */
   short dayto;  /* Ending Day you can morph into this */
   short dex; /* Amount of dex added */
   short dodge;  /* Percent of dodge added IE 1 = 1% */
   short favourused;   /* Amount of favour to morph */
   short hpused; /* Amount of hps used to morph */
   short inte;   /* Amount of Int gained/lost */
   short lck; /* Amount of Lck gained/lost */
   short level;  /* Minimum level to use this morph */
   short manaused;  /* Amount of mana used to morph */
   short moveused;  /* Amount of move used to morph */
   short parry;  /* Percent of parry added IE 1 = 1% */
   short pkill;  /* Pkill Only, Peacefull Only or Both */
   short saving_breath;   /* Below are saving adjusted */
   short saving_para_petri;
   short saving_poison_death;
   short saving_spell_staff;
   short saving_wand;
   short sex; /* The sex that can morph into this */
   short str; /* Amount of str gained lost */
   short timefrom;  /* Hour starting you can morph */
   short timeto; /* Hour ending that you can morph */
   short tumble; /* Percent of tumble added IE 1 = 1% */
   short wis; /* Amount of Wis gained/lost */
   bool no_cast;  /* Can you cast a spell to morph into it */
   bool cast_allowed;   /* Can you cast spells whilst morphed into this */
   bool objuse[3];   /* Objects needed to morph */
};
