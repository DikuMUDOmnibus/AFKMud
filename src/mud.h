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
 *                          Main mud header file                            *
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <typeinfo>
#if defined(__OpenBSD__) || defined(__FreeBSD__)
#include <sys/types.h>
#endif

/* Used in basereport and world commands - don't remove these!
 * If you want to add your own, make a new set of defines and ADD the information.
 * Removing this is a violation of your license agreement.
 */
#define CODENAME "AFKMud"
#define CODEVERSION "1.77"
#define COPYRIGHT "Copyright The Alsherok Team 1997-2006. All rights reserved."

#define LGST 4096 /* Large String */
#define SMST 1024 /* Small String */

/*
 * Short scalar types.
 */
#if	!defined(FALSE)
#define FALSE	 0
#endif

#if	!defined(TRUE)
#define TRUE	 1
#endif

#if	!defined(BERR)
#define BERR	 255
#endif

typedef int ch_ret;

/*
 * Structure types.
 */
typedef struct affect_data AFFECT_DATA;
typedef struct area_data AREA_DATA;
typedef struct char_data CHAR_DATA;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct mob_index_data MOB_INDEX_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;
typedef struct pc_data PC_DATA;
typedef struct reset_data RESET_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct race_type RACE_TYPE;
typedef struct class_type CLASS_TYPE;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct weather_data WEATHER_DATA;
typedef struct neighbor_data NEIGHBOR_DATA;  /* FB */
typedef struct teleport_data TELEPORT_DATA;
typedef struct timer_data TIMER;
typedef struct system_data SYSTEM_DATA;
typedef struct smaug_affect SMAUG_AFF;
typedef struct skill_type SKILLTYPE;
typedef struct social_type SOCIALTYPE;
typedef struct cmd_type CMDTYPE;
typedef struct extended_bitvector EXT_BV;

/*
 * Function types. Samson. Stop. Don't do it! NO! You have to keep these things you idiot!
 */
typedef void DO_FUN( CHAR_DATA * ch, char *argument );
typedef ch_ret SPELL_FUN( int sn, int level, CHAR_DATA * ch, void *vo );
typedef bool SPEC_FUN( CHAR_DATA * ch );

/* "Oh God Samson, this is so HACKISH!" "Yes my son, but this preserves dlsym, which is good."
 * "*The masses bow down in worship to their God....*"
 */
#define CMDF extern "C" void
#define SPELLF extern "C" ch_ret
#define SPECF extern "C" bool

#define DUR_CONV	93.333333333333333333333333   /* Original value: 23.3333... - Quadrupled for time changes */

/* Hidden tilde char generated with alt155 on the number pad.
 * The code blocks the use of these symbols by default, so this should be quite safe. Samson 3-14-04
 */
#define HIDDEN_TILDE	'¢'

/* 32bit bitvector defines */
#define BV00		(1 <<  0)
#define BV01		(1 <<  1)
#define BV02		(1 <<  2)
#define BV03		(1 <<  3)
#define BV04		(1 <<  4)
#define BV05		(1 <<  5)
#define BV06		(1 <<  6)
#define BV07		(1 <<  7)
#define BV08		(1 <<  8)
#define BV09		(1 <<  9)
#define BV10		(1 << 10)
#define BV11		(1 << 11)
#define BV12		(1 << 12)
#define BV13		(1 << 13)
#define BV14		(1 << 14)
#define BV15		(1 << 15)
#define BV16		(1 << 16)
#define BV17		(1 << 17)
#define BV18		(1 << 18)
#define BV19		(1 << 19)
#define BV20		(1 << 20)
#define BV21		(1 << 21)
#define BV22		(1 << 22)
#define BV23		(1 << 23)
#define BV24		(1 << 24)
#define BV25		(1 << 25)
#define BV26		(1 << 26)
#define BV27		(1 << 27)
#define BV28		(1 << 28)
#define BV29		(1 << 29)
#define BV30		(1 << 30)
#define BV31		(1 << 31)
/* 32 USED! DO NOT ADD MORE! SB */

/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH		 2048
#define MAX_STRING_LENGTH	 8192 /* Longhand names retained for compatibility with Smaug */
#define MAX_INPUT_LENGTH	 2048
#define MSL MAX_STRING_LENGTH /* Shorthand versions used througout the code */
#define MIL MAX_INPUT_LENGTH
#define MAX_INBUF_SIZE		 4096

/*
 * Command logging types.
 */
typedef enum
{
   LOG_NORMAL, LOG_ALWAYS, LOG_NEVER, LOG_BUILD, LOG_HIGH, LOG_COMM, LOG_WARN, LOG_INFO, LOG_AUTH, LOG_ALL
} log_types;

/*
 * Return types for move_char, damage, greet_trigger, etc, etc
 * Added by Thoric to get rid of bugs
 */
typedef enum
{
   rNONE, rCHAR_DIED, rVICT_DIED, rSPELL_FAILED, rVICT_IMMUNE, rSTOP, rERROR = 255
} ret_types;

/* Echo types for echo_to_all */
#define ECHOTAR_ALL	0
#define ECHOTAR_PC	1
#define ECHOTAR_IMM	2

/*
 * Defines for extended bitvectors
 */
#ifndef INTBITS
#define INTBITS	32
#endif
#define XBM		31 /* extended bitmask   ( INTBITS - 1 )  */
#define RSV		5  /* right-shift value  ( sqrt(XBM+1) )  */
#define XBI		4  /* integers in an extended bitvector   */
#define MAX_BITS	XBI * INTBITS
/*
 * Structure for extended bitvectors -- Thoric
 */
struct extended_bitvector
{
   unsigned int bits[XBI]; /* Needs to be unsigned to compile in Redhat 6 - Samson */
};

/* Global Skill Numbers */
#define ASSIGN_GSN(gsn, skill)					\
do										\
{										\
   if( ( (gsn) = skill_lookup( (skill) ) ) == -1 ) \
	log_printf( "ASSIGN_GSN: Skill %s not found.\n",	(skill) );	\
} while(0)

/*
 * These are skill_lookup return values for common skills and spells.
 */
extern short gsn_style_evasive;
extern short gsn_style_defensive;
extern short gsn_style_standard;
extern short gsn_style_aggressive;
extern short gsn_style_berserk;

extern short gsn_backheel;   /* Samson 5-31-00 */
extern short gsn_metallurgy; /* Samson 5-31-00 */
extern short gsn_scout;   /* Samson 5-29-00 */
extern short gsn_scry; /* Samson 5-29-00 */
extern short gsn_tinker;  /* Samson 4-25-00 */
extern short gsn_deathsong;  /* Samson 4-25-00 */
extern short gsn_swim; /* Samson 4-24-00 */
extern short gsn_tenacity;   /* Samson 4-24-00 */
extern short gsn_bargain; /* Samson 4-23-00 */
extern short gsn_bladesong;  /* Samson 4-23-00 */
extern short gsn_elvensong;  /* Samson 4-23-00 */
extern short gsn_reverie; /* Samson 4-23-00 */
extern short gsn_mining;  /* Samson 4-17-00 */
extern short gsn_woodcall;   /* Samson 4-17-00 */
extern short gsn_forage;  /* Samson 3-26-00 */
extern short gsn_spy;  /* Samson 6-20-99 */
extern short gsn_charge;  /* Samson 6-07-99 */
extern short gsn_retreat; /* Samson 5-27-99 */
extern short gsn_detrap;
extern short gsn_backstab;
extern short gsn_circle;
extern short gsn_cook;
extern short gsn_assassinate;   /* Samson */
extern short gsn_feign;   /* Samson 10-22-98 */
extern short gsn_quiv; /* Samson 6-02-99 */
extern short gsn_dodge;
extern short gsn_hide;
extern short gsn_peek;
extern short gsn_pick_lock;
extern short gsn_sneak;
extern short gsn_steal;
extern short gsn_gouge;
extern short gsn_track;
extern short gsn_search;
extern short gsn_dig;
extern short gsn_mount;
extern short gsn_bashdoor;
extern short gsn_berserk;
extern short gsn_hitall;

extern short gsn_disarm;
extern short gsn_enhanced_damage;
extern short gsn_kick;
extern short gsn_parry;
extern short gsn_rescue;
extern short gsn_dual_wield;

extern short gsn_aid;

/* used to do specific lookups */
extern short gsn_first_spell;
extern short gsn_first_skill;
extern short gsn_first_weapon;
extern short gsn_first_tongue;
extern short gsn_first_ability;
extern short gsn_first_lore;
extern short gsn_top_sn;

/* spells */
extern short gsn_blindness;
extern short gsn_charm_person;
extern short gsn_aqua_breath;
extern short gsn_curse;
extern short gsn_invis;
extern short gsn_mass_invis;
extern short gsn_poison;
extern short gsn_sleep;
extern short gsn_silence; /* Samson 9-26-98 */
extern short gsn_paralyze;   /* Samson 9-26-98 */
extern short gsn_fireball;   /* for fireshield  */
extern short gsn_chill_touch;   /* for iceshield   */
extern short gsn_lightning_bolt;   /* for shockshield */

/* newer attack skills */
extern short gsn_punch;
extern short gsn_bash;
extern short gsn_stun;
extern short gsn_bite;
extern short gsn_claw;
extern short gsn_sting;
extern short gsn_tail;

extern short gsn_poison_weapon;
extern short gsn_scribe;
extern short gsn_brew;
extern short gsn_climb;

/* changed to new weapon types - Grimm */
extern short gsn_pugilism;
extern short gsn_swords;
extern short gsn_daggers;
extern short gsn_whips;
extern short gsn_talonous_arms;
extern short gsn_maces_hammers;
extern short gsn_blowguns;
extern short gsn_slings;
extern short gsn_axes;
extern short gsn_spears;
extern short gsn_staves;
extern short gsn_archery;

extern short gsn_grip;
extern short gsn_slice;
extern short gsn_tumble;

/* Language gsns. -- Altrag */
extern short gsn_common;
extern short gsn_elven;
extern short gsn_dwarven;
extern short gsn_pixie;
extern short gsn_ogre;
extern short gsn_orcish;
extern short gsn_trollish;
extern short gsn_goblin;
extern short gsn_halfling;

extern short gsn_tan;
extern short gsn_dragon_ride;

#include "mudcfg.h"  /* Contains definitions specific to your mud - will not be covered by patches. Samson 3-14-04 */
#include "color.h"   /* Custom color stuff */
#include "olc.h"  /* Oasis OLC code and global definitions */

/*
 * Time and weather stuff.
 */
typedef enum
{
   SUN_DARK, SUN_RISE, SUN_LIGHT, SUN_SET
} sun_positions;

typedef enum
{
   SKY_CLOUDLESS, SKY_CLOUDY, SKY_RAINING, SKY_LIGHTNING
} sky_conditions;

typedef enum
{
   TEMP_COLD, TEMP_COOL, TEMP_NORMAL, TEMP_WARM, TEMP_HOT
} temp_conditions;

typedef enum
{
   PRECIP_ARID, PRECIP_DRY, PRECIP_NORMAL, PRECIP_DAMP, PRECIP_WET
} precip_conditions;

typedef enum
{
   WIND_STILL, WIND_CALM, WIND_NORMAL, WIND_BREEZY, WIND_WINDY
} wind_conditions;

#define GET_TEMP_UNIT(weather)   ((weather->temp + 3*weath_unit - 1)/weath_unit)
#define GET_PRECIP_UNIT(weather) ((weather->precip + 3*weath_unit - 1)/weath_unit)
#define GET_WIND_UNIT(weather)   ((weather->wind + 3*weath_unit - 1)/weath_unit)

#define IS_RAINING(weather)      (GET_PRECIP_UNIT(weather)>PRECIP_NORMAL)
#define IS_WINDY(weather)        (GET_WIND_UNIT(weather)>WIND_NORMAL)
#define IS_CLOUDY(weather)       (GET_PRECIP_UNIT(weather)>1)
#define IS_TCOLD(weather)        (GET_TEMP_UNIT(weather)==TEMP_COLD)
#define IS_COOL(weather)         (GET_TEMP_UNIT(weather)==TEMP_COOL)
#define IS_HOT(weather)          (GET_TEMP_UNIT(weather)==TEMP_HOT)
#define IS_WARM(weather)         (GET_TEMP_UNIT(weather)==TEMP_WARM)
#define IS_SNOWING(weather)      (IS_RAINING(weather) && IS_COOL(weather))

struct time_info_data
{
   int hour;
   int day;
   int month;
   int year;
   int sunlight;
   int season; /* Samson 5-6-99 */
};

/* Define maximum number of climate settings - FB */
#define MAX_CLIMATE 5

struct weather_data
{
   NEIGHBOR_DATA *first_neighbor;   /* areas which affect weather sys */
   NEIGHBOR_DATA *last_neighbor;
   char *echo; /* echo string */
/*    int			mmhg;
    int			change;
    int			sky;
    int			sunlight; */
   int temp;   /* temperature */
   int precip; /* precipitation */
   int wind;   /* umm... wind */
   int temp_vector;  /* vectors controlling */
   int precip_vector;   /* rate of change */
   int wind_vector;
   int climate_temp; /* climate of the area */
   int climate_precip;
   int climate_wind;
   int echo_color;   /* color for the echo */
};

struct neighbor_data
{
   NEIGHBOR_DATA *next;
   NEIGHBOR_DATA *prev;
   AREA_DATA *address;
   char *name;
};

/* short cut crash bug fix provided by gfinello@mail.karmanet.it*/
typedef enum
{ relMSET_ON, relOSET_ON } relation_type;

typedef struct rel_data REL_DATA;

struct rel_data
{
   REL_DATA *next;
   REL_DATA *prev;
   void *Actor;
   void *Subject;
   relation_type Type;
};

/*
 * Connected state for a channel. Modified on varying dates - Samson
 */
typedef enum
{
   CON_GET_NAME = -99, CON_GET_OLD_PASSWORD,
   CON_CONFIRM_NEW_NAME, CON_GET_NEW_PASSWORD, CON_CONFIRM_NEW_PASSWORD,
   CON_GET_PORT_PASSWORD, CON_GET_NEW_SEX, CON_PRESS_ENTER,
   CON_READ_MOTD, CON_COPYOVER_RECOVER, CON_PLOADED,
   CON_PLAYING = 0, CON_EDITING, CON_ROLL_STATS,
   CON_OEDIT, CON_MEDIT, CON_REDIT, /* Oasis OLC */
   CON_PRIZENAME, CON_CONFIRMPRIZENAME, CON_PRIZEKEY,
   CON_CONFIRMPRIZEKEY, CON_DELETE, CON_RAISE_STAT,
   CON_BOARD, CON_FORKED
} connection_types;

/*
 * Character substates
 */
typedef enum
{
   SUB_NONE, SUB_PAUSE, SUB_PERSONAL_DESC, SUB_BAN_DESC, SUB_OBJ_LONG,
   SUB_OBJ_EXTRA, SUB_MOB_DESC, SUB_ROOM_DESC,
   SUB_ROOM_EXTRA, SUB_WRITING_NOTE, SUB_MPROG_EDIT,
   SUB_HELP_EDIT, SUB_PERSONAL_BIO, SUB_REPEATCMD,
   SUB_RESTRICTED, SUB_DEITYDESC, SUB_MORPH_DESC, SUB_MORPH_HELP,
   SUB_PROJ_DESC, SUB_SLAYCMSG, SUB_SLAYVMSG, SUB_SLAYRMSG, SUB_EDMOTD,
   SUB_ROOM_DESC_NITE, SUB_OVERLAND_DESC, SUB_BOARD_TO, SUB_BOARD_SUBJECT,
   SUB_BOARD_STICKY, SUB_BOARD_TEXT, SUB_BOARD_CONFIRM, SUB_BOARD_REDO_MENU,
   SUB_EDIT_ABORT,
   /*
    * timer types ONLY below this point 
    */
   SUB_TIMER_DO_ABORT = 128, SUB_TIMER_CANT_ABORT
} char_substates;

/*
 * Descriptor (channel) structure.
 */
struct descriptor_data
{
   DESCRIPTOR_DATA *next;
   DESCRIPTOR_DATA *prev;
   DESCRIPTOR_DATA *snoop_by;
   CHAR_DATA *character;
   CHAR_DATA *original;
   OLC_DATA *olc; /* Tagith - Oasis OLC */
   struct mccp_data *mccp; /* Mud Client Compression Protocol */
   char *host;
   char *outbuf;
   char *pagebuf;
   char *pagepoint;
   char *client;  /* Client detection */
   char inbuf[MAX_INBUF_SIZE];
   char incomm[MIL];
   char inlast[MIL];
   int port;
   int descriptor;
   short connected;
   short idle;
   short lines;
   short scrlen;
   bool fcommand;
   int repeat;
   unsigned long outsize;
   int outtop;
   unsigned long pagesize;
   int pagetop;
   char pagecmd;
   char pagecolor;
   int newstate;
   unsigned char prevcolor;
   int ifd;
#ifndef S_SPLINT_S
   pid_t ipid;
   pid_t process; /* Samson 4-16-98 - For new command shell code */
#endif
   bool msp_detected;
   bool mxp_detected;
   bool can_compress;
};

/*
 * Attribute bonus structures.
 */
struct str_app_type
{
   short tohit;
   short todam;
   short carry;
   short wield;
};

struct int_app_type
{
   short learn;
};

struct wis_app_type
{
   short practice;
};

struct dex_app_type
{
   short defensive;
};

struct con_app_type
{
   short hitp;
   short shock;
};

struct cha_app_type
{
   short charm;
};

struct lck_app_type
{
   short luck;
};

/*
 * Interface stuff -- Heath
 */
typedef enum
{
   INT_DALE, INT_SMAUG, INT_AFKMUD
} interface_types;

/*
 * TO types for act.
 */
typedef enum
{ TO_ROOM, TO_NOTVICT, TO_VICT, TO_CHAR, TO_CANSEE, TO_THIRD } to_types;

/*
 * Per-class stuff.
 */
struct class_type
{
   char *who_name;   /* Name for 'who'    */
   EXT_BV affected;
   short attr_prime;   /* Prime attribute (Not Used - Samson) */
   EXT_BV resist;
   EXT_BV suscept;
   int weapon; /* Vnum of Weapon given at creation */
   int armor;  /* Vnum of Body Armor given at creation - Samson */
   int legwear;   /* Vnum of Legwear given at creation - Samson 1-3-99 */
   int headwear;  /* Vnum of Headwear given at creation - Samson 1-3-99 */
   int armwear;   /* Vnum of Armwear given at creation - Samson 1-3-99 */
   int footwear;  /* Vnum of Footwear given at creation - Samson 1-3-99 */
   int shield; /* Vnum of Shield given at creation - Samson 1-3-99 */
   int held;   /* Vnum of held item given at creation - Samson 1-3-99 */
   short skill_adept;  /* Maximum skill level */
   int base_thac0;   /* Thac0 for level 1 - Dwip 5-11-01 */
   float thac0_gain; /* Thac0 amount gained per level - Dwip 5-11-01 */
   short hp_min; /* Min hp gained on leveling  */
   short hp_max; /* Max hp gained on leveling  */
   bool fMana; /* Class gains mana on level  */
};

/* race dedicated stuff */
struct race_type
{
   char race_name[16];  /* Race name         */
   EXT_BV affected;  /* Default affect bitvectors  */
   short str_plus;  /* Str bonus/penalty    */
   short dex_plus;  /* Dex      "        */
   short wis_plus;  /* Wis      "        */
   short int_plus;  /* Int      "        */
   short con_plus;  /* Con      "        */
   short cha_plus;  /* Cha      "        */
   short lck_plus;  /* Lck       "       */
   short hit;
   short mana;
   EXT_BV resist; /* Bugfix: Samson 5-7-99, and again on 9/9/05 */
   EXT_BV suscept;   /* Bugfix: Samson 5-7-99, and again on 9/9/05 */
   int class_restriction;  /* Flags for illegal classes  */
   int body_parts;   /* Bodyparts this race has */
   int language;  /* Default racial language      */
   short ac_plus;
   short alignment;
   EXT_BV attacks;
   EXT_BV defenses;
   short minalign;
   short maxalign;
   short exp_multiplier;
   short height;
   short weight;
   short hunger_mod;
   short thirst_mod;
   short saving_poison_death;
   short saving_wand;
   short saving_para_petri;
   short saving_breath;
   short saving_spell_staff;
   char *where_name[MAX_WHERE_NAME];
   short mana_regen;
   short hp_regen;
};

/*
 * An affect.
 *
 * So limited... so few fields... should we add more?
 */
struct affect_data
{
   AFFECT_DATA *next;
   AFFECT_DATA *prev;

   EXT_BV rismod;
   int bit;
   int duration;
   int modifier;
   short location;
   short type;
};

/*
 * A SMAUG spell
 */
struct smaug_affect
{
   SMAUG_AFF *next;
   char *duration;
   short location;
   char *modifier;
   int bitvector; /* this is the bit number */
};

/*
 * Autosave flags
 */
#define SV_DEATH		  BV00   /* Save on death */
#define SV_KILL			  BV01   /* Save when kill made */
#define SV_PASSCHG		  BV02   /* Save on password change */
#define SV_DROP			  BV03   /* Save on drop */
#define SV_PUT			  BV04   /* Save on put */
#define SV_GIVE			  BV05   /* Save on give */
#define SV_AUTO			  BV06   /* Auto save every x minutes (define in cset) */
#define SV_ZAPDROP		  BV07   /* Save when eq zaps */
#define SV_AUCTION		  BV08   /* Save on auction */
#define SV_GET			  BV09   /* Save on get */
#define SV_RECEIVE		  BV10   /* Save when receiving */
#define SV_IDLE			  BV11   /* Save when char goes idle */
/* BV12 */
/* BV13 */
#define SV_FILL			  BV14   /* Save on do_fill */
#define SV_EMPTY		  BV15   /* Save on do_empty */

/*
 * Flags for act_string -- Shaddai
 */
#define STRING_NONE               0
#define STRING_IMM                BV01

/*
 * old flags for conversion purposes -- will not conflict with the flags below
 */
#define OLD_SF_SAVE_HALF_DAMAGE	  BV18   /* old save for half damage */
#define OLD_SF_SAVE_NEGATES	  BV19   /* old save negates affect  */

/*
 * Skill/Spell flags	The minimum BV *MUST* be 11!
 */
#define SF_WATER		  BV00
#define SF_EARTH		  BV01
#define SF_AIR			  BV02
#define SF_ASTRAL		  BV03
#define SF_AREA			  BV04   /* is an area spell      */
#define SF_DISTANT		  BV05   /* affects something far away  */
#define SF_REVERSE		  BV06
#define SF_NOSELF		  	  BV07   /* Can't target yourself!   */
#define SF_NOCHARGE		  BV08   /* cannot be chagred into a wand */
#define SF_ACCUMULATIVE		  BV09   /* is accumulative    */
#define SF_RECASTABLE		  BV10   /* can be refreshed      */
#define SF_NOSCRIBE		  BV11   /* cannot be scribed     */
#define SF_NOBREW		  	  BV12   /* cannot be brewed      */
#define SF_GROUPSPELL		  BV13   /* only affects group members  */
#define SF_OBJECT		  	  BV14   /* directed at an object   */
#define SF_CHARACTER		  BV15   /* directed at a character  */
#define SF_SECRETSKILL		  BV16   /* hidden unless learned   */
#define SF_PKSENSITIVE		  BV17   /* much harder for plr vs. plr   */
#define SF_STOPONFAIL		  BV18   /* stops spell on first failure */
#define SF_NOFIGHT		  BV19   /* stops if char fighting       */
#define SF_NODISPEL             BV20   /* stops spell from being dispelled */
#define SF_RANDOMTARGET		  BV21   /* chooses a random target */
#define SF_NOMOUNT            BV22  /* Stops if PC is mounted - Samson 11-03-03 */

typedef enum
{ SS_NONE, SS_POISON_DEATH, SS_ROD_WANDS, SS_PARA_PETRI,
   SS_BREATH, SS_SPELL_STAFF
} save_types;

#define ALL_BITS		INT_MAX
#define SDAM_MASK		ALL_BITS & ~(BV00 | BV01 | BV02)
#define SACT_MASK		ALL_BITS & ~(BV03 | BV04 | BV05)
#define SCLA_MASK		ALL_BITS & ~(BV06 | BV07 | BV08)
#define SPOW_MASK		ALL_BITS & ~(BV09 | BV10)
#define SSAV_MASK		ALL_BITS & ~(BV11 | BV12 | BV13)

typedef enum
{ SD_NONE, SD_FIRE, SD_COLD, SD_ELECTRICITY, SD_ENERGY, SD_ACID,
   SD_POISON, SD_DRAIN
} spell_dam_types;

typedef enum
{ SA_NONE, SA_CREATE, SA_DESTROY, SA_RESIST, SA_SUSCEPT,
   SA_DIVINATE, SA_OBSCURE, SA_CHANGE
} spell_act_types;

typedef enum
{ SP_NONE, SP_MINOR, SP_GREATER, SP_MAJOR } spell_power_types;

typedef enum
{ SC_NONE, SC_LUNAR, SC_SOLAR, SC_TRAVEL, SC_SUMMON,
   SC_LIFE, SC_DEATH, SC_ILLUSION
} spell_class_types;

typedef enum
{ SE_NONE, SE_NEGATE, SE_EIGHTHDAM, SE_QUARTERDAM, SE_HALFDAM,
   SE_3QTRDAM, SE_REFLECT, SE_ABSORB
} spell_save_effects;

#define PT_WATER	100
#define PT_AIR		200
#define PT_EARTH	300
#define PT_FIRE		400

/*
 * Push/pull types for exits					-Thoric
 * To differentiate between the current of a river, or a strong gust of wind
 */
typedef enum
{
   PULL_UNDEFINED, PULL_VORTEX, PULL_VACUUM, PULL_SLIP, PULL_ICE, PULL_MYSTERIOUS,
   PULL_CURRENT = PT_WATER, PULL_WAVE, PULL_WHIRLPOOL, PULL_GEYSER,
   PULL_WIND = PT_AIR, PULL_STORM, PULL_COLDWIND, PULL_BREEZE,
   PULL_LANDSLIDE = PT_EARTH, PULL_SINKHOLE, PULL_QUICKSAND, PULL_EARTHQUAKE,
   PULL_LAVA = PT_FIRE, PULL_HOTAIR
} dir_pulltypes;

/* Auth Flags */
#define FLAG_WRAUTH		      1
#define FLAG_AUTH		      2

/*
 * Conditions.
 */
typedef enum
{
   COND_DRUNK, COND_FULL, COND_THIRST, MAX_CONDS
} conditions;

/*
 * Styles.
 */
typedef enum
{
   STYLE_BERSERK, STYLE_AGGRESSIVE, STYLE_FIGHTING, STYLE_DEFENSIVE,
   STYLE_EVASIVE
} styles;

/*
 * ACT bits for players.
 */
/* DAMMIT! Don't forget to add these things to build.c!! */
typedef enum
{
   PLR_IS_NPC, PLR_BOUGHT_PET, PLR_SHOVEDRAG, PLR_AUTOEXIT, PLR_AUTOLOOT,
   PLR_AUTOSAC, PLR_BLANK, PLR_unused, PLR_BRIEF, PLR_COMBINE, PLR_PROMPT,
   PLR_TELNET_GA, PLR_HOLYLIGHT, PLR_WIZINVIS, PLR_ROOMVNUM, PLR_SILENCE,
   PLR_NO_EMOTE, PLR_BOARDED, PLR_NO_TELL, PLR_LOG, PLR_DENY, PLR_FREEZE,
   PLR_EXEMPT, PLR_ONSHIP, PLR_LITTERBUG, PLR_ANSI, PLR_unused2, PLR_unused3, PLR_FLEE,
   PLR_AUTOGOLD, PLR_GHOST, PLR_AFK, PLR_INVISPROMPT, PLR_BUSY, PLR_AUTOASSIST,
   PLR_SMARTSAC, PLR_IDLING, PLR_ONMAP, PLR_MAPEDIT, PLR_GUILDSPLIT, PLR_GROUPSPLIT,
   PLR_MSP, PLR_MXP, PLR_COMPASS, PLR_MXPPROMPT, MAX_PLR_FLAG
} player_flags;

/* Bits for pc_data->flags */
/* DAMMIT! Don't forget to add these things to build.c!! */
#define PCFLAG_R1                BV00
#define PCFLAG_DEADLY            BV01
#define PCFLAG_UNAUTHED		   BV02
#define PCFLAG_NORECALL          BV03
#define PCFLAG_NOINTRO           BV04
#define PCFLAG_GAG		   BV05
#define PCFLAG_RETIRED           BV06
#define PCFLAG_GUEST             BV07
#define PCFLAG_NOSUMMON		   BV08
#define PCFLAG_PAGERON		   BV09
#define PCFLAG_NOTITLE           BV10
#define PCFLAG_GROUPWHO		   BV11
#define PCFLAG_DIAGNOSE		   BV12
#define PCFLAG_HIGHGAG		   BV13
#define PCFLAG_WATCH		   BV14  /* see function "do_watch" */
#define PCFLAG_HELPSTART	   BV15  /* Force new players to help start */
#define PCFLAG_AUTOFLAGS         BV16  /* Added by Samson 12-10-97 for new flag display */
#define PCFLAG_SECTORD		   BV17  /* Added by Samson 12-10-97 for new sector display */
#define PCFLAG_ANAME		   BV18  /* Added by Samson 12-13-97 for new area display */
#define PCFLAG_NOBEEP		   BV19  /* Added by Samson 2-15-98 */
#define PCFLAG_PASSDOOR          BV20  /* Added by Samson 3-21-98 */
#define PCFLAG_PRIVACY		   BV21  /* Added by Samson 6-11-99 Finger privacy */
#define PCFLAG_NOTELL		   BV22  /* Samson 3-2-02 */
#define PCFLAG_CHECKBOARD        BV23  /* Samson 4-28-02 */
#define PCFLAG_NOQUOTE           BV24  /* Samson 4-30-03 */

#define MAX_PCFLAG 25

typedef enum
{
   TIMER_NONE, TIMER_RECENTFIGHT, TIMER_SHOVEDRAG, TIMER_DO_FUN,
   TIMER_APPLIED, TIMER_PKILLED, TIMER_ASUPRESSED
} timer_types;

struct timer_data
{
   TIMER *prev;
   TIMER *next;
   DO_FUN *do_fun;
   int value;
   short type;
   int count;
};

/* Area flags - Narn Mar/96 */
/* Don't forget to update build.c!!! */
#define AFLAG_NOPKILL         BV00
#define AFLAG_NOCAMP		BV01
#define AFLAG_NOASTRAL		BV02
#define AFLAG_NOPORTAL		BV03
#define AFLAG_NORECALL		BV04
#define AFLAG_NOSUMMON		BV05
#define AFLAG_NOSCRY		BV06
#define AFLAG_NOTELEPORT	BV07
#define AFLAG_ARENA		BV08
#define AFLAG_NOBEACON		BV09
#define AFLAG_NOQUIT		BV10
/* 11 - 30 are available */
#define AFLAG_PROTOTYPE       BV31

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct mob_index_data
{
   MOB_INDEX_DATA *next;
   AREA_DATA *area;
   SPEC_FUN *spec_fun;
   struct shop_data *pShop;
   struct repairshop_data *rShop;
   struct mob_prog_data *mudprogs;
   EXT_BV progtypes;
   EXT_BV act;
   EXT_BV affected_by;
   EXT_BV attacks;
   EXT_BV defenses;
   char *player_name;
   char *short_descr;
   char *long_descr;
   char *chardesc;
   char *spec_funname;
   int vnum;
   short count;
   short killed;
   short sex;
   short level;
   short alignment;
   short mobthac0;
   short ac;
   short hitnodice;
   short hitsizedice;
   short hitplus;
   short damnodice;
   short damsizedice;
   short damplus;
   float numattacks;
   int gold;
   int exp;
   int xflags;
   EXT_BV resistant;
   EXT_BV immune;
   EXT_BV susceptible;
   EXT_BV absorb; /* Samson 3-16-00 */
   short max_move;  /* Samson 7-14-00 */
   short max_mana;  /* Samson 7-14-00 */
   int speaks;
   int speaking;
   short position;
   short defposition;
   short height;
   short weight;
   short race;
   short Class;
   short hitroll;
   short damroll;
   short perm_str;
   short perm_int;
   short perm_wis;
   short perm_dex;
   short perm_con;
   short perm_cha;
   short perm_lck;
   short saving_poison_death;
   short saving_wand;
   short saving_para_petri;
   short saving_breath;
   short saving_spell_staff;
};

/*
 * One character (PC or NPC).
 */
struct char_data
{
   CHAR_DATA *next;
   CHAR_DATA *prev;
   CHAR_DATA *next_in_room;
   CHAR_DATA *prev_in_room;
   CHAR_DATA *first_pet;
   CHAR_DATA *last_pet;
   CHAR_DATA *next_pet;
   CHAR_DATA *prev_pet;
   CHAR_DATA *master;
   CHAR_DATA *leader;
   struct fighting_data *fighting;
   struct hunt_hate_fear *hunting;
   struct hunt_hate_fear *fearing;
   struct hunt_hate_fear *hating;
   struct char_morph *morph;
   CHAR_DATA *reply;
   CHAR_DATA *switched;
   CHAR_DATA *mount;
   MOB_INDEX_DATA *pIndexData;
   AFFECT_DATA *first_affect;
   AFFECT_DATA *last_affect;
   OBJ_DATA *first_carrying;
   OBJ_DATA *last_carrying;
   OBJ_DATA *on;  /* Xerves' Furniture Code - Samson 7-20-00 */
   ROOM_INDEX_DATA *in_room;
   ROOM_INDEX_DATA *was_in_room;
   ROOM_INDEX_DATA *orig_room;   /* Xorith's boards */
   PC_DATA *pcdata;
   DESCRIPTOR_DATA *desc;
   struct bit_data *first_abit;  /* abit/qbit code */
   struct bit_data *last_abit;
   CHAR_DATA *my_skyship;  /* Bond skyship to player */
   CHAR_DATA *my_rider; /* Bond player to skyship */
   struct ship_data *on_ship; /* Ship char is on, or NULL if not - Samson 1-6-00 */
   TIMER *first_timer;
   TIMER *last_timer;
   DO_FUN *last_cmd;
   DO_FUN *prev_cmd; /* mapping */
   SPEC_FUN *spec_fun;
   EXT_BV act;
   EXT_BV affected_by;
   EXT_BV no_affected_by;
   EXT_BV attacks;
   EXT_BV defenses;
   char *spec_funname;
   struct mob_prog_act_list *mpact;
   int mpactnum;
   unsigned short mpscriptpos;
   char *alloc_ptr;  /* Must str_dup and free this one */
   short substate;
   int tempnum;
   char *name;
   char *short_descr;
   char *long_descr;
   char *chardesc;
   short num_fighting;
   short sex;
   short Class;
   short race;
   short level;
   short trust;
   short timer;
   short wait;
   short hit;
   short max_hit;
   short hit_regen;
   short mana;
   short max_mana;
   short mana_regen;
   short move;
   short max_move;
   short move_regen;
   short spellfail;
   float numattacks;
   short amp;
   int gold;
   int exp;
   int carry_weight;
   int carry_number;
   int xflags;
   EXT_BV resistant;
   EXT_BV no_resistant;
   EXT_BV immune;
   EXT_BV no_immune;
   EXT_BV susceptible;
   EXT_BV no_susceptible;
   int speaks;
   int speaking;
   short saving_poison_death;
   short saving_wand;
   short saving_para_petri;
   short saving_breath;
   short saving_spell_staff;
   short alignment;
   short barenumdie;
   short baresizedie;
   short mobthac0;
   short hitroll;
   short damroll;
   short hitplus;
   short damplus;
   short position;
   short defposition;
   short style;
   short height;
   short weight;
   short armor;
   short wimpy;
   short perm_str;
   short perm_int;
   short perm_wis;
   short perm_dex;
   short perm_con;
   short perm_cha;
   short perm_lck;
   short mod_str;
   short mod_int;
   short mod_wis;
   short mod_dex;
   short mod_con;
   short mod_cha;
   short mod_lck;
   short mental_state; /* simplified */
   short mobinvis;  /* Mobinvis level SB */
   EXT_BV absorb; /* Absorbtion flag for RIS data - Samson 3-16-00 */
   int home_vnum; /* For sentinel mobs only, used during hotboot world save - Samson 4-1-01 */
   short x;   /* Coordinates on the overland map - Samson 7-31-99 */
   short y;
   short map; /* Which map are they on? - Samson 8-3-99 */
   short sector; /* Type of terrain to restrict a wandering mob to on overland - Samson 7-27-00 */
   bool has_skyship; /* Identifies has skyship */
   bool inflight; /* skyship is in flight   */
   int zzzzz;  /* skyship is idling      */
   int dcoordx;   /* Destination X coord   */
   int dcoordy;   /* Destination Y coord   */
   bool backtracking;   /* Unsafe landing flag   */
   int lcoordx;   /* Launch X coord  */
   int lcoordy;   /* Launch Y coord  */
   int heading;   /* The skyship's directional heading */
};

/*
 * Data which only PC's have. Modified from original form by Samson.
 */
struct pc_data
{
   AREA_DATA *area;  /* For the area a PC has been assigned to build */
   struct clan_data *clan;
   struct council_data *council;
   struct deity_data *deity;
   struct editor_data *editor;
   struct note_data *pnote;
   struct note_data *first_comment;
   struct note_data *last_comment;
   struct alias_type *first_alias;
   struct alias_type *last_alias;
   struct zone_data *first_zone; /* List of zones this PC has visited - Samson 7-11-00 */
   struct zone_data *last_zone;  /* Stores only the Area's name for crosschecking */
   struct bit_data *first_qbit;  /* abit/qbit code */
   struct bit_data *last_qbit;
   struct ignore_data *first_ignored;  /* keep track of who to ignore */
   struct ignore_data *last_ignored;
   struct board_chardata *first_boarddata;
   struct board_chardata *last_boarddata;
   struct board_data *board;
#ifdef I3
   struct i3_chardata *i3chardata;
#endif
#ifdef IMC
   struct imcchar_data *imcchardata;
#endif
   short cmd_recurse;
   void *spare_ptr;
   void *dest_buf;   /* This one is to assign to differen things */
   char *homepage;
   char *clan_name;
   char *council_name;
   char *deity_name;
   char *pwd;
   char *bamfin;
   char *bamfout;
   char *filename;   /* For the safe mset name -Shaddai */
   char *rank;
   char *title;
   char *bestowments;   /* Special bestowed commands     */
   char *helled_by;
   char *bio;  /* Personal Bio */
   char *authed_by;  /* what crazy imm authed this name ;) */
   char *prompt;  /* User config prompts */
   char *fprompt; /* Fight prompts */
   char *subprompt;  /* Substate prompt */
   char *email;   /* Email address - Samson */
   char *afkbuf;  /* afk reason buffer - Samson 8-31-98 */
   char *motd_buf;   /* A temp buffer for editing MOTDs - 12-31-00 */
   char *say_history[MAX_SAYHISTORY];  /* Say history -- Kratas */
   char *tell_history[MAX_TELLHISTORY];
   char *lasthost;   /* Stores host info so it doesn't have to depend on descriptor, for things like finger */
   char *chan_listen;   /* For dynamic channels - Samson 3-2-02 */
   int flags;  /* Whether the player is deadly and whatever else we add.      */
   int pkills; /* Number of pkills on behalf of clan */
   int pdeaths;   /* Number of times pkilled (legally)  */
   int mkills; /* Number of mobs killed         */
   int mdeaths;   /* Number of deaths due to mobs       */
   int illegal_pk;   /* Number of illegal pk's committed   */
   long int restore_time;  /* The last time the char did a restore all */
   int low_vnum;  /* vnum range */
   int hi_vnum;
   short wizinvis;  /* wizinvis level */
   short min_snoop; /* minimum snoop level */
   short condition[MAX_CONDS];
   short learned[MAX_SKILL];
   short favor;  /* deity favor */
   short practice;
   time_t release_date; /* Auto-helling.. Altrag */
   short pagerlen;  /* For pager (NOT menus) */
   int home;
   int balance;   /* Bank balance - Samson */
   int exgold; /* Extragold affect - Samson */
   short camp;   /* Did the player camp or rent? Samson 9-19-98 */
   int icq; /* ICQ number for player - Samson 1-4-99 */
   short beacon[MAX_BEACONS];   /* For beacon spell, recall points - Samson 2-7-99 */
   short interface; /* DOTD Interface code */
   short charmies;  /* Number of Charmies */
   short realm;  /* What immortal realm are they in? - Samson 6-6-99 */
   int secedit;   /* Overland Map OLC - Samson 8-1-99 */
   int rent;   /* Saves amount of daily rent built up - Samson 1-24-00 */
   bool norares;  /* Toggled so we can tell the PC he ran out of money - Samson 1-24-00 */
   bool autorent; /* Is this PC an autorent? - Samson 7-27-00 */
   int alsherok;  /* Last room they rented in on Alsherok - Samson 12-20-00 */
   int eletar; /* Last room they rented in on Eletar - Samson 12-20-00 */
   int alatia; /* Last room they rented in on Alatia - Samson 12-20-00 */
   time_t motd;   /* Last time they read an MOTD - Samson 12-31-00 */
   time_t imotd;  /* Last time they read an IMOTD for immortals - 12-31-00 */
   int spam;   /* How many times have they triggered the spamguard? - 3-18-01 */
   bool hotboot;  /* Used only to force hotboot to save keys etc that normally get stripped - Samson 6-22-01 */
   short age_bonus;
   short age;
   short day;
   short month;
   short year;
   int played;
   time_t logon;
   time_t save_time;
   int timezone;
   short colors[MAX_COLORS]; /* Custom color codes - Samson 9-28-98 */
   int version;   /* Temporary variable to track pfile password conversion */
};

/*
 * Damage types from the attack_table[]
 */
/* modified for new weapon_types - Grimm */
/* Trimmed down to reduce duplicated types - Samson 1-9-00 */
typedef enum
{
   DAM_HIT, DAM_SLASH, DAM_STAB, DAM_HACK, DAM_CRUSH, DAM_LASH,
   DAM_PIERCE, DAM_THRUST, DAM_MAX_TYPE
} damage_types;

/*
 * Extra description data for a room or object.
 */
struct extra_descr_data
{
   EXTRA_DESCR_DATA *next; /* Next in list                     */
   EXTRA_DESCR_DATA *prev; /* Previous in list                 */
   char *keyword; /* Keyword in look/examine          */
   char *extradesc;  /* What to see                      */
};

/*
 * Prototype for an object.
 */
struct obj_index_data
{
   OBJ_INDEX_DATA *next;
   EXTRA_DESCR_DATA *first_extradesc;
   EXTRA_DESCR_DATA *last_extradesc;
   AFFECT_DATA *first_affect;
   AFFECT_DATA *last_affect;
   AREA_DATA *area;
   struct mob_prog_data *mudprogs;  /* objprogs */
   EXT_BV progtypes; /* objprogs */
   EXT_BV extra_flags;
   char *name;
   char *short_descr;
   char *objdesc;
   char *action_desc;
   char *socket[3];  /* Name of rune/gem the item has in each socket - Samson 3-31-02 */
   int vnum;
   short level;
   short item_type;
   int magic_flags;  /* Need more bitvectors for spells - Scryn */
   int wear_flags;
   short count;
   short weight;
   int cost;
   int value[11]; /* Raised to 11 by Samson on 12-14-02 */
   short layers;
   int rent;   /* Yes, this is used. :) */
   int limit;  /* Limit on how many of these are allowed to load - Samson 1-9-00 */
};

/*
 * One object.
 */
struct obj_data
{
   OBJ_DATA *next;
   OBJ_DATA *prev;
   OBJ_DATA *next_content;
   OBJ_DATA *prev_content;
   OBJ_DATA *first_content;
   OBJ_DATA *last_content;
   OBJ_DATA *in_obj;
   CHAR_DATA *carried_by;
   EXTRA_DESCR_DATA *first_extradesc;
   EXTRA_DESCR_DATA *last_extradesc;
   AFFECT_DATA *first_affect;
   AFFECT_DATA *last_affect;
   OBJ_INDEX_DATA *pIndexData;
   ROOM_INDEX_DATA *in_room;
   EXT_BV extra_flags;
   struct mob_prog_act_list *mpact; /* mudprogs */
   int mpactnum;  /* mudprogs */
   char *name;
   char *short_descr;
   char *objdesc;
   char *action_desc;
   char *owner;   /* Who owns this item? Used with personal flag for Sindhae prizes. */
   char *seller;  /* Who put the item up for auction? */
   char *buyer;   /* Who made the final bid on the item? */
   char *socket[3];  /* Name of rune/gem the item has in each socket - Samson 3-31-02 */
   int bid; /* What was the amount of the final bid? */
   short day; /* What day of the week was it offered or sold? */
   short month;  /* What month? */
   short year;   /* What year? */
   short item_type;
   unsigned short mpscriptpos;
   int magic_flags;  /*Need more bitvectors for spells - Scryn */
   int wear_flags;
   short wear_loc;
   short weight;
   int cost;
   short level;
   short timer;
   int value[11]; /* Raised to 11 by Samson on 12-14-02 */
   short count;  /* support for object grouping */
   int rent;   /* Oh, and yes, this is being used :) */
   int room_vnum; /* Track it's room vnum for hotbooting and such */
   short x;   /* Object coordinates on overland maps - Samson 8-21-99 */
   short y;
   short map; /* Which map is it on? - Samson 8-21-99 */
};

/*
 * Exit data.
 */
struct exit_data
{
   EXIT_DATA *prev;  /* previous exit in linked list  */
   EXIT_DATA *next;  /* next exit in linked list   */
   EXIT_DATA *rexit; /* Reverse exit pointer    */
   ROOM_INDEX_DATA *to_room;  /* Pointer to destination room   */
   EXT_BV exit_info; /* door states & other flags */
   char *keyword; /* Keywords for exit or door  */
   char *exitdesc;   /* Description of exit     */
   int vnum;   /* Vnum of room exit leads to */
   int rvnum;  /* Vnum of room in opposite dir  */
   int key; /* Key vnum       */
   short vdir;   /* Physical "direction"    */
   short pull;   /* pull of direction (current)   */
   short pulltype;  /* type of pull (current, wind)  */
   short x;   /* Coordinates to Overland Map - Samson 7-31-99 */
   short y;
};

/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'H': hide an object
 *   'T': trap an object
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct reset_data
{
   RESET_DATA *next;
   RESET_DATA *prev;
   RESET_DATA *first_reset;
   RESET_DATA *last_reset;
   RESET_DATA *next_reset;
   RESET_DATA *prev_reset;
   char command;
   int extra;
   int arg1;
   int arg2;
   int arg3;
   short arg4;   /* arg4 - arg6 used for overland coordinates */
   short arg5;
   short arg6;
};

/*
 * Area definition.
 */
struct area_data
{
   AREA_DATA *next;
   AREA_DATA *prev;
   AREA_DATA *next_sort;   /* Vnum sort */
   AREA_DATA *prev_sort;
   AREA_DATA *next_sort_name; /* Used for alphanum. sort */
   AREA_DATA *prev_sort_name; /* Ditto, Fireblade */
   WEATHER_DATA *weather;  /* FB */
   ROOM_INDEX_DATA *first_room;
   ROOM_INDEX_DATA *last_room;
   char *name;
   char *filename;
   char *author;  /* Scryn */
   char *resetmsg;   /* Rennard */
   int flags;
   short status; /* h, 8/11 */
   short age;
   short nplayer;
   short reset_frequency;
   int low_vnum;
   int hi_vnum;
   int low_soft_range;
   int hi_soft_range;
   int low_hard_range;
   int hi_hard_range;
   short continent; /* Added for Overland support - Samson 9-16-00 */
   short x;   /* Coordinates of a zone on the overland, for recall/death purposes - Samson 12-25-00 */
   short y;
   short version;   /* Replaces the file_ver method of tracking - Samson 12-23-02 */
   time_t last_resettime;  /* Tracking for when the area was last reset. Debugging tool. Samson 3-6-04 */
   short tg_nothing;   /* TG Values are for area-specific random treasure chances - Samson 11-25-04 */
   short tg_gold;
   short tg_item;
   short tg_gem; /* Runes come after gems and go up to 100% */
   short tg_scroll; /* These are for specific chances of a particular item type - Samson 11-25-04 */
   short tg_potion;
   short tg_wand;
   short tg_armor;  /* Weapons come after armors and go up to 100% */
};

/*
 * Used to keep track of system settings and statistics		-Thoric
 */
struct system_data
{
   void *dlHandle;   /* libdl System Handle - Trax */
   char *time_of_max;   /* Time of max ever */
   char *mud_name;   /* Name of mud */
   char *admin_email;   /* Email address for admin - Samson 10-17-98 */
   char *password;   /* Port access code */
   char *telnet;  /* Store telnet address for who/webwho */
   char *http; /* Store web address for who/webwho */
   int maxplayers;   /* Maximum players this boot   */
   int alltimemax;   /* Maximum players ever   */
   bool NO_NAME_RESOLVING; /* Hostnames are not resolved  */
   bool DENY_NEW_PLAYERS;  /* New players cannot connect  */
   bool WAIT_FOR_AUTH;  /* New players must be auth'ed */
   short read_all_mail;   /* Read all player mail(was 54) */
   short read_mail_free;  /* Read mail for free (was 51) */
   short write_mail_free; /* Write mail for free(was 51) */
   short take_others_mail;   /* Take others mail (was 54)   */
   short build_level;  /* Level of build channel LEVEL_BUILD */
   short level_modify_proto; /* Level to modify prototype stuff LEVEL_LESSER */
   short level_override_private;   /* override private flag */
   short level_mset_player;  /* Level to mset a player */
   short bash_plr_vs_plr; /* Bash mod player vs. player */
   short bash_nontank; /* Bash mod basher != primary attacker */
   short gouge_plr_vs_plr;   /* Gouge mod player vs. player */
   short gouge_nontank;   /* Gouge mod player != primary attacker */
   short stun_plr_vs_plr; /* Stun mod player vs. player */
   short stun_regular; /* Stun difficult */
   short dodge_mod; /* Divide dodge chance by */
   short parry_mod; /* Divide parry chance by */
   short tumble_mod;   /* Divide tumble chance by */
   short dam_plr_vs_plr;  /* Damage mod player vs. player */
   short dam_plr_vs_mob;  /* Damage mod player vs. mobile */
   short dam_mob_vs_plr;  /* Damage mod mobile vs. player */
   short dam_mob_vs_mob;  /* Damage mod mobile vs. mobile */
   short level_getobjnotake; /* Get objects without take flag */
   short level_forcepc;   /* The level at which you can use force on players. */
   short bestow_dif;   /* Max # of levels between trust and command level for a bestow to work --Blodkai */
   int save_flags;   /* Toggles for saving conditions */
   short save_frequency;  /* How often to autosave someone */
   bool check_imm_host; /* Do we check immortal's hosts? */
   bool save_pets;   /* Do pets save? */
   bool WIZLOCK;  /* Is the game wizlocked? - Samson 8-2-98 */
   bool IMPLOCK;  /* Is the game implocked? - Samson 8-2-98 */
   bool LOCKDOWN; /* Is the game locked down? - Samson 8-23-98 */
   short newbie_purge; /* Level to auto-purge newbies at - Samson 12-27-98 */
   short regular_purge;   /* Level to purge normal players at - Samson 12-27-98 */
   bool CLEANPFILES; /* Should the mud clean up pfiles daily? - Samson 12-27-98 */
   bool TESTINGMODE; /* Blocks file copies to main port when active - Samson 1-31-99 */
   bool RENT;  /* Toggle to enable or disable charging rent - Samson 7-24-99 */
   short mapsize;   /* Laziness feature mostly. Changes the overland map visibility radius */
   time_t motd;   /* Last time MOTD was edited */
   time_t imotd;  /* Last time IMOTD was edited */
   bool webtoggle;   /* For webserver code, starts the webserver if this is TRUE */
   bool webcounter;  /* Logs web hits if this is TRUE */
   bool webrunning;  /* Set when successfully starting webserver. DOES NOT SAVE */
   int auctionseconds;  /* Seconds between auction events */
   int maxvnum;
   int minguildlevel;
   int maxcondval;
   int maxign;
   int maximpact;
   int maxholiday;
   int initcond;
   int minrent;
   int secpertick;
   int pulsepersec;
   int pulsetick;
   int pulseviolence;
   int pulsespell;
   int pulsemobile;
   int pulsecalendar;
   int pulseenvironment;
   int pulseskyship;
   int hoursperday;
   int daysperweek;
   int dayspermonth;
   int monthsperyear;
   int daysperyear;
   int hoursunrise;
   int hourdaybegin;
   int hournoon;
   int hoursunset;
   int hournightbegin;
   int hourmidnight;
   int rebootcount;  /* How many minutes to count down for a reboot - Samson 4-22-03 */
   bool crashhandler;   /* Do we intercept SIGSEGV - Samson 3-11-04 */
};

/*
 * Room type.
 */
struct room_index_data
{
   ROOM_INDEX_DATA *next;
   CHAR_DATA *first_person;   /* people in the room  */
   CHAR_DATA *last_person; /*      ..    */
   OBJ_DATA *first_content;   /* objects on floor    */
   OBJ_DATA *last_content; /*      ..    */
   EXTRA_DESCR_DATA *first_extradesc;  /* extra descriptions */
   EXTRA_DESCR_DATA *last_extradesc;   /*      ..    */
   AREA_DATA *area;
   ROOM_INDEX_DATA *next_aroom;  /* Rooms within an area */
   ROOM_INDEX_DATA *prev_aroom;
   EXIT_DATA *first_exit;  /* exits from the room */
   EXIT_DATA *last_exit;   /*      ..    */
   AFFECT_DATA *first_affect; /* effects on the room */
   AFFECT_DATA *last_affect;  /*      ..    */
   RESET_DATA *first_reset;
   RESET_DATA *last_reset;
   RESET_DATA *last_mob_reset;
   RESET_DATA *last_obj_reset;
   struct mob_prog_data *mudprogs;  /* mudprogs */
   struct mob_prog_act_list *mpact; /* mudprogs */
   int mpactnum;  /* mudprogs */
   unsigned short mpscriptpos;
   char *name;
   char *roomdesc;   /* So that it can now be more easily grep'd - Samson 10-16-03 */
   char *nitedesc;   /* added NiteDesc -- Dracones */
   int vnum;
   EXT_BV room_flags;
   EXT_BV progtypes; /* mudprogs */
   short light;  /* amount of light in the room */
   short sector_type;
   short winter_sector;   /* Stores the original sector type for stuff that freezes in winter - Samson 7-19-00 */
   int tele_vnum;
   short tele_delay;
   short tunnel; /* max people that will fit */
};

/*
 * Delayed teleport type.
 */
struct teleport_data
{
   TELEPORT_DATA *next;
   TELEPORT_DATA *prev;
   ROOM_INDEX_DATA *room;
   short timer;
};

/*
 * Types of skill numbers.  Used to keep separate lists of sn's
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED               -1
#define TYPE_HIT                     1000 /* allows for 1000 skills/spells */
#define TYPE_HERB		     2000   /* allows for 1000 attack types  */
#define TYPE_PERSONAL		     3000   /* allows for 1000 herb types    */
#define TYPE_RACIAL		     4000   /* allows for 1000 personal types */
#define TYPE_DISEASE		     5000   /* allows for 1000 racial types  */

/*
 *  Target types.
 */
typedef enum
{
   TAR_IGNORE, TAR_CHAR_OFFENSIVE, TAR_CHAR_DEFENSIVE, TAR_CHAR_SELF,
   TAR_OBJ_INV
} target_types;

typedef enum
{
   SKILL_UNKNOWN, SKILL_SPELL, SKILL_SKILL, SKILL_WEAPON, SKILL_TONGUE,
   SKILL_HERB, SKILL_RACIAL, SKILL_DISEASE, SKILL_LORE
} skill_types;

/*
 * Skills include spells as a particular case.
 */
struct skill_type
{
   SMAUG_AFF *affects;  /* Spell affects, if any   */
   char *name; /* Name of skill     */
   short skill_level[MAX_CLASS];   /* Level needed by class   */
   short skill_adept[MAX_CLASS];   /* Max attainable % in this skill */
   short race_level[MAX_RACE];  /* Racial abilities: level      */
   short race_adept[MAX_RACE];  /* Racial abilities: adept      */
   SPELL_FUN *spell_fun;   /* Spell pointer (for spells) */
   char *spell_fun_name;   /* Spell function name - Trax */
   DO_FUN *skill_fun;   /* Skill pointer (for skills) */
   char *skill_fun_name;   /* Skill function name - Trax */
   short target; /* Legal targets     */
   short minimum_position;   /* Position for caster / user */
   short slot;   /* Slot for #OBJECT loading   */
   short min_mana;  /* Minimum mana used    */
   short beats;  /* Rounds required to use skill  */
   short guild;  /* Which guild the skill belongs to */
   short min_level; /* Minimum level to be able to cast */
   short type;   /* Spell/Skill/Weapon/Tongue  */
   short range;  /* Range of spell (rooms)  */
   int info;   /* Spell action/class/etc  */
   int flags;  /* Flags       */
   char *noun_damage;   /* Damage message    */
   char *msg_off; /* Wear off message     */
   char *hit_char;   /* Success message to caster  */
   char *hit_vict;   /* Success message to victim  */
   char *hit_room;   /* Success message to room */
   char *hit_dest;   /* Success message to dest room  */
   char *miss_char;  /* Failure message to caster  */
   char *miss_vict;  /* Failure message to victim  */
   char *miss_room;  /* Failure message to room */
   char *die_char;   /* Victim death msg to caster */
   char *die_vict;   /* Victim death msg to victim */
   char *die_room;   /* Victim death msg to room   */
   char *imm_char;   /* Victim immune msg to caster   */
   char *imm_vict;   /* Victim immune msg to victim   */
   char *imm_room;   /* Victim immune msg to room  */
   char *dice; /* Dice roll         */
   char *author;  /* Skill's author */
   char *components; /* Spell components, if any   */
   char *teachers;   /* Skill requires a special teacher */
   int value;  /* Misc value        */
   int spell_sector; /* Sector Spell work    */
   char saves; /* What saving spell applies  */
   char difficulty;  /* Difficulty of casting/learning */
   char participants;   /* # of required participants */
   int rent;   /* Adjusted rent value used in object creation, accounts for SMAUG_AFF's */
};

/*
 * So we can have different configs for different ports -- Shaddai
 */
extern int port;

/*
 * Cmd flag names --Shaddai
 */
extern char *const cmd_flags[];

/*
 * Utility macros.
 */
#define UMIN(a, b)		((a) < (b) ? (a) : (b))
#define UMAX(a, b)		((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)		((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)		((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define UPPER(c)		((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
/* Safe fclose macro adopted from DOTD Codebase */
#define FCLOSE(fp)  fclose((fp)); (fp)=NULL;

/* This damn thing is used in so many places it was about time to just move it here - Samson 10-4-03 */
#define KEY( literal, field, value ) \
if( !str_cmp( word, (literal) ) )    \
{                                    \
   (field) = (value);                \
   fMatch = TRUE;                    \
   break;                            \
}

/*
 * Macros for accessing virtually unlimited bitvectors.		-Thoric
 *
 * Note that these macros use the bit number rather than the bit value
 * itself -- which means that you can only access _one_ bit at a time
 *
 * This code uses an array of integers
 */

/*
 * The functions for these prototypes can be found in misc.c
 * They are up here because they are used by the macros below
 */
bool ext_is_empty( EXT_BV * bits );
void ext_clear_bits( EXT_BV * bits );
int ext_has_bits( EXT_BV * var, EXT_BV * bits );
bool ext_same_bits( EXT_BV * var, EXT_BV * bits );
void ext_set_bits( EXT_BV * var, EXT_BV * bits );
void ext_remove_bits( EXT_BV * var, EXT_BV * bits );
void ext_toggle_bits( EXT_BV * var, EXT_BV * bits );

/*
 * Here are the extended bitvector macros:
 */
#define xIS_SET(var, bit)           ((var).bits[(bit) >> RSV] & 1 << ((bit) & XBM))
#define xSET_BIT(var, bit)          ((var).bits[(bit) >> RSV] |= 1 << ((bit) & XBM))
#define xSET_BITS(var, bit)         (ext_set_bits(&(var), &(bit)))
#define xREMOVE_BIT(var, bit)       ((var).bits[(bit) >> RSV] &= ~(1 << ((bit) & XBM)))
#define xREMOVE_BITS(var, bit)      (ext_remove_bits(&(var), &(bit)))
#define xTOGGLE_BIT(var, bit)       ((var).bits[(bit) >> RSV] ^= 1 << ((bit) & XBM))
#define xTOGGLE_BITS(var, bit)      (ext_toggle_bits(&(var), &(bit)))
#define xCLEAR_BITS(var)            (ext_clear_bits(&(var)))
#define xIS_EMPTY(var)              (ext_is_empty(&(var)))
#define xHAS_BITS(var, bit)         (ext_has_bits(&(var), &(bit)))
#define xSAME_BITS(var, bit)        (ext_same_bits(&(var), &(bit)))

/*
 * Old-style Bit manipulation macros
 *
 * The bit passed is the actual value of the bit (Use the BV## defines)
 */
/* A whole bunch of these were taken from the DOTD codebase - Samson */
#define IS_SET(flag, bit)	((flag) & (bit))
#define SET_BIT(var, bit)	((var) |= (bit))
#define REMOVE_BIT(var, bit)	((var) &= ~(bit))
#define TOGGLE_BIT(var, bit)	((var) ^= (bit))

#define IS_AREA_FLAG(var, bit)         IS_SET((var)->flags, (bit))
#define SET_AREA_FLAG(var, bit )       SET_BIT((var)->flags, (bit))
#define REMOVE_AREA_FLAG(var, bit)     REMOVE_BIT((var)->flags, (bit))
#define IS_AREA_STATUS(var, bit)       IS_SET((var)->status, (bit))
#define SET_AREA_STATUS(var, bit)      SET_BIT((var)->status, (bit))
#define REMOVE_AREA_STATUS(var, bit)   REMOVE_BIT((var)->status, (bit))
#define IS_ROOM_FLAG(var, bit)         xIS_SET((var)->room_flags, (bit))
#define SET_ROOM_FLAG(var, bit)        xSET_BIT((var)->room_flags, (bit))
#define REMOVE_ROOM_FLAG(var, bit)     xREMOVE_BIT((var)->room_flags, (bit))
#define IS_OBJ_FLAG(var, bit)		   xIS_SET((var)->extra_flags, (bit))
#define SET_OBJ_FLAG(var, bit)	   xSET_BIT((var)->extra_flags, (bit))
#define REMOVE_OBJ_FLAG(var, bit)	   xREMOVE_BIT((var)->extra_flags, (bit))
#define IS_MAGIC_FLAG(obj, flag)       IS_SET((obj)->magic_flags, (flag))
#define SET_MAGIC_FLAG(obj, flag)      SET_BIT((obj)->magic_flags, (flag))
#define REMOVE_MAGIC_FLAG(obj, flag)   REMOVE_BIT((obj)->magic_flags, (flag))
#define CAN_WEAR(obj, part)            IS_SET((obj)->wear_flags, (part))
#define IS_WEAR_FLAG(var, bit)	   IS_SET((var)->wear_flags, (bit))
#define SET_WEAR_FLAG(var, bit)	   SET_BIT((var)->wear_flags, (bit))
#define REMOVE_WEAR_FLAG(var, bit)	   REMOVE_BIT((var)->wear_flags, (bit))
#define IS_ACT_FLAG(var, bit)          ( IS_NPC(var) && xIS_SET((var)->act, (bit)) )
#define SET_ACT_FLAG(var, bit)         xSET_BIT((var)->act, (bit))
#define REMOVE_ACT_FLAG(var, bit)      xREMOVE_BIT((var)->act, (bit))
#define IS_PLR_FLAG(var, bit)          ( !IS_NPC(var) && xIS_SET((var)->act, (bit)) )
#define SET_PLR_FLAG(var, bit)         xSET_BIT((var)->act, (bit))
#define REMOVE_PLR_FLAG(var, bit)      xREMOVE_BIT((var)->act, (bit))
#define IS_SAVE_FLAG(bit)              IS_SET( sysdata.save_flags, (bit) )
#define SET_SAVE_FLAG(bit)             SET_BIT( sysdata.save_flags, (bit) )
#define REMOVE_SAVE_FLAG(bit)          REMOVE_BIT( sysdata.save_flags, (bit) )
#define IS_PCFLAG(var, bit)            ( !IS_NPC(var) && IS_SET((var)->pcdata->flags, (bit)) )
#define SET_PCFLAG(var, bit)           SET_BIT((var)->pcdata->flags, (bit))
#define REMOVE_PCFLAG(var, bit)        REMOVE_BIT((var)->pcdata->flags, (bit))
#define IS_EXIT_FLAG(var, bit)         xIS_SET((var)->exit_info, (bit))
#define SET_EXIT_FLAG(var, bit)        xSET_BIT((var)->exit_info, (bit))
#define REMOVE_EXIT_FLAG(var, bit)     xREMOVE_BIT((var)->exit_info, (bit))
#define IS_IMMUNE(ch, ris)	         xIS_SET((ch)->immune, (ris))
#define SET_IMMUNE(ch, ris)            xSET_BIT((ch)->immune, (ris))
#define REMOVE_IMMUNE(ch, ris)         xREMOVE_BIT((ch)->immune, (ris))
#define IS_RESIS(ch, ris)	         xIS_SET((ch)->resistant, (ris))
#define SET_RESIS(ch, ris)             xSET_BIT((ch)->resistant, (ris))
#define REMOVE_RESIS(ch, ris)          xREMOVE_BIT((ch)->resistant, (ris))
#define IS_SUSCEP(ch, ris)	         xIS_SET((ch)->susceptible, (ris))
#define SET_SUSCEP(ch, ris)            xSET_BIT((ch)->susceptible, (ris))
#define REMOVE_SUSCEP(ch, ris)         xREMOVE_BIT((ch)->susceptible, (ris))
#define IS_ABSORB(ch, ris)		   xIS_SET((ch)->absorb, (ris))
#define SET_ABSORB(ch, ris)            xSET_BIT((ch)->absorb, (ris))
#define REMOVE_ABSORB(ch, ris)         xREMOVE_BIT((ch)->absorb, (ris))
#define IS_ATTACK(ch, attack)		   ( IS_NPC((ch)) && xIS_SET((ch)->attacks, (attack)) )
#define SET_ATTACK(ch, attack)         xSET_BIT((ch)->attacks, (attack))
#define REMOVE_ATTACK(ch, attack)      xREMOVE_BIT((ch)->attacks, (attack))
#define IS_DEFENSE(ch, defense)	   ( IS_NPC((ch)) && xIS_SET((ch)->defenses, (defense)) )
#define SET_DEFENSE(ch, defense)       xSET_BIT((ch)->defenses, (defense))
#define REMOVE_DEFENSE(ch, defense)    xREMOVE_BIT((ch)->defenses, (defense))
#define IS_AFFECTED(ch, sn)	         xIS_SET((ch)->affected_by, (sn))
#define SET_AFFECTED(ch, sn)           xSET_BIT((ch)->affected_by, (sn))
#define REMOVE_AFFECTED(ch, sn)        xREMOVE_BIT((ch)->affected_by, (sn))
#define IS_CMD_FLAG(cmd, flag)         IS_SET((cmd)->flags, (flag))
#define SET_CMD_FLAG(cmd, flag)        SET_BIT((cmd)->flags, (flag))
#define REMOVE_CMD_FLAG(cmd, flag)     REMOVE_BIT((cmd)->flags, (flag))

/*
 * Memory allocation macros.
 */
#define CREATE(result, type, number)					\
do											\
{											\
    if (!((result) = (type *) calloc ((number), sizeof(type))))	\
    {											\
      perror("calloc failure");                                         \
      fprintf(stderr, "Calloc failure @ %s:%d\n", __FILE__, __LINE__ ); \
	abort();									\
    }											\
} while(0)

#define RECREATE(result,type,number)					\
do											\
{											\
   if(!((result) = (type *)realloc((result), sizeof(type) * (number)))) \
    {											\
	log_printf( "Realloc failure @ %s:%d\n", __FILE__, __LINE__ ); \
	abort();									\
    }											\
} while(0)

#if defined(__FreeBSD__)
#define DISPOSE(point)                      \
do                                          \
{                                           \
   if( (point) )                            \
   {                                        \
      free( (point) );                      \
      (point) = NULL;                       \
   }                                        \
} while(0)
#else
#define DISPOSE(point)                         \
do                                             \
{                                              \
   if( (point) )                               \
   {                                           \
      if( typeid((point)) == typeid(char*) )   \
      {                                        \
         if( in_hash_table( (char*)(point) ) ) \
         {                                     \
            log_printf( "&RDISPOSE called on STRALLOC pointer: %s, line %d\n", __FILE__, __LINE__ ); \
            log_string( "Attempting to correct." ); \
            if( str_free( (char*)(point) ) == -1 ) \
               log_printf( "&RSTRFREEing bad pointer: %s, line %d\n", __FILE__, __LINE__ ); \
         }                                     \
         else                                  \
            free( (point) );                   \
      }                                        \
      else                                     \
         free( (point) );                      \
      (point) = NULL;                          \
   }                                           \
   else                                          \
      (point) = NULL;                            \
} while(0)
#endif

#define STRALLOC(point)		str_alloc((point))
#define QUICKLINK(point)	quick_link((point))

#if defined(__FreeBSD__)
#define STRFREE(point)                          \
do                                              \
{                                               \
   if((point))                                  \
   {                                            \
      if( str_free((point)) == -1 )             \
         bug( "&RSTRFREEing bad pointer: %s, line %d", __FILE__, __LINE__ ); \
      (point) = NULL;                           \
   }                                            \
} while(0)
#else
#define STRFREE(point)                           \
do                                               \
{                                                \
   if((point))                                   \
   {                                             \
      if( !in_hash_table( (point) ) )            \
      {                                          \
         log_printf( "&RSTRFREE called on str_dup pointer: %s, line %d\n", __FILE__, __LINE__ ); \
         log_string( "Attempting to correct." ); \
         free( (point) );                        \
      }                                          \
      else if( str_free((point)) == -1 )         \
         log_printf( "&RSTRFREEing bad pointer: %s, line %d\n", __FILE__, __LINE__ ); \
      (point) = NULL;                            \
   }                                             \
   else                                          \
      (point) = NULL;                            \
} while(0)
#endif

/* double-linked list handling macros -Thoric */
/* Updated by Scion 8/6/1999 */
#define LINK(link, first, last, next, prev)                     	\
do                                                              	\
{                                                               	\
   if ( !(first) )								\
   {                                           				\
      (first) = (link);				                       	\
      (last) = (link);							    	\
   }											\
   else                                                      	\
      (last)->next = (link);			                       	\
   (link)->next = NULL;			                         	\
   if ((first) == (link))							\
      (link)->prev = NULL;							\
   else										\
      (link)->prev = (last);			                       	\
   (last) = (link);				                       	\
} while(0)

#define INSERT(link, insert, first, next, prev)                 \
do                                                              \
{                                                               \
   (link)->prev = (insert)->prev;			                \
   if ( !(insert)->prev )                                       \
      (first) = (link);                                         \
   else                                                         \
      (insert)->prev->next = (link);                            \
   (insert)->prev = (link);                                     \
   (link)->next = (insert);                                     \
} while(0)

#define UNLINK(link, first, last, next, prev)                   	\
do                                                              	\
{                                                               	\
	if ( !(link)->prev )							\
	{			                                    	\
         (first) = (link)->next;			                 	\
	   if ((first))							 	\
	      (first)->prev = NULL;						\
	} 										\
	else										\
	{                                                 		\
         (link)->prev->next = (link)->next;                 	\
	}										\
	if ( !(link)->next ) 							\
	{				                                    \
         (last) = (link)->prev;                 			\
      if((last))                                \
	      (last)->next = NULL;						\
	} 										\
	else										\
	{                                                    		\
         (link)->next->prev = (link)->prev;                 	\
	}										\
} while(0)

#define CHECK_SUBRESTRICTED(ch)					\
do								\
{								\
   if( (ch)->substate == SUB_RESTRICTED )       \
    {								\
	send_to_char( "You cannot use this command from within another command.\n\r", ch );	\
	return;							\
    }								\
} while(0)

/*
 * Character macros.
 */
#define GET_AMAGICP(ch)         ((ch)->amp)
#define GET_INTF(ch)		(IS_NPC((ch)) ? INT_DALE : ((ch)->pcdata->interface))
#define GET_MANA(ch)            ((ch)->mana)
#define GET_MAX_MANA(ch)        ((ch)->max_mana)
#define GET_HIT(ch)             ((ch)->hit)
#define GET_MAX_HIT(ch)         ((ch)->max_hit)
#define GET_MOVE(ch)            ((ch)->move)
#define GET_MAX_MOVE(ch)        ((ch)->max_move)
#define GET_EXP(ch)             ((ch)->exp)
#define GET_COND(ch, i)         ((ch)->pcdata->condition[(i)])
#define GET_PRACS(ch)           ((ch)->practice)
#define GET_ALIGN(ch)           ((ch)->alignment)
#define GET_POS(ch)             ((ch)->position)
#define GET_NAME(ch)            ((ch)->name)
#define GET_TIME_PLAYED(ch)     (((ch)->pcdata->played + (current_time - (ch)->pcdata->logon)) / 3600)
#define GET_RACE(ch)            ((ch)->race)
#define IS_NPC(ch)		(xIS_SET((ch)->act, ACT_IS_NPC))
#define IS_IMP(ch)		((ch)->level >= LEVEL_KL)
#define IS_IMMORTAL(ch)		((ch)->level >= LEVEL_IMMORTAL)  /* Modified by Samson */
#define IS_HERO(ch)		((ch)->level >= LEVEL_AVATAR) /* Modified by Samson */
/* Retired and guest imms. */
#define IS_RETIRED(ch)        ( IS_PCFLAG( (ch), PCFLAG_RETIRED ) )
#define IS_GUEST(ch)          ( IS_PCFLAG( (ch), PCFLAG_GUEST ) )
#define HAS_BODYPART(ch, part)	((ch)->xflags == 0 || IS_SET((ch)->xflags, (part)))
#define IS_MOUNTED(ch)   	( (ch)->position == POS_MOUNTED && (ch)->mount != NULL )

#define CAN_CAST(ch)		( ( (ch)->Class != CLASS_WARRIOR && (ch)->Class != CLASS_ROGUE  \
					&& (ch)->Class != CLASS_MONK ) || IS_IMMORTAL((ch)) )

#define IS_GOOD(ch)		((ch)->alignment >= 350)
#define IS_EVIL(ch)		((ch)->alignment <= -350)
#define IS_NEUTRAL(ch)		(!IS_GOOD((ch)) && !IS_EVIL((ch)))

#define IS_AWAKE(ch)		( (ch)->position > POS_SLEEPING )
#define GET_AC(ch)		( (ch)->armor + ( IS_AWAKE(ch) ? dex_app[get_curr_dex(ch)].defensive : 0 ) )
#define GET_HITROLL(ch)		( (ch)->hitroll + str_app[get_curr_str(ch)].tohit )

/* Thanks to Chriss Baeke for noticing damplus was unused */
#define GET_DAMROLL(ch)	( (ch)->damroll +(ch)->damplus + str_app[get_curr_str(ch)].todam )

#define IS_OUTSIDE(ch)		(!IS_ROOM_FLAG( (ch)->in_room, ROOM_INDOORS ) \
				    && !IS_ROOM_FLAG( (ch)->in_room, ROOM_TUNNEL ) \
				    && !IS_ROOM_FLAG( (ch)->in_room, ROOM_CAVE ) \
				    && !IS_ROOM_FLAG( (ch)->in_room, ROOM_CAVERN ) )

#define INDOOR_SECTOR(sect) (  (sect) == SECT_INDOORS || 	           \
				       (sect) == SECT_UNDERWATER ||               \
                               (sect) == SECT_OCEANFLOOR ||               \
                               (sect) == SECT_UNDERGROUND )

#define IS_DRUNK(ch, drunk)     (number_percent() < ( (ch)->pcdata->condition[COND_DRUNK] * 2 / (drunk) ) )

#define IS_PKILL(ch)            ( IS_PCFLAG((ch), PCFLAG_DEADLY) )

#define CAN_PKILL(ch)           IS_PKILL((ch))

#define WAIT_STATE(ch, npulse)      ((ch)->wait = UMAX((ch)->wait, (IS_IMMORTAL(ch) ? 0 :(npulse))))

#define EXIT(ch, door)		( get_exit( (ch)->in_room, door ) )

#define CAN_GO(ch, door)	(EXIT((ch),(door)) && (EXIT((ch),(door))->to_room != NULL)  \
                          	&& !IS_EXIT_FLAG(EXIT((ch), (door)), EX_CLOSED))

#define IS_FLOATING(ch)		( IS_AFFECTED((ch), AFF_FLYING) || IS_AFFECTED((ch), AFF_FLOATING) )

#define IS_VALID_SN(sn)		( (sn) >=0 && (sn) < MAX_SKILL && skill_table[(sn)] && skill_table[(sn)]->name )

#define IS_VALID_HERB(sn)	( (sn) >=0 && (sn) < MAX_HERB	&& herb_table[(sn)] && herb_table[(sn)]->name )

#define IS_VALID_DISEASE(sn)	( (sn) >=0 && (sn) < MAX_DISEASE && disease_table[(sn)] && disease_table[(sn)]->name )

#define SPELL_FLAG(skill, flag)	( IS_SET((skill)->flags, (flag)) )
#define SPELL_DAMAGE(skill)	( ((skill)->info      ) & 7 )
#define SPELL_ACTION(skill)	( ((skill)->info >>  3) & 7 )
#define SPELL_CLASS(skill)	( ((skill)->info >>  6) & 7 )
#define SPELL_POWER(skill)	( ((skill)->info >>  9) & 3 )
#define SPELL_SAVE(skill)	( ((skill)->info >> 11) & 7 )
#define SET_SDAM(skill, val)	( (skill)->info =  ((skill)->info & SDAM_MASK) + ((val) & 7) )
#define SET_SACT(skill, val)	( (skill)->info =  ((skill)->info & SACT_MASK) + (((val) & 7) << 3) )
#define SET_SCLA(skill, val)	( (skill)->info =  ((skill)->info & SCLA_MASK) + (((val) & 7) << 6) )
#define SET_SPOW(skill, val)	( (skill)->info =  ((skill)->info & SPOW_MASK) + (((val) & 3) << 9) )
#define SET_SSAV(skill, val)	( (skill)->info =  ((skill)->info & SSAV_MASK) + (((val) & 7) << 11) )

/* RIS by gsn lookups. -- Altrag.
   Will need to add some || stuff for spells that need a special GSN. */

#define IS_FIRE(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_FIRE )
#define IS_COLD(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_COLD )
#define IS_ACID(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_ACID )
#define IS_ELECTRICITY(dt)	( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_ELECTRICITY )
#define IS_ENERGY(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_ENERGY )
#define IS_DRAIN(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_DRAIN )
#define IS_POISON(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_POISON )

/*
 * MudProg macros.						-Thoric
 */
#define HAS_PROG(what, prog)	(xIS_SET((what)->progtypes, (prog)))

/*
 * Description macros.
 */
#define PERS(ch, looker, from) ( can_see( (looker), (ch), (from) ) ? \
	( IS_NPC(ch) ? (ch)->short_descr : (ch)->name ) : "Someone" )

#define log_string(txt)		( log_string_plus( (txt), LOG_NORMAL, LEVEL_LOG ) )
#define dam_message(ch, victim, dam, dt)	( new_dam_message((ch), (victim), (dam), (dt), NULL) )

/*
 *  Defines for the command flags. --Shaddai
 */
#define CMD_POSSESS		BV00
#define CMD_POLYMORPHED		BV01
#define CMD_WATCH			BV02  /* FB */
#define CMD_ACTION		BV03  /* Samson 7-7-00 */
#define CMD_NOSPAM            BV04  /* Used to flag commands for the spamguard - Samson 12-27-01 */
#define CMD_GHOST			BV05  /* Commands allowed as a ghost - Samson 10-25-02 */
#define CMD_MUDPROG           BV06  /* Command is only used by mudprogs. Prevents display on help/commands. Samson 11-26-03 */
#define CMD_NOFORCE           BV07  /* Command can't be forced using either the force command or mpforce - Samson 3-3-04 */

/*
 * Structure for a command in the command lookup table.
 */
struct cmd_type
{
   CMDTYPE *next;
   DO_FUN *do_fun;
   char *name;
   char *fun_name;   /* Added to hold the func name and dump some functions totally - Trax */
   int flags;  /* Added for Checking interpret stuff -Shaddai */
   short position;
   short level;
   short log;
};

/*
 * Structure for a social in the socials table.
 */
struct social_type
{
   SOCIALTYPE *next;
   char *name;
   char *char_no_arg;
   char *others_no_arg;
   char *char_found;
   char *others_found;
   char *vict_found;
   char *char_auto;
   char *others_auto;
   char *obj_self;
   char *obj_others;
};

/*
 * Global constants.
 */
extern time_t last_restore_all_time;

extern const struct str_app_type str_app[26];
extern const struct int_app_type int_app[26];
extern const struct wis_app_type wis_app[26];
extern const struct dex_app_type dex_app[26];
extern const struct con_app_type con_app[26];
extern const struct cha_app_type cha_app[26];
extern const struct lck_app_type lck_app[26];

extern struct race_type *race_table[MAX_RACE];
extern char *attack_table[DAM_MAX_TYPE];
extern char *attack_table_plural[DAM_MAX_TYPE];

extern char **const s_message_table[DAM_MAX_TYPE];
extern char **const p_message_table[DAM_MAX_TYPE];

extern char *const skill_tname[];
extern char *const dir_name[];
extern char *const short_dirname[];
extern char *const where_name[MAX_WHERE_NAME];
extern const short rev_dir[];
extern const int trap_door[];
extern char *const save_flag[];
extern char *const r_flags[];
extern char *const ex_flags[];
extern char *const w_flags[];
extern char *const item_w_flags[];
extern char *const o_flags[];
extern char *const a_flags[];
extern char *const o_types[];
extern char *const a_types[];
extern char *const act_flags[];
extern char *const plr_flags[];
extern char *const pc_flags[];
extern char *const trap_types[];
extern char *const trap_flags[];
extern char *const ris_flags[];
extern char *const trig_flags[];
extern char *const part_flags[];
extern char *const npc_race[];
extern char *const npc_class[];
extern char *const defense_flags[];
extern char *const attack_flags[];
extern char *const area_flags[];
extern char *const ex_pmisc[];
extern char *const ex_pwater[];
extern char *const ex_pair[];
extern char *const ex_pearth[];
extern char *const ex_pfire[];
extern char *const wear_locs[];

extern int const lang_array[];
extern char *const lang_names[];

extern char *const temp_settings[]; /* FB */
extern char *const precip_settings[];
extern char *const wind_settings[];
extern char *const preciptemp_msg[6][6];
extern char *const windtemp_msg[6][6];
extern char *const precip_msg[];
extern char *const wind_msg[];

/*
 * Global variables.
 */
/* 
 * Stuff for area versions --Shaddai
 */
#define HAS_SPELL_INDEX     -1
/* This is to tell if act uses uppercasestring or not --Shaddai */
extern bool DONT_UPPER;
extern bool MOBtrigger;
extern bool mud_down;
extern bool DONTSAVE;
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern const char echo_off_str[];
extern int top_area;
extern int top_mob_index;
extern int top_obj_index;
extern int top_room;
extern char str_boot_time[];
extern CHAR_DATA *timechar;
extern bool fBootDb;
extern char strArea[MIL];
extern int falling;

extern char *target_name;
extern char *ranged_target_name;
extern int numobjsloaded;
extern int nummobsloaded;
extern int physicalobjects;
extern int last_pkroom;
extern int num_descriptors;
extern struct system_data sysdata;
extern int top_sn;
extern int top_herb;

extern CMDTYPE *command_hash[126];
extern struct class_type *class_table[MAX_CLASS];
extern char *title_table[MAX_CLASS][MAX_LEVEL + 1][2];

extern SKILLTYPE *skill_table[MAX_SKILL];
extern SOCIALTYPE *social_index[27];
extern ch_ret global_retcode;
extern SKILLTYPE *herb_table[MAX_HERB];
extern SKILLTYPE *disease_table[MAX_DISEASE];

extern CHAR_DATA *first_char;
extern CHAR_DATA *last_char;
extern DESCRIPTOR_DATA *first_descriptor;
extern DESCRIPTOR_DATA *last_descriptor;
extern OBJ_DATA *first_object;
extern OBJ_DATA *last_object;
extern AREA_DATA *first_area;
extern AREA_DATA *last_area;
extern AREA_DATA *first_area_nsort;
extern AREA_DATA *last_area_nsort;
extern AREA_DATA *first_area_vsort;
extern AREA_DATA *last_area_vsort;
extern TELEPORT_DATA *first_teleport;
extern TELEPORT_DATA *last_teleport;
extern OBJ_DATA *save_equipment[MAX_WEAR][MAX_LAYERS];
extern OBJ_DATA *mob_save_equipment[MAX_WEAR][MAX_LAYERS];
extern CHAR_DATA *quitting_char;
extern CHAR_DATA *loading_char;
extern CHAR_DATA *saving_char;

extern time_t current_time;
extern bool fLogAll;
extern TIME_INFO_DATA time_info;
extern WEATHER_DATA weather_info;
extern int weath_unit;
extern int rand_factor;
extern int climate_factor;
extern int neigh_factor;
extern int max_vector;

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
/* Actually.... it's not. If it were, then there would be ALOT more here.
 * As it stands, some of this didn't need to be globally aware, so there's even less.
 */
#define CD	CHAR_DATA
#define MID	MOB_INDEX_DATA
#define OD	OBJ_DATA
#define OID	OBJ_INDEX_DATA
#define RID	ROOM_INDEX_DATA
#define CL struct clan_data
#define CO struct council_data
#define EDD	EXTRA_DESCR_DATA
#define RD	RESET_DATA
#define ED	EXIT_DATA
#define	ST	SOCIALTYPE
#define SK	SKILLTYPE

/* Formatted log output - 3-07-02 */
void log_printf( const char *fmt, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );

/* act_comm.c */
bool is_same_group( CHAR_DATA * ach, CHAR_DATA * bch );
int knows_language( CHAR_DATA * ch, int language, CHAR_DATA * cch );

/* act_info.c */
char *format_obj_to_char( OBJ_DATA * obj, CHAR_DATA * ch, bool fShort );
void show_list_to_char( OBJ_DATA * list, CHAR_DATA * ch, bool fShort, bool fShowNothing );
bool is_ignoring( CHAR_DATA * ch, CHAR_DATA * ign_ch );

/* act_move.c */
ED *find_door( CHAR_DATA * ch, char *arg, bool quiet );
ED *get_exit( ROOM_INDEX_DATA * room, short dir );
ED *get_exit_to( ROOM_INDEX_DATA * room, short dir, int vnum );
ch_ret move_char( CHAR_DATA * ch, EXIT_DATA * pexit, int fall, int direction, bool running );
char *rev_exit( short vdir );
int get_dirnum( char *flag );

/* act_obj.c */
short get_obj_resistance( OBJ_DATA * obj );
int item_ego( OBJ_DATA * obj );
int char_ego( CHAR_DATA * ch );

/* act_wiz.c */
RID *find_location( CHAR_DATA * ch, char *arg );
void echo_to_all( short AT_COLOR, char *argument, short tar );
void echo_all_printf( short AT_COLOR, short tar, char *Str, ... );

/* build.c */
int get_risflag( char *flag );
int get_defenseflag( char *flag );
int get_attackflag( char *flag );
int get_npc_sex( char *sex );
int get_npc_position( char *position );
int get_npc_class( char *Class );
int get_npc_race( char *race );
int get_pc_race( char *type );
int get_pc_class( char *Class );
int get_actflag( char *flag );
int get_pcflag( char *flag );
int get_plrflag( char *flag );
int get_langnum( char *flag );
int get_rflag( char *flag );
int get_exflag( char *flag );
int get_sectypes( char *sector );
int get_areaflag( char *flag );
int get_partflag( char *flag );
int get_magflag( char *flag );
int get_otype( const char *type );
int get_aflag( char *flag );
int get_cmdflag( char *flag );
int get_atype( char *type );
int get_oflag( char *flag );
int get_wflag( char *flag );
int get_dir( char *txt );
char *flag_string( int bitvector, char *const flagarray[] );
char *ext_flag_string( EXT_BV * bitvector, char *const flagarray[] );
char *strip_cr( char *str );
bool can_rmodify( CHAR_DATA * ch, ROOM_INDEX_DATA * room );
bool can_omodify( CHAR_DATA * ch, OBJ_DATA * obj );
bool can_mmodify( CHAR_DATA * ch, CHAR_DATA * mob );
void assign_area( CHAR_DATA * ch );
void fold_area( AREA_DATA * tarea, char *filename, bool install );

/* calendar.c */
char *c_time( time_t curtime, int tz );

/* channels.c */
int hasname( const char *list, const char *name );

/* color.c */
void send_to_char( const char *txt, CHAR_DATA * ch );
void send_to_char_color( const char *txt, CHAR_DATA * ch );
void send_to_pager( const char *txt, CHAR_DATA * ch );
void send_to_pager_color( const char *txt, CHAR_DATA * ch );
void ch_printf( CHAR_DATA * ch, const char *fmt, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void pager_printf( CHAR_DATA * ch, const char *fmt, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );

/* comm.c */
bool check_parse_name( char *name, bool newchar );
void close_socket( DESCRIPTOR_DATA * dclose, bool force );
void write_to_buffer( DESCRIPTOR_DATA * d, const char *txt, unsigned int length );
void buffer_printf( DESCRIPTOR_DATA * d, const char *fmt, ... );
void act( short AType, const char *format, CHAR_DATA * ch, void *arg1, void *arg2, int type );
void act_printf( short AType, CHAR_DATA * ch, void *arg1, void *arg2, int type, const char *str, ... );
void ch_printf_color( CHAR_DATA * ch, const char *fmt, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void pager_printf_color( CHAR_DATA * ch, const char *fmt, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
char *obj_short( OBJ_DATA * obj );

/* db.c */
bool exists_file( char *name );
void shutdown_mud( char *reason );
void log_printf_plus( short log_type, short level, const char *fmt, ... );
char fread_letter( FILE * fp );
int fread_number( FILE * fp );
short fread_short( FILE * fp );
long fread_long( FILE * fp );
float fread_float( FILE * fp );
char *fread_string( FILE * fp );
char *fread_flagstring( FILE * fp );
char *fread_string_nohash( FILE * fp );
void fread_to_eol( FILE * fp );
char *fread_line( FILE * fp );
char *fread_word( FILE * fp );
EXT_BV fread_bitvector( FILE * fp );
char *print_bitvector( EXT_BV * bits );
bool is_valid_filename( CHAR_DATA *ch, const char *direct, const char *filename );
void show_file( CHAR_DATA * ch, char *filename );
CD *create_mobile( MOB_INDEX_DATA * pMobIndex );
OD *create_object( OBJ_INDEX_DATA * pObjIndex, int level );
void clear_char( CHAR_DATA * ch );
void free_char( CHAR_DATA * ch );
char *get_extra_descr( const char *name, EXTRA_DESCR_DATA * ed );
MID *get_mob_index( int vnum );
OID *get_obj_index( int vnum );
RID *get_room_index( int vnum );
int number_fuzzy( int number );
int number_range( int from, int to );
int number_percent( void );
int number_door( void );
int number_bits( int width );
int dice( int number, int size );
void append_file( CHAR_DATA * ch, char *file, char *fmt, ... );
void append_to_file( char *file, char *fmt, ... );
void bug( const char *str, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void log_string_plus( const char *str, short log_type, short level );
RID *make_room( int, AREA_DATA* );
OID *make_object( int vnum, int cvnum, char *name );
MID *make_mobile( int vnum, int cvnum, char *name );
ED *make_exit( ROOM_INDEX_DATA * pRoomIndex, ROOM_INDEX_DATA * to_room, short door );
void fix_area_exits( AREA_DATA * tarea );
void randomize_exits( ROOM_INDEX_DATA * room, short maxdir );
void make_wizlist( void );
void tail_chain( void );
AREA_DATA *create_area( void );
void sort_area_name( AREA_DATA * pArea );
void sort_area_vnums( AREA_DATA * pArea );

/* editor.c */
char *str_dup( const char *str );
void stralloc_printf( char **pointer, char *fmt, ... );
void strdup_printf( char **pointer, char *fmt, ... );
size_t mudstrlcpy( char *dst, const char *src, size_t siz );
size_t mudstrlcat( char *dst, const char *src, size_t siz );
void smash_tilde( char *str );
void hide_tilde( char *str );
char *show_tilde( const char *str );
void editor_desc_printf( CHAR_DATA * ch, char *desc_fmt, ... );
void start_editing( CHAR_DATA * ch, char *data );
void stop_editing( CHAR_DATA * ch );
char *copy_buffer( CHAR_DATA * ch );
char *copy_buffer_nohash( CHAR_DATA * ch );
void set_editor_desc( CHAR_DATA * ch, char *new_desc );
char *strrep( const char *src, const char *sch, const char *rep );
char *strrepa( const char *src, const char *sch[], const char *rep[] );
bool str_cmp( const char *astr, const char *bstr );
bool str_prefix( const char *astr, const char *bstr );
bool str_infix( const char *astr, const char *bstr );
bool str_suffix( const char *astr, const char *bstr );
char *capitalize( const char *str );
char *strlower( const char *str );
char *strupper( const char *str );
char *aoran( const char *str );

/* fight.c */
void stop_hunting( CHAR_DATA * ch );
void stop_hating( CHAR_DATA * ch );
void stop_fearing( CHAR_DATA * ch );
void stop_fighting( CHAR_DATA * ch, bool fBoth );
CHAR_DATA *who_fighting( CHAR_DATA * ch );
void update_pos( CHAR_DATA * victim );
ch_ret multi_hit( CHAR_DATA * ch, CHAR_DATA * victim, int dt );
ch_ret damage( CHAR_DATA * ch, CHAR_DATA * victim, double dam, int dt );

/* handler.c */
long exp_level( int level );
short get_trust( CHAR_DATA * ch );
short get_age( CHAR_DATA * ch );
short calculate_age( CHAR_DATA * ch );
short get_curr_str( CHAR_DATA * ch );
short get_curr_int( CHAR_DATA * ch );
short get_curr_wis( CHAR_DATA * ch );
short get_curr_dex( CHAR_DATA * ch );
short get_curr_con( CHAR_DATA * ch );
short get_curr_cha( CHAR_DATA * ch );
short get_curr_lck( CHAR_DATA * ch );
bool can_take_proto( CHAR_DATA * ch );
int can_carry_n( CHAR_DATA * ch );
int can_carry_w( CHAR_DATA * ch );
bool is_name( const char *str, char *namelist );
bool is_name_prefix( const char *str, char *namelist );
bool nifty_is_name( char *str, const char *namelist );
bool nifty_is_name_prefix( char *str, char *namelist );
void affect_to_char( CHAR_DATA * ch, AFFECT_DATA * paf );
void affect_remove( CHAR_DATA * ch, AFFECT_DATA * paf );
void affect_strip( CHAR_DATA * ch, int sn );
bool is_affected( CHAR_DATA * ch, int sn );
void affect_join( CHAR_DATA * ch, AFFECT_DATA * paf );
void char_from_room( CHAR_DATA * ch );
bool char_to_room( CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex );
OD *obj_to_char( OBJ_DATA * obj, CHAR_DATA * ch );
void obj_from_char( OBJ_DATA * obj );
int apply_ac( OBJ_DATA * obj, int iWear );
OD *get_eq_char( CHAR_DATA * ch, int iWear );
void equip_char( CHAR_DATA * ch, OBJ_DATA * obj, int iWear );
void unequip_char( CHAR_DATA * ch, OBJ_DATA * obj );
void obj_from_room( OBJ_DATA * obj );
OD *obj_to_room( OBJ_DATA * obj, ROOM_INDEX_DATA * pRoomIndex, CHAR_DATA * ch );
OD *obj_to_obj( OBJ_DATA * obj, OBJ_DATA * obj_to );
void obj_from_obj( OBJ_DATA * obj );
void extract_obj( OBJ_DATA * obj );
void extract_exit( ROOM_INDEX_DATA * room, EXIT_DATA * pexit );
void clean_room( ROOM_INDEX_DATA * room );
void clean_obj( OBJ_INDEX_DATA * obj );
void clean_mob( MOB_INDEX_DATA * mob );
void clean_resets( AREA_DATA * tarea );
void extract_char( CHAR_DATA * ch, bool fPull );
CD *get_char_room( CHAR_DATA * ch, char *argument );
CD *get_char_world( CHAR_DATA * ch, char *argument );
OD *get_obj_list( CHAR_DATA * ch, char *argument, OBJ_DATA * list );
OD *get_obj_list_rev( CHAR_DATA * ch, char *argument, OBJ_DATA * list );
OD *get_obj_carry( CHAR_DATA * ch, char *argument );
OD *get_obj_wear( CHAR_DATA * ch, char *argument );
OD *get_obj_vnum( CHAR_DATA * ch, int vnum );
OD *get_obj_here( CHAR_DATA * ch, char *argument );
OD *get_obj_world( CHAR_DATA * ch, char *argument );
int get_obj_number( OBJ_DATA * obj );
int get_obj_weight( OBJ_DATA * obj );
int get_real_obj_weight( OBJ_DATA * obj );
bool room_is_dark( ROOM_INDEX_DATA * pRoomIndex, CHAR_DATA * ch );
bool room_is_private( ROOM_INDEX_DATA * pRoomIndex );
bool can_see( CHAR_DATA * ch, CHAR_DATA * victim, bool override );
bool can_see_obj( CHAR_DATA * ch, OBJ_DATA * obj, bool override );
bool can_drop_obj( CHAR_DATA * ch, OBJ_DATA * obj );
char *item_type_name( OBJ_DATA * obj );
char *affect_loc_name( int location );
const char *affect_bit_name( EXT_BV * vector );
const char *extra_bit_name( EXT_BV * extra_flags );
const char *magic_bit_name( int magic_flags );
ch_ret check_for_trap( CHAR_DATA * ch, OBJ_DATA * obj, int flag );
bool is_trapped( OBJ_DATA * obj );
OD *get_trap( OBJ_DATA * obj );
ch_ret spring_trap( CHAR_DATA * ch, OBJ_DATA * obj );
void showaffect( CHAR_DATA * ch, AFFECT_DATA * paf );
void set_cur_obj( OBJ_DATA * obj );
bool obj_extracted( OBJ_DATA * obj );
bool char_died( CHAR_DATA * ch );
void add_timer( CHAR_DATA * ch, short type, short count, DO_FUN * fun, int value );
TIMER *get_timerptr( CHAR_DATA * ch, short type );
short get_timer( CHAR_DATA * ch, short type );
void extract_timer( CHAR_DATA * ch, TIMER * timer );
void remove_timer( CHAR_DATA * ch, short type );
bool in_hard_range( CHAR_DATA * ch, AREA_DATA * tarea );
bool chance( CHAR_DATA * ch, short percent );
OD *clone_object( OBJ_DATA * obj );
void split_obj( OBJ_DATA * obj, int num );
void separate_obj( OBJ_DATA * obj );
bool empty_obj( OBJ_DATA * obj, OBJ_DATA * destobj, ROOM_INDEX_DATA * destroom );
OD *find_obj( CHAR_DATA * ch, char *argument, bool carryonly );
bool ms_find_obj( CHAR_DATA * ch );
void worsen_mental_state( CHAR_DATA * ch, int mod );
void better_mental_state( CHAR_DATA * ch, int mod );
void update_aris( CHAR_DATA * ch );
AREA_DATA *get_area( char *name );  /* FB */
OD *get_objtype( CHAR_DATA * ch, short type );
void check_switches( bool possess );
void check_switch( CHAR_DATA *ch, bool possess );

/* hashstr.c */
char *str_alloc( const char *str );
char *quick_link( char *str );
int str_free( char *str );
void show_hash( int count );
char *hash_stats( void );
char *check_hash( char *str );
void hash_dump( int hash );
void show_high_hash( int top );
bool in_hash_table( char *str );

/* interp.c */
void cmdf( CHAR_DATA * ch, char *fmt, ... );
void funcf( CHAR_DATA * ch, DO_FUN * cmd, char *fmt, ... );
bool check_pos( CHAR_DATA * ch, short position );
void interpret( CHAR_DATA * ch, char *argument );
bool is_number( const char *arg );
int number_argument( char *argument, char *arg );
char *one_argument( char *argument, char *arg_first );
char *one_argument2( char *argument, char *arg_first );
ST *find_social( char *command );
CMDTYPE *find_command( char *command );

/* magic.c */
bool can_charm( CHAR_DATA * ch );
bool process_spell_components( CHAR_DATA * ch, int sn );
int find_spell( CHAR_DATA * ch, const char *name, bool know );
int skill_lookup( const char *name );
int herb_lookup( const char *name );
int slot_lookup( int slot );
int bsearch_skill_exact( const char *name, int first, int top );
bool saves_poison_death( int level, CHAR_DATA * victim );
bool saves_para_petri( int level, CHAR_DATA * victim );
bool saves_breath( int level, CHAR_DATA * victim );
bool saves_spell_staff( int level, CHAR_DATA * victim );
ch_ret obj_cast_spell( int sn, int level, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj );
int dice_parse( CHAR_DATA * ch, int level, char *xexp );
SK *get_skilltype( int sn );

/* misc.c */
EXT_BV meb( int bit );
EXT_BV multimeb( int bit, ... );

/* mud_comm.c */
char *mprog_type_to_name( int type );

/* mud_prog.c */
void mprog_entry_trigger( CHAR_DATA * mob );
void mprog_greet_trigger( CHAR_DATA * mob );
void progbug( char *str, CHAR_DATA * mob );
void progbugf( CHAR_DATA * mob, char *str, ... );
void rset_supermob( ROOM_INDEX_DATA * room );
void release_supermob( void );

/* overland.c */
bool is_same_map( CHAR_DATA * ch, CHAR_DATA * victim );
void fix_maps( CHAR_DATA * ch, CHAR_DATA * victim );
void enter_map( CHAR_DATA * ch, EXIT_DATA * pexit, int x, int y, int continent );
void leave_map( CHAR_DATA * ch, CHAR_DATA * victim, ROOM_INDEX_DATA * target );
void collect_followers( CHAR_DATA * ch, ROOM_INDEX_DATA * from, ROOM_INDEX_DATA * to );

/* player.c */
void unbind_follower( CHAR_DATA * mob, CHAR_DATA * ch );
void bind_follower( CHAR_DATA * mob, CHAR_DATA * ch, int sn, int duration );
char *condtxt( int current, int base );
bool has_visited( CHAR_DATA * ch, AREA_DATA * area );
char *get_class( CHAR_DATA * ch );
char *get_race( CHAR_DATA * ch );
void set_title( CHAR_DATA * ch, char *title );
int IsHumanoid( CHAR_DATA * ch );
int IsRideable( CHAR_DATA * ch );
int IsUndead( CHAR_DATA * ch );
int IsDragon( CHAR_DATA * ch );
int IsGoodSide( CHAR_DATA * ch );
int IsBadSide( CHAR_DATA * ch );
int race_bodyparts( CHAR_DATA * ch );

/* reset.c */
RD *add_reset( ROOM_INDEX_DATA * room, char letter, int extra, int arg1, int arg2, int arg3, short arg4, short arg5,
               short arg6 );
void reset_area( AREA_DATA * pArea );

/* save.c */
/* object saving defines for fread/write_obj. -- Altrag */
#define OS_CARRY	0
#define OS_CORPSE	1
void save_char_obj( CHAR_DATA * ch );
bool load_char_obj( DESCRIPTOR_DATA * d, char *name, bool preload, bool copyover );
void de_equip_char( CHAR_DATA * ch );
void re_equip_char( CHAR_DATA * ch );

/* skills.c */
void learn_from_failure( CHAR_DATA * ch, int sn );
void disarm( CHAR_DATA * ch, CHAR_DATA * victim );

/* tables.c */
int get_skill( char *skilltype );
void save_skill_table( void );
void sort_skill_table( void );
SPELL_FUN *spell_function( char *name );
DO_FUN *skill_function( char *name );
SPEC_FUN *m_spec_lookup( char *name );
void write_class_file( int cl );
void save_classes( void );
void save_races( void );
void load_classes( void );
void load_herb_table( void );
void save_herb_table( void );
void load_races( void );
void load_tongues( void );

/* track.c */
void found_prey( CHAR_DATA * ch, CHAR_DATA * victim );

/* update.c */
void advance_level( CHAR_DATA * ch );
void gain_exp( CHAR_DATA * ch, double gain );
void gain_condition( CHAR_DATA * ch, int iCond, int value );
void update_handler( void );
void remove_portal( OBJ_DATA * portal );
void weather_update( void );

#undef	SK
#undef	CO
#undef	ST
#undef	CD
#undef	MID
#undef	OD
#undef	OID
#undef	RID
#undef	BD
#undef	CL
#undef	EDD
#undef	RD
#undef	ED

/*
 * mudprograms stuff
 */
extern CHAR_DATA *supermob;
extern OBJ_DATA *supermob_obj;

/*
 * MUD_PROGS START HERE
 * (object stuff)
 */
void oprog_greet_trigger( CHAR_DATA * ch );
void oprog_speech_trigger( char *txt, CHAR_DATA * ch );
void oprog_random_trigger( OBJ_DATA * obj );
void oprog_month_trigger( OBJ_DATA * obj );
void oprog_remove_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
void oprog_sac_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
void oprog_get_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
void oprog_damage_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
void oprog_repair_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
void oprog_drop_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
void oprog_examine_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
void oprog_zap_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
void oprog_pull_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
void oprog_push_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
void oprog_and_speech_trigger( char *txt, CHAR_DATA * ch );
void oprog_wear_trigger( CHAR_DATA * ch, OBJ_DATA * obj );
bool oprog_use_trigger( CHAR_DATA * ch, OBJ_DATA * obj, CHAR_DATA * vict, OBJ_DATA * targ, void *vo );

/* mud prog defines */
#define ERROR_PROG        -1
#define IN_FILE_PROG      -2

typedef enum
{
   ACT_PROG, SPEECH_PROG, RAND_PROG, FIGHT_PROG, DEATH_PROG, HITPRCNT_PROG,
   ENTRY_PROG, GREET_PROG, ALL_GREET_PROG, GIVE_PROG, BRIBE_PROG, HOUR_PROG,
   TIME_PROG, WEAR_PROG, REMOVE_PROG, SAC_PROG, LOOK_PROG, EXA_PROG, ZAP_PROG,
   GET_PROG, DROP_PROG, DAMAGE_PROG, REPAIR_PROG, RANDIW_PROG, SPEECHIW_PROG,
   PULL_PROG, PUSH_PROG, SLEEP_PROG, REST_PROG, LEAVE_PROG, SCRIPT_PROG,
   USE_PROG, SPEECH_AND_PROG, MONTH_PROG, KEYWORD_PROG
} prog_types;

/*
 * For backwards compatability
 */
#define RDEATH_PROG DEATH_PROG
#define ENTER_PROG  ENTRY_PROG
#define RFIGHT_PROG FIGHT_PROG
#define RGREET_PROG GREET_PROG
#define OGREET_PROG GREET_PROG

void rprog_leave_trigger( CHAR_DATA * ch );
void rprog_enter_trigger( CHAR_DATA * ch );
void rprog_sleep_trigger( CHAR_DATA * ch );
void rprog_rest_trigger( CHAR_DATA * ch );
void rprog_rfight_trigger( CHAR_DATA * ch );
void rprog_death_trigger( CHAR_DATA * killer, CHAR_DATA * ch );
void rprog_speech_trigger( char *txt, CHAR_DATA * ch );
void rprog_random_trigger( CHAR_DATA * ch );
void rprog_time_trigger( CHAR_DATA * ch );
void rprog_month_trigger( CHAR_DATA * ch );
void rprog_hour_trigger( CHAR_DATA * ch );
void rprog_and_speech_trigger( char *txt, CHAR_DATA * ch );
void oprog_act_trigger( char *buf, OBJ_DATA * mobj, CHAR_DATA * ch, OBJ_DATA * obj, void *vo );
void rprog_act_trigger( char *buf, ROOM_INDEX_DATA * room, CHAR_DATA * ch, OBJ_DATA * obj, void *vo );

#define GET_ADEPT(ch,sn)    (  skill_table[(sn)]->skill_adept[(ch)->Class])
#define LEARNED(ch,sn)	    (IS_NPC(ch) ? 80 : URANGE(0, ch->pcdata->learned[sn], 101))
