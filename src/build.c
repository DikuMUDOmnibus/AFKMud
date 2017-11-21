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
 *                   Online Building and Editing Module                     *
 ****************************************************************************/

#include <string.h>
#include <unistd.h>
#include "mud.h"
#include "calendar.h"
#include "clans.h"
#include "deity.h"
#include "mud_prog.h"
#include "overland.h"
#include "shops.h"
#include "treasure.h"

extern int top_affect;
extern int top_reset;
extern int top_ed;

REL_DATA *first_relation = NULL;
REL_DATA *last_relation = NULL;

int set_obj_rent( OBJ_INDEX_DATA * obj );
void build_wizinfo( void );   /* For mset realm option - Samson 6-6-99 */
CMDF do_rstat( CHAR_DATA * ch, char *argument );
CMDF do_mstat( CHAR_DATA * ch, char *argument );
CMDF do_ostat( CHAR_DATA * ch, char *argument );
bool validate_spec_fun( char *name );
void save_clan( CLAN_DATA * clan );
CLAN_DATA *get_clan( char *name );
COUNCIL_DATA *get_council( char *name );
DEITY_DATA *get_deity( char *name );
int mob_xp( CHAR_DATA * mob );
void armorgen( OBJ_DATA * obj );
void weapongen( OBJ_DATA * obj );
char *sprint_reset( RESET_DATA * pReset, short &num );
void delete_room( ROOM_INDEX_DATA * room );
void delete_obj( OBJ_INDEX_DATA * obj );
void delete_mob( MOB_INDEX_DATA * mob );
void close_area( AREA_DATA * pArea );
void fix_exits( void );
void affect_modify( CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd );

/*
 * Exit Pull/push types
 * (water, air, earth, fire)
 */
char *const ex_pmisc[] = { "undefined", "vortex", "vacuum", "slip", "ice", "mysterious" };

char *const ex_pwater[] = { "current", "wave", "whirlpool", "geyser" };

char *const ex_pair[] = { "wind", "storm", "coldwind", "breeze" };

char *const ex_pearth[] = { "landslide", "sinkhole", "quicksand", "earthquake" };

char *const ex_pfire[] = { "lava", "hotair" };

/* Stuff that isn't from stock Smaug */

char *const npc_sex[SEX_MAX] = {
   "neuter", "male", "female", "hermaphrodyte"
};

char *const npc_position[POS_MAX] = {
   "dead", "mortal", "incapacitated", "stunned", "sleeping",
   "resting", "sitting", "berserk", "aggressive", "fighting", "defensive",
   "evasive", "standing", "mounted", "shove", "drag"
};

char *const campgear[GEAR_MAX] = {
   "none", "bedroll", "misc gear", "firewood"
};

char *const ores[ORE_MAX] = {
   "none", "iron", "gold", "silver", "adamantine", "mithril", "blackmite",
   "titanium", "steel", "bronze", "dwarven", "elven"
};

char *const container_flags[] = {
   "closeable", "pickproof", "closed", "locked", "eatkey", "r1", "r2", "r3",
   "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
   "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25", "r26",
   "r27"
};

char *weapon_skills[WEP_MAX] = {
   "Barehand", "Sword", "Dagger", "Whip", "Talon",
   "Mace", "Archery", "Blowgun", "Sling", "Axe", "Spear",
   "Staff"
};

char *projectiles[PROJ_MAX] = {
   "Bolt", "Arrow", "Dart", "Stone"
};

/* Area continent table for continent/plane system */
char *const continents[] = {
   "alsherok", "eletar", "alatia", "r1", "r2", "r3", "r4",
   "r5", "astral", "past", "immortal", "varsis"
};


char *const log_flag[] = {
   "normal", "always", "never", "build", "high", "comm", "warn", "all"
};

char *const furniture_flags[] = {
   "sit_on", "sit_in", "sit_at",
   "stand_on", "stand_in", "stand_at",
   "sleep_on", "sleep_in", "sleep_at",
   "rest_on", "rest_in", "rest_at"
};

char *const interfaces[] = {
   "Dale", "Smaug", "AFKMud"
};

/* End of non-stock stuff */

char *const save_flag[] = {
   "death", "kill", "passwd", "drop", "put", "give", "auto", "zap",
   "auction", "get", "receive", "idle", "r12", "r13", "fill",
   "empty", "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24",
   "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

char *const item_w_flags[] = {
   "take", "finger", "finger", "neck", "neck", "body", "head", "legs", "feet",
   "hands", "arms", "shield", "about", "waist", "wrist", "wrist", "wield",
   "hold", "dual", "ears", "eyes", "missile", "back", "face", "ankle", "ankle",
   "lodge_rib", "lodge_arm", "lodge_leg", "r7", "r8", "r9", "r10", "r11", "r12", "r13"
};

char *const ex_flags[] = {
   "isdoor", "closed", "locked", "secret", "swim", "pickproof", "fly", "climb",
   "dig", "eatkey", "nopassdoor", "hidden", "passage", "portal", "overland", "arrowslit",
   "can_climb", "can_enter", "can_leave", "auto", "noflee", "searchable",
   "bashed", "bashproof", "nomob", "window", "can_look", "isbolt", "bolted",
   "fortified", "heavy", "medium", "light", "crumbling", "destroyed", ""
};

char *const r_flags[] = {
   "dark", "death", "nomob", "indoors", "safe", "nocamp", "nosummon",
   "nomagic", "tunnel", "private", "silence", "nosupplicate", "arena", "nomissile",
   "norecall", "noportal", "noastral", "nodrop", "clanstoreroom", "teleport",
   "teleshowdesc", "nofloor", "solitary", "petshop", "donation", "nodropall",
   "logspeech", "proto", "noteleport", "noscry", "cave", "cavern", "nobeacon",
   "auction", "map", "forge", "guildinn", "isolated", "watchtower",
   "noquit", "telenofly", ""
};

char *const o_flags[] = {
   "glow", "hum", "metal", "mineral", "organic", "invis", "magic", "nodrop", "bless",
   "antigood", "antievil", "antineutral", "anticleric", "antimage",
   "antirogue", "antiwarrior", "inventory", "noremove", "twohand", "evil",
   "donated", "clanobject", "clancorpse", "antibard", "hidden",
   "antidruid", "poisoned", "covering", "deathrot", "buried", "proto",
   "nolocate", "groundrot", "antimonk", "loyal", "brittle", "resistant",
   "immune", "antimen", "antiwomen", "antineuter", "antiherma", "antisun", "antiranger",
   "antipaladin", "antinecro", "antiapal", "onlycleric", "onlymage", "onlyrogue",
   "onlywarrior", "onlybard", "onlydruid", "onlymonk", "onlyranger", "onlypaladin",
   "onlynecro", "onlyapal", "auction", "onmap", "personal", "lodged", "sindhae",
   "mustmount", "noauction", ""
};

char *const mag_flags[] = {
   "returning", "backstabber", "bane", "loyal", "haste", "drain",
   "lightning_blade", "r7", "r8", "r9", "r10", "r11", "r12", "r13",
   "r14", "r15", "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
   "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

char *const w_flags[] = {
   "take", "finger", "neck", "body", "head", "legs", "feet", "hands", "arms",
   "shield", "about", "waist", "wrist", "wield", "hold", "dual", "ears", "eyes",
   "missile", "back", "face", "ankle", "lodge_rib", "lodge_arm", "lodge_leg",
   "r7", "r8", "r9", "r10", "r11", "r12", "r13"
};

/* Area Flags for continent and plane system - Samson 3-28-98 */
char *const area_flags[] = {
   "nopkill", "nocamp", "noastral", "noportal", "norecall", "nosummon", "noscry",
   "noteleport", "arena", "nobeacon", "noquit", "r11", "r12", "r13", "r14",
   "r15", "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24",
   "r25", "r26", "r27", "r28", "r29", "r30", "prototype"
};

char *const o_types[] = {
   "none", "light", "scroll", "wand", "staff", "weapon", "UNUSED1", "UNUSED2",
   "treasure", "armor", "potion", "clothing", "furniture", "trash", "UNUSED4",
   "container", "UNUSED5", "drinkcon", "key", "food", "money", "pen", "boat",
   "corpse", "corpse_pc", "fountain", "pill", "blood", "bloodstain",
   "scraps", "pipe", "herbcon", "herb", "incense", "fire", "book", "switch",
   "lever", "pullchain", "button", "dial", "rune", "runepouch", "match", "trap",
   "map", "portal", "paper", "tinder", "lockpick", "spike", "disease", "oil",
   "fuel", "piece", "tree", "missileweapon", "projectile", "quiver", "shovel",
   "salve", "cook", "keyring", "odor", "campgear", "drinkmix", "instrument", "ore"
};

char *const a_types[] = {
   "none", "strength", "dexterity", "intelligence", "wisdom", "constitution",
   "sex", "Class", "level", "age", "height", "weight", "mana", "hit", "move",
   "gold", "experience", "armor", "hitroll", "damroll", "save_poison", "save_rod",
   "save_para", "save_breath", "save_spell", "charisma", "affected", "resistant",
   "immune", "susceptible", "weaponspell", "luck", "backstab", "pick", "track",
   "steal", "sneak", "hide", "palm", "detrap", "dodge", "spellfail", "scan", "gouge",
   "search", "mount", "disarm", "kick", "parry", "bash", "stun", "punch", "climb",
   "grip", "scribe", "brew", "wearspell", "removespell", "emotion", "mentalstate",
   "stripsn", "remove", "dig", "full", "thirst", "drunk", "hitregen",
   "manaregen", "moveregen", "antimagic", "roomflag", "sectortype",
   "roomlight", "televnum", "teledelay", "cook", "recurringspell", "race", "hit-n-dam",
   "save_all", "eat_spell", "race_slayer", "align_slayer", "contagious",
   "ext_affect", "odor", "peek", "absorb", "attacks", "extragold", "allstats"
};

char *const a_flags[] = {
   "NONE", "blind", "invisible", "detect_evil", "detect_invis", "detect_magic",
   "detect_hidden", "hold", "sanctuary", "faerie_fire", "infrared", "curse",
   "spy", "poison", "protect", "paralysis", "sneak", "hide", "sleep",
   "charm", "flying", "acidmist", "floating", "truesight", "detect_traps",
   "scrying", "fireshield", "shockshield", "venomshield", "iceshield", "wizardeye", /* Max obj affect - stupid BV shit */
   "berserk", "aqua_breath", "recurringspell", "contagious", "bladebarrier",
   "silence", "animal_invis", "heat_stuff", "life_prot", "dragonride",
   "growth", "tree_travel", "travelling", "telepathy", "ethereal",
   "passdoor", "quiv", "_flaming", "haste", "slow", "elvensong", "bladesong",
   "reverie", "tenacity", "deathsong", "possess", "notrack", "enlighten", "treetalk",
   "spamguard", "bash", ""
};

char *const act_flags[] = {
   "npc", "sentinel", "scavenger", "innkeeper", "banker", "aggressive", "stayarea",
   "wimpy", "pet", "autonomous", "practice", "immortal", "deadly", "polyself",
   "meta_aggr", "guardian", "boarded", "nowander", "mountable", "mounted",
   "scholar", "secretive", "hardhat", "mobinvis", "noassist", "illusion",
   "pacifist", "noattack", "annoying", "auction", "proto", "mage", "warrior", "cleric",
   "rogue", "druid", "monk", "paladin", "ranger", "necromancer", "antipaladin",
   "huge", "greet", "teacher", "onmap", "smith", "guildauc", "guildbank", "guildvendor",
   "guildrepair", "guildforge", "idmob", "guildidmob"
};

char *const pc_flags[] = {
   "r1", "deadly", "unauthed", "norecall", "nointro", "gag", "retired", "guest",
   "nosummon", "pager", "notitled", "groupwho", "diagnose", "highgag", "watch",
   "nstart", "flags", "sector", "aname", "nobeep", "passdoor", "privacy",
   "notell", "checkboards", "noquote", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

char *const plr_flags[] = {
   "npc", "boughtpet", "shovedrag", "autoexits", "autoloot", "autosac", "blank",
   "UNUSED", "brief", "combine", "prompt", "telnet_ga", "holylight",
   "wizinvis", "roomvnum", "silence", "noemote", "boarded", "notell", "log",
   "deny", "freeze", "exempt", "onship", "litterbug", "ansi", "UNUSED3", "UNUSED3",
   "flee", "autogold", "ghost", "afk", "invisprompt", "busy", "autoassist",
   "smartsac", "idle", "onmap", "mapedit", "guildsplit", "groupsplit",
   "msp", "mxp", "compass", "mxpprompt", ""
};

char *const trap_types[] = {
   "Generic", "Poison Gas", "Poison Dart", "Poison Needle", "Poison Dagger",
   "Poison Arrow", "Blinding Gas", "Sleep Gas", "Flame", "Explosion",
   "Acid Spray", "Electric Shock", "Blade", "Sex Change"
};

char *const trap_flags[] = {
   "room", "obj", "enter", "leave", "open", "close", "get", "put", "pick",
   "unlock", "north", "south", "east", "west", "up", "down", "examine",
   "northeast", "northwest", "southeast", "southwest", "r6", "r7", "r8",
   "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

char *const cmd_flags[] = {
   "possessed", "polymorphed", "watch", "action", "nospam", "ghost", "mudprog",
   "noforce", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16",
   "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27",
   "r28", "r29", "r30"
};

char *const wear_locs[] = {
   "light", "finger1", "finger2", "neck1", "neck2", "body", "head", "legs",
   "feet", "hands", "arms", "shield", "about", "waist", "wrist1", "wrist2",
   "wield", "hold", "dual_wield", "ears", "eyes", "missile_wield", "back",
   "face", "ankle1", "ankle2"
};

char *const ris_flags[] = {
   "NONE", "fire", "cold", "electricity", "energy", "blunt", "pierce", "slash", "acid",
   "poison", "drain", "sleep", "charm", "hold", "nonmagic", "plus1", "plus2",
   "plus3", "plus4", "plus5", "plus6", "magic", "paralysis", "good", "evil", "hack",
   "lash", "r5", "r6", "r7", "r8", "r9", "r10"
};

char *const trig_flags[] = {
   "up", "unlock", "lock", "d_north", "d_south", "d_east", "d_west", "d_up",
   "d_down", "door", "container", "open", "close", "passage", "oload", "mload",
   "teleport", "teleportall", "teleportplus", "death", "cast", "fakeblade",
   "rand4", "rand6", "trapdoor", "anotherroom", "usedial", "absolutevnum",
   "showroomdesc", "autoreturn", "r2", "r3"
};

char *const part_flags[] = {
   "head", "arms", "legs", "heart", "brains", "guts", "hands", "feet", "fingers",
   "ear", "eye", "long_tongue", "eyestalks", "tentacles", "fins", "wings",
   "tail", "scales", "claws", "fangs", "horns", "tusks", "tailattack",
   "sharpscales", "beak", "haunches", "hooves", "paws", "forelegs", "feathers",
   "r1", "r2"
};

char *const attack_flags[] = {
   "bite", "claws", "tail", "sting", "punch", "kick", "trip", "bash", "stun",
   "gouge", "backstab", "age", "drain", "firebreath", "frostbreath",
   "acidbreath", "lightnbreath", "gasbreath", "poison", "nastypoison", "gaze",
   "blindness", "causeserious", "earthquake", "causecritical", "curse",
   "flamestrike", "harm", "fireball", "colorspray", "weaken", "spiralblast"
};

char *const defense_flags[] = {
   "parry", "dodge", "heal", "curelight", "cureserious", "curecritical",
   "dispelmagic", "dispelevil", "sanctuary", "r1", "r2", "shield", "bless",
   "stoneskin", "teleport", "r3", "r4", "r5", "r6", "disarm", "r7", "grip",
   "truesight", "r8", "r9", "r10", "r11", "r12"
};

/*
 * Note: I put them all in one big set of flags since almost all of these
 * can be shared between mobs, objs and rooms for the exception of
 * bribe and hitprcnt, which will probably only be used on mobs.
 * ie: drop -- for an object, it would be triggered when that object is
 * dropped; -- for a room, it would be triggered when anything is dropped
 *          -- for a mob, it would be triggered when anything is dropped
 *
 * Something to consider: some of these triggers can be grouped together,
 * and differentiated by different arguments... for example:
 *  hour and time, rand and randiw, speech and speechiw
 * 
 */
char *const mprog_flags[] = {
   "act", "speech", "rand", "fight", "death", "hitprcnt", "entry", "greet",
   "allgreet", "give", "bribe", "hour", "time", "wear", "remove", "sac",
   "look", "exa", "zap", "get", "drop", "damage", "repair", "randiw",
   "speechiw", "pull", "push", "sleep", "rest", "leave", "script", "use",
   "speechand", "month", "keyword"
};

/* Bamfin parsing code by Altrag, installed by Samson 12-10-97
   Allows use of $n where the player's name would be */
char *bamf_print( char *fmt, CHAR_DATA * ch )
{
   static char buf[MSL];

   mudstrlcpy( buf, strrep( fmt, "$n", ch->name ), MSL );
   mudstrlcpy( buf, strrep( buf, "$N", ch->name ), MSL );

   return buf;
}

/*
 * Relations created to fix a crash bug with oset on and rset on
 * code by: gfinello@mail.karmanet.it
 */
void RelCreate( relation_type tp, void *actor, void *subject )
{
   REL_DATA *tmp;

   if( tp < relMSET_ON || tp > relOSET_ON )
   {
      bug( "RelCreate: invalid type (%d)", tp );
      return;
   }
   if( !actor )
   {
      bug( "%s", "RelCreate: NULL actor" );
      return;
   }
   if( !subject )
   {
      bug( "%s", "RelCreate: NULL subject" );
      return;
   }
   for( tmp = first_relation; tmp; tmp = tmp->next )
      if( tmp->Type == tp && tmp->Actor == actor && tmp->Subject == subject )
      {
         bug( "%s", "RelCreate: duplicated relation" );
         return;
      }
   CREATE( tmp, REL_DATA, 1 );
   tmp->Type = tp;
   tmp->Actor = actor;
   tmp->Subject = subject;
   LINK( tmp, first_relation, last_relation, next, prev );
}

/*
 * Relations created to fix a crash bug with oset on and rset on
 * code by: gfinello@mail.karmanet.it
 */
void RelDestroy( relation_type tp, void *actor, void *subject )
{
   REL_DATA *rq;

   if( tp < relMSET_ON || tp > relOSET_ON )
   {
      bug( "RelDestroy: invalid type (%d)", tp );
      return;
   }
   if( !actor )
   {
      bug( "%s", "RelDestroy: NULL actor" );
      return;
   }
   if( !subject )
   {
      bug( "%s", "RelDestroy: NULL subject" );
      return;
   }
   for( rq = first_relation; rq; rq = rq->next )
      if( rq->Type == tp && rq->Actor == actor && rq->Subject == subject )
      {
         UNLINK( rq, first_relation, last_relation, next, prev );
         /*
          * Dispose will also set to NULL the passed parameter 
          */
         DISPOSE( rq );
         break;
      }
   return;
}

char *flag_string( int bitvector, char *const flagarray[] )
{
   static char buf[MSL];
   int x;

   buf[0] = '\0';
   for( x = 0; x < 32; x++ )
      if( IS_SET( bitvector, 1 << x ) )
      {
         mudstrlcat( buf, flagarray[x], MSL );
         /*
          * don't catenate a blank if the last char is blank  --Gorog 
          */
         if( buf[0] != '\0' && ' ' != buf[strlen( buf ) - 1] )
            mudstrlcat( buf, " ", MSL );
      }

   if( ( x = strlen( buf ) ) > 0 )
      buf[--x] = '\0';

   return buf;
}

char *ext_flag_string( EXT_BV * bitvector, char *const flagarray[] )
{
   static char buf[MSL];
   int x;

   buf[0] = '\0';
   for( x = 0; x < MAX_BITS; x++ )
      if( xIS_SET( *bitvector, x ) )
      {
         mudstrlcat( buf, flagarray[x], MSL );
         /*
          * don't catenate a blank if the last char is blank  --Gorog 
          */
         if( buf[0] != '\0' && ' ' != buf[strlen( buf ) - 1] )
            mudstrlcat( buf, " ", MSL );
      }

   if( ( x = strlen( buf ) ) > 0 )
      buf[--x] = '\0';

   return buf;
}

bool can_rmodify( CHAR_DATA * ch, ROOM_INDEX_DATA * room )
{
   int vnum = room->vnum;
   AREA_DATA *pArea;

   if( IS_NPC( ch ) )
      return FALSE;

   if( get_trust( ch ) >= sysdata.level_modify_proto )
      return TRUE;

   if( !IS_ROOM_FLAG( room, ROOM_PROTOTYPE ) )
   {
      send_to_char( "You cannot modify this room.\n\r", ch );
      return FALSE;
   }

   if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
   {
      send_to_char( "You must have an assigned area to modify this room.\n\r", ch );
      return FALSE;
   }

   if( vnum >= pArea->low_vnum && vnum <= pArea->hi_vnum )
      return TRUE;

   send_to_char( "That room is not in your allocated range.\n\r", ch );
   return FALSE;
}

bool can_omodify( CHAR_DATA * ch, OBJ_DATA * obj )
{
   int vnum = obj->pIndexData->vnum;
   AREA_DATA *pArea;

   if( IS_NPC( ch ) )
      return FALSE;

   if( get_trust( ch ) >= sysdata.level_modify_proto )
      return TRUE;

   if( !IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
   {
      send_to_char( "You cannot modify this object.\n\r", ch );
      return FALSE;
   }

   if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
   {
      send_to_char( "You must have an assigned area to modify this object.\n\r", ch );
      return FALSE;
   }

   if( vnum >= pArea->low_vnum && vnum <= pArea->hi_vnum )
      return TRUE;

   send_to_char( "That object is not in your allocated range.\n\r", ch );
   return FALSE;
}

bool can_mmodify( CHAR_DATA * ch, CHAR_DATA * mob )
{
   int vnum;
   AREA_DATA *pArea;

   if( mob == ch )
      return TRUE;

   if( !IS_NPC( mob ) )
   {
      if( get_trust( ch ) >= sysdata.level_modify_proto && get_trust( ch ) > get_trust( mob ) )
         return TRUE;
      else
         send_to_char( "You can't do that.\n\r", ch );
      return FALSE;
   }

   vnum = mob->pIndexData->vnum;

   if( IS_NPC( ch ) )
      return FALSE;

   if( get_trust( ch ) >= sysdata.level_modify_proto )
      return TRUE;

   if( !IS_ACT_FLAG( mob, ACT_PROTOTYPE ) )
   {
      send_to_char( "You cannot modify this mobile.\n\r", ch );
      return FALSE;
   }

   if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
   {
      send_to_char( "You must have an assigned area to modify this mobile.\n\r", ch );
      return FALSE;
   }

   if( vnum >= pArea->low_vnum && vnum <= pArea->hi_vnum )
      return TRUE;

   send_to_char( "That mobile is not in your allocated range.\n\r", ch );
   return FALSE;
}

int get_saveflag( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( save_flag ) / sizeof( save_flag[0] ); x++ )
      if( !str_cmp( name, save_flag[x] ) )
         return x;
   return -1;
}

int get_logflag( char *flag )
{
   int x;

   for( x = 0; x <= LOG_ALL; x++ )
      if( !str_cmp( flag, log_flag[x] ) )
         return x;
   return -1;
}

int get_npc_sex( char *sex )
{
   int x;

   for( x = 0; x < SEX_MAX; x++ )
      if( !str_cmp( sex, npc_sex[x] ) )
         return x;
   return -1;
}

int get_npc_position( char *position )
{
   int x;

   for( x = 0; x < POS_MAX; x++ )
      if( !str_cmp( position, npc_position[x] ) )
         return x;
   return -1;
}

int get_sectypes( char *sector )
{
   int x;

   for( x = 0; x < SECT_MAX; x++ )
      if( !str_cmp( sector, sect_types[x] ) )
         return x;
   return -1;
}

int get_pc_class( char *Class )
{
   int x;

   for( x = 0; x < MAX_PC_CLASS; x++ )
      if( !str_cmp( class_table[x]->who_name, Class ) )
         return x;
   return -1;
}

int get_npc_class( char *Class )
{
   int x;

   for( x = 0; x < MAX_NPC_CLASS; x++ )
      if( !str_cmp( Class, npc_class[x] ) )
         return x;
   return -1;
}

int get_continent( char *continent )
{
   int x;

   for( x = 0; x < ACON_MAX; x++ )
      if( !str_cmp( continent, continents[x] ) )
         return x;
   return -1;
}

int get_pc_race( char *type )
{
   int i;

   for( i = 0; i < MAX_PC_RACE; i++ )
      if( !str_cmp( type, race_table[i]->race_name ) )
         return i;
   return -1;
}

/* Used during bootup to convert magic flags to BV value - Samson 5-11-99 */
int get_magflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( mag_flags ) / sizeof( mag_flags[0] ) ); x++ )
      if( !str_cmp( flag, mag_flags[x] ) )
         return x;
   return -1;
}

int get_otype( const char *type )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( o_types ) / sizeof( o_types[0] ) ); x++ )
      if( !str_cmp( type, o_types[x] ) )
         return x;
   return -1;
}

int get_aflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( a_flags ) / sizeof( a_flags[0] ) ); x++ )
      if( !str_cmp( flag, a_flags[x] ) )
         return x;
   return -1;
}

int get_traptype( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( trap_types ) / sizeof( trap_types[0] ) ); x++ )
      if( !str_cmp( flag, trap_types[x] ) )
         return x;
   return -1;
}

int get_trapflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( trap_flags ) / sizeof( trap_flags[0] ) ); x++ )
      if( !str_cmp( flag, trap_flags[x] ) )
         return x;
   return -1;
}

int get_atype( char *type )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( a_types ) / sizeof( a_types[0] ) ); x++ )
      if( !str_cmp( type, a_types[x] ) )
         return x;
   return -1;
}

int get_npc_race( char *type )
{
   int x;

   for( x = 0; x < MAX_NPC_RACE; x++ )
      if( !str_cmp( type, npc_race[x] ) )
         return x;
   return -1;
}

int get_wearloc( char *type )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( wear_locs ) / sizeof( wear_locs[0] ) ); x++ )
      if( !str_cmp( type, wear_locs[x] ) )
         return x;
   return -1;
}

int get_exflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( ex_flags ) / sizeof( ex_flags[0] ) ); x++ )
      if( !str_cmp( flag, ex_flags[x] ) )
         return x;
   return -1;
}

int get_pulltype( char *type )
{
   unsigned int x;

   if( !str_cmp( type, "none" ) || !str_cmp( type, "clear" ) )
      return 0;

   for( x = 0; x < ( sizeof( ex_pmisc ) / sizeof( ex_pmisc[0] ) ); x++ )
      if( !str_cmp( type, ex_pmisc[x] ) )
         return x;

   for( x = 0; x < ( sizeof( ex_pwater ) / sizeof( ex_pwater[0] ) ); x++ )
      if( !str_cmp( type, ex_pwater[x] ) )
         return x + PT_WATER;
   for( x = 0; x < ( sizeof( ex_pair ) / sizeof( ex_pair[0] ) ); x++ )
      if( !str_cmp( type, ex_pair[x] ) )
         return x + PT_AIR;
   for( x = 0; x < ( sizeof( ex_pearth ) / sizeof( ex_pearth[0] ) ); x++ )
      if( !str_cmp( type, ex_pearth[x] ) )
         return x + PT_EARTH;
   for( x = 0; x < ( sizeof( ex_pfire ) / sizeof( ex_pfire[0] ) ); x++ )
      if( !str_cmp( type, ex_pfire[x] ) )
         return x + PT_FIRE;
   return -1;
}

int get_rflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( r_flags ) / sizeof( r_flags[0] ) ); x++ )
      if( !str_cmp( flag, r_flags[x] ) )
         return x;
   return -1;
}

int get_mpflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( mprog_flags ) / sizeof( mprog_flags[0] ) ); x++ )
      if( !str_cmp( flag, mprog_flags[x] ) )
         return x;
   return -1;
}

int get_oflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( o_flags ) / sizeof( o_flags[0] ) ); x++ )
      if( !str_cmp( flag, o_flags[x] ) )
         return x;
   return -1;
}

int get_areaflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( area_flags ) / sizeof( area_flags[0] ) ); x++ )
      if( !str_cmp( flag, area_flags[x] ) )
         return x;
   return -1;
}

int get_wflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( w_flags ) / sizeof( w_flags[0] ) ); x++ )
      if( !str_cmp( flag, w_flags[x] ) )
         return x;
   return -1;
}

int get_actflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( act_flags ) / sizeof( act_flags[0] ) ); x++ )
      if( !str_cmp( flag, act_flags[x] ) )
         return x;
   return -1;
}

int get_pcflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( pc_flags ) / sizeof( pc_flags[0] ) ); x++ )
      if( !str_cmp( flag, pc_flags[x] ) )
         return x;
   return -1;
}

int get_plrflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( plr_flags ) / sizeof( plr_flags[0] ) ); x++ )
      if( !str_cmp( flag, plr_flags[x] ) )
         return x;
   return -1;
}

int get_risflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( ris_flags ) / sizeof( ris_flags[0] ) ); x++ )
      if( !str_cmp( flag, ris_flags[x] ) )
         return x;
   return -1;
}

/*
 * For use with cedit --Shaddai
 */
int get_cmdflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( cmd_flags ) / sizeof( cmd_flags[0] ) ); x++ )
      if( !str_cmp( flag, cmd_flags[x] ) )
         return x;
   return -1;
}

int get_trigflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( trig_flags ) / sizeof( trig_flags[0] ) ); x++ )
      if( !str_cmp( flag, trig_flags[x] ) )
         return x;
   return -1;
}

int get_partflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( part_flags ) / sizeof( part_flags[0] ) ); x++ )
      if( !str_cmp( flag, part_flags[x] ) )
         return x;
   return -1;
}

int get_attackflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( attack_flags ) / sizeof( attack_flags[0] ) ); x++ )
      if( !str_cmp( flag, attack_flags[x] ) )
         return x;
   return -1;
}

int get_defenseflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( defense_flags ) / sizeof( defense_flags[0] ) ); x++ )
      if( !str_cmp( flag, defense_flags[x] ) )
         return x;
   return -1;
}

int get_langflag( char *flag )
{
   int x;

   for( x = 0; lang_array[x] != LANG_UNKNOWN; x++ )
      if( !str_cmp( flag, lang_names[x] ) )
         return lang_array[x];
   return LANG_UNKNOWN;
}
int get_langnum( char *flag )
{
   int x;

   for( x = 0; lang_array[x] != LANG_UNKNOWN; x++ )
      if( !str_cmp( flag, lang_names[x] ) )
         return x;
   return -1;
}

/*
 * Remove carriage returns from a line
 */
char *strip_cr( char *str )
{
   static char newstr[MSL];
   int i, j;

   if( !str || str[0] == '\0' )
      return "";

   for( i = j = 0; str[i] != '\0'; i++ )
      if( str[i] != '\r' )
         newstr[j++] = str[i];
   newstr[j] = '\0';
   return newstr;
}

char *obj_short( OBJ_DATA * obj )
{
   static char buf[MSL];

   if( obj->count > 1 )
   {
      snprintf( buf, MSL, "%s (%d)", obj->short_descr, obj->count );
      return buf;
   }
   return obj->short_descr;
}

void goto_char( CHAR_DATA * ch, CHAR_DATA * wch, char *argument )
{
   ROOM_INDEX_DATA *location;

   set_char_color( AT_IMMORT, ch );
   location = wch->in_room;

   if( is_ignoring( wch, ch ) )
   {
      send_to_char( "No such location.\n\r", ch );
      return;
   }

   if( room_is_private( location ) )
   {
      if( ch->level < sysdata.level_override_private )
      {
         send_to_char( "That room is private right now.\n\r", ch );
         return;
      }
      else
         send_to_char( "Overriding private flag!\n\r", ch );
   }

   if( IS_ROOM_FLAG( location, ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      send_to_char( "Go away! That room has been sealed for privacy!\n\r", ch );
      return;
   }

   if( ch->fighting )
      stop_fighting( ch, TRUE );

   /*
    * Modified bamfout processing by Altrag, installed by Samson 12-10-97 
    */
   if( ch->pcdata && ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
      act( AT_IMMORT, "$T", ch, NULL, bamf_print( ch->pcdata->bamfout, ch ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n vanishes suddenly into thin air.", ch, NULL, NULL, TO_CANSEE );

   leave_map( ch, wch, location );

   /*
    * Modified bamfin processing by Altrag, installed by Samson 12-10-97 
    */
   if( ch->pcdata && ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
      act( AT_IMMORT, "$T", ch, NULL, bamf_print( ch->pcdata->bamfin, ch ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n appears suddenly out of thin air.", ch, NULL, NULL, TO_CANSEE );

   return;
}

void goto_obj( CHAR_DATA * ch, OBJ_DATA * obj, char *argument )
{
   ROOM_INDEX_DATA *location;

   set_char_color( AT_IMMORT, ch );
   location = obj->in_room;

   if( !location && obj->carried_by )
      location = obj->carried_by->in_room;
   else  /* It's in a container, this becomes too much hassle to recursively locate */
   {
      ch_printf( ch, "%s is inside a container. Try locating that container first.\n\r", argument );
      return;
   }

   if( !location )
   {
      bug( "%s", "goto_obj: Object in NULL room!" );
      return;
   }

   if( room_is_private( location ) )
   {
      if( ch->level < sysdata.level_override_private )
      {
         send_to_char( "That room is private right now.\n\r", ch );
         return;
      }
      else
         send_to_char( "Overriding private flag!\n\r", ch );
   }

   if( IS_ROOM_FLAG( location, ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      send_to_char( "Go away! That room has been sealed for privacy!\n\r", ch );
      return;
   }

   if( ch->fighting )
      stop_fighting( ch, TRUE );

   /*
    * Modified bamfout processing by Altrag, installed by Samson 12-10-97 
    */
   if( ch->pcdata && ch->pcdata->bamfout[0] )
      act( AT_IMMORT, "$T", ch, NULL, bamf_print( ch->pcdata->bamfout, ch ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n vanishes suddenly into thin air.", ch, NULL, NULL, TO_CANSEE );

   leave_map( ch, NULL, location );

   /*
    * Modified bamfin processing by Altrag, installed by Samson 12-10-97 
    */
   if( ch->pcdata && ch->pcdata->bamfin[0] )
      act( AT_IMMORT, "$T", ch, NULL, bamf_print( ch->pcdata->bamfin, ch ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n appears suddenly out of thin air.", ch, NULL, NULL, TO_CANSEE );

   return;
}

/* Function modified from original form - Samson 12-10-97 */
CMDF do_goto( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   ROOM_INDEX_DATA *location, *in_room;
   CHAR_DATA *wch;
   OBJ_DATA *obj;
   AREA_DATA *pArea;
   int vnum;

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Goto where?\n\r", ch );
      return;
   }

   /*
    * Begin Overland Map additions 
    */
   if( !str_cmp( arg, "map" ) )
   {
      char arg1[MIL];
      char arg2[MIL];
      int x, y;
      int map = -1;

      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );

      if( !arg1 || arg1[0] == '\0' )
      {
         send_to_char( "Goto which map??\n\r", ch );
         return;
      }

      if( !str_cmp( arg1, "alsherok" ) )
         map = ACON_ALSHEROK;

      if( !str_cmp( arg1, "alatia" ) )
         map = ACON_ALATIA;

      if( !str_cmp( arg1, "eletar" ) )
         map = ACON_ELETAR;

      if( map == -1 )
      {
         ch_printf( ch, "There isn't a map for '%s'.\n\r", arg1 );
         return;
      }

      if( arg2[0] == '\0' && argument[0] == '\0' )
      {
         enter_map( ch, NULL, 499, 499, map );
         return;
      }

      if( arg2[0] == '\0' || argument[0] == '\0' )
      {
         send_to_char( "Usage: goto map <mapname> <X> <Y>\n\r", ch );
         return;
      }

      x = atoi( arg2 );
      y = atoi( argument );

      if( x < 0 || x >= MAX_X )
      {
         ch_printf( ch, "Valid x coordinates are 0 to %d.\n\r", MAX_X - 1 );
         return;
      }

      if( y < 0 || y >= MAX_Y )
      {
         ch_printf( ch, "Valid y coordinates are 0 to %d.\n\r", MAX_Y - 1 );
         return;
      }

      enter_map( ch, NULL, x, y, map );
      return;
   }
   /*
    * End of Overland Map additions 
    */

   if( !is_number( arg ) )
   {
      if( ( wch = get_char_world( ch, arg ) ) != NULL && wch->in_room != NULL )
      {
         goto_char( ch, wch, arg );
         return;
      }

      if( ( obj = get_obj_world( ch, arg ) ) != NULL )
      {
         goto_obj( ch, obj, arg );
         return;
      }
   }

   if( ( location = find_location( ch, arg ) ) == NULL )
   {
      vnum = atoi( arg );
      if( vnum < 0 || get_room_index( vnum ) )
      {
         send_to_char( "You cannot find that...\n\r", ch );
         return;
      }

      if( get_trust( ch ) < LEVEL_CREATOR || vnum < 1 || IS_NPC( ch ) || !ch->pcdata->area )
      {
         send_to_char( "No such location.\n\r", ch );
         return;
      }

      if( get_trust( ch ) < sysdata.level_modify_proto )
      {
         if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
         {
            send_to_char( "You must have an assigned area to create rooms.\n\r", ch );
            return;
         }
         if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
         {
            send_to_char( "That room is not within your assigned range.\n\r", ch );
            return;
         }
      }
      if( vnum < 1 || vnum > sysdata.maxvnum )
      {
         ch_printf( ch, "Invalid vnum. Allowable range is 1 to %d\n\r", sysdata.maxvnum );
         return;
      }
      location = make_room( vnum, ch->pcdata->area );
      if( !location )
      {
         bug( "%s: make_room failed", __FUNCTION__ );
         return;
      }
      send_to_char( "&WWaving your hand, you form order from swirling chaos,\n\rand step into a new reality...\n\r", ch );
   }

   if( room_is_private( location ) )
   {
      if( ch->level < sysdata.level_override_private )
      {
         send_to_char( "That room is private right now.\n\r", ch );
         return;
      }
      else
         send_to_char( "Overriding private flag!\n\r", ch );
   }

   if( IS_ROOM_FLAG( location, ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      send_to_char( "Go away! That room has been sealed for privacy!\n\r", ch );
      return;
   }

   in_room = ch->in_room;
   if( ch->fighting )
      stop_fighting( ch, TRUE );

   /*
    * Modified bamfout processing by Altrag, installed by Samson 12-10-97 
    */
   if( ch->pcdata && ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
      act( AT_IMMORT, "$T", ch, NULL, bamf_print( ch->pcdata->bamfout, ch ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n vanishes suddenly into thin air.", ch, NULL, NULL, TO_CANSEE );

   /*
    * It's assumed that if you've come this far, it's a room vnum you entered 
    */
   leave_map( ch, NULL, location );

   if( ch->pcdata && ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
      act( AT_IMMORT, "$T", ch, NULL, bamf_print( ch->pcdata->bamfin, ch ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n appears suddenly out of thin air.", ch, NULL, NULL, TO_CANSEE );

   return;
}

/* Function modified from original form - Samson */
/* Eliminated clan_type GUILD 11-30-98 */
CMDF do_mset( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL], arg3[MIL];
   int num, size, plus, v2, value, minattr, maxattr;
   char char1, char2;
   CHAR_DATA *victim, *tmpmob;
   bool lockvictim;
   char *origarg = argument;

   set_char_color( AT_PLAIN, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mob's can't mset\n\r", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_MOB_DESC:

         if( !ch->pcdata->dest_buf )
         {
            send_to_char( "Fatal error: report to Samson.\n\r", ch );
            bug( "%s", "do_mset: sub_mob_desc: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            return;
         }
         victim = ( CHAR_DATA * ) ch->pcdata->dest_buf;

         if( char_died( victim ) )
         {
            send_to_char( "Your victim died!\n\r", ch );
            stop_editing( ch );
            return;
         }
         STRFREE( victim->chardesc );
         victim->chardesc = copy_buffer( ch );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->chardesc );
            victim->pIndexData->chardesc = QUICKLINK( victim->chardesc );
         }
         tmpmob = ( CHAR_DATA * ) ch->pcdata->spare_ptr;
         stop_editing( ch );
         ch->pcdata->dest_buf = tmpmob;
         ch->substate = ch->tempnum;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting description.\n\r", ch );
         return;
   }

   victim = NULL;
   lockvictim = FALSE;
   smash_tilde( argument );

   if( ch->substate == SUB_REPEATCMD )
   {
      victim = ( CHAR_DATA * ) ch->pcdata->dest_buf;

      if( !victim )
      {
         send_to_char( "Your victim died!\n\r", ch );
         argument = "done";
      }
      if( !argument || argument[0] == '\0' || !str_cmp( argument, " " ) || !str_cmp( argument, "stat" ) )
      {
         if( victim )
         {
            if( !IS_NPC( victim ) )
               do_mstat( ch, victim->name );
            else
               funcf( ch, do_mstat, "%d", victim->pIndexData->vnum );
         }
         else
            send_to_char( "No victim selected.  Type '?' for help.\n\r", ch );
         return;
      }

      if( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) )
      {
         if( ch->pcdata->dest_buf )
            RelDestroy( relMSET_ON, ch, ch->pcdata->dest_buf );
         send_to_char( "Mset mode off.\n\r", ch );
         ch->substate = SUB_NONE;
         ch->pcdata->dest_buf = NULL;
         STRFREE( ch->pcdata->subprompt );
         return;
      }
   }
   if( victim )
   {
      lockvictim = TRUE;
      mudstrlcpy( arg1, victim->name, MIL );
      argument = one_argument( argument, arg2 );
      mudstrlcpy( arg3, argument, MIL );
   }
   else
   {
      lockvictim = FALSE;
      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );
      mudstrlcpy( arg3, argument, MIL );
   }

   if( !str_cmp( arg1, "on" ) )
   {
      send_to_char( "Syntax: mset <victim|vnum> on.\n\r", ch );
      return;
   }

   if( arg1[0] == '\0' || ( arg2[0] == '\0' && ch->substate != SUB_REPEATCMD ) || !str_cmp( arg1, "?" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
      {
         if( victim )
            send_to_char( "Syntax: <field>  <value>\n\r", ch );
         else
            send_to_char( "Syntax: <victim> <field>  <value>\n\r", ch );
      }

      /*
       * Output reformatted by Whir - 8/27/98 
       */
      send_to_char( "Syntax: mset <victim> <field>  <value>\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Field being one of:\n\r", ch );
      send_to_char( " [Naming]\n\r", ch );
      send_to_char( "   |name       |short      |long       |title      |description\n\r", ch );
      send_to_char( " [Stats]\n\r", ch );
      send_to_char( "   |height     |weight     |sex        |align      |race\n\r", ch );
      send_to_char( "   |gold       |hp         |mana       |move       |pracs\n\r", ch );
      send_to_char( "   |Class      |level      |pos        |defpos     |part\n\r", ch );
      send_to_char( "   |speaking   |speaks     |resist (r) |immune (i) |suscept (s)\n\r", ch );
      send_to_char( "   |absorb	|agemod	|		|		|\n\r", ch );
      send_to_char( " [Combat stats]\n\r", ch );
      send_to_char( "   |thac0      |numattacks |armor      |affected   |attack\n\r", ch );
      send_to_char( "   |defense    |           |           |           |\n\r", ch );
      send_to_char( " [Groups]\n\r", ch );
      send_to_char( "   |clan       |council\n\r", ch );
      send_to_char( "   |favor      |deity      |           |           |\n\r", ch );
      send_to_char( " [Misc]\n\r", ch );
      send_to_char( "   |thirst     |drunk      |hunger     |flags\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "  see BODYPARTS, RIS, LANGAUGES, and SAVINGTHROWS for help\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "For editing index/prototype mobiles:\n\r", ch );
      send_to_char( "   |hitnumdie  |hitsizedie |hitplus (hit points)\n\r", ch );
      send_to_char( "   |damnumdie  |damsizedie |damplus (damage roll)\n\r", ch );
      if( ch->level >= LEVEL_ADMIN )
      {
         send_to_char( "To toggle pkill flag: pkill\n\r", ch );
         send_to_char( "------------------------------------------------------------\n\r", ch );
         send_to_char( "   |minsnoop  |realm\n\r", ch );
      }
      return;
   }
   if( !victim && get_trust( ch ) < LEVEL_GOD )
   {
      if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
      {
         send_to_char( "Where are they again?\n\r", ch );
         return;
      }
   }
   else if( !victim )
   {
      if( ( victim = get_char_world( ch, arg1 ) ) == NULL )
      {
         ch_printf( ch, "Sorry, %s doesn't seem to exist in this universe.\n\r", arg1 );
         return;
      }
   }

   if( get_trust( ch ) < get_trust( victim ) && !IS_NPC( victim ) )
   {
      send_to_char( "Not this time chummer!\n\r", ch );
      ch->pcdata->dest_buf = NULL;
      return;
   }

   if( !can_mmodify( ch, victim ) )
      return;

   if( lockvictim )
      ch->pcdata->dest_buf = victim;

   if( IS_NPC( victim ) )
   {
      minattr = 1;
      maxattr = 25;
   }
   else
   {
      minattr = 3;
      maxattr = 18;
   }

   if( !str_cmp( arg2, "on" ) )
   {
      CHECK_SUBRESTRICTED( ch );
      ch_printf( ch, "Mset mode on. (Editing %s).\n\r", victim->name );
      ch->substate = SUB_REPEATCMD;
      ch->pcdata->dest_buf = victim;
      if( IS_NPC( victim ) )
         stralloc_printf( &ch->pcdata->subprompt, "<&CMset &W#%d&w> %%i", victim->pIndexData->vnum );
      else
         stralloc_printf( &ch->pcdata->subprompt, "<&CMset &W%s&w> %%i", victim->name );
      RelCreate( relMSET_ON, ch, victim );
      return;
   }
   value = is_number( arg3 ) ? atoi( arg3 ) : -1;

   if( atoi( arg3 ) < -1 && value == -1 )
      value = atoi( arg3 );

   if( !str_cmp( arg2, "str" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch_printf( ch, "Strength range is %d to %d.\n\r", minattr, maxattr );
         return;
      }
      victim->perm_str = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->perm_str = value;
      send_to_char( "Victim str set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "int" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch_printf( ch, "Intelligence range is %d to %d.\n\r", minattr, maxattr );
         return;
      }
      victim->perm_int = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->perm_int = value;
      send_to_char( "Victim int set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "wis" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch_printf( ch, "Wisdom range is %d to %d.\n\r", minattr, maxattr );
         return;
      }
      victim->perm_wis = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->perm_wis = value;
      send_to_char( "Victim wis set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "dex" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch_printf( ch, "Dexterity range is %d to %d.\n\r", minattr, maxattr );
         return;
      }
      victim->perm_dex = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->perm_dex = value;
      send_to_char( "Victim dex set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "con" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch_printf( ch, "Constitution range is %d to %d.\n\r", minattr, maxattr );
         return;
      }
      victim->perm_con = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->perm_con = value;
      send_to_char( "Victim con set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "cha" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch_printf( ch, "Charisma range is %d to %d.\n\r", minattr, maxattr );
         return;
      }
      victim->perm_cha = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->perm_cha = value;
      send_to_char( "Victim cha set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "lck" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch_printf( ch, "Luck range is %d to %d.\n\r", minattr, maxattr );
         return;
      }
      victim->perm_lck = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->perm_lck = value;
      send_to_char( "Victim lck set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "height" ) )
   {
      if( value < 0 || value > 500 )
      {
         send_to_char( "Height range is 0 to 500.\n\r", ch );
         return;
      }
      victim->height = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->height = value;
      send_to_char( "Victim height set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "weight" ) )
   {
      if( value < 0 || value > 30000 )
      {
         send_to_char( "Weight range is 0 to 30000.\n\r", ch );
         return;
      }
      victim->weight = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->weight = value;
      send_to_char( "Victim weight set.\n\r", ch );
      return;
   }

   /*
    * Altered to allow sex changes by name instead of number - Samson 8-3-98 
    */
   if( !str_cmp( arg2, "sex" ) )
   {
      value = get_npc_sex( arg3 );

      if( value < 0 || value >= SEX_MAX )
      {
         send_to_char( "Invalid sex.\n\r", ch );
         return;
      }
      victim->sex = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->sex = value;
      send_to_char( "Victim sex set.\n\r", ch );
      return;
   }

   /*
    * Altered to allow Class changes by name instead of number - Samson 8-2-98 
    */
   if( !str_cmp( arg2, "Class" ) )
   {
      value = get_npc_class( arg3 );

      if( !IS_NPC( victim ) && ( value < 0 || value >= MAX_CLASS ) )
      {
         ch_printf( ch, "%s is not a valid player Class.\n\r", arg3 );
         return;
      }
      if( IS_NPC( victim ) && ( value < 0 || value >= MAX_NPC_CLASS ) )
      {
         ch_printf( ch, "%s is not a valid NPC Class.\n\r", arg3 );
         return;
      }
      victim->Class = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->Class = value;
      send_to_char( "Victim Class set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "race" ) )
   {
      if( IS_NPC( victim ) )
         value = get_npc_race( arg3 );
      else
         value = get_pc_race( arg3 );

      if( !IS_NPC( victim ) && ( value < 0 || value >= MAX_PC_RACE ) )
      {
         ch_printf( ch, "%s is not a valid player race.\n\r", arg3 );
         return;
      }
      if( IS_NPC( victim ) && ( value < 0 || value >= MAX_NPC_RACE ) )
      {
         ch_printf( ch, "%s is not a valid NPC race.\n\r", arg3 );
         return;
      }
      victim->race = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->race = value;
      send_to_char( "Victim race set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "armor" ) || !str_cmp( arg2, "ac" ) )
   {
      if( value < -300 || value > 300 )
      {
         send_to_char( "AC range is -300 to 300.\n\r", ch );
         return;
      }
      victim->armor = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->ac = value;
      send_to_char( "Victim ac set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "gold" ) )
   {
      if( value < 0 )
         value = -1;
      victim->gold = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->gold = value;
      send_to_char( "Victim gold set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "exp" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "Not on PC's.\n\r", ch );
         return;
      }

      if( !str_cmp( arg3, "x" ) || value < 0 )
         value = -1;

      if( value == -1 )
         victim->exp = mob_xp( victim );
      else
         victim->exp = value;

      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->exp = value;
      send_to_char( "Victim experience value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "hp" ) )
   {
      if( value < 1 || value > 32700 )
      {
         send_to_char( "Hp range is 1 to 32,700 hit points.\n\r", ch );
         return;
      }
      victim->max_hit = value;
      send_to_char( "Victim hp set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "mana" ) )
   {
      if( value < 0 || value > 32700 )
      {
         send_to_char( "Mana range is 0 to 32,700 mana points.\n\r", ch );
         return;
      }
      victim->max_mana = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->max_mana = value;
      send_to_char( "Victim mana set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "move" ) )
   {
      if( value < 0 || value > 32700 )
      {
         send_to_char( "Move range is 0 to 32,700 move points.\n\r", ch );
         return;
      }
      victim->max_move = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->max_move = value;
      send_to_char( "Victim moves set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "practice" ) )
   {
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if( value < 0 || value > 500 )
      {
         send_to_char( "Practice range is 0 to 500 sessions.\n\r", ch );
         return;
      }
      victim->pcdata->practice = value;
      send_to_char( "Victim practices set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "align" ) )
   {
      if( value < -1000 || value > 1000 )
      {
         send_to_char( "Alignment range is -1000 to 1000.\n\r", ch );
         return;
      }
      victim->alignment = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->alignment = value;
      send_to_char( "Victim alignment set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "rank" ) )
   {
      if( ch->level < LEVEL_ASCENDANT )
      {
         send_to_char( "You can't change your rank.\n\r", ch );
         return;
      }
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }
      if( victim->level < LEVEL_ASCENDANT && !IS_IMP( ch ) )
      {
         send_to_char( "Rank is intended for high level immortals ONLY.\n\r", ch );
         return;
      }
      smash_tilde( argument );
      STRFREE( victim->pcdata->rank );
      if( argument && argument[0] != '\0' && str_cmp( argument, "none" ) )
         victim->pcdata->rank = STRALLOC( argument );
      send_to_char( "Ok.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "favor" ) )
   {
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if( value < -2500 || value > 2500 )
      {
         send_to_char( "Range is from -2500 to 2500.\n\r", ch );
         return;
      }

      victim->pcdata->favor = value;
      return;
   }

   if( !str_cmp( arg2, "mentalstate" ) )
   {
      if( value < -100 || value > 100 )
      {
         send_to_char( "Value must be in range -100 to +100.\n\r", ch );
         return;
      }
      victim->mental_state = value;
      send_to_char( "Victim mental state set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "thirst" ) )
   {
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if( value < -1 || value > 100 )
      {
         send_to_char( "Thirst range is -1 to 100.\n\r", ch );
         return;
      }
      victim->pcdata->condition[COND_THIRST] = value;
      send_to_char( "Victim thirst set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "drunk" ) )
   {
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if( value < 0 || value > 100 )
      {
         send_to_char( "Drunk range is 0 to 100.\n\r", ch );
         return;
      }

      victim->pcdata->condition[COND_DRUNK] = value;
      send_to_char( "Victim drunk set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "agemod" ) )
   {
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      victim->pcdata->age_bonus = value;
      send_to_char( "Victim agemod set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "hunger" ) )
   {
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if( value < -1 || value > 100 )
      {
         send_to_char( "Hunger range is -1 to 100.\n\r", ch );
         return;
      }
      victim->pcdata->condition[COND_FULL] = value;
      send_to_char( "Victim hunger set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "realm" ) )
   {
      if( !IS_IMP( ch ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }
      if( !IS_IMMORTAL( victim ) )
      {
         send_to_char( "This can only be done for immortals.\n\r", ch );
         return;
      }
      if( value < 0 || value > 5 )
      {
         send_to_char( "Valid range for realm is 0 - 5. See 'help realms'.\n\r", ch );
         return;
      }
      if( victim->pcdata )
      {
         victim->pcdata->realm = value;
         save_char_obj( victim );
         build_wizinfo(  );
         return;
      }
   }

   if( !str_cmp( arg2, "minsnoop" ) )
   {
      if( !IS_IMP( ch ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }
      victim->pcdata->min_snoop = value;
      return;
   }

   if( !str_cmp( arg2, "clan" ) )
   {
      CLAN_DATA *clan;

      if( !IS_IMP( ch ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }

      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if( !arg3 || arg3[0] == '\0' )
      {
         /*
          * Crash bug fix, oops guess I should have caught this one :)
          * * But it was early in the morning :P --Shaddai 
          */
         if( victim->pcdata->clan == NULL )
            return;
         /*
          * Added a check on immortals so immortals don't take up
          * * any membership space. --Shaddai
          */
         if( !IS_IMMORTAL( victim ) )
         {
            --victim->pcdata->clan->members;
            if( victim->pcdata->clan->members < 0 )
               victim->pcdata->clan->members = 0;
            save_clan( victim->pcdata->clan );
         }
         STRFREE( victim->pcdata->clan_name );
         victim->pcdata->clan = NULL;
         return;
      }
      clan = get_clan( arg3 );
      if( !clan )
      {
         send_to_char( "No such clan.\n\r", ch );
         return;
      }
      if( victim->pcdata->clan != NULL && !IS_IMMORTAL( victim ) )
      {
         --victim->pcdata->clan->members;
         if( victim->pcdata->clan->members < 0 )
            victim->pcdata->clan->members = 0;
         save_clan( victim->pcdata->clan );
      }
      STRFREE( victim->pcdata->clan_name );
      victim->pcdata->clan_name = QUICKLINK( clan->name );
      victim->pcdata->clan = clan;
      if( !IS_IMMORTAL( victim ) )
      {
         ++victim->pcdata->clan->members;
         save_clan( victim->pcdata->clan );
      }
      return;
   }

   if( !str_cmp( arg2, "deity" ) )
   {
      DEITY_DATA *deity;

      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if( !arg3 || arg3[0] == '\0' )
      {
         if( victim->pcdata->deity )
         {
            --victim->pcdata->deity->worshippers;
            if( victim->pcdata->deity->worshippers < 0 )
               victim->pcdata->deity->worshippers = 0;
            save_deity( victim->pcdata->deity );
         }
         STRFREE( victim->pcdata->deity_name );
         victim->pcdata->deity = NULL;
         send_to_char( "Deity removed.\n\r", ch );
         return;
      }

      deity = get_deity( arg3 );
      if( !deity )
      {
         send_to_char( "No such deity.\n\r", ch );
         return;
      }
      if( victim->pcdata->deity )
      {
         --victim->pcdata->deity->worshippers;
         if( victim->pcdata->deity->worshippers < 0 )
            victim->pcdata->deity->worshippers = 0;
         save_deity( victim->pcdata->deity );
      }
      STRFREE( victim->pcdata->deity_name );
      victim->pcdata->deity_name = QUICKLINK( deity->name );
      victim->pcdata->deity = deity;
      ++deity->worshippers;
      save_deity( deity );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "council" ) )
   {
      COUNCIL_DATA *council;

      if( !IS_IMP( ch ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if( !arg3 || arg3[0] == '\0' )
      {
         STRFREE( victim->pcdata->council_name );
         victim->pcdata->council = NULL;
         check_switch( victim, FALSE );
         send_to_char( "Removed from council.\n\rPlease make sure you adjust that council's members accordingly.\n\r", ch );
         return;
      }

      council = get_council( arg3 );
      if( !council )
      {
         send_to_char( "No such council.\n\r", ch );
         return;
      }
      STRFREE( victim->pcdata->council_name );
      victim->pcdata->council_name = QUICKLINK( council->name );
      victim->pcdata->council = council;
      send_to_char( "Done.\n\rPlease make sure you adjust that council's members accordingly.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      STRFREE( victim->short_descr );
      victim->short_descr = STRALLOC( arg3 );
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
      {
         STRFREE( victim->pIndexData->short_descr );
         victim->pIndexData->short_descr = QUICKLINK( victim->short_descr );
      }
      send_to_char( "Victim short description set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      stralloc_printf( &victim->long_descr, "%s\n\r", arg3 );
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
      {
         STRFREE( victim->pIndexData->long_descr );
         victim->pIndexData->long_descr = QUICKLINK( victim->long_descr );
      }
      send_to_char( "Victim long description set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "description" ) || !str_cmp( arg2, "desc" ) )
   {
      if( arg3 && arg3[0] != '\0' )
      {
         STRFREE( victim->chardesc );
         victim->chardesc = STRALLOC( arg3 );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->chardesc );
            victim->pIndexData->chardesc = QUICKLINK( victim->chardesc );
         }
         return;
      }
      CHECK_SUBRESTRICTED( ch );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;

      if( lockvictim )
         ch->pcdata->spare_ptr = victim;
      else
         ch->pcdata->spare_ptr = NULL;

      ch->substate = SUB_MOB_DESC;
      ch->pcdata->dest_buf = victim;
      if( !victim->chardesc || victim->chardesc[0] == '\0' )
         victim->chardesc = STRALLOC( "" );
      start_editing( ch, victim->chardesc );
      if( IS_NPC( victim ) )
         editor_desc_printf( ch, "Description of mob, vnum %ld (%s).", victim->pIndexData->vnum, victim->name );
      else
         editor_desc_printf( ch, "Description of player %s.", capitalize( victim->name ) );
      return;
   }

   if( !str_cmp( arg2, "title" ) )
   {
      if( IS_NPC( victim ) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      set_title( victim, arg3 );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      bool pcflag, protoflag = FALSE;

      if( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_GREATER )
      {
         send_to_char( "You can only modify a mobile's flags.\n\r", ch );
         return;
      }

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> flags <flag> [flag]...\n\r", ch );
         return;
      }

      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         protoflag = TRUE;

      while( argument[0] != '\0' )
      {
         pcflag = FALSE;
         argument = one_argument( argument, arg3 );
         value = IS_NPC( victim ) ? get_actflag( arg3 ) : get_plrflag( arg3 );

         if( !IS_NPC( victim ) && ( value < 0 || value > MAX_BITS ) )
         {
            pcflag = TRUE;
            value = get_pcflag( arg3 );
         }
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
         {
            if( IS_NPC( victim ) && value == ACT_PROTOTYPE && ch->level < sysdata.level_modify_proto )
               send_to_char( "You cannot change the prototype flag.\n\r", ch );
            else if( IS_NPC( victim ) && value == ACT_IS_NPC )
               send_to_char( "If that could be changed, it would cause many problems.\n\r", ch );
            else
            {
               if( pcflag )
                  TOGGLE_BIT( victim->pcdata->flags, 1 << value );
               else
                  xTOGGLE_BIT( victim->act, value );
            }
         }
      }
      send_to_char( "Victim flags set.\n\r", ch );
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) || ( value == ACT_PROTOTYPE && protoflag ) )
         victim->pIndexData->act = victim->act;
      return;
   }

   if( !str_cmp( arg2, "affected" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's flags.\n\r", ch );
         return;
      }

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> affected <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_aflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( victim->affected_by, value );
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->affected_by = victim->affected_by;
      send_to_char( "Victim affects set.\n\r", ch );
      return;
   }

   /*
    * save some more finger-leather for setting RIS stuff
    */
   if( !str_cmp( arg2, "r" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's ris.\n\r", ch );
         return;
      }
      funcf( ch, do_mset, "%s resistant %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "i" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's ris.\n\r", ch );
         return;
      }
      funcf( ch, do_mset, "%s immune %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "s" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's ris.\n\r", ch );
         return;
      }
      funcf( ch, do_mset, "%s susceptible %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "ri" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's ris.\n\r", ch );
         return;
      }
      funcf( ch, do_mset, "%s resistant %s", arg1, arg3 );
      funcf( ch, do_mset, "%s immune %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "rs" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's ris.\n\r", ch );
         return;
      }
      funcf( ch, do_mset, "%s resistant %s", arg1, arg3 );
      funcf( ch, do_mset, "%s susceptible %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "is" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's ris.\n\r", ch );
         return;
      }
      funcf( ch, do_mset, "%s immune %s", arg1, arg3 );
      funcf( ch, do_mset, "%s susceptible %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "ris" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's ris.\n\r", ch );
         return;
      }
      funcf( ch, do_mset, "%s resistant %s", arg1, arg3 );
      funcf( ch, do_mset, "%s immune %s", arg1, arg3 );
      funcf( ch, do_mset, "%s susceptible %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "resistant" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's resistancies.\n\r", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> resistant <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( victim->resistant, value );
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->resistant = victim->resistant;
      send_to_char( "Victim resistances set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "immune" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's immunities.\n\r", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> immune <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( victim->immune, value );
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->immune = victim->immune;
      send_to_char( "Victim immunities set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "susceptible" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's susceptibilities.\n\r", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> susceptible <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( victim->susceptible, value );
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->susceptible = victim->susceptible;
      send_to_char( "Victim susceptibilities set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "absorb" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's absorbs.\n\r", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> absorb <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( victim->absorb, value );
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->absorb = victim->absorb;
      send_to_char( "Victim absorb set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "part" ) )
   {
      if( !IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "You can only modify a mobile's parts.\n\r", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> part <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_partflag( arg3 );
         if( value < 0 || value > 31 )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            TOGGLE_BIT( victim->xflags, 1 << value );
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->xflags = victim->xflags;
      send_to_char( "Victim body parts set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "pkill" ) )
   {
      if( IS_NPC( victim ) )
      {
         send_to_char( "Player Characters only.\n\r", ch );
         return;
      }

      if( IS_PCFLAG( victim, PCFLAG_DEADLY ) )
      {
         REMOVE_PCFLAG( victim, PCFLAG_DEADLY );
         send_to_char( "You are now a NON-PKILL player.\n\r", victim );
         if( ch != victim )
            send_to_char( "That player is now non-pkill.\n\r", ch );
      }
      else
      {
         SET_PCFLAG( victim, PCFLAG_DEADLY );
         send_to_char( "You are now a PKILL player.\n\r", victim );
         if( ch != victim )
            send_to_char( "That player is now pkill.\n\r", ch );
      }
      if( victim->pcdata->clan && !IS_IMMORTAL( victim ) )
      {
         if( victim->speaking & LANG_CLAN )
            victim->speaking = LANG_COMMON;
         REMOVE_BIT( victim->speaks, LANG_CLAN );
         --victim->pcdata->clan->members;
         if( !str_cmp( victim->name, victim->pcdata->clan->leader ) )
            STRFREE( victim->pcdata->clan->leader );
         if( !str_cmp( victim->name, victim->pcdata->clan->number1 ) )
            STRFREE( victim->pcdata->clan->number1 );
         if( !str_cmp( victim->name, victim->pcdata->clan->number2 ) )
            STRFREE( victim->pcdata->clan->number2 );
         save_clan( victim->pcdata->clan );
         STRFREE( victim->pcdata->clan_name );
         victim->pcdata->clan = NULL;
      }
      save_char_obj( victim );
      return;
   }

   if( !str_cmp( arg2, "speaks" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> speaks <language> [language] ...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_langflag( arg3 );
         if( value == LANG_UNKNOWN )
            ch_printf( ch, "Unknown language: %s\n\r", arg3 );
         else if( !IS_NPC( victim ) )
         {
            if( !( value &= VALID_LANGS ) )
            {
               ch_printf( ch, "Players may not know %s.\n\r", arg3 );
               continue;
            }
         }
         v2 = get_langnum( arg3 );
         if( v2 == -1 )
            ch_printf( ch, "Unknown language: %s\n\r", arg3 );
         else
            TOGGLE_BIT( victim->speaks, 1 << v2 );
      }
      if( !IS_NPC( victim ) )
      {
         REMOVE_BIT( victim->speaks, race_table[victim->race]->language );
         if( !knows_language( victim, victim->speaking, victim ) )
            victim->speaking = race_table[victim->race]->language;
      }
      else if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->speaks = victim->speaks;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "speaking" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> speaking <language> [language]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_langflag( arg3 );
         if( value == LANG_UNKNOWN )
            ch_printf( ch, "Unknown language: %s\n\r", arg3 );
         else
         {
            v2 = get_langnum( arg3 );
            if( v2 == -1 )
               ch_printf( ch, "Unknown language: %s\n\r", arg3 );
            else
               TOGGLE_BIT( victim->speaking, 1 << v2 );
         }
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->speaking = victim->speaking;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !IS_NPC( victim ) )
   {
      ch_printf( ch, "Cannot change %s on PC's.\n\r", arg2 );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      if( value < 0 || value > LEVEL_AVATAR + 10 )
      {
         ch_printf( ch, "Level range is 0 to %d.\n\r", LEVEL_AVATAR + 10 );
         return;
      }
      victim->level = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->level = value;
      send_to_char( "Victim level set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "numattacks" ) )
   {
      if( value < 0 || value > 10 )
      {
         send_to_char( "Attacks range is 0 to 10.\n\r", ch );
         return;
      }
      victim->numattacks = ( float )( value );
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->numattacks = ( float )( value );
      send_to_char( "Victim numattacks set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "thac0" ) || !str_cmp( arg2, "thaco" ) )
   {
      if( !str_cmp( arg3, "x" ) || value > 20 )
         value = 21;

      victim->mobthac0 = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->mobthac0 = value;
      send_to_char( "Victim thac0 set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( !arg3 || arg3[0] == '\0' )
      {
         send_to_char( "Cannot set empty keywords!\n\r", ch );
         return;
      }

      STRFREE( victim->name );
      victim->name = STRALLOC( arg3 );
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
      {
         STRFREE( victim->pIndexData->player_name );
         victim->pIndexData->player_name = QUICKLINK( victim->name );
      }
      send_to_char( "Victim keywords set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "spec" ) || !str_cmp( arg2, "spec_fun" ) )
   {
      SPEC_FUN *specfun;

      if( !str_cmp( arg3, "none" ) )
      {
         victim->spec_fun = NULL;
         STRFREE( victim->spec_funname );
         send_to_char( "Special function removed.\n\r", ch );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         {
            victim->pIndexData->spec_fun = NULL;
            STRFREE( victim->pIndexData->spec_funname );
         }
         return;
      }

      if( !( specfun = m_spec_lookup( arg3 ) ) )
      {
         send_to_char( "No such function.\n\r", ch );
         return;
      }

      if( !validate_spec_fun( arg3 ) )
      {
         ch_printf( ch, "%s is not a valid spec_fun for mobiles.\n\r", arg3 );
         return;
      }

      victim->spec_fun = specfun;
      STRFREE( victim->spec_funname );
      victim->spec_funname = STRALLOC( arg3 );
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
      {
         victim->pIndexData->spec_fun = victim->spec_fun;
         STRFREE( victim->pIndexData->spec_funname );
         victim->pIndexData->spec_funname = STRALLOC( arg3 );
      }
      send_to_char( "Victim special function set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "attack" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> attack <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_attackflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( victim->attacks, value );
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->attacks = victim->attacks;
      send_to_char( "Victim attacks set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "defense" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: mset <victim> defense <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_defenseflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( victim->defenses, value );
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->defenses = victim->defenses;
      send_to_char( "Victim defenses set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "pos" ) )
   {
      value = get_npc_position( arg3 );

      if( value < 0 || value > POS_STANDING )
      {
         send_to_char( "Invalid position.\n\r", ch );
         return;
      }
      victim->position = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->position = victim->position;
      send_to_char( "Victim position set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "defpos" ) )
   {
      value = get_npc_position( arg3 );

      if( value < 0 || value > POS_STANDING )
      {
         send_to_char( "Invalid position.\n\r", ch );
         return;
      }
      victim->defposition = value;
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->defposition = victim->defposition;
      send_to_char( "Victim default position set.\n\r", ch );
      return;
   }

   /*
    * save some finger-leather
    */
   if( !str_cmp( arg2, "hitdie" ) )
   {
      sscanf( arg3, "%d %c %d %c %d", &num, &char1, &size, &char2, &plus );
      funcf( ch, do_mset, "%s hitnumdie %d", arg1, num );
      funcf( ch, do_mset, "%s hitsizedie %d", arg1, size );
      funcf( ch, do_mset, "%s hitplus %d", arg1, plus );
      return;
   }

   /*
    * save some more finger-leather
    */
   if( !str_cmp( arg2, "damdie" ) )
   {
      sscanf( arg3, "%d %c %d %c %d", &num, &char1, &size, &char2, &plus );
      funcf( ch, do_mset, "%s damnumdie %d", arg1, num );
      funcf( ch, do_mset, "%s damsizedie %d", arg1, size );
      funcf( ch, do_mset, "%s damplus %d", arg1, plus );
      return;
   }

   if( !str_cmp( arg2, "hitnumdie" ) )
   {
      if( value < 0 || value > 32700 )
      {
         send_to_char( "Number of hitpoint dice range is 0 to 32700.\n\r", ch );
         return;
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->hitnodice = value;
      send_to_char( "Victim hp dice number set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "hitsizedie" ) )
   {
      if( value < 0 || value > 32700 )
      {
         send_to_char( "Hitpoint dice size range is 0 to 32700.\n\r", ch );
         return;
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->hitsizedice = value;
      send_to_char( "Victim hp dice size set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "hitplus" ) )
   {
      if( value < 0 || value > 32700 )
      {
         send_to_char( "Hitpoint bonus range is 0 to 32700.\n\r", ch );
         return;
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->hitplus = value;
      send_to_char( "Victim hp bonus set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "damnumdie" ) )
   {
      if( value < 0 || value > 100 )
      {
         send_to_char( "Number of damage dice range is 0 to 100.\n\r", ch );
         return;
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->damnodice = value;
      send_to_char( "Victim damage dice number set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "damsizedie" ) )
   {
      if( value < 0 || value > 100 )
      {
         send_to_char( "Damage dice size range is 0 to 100.\n\r", ch );
         return;
      }
      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->damsizedice = value;
      send_to_char( "Victim damage dice size set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "damplus" ) )
   {
      if( value < 0 || value > 1000 )
      {
         send_to_char( "Damage bonus range is 0 to 1000.\n\r", ch );
         return;
      }

      if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         victim->pIndexData->damplus = value;
      send_to_char( "Victim damage bonus set.\n\r", ch );
      return;
   }

   /*
    * Generate usage message.
    */
   if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_mset;
   }
   else
      do_mset( ch, "" );
   return;
}

EXTRA_DESCR_DATA *SetRExtra( ROOM_INDEX_DATA * room, char *keywords )
{
   EXTRA_DESCR_DATA *ed;

   for( ed = room->first_extradesc; ed; ed = ed->next )
   {
      if( is_name( keywords, ed->keyword ) )
         break;
   }
   if( !ed )
   {
      CREATE( ed, EXTRA_DESCR_DATA, 1 );
      LINK( ed, room->first_extradesc, room->last_extradesc, next, prev );
      ed->keyword = STRALLOC( keywords );
      top_ed++;
   }
   return ed;
}

bool DelRExtra( ROOM_INDEX_DATA * room, char *keywords )
{
   EXTRA_DESCR_DATA *rmed;

   for( rmed = room->first_extradesc; rmed; rmed = rmed->next )
   {
      if( is_name( keywords, rmed->keyword ) )
         break;
   }
   if( !rmed )
      return FALSE;
   UNLINK( rmed, room->first_extradesc, room->last_extradesc, next, prev );
   STRFREE( rmed->keyword );
   STRFREE( rmed->extradesc );
   DISPOSE( rmed );
   top_ed--;
   return TRUE;
}

EXTRA_DESCR_DATA *SetOExtra( OBJ_DATA * obj, char *keywords )
{
   EXTRA_DESCR_DATA *ed;

   for( ed = obj->first_extradesc; ed; ed = ed->next )
   {
      if( is_name( keywords, ed->keyword ) )
         break;
   }
   if( !ed )
   {
      CREATE( ed, EXTRA_DESCR_DATA, 1 );
      LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
      ed->keyword = STRALLOC( keywords );
      top_ed++;
   }
   return ed;
}

bool DelOExtra( OBJ_DATA * obj, char *keywords )
{
   EXTRA_DESCR_DATA *rmed;

   for( rmed = obj->first_extradesc; rmed; rmed = rmed->next )
   {
      if( is_name( keywords, rmed->keyword ) )
         break;
   }
   if( !rmed )
      return FALSE;
   UNLINK( rmed, obj->first_extradesc, obj->last_extradesc, next, prev );
   STRFREE( rmed->keyword );
   STRFREE( rmed->extradesc );
   DISPOSE( rmed );
   top_ed--;
   return TRUE;
}

EXTRA_DESCR_DATA *SetOExtraProto( OBJ_INDEX_DATA * obj, char *keywords )
{
   EXTRA_DESCR_DATA *ed;

   for( ed = obj->first_extradesc; ed; ed = ed->next )
   {
      if( is_name( keywords, ed->keyword ) )
         break;
   }
   if( !ed )
   {
      CREATE( ed, EXTRA_DESCR_DATA, 1 );
      LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
      ed->keyword = STRALLOC( keywords );
      top_ed++;
   }
   return ed;
}

bool DelOExtraProto( OBJ_INDEX_DATA * obj, char *keywords )
{
   EXTRA_DESCR_DATA *rmed;

   for( rmed = obj->first_extradesc; rmed; rmed = rmed->next )
   {
      if( is_name( keywords, rmed->keyword ) )
         break;
   }
   if( !rmed )
      return FALSE;
   UNLINK( rmed, obj->first_extradesc, obj->last_extradesc, next, prev );
   STRFREE( rmed->keyword );
   STRFREE( rmed->extradesc );
   DISPOSE( rmed );
   top_ed--;
   return TRUE;
}

CMDF do_oset( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL], arg3[MIL];
   OBJ_DATA *obj, *tmpobj;
   EXTRA_DESCR_DATA *ed;
   bool lockobj;
   char *origarg = argument;

   int value, tmp;

   set_char_color( AT_PLAIN, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mob's can't oset\n\r", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_OBJ_EXTRA:
         if( !ch->pcdata->dest_buf )
         {
            send_to_char( "Fatal error: report to Samson.\n\r", ch );
            bug( "%s", "do_oset: sub_obj_extra: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            return;
         }
         /*
          * hopefully the object didn't get extracted...
          * if you're REALLY paranoid, you could always go through
          * the object and index-object lists, searching through the
          * extra_descr lists for a matching pointer...
          */
         ed = ( EXTRA_DESCR_DATA * ) ch->pcdata->dest_buf;
         STRFREE( ed->extradesc );
         ed->extradesc = copy_buffer( ch );
         tmpobj = ( OBJ_DATA * ) ch->pcdata->spare_ptr;
         stop_editing( ch );
         ch->pcdata->dest_buf = tmpobj;
         ch->substate = ch->tempnum;
         return;

      case SUB_OBJ_LONG:
         if( !ch->pcdata->dest_buf )
         {
            send_to_char( "Fatal error: report to Samson.\n\r", ch );
            bug( "%s", "do_oset: sub_obj_long: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            return;
         }
         obj = ( OBJ_DATA * ) ch->pcdata->dest_buf;
         if( obj && obj_extracted( obj ) )
         {
            send_to_char( "Your object was extracted!\n\r", ch );
            stop_editing( ch );
            return;
         }
         STRFREE( obj->objdesc );
         obj->objdesc = copy_buffer( ch );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         {
            if( can_omodify( ch, obj ) )
            {
               STRFREE( obj->pIndexData->objdesc );
               obj->pIndexData->objdesc = QUICKLINK( obj->objdesc );
            }
         }
         tmpobj = ( OBJ_DATA * ) ch->pcdata->spare_ptr;
         stop_editing( ch );
         ch->substate = ch->tempnum;
         ch->pcdata->dest_buf = tmpobj;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting description.\n\r", ch );
         return;
   }

   obj = NULL;
   smash_tilde( argument );

   if( ch->substate == SUB_REPEATCMD )
   {
      obj = ( OBJ_DATA * ) ch->pcdata->dest_buf;
      if( !obj )
      {
         send_to_char( "Your object was extracted!\n\r", ch );
         argument = "done";
      }
      if( argument[0] == '\0' || !str_cmp( argument, " " ) || !str_cmp( argument, "stat" ) )
      {
         if( obj )
            funcf( ch, do_ostat, "%d", obj->pIndexData->vnum );
         else
            send_to_char( "No object selected.  Type '?' for help.\n\r", ch );
         return;
      }
      if( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) )
      {
         if( ch->pcdata->dest_buf )
            RelDestroy( relOSET_ON, ch, ch->pcdata->dest_buf );
         send_to_char( "Oset mode off.\n\r", ch );
         ch->substate = SUB_NONE;
         ch->pcdata->dest_buf = NULL;
         STRFREE( ch->pcdata->subprompt );
         return;
      }
   }
   if( obj )
   {
      lockobj = TRUE;
      mudstrlcpy( arg1, obj->name, MIL );
      argument = one_argument( argument, arg2 );
      mudstrlcpy( arg3, argument, MIL );
   }
   else
   {
      lockobj = FALSE;
      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );
      mudstrlcpy( arg3, argument, MIL );
   }

   if( !str_cmp( arg1, "on" ) )
   {
      send_to_char( "Syntax: oset <object|vnum> on.\n\r", ch );
      return;
   }

   if( arg1[0] == '\0' || arg2[0] == '\0' || !str_cmp( arg1, "?" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
      {
         if( obj )
            send_to_char( "Syntax: <field>  <value>\n\r", ch );
         else
            send_to_char( "Syntax: <object> <field>  <value>\n\r", ch );
      }
      else
         send_to_char( "Syntax: oset <object> <field>  <value>\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Field being one of:\n\r", ch );
      send_to_char( "  flags wear level weight cost rent limit timer\n\r", ch );
      send_to_char( "  name short long ed rmed actiondesc\n\r", ch );
      send_to_char( "  type value0 value1 value2 value3 value4 value5 value6 value7\n\r", ch );
      send_to_char( "  affect rmaffect layers\n\r", ch );
      send_to_char( "For weapons:             For armor:\n\r", ch );
      send_to_char( "  weapontype condition     ac condition\n\r", ch );
      send_to_char( "For scrolls, potions and pills:\n\r", ch );
      send_to_char( "  slevel spell1 spell2 spell3\n\r", ch );
      send_to_char( "For wands and staves:\n\r", ch );
      send_to_char( "  slevel spell maxcharges charges\n\r", ch );
      send_to_char( "For containers:          For levers and switches:\n\r", ch );
      send_to_char( "  cflags key capacity      tflags\n\r", ch );
      return;
   }

   if( !obj && get_trust( ch ) < LEVEL_GOD )
   {
      if( ( obj = get_obj_here( ch, arg1 ) ) == NULL )
      {
         send_to_char( "You can't find that here.\n\r", ch );
         return;
      }
   }
   else if( !obj )
   {
      if( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
      {
         send_to_char( "There is nothing like that in all the lands.\n\r", ch );
         return;
      }
   }
   if( lockobj )
      ch->pcdata->dest_buf = obj;

   separate_obj( obj );
   value = atoi( arg3 );

   if( !can_omodify( ch, obj ) )
      return;

   if( !str_cmp( arg2, "on" ) )
   {
      CHECK_SUBRESTRICTED( ch );
      ch_printf( ch, "Oset mode on. (Editing '%s' vnum %d).\n\r", obj->name, obj->pIndexData->vnum );
      ch->substate = SUB_REPEATCMD;
      ch->pcdata->dest_buf = obj;
      stralloc_printf( &ch->pcdata->subprompt, "<&COset &W#%d&w> %%i", obj->pIndexData->vnum );
      RelCreate( relOSET_ON, ch, obj );
      return;
   }

   if( !str_cmp( arg2, "owner" ) )
   {
      if( !IS_IMP( ch ) )
      {
         do_oset( ch, "" );
         return;
      }

      STRFREE( obj->owner );
      obj->owner = STRALLOC( arg3 );
      send_to_char( "Object owner set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      bool proto = FALSE;

      if( !arg3 || arg3[0] == '\0' )
      {
         send_to_char( "Cannot set empty keywords!\n\r", ch );
         return;
      }

      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         proto = TRUE;
      STRFREE( obj->name );
      obj->name = STRALLOC( arg3 );
      if( proto )
      {
         STRFREE( obj->pIndexData->name );
         obj->pIndexData->name = QUICKLINK( obj->name );
      }
      send_to_char( "Object keywords set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      {
         STRFREE( obj->short_descr );
         obj->short_descr = STRALLOC( arg3 );
         STRFREE( obj->pIndexData->short_descr );
         obj->pIndexData->short_descr = QUICKLINK( obj->short_descr );
      }
      else
         /*
          * Feature added by Narn, Apr/96 
          * * If the item is not proto, add the word 'rename' to the keywords
          * * if it is not already there.
          */
      {
         STRFREE( obj->short_descr );
         obj->short_descr = STRALLOC( arg3 );
         if( str_infix( "rename", obj->name ) )
            stralloc_printf( &obj->name, "%s %s", obj->name, "rename" );
      }
      send_to_char( "Object short description set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      if( arg3 && arg3[0] != '\0' )
      {
         STRFREE( obj->objdesc );
         obj->objdesc = STRALLOC( arg3 );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->objdesc );
            obj->pIndexData->objdesc = QUICKLINK( obj->objdesc );
            return;
         }
         send_to_char( "Object long description set.\n\r", ch );
         return;
      }
      CHECK_SUBRESTRICTED( ch );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      if( lockobj )
         ch->pcdata->spare_ptr = obj;
      else
         ch->pcdata->spare_ptr = NULL;
      ch->substate = SUB_OBJ_LONG;
      ch->pcdata->dest_buf = obj;
      if( !obj->objdesc || obj->objdesc[0] == '\0' )
         obj->objdesc = STRALLOC( "" );
      start_editing( ch, obj->objdesc );
      editor_desc_printf( ch, "Object long desc, vnum %d (%s).", obj->pIndexData->vnum, obj->short_descr );
      return;
   }

   if( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
   {
      obj->value[0] = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[0] = value;
      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
   {
      obj->value[1] = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[1] = value;
      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
   {
      obj->value[2] = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->value[2] = value;
         if( obj->item_type == ITEM_WEAPON && value != 0 )
            obj->value[2] = obj->pIndexData->value[1] * obj->pIndexData->value[2];
      }
      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
   {
      if( obj->item_type == ITEM_ARMOR && ( value < 0 || value >= TATP_MAX ) )
      {
         ch_printf( ch, "Value is out of range for armor type. Range is 0 to %d\n\r", TATP_MAX );
         return;
      }

      obj->value[3] = value;

      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[3] = value;

      /*
       * Automatic armor stat generation, v3 and v4 must both be non-zero 
       */
      if( obj->item_type == ITEM_ARMOR && obj->value[3] > 0 && obj->value[4] > 0 )
         armorgen( obj );

      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
   {
      if( obj->item_type == ITEM_ARMOR && ( value < 0 || value >= TMAT_MAX ) )
      {
         ch_printf( ch, "Value is out of range for material type. Range is 0 to %d\n\r", TMAT_MAX );
         return;
      }

      obj->value[4] = value;

      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[4] = value;

      /*
       * Automatic armor stat generation, v3 and v4 must both be non-zero 
       */
      if( obj->item_type == ITEM_ARMOR && obj->value[3] > 0 && obj->value[4] > 0 )
         armorgen( obj );

      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "value5" ) || !str_cmp( arg2, "v5" ) )
   {
      if( obj->item_type == ITEM_CORPSE_PC && !IS_IMP( ch ) )
      {
         send_to_char( "Cannot alter the skeleton value.\n\r", ch );
         return;
      }
      obj->value[5] = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[5] = value;
      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "value6" ) || !str_cmp( arg2, "v6" ) )
   {
      obj->value[6] = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[6] = value;
      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "value7" ) || !str_cmp( arg2, "v7" ) )
   {
      obj->value[7] = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[7] = value;
      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "value8" ) || !str_cmp( arg2, "v8" ) )
   {
      if( obj->item_type == ITEM_WEAPON && ( value < 0 || value >= TWTP_MAX ) )
      {
         ch_printf( ch, "Value is out of range for weapon type. Range is 0 to %d\n\r", TWTP_MAX );
         return;
      }

      obj->value[8] = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[8] = value;

      if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
         weapongen( obj );

      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "value9" ) || !str_cmp( arg2, "v9" ) )
   {
      if( obj->item_type == ITEM_WEAPON && ( value < 0 || value >= TMAT_MAX ) )
      {
         ch_printf( ch, "Value is out of range for material type. Range is 0 to %d\n\r", TMAT_MAX );
         return;
      }

      obj->value[9] = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[9] = value;

      if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
         weapongen( obj );

      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "value10" ) || !str_cmp( arg2, "v10" ) )
   {
      if( obj->item_type == ITEM_WEAPON && ( value < 0 || value >= TQUAL_MAX ) )
      {
         ch_printf( ch, "Value is out of range for quality type. Range is 0 to %d\n\r", TQUAL_MAX );
         return;
      }

      obj->value[10] = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[10] = value;

      if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
         weapongen( obj );

      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> type <type>\n\r", ch );
         return;
      }
      value = get_otype( argument );
      if( value < 1 )
      {
         ch_printf( ch, "Unknown type: %s\n\r", arg3 );
         return;
      }
      obj->item_type = ( short ) value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->item_type = obj->item_type;
      ch_printf( ch, "Object type set to %s.\n\r", arg3 );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> flags <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_oflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
         {
            if( value == ITEM_PROTOTYPE && get_trust( ch ) < sysdata.level_modify_proto )
               send_to_char( "You cannot change the prototype flag.\n\r", ch );
            else
            {
               xTOGGLE_BIT( obj->extra_flags, value );
               if( value == ITEM_PROTOTYPE )
                  obj->pIndexData->extra_flags = obj->extra_flags;
            }
         }
      }
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->extra_flags = obj->extra_flags;
      send_to_char( "Object extra flags set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "wear" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> wear <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_wflag( arg3 );
         if( value < 0 || value > 31 )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            TOGGLE_BIT( obj->wear_flags, 1 << value );
      }
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->wear_flags = obj->wear_flags;
      send_to_char( "Object wear flags set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      obj->level = value;
      send_to_char( "Object level set. Note: This is not permanently changed.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "weight" ) )
   {
      obj->weight = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->weight = value;
      send_to_char( "Object weight set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "cost" ) )
   {
      obj->cost = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->cost = value;
      send_to_char( "Object cost set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "rent" ) )
   {
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->rent = value;
         if( value == -2 )
            obj->rent = set_obj_rent( obj->pIndexData );
         send_to_char( "&GObject rent set.\n\r", ch );
         if( obj->rent == -2 )
            send_to_char( "&YWARNING: This object exceeds allowable rent specs.\n\r", ch );
      }
      else
      {
         obj->rent = value;
         if( value == -2 )
            obj->rent = set_obj_rent( obj->pIndexData );
         send_to_char( "&GObject rent set.\n\r", ch );
         if( obj->rent == -2 )
            send_to_char( "&YWARNING: This object exceeds allowable rent specs.\n\r", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "limit" ) )
   {
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->limit = value;
         send_to_char( "Object limit set.\n\r", ch );
      }
      else
         send_to_char( "Item must have prototype flag to set this value.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "layers" ) )
   {
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->layers = value;
         send_to_char( "Object layers set.\n\r", ch );
      }
      else
         send_to_char( "Item must have prototype flag to set this value.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "timer" ) )
   {
      obj->timer = value;
      send_to_char( "Object timer set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "prizeowner" ) || !str_cmp( arg2, "owner" ) )
   {
      STRFREE( obj->owner );
      obj->owner = STRALLOC( arg3 );
      send_to_char( "Object prize ownership changed.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "actiondesc" ) )
   {
      if( strstr( arg3, "%n" ) || strstr( arg3, "%d" ) || strstr( arg3, "%l" ) )
      {
         send_to_char( "Illegal characters!\n\r", ch );
         return;
      }
      STRFREE( obj->action_desc );
      obj->action_desc = STRALLOC( arg3 );
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      {
         STRFREE( obj->pIndexData->action_desc );
         obj->pIndexData->action_desc = QUICKLINK( obj->action_desc );
      }
      send_to_char( "Object action description set.\n\r", ch );
      return;
   }

   /*
    * Crash fix and name support by Shaddai 
    */
   if( !str_cmp( arg2, "affect" ) )
   {
      AFFECT_DATA *paf;
      EXT_BV risabit;
      short loc;
      bool found = false;

      xCLEAR_BITS( risabit );

      argument = one_argument( argument, arg2 );
      if( !arg2 || arg2[0] == '\0' || !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> affect <field> <value>\n\r", ch );
         return;
      }
      loc = get_atype( arg2 );
      if( loc < 1 )
      {
         ch_printf( ch, "Unknown field: %s\n\r", arg2 );
         return;
      }
      if( loc == APPLY_AFFECT )
      {
         argument = one_argument( argument, arg3 );
         if( loc == APPLY_AFFECT )
         {
            value = get_aflag( arg3 );

            if( value < 0 || value >= MAX_AFFECTED_BY )
               ch_printf( ch, "Unknown affect: %s\n\r", arg3 );
            else
               found = true;
         }
      }
      else if( loc == APPLY_RESISTANT || loc == APPLY_IMMUNE || loc == APPLY_SUSCEPTIBLE || loc == APPLY_ABSORB )
      {
         char *risa = arg3;
         char flag[MIL];

         while( risa[0] != '\0' )
         {
            risa = one_argument( risa, flag );
            value = get_risflag( flag );

            if( value < 0 || value >= MAX_RIS_FLAG )
               ch_printf( ch, "Unknown flag: %s\n\r", flag );
            else
            {
               xSET_BIT( risabit, value );
               found = true;
            }
         }
      }
      else if( loc == APPLY_WEAPONSPELL
               || loc == APPLY_WEARSPELL
               || loc == APPLY_REMOVESPELL || loc == APPLY_STRIPSN || loc == APPLY_RECURRINGSPELL || loc == APPLY_EAT_SPELL )
      {
         value = skill_lookup( arg3 );

         if( !IS_VALID_SN( value ) )
            ch_printf( ch, "Invalid spell: %s", arg3 );
         else
            found = true;
      }
      else
      {
         value = atoi( arg3 );
         found = true;
      }
      if( !found )
         return;

      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      paf->location = loc;
      paf->modifier = value;
      paf->rismod = risabit;
      paf->bit = 0;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      {
         if( loc != APPLY_WEARSPELL && loc != APPLY_REMOVESPELL && loc != APPLY_STRIPSN && loc != APPLY_WEAPONSPELL )
         {
            CHAR_DATA *vch;
            OBJ_DATA *eq;

            for( vch = first_char; vch; vch = vch->next )
            {
               for( eq = vch->first_carrying; eq; eq = eq->next_content )
               {
                  if( eq->pIndexData == obj->pIndexData && eq->wear_loc != WEAR_NONE )
                     affect_modify( vch, paf, TRUE );
               }
            }
         }
         LINK( paf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next, prev );
      }
      else
         LINK( paf, obj->first_affect, obj->last_affect, next, prev );
      ++top_affect;
      send_to_char( "Object affect added.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "rmaffect" ) )
   {
      AFFECT_DATA *paf;
      short loc, count;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> rmaffect <affect#>\n\r", ch );
         return;
      }
      loc = atoi( argument );
      if( loc < 1 )
      {
         send_to_char( "Invalid number.\n\r", ch );
         return;
      }

      count = 0;

      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      {
         OBJ_INDEX_DATA *pObjIndex;

         pObjIndex = obj->pIndexData;
         for( paf = pObjIndex->first_affect; paf; paf = paf->next )
         {
            if( ++count == loc )
            {
               UNLINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
               if( paf->location != APPLY_WEARSPELL && paf->location != APPLY_REMOVESPELL && paf->location != APPLY_STRIPSN
                && paf->location != APPLY_WEAPONSPELL )
               {
                  CHAR_DATA *vch;
                  OBJ_DATA *eq;

                  for( vch = first_char; vch; vch = vch->next )
                  {
                     for( eq = vch->first_carrying; eq; eq = eq->next_content )
                     {
                        if( eq->pIndexData == pObjIndex && eq->wear_loc != WEAR_NONE )
                           affect_modify( vch, paf, FALSE );
                     }
                  }
               }
               DISPOSE( paf );
               send_to_char( "Removed.\n\r", ch );
               --top_affect;
               return;
            }
         }
      }
      else
      {
         for( paf = obj->first_affect; paf; paf = paf->next )
         {
            if( ++count == loc )
            {
               UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
               DISPOSE( paf );
               send_to_char( "Object affect removed.\n\r", ch );
               --top_affect;
               return;
            }
         }
         send_to_char( "Object affect not found.\n\r", ch );
         return;
      }
   }

   if( !str_cmp( arg2, "ed" ) )
   {
      if( !arg3 || arg3[0] == '\0' )
      {
         send_to_char( "Syntax: oset <object> ed <keywords>\n\r", ch );
         return;
      }
      CHECK_SUBRESTRICTED( ch );
      if( obj->timer )
      {
         send_to_char( "It's not safe to edit an extra description on an object with a timer.\n\rTurn it off first.\n\r",
                       ch );
         return;
      }
      if( obj->item_type == ITEM_PAPER )
      {
         send_to_char( "You can not add an extra description to a note paper at the moment.\n\r", ch );
         return;
      }
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         ed = SetOExtraProto( obj->pIndexData, arg3 );
      else
         ed = SetOExtra( obj, arg3 );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      if( lockobj )
         ch->pcdata->spare_ptr = obj;
      else
         ch->pcdata->spare_ptr = NULL;
      ch->substate = SUB_OBJ_EXTRA;
      ch->pcdata->dest_buf = ed;
      if( !ed->extradesc || ed->extradesc[0] == '\0' )
         ed->extradesc = STRALLOC( "" );
      start_editing( ch, ed->extradesc );
      editor_desc_printf( ch, "Extra description '%s' on object vnum %d (%s).",
                          arg3, obj->pIndexData->vnum, obj->short_descr );
      return;
   }

   if( !str_cmp( arg2, "rmed" ) )
   {
      if( !arg3 || arg3[0] == '\0' )
      {
         send_to_char( "Syntax: oset <object> rmed <keywords>\n\r", ch );
         return;
      }
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      {
         if( DelOExtraProto( obj->pIndexData, arg3 ) )
            send_to_char( "Deleted.\n\r", ch );
         else
            send_to_char( "Not found.\n\r", ch );
         return;
      }
      if( DelOExtra( obj, arg3 ) )
         send_to_char( "Deleted.\n\r", ch );
      else
         send_to_char( "Not found.\n\r", ch );
      return;
   }
   /*
    * save some finger-leather
    */
   if( !str_cmp( arg2, "ris" ) )
   {
      funcf( ch, do_oset, "%s affect resistant %s", arg1, arg3 );
      funcf( ch, do_oset, "%s affect immune %s", arg1, arg3 );
      funcf( ch, do_oset, "%s affect susceptible %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "r" ) )
   {
      funcf( ch, do_oset, "%s affect resistant %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "i" ) )
   {
      funcf( ch, do_oset, "%s affect immune %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "s" ) )
   {
      funcf( ch, do_oset, "%s affect susceptible %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "ri" ) )
   {
      funcf( ch, do_oset, "%s affect resistant %s", arg1, arg3 );
      funcf( ch, do_oset, "%s affect immune %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "rs" ) )
   {
      funcf( ch, do_oset, "%s affect resistant %s", arg1, arg3 );
      funcf( ch, do_oset, "%s affect susceptible %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "is" ) )
   {
      funcf( ch, do_oset, "%s affect immune %s", arg1, arg3 );
      funcf( ch, do_oset, "%s affect susceptible %s", arg1, arg3 );
      return;
   }

   /*
    * Make it easier to set special object values by name than number
    *                  -Thoric
    */
   tmp = -1;
   switch ( obj->item_type )
   {
      case ITEM_PROJECTILE:
         if( !str_cmp( arg2, "missiletype" ) )
         {
            unsigned int x;

            value = -1;
            for( x = 0; x < sizeof( projectiles ) / sizeof( projectiles[0] ); x++ )
               if( !str_cmp( arg3, projectiles[x] ) )
                  value = x;
            if( value < 0 )
            {
               send_to_char( "Unknown projectile type.\n\r", ch );
               return;
            }
            tmp = 4;
            break;
         }

         if( !str_cmp( arg2, "damtype" ) )
         {
            unsigned int x;

            value = -1;
            for( x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); x++ )
               if( !str_cmp( arg3, attack_table[x] ) )
                  value = x;
            if( value < 0 )
            {
               send_to_char( "Unknown damage type.\n\r", ch );
               return;
            }
            tmp = 3;
            break;
         }
      case ITEM_WEAPON:
         if( !str_cmp( arg2, "weapontype" ) )
         {
            unsigned int x;

            value = -1;
            for( x = 0; x < sizeof( weapon_skills ) / sizeof( weapon_skills[0] ); x++ )
               if( !str_cmp( arg3, weapon_skills[x] ) )
                  value = x;
            if( value < 0 )
            {
               send_to_char( "Unknown weapon type.\n\r", ch );
               return;
            }
            tmp = 4;
            break;
         }

         if( !str_cmp( arg2, "damtype" ) )
         {
            unsigned int x;

            value = -1;
            for( x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); x++ )
               if( !str_cmp( arg3, attack_table[x] ) )
                  value = x;
            if( value < 0 )
            {
               send_to_char( "Unknown damage type.\n\r", ch );
               return;
            }
            tmp = 3;
            break;
         }
         if( !str_cmp( arg2, "condition" ) )
            tmp = 0;
         if( !str_cmp( arg2, "damage" ) )
            tmp = 6;
         break;
      case ITEM_ARMOR:
         if( !str_cmp( arg2, "condition" ) )
            tmp = 0;
         if( !str_cmp( arg2, "ac" ) )
            tmp = 1;
         break;
      case ITEM_SALVE:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "maxdoses" ) )
            tmp = 1;
         if( !str_cmp( arg2, "doses" ) )
            tmp = 2;
         if( !str_cmp( arg2, "delay" ) )
            tmp = 3;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 4;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 5;
         if( tmp >= 4 && tmp <= 5 )
            value = skill_lookup( arg3 );
         break;
      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_PILL:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 1;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 2;
         if( !str_cmp( arg2, "spell3" ) )
            tmp = 3;
         if( tmp >= 1 && tmp <= 3 )
            value = skill_lookup( arg3 );
         break;
      case ITEM_STAFF:
      case ITEM_WAND:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell" ) )
         {
            tmp = 3;
            value = skill_lookup( arg3 );
         }
         if( !str_cmp( arg2, "maxcharges" ) )
            tmp = 1;
         if( !str_cmp( arg2, "charges" ) )
            tmp = 2;
         break;
      case ITEM_CONTAINER:
         if( !str_cmp( arg2, "capacity" ) )
            tmp = 0;
         if( !str_cmp( arg2, "cflags" ) )
            tmp = 1;
         if( !str_cmp( arg2, "key" ) )
            tmp = 2;
         break;
      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
      case ITEM_BUTTON:
         if( !str_cmp( arg2, "tflags" ) )
         {
            int tmpval = 0;

            tmp = 0;
            argument = arg3;
            while( argument && argument[0] != '\0' )
            {
               argument = one_argument( argument, arg3 );
               value = get_trigflag( arg3 );
               if( value < 0 || value > 31 )
                  ch_printf( ch, "Invalid tflag %s\r\n", arg3 );
               else
                  tmpval += ( 1 << value );
            }
            value = tmpval;
         }
         break;
   }
   if( tmp >= 0 && tmp <= 10 )
   {
      obj->value[tmp] = value;
      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         obj->pIndexData->value[tmp] = value;
      send_to_char( "Object value set.\n\r", ch );
      return;
   }

   /*
    * Generate usage message.
    */
   if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_oset;
   }
   else
      do_oset( ch, "" );
   return;
}

/*
 * Returns value 0 - 9 based on directional text.
 */
int get_dir( char *txt )
{
   int edir;
   char c1, c2;

   if( !str_cmp( txt, "north" ) )
      return DIR_NORTH;
   if( !str_cmp( txt, "south" ) )
      return DIR_SOUTH;
   if( !str_cmp( txt, "east" ) )
      return DIR_EAST;
   if( !str_cmp( txt, "west" ) )
      return DIR_WEST;
   if( !str_cmp( txt, "up" ) )
      return DIR_UP;
   if( !str_cmp( txt, "down" ) )
      return DIR_DOWN;
   if( !str_cmp( txt, "northeast" ) )
      return DIR_NORTHEAST;
   if( !str_cmp( txt, "northwest" ) )
      return DIR_NORTHWEST;
   if( !str_cmp( txt, "southeast" ) )
      return DIR_SOUTHEAST;
   if( !str_cmp( txt, "southwest" ) )
      return DIR_SOUTHWEST;
   if( !str_cmp( txt, "somewhere" ) )
      return DIR_SOMEWHERE;

   c1 = txt[0];
   if( c1 == '\0' )
      return 0;
   c2 = txt[1];
   edir = 0;
   switch ( c1 )
   {
      case 'n':
         switch ( c2 )
         {
            default:
               edir = 0;
               break;   /* north */
            case 'e':
               edir = 6;
               break;   /* ne   */
            case 'w':
               edir = 7;
               break;   /* nw   */
         }
         break;
      case '0':
         edir = 0;
         break;   /* north */
      case 'e':
      case '1':
         edir = 1;
         break;   /* east  */
      case 's':
         switch ( c2 )
         {
            default:
               edir = 2;
               break;   /* south */
            case 'e':
               edir = 8;
               break;   /* se   */
            case 'w':
               edir = 9;
               break;   /* sw   */
         }
         break;
      case '2':
         edir = 2;
         break;   /* south */
      case 'w':
      case '3':
         edir = 3;
         break;   /* west  */
      case 'u':
      case '4':
         edir = 4;
         break;   /* up    */
      case 'd':
      case '5':
         edir = 5;
         break;   /* down  */
      case '6':
         edir = 6;
         break;   /* ne   */
      case '7':
         edir = 7;
         break;   /* nw   */
      case '8':
         edir = 8;
         break;   /* se   */
      case '9':
         edir = 9;
         break;   /* sw   */
      case '?':
         edir = 10;
         break;   /* somewhere */
   }
   return edir;
}

/*
 * Function to get an exit, leading the the specified room
 */
EXIT_DATA *get_exit_to( ROOM_INDEX_DATA * room, short dir, int vnum )
{
   EXIT_DATA *xit;

   if( !room )
   {
      bug( "%s", "Get_exit: NULL room" );
      return NULL;
   }

   for( xit = room->first_exit; xit; xit = xit->next )
      if( xit->vdir == dir && xit->vnum == vnum )
         return xit;
   return NULL;
}

/*
 * Function to get the nth exit of a room			-Thoric
 */
EXIT_DATA *get_exit_num( ROOM_INDEX_DATA * room, short count )
{
   EXIT_DATA *xit;
   int cnt;

   if( !room )
   {
      bug( "%s", "Get_exit_num: NULL room" );
      return NULL;
   }

   for( cnt = 0, xit = room->first_exit; xit; xit = xit->next )
      if( ++cnt == count )
         return xit;
   return NULL;
}

/* Modified by Samson to allow editing sector types by name */
CMDF do_redit( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], arg2[MIL], arg3[MIL];
   ROOM_INDEX_DATA *location, *tmp;
   EXTRA_DESCR_DATA *ed;
   char dir = 0;
   EXIT_DATA *xit, *texit;
   int value = 0, edir = 0, ekey, evnum;
   char *origarg = argument;

   set_char_color( AT_PLAIN, ch );
   if( !ch->desc )
   {
      send_to_char( "You have no descriptor.\n\r", ch );
      return;
   }

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPCs can't redit\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_ROOM_DESC:
         location = ( ROOM_INDEX_DATA * ) ch->pcdata->dest_buf;
         if( !location )
         {
            bug( "%s", "redit: sub_room_desc: NULL ch->pcdata->dest_buf" );
            location = ch->in_room;
         }
         DISPOSE( location->roomdesc );
         location->roomdesc = copy_buffer_nohash( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         return;
      case SUB_ROOM_DESC_NITE:  /* NiteDesc by Dracones */
         location = ( ROOM_INDEX_DATA * ) ch->pcdata->dest_buf;
         if( !location )
         {
            bug( "%s", "redit: sub_room_desc_nite: NULL ch->pcdata->dest_buf" );
            location = ch->in_room;
         }
         DISPOSE( location->nitedesc );
         location->nitedesc = copy_buffer_nohash( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         return;
      case SUB_ROOM_EXTRA:
         ed = ( EXTRA_DESCR_DATA * ) ch->pcdata->dest_buf;
         if( !ed )
         {
            bug( "%s", "redit: sub_room_extra: NULL ch->pcdata->dest_buf" );
            stop_editing( ch );
            return;
         }
         STRFREE( ed->extradesc );
         ed->extradesc = copy_buffer( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting description.\n\r", ch );
         return;
   }

   location = ch->in_room;

   smash_tilde( argument );
   argument = one_argument( argument, arg );
   if( ch->substate == SUB_REPEATCMD )
   {
      if( !arg || arg[0] == '\0' )
      {
         do_rstat( ch, "" );
         return;
      }
      if( !str_cmp( arg, "done" ) || !str_cmp( arg, "off" ) )
      {
         send_to_char( "Redit mode off.\n\r", ch );
         STRFREE( ch->pcdata->subprompt );
         ch->substate = SUB_NONE;
         return;
      }
   }
   if( !arg || arg[0] == '\0' || !str_cmp( arg, "?" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         send_to_char( "Syntax: <field> value\n\r", ch );
      else
         send_to_char( "Syntax: redit <field> value\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Field being one of:\n\r", ch );
      send_to_char( "  name desc nitedesc ed rmed\n\r", ch );
      send_to_char( "  exit bexit exdesc exflags exname exkey excoord\n\r", ch );
      send_to_char( "  flags sector teledelay televnum tunnel\n\r", ch );
      send_to_char( "  rlist pulltype pull push\n\r", ch );
      if( IS_IMP( ch ) )
         send_to_char( "  undelete\n\r", ch );
      return;
   }

   if( !can_rmodify( ch, location ) )
      return;

   if( !str_cmp( arg, "on" ) )
   {
      CHECK_SUBRESTRICTED( ch );
      send_to_char( "Redit mode on.\n\r", ch );
      ch->substate = SUB_REPEATCMD;
      STRFREE( ch->pcdata->subprompt );
      ch->pcdata->subprompt = STRALLOC( "<&CRedit &W#%r&w> %i" );
      return;
   }

   if( !str_cmp( arg, "name" ) )
   {
      if( argument[0] == '\0' )
      {
         send_to_char( "Set the room name.  A very brief single line room description.\n\r", ch );
         send_to_char( "Usage: redit name <Room summary>\n\r", ch );
         return;
      }
      STRFREE( location->name );
      location->name = STRALLOC( argument );
      send_to_char( "Room name set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "desc" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_ROOM_DESC;
      ch->pcdata->dest_buf = location;
      if( !location->roomdesc || location->roomdesc[0] == '\0' )
         location->roomdesc = str_dup( "" );
      start_editing( ch, location->roomdesc );
      editor_desc_printf( ch, "Description of room vnum %d (%s).", location->vnum, location->name );
      return;
   }

   /*
    * nitedesc editing by Dracones 
    */
   if( !str_cmp( arg, "nitedesc" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_ROOM_DESC_NITE;
      ch->pcdata->dest_buf = location;
      if( !location->nitedesc || location->nitedesc[0] == '\0' )
         location->nitedesc = str_dup( "" );
      start_editing( ch, location->nitedesc );
      editor_desc_printf( ch, "Night description of room vnum %d (%s).", location->vnum, location->name );
      return;
   }

   if( !str_cmp( arg, "tunnel" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Set the maximum characters allowed in the room at one time. (0 = unlimited).\n\r", ch );
         send_to_char( "Usage: redit tunnel <value>\n\r", ch );
         return;
      }
      location->tunnel = URANGE( 0, atoi( argument ), 1000 );
      send_to_char( "Tunnel value set.\n\r", ch );
      return;
   }

   /*
    * Crash fix and name support by Shaddai 
    */
   if( !str_cmp( arg, "affect" ) )
   {
      AFFECT_DATA *paf;
      EXT_BV risabit;
      short loc;
      bool found = false;

      xCLEAR_BITS( risabit );

      argument = one_argument( argument, arg2 );
      if( !arg2 || arg2[0] == '\0' || !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: redit affect <field> <value>\n\r", ch );
         return;
      }
      loc = get_atype( arg2 );
      if( loc < 1 )
      {
         ch_printf( ch, "Unknown field: %s\n\r", arg2 );
         return;
      }
      if( loc == APPLY_AFFECT )
      {
         argument = one_argument( argument, arg3 );
         if( loc == APPLY_AFFECT )
         {
            value = get_aflag( arg3 );

            if( value < 0 || value >= MAX_AFFECTED_BY )
               ch_printf( ch, "Unknown affect: %s\n\r", arg3 );
            else
               found = true;
         }
      }
      else if( loc == APPLY_RESISTANT || loc == APPLY_IMMUNE || loc == APPLY_SUSCEPTIBLE || loc == APPLY_ABSORB )
      {
         char *risa = argument;
         char flag[MIL];

         while( risa[0] != '\0' )
         {
            risa = one_argument( risa, flag );
            value = get_risflag( flag );

            if( value < 0 || value >= MAX_RIS_FLAG )
               ch_printf( ch, "Unknown flag: %s\n\r", flag );
            else
            {
               xSET_BIT( risabit, value );
               found = true;
            }
         }
      }
      else if( loc == APPLY_WEAPONSPELL
               || loc == APPLY_WEARSPELL
               || loc == APPLY_REMOVESPELL || loc == APPLY_STRIPSN || loc == APPLY_RECURRINGSPELL || loc == APPLY_EAT_SPELL )
      {
         value = skill_lookup( argument );

         if( !IS_VALID_SN( value ) )
            ch_printf( ch, "Invalid spell: %s", argument );
         else
            found = true;
      }
      else
      {
         value = atoi( argument );
         found = true;
      }
      if( !found )
         return;

      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      paf->location = loc;
      paf->modifier = value;
      paf->rismod = risabit;
      paf->bit = 0;
      LINK( paf, location->first_affect, location->last_affect, next, prev );
      ++top_affect;
      send_to_char( "Room affect added.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "rmaffect" ) )
   {
      AFFECT_DATA *paf;
      short loc, count;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: redit rmaffect <affect#>\n\r", ch );
         return;
      }
      loc = atoi( argument );
      if( loc < 1 )
      {
         send_to_char( "Invalid number.\n\r", ch );
         return;
      }

      count = 0;
      for( paf = location->first_affect; paf; paf = paf->next )
      {
         if( ++count == loc )
         {
            UNLINK( paf, location->first_affect, location->last_affect, next, prev );
            DISPOSE( paf );
            send_to_char( "Room affect removed.\n\r", ch );
            --top_affect;
            return;
         }
      }
      send_to_char( "Room affect not found.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "ed" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Create an extra description.\n\r", ch );
         send_to_char( "You must supply keyword(s).\n\r", ch );
         return;
      }
      CHECK_SUBRESTRICTED( ch );
      ed = SetRExtra( location, argument );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_ROOM_EXTRA;
      ch->pcdata->dest_buf = ed;
      if( ed->extradesc == NULL )
         ed->extradesc = STRALLOC( "" );
      start_editing( ch, ed->extradesc );
      editor_desc_printf( ch, "Extra description '%s' on room %d (%s).", argument, location->vnum, location->name );
      return;
   }

   if( !str_cmp( arg, "rmed" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Remove an extra description.\n\r", ch );
         send_to_char( "You must supply keyword(s).\n\r", ch );
         return;
      }
      if( DelRExtra( location, argument ) )
         send_to_char( "Extra description deleted.\n\r", ch );
      else
         send_to_char( "Extra description not found.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "rlist" ) )
   {
      reset_data *pReset;
      char *rbuf;
      short num;

      if( !location->first_reset )
      {
         send_to_char( "This room has no resets to list.\n\r", ch );
         return;
      }
      num = 0;
      for( pReset = location->first_reset; pReset; pReset = pReset->next )
      {
         num++;
         if( !( rbuf = sprint_reset( pReset, num ) ) )
            continue;
         send_to_char( rbuf, ch );
      }
      return;
   }

   if( !str_cmp( arg, "flags" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Toggle the room flags.\n\r", ch );
         send_to_char( "Usage: redit flags <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg2 );
         value = get_rflag( arg2 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg2 );
         else
         {
            if( value == ROOM_PROTOTYPE && ch->level < sysdata.level_modify_proto )
               send_to_char( "You cannot change the prototype flag.\n\r", ch );
            else
               xTOGGLE_BIT( location->room_flags, value );
         }
      }
      send_to_char( "Room flags set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "teledelay" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Set the delay of the teleport. (0 = off).\n\r", ch );
         send_to_char( "Usage: redit teledelay <value>\n\r", ch );
         return;
      }
      location->tele_delay = atoi( argument );
      send_to_char( "Teledelay set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "televnum" ) )
   {
      ROOM_INDEX_DATA *temp = NULL;
      int tvnum;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Set the vnum of the room to teleport to.\n\r", ch );
         send_to_char( "Usage: redit televnum <vnum>\n\r", ch );
         return;
      }
      tvnum = atoi( argument );
      if( tvnum < 1 || tvnum > sysdata.maxvnum )
      {
         ch_printf( ch, "Invalid vnum. Allowable range is 1 to %d\n\r", sysdata.maxvnum );
         return;
      }
      if( ( temp = get_room_index( tvnum ) ) == NULL )
      {
         send_to_char( "Target vnum does not exist yet.\n\r", ch );
         return;
      }
      location->tele_vnum = tvnum;
      send_to_char( "Televnum set.\n\r", ch );
      return;
   }

   /*
    * Sector editing now handled via name instead of numerical value - Samson 
    */
   if( !str_cmp( arg, "sector" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Set the sector type.\n\r", ch );
         send_to_char( "Usage: redit sector <name>\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg2 );
      value = get_sectypes( arg2 );
      if( value < 0 || value >= SECT_MAX )
      {
         location->sector_type = 1;
         send_to_char( "Invalid sector type, set to city by default.\n\r", ch );
      }
      else
      {
         location->sector_type = value;
         ch_printf( ch, "Sector type set to %s.\n\r", arg2 );
      }
      return;
   }

   if( !str_cmp( arg, "exkey" ) )
   {
      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( arg2[0] == '\0' || arg3[0] == '\0' )
      {
         send_to_char( "Usage: redit exkey <dir> <key vnum>\n\r", ch );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      value = atoi( arg3 );
      if( !xit )
      {
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\n\r", ch );
         return;
      }
      xit->key = value;
      send_to_char( "Exit key vnum set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "excoord" ) )
   {
      int x, y;

      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( arg2[0] == '\0' || arg3[0] == '\0' || argument[0] == '\0' )
      {
         send_to_char( "Usage: redit excoord <dir> <X> <Y>\n\r", ch );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }

      x = atoi( arg3 );
      y = atoi( argument );

      if( x < 0 || x >= MAX_X )
      {
         ch_printf( ch, "Valid X coordinates are 0 to %d.\n\r", MAX_X - 1 );
         return;
      }

      if( y < 0 || y >= MAX_Y )
      {
         ch_printf( ch, "Valid Y coordinates are 0 to %d.\n\r", MAX_Y - 1 );
         return;
      }

      if( !xit )
      {
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\n\r", ch );
         return;
      }
      xit->x = x;
      xit->y = y;
      send_to_char( "Exit coordinates set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "exname" ) )
   {
      argument = one_argument( argument, arg2 );
      if( !arg2 || arg2[0] == '\0' )
      {
         send_to_char( "Change or clear exit keywords.\n\r", ch );
         send_to_char( "Usage: redit exname <dir> [keywords]\n\r", ch );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      if( !xit )
      {
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\n\r", ch );
         return;
      }
      STRFREE( xit->keyword );
      xit->keyword = STRALLOC( argument );
      send_to_char( "Exit keywords set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "exflags" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Toggle or display exit flags.\n\r", ch );
         send_to_char( "Usage: redit exflags <dir> <flag> [flag]...\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg2 );
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      if( !xit )
      {
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\n\r", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         ch_printf( ch, "Flags for exit direction: %d  Keywords: %s  Key: %d\n\r[ %s ]\n\r",
                    xit->vdir, xit->keyword, xit->key, ext_flag_string( &xit->exit_info, ex_flags ) );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg2 );
         value = get_exflag( arg2 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg2 );
         else
            xTOGGLE_BIT( xit->exit_info, value );
      }
      send_to_char( "Exit flags set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "exit" ) )
   {
      bool addexit, numnotdir;

      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( !arg2 || arg2[0] == '\0' )
      {
         send_to_char( "Create, change or remove an exit.\r\n", ch );
         send_to_char( "Usage: redit exit <dir> [room] [key] [keyword] [flags]\r\n", ch );
         return;
      }

      // Pick a direction. Variable edir assumes this value once set.
      addexit = numnotdir = FALSE;
      switch( arg2[0] )
      {
         default:
            edir = get_dir( arg2 );
            break;
         case '+':
            edir = get_dir( arg2 + 1 );
            addexit = TRUE;
            break;
         case '#':
            edir = atoi( arg2 + 1 );
            numnotdir = TRUE;
            break;
      }

      // Pick a target room number for the exit. Set to 0 if not found.
      if( !arg3 || arg3[0] == '\0' )
         evnum = 0;
      else
         evnum = atoi( arg3 );

      if( numnotdir )
      {
         if( ( xit = get_exit_num( location, edir ) ) != NULL )
            edir = xit->vdir;
      }
      else
         xit = get_exit( location, edir );

      // If the evnum value is 0, delete this exit and ignore all other arguments to the command.
      if( !evnum )
      {
         if( xit )
         {
            extract_exit( location, xit );
            send_to_char( "Exit removed.\r\n", ch );
            return;
         }
         send_to_char( "No exit in that direction.\r\n", ch );
         return;
      }

      // Validate the target room vnum is within allowable maximums.
      if( evnum < 1 || evnum > ( sysdata.maxvnum - 1 ) )
      {
         send_to_char( "Invalid room number.\r\n", ch );
         return;
      }

      // Check for existing target room....
      if( !( tmp = get_room_index( evnum ) ) )
      {
         // If outside the person's vnum range, bail out. FIXME: Check for people who can edit globally.
         if( evnum < ch->pcdata->low_vnum || evnum > ch->pcdata->hi_vnum )
         {
            ch_printf( ch, "Room #%d does not exist.\r\n", evnum );
            return;
         }

         // Create the target room if the vnum did not exist yet.
         tmp = make_room( evnum, ch->pcdata->area );
         if( !tmp )
         {
            bug( "%s: make_room failed", __FUNCTION__ );
            return;
         }
      }

      // Actually add or change the exit affected.
      if( addexit || !xit )
      {
         if( numnotdir )
         {
            send_to_char( "Cannot add an exit by number, sorry.\r\n", ch );
            return;
         }
         if( addexit && xit && get_exit_to( location, edir, tmp->vnum ) )
         {
            send_to_char( "There is already an exit in that direction leading to that location.\r\n", ch );
            return;
         }
         xit = make_exit( location, tmp, edir );
         xit->key = -1;
         xCLEAR_BITS( xit->exit_info );
         act( AT_IMMORT, "$n reveals a hidden passage!", ch, NULL, NULL, TO_ROOM );
      }
      else
         act( AT_IMMORT, "Something is different...", ch, NULL, NULL, TO_ROOM );

      // A sanity check to make sure it got sent to the proper place.
      if( xit->to_room != tmp )
      {
         xit->to_room = tmp;
         xit->vnum = evnum;
         texit = get_exit_to( xit->to_room, rev_dir[edir], location->vnum );
         if( texit )
         {
            texit->rexit = xit;
            xit->rexit = texit;
         }
      }

      // Set the vnum of the key required to unlock this exit.
      argument = one_argument( argument, arg3 );
      if( arg3 && arg3[0] != '\0' )
      {
         ekey = atoi( arg3 );
         if( ekey != 0 || arg3[0] == '0' )
            xit->key = ekey;
      }

      // Set a keyword on this exit. "door", "gate", etc. Only accepts *ONE* keyword.
      argument = one_argument( argument, arg3 );
      if( arg3 && arg3[0] != '\0' )
      {
         STRFREE( xit->keyword );
         xit->keyword = STRALLOC( arg3 );
      }

      // And finally set any flags which have been specified.
      if( argument && argument[0] != '\0' )
      {
         while( argument[0] != '\0' )
         {
            argument = one_argument( argument, arg3 );
            value = get_exflag( arg3 );
            if( value < 0 || value > MAX_BITS )
               ch_printf( ch, "Unknown exit flag: %s\r\n", arg3 );
            else
               xTOGGLE_BIT( xit->exit_info, value );
         }
      }

      // WOO! Finally done. Inform the user.
      send_to_char( "New exit added.\r\n", ch );
      return;
   }

   /*
    * Twisted and evil, but works           -Thoric
    * Makes an exit, and the reverse in one shot.
    */
   if( !str_cmp( arg, "bexit" ) )
   {
      EXIT_DATA *bxit, *rxit;
      ROOM_INDEX_DATA *tmploc;
      int vnum, exnum;
      char rvnum[MIL];
      bool numnotdir;

      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( !arg2 || arg2[0] == '\0' )
      {
         send_to_char( "Create, change or remove a two-way exit.\n\r", ch );
         send_to_char( "Usage: redit bexit <dir> [room] [flags] [key] [keywords]\n\r", ch );
         return;
      }
      numnotdir = FALSE;
      switch ( arg2[0] )
      {
         default:
            edir = get_dir( arg2 );
            break;
         case '#':
            numnotdir = TRUE;
            edir = atoi( arg2 + 1 );
            break;
         case '+':
            edir = get_dir( arg2 + 1 );
            break;
      }
      tmploc = location;
      exnum = edir;
      if( numnotdir )
      {
         if( ( bxit = get_exit_num( tmploc, edir ) ) != NULL )
            edir = bxit->vdir;
      }
      else
         bxit = get_exit( tmploc, edir );
      rxit = NULL;
      vnum = 0;
      rvnum[0] = '\0';
      if( bxit )
      {
         vnum = bxit->vnum;
         if( arg3 || arg3[0] != '\0' )
            snprintf( rvnum, MIL, "%d", tmploc->vnum );
         if( bxit->to_room )
            rxit = get_exit( bxit->to_room, rev_dir[edir] );
         else
            rxit = NULL;
      }
      funcf( ch, do_redit, "exit %s %s %s", arg2, arg3, argument );
      if( numnotdir )
         bxit = get_exit_num( tmploc, exnum );
      else
         bxit = get_exit( tmploc, edir );
      if( !rxit && bxit )
      {
         vnum = bxit->vnum;
         if( arg3[0] != '\0' )
            snprintf( rvnum, MIL, "%d", tmploc->vnum );
         if( bxit->to_room )
            rxit = get_exit( bxit->to_room, rev_dir[edir] );
         else
            rxit = NULL;
      }
      if( vnum )
         cmdf( ch, "at %d redit exit %d %s %s", vnum, rev_dir[edir], rvnum, argument );
      return;
   }

   if( !str_cmp( arg, "pulltype" ) || !str_cmp( arg, "pushtype" ) )
   {
      int pt;

      argument = one_argument( argument, arg2 );
      if( !arg2 || arg2[0] == '\0' )
      {
         ch_printf( ch, "Set the %s between this room, and the destination room.\n\r", arg );
         ch_printf( ch, "Usage: redit %s <dir> <type>\n\r", arg );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      if( xit )
      {
         if( ( pt = get_pulltype( argument ) ) == -1 )
            ch_printf( ch, "Unknown pulltype: %s.  (See help PULLTYPES)\n\r", argument );
         else
         {
            xit->pulltype = pt;
            send_to_char( "Done.\n\r", ch );
            return;
         }
      }
      send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "pull" ) )
   {
      argument = one_argument( argument, arg2 );
      if( !arg2 || arg2[0] == '\0' )
      {
         send_to_char( "Set the 'pull' between this room, and the destination room.\n\r", ch );
         send_to_char( "Usage: redit pull <dir> <force (0 to 100)>\n\r", ch );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      if( xit )
      {
         xit->pull = URANGE( -100, atoi( argument ), 100 );
         send_to_char( "Done.\n\r", ch );
         return;
      }
      send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "push" ) )
   {
      argument = one_argument( argument, arg2 );
      if( !arg2 || arg2[0] == '\0' )
      {
         send_to_char( "Set the 'push' away from the destination room in the opposite direction.\n\r", ch );
         send_to_char( "Usage: redit push <dir> <force (0 to 100)>\n\r", ch );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      if( xit )
      {
         xit->pull = URANGE( -100, -( atoi( argument ) ), 100 );
         send_to_char( "Done.\n\r", ch );
         return;
      }
      send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "exdesc" ) )
   {
      argument = one_argument( argument, arg2 );
      if( !arg2 || arg2[0] == '\0' )
      {
         send_to_char( "Create or clear a description for an exit.\n\r", ch );
         send_to_char( "Usage: redit exdesc <dir> [description]\n\r", ch );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      if( xit )
      {
         STRFREE( xit->exitdesc );
         if( argument && argument[0] != '\0' )
            stralloc_printf( &xit->exitdesc, "%s\n\r", argument );
         send_to_char( "Exit description set.\n\r", ch );
         return;
      }
      send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\n\r", ch );
      return;
   }

   /*
    * Generate usage message.
    */
   if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_redit;
   }
   else
      do_redit( ch, "" );
   return;
}

CMDF do_ocreate( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], arg2[MIL];
   OBJ_INDEX_DATA *pObjIndex;
   OBJ_DATA *obj;
   int vnum, cvnum;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobiles cannot create.\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   vnum = is_number( arg ) ? atoi( arg ) : -1;

   if( vnum == -1 || !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage:  ocreate <vnum> [copy vnum] <item name>\n\r", ch );
      return;
   }

   if( vnum < 1 || vnum > sysdata.maxvnum )
   {
      ch_printf( ch, "Invalid vnum. Allowable range is 1 to %d\n\r", sysdata.maxvnum );
      return;
   }

   one_argument( argument, arg2 );
   cvnum = atoi( arg2 );
   if( cvnum != 0 )
      argument = one_argument( argument, arg2 );
   if( cvnum < 1 )
      cvnum = 0;

   if( get_obj_index( vnum ) )
   {
      send_to_char( "An object with that number already exists.\n\r", ch );
      return;
   }

   if( IS_NPC( ch ) )
      return;

   if( get_trust( ch ) < LEVEL_LESSER )
   {
      AREA_DATA *pArea;

      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         send_to_char( "You must have an assigned area to create objects.\n\r", ch );
         return;
      }
      if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
      {
         send_to_char( "That number is not in your allocated range.\n\r", ch );
         return;
      }
   }

   pObjIndex = make_object( vnum, cvnum, argument );
   if( !pObjIndex )
   {
      send_to_char( "Error.\n\r", ch );
      bug( "%s", "do_ocreate: make_object failed." );
      return;
   }
   if( !( obj = create_object( pObjIndex, ch->level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   obj_to_char( obj, ch );

   act( AT_IMMORT, "$n makes arcane gestures, and opens $s hands to reveal $p!", ch, obj, NULL, TO_ROOM );
   ch_printf( ch, "&YYou make arcane gestures, and open your hands to reveal %s!\n\rObjVnum:  &W%d   &YKeywords:  &W%s\n\r",
              pObjIndex->short_descr, pObjIndex->vnum, pObjIndex->name );
}

CMDF do_mcreate( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], arg2[MIL];
   MOB_INDEX_DATA *pMobIndex;
   CHAR_DATA *mob;
   int vnum, cvnum;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobiles cannot create.\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   vnum = is_number( arg ) ? atoi( arg ) : -1;

   if( vnum == -1 || !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage:  mcreate <vnum> [cvnum] <mobile name>\n\r", ch );
      return;
   }

   if( vnum < 1 || vnum > sysdata.maxvnum )
   {
      ch_printf( ch, "Invalid vnum. Allowable range is 1 to %d\n\r", sysdata.maxvnum );
      return;
   }

   one_argument( argument, arg2 );
   cvnum = atoi( arg2 );
   if( cvnum != 0 )
      argument = one_argument( argument, arg2 );
   if( cvnum < 1 )
      cvnum = 0;

   if( get_mob_index( vnum ) )
   {
      send_to_char( "A mobile with that number already exists.\n\r", ch );
      return;
   }

   if( IS_NPC( ch ) )
      return;

   if( get_trust( ch ) < LEVEL_LESSER )
   {
      AREA_DATA *pArea;

      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         send_to_char( "You must have an assigned area to create mobiles.\n\r", ch );
         return;
      }
      if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
      {
         send_to_char( "That number is not in your allocated range.\n\r", ch );
         return;
      }
   }

   pMobIndex = make_mobile( vnum, cvnum, argument );
   if( !pMobIndex )
   {
      send_to_char( "Error.\n\r", ch );
      bug( "%s", "do_mcreate: make_mobile failed." );
      return;
   }
   mob = create_mobile( pMobIndex );
   if( !char_to_room( mob, ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   /*
    * If you create one on the map, make sure it gets placed properly - Samson 8-21-99 
    */
   fix_maps( ch, mob );

   act( AT_IMMORT, "$n waves $s arms about, and $N appears at $s command!", ch, NULL, mob, TO_ROOM );
   ch_printf( ch, "&YYou wave your arms about, and %s appears at your command!\n\rMobVnum:  &W%d   &YKeywords:  &W%s\n\r",
              pMobIndex->short_descr, pMobIndex->vnum, pMobIndex->player_name );
}

void assign_area( CHAR_DATA * ch )
{
   char taf[256];
   AREA_DATA *tarea, *tmp;

   if( IS_NPC( ch ) )
      return;

   if( get_trust( ch ) > LEVEL_IMMORTAL && ch->pcdata->low_vnum && ch->pcdata->hi_vnum )
   {
      tarea = ch->pcdata->area;
      snprintf( taf, 256, "%s.are", capitalize( ch->name ) );
      if( !tarea )
      {
         for( tmp = first_area; tmp; tmp = tmp->next )
            if( !str_cmp( taf, tmp->filename ) )
            {
               tarea = tmp;
               break;
            }
      }
      if( !tarea )
      {
         log_printf_plus( LOG_BUILD, ch->level, "Creating area entry for %s", ch->name );

         tarea = create_area(  );
         strdup_printf( &tarea->name, "[PROTO] %s's area in progress", ch->name );
         tarea->filename = str_dup( taf );
         stralloc_printf( &tarea->author, "%s", ch->name );
         sort_area_name( tarea );
         sort_area_vnums( tarea );
      }
      else
         log_printf_plus( LOG_BUILD, ch->level, "Updating area entry for %s", ch->name );

      tarea->low_vnum = ch->pcdata->low_vnum;
      tarea->hi_vnum = ch->pcdata->hi_vnum;
      ch->pcdata->area = tarea;
   }
}

/* Function modified from original form - Samson */
/* Function mostly rewritten by Xorith */
CMDF do_aassign( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *tarea = NULL;

   set_char_color( AT_IMMORT, ch );

   if( IS_NPC( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: aassign <filename.are>  - Assigns you an area for building.\n\r", ch );
      send_to_char( "        aassign none/null/clear - Clears your assigned area and restores your "
                    "building area (if any).\n\r", ch );
      if( get_trust( ch ) < LEVEL_GOD )
         send_to_char( "Note: You can only aassign areas bestowed upon you.\n\r", ch );
      if( get_trust( ch ) < sysdata.level_modify_proto )
         send_to_char( "Note: You can only aassign areas that are marked as prototype.\n\r", ch );
      return;
   }

   if( !str_cmp( "none", argument ) || !str_cmp( "null", argument ) || !str_cmp( "clear", argument ) )
   {
      ch->pcdata->area = NULL;
      assign_area( ch );
      if( !ch->pcdata->area )
         send_to_char( "Area pointer cleared.\n\r", ch );
      else
         send_to_char( "Originally assigned area restored.\n\r", ch );
      return;
   }

   if( get_trust( ch ) < LEVEL_GOD && !hasname( ch->pcdata->bestowments, argument ) )
   {
      send_to_char( "You must have your desired area bestowed upon you by your superiors.\n\r", ch );
      return;
   }

   for( tarea = first_area; tarea; tarea = tarea->next )
      if( !str_cmp( argument, tarea->filename ) )
         break;

   if( !tarea )
   {
      ch_printf( ch, "The area '%s' does not exsist. Please use the 'zones' command for a list.\n\r", argument );
      return;
   }

   if( !IS_SET( tarea->flags, AFLAG_PROTOTYPE ) && get_trust( ch ) < sysdata.level_modify_proto )
   {
      ch_printf( ch, "The area '%s' is not a proto area, and you're not authorized to work on non-proto areas.\n\r",
                 tarea->name );
      return;
   }

   ch->pcdata->area = tarea;
   ch_printf( ch, "Assigning you: %s\n\r", tarea->name );
   log_printf( "Assigning %s to %s.", tarea->name, ch->name );
   return;
}

/* Moved here from mud.h cause it's just easier to compile 1 file :) */
/* Stock value of 1 */
/* Raised to 2 by Samson mob sex+race, obj type converted to text */
/* Raised to 3 by Samson when mob Class converted to text */
/* Raised to 4 by Samson when room sector converted to text */
/* Raised to 5 by Samson when mob+obj+room BV flags converted to text */
/* Raised to 6 by Samson when mob hitplus became the only HP stat saved */
/* Raised to 7 by Samson to add overland map coordinates to exits */
/* Raised to 8 by Samson to add absorb flag to PCs/Mobs */
/* Raised to 9 by Samson after cleaning up hit/dam code */
/* Raised to 10 by Samson after making the format more readable */
/* Raised to 11 by Samson after the damned horse movement project blew up for the 4th time */
/* Raised to 12 after converting exit flags to EXT_BV and making them save as text. */
/* Raised to 13 to auto-convert broken Overland exit coords */
/* Raised to 14 to support nighttime room descriptions */
/* Raised to 15 to synchronize thac0 values on old mobs */
/* Raised to 16 to fix old mob xp values */
/* Raised to 17 to retrofit old mobs for random treasure code */
/* Raised to 18 for armorgen retrofits ( Could this be getting out of hand? ) */
/* Raised to 19 to activate overland resets */
/* Raised to 20 for object affect modifiers + official AFKMud 2.0 format */
#define AREA_VERSION_WRITE 20

/* Function modified from original form - Samson */
void fold_area( AREA_DATA * tarea, char *filename, bool install )
{
   RESET_DATA *pReset, *tReset, *gReset;
   ROOM_INDEX_DATA *room;
   MOB_INDEX_DATA *pMobIndex;
   OBJ_INDEX_DATA *pObjIndex;
   MPROG_DATA *mprog;
   EXIT_DATA *xit;
   EXTRA_DESCR_DATA *ed;
   AFFECT_DATA *paf;
   SHOP_DATA *pShop;
   REPAIR_DATA *pRepair;
   NEIGHBOR_DATA *neigh;
   char buf[256];
   FILE *fpout;
   int vnum, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, val10;

   log_printf_plus( LOG_BUILD, LEVEL_GREATER, "Saving %s...", tarea->filename );

   snprintf( buf, 256, "%s.bak", filename );
   rename( filename, buf );
   if( !( fpout = fopen( filename, "w" ) ) )
   {
      bug( "%s", "fold_area: fopen" );
      perror( filename );
      return;
   }

   tarea->version = AREA_VERSION_WRITE;

   fprintf( fpout, "#AREA   %s~\n\n", tarea->name );
   fprintf( fpout, "#VERSION %d\n\n", AREA_VERSION_WRITE );
   fprintf( fpout, "#AUTHOR %s~\n\n", tarea->author );
   fprintf( fpout, "#VNUMS %d %d\n\n", tarea->low_vnum, tarea->hi_vnum );
   fprintf( fpout, "%s", "#RANGES\n" );
   fprintf( fpout, "%d %d %d %d\n", tarea->low_soft_range, tarea->hi_soft_range, tarea->low_hard_range,
            tarea->hi_hard_range );
   fprintf( fpout, "%s", "$\n\n" );
   if( tarea->resetmsg )   /* Rennard */
      fprintf( fpout, "#RESETMSG %s~\n\n", tarea->resetmsg );
   if( tarea->reset_frequency )
      fprintf( fpout, "#RESETFREQUENCY %d\n\n", tarea->reset_frequency );
   fprintf( fpout, "#FLAGS\n%s~\n\n", flag_string( tarea->flags, area_flags ) );
   fprintf( fpout, "#CONTINENT %s~\n\n", continents[tarea->continent] );
   fprintf( fpout, "#COORDS %d %d\n\n", tarea->x, tarea->y );
   fprintf( fpout, "#CLIMATE %d %d %d\n\n", tarea->weather->climate_temp,
            tarea->weather->climate_precip, tarea->weather->climate_wind );
   fprintf( fpout, "%s", "#TREASURE\n" );
   fprintf( fpout, "%d %d %d %d\n", tarea->tg_nothing, tarea->tg_gold, tarea->tg_item, tarea->tg_gem );
   fprintf( fpout, "%d %d %d %d\n\n", tarea->tg_scroll, tarea->tg_potion, tarea->tg_wand, tarea->tg_armor );
   /*
    * neighboring weather systems - FB 
    */
   for( neigh = tarea->weather->first_neighbor; neigh; neigh = neigh->next )
      fprintf( fpout, "#NEIGHBOR %s~\n\n", neigh->name );

   /*
    * save mobiles 
    */
   fprintf( fpout, "%s", "#MOBILES\n" );
   for( vnum = tarea->low_vnum; vnum <= tarea->hi_vnum; vnum++ )
   {
      if( !( pMobIndex = get_mob_index( vnum ) ) )
         continue;
      if( install )
         REMOVE_ACT_FLAG( pMobIndex, ACT_PROTOTYPE );
      fprintf( fpout, "#%d\n", vnum );
      fprintf( fpout, "%s~\n", pMobIndex->player_name );
      fprintf( fpout, "%s~\n", pMobIndex->short_descr );
      if( pMobIndex->long_descr && pMobIndex->long_descr[0] != '\0' )
         fprintf( fpout, "%s~\n", strip_cr( pMobIndex->long_descr ) );
      else
         fprintf( fpout, "%s", "~\n" );
      if( !pMobIndex->chardesc || pMobIndex->chardesc[0] == '\0' )
         fprintf( fpout, "%s", "~\n" );
      else
         fprintf( fpout, "%s~\n", strip_cr( pMobIndex->chardesc ) );
      fprintf( fpout, "%s~\n", ext_flag_string( &pMobIndex->act, act_flags ) );
      fprintf( fpout, "%s~\n", ext_flag_string( &pMobIndex->affected_by, a_flags ) );
      fprintf( fpout, "%d %d %d %d %d %d %d %f\n", pMobIndex->alignment,
               pMobIndex->gold, 0, pMobIndex->height, pMobIndex->weight, pMobIndex->max_move, pMobIndex->max_mana,
               pMobIndex->numattacks );
      fprintf( fpout, "%d %d %d %d ", pMobIndex->level, pMobIndex->mobthac0, pMobIndex->ac, pMobIndex->hitplus );
      fprintf( fpout, "%dd%d+%d\n", pMobIndex->damnodice, pMobIndex->damsizedice, pMobIndex->damplus );
      fprintf( fpout, "%s~\n", flag_string( pMobIndex->speaks, lang_names ) );
      fprintf( fpout, "%s~\n", flag_string( pMobIndex->speaking, lang_names ) );
      fprintf( fpout, "%s~\n", npc_position[pMobIndex->position] );
      fprintf( fpout, "%s~\n", npc_position[pMobIndex->defposition] );
      fprintf( fpout, "%s~\n", npc_sex[pMobIndex->sex] );
      fprintf( fpout, "%s~\n", npc_race[pMobIndex->race] );
      fprintf( fpout, "%s~\n", npc_class[pMobIndex->Class] );
      fprintf( fpout, "%s~\n", flag_string( pMobIndex->xflags, part_flags ) );
      fprintf( fpout, "%s~\n", ext_flag_string( &pMobIndex->resistant, ris_flags ) );
      fprintf( fpout, "%s~\n", ext_flag_string( &pMobIndex->immune, ris_flags ) );
      fprintf( fpout, "%s~\n", ext_flag_string( &pMobIndex->susceptible, ris_flags ) );
      fprintf( fpout, "%s~\n", ext_flag_string( &pMobIndex->absorb, ris_flags ) );
      fprintf( fpout, "%s~\n", ext_flag_string( &pMobIndex->attacks, attack_flags ) );
      fprintf( fpout, "%s~\n", ext_flag_string( &pMobIndex->defenses, defense_flags ) );

      if( pMobIndex->mudprogs )
      {
         int count = 0;
         for( mprog = pMobIndex->mudprogs; mprog; mprog = mprog->next )
         {
            if( ( mprog->arglist && mprog->arglist[0] != '\0' ) )
            {
               if( mprog->type == IN_FILE_PROG )
               {
                  fprintf( fpout, "> %s %s~\n", mprog_type_to_name( mprog->type ), mprog->arglist );
                  count++;
               }
               // Don't let it save progs which came from files. That would be silly.
               else if( mprog->comlist && mprog->comlist[0] != '\0' && !mprog->fileprog )
               {
                  fprintf( fpout, "> %s %s~\n%s~\n", mprog_type_to_name( mprog->type ),
                     mprog->arglist, strip_cr( mprog->comlist ) );
                  count++;
               }
            }
         }
         if( count > 0 )
            fprintf( fpout, "%s", "|\n" );
      }
   }
   fprintf( fpout, "%s", "#0\n\n\n" );

   /*
    * save objects 
    */
   fprintf( fpout, "%s", "#OBJECTS\n" );
   for( vnum = tarea->low_vnum; vnum <= tarea->hi_vnum; vnum++ )
   {
      if( !( pObjIndex = get_obj_index( vnum ) ) )
         continue;
      if( install )
         REMOVE_OBJ_FLAG( pObjIndex, ITEM_PROTOTYPE );
      fprintf( fpout, "#%d\n", vnum );
      fprintf( fpout, "%s~\n", pObjIndex->name );
      fprintf( fpout, "%s~\n", pObjIndex->short_descr );
      if( !pObjIndex->objdesc || pObjIndex->objdesc[0] == '\0' )
         fprintf( fpout, "%s", "~\n" );
      else
         fprintf( fpout, "%s~\n", pObjIndex->objdesc );
      if( !pObjIndex->action_desc || pObjIndex->action_desc[0] == '\0' )
         fprintf( fpout, "%s", "~\n" );
      else
         fprintf( fpout, "%s~\n", pObjIndex->action_desc );
      fprintf( fpout, "%s~\n", o_types[pObjIndex->item_type] );
      fprintf( fpout, "%s~\n", ext_flag_string( &pObjIndex->extra_flags, o_flags ) );
      fprintf( fpout, "%s~\n", flag_string( pObjIndex->wear_flags, w_flags ) );
      fprintf( fpout, "%s~\n", flag_string( pObjIndex->magic_flags, mag_flags ) );

      val0 = pObjIndex->value[0];
      val1 = pObjIndex->value[1];
      val2 = pObjIndex->value[2];
      val3 = pObjIndex->value[3];
      val4 = pObjIndex->value[4];
      val5 = pObjIndex->value[5];
      val6 = pObjIndex->value[6];
      val7 = pObjIndex->value[7];
      val8 = pObjIndex->value[8];
      val9 = pObjIndex->value[9];
      val10 = pObjIndex->value[10];
      switch ( pObjIndex->item_type )
      {
         case ITEM_PILL:
         case ITEM_POTION:
         case ITEM_SCROLL:
            if( IS_VALID_SN( val1 ) )
               val1 = HAS_SPELL_INDEX;
            if( IS_VALID_SN( val2 ) )
               val2 = HAS_SPELL_INDEX;
            if( IS_VALID_SN( val3 ) )
               val3 = HAS_SPELL_INDEX;
            break;
         case ITEM_STAFF:
         case ITEM_WAND:
            if( IS_VALID_SN( val3 ) )
               val3 = HAS_SPELL_INDEX;
            break;
         case ITEM_SALVE:
            if( IS_VALID_SN( val4 ) )
               val4 = HAS_SPELL_INDEX;
            if( IS_VALID_SN( val5 ) )
               val5 = HAS_SPELL_INDEX;
            break;
      }
      fprintf( fpout, "%d %d %d %d %d %d %d %d %d %d %d\n",
               val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, val10 );

      fprintf( fpout, "%d %d %d %d %d %s %s %s\n", pObjIndex->weight, pObjIndex->cost, pObjIndex->rent,
               pObjIndex->limit, pObjIndex->layers,
               pObjIndex->socket[0] ? pObjIndex->socket[0] : "None",
               pObjIndex->socket[1] ? pObjIndex->socket[1] : "None", pObjIndex->socket[2] ? pObjIndex->socket[2] : "None" );

      switch ( pObjIndex->item_type )
      {
         case ITEM_PILL:
         case ITEM_POTION:
         case ITEM_SCROLL:
            fprintf( fpout, "'%s' '%s' '%s'\n",
                     IS_VALID_SN( pObjIndex->value[1] ) ? skill_table[pObjIndex->value[1]]->name : "NONE",
                     IS_VALID_SN( pObjIndex->value[2] ) ? skill_table[pObjIndex->value[2]]->name : "NONE",
                     IS_VALID_SN( pObjIndex->value[3] ) ? skill_table[pObjIndex->value[3]]->name : "NONE" );
            break;
         case ITEM_STAFF:
         case ITEM_WAND:
            fprintf( fpout, "'%s'\n", IS_VALID_SN( pObjIndex->value[3] ) ? skill_table[pObjIndex->value[3]]->name : "NONE" );
            break;
         case ITEM_SALVE:
            fprintf( fpout, "'%s' '%s'\n",
                     IS_VALID_SN( pObjIndex->value[4] ) ? skill_table[pObjIndex->value[4]]->name : "NONE",
                     IS_VALID_SN( pObjIndex->value[5] ) ? skill_table[pObjIndex->value[5]]->name : "NONE" );
            break;
      }
      for( ed = pObjIndex->first_extradesc; ed; ed = ed->next )
      {
         if( ed->extradesc && ed->extradesc[0] != '\0' )
            fprintf( fpout, "E\n%s~\n%s~\n", ed->keyword, strip_cr( ed->extradesc ) );
         else
            fprintf( fpout, "E\n%s~\n~\n", ed->keyword );
      }

      for( paf = pObjIndex->first_affect; paf; paf = paf->next )
         if( paf->location == APPLY_AFFECT )
            fprintf( fpout, "A %s '%s'\n", a_types[paf->location], a_flags[paf->modifier] );
         else if( paf->location == APPLY_WEAPONSPELL
                  || paf->location == APPLY_WEARSPELL
                  || paf->location == APPLY_REMOVESPELL
                  || paf->location == APPLY_STRIPSN
                  || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
            fprintf( fpout, "A %s '%s'\n", a_types[paf->location],
                     IS_VALID_SN( paf->modifier ) ? skill_table[paf->modifier]->name : "UNKNOWN" );
         else if( paf->location == APPLY_RESISTANT
                  || paf->location == APPLY_IMMUNE || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
            fprintf( fpout, "A %s %s~\n", a_types[paf->location], ext_flag_string( &paf->rismod, ris_flags ) );
         else
            fprintf( fpout, "A %s %d\n", a_types[paf->location], paf->modifier );

      if( pObjIndex->mudprogs )
      {
         int count = 0;
         for( mprog = pObjIndex->mudprogs; mprog; mprog = mprog->next )
         {
            if( ( mprog->arglist && mprog->arglist[0] != '\0' ) )
            {
               if( mprog->type == IN_FILE_PROG )
               {
                  fprintf( fpout, "> %s %s~\n", mprog_type_to_name( mprog->type ), mprog->arglist );
                  count++;
               }
               // Don't let it save progs which came from files. That would be silly.
               else if( mprog->comlist && mprog->comlist[0] != '\0' && !mprog->fileprog )
               {
                  fprintf( fpout, "> %s %s~\n%s~\n", mprog_type_to_name( mprog->type ),
                     mprog->arglist, strip_cr( mprog->comlist ) );
                  count++;
               }
            }
         }
         if( count > 0 )
            fprintf( fpout, "%s", "|\n" );
      }
   }
   fprintf( fpout, "%s", "#0\n\n\n" );

   /*
    * save rooms 
    */
   fprintf( fpout, "%s", "#ROOMS\n" );
   for( vnum = tarea->low_vnum; vnum <= tarea->hi_vnum; vnum++ )
   {
      if( !( room = get_room_index( vnum ) ) )
         continue;
      if( install )
      {
         CHAR_DATA *victim, *vnext;
         OBJ_DATA *obj, *obj_next;

         /*
          * remove prototype flag from room 
          */
         REMOVE_ROOM_FLAG( room, ROOM_PROTOTYPE );
         /*
          * purge room of (prototyped) mobiles 
          */
         for( victim = room->first_person; victim; victim = vnext )
         {
            vnext = victim->next_in_room;
            if( IS_NPC( victim ) )
               extract_char( victim, TRUE );
         }
         /*
          * purge room of (prototyped) objects 
          */
         for( obj = room->first_content; obj; obj = obj_next )
         {
            obj_next = obj->next_content;
            extract_obj( obj );
         }
      }
      fprintf( fpout, "#%d\n", vnum );
      fprintf( fpout, "%s~\n", strip_cr( room->name ) );
      if( !room->roomdesc || room->roomdesc[0] == '\0' )
         fprintf( fpout, "%s", "~\n" );
      else
         fprintf( fpout, "%s~\n", strip_cr( room->roomdesc ) );
      /*
       * write NiteDesc's -- Dracones 
       */
      if( !room->nitedesc || room->nitedesc[0] == '\0' )
         fprintf( fpout, "%s", "~\n" );
      else
         fprintf( fpout, "%s~\n", strip_cr( room->nitedesc ) );

      /*
       * Retain the ORIGINAL sector type to the area file - Samson 7-19-00 
       */
      if( time_info.season == SEASON_WINTER && room->winter_sector != -1 )
         room->sector_type = room->winter_sector;

      fprintf( fpout, "%s~\n", strip_cr( sect_types[room->sector_type] ) );

      /*
       * And change it back again so that the season is not disturbed in play - Samson 7-19-00 
       */
      if( time_info.season == SEASON_WINTER )
      {
         switch ( room->sector_type )
         {
            case SECT_WATER_NOSWIM:
            case SECT_WATER_SWIM:
               room->winter_sector = room->sector_type;
               room->sector_type = SECT_ICE;
         }
      }

      fprintf( fpout, "%s~\n", ext_flag_string( &room->room_flags, r_flags ) );

      if( ( room->tele_delay > 0 && room->tele_vnum > 0 ) || room->tunnel > 0 )
         fprintf( fpout, "1 %d %d %d\n", room->tele_delay, room->tele_vnum, room->tunnel );
      else
         fprintf( fpout, "%s", "0\n" );

      for( xit = room->first_exit; xit; xit = xit->next )
      {
         if( IS_EXIT_FLAG( xit, EX_PORTAL ) )   /* don't fold portals */
            continue;
         fprintf( fpout, "%s", "D\n" );
         fprintf( fpout, "%s~\n", strip_cr( dir_name[xit->vdir] ) );
         if( xit->exitdesc && xit->exitdesc[0] != '\0' )
            fprintf( fpout, "%s~\n", strip_cr( xit->exitdesc ) );
         else
            fprintf( fpout, "%s", "~\n" );
         if( xit->keyword && xit->keyword[0] != '\0' )
            fprintf( fpout, "%s~\n", strip_cr( xit->keyword ) );
         else
            fprintf( fpout, "%s", "~\n" );
         fprintf( fpout, "%s~\n", ext_flag_string( &xit->exit_info, ex_flags ) );
         if( xit->pull )
            fprintf( fpout, "%d %d %d %d %d %d\n", xit->key, xit->vnum, xit->x, xit->y, xit->pulltype, xit->pull );

         else
            fprintf( fpout, "%d %d %d %d\n", xit->key, xit->vnum, xit->x, xit->y );
      }

      for( pReset = room->first_reset; pReset; pReset = pReset->next )
      {
         switch ( pReset->command ) /* extra arg1 arg2 arg3 */
         {
            default:
            case '*':
               break;
            case 'm':
            case 'M':
            case 'o':
            case 'O':
               fprintf( fpout, "R %c %d %d %d %d %d %d %d\n", UPPER( pReset->command ),
                        pReset->extra, pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5, pReset->arg6 );

               for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
               {
                  switch ( tReset->command )
                  {
                     case 'p':
                     case 'P':
                     case 'e':
                     case 'E':
                        fprintf( fpout, "  R %c %d %d %d %d\n", UPPER( tReset->command ),
                                 tReset->extra, tReset->arg1, tReset->arg2, tReset->arg3 );
                        if( tReset->first_reset )
                        {
                           for( gReset = tReset->first_reset; gReset; gReset = gReset->next_reset )
                           {
                              if( gReset->command != 'p' && gReset->command != 'P' )
                                 continue;
                              fprintf( fpout, "    R %c %d %d %d %d\n", UPPER( gReset->command ),
                                       gReset->extra, gReset->arg1, gReset->arg2, gReset->arg3 );
                           }
                        }
                        break;

                     case 'g':
                     case 'G':
                        fprintf( fpout, "  R %c %d %d %d\n", UPPER( tReset->command ),
                                 tReset->extra, tReset->arg1, tReset->arg2 );
                        if( tReset->first_reset )
                        {
                           for( gReset = tReset->first_reset; gReset; gReset = gReset->next_reset )
                           {
                              if( gReset->command != 'p' && gReset->command != 'P' )
                                 continue;
                              fprintf( fpout, "    R %c %d %d %d %d\n", UPPER( gReset->command ),
                                       gReset->extra, gReset->arg1, gReset->arg2, gReset->arg3 );
                           }
                        }
                        break;

                     case 't':
                     case 'T':
                     case 'h':
                     case 'H':
                        fprintf( fpout, "  R %c %d %d %d %d\n", UPPER( tReset->command ),
                                 tReset->extra, tReset->arg1, tReset->arg2, tReset->arg3 );
                        break;
                  }
               }
               break;

            case 'd':
            case 'D':
            case 't':
            case 'T':
            case 'h':
            case 'H':
               fprintf( fpout, "R %c %d %d %d %d\n", UPPER( pReset->command ),
                        pReset->extra, pReset->arg1, pReset->arg2, pReset->arg3 );
               break;

            case 'r':
            case 'R':
               fprintf( fpout, "R %c %d %d %d\n", UPPER( pReset->command ), pReset->extra, pReset->arg1, pReset->arg2 );
               break;
         }
      }

      for( ed = room->first_extradesc; ed; ed = ed->next )
      {
         if( ed->extradesc && ed->extradesc[0] != '\0' )
            fprintf( fpout, "E\n%s~\n%s~\n", ed->keyword, strip_cr( ed->extradesc ) );
         else
            fprintf( fpout, "E\n%s~\n~\n", ed->keyword );
      }

      if( room->mudprogs )
      {
         int count = 0;
         for( mprog = room->mudprogs; mprog; mprog = mprog->next )
         {
            if( ( mprog->arglist && mprog->arglist[0] != '\0' ) )
            {
               if( mprog->type == IN_FILE_PROG )
               {
                  fprintf( fpout, "> %s %s~\n", mprog_type_to_name( mprog->type ), mprog->arglist );
                  count++;
               }
               // Don't let it save progs which came from files. That would be silly.
               else if( mprog->comlist && mprog->comlist[0] != '\0' && !mprog->fileprog )
               {
                  fprintf( fpout, "> %s %s~\n%s~\n", mprog_type_to_name( mprog->type ),
                     mprog->arglist, strip_cr( mprog->comlist ) );
                  count++;
               }
            }
         }
         if( count > 0 )
            fprintf( fpout, "%s", "|\n" );
      }
      fprintf( fpout, "%s", "S\n" );
   }
   fprintf( fpout, "%s", "#0\n\n\n" );

   /*
    * save shops 
    */
   fprintf( fpout, "%s", "#SHOPS\n" );
   for( vnum = tarea->low_vnum; vnum <= tarea->hi_vnum; vnum++ )
   {
      if( !( pMobIndex = get_mob_index( vnum ) ) )
         continue;
      if( !( pShop = pMobIndex->pShop ) )
         continue;
      fprintf( fpout, " %d   %2d %2d %2d %2d %2d   %3d %3d",
               pShop->keeper, pShop->buy_type[0], pShop->buy_type[1], pShop->buy_type[2],
               pShop->buy_type[3], pShop->buy_type[4], pShop->profit_buy, pShop->profit_sell );
      fprintf( fpout, "        %2d %2d    ; %s\n", pShop->open_hour, pShop->close_hour, pMobIndex->short_descr );
   }
   fprintf( fpout, "%s", "0\n\n\n" );

   /*
    * save repair shops 
    */
   fprintf( fpout, "%s", "#REPAIRS\n" );
   for( vnum = tarea->low_vnum; vnum <= tarea->hi_vnum; vnum++ )
   {
      if( !( pMobIndex = get_mob_index( vnum ) ) )
         continue;
      if( !( pRepair = pMobIndex->rShop ) )
         continue;
      fprintf( fpout, " %d   %2d %2d %2d         %3d %3d",
               pRepair->keeper, pRepair->fix_type[0], pRepair->fix_type[1],
               pRepair->fix_type[2], pRepair->profit_fix, pRepair->shop_type );
      fprintf( fpout, "        %2d %2d    ; %s\n", pRepair->open_hour, pRepair->close_hour, pMobIndex->short_descr );
   }
   fprintf( fpout, "%s", "0\n\n\n" );

   /*
    * save specials 
    */
   fprintf( fpout, "%s", "#SPECIALS\n" );
   for( vnum = tarea->low_vnum; vnum <= tarea->hi_vnum; vnum++ )
   {
      if( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
         if( pMobIndex->spec_fun )
            fprintf( fpout, "M  %d %s\n", pMobIndex->vnum, pMobIndex->spec_funname );
   }
   fprintf( fpout, "%s", "S\n\n\n" );

   /*
    * END 
    */
   fprintf( fpout, "%s", "#$\n" );
   FCLOSE( fpout );
   return;
}

/* Function modified from original form - Samson */
CMDF do_savearea( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *tarea;
   char filename[256];

   set_char_color( AT_IMMORT, ch );

   if( IS_NPC( ch ) || get_trust( ch ) < LEVEL_CREATOR || !ch->pcdata || ( argument[0] == '\0' && !ch->pcdata->area ) )
   {
      send_to_char( "You don't have an assigned area to save.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
      tarea = ch->pcdata->area;
   else
   {
      bool found;

      if( get_trust( ch ) < LEVEL_GOD )
      {
         send_to_char( "You can only save your own area.\n\r", ch );
         return;
      }
      for( found = FALSE, tarea = first_area; tarea; tarea = tarea->next )
         if( !str_cmp( tarea->filename, argument ) )
         {
            found = TRUE;
            break;
         }
      if( !found )
      {
         send_to_char( "Area not found.\n\r", ch );
         return;
      }
   }

   if( !tarea )
   {
      send_to_char( "No area to save.\n\r", ch );
      return;
   }

   if( !IS_AREA_FLAG( tarea, AFLAG_PROTOTYPE ) )
   {
      ch_printf( ch, "Cannot savearea %s, use foldarea instead.\n\r", tarea->filename );
      return;
   }
   snprintf( filename, 256, "%s%s", BUILD_DIR, tarea->filename );
   send_to_char( "Saving area...\n\r", ch );
   fold_area( tarea, filename, FALSE );
   set_char_color( AT_IMMORT, ch );
   send_to_char( "Done.\n\r", ch );
}

/* Function modified from original form - Samson */
CMDF do_foldarea( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *tarea;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Fold what?\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      for( tarea = first_area; tarea; tarea = tarea->next )
      {
         if( !IS_SET( tarea->flags, AFLAG_PROTOTYPE ) )
            fold_area( tarea, tarea->filename, FALSE );
      }
      set_char_color( AT_IMMORT, ch );
      send_to_char( "Folding completed.\n\r", ch );
      return;
   }

   for( tarea = first_area; tarea; tarea = tarea->next )
   {
      if( !str_cmp( tarea->filename, argument ) )
      {
         if( IS_SET( tarea->flags, AFLAG_PROTOTYPE ) )
         {
            ch_printf( ch, "Cannot fold %s, use savearea instead.\n\r", tarea->filename );
            return;
         }
         send_to_char( "Folding area...\n\r", ch );
         fold_area( tarea, tarea->filename, FALSE );
         set_char_color( AT_IMMORT, ch );
         send_to_char( "Done.\n\r", ch );
         return;
      }
   }
   send_to_char( "No such area exists.\n\r", ch );
   return;
}

void write_area_list( void )
{
   AREA_DATA *tarea;
   FILE *fpout;

   fpout = fopen( AREA_LIST, "w" );
   if( !fpout )
   {
      bug( "%s", "FATAL: cannot open area.lst for writing!" );
      return;
   }
   fprintf( fpout, "%s", "help.are\n" );
   for( tarea = first_area; tarea; tarea = tarea->next )
      fprintf( fpout, "%s\n", tarea->filename );
   fprintf( fpout, "%s", "$\n" );
   FCLOSE( fpout );
}

/*
 * A complicated to use command as it currently exists.		-Thoric
 * Once area->author and area->name are cleaned up... it will be easier
 */
CMDF do_installarea( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *tarea;
   char arg1[MIL], arg2[MIL], oldfilename[256];
   char buf[256];
   int num;
   DESCRIPTOR_DATA *d;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( ( !arg1[0] || arg1[0] == '\0' ) || ( !arg2[0] || arg2[0] == '\0' ) || ( !argument || argument[0] == '\0' ) )
   {
      send_to_char( "Syntax: installarea <current filename> <new filename> <Area name>\n\r", ch );
      return;
   }

   for( tarea = first_area; tarea; tarea = tarea->next )
   {
      if( !IS_AREA_FLAG( tarea, AFLAG_PROTOTYPE ) )
         continue;

      if( !str_cmp( tarea->filename, arg1 ) )
      {
         if( argument && argument[0] != '\0' )
         {
            DISPOSE( tarea->name );
            tarea->name = str_dup( argument );
         }

         if( exists_file( arg2 ) )
         {
            ch_printf( ch, "An area with filename %s already exists - choose another.\n\r", arg2 );
            return;
         }

         mudstrlcpy( oldfilename, tarea->filename, 256 );
         DISPOSE( tarea->filename );
         tarea->filename = str_dup( arg2 );

         /*
          * Fold area with install flag -- auto-removes prototype flags 
          */
         REMOVE_AREA_FLAG( tarea, AFLAG_PROTOTYPE );
         ch_printf( ch, "Saving and installing %s...\n\r", tarea->filename );
         fold_area( tarea, tarea->filename, TRUE );

         /*
          * Fix up author if online 
          */
         for( d = first_descriptor; d; d = d->next )
            if( d->character && d->character->pcdata && d->character->pcdata->area == tarea )
            {
               /*
                * remove area from author 
                */
               d->character->pcdata->area = NULL;
               /*
                * clear out author vnums  
                */
               d->character->pcdata->low_vnum = 0;
               d->character->pcdata->hi_vnum = 0;
               save_char_obj( d->character );
            }

         top_area++;
         send_to_char( "Writing area.lst...\n\r", ch );
         write_area_list(  );
         send_to_char( "Resetting new area.\n\r", ch );
         num = tarea->nplayer;
         tarea->nplayer = 0;
         reset_area( tarea );
         tarea->nplayer = num;
         send_to_char( "Removing author's building file.\n\r", ch );
         snprintf( buf, 256, "%s%s", BUILD_DIR, oldfilename );
         unlink( buf );
         snprintf( buf, 256, "%s%s.bak", BUILD_DIR, oldfilename );
         unlink( buf );
         send_to_char( "Done.\n\r", ch );
         return;
      }
   }
   ch_printf( ch, "No area with filename %s exists in the building directory.\n\r", arg1 );
   return;
}

CMDF do_astat( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *tarea;
   bool proto, found;
   found = FALSE;
   proto = FALSE;

   set_char_color( AT_PLAIN, ch );

   for( tarea = first_area; tarea; tarea = tarea->next )
      if( !str_cmp( tarea->filename, argument ) )
      {
         found = TRUE;
         break;
      }

   if( !found )
   {
      if( argument && argument[0] != '\0' )
      {
         send_to_char( "Area not found.  Check 'zones' or 'vnums'.\n\r", ch );
         return;
      }
      else
         tarea = ch->in_room->area;
   }

   ch_printf( ch, "\n\r&wName:     &W%s\n\r&wFilename: &W%-20s  &wPrototype: &W%s\n\r&wAuthor:   &W%s\n\r",
              tarea->name, tarea->filename, IS_SET( tarea->flags, AFLAG_PROTOTYPE ) ? "yes" : "no", tarea->author );
   ch_printf( ch, "&wLast reset at: &W%s\n\r", c_time( tarea->last_resettime, -1 ) );
   ch_printf( ch, "&wVersion: &W%-3d &wAge: &W%-3d  &wCurrent number of players: &W%-3d\n\r",
              tarea->version, tarea->age, tarea->nplayer );
   ch_printf( ch, "&wlow_vnum: &W%5d    &whi_vnum: &W%5d\n\r", tarea->low_vnum, tarea->hi_vnum );
   ch_printf( ch, "&wSoft range: &W%d - %d    &wHard range: &W%d - %d\n\r",
              tarea->low_soft_range, tarea->hi_soft_range, tarea->low_hard_range, tarea->hi_hard_range );
   ch_printf( ch, "&wArea flags: &W%s\n\r", flag_string( tarea->flags, area_flags ) );

   send_to_char( "&wTreasure Settings:\n\r", ch );
   ch_printf( ch, "&wNothing: &W%-3hu &wGold:   &W%-3hu &wItem: &W%-3hu &wGem:   &W%-3hu\n\r",
              tarea->tg_nothing, tarea->tg_gold, tarea->tg_item, tarea->tg_gem );
   ch_printf( ch, "&wScroll:  &W%-3hu &wPotion: &W%-3hu &wWand: &W%-3hu &wArmor: &W%-3hu\n\r",
              tarea->tg_scroll, tarea->tg_potion, tarea->tg_wand, tarea->tg_armor );

   ch_printf( ch, "&wContinent or Plane: &W%s\n\r", continents[tarea->continent] );

   ch_printf( ch, "&wCoordinates: &W%d %d\n\r", tarea->x, tarea->y );

   ch_printf( ch, "&wResetmsg: &W%s\n\r", tarea->resetmsg ? tarea->resetmsg : "(default)" ); /* Rennard */
   ch_printf( ch, "&wReset frequency: &W%d &wminutes.\n\r", tarea->reset_frequency ? tarea->reset_frequency : 15 );
}

bool check_area_conflict( AREA_DATA *area, int low_range, int hi_range )
{
   if( low_range < area->low_vnum && area->low_vnum < hi_range )
      return TRUE;

   if( low_range < area->hi_vnum && area->hi_vnum < hi_range )
      return TRUE;

   if( ( low_range >= area->low_vnum ) && ( low_range <= area->hi_vnum ) )
      return TRUE;

   if( ( hi_range <= area->hi_vnum ) && ( hi_range >= area->low_vnum ) )
      return TRUE;

   return FALSE;
}

/* check other areas for a conflict while ignoring the current area */
bool check_for_area_conflicts( AREA_DATA *carea, int lo, int hi )
{
   AREA_DATA *area;

   for( area = first_area; area; area = area->next )
      if( area != carea && check_area_conflict( area, lo, hi ) )
         return TRUE;

   return FALSE;
}

CMDF do_aset( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *tarea;
   char arg1[MIL], arg2[MIL], arg3[MIL];
   bool proto, found;
   int vnum, value;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   vnum = atoi( argument );
   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Usage: aset <area filename> <field> <value>\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "  low_vnum hi_vnum coords\n\r", ch );
      send_to_char( "  name filename low_soft hi_soft low_hard hi_hard\n\r", ch );
      send_to_char( "  author resetmsg resetfreq flags\n\r", ch );
      send_to_char( "  nothing gold item gem\n\r", ch );
      send_to_char( "  scroll potion wand armor\n\r", ch );
      return;
   }

   found = FALSE;
   proto = FALSE;
   for( tarea = first_area; tarea; tarea = tarea->next )
      if( !str_cmp( tarea->filename, arg1 ) )
      {
         found = TRUE;
         break;
      }

   if( !found )
   {
      send_to_char( "Area not found.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      AREA_DATA *uarea;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't set an area's name to nothing.\r\n", ch );
         return;
      }

      for( uarea = first_area; uarea; uarea = uarea->next )
      {
         if( !str_cmp( uarea->name, argument ) )
         {
            send_to_char( "There is already an area with that name.\r\n", ch );
            return;
         }
      }

      DISPOSE( tarea->name );
      tarea->name = str_dup( argument );
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( proto )
      {
         send_to_char( "You should only change the filename of installed areas.\r\n", ch );
         return;
      }

      if( !is_valid_filename( ch, "", argument ) )
         return;

      mudstrlcpy( filename, tarea->filename, 256 );
      DISPOSE( tarea->filename );
      tarea->filename = str_dup( argument );
      rename( filename, tarea->filename );
      write_area_list(  );
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "continent" ) )
   {
      /*
       * Area continent editing - Samson 8-8-98 
       */
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Set the area's continent.\n\r", ch );
         send_to_char( "Usage: aset continent <name>\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg2 );
      value = get_continent( arg2 );
      if( value < 0 || value > ACON_MAX )
      {
         tarea->continent = 0;
         send_to_char( "Invalid area continent, set to 'alsherok' by default.\n\r", ch );
      }
      else
      {
         tarea->continent = value;
         ch_printf( ch, "Area continent set to %s.\n\r", arg2 );
      }
      return;
   }

   if( !str_cmp( arg2, "low_vnum" ) )
   {
      if( check_for_area_conflicts( tarea, tarea->low_vnum, vnum ) )
      {
         ch_printf( ch, "Setting %d for low_vnum would conflict with another area.\r\n", vnum );
         return;
      }
      if( tarea->hi_vnum < vnum )
      {
         ch_printf( ch, "Vnum %d exceeds the hi_vnum of %d for this area.\r\n", vnum, tarea->hi_vnum );
         return;
      }
      tarea->low_vnum = vnum;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "hi_vnum" ) )
   {
      if( check_for_area_conflicts( tarea, tarea->hi_vnum, vnum ) )
      {
         ch_printf( ch, "Setting %d for hi_vnum would conflict with another area.\r\n", vnum );
         return;
      }
      if( tarea->low_vnum > vnum )
      {
         ch_printf( ch, "Cannot set %d for hi_vnum smaller than the low_vnum of %d.\r\n", vnum, tarea->low_vnum );
         return;
      }
      tarea->hi_vnum = vnum;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "coords" ) )
   {
      int x, y;

      argument = one_argument( argument, arg3 );

      if( arg3[0] == '\0' || argument[0] == '\0' )
      {
         send_to_char( "You must specify X and Y coordinates for this.\n\r", ch );
         return;
      }

      x = atoi( arg3 );
      y = atoi( argument );

      if( x < 0 || x >= MAX_X )
      {
         ch_printf( ch, "Valid X coordinates are from 0 to %d.\n\r", MAX_X );
         return;
      }

      if( y < 0 || y >= MAX_Y )
      {
         ch_printf( ch, "Valid Y coordinates are from 0 to %d.\n\r", MAX_Y );
         return;
      }

      tarea->x = x;
      tarea->y = y;

      send_to_char( "Area coordinates set.\n\r", ch );

      return;
   }

   if( !str_cmp( arg2, "low_soft" ) )
   {
      if( vnum < 0 || vnum > MAX_LEVEL )
      {
         send_to_char( "That is not an acceptable value.\n\r", ch );
         return;
      }

      tarea->low_soft_range = vnum;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "hi_soft" ) )
   {
      if( vnum < 0 || vnum > MAX_LEVEL )
      {
         send_to_char( "That is not an acceptable value.\n\r", ch );
         return;
      }

      tarea->hi_soft_range = vnum;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "low_hard" ) )
   {
      if( vnum < 0 || vnum > MAX_LEVEL )
      {
         send_to_char( "That is not an acceptable value.\n\r", ch );
         return;
      }

      tarea->low_hard_range = vnum;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "hi_hard" ) )
   {
      if( vnum < 0 || vnum > MAX_LEVEL )
      {
         send_to_char( "That is not an acceptable value.\n\r", ch );
         return;
      }

      tarea->hi_hard_range = vnum;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "author" ) )
   {
      STRFREE( tarea->author );
      tarea->author = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "resetmsg" ) )
   {
      DISPOSE( tarea->resetmsg );
      if( str_cmp( argument, "clear" ) )
         tarea->resetmsg = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }  /* Rennard */

   if( !str_cmp( arg2, "resetfreq" ) )
   {
      tarea->reset_frequency = vnum;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: aset <filename> flags <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_areaflag( arg3 );
         if( value < 0 || value > 31 )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
         {
            if( IS_SET( tarea->flags, 1 << value ) )
               REMOVE_AREA_FLAG( tarea, 1 << value );
            else
               SET_AREA_FLAG( tarea, 1 << value );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "nothing" ) )
   {
      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "Usage: aset <filename> nothing <percentage>\n\r", ch );
         return;
      }
      value = atoi( argument );
      tarea->tg_nothing = value;
      ch_printf( ch, "Area chance to generate nothing set to %hu%%\n\r", value );
      return;
   }

   if( !str_cmp( arg2, "gold" ) )
   {
      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "Usage: aset <filename> gold <percentage>\n\r", ch );
         return;
      }
      value = atoi( argument );
      tarea->tg_gold = value;
      ch_printf( ch, "Area chance to generate gold set to %hu%%\n\r", value );
      return;
   }

   if( !str_cmp( arg2, "item" ) )
   {
      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "Usage: aset <filename> item <percentage>\n\r", ch );
         return;
      }
      value = atoi( argument );
      tarea->tg_item = value;
      ch_printf( ch, "Area chance to generate item set to %hu%%\n\r", value );
      return;
   }

   if( !str_cmp( arg2, "gem" ) )
   {
      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "Usage: aset <filename> gem <percentage>\n\r", ch );
         return;
      }
      value = atoi( argument );
      tarea->tg_gem = value;
      ch_printf( ch, "Area chance to generate gem set to %hu%%\n\r", value );
      return;
   }

   if( !str_cmp( arg2, "scroll" ) )
   {
      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "Usage: aset <filename> scroll <percentage>\n\r", ch );
         return;
      }
      value = atoi( argument );
      tarea->tg_scroll = value;
      ch_printf( ch, "Area chance to generate scroll set to %hu%%\n\r", value );
      return;
   }

   if( !str_cmp( arg2, "potion" ) )
   {
      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "Usage: aset <filename> potion <percentage>\n\r", ch );
         return;
      }
      value = atoi( argument );
      tarea->tg_potion = value;
      ch_printf( ch, "Area chance to generate potion set to %hu%%\n\r", value );
      return;
   }

   if( !str_cmp( arg2, "wand" ) )
   {
      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "Usage: aset <filename> wand <percentage>\n\r", ch );
         return;
      }
      value = atoi( argument );
      tarea->tg_wand = value;
      ch_printf( ch, "Area chance to generate wand set to %hu%%\n\r", value );
      return;
   }

   if( !str_cmp( arg2, "armor" ) )
   {
      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "Usage: aset <filename> armor <percentage>\n\r", ch );
         return;
      }
      value = atoi( argument );
      tarea->tg_armor = value;
      ch_printf( ch, "Area chance to generate armor set to %hu%%\n\r", value );
      return;
   }

   do_aset( ch, "" );
   return;
}

CMDF do_rlist( CHAR_DATA * ch, char *argument )
{
   ROOM_INDEX_DATA *room;
   char arg1[MIL];
   int lrange, trange, vnum;

   set_pager_color( AT_PLAIN, ch );

   argument = one_argument( argument, arg1 );

   lrange = ( is_number( arg1 ) ? atoi( arg1 ) : 1 );
   trange = ( is_number( argument ) ? atoi( argument ) : 2 );

   for( vnum = lrange; vnum <= trange; vnum++ )
   {
      if( ( room = get_room_index( vnum ) ) == NULL )
         continue;
      pager_printf( ch, "%5d) %s\n\r", vnum, room->name );
   }
   return;
}

CMDF do_olist( CHAR_DATA * ch, char *argument )
{
   OBJ_INDEX_DATA *obj;
   char arg1[MIL];
   int lrange, trange, vnum;

   set_pager_color( AT_PLAIN, ch );

   argument = one_argument( argument, arg1 );

   lrange = ( is_number( arg1 ) ? atoi( arg1 ) : 1 );
   trange = ( is_number( argument ) ? atoi( argument ) : 2 );

   for( vnum = lrange; vnum <= trange; vnum++ )
   {
      if( ( obj = get_obj_index( vnum ) ) == NULL )
         continue;
      pager_printf( ch, "%5d) %-20s (%s)\n\r", vnum, obj->name, obj->short_descr );
   }
   return;
}

CMDF do_mlist( CHAR_DATA * ch, char *argument )
{
   MOB_INDEX_DATA *mob;
   char arg1[MIL];
   int lrange, trange, vnum;

   set_pager_color( AT_PLAIN, ch );

   argument = one_argument( argument, arg1 );

   lrange = ( is_number( arg1 ) ? atoi( arg1 ) : 1 );
   trange = ( is_number( argument ) ? atoi( argument ) : 2 );

   for( vnum = lrange; vnum <= trange; vnum++ )
   {
      if( ( mob = get_mob_index( vnum ) ) == NULL )
         continue;
      pager_printf( ch, "%5d) %-20s '%s'\n\r", vnum, mob->player_name, mob->short_descr );
   }
   return;
}

/* Consolidated list command - Samson 3-21-98 */
/* Command online renamed to 'show' via cedit - Samson 9-8-98 */
CMDF do_vlist( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], arg2[MIL];
   AREA_DATA *tarea = NULL;
   bool found = FALSE;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Syntax:\n\r", ch );
      if( ch->level >= LEVEL_DEMI )
      {
         send_to_char( "show mob <lowvnum> <hivnum>\n\r", ch );
         send_to_char( "OR: show mob <area filename>\n\r", ch );
      }
      if( ch->level >= LEVEL_SAVIOR )
      {
         send_to_char( "\n\rshow obj <lowvnum> <hivnum>\n\r", ch );
         send_to_char( "OR: show obj <area filename>\n\r", ch );
         send_to_char( "\n\rshow room <lowvnum> <hivnum>\n\r", ch );
         send_to_char( "OR: show room <area filename>\n\r", ch );
      }
      return;
   }

   if( !is_number( arg2 ) )
   {
      for( tarea = first_area; tarea; tarea = tarea->next )
         if( !str_cmp( tarea->filename, arg2 ) )
         {
            found = TRUE;
            break;
         }

      if( !found )
      {
         if( arg2 && arg2[0] != '\0' )
         {
            send_to_char( "Area not found.\n\r", ch );
            return;
         }
         else
            tarea = ch->in_room->area;
      }
   }

   if( ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "m" ) ) && ch->level >= LEVEL_DEMI )
   {
      if( arg2 && arg2[0] != '\0' && is_number( arg2 ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            do_vlist( ch, "" );
            return;
         }
         funcf( ch, do_mlist, "%s %s", arg2, argument );
         return;
      }
      funcf( ch, do_mlist, "%d %d", tarea->low_vnum, tarea->hi_vnum );
      return;
   }

   if( ( !str_cmp( arg, "obj" ) || !str_cmp( arg, "o" ) ) && ch->level >= LEVEL_SAVIOR )
   {
      if( arg2 && arg2[0] != '\0' && is_number( arg2 ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            do_vlist( ch, "" );
            return;
         }
         funcf( ch, do_olist, "%s %s", arg2, argument );
         return;
      }
      funcf( ch, do_olist, "%d %d", tarea->low_vnum, tarea->hi_vnum );
      return;
   }

   if( !str_cmp( arg, "room" ) || !str_cmp( arg, "r" ) )
   {
      if( arg2 && arg2[0] != '\0' && is_number( arg2 ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            do_vlist( ch, "" );
            return;
         }
         funcf( ch, do_rlist, "%s %s", arg2, argument );
         return;
      }
      funcf( ch, do_rlist, "%d %d", tarea->low_vnum, tarea->hi_vnum );
      return;
   }
   /*
    * echo syntax 
    */
   do_vlist( ch, "" );
}

void mpedit( CHAR_DATA * ch, MPROG_DATA * mprg, int mptype, char *argument )
{
   if( mptype != -1 )
   {
      mprg->type = mptype;
      STRFREE( mprg->arglist );
      mprg->arglist = STRALLOC( argument );
   }
   ch->substate = SUB_MPROG_EDIT;
   ch->pcdata->dest_buf = mprg;
   if( !mprg->comlist )
      mprg->comlist = STRALLOC( "" );
   start_editing( ch, mprg->comlist );
   editor_desc_printf( ch, "Program '%s %s'.", mprog_type_to_name( mprg->type ), mprg->arglist );
   return;
}

/*
 * Mobprogram editing - cumbersome				-Thoric
 */
CMDF do_mpedit( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL], arg3[MIL], arg4[MIL];
   CHAR_DATA *victim;
   MPROG_DATA *mprog, *mprg, *mprg_next = NULL;
   int value, mptype = -1, cnt;

   set_char_color( AT_PLAIN, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mob's can't mpedit\n\r", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_RESTRICTED:
         send_to_char( "You can't use this command from within another command.\r\n", ch );
         return;
      case SUB_MPROG_EDIT:
         if( !ch->pcdata->dest_buf )
         {
            send_to_char( "Fatal error: report to Samson.\n\r", ch );
            bug( "%s", "do_mpedit: sub_mprog_edit: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            return;
         }
         mprog = ( MPROG_DATA * ) ch->pcdata->dest_buf;
         STRFREE( mprog->comlist );
         mprog->comlist = copy_buffer( ch );
         stop_editing( ch );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting mobprog.\n\r", ch );
         return;
   }
   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   value = atoi( arg3 );
   if( !arg1 || arg1[0] == '\0' || !arg2 || arg2[0] == '\0' )
   {
      send_to_char( "Syntax: mpedit <victim> <command> [number] <program> <value>\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Command being one of:\n\r", ch );
      send_to_char( "  add delete insert edit list\n\r", ch );
      send_to_char( "Program being one of:\n\r", ch );
      send_to_char( "  act speech rand fight hitprcnt greet allgreet\n\r", ch );
      send_to_char( "  entry give bribe death time hour script keyword\n\r", ch );
      return;
   }

   if( get_trust( ch ) < LEVEL_GOD )
   {
      if( !( victim = get_char_room( ch, arg1 ) ) )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }
   }
   else
   {
      if( !( victim = get_char_world( ch, arg1 ) ) )
      {
         send_to_char( "No one like that in all the realms.\n\r", ch );
         return;
      }
   }

   if( get_trust( ch ) < victim->level || !IS_NPC( victim ) )
   {
      send_to_char( "You can't do that!\n\r", ch );
      return;
   }

   if( !can_mmodify( ch, victim ) )
      return;

   if( !IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
   {
      send_to_char( "A mobile must have a prototype flag to be mpset.\n\r", ch );
      return;
   }

   mprog = victim->pIndexData->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !str_cmp( arg2, "list" ) )
   {
      cnt = 0;
      if( !mprog )
      {
         send_to_char( "That mobile has no mob programs.\n\r", ch );
         return;
      }

      if( value < 1 )
      {
         if( str_cmp( "full", arg3 ) )
         {
            for( mprg = mprog; mprg; mprg = mprg->next )
               ch_printf( ch, "%d>%s %s\n\r", ++cnt, mprog_type_to_name( mprg->type ), mprg->arglist );
            return;
         }
         else
         {
            for( mprg = mprog; mprg; mprg = mprg->next )
               ch_printf( ch, "%d>%s %s\n\r%s\n\r", ++cnt, mprog_type_to_name( mprg->type ), mprg->arglist, mprg->comlist );
            return;
         }
      }
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            ch_printf( ch, "%d>%s %s\n\r%s\n\r", cnt, mprog_type_to_name( mprg->type ), mprg->arglist, mprg->comlist );
            break;
         }
      }
      if( !mprg )
         send_to_char( "Program not found.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "edit" ) )
   {
      if( !mprog )
      {
         send_to_char( "That mobile has no mob programs.\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      if( arg4 && arg4[0] != '\0' )
      {
         mptype = get_mpflag( arg4 );
         if( mptype == -1 )
         {
            send_to_char( "Unknown program type.\n\r", ch );
            return;
         }
      }
      else
         mptype = -1;
      if( value < 1 )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      cnt = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mpedit( ch, mprg, mptype, argument );
            xCLEAR_BITS( victim->pIndexData->progtypes );
            for( mprg = mprog; mprg; mprg = mprg->next )
               xSET_BIT( victim->pIndexData->progtypes, mprg->type );
            return;
         }
      }
      send_to_char( "Program not found.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      int num;
      bool found;

      if( !mprog )
      {
         send_to_char( "That mobile has no mob programs.\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      if( value < 1 )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      cnt = 0;
      found = FALSE;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mptype = mprg->type;
            found = TRUE;
            break;
         }
      }
      if( !found )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      cnt = num = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
         if( mprg->type == mptype )
            num++;
      if( value == 1 )
      {
         mprg_next = victim->pIndexData->mudprogs;
         victim->pIndexData->mudprogs = mprg_next->next;
      }
      else
      {
         for( mprg = mprog; mprg; mprg = mprg_next )
         {
            mprg_next = mprg->next;
            if( ++cnt == ( value - 1 ) )
            {
               mprg->next = mprg_next->next;
               break;
            }
         }
      }
      if( mprg_next )
      {
         STRFREE( mprg_next->arglist );
         STRFREE( mprg_next->comlist );
         DISPOSE( mprg_next );
         if( num <= 1 )
            xREMOVE_BIT( victim->pIndexData->progtypes, mptype );
         send_to_char( "Program removed.\n\r", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "insert" ) )
   {
      if( !mprog )
      {
         send_to_char( "That mobile has no mob programs.\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      mptype = get_mpflag( arg4 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\n\r", ch );
         return;
      }
      if( value < 1 )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      if( value == 1 )
      {
         CREATE( mprg, MPROG_DATA, 1 );
         xSET_BIT( victim->pIndexData->progtypes, mptype );
         mpedit( ch, mprg, mptype, argument );
         mprg->next = mprog;
         victim->pIndexData->mudprogs = mprg;
         return;
      }
      cnt = 1;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value && mprg->next )
         {
            CREATE( mprg_next, MPROG_DATA, 1 );
            xSET_BIT( victim->pIndexData->progtypes, mptype );
            mpedit( ch, mprg_next, mptype, argument );
            mprg_next->next = mprg->next;
            mprg->next = mprg_next;
            return;
         }
      }
      send_to_char( "Program not found.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "add" ) )
   {
      mptype = get_mpflag( arg3 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\n\r", ch );
         return;
      }
      if( mprog != NULL )
         for( ; mprog->next; mprog = mprog->next );
      CREATE( mprg, MPROG_DATA, 1 );
      if( mprog )
         mprog->next = mprg;
      else
         victim->pIndexData->mudprogs = mprg;
      xSET_BIT( victim->pIndexData->progtypes, mptype );
      mpedit( ch, mprg, mptype, argument );
      mprg->next = NULL;
      return;
   }

   do_mpedit( ch, "" );
}

CMDF do_opedit( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL], arg3[MIL], arg4[MIL];
   OBJ_DATA *obj;
   MPROG_DATA *mprog, *mprg, *mprg_next = NULL;
   int value, mptype = -1, cnt;

   set_char_color( AT_PLAIN, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mob's can't opedit\n\r", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_RESTRICTED:
         send_to_char( "You can't use this command from within another command.\r\n", ch );
         return;
      case SUB_MPROG_EDIT:
         if( !ch->pcdata->dest_buf )
         {
            send_to_char( "Fatal error: report to Samson.\n\r", ch );
            bug( "%s", "do_opedit: sub_oprog_edit: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            return;
         }
         mprog = ( MPROG_DATA * ) ch->pcdata->dest_buf;
         STRFREE( mprog->comlist );
         mprog->comlist = copy_buffer( ch );
         stop_editing( ch );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting objprog.\n\r", ch );
         return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   value = atoi( arg3 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Syntax: opedit <object> <command> [number] <program> <value>\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Command being one of:\n\r", ch );
      send_to_char( "  add delete insert edit list\n\r", ch );
      send_to_char( "Program being one of:\n\r", ch );
      send_to_char( "  act speech rand wear remove sac zap get\n\r", ch );
      send_to_char( "  drop damage repair greet exa use keyword\n\r", ch );
      send_to_char( "  pull push (for levers, pullchains, buttons)\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Object should be in your inventory to edit.\n\r", ch );
      return;
   }

   if( get_trust( ch ) < LEVEL_GOD )
   {
      if( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
      {
         send_to_char( "You aren't carrying that.\n\r", ch );
         return;
      }
   }
   else
   {
      if( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
      {
         send_to_char( "Nothing like that in all the realms.\n\r", ch );
         return;
      }
   }

   if( !can_omodify( ch, obj ) )
      return;

   if( !IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
   {
      send_to_char( "An object must have a prototype flag to be opset.\n\r", ch );
      return;
   }

   mprog = obj->pIndexData->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !str_cmp( arg2, "list" ) )
   {
      cnt = 0;
      if( !mprog )
      {
         send_to_char( "That object has no obj programs.\n\r", ch );
         return;
      }
      for( mprg = mprog; mprg; mprg = mprg->next )
         ch_printf( ch, "%d>%s %s\n\r%s\n\r", ++cnt, mprog_type_to_name( mprg->type ), mprg->arglist, mprg->comlist );
      return;
   }

   if( !str_cmp( arg2, "edit" ) )
   {
      if( !mprog )
      {
         send_to_char( "That object has no obj programs.\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      if( arg4[0] != '\0' )
      {
         mptype = get_mpflag( arg4 );
         if( mptype == -1 )
         {
            send_to_char( "Unknown program type.\n\r", ch );
            return;
         }
      }
      else
         mptype = -1;
      if( value < 1 )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      cnt = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mpedit( ch, mprg, mptype, argument );
            xCLEAR_BITS( obj->pIndexData->progtypes );
            for( mprg = mprog; mprg; mprg = mprg->next )
               xSET_BIT( obj->pIndexData->progtypes, mprg->type );
            return;
         }
      }
      send_to_char( "Program not found.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      int num;
      bool found;

      if( !mprog )
      {
         send_to_char( "That object has no obj programs.\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      if( value < 1 )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      cnt = 0;
      found = FALSE;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mptype = mprg->type;
            found = TRUE;
            break;
         }
      }
      if( !found )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      cnt = num = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
         if( mprg->type == mptype )
            num++;
      if( value == 1 )
      {
         mprg_next = obj->pIndexData->mudprogs;
         obj->pIndexData->mudprogs = mprg_next->next;
      }
      else
      {
         for( mprg = mprog; mprg; mprg = mprg_next )
         {
            mprg_next = mprg->next;
            if( ++cnt == ( value - 1 ) )
            {
               mprg->next = mprg_next->next;
               break;
            }
         }
      }
      if( mprg_next )
      {
         STRFREE( mprg_next->arglist );
         STRFREE( mprg_next->comlist );
         DISPOSE( mprg_next );
         if( num <= 1 )
            xREMOVE_BIT( obj->pIndexData->progtypes, mptype );
         send_to_char( "Program removed.\n\r", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "insert" ) )
   {
      if( !mprog )
      {
         send_to_char( "That object has no obj programs.\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      mptype = get_mpflag( arg4 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\n\r", ch );
         return;
      }
      if( value < 1 )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      if( value == 1 )
      {
         CREATE( mprg, MPROG_DATA, 1 );
         xSET_BIT( obj->pIndexData->progtypes, mptype );
         mpedit( ch, mprg, mptype, argument );
         mprg->next = mprog;
         obj->pIndexData->mudprogs = mprg;
         return;
      }
      cnt = 1;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value && mprg->next )
         {
            CREATE( mprg_next, MPROG_DATA, 1 );
            xSET_BIT( obj->pIndexData->progtypes, mptype );
            mpedit( ch, mprg_next, mptype, argument );
            mprg_next->next = mprg->next;
            mprg->next = mprg_next;
            return;
         }
      }
      send_to_char( "Program not found.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "add" ) )
   {
      mptype = get_mpflag( arg3 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\n\r", ch );
         return;
      }
      if( mprog != NULL )
         for( ; mprog->next; mprog = mprog->next );
      CREATE( mprg, MPROG_DATA, 1 );

      if( mprog )
         mprog->next = mprg;
      else
         obj->pIndexData->mudprogs = mprg;
      xSET_BIT( obj->pIndexData->progtypes, mptype );
      mpedit( ch, mprg, mptype, argument );
      mprg->next = NULL;
      return;
   }

   do_opedit( ch, "" );
}

/*
 * RoomProg Support
 */
void rpedit( CHAR_DATA * ch, MPROG_DATA * mprg, int mptype, char *argument )
{
   if( mptype != -1 )
   {
      mprg->type = mptype;
      STRFREE( mprg->arglist );
      mprg->arglist = STRALLOC( argument );
   }
   ch->substate = SUB_MPROG_EDIT;
   ch->pcdata->dest_buf = mprg;
   if( !mprg->comlist )
      mprg->comlist = STRALLOC( "" );
   start_editing( ch, mprg->comlist );
   set_editor_desc( ch, "A roomprog of some kind.." );
   return;
}

CMDF do_rpedit( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   char arg2[MIL];
   char arg3[MIL];
   MPROG_DATA *mprog, *mprg, *mprg_next = NULL;
   int value, mptype = -1, cnt;

   set_char_color( AT_PLAIN, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mob's can't rpedit\n\r", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_RESTRICTED:
         send_to_char( "You can't use this command from within another command.\r\n", ch );
         return;
      case SUB_MPROG_EDIT:
         if( !ch->pcdata->dest_buf )
         {
            send_to_char( "Fatal error: report to Samson.\n\r", ch );
            bug( "%s", "do_opedit: sub_oprog_edit: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            return;
         }
         mprog = ( MPROG_DATA * ) ch->pcdata->dest_buf;
         STRFREE( mprog->comlist );
         mprog->comlist = copy_buffer( ch );
         stop_editing( ch );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting roomprog.\n\r", ch );
         return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   value = atoi( arg2 );
   /*
    * argument = one_argument( argument, arg3 ); 
    */

   if( arg1[0] == '\0' )
   {
      send_to_char( "Syntax: rpedit <command> [number] <program> <value>\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Command being one of:\n\r", ch );
      send_to_char( "  add delete insert edit list\n\r", ch );
      send_to_char( "Program being one of:\n\r", ch );
      send_to_char( "  act speech rand sleep rest rfight entry\n\r", ch );
      send_to_char( "  leave death keyword\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "You should be standing in room you wish to edit.\n\r", ch );
      return;
   }

   if( !can_rmodify( ch, ch->in_room ) )
      return;

   mprog = ch->in_room->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !str_cmp( arg1, "list" ) )
   {
      cnt = 0;
      if( !mprog )
      {
         send_to_char( "This room has no room programs.\n\r", ch );
         return;
      }
      for( mprg = mprog; mprg; mprg = mprg->next )
         ch_printf( ch, "%d>%s %s\n\r%s\n\r", ++cnt, mprog_type_to_name( mprg->type ), mprg->arglist, mprg->comlist );
      return;
   }

   if( !str_cmp( arg1, "edit" ) )
   {
      if( !mprog )
      {
         send_to_char( "This room has no room programs.\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg3 );
      if( arg3[0] != '\0' )
      {
         mptype = get_mpflag( arg3 );
         if( mptype == -1 )
         {
            send_to_char( "Unknown program type.\n\r", ch );
            return;
         }
      }
      else
         mptype = -1;
      if( value < 1 )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      cnt = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mpedit( ch, mprg, mptype, argument );
            xCLEAR_BITS( ch->in_room->progtypes );
            for( mprg = mprog; mprg; mprg = mprg->next )
               xSET_BIT( ch->in_room->progtypes, mprg->type );
            return;
         }
      }
      send_to_char( "Program not found.\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "delete" ) )
   {
      int num;
      bool found;

      if( !mprog )
      {
         send_to_char( "That room has no room programs.\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg3 );
      if( value < 1 )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      cnt = 0;
      found = FALSE;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mptype = mprg->type;
            found = TRUE;
            break;
         }
      }
      if( !found )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      cnt = num = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
         if( mprg->type == mptype )
            num++;
      if( value == 1 )
      {
         mprg_next = ch->in_room->mudprogs;
         ch->in_room->mudprogs = mprg_next->next;
      }
      else
         for( mprg = mprog; mprg; mprg = mprg_next )
         {
            mprg_next = mprg->next;
            if( ++cnt == ( value - 1 ) )
            {
               mprg->next = mprg_next->next;
               break;
            }
         }
      if( mprg_next )
      {
         STRFREE( mprg_next->arglist );
         STRFREE( mprg_next->comlist );
         DISPOSE( mprg_next );
         if( num <= 1 )
            xREMOVE_BIT( ch->in_room->progtypes, mptype );
         send_to_char( "Program removed.\n\r", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "insert" ) )
   {
      if( !mprog )
      {
         send_to_char( "That room has no room programs.\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg3 );
      mptype = get_mpflag( arg2 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\n\r", ch );
         return;
      }
      if( value < 1 )
      {
         send_to_char( "Program not found.\n\r", ch );
         return;
      }
      if( value == 1 )
      {
         CREATE( mprg, MPROG_DATA, 1 );
         xSET_BIT( ch->in_room->progtypes, mptype );
         mpedit( ch, mprg, mptype, argument );
         mprg->next = mprog;
         ch->in_room->mudprogs = mprg;
         return;
      }
      cnt = 1;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value && mprg->next )
         {
            CREATE( mprg_next, MPROG_DATA, 1 );
            xSET_BIT( ch->in_room->progtypes, mptype );
            mpedit( ch, mprg_next, mptype, argument );
            mprg_next->next = mprg->next;
            mprg->next = mprg_next;
            return;
         }
      }
      send_to_char( "Program not found.\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "add" ) )
   {
      mptype = get_mpflag( arg2 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\n\r", ch );
         return;
      }
      if( mprog )
         for( ; mprog->next; mprog = mprog->next );
      CREATE( mprg, MPROG_DATA, 1 );
      if( mprog )
         mprog->next = mprg;
      else
         ch->in_room->mudprogs = mprg;
      xSET_BIT( ch->in_room->progtypes, mptype );
      mpedit( ch, mprg, mptype, argument );
      mprg->next = NULL;
      return;
   }

   do_rpedit( ch, "" );
}

CMDF do_adelete( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *area = NULL;
   char arg[MIL], filename[256];

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: adelete <areafilename>\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   for( area = first_area; area; area = area->next )
   {
      if( !str_cmp( area->filename, arg ) )
         break;
   }

   if( !area )
   {
      ch_printf( ch, "No such area as %s\n\r", arg );
      return;
   }

   if( !argument || argument[0] == '\0' || str_cmp( argument, "yes" ) )
   {
      send_to_char( "&RThis action must be confirmed before executing. It is not reversable.\n\r", ch );
      ch_printf( ch, "&RTo delete this area, type: &Wadelete %s yes&D", arg );
      return;
   }
   if( IS_AREA_FLAG( area, AFLAG_PROTOTYPE ) )
      snprintf( filename, 256, "%s%s", BUILD_DIR, area->filename );
   else
      mudstrlcpy( filename, area->filename, 256 );
   close_area( area );
   unlink( filename );
   write_area_list(  );
   ch_printf( ch, "&W%s&R has been destroyed.&D\n\r", arg );
   return;
}

CMDF do_rdelete( CHAR_DATA * ch, char *argument )
{
   ROOM_INDEX_DATA *location;

   if( ch->substate == SUB_RESTRICTED )
   {
      send_to_char( "You can't do that while in a subprompt.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Delete which room?\n\r", ch );
      return;
   }

   /*
    * Find the room. 
    */
   if( !( location = find_location( ch, argument ) ) )
   {
      send_to_char( "No such location.\n\r", ch );
      return;
   }

   /*
    * Does the player have the right to delete this room? 
    */
   if( get_trust( ch ) < sysdata.level_modify_proto
       && ( location->vnum < ch->pcdata->low_vnum || location->vnum > ch->pcdata->hi_vnum ) )
   {
      send_to_char( "That room is not in your assigned range.\n\r", ch );
      return;
   }
   delete_room( location );
   fix_exits(); /* Need to call this to solve a crash */
   ch_printf( ch, "Room %s has been deleted.\n\r", argument );
   return;
}

CMDF do_odelete( CHAR_DATA * ch, char *argument )
{
   OBJ_INDEX_DATA *obj;
   int vnum;

   if( ch->substate == SUB_RESTRICTED )
   {
      send_to_char( "You can't do that while in a subprompt.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Delete which object?\n\r", ch );
      return;
   }

   if( !is_number( argument ) )
   {
      send_to_char( "You must specify the object's vnum to delete it.\n\r", ch );
      return;
   }

   vnum = atoi( argument );

   /*
    * Find the obj. 
    */
   if( !( obj = get_obj_index( vnum ) ) )
   {
      send_to_char( "No such object.\n\r", ch );
      return;
   }

   /*
    * Does the player have the right to delete this object? 
    */
   if( get_trust( ch ) < sysdata.level_modify_proto
       && ( obj->vnum < ch->pcdata->low_vnum || obj->vnum > ch->pcdata->hi_vnum ) )
   {
      send_to_char( "That object is not in your assigned range.\n\r", ch );
      return;
   }
   delete_obj( obj );
   ch_printf( ch, "Object %d has been deleted.\n\r", vnum );
   return;
}

CMDF do_mdelete( CHAR_DATA * ch, char *argument )
{
   MOB_INDEX_DATA *mob;
   int vnum;

   if( ch->substate == SUB_RESTRICTED )
   {
      send_to_char( "You can't do that while in a subprompt.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Delete which mob?\n\r", ch );
      return;
   }

   if( !is_number( argument ) )
   {
      send_to_char( "You must specify the mob's vnum to delete it.\n\r", ch );
      return;
   }

   vnum = atoi( argument );

   /*
    * Find the mob. 
    */
   if( !( mob = get_mob_index( vnum ) ) )
   {
      send_to_char( "No such mob.\n\r", ch );
      return;
   }

   /*
    * Does the player have the right to delete this mob? 
    */
   if( get_trust( ch ) < sysdata.level_modify_proto
       && ( mob->vnum < ch->pcdata->low_vnum || mob->vnum > ch->pcdata->hi_vnum ) )
   {
      send_to_char( "That mob is not in your assigned range.\n\r", ch );
      return;
   }
   delete_mob( mob );
   ch_printf( ch, "Mob %d has been deleted.\n\r", vnum );
   return;
}

/*
 * function to allow modification of an area's climate
 * Last modified: July 15, 1997
 * Fireblade
 */
CMDF do_climate( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   AREA_DATA *area;

   /*
    * Little error checking 
    */
   if( !ch )
   {
      bug( "%s", "do_climate: NULL character." );
      return;
   }
   else if( !ch->in_room )
   {
      bug( "do_climate: %s not in a room.", ch->name );
      return;
   }
   else if( !ch->in_room->area )
   {
      bug( "do_climate: %s not in an area.", ch->name );
      return;
   }
   else if( !ch->in_room->area->weather )
   {
      bug( "do_climate: %s with NULL weather data.", ch->in_room->area->name );
      return;
   }

   set_char_color( AT_BLUE, ch );

   area = ch->in_room->area;

   argument = one_argument( argument, arg );

   /*
    * Display current climate settings 
    */
   if( !arg || arg[0] == '\0' )
   {
      NEIGHBOR_DATA *neigh;

      ch_printf( ch, "%s:\n\r", area->name );
      ch_printf( ch, "\tTemperature:\t%s\n\r", temp_settings[area->weather->climate_temp] );
      ch_printf( ch, "\tPrecipitation:\t%s\n\r", precip_settings[area->weather->climate_precip] );
      ch_printf( ch, "\tWind:\t\t%s\n\r", wind_settings[area->weather->climate_wind] );

      if( area->weather->first_neighbor )
         send_to_char( "\n\rNeighboring weather systems:\n\r", ch );

      for( neigh = area->weather->first_neighbor; neigh; neigh = neigh->next )
         ch_printf( ch, "\t%s\n\r", neigh->name );
      return;
   }

   /*
    * set climate temperature 
    */
   if( !str_cmp( arg, "temp" ) )
   {
      int i;
      argument = one_argument( argument, arg );

      for( i = 0; i < MAX_CLIMATE; i++ )
      {
         if( str_cmp( arg, temp_settings[i] ) )
            continue;

         area->weather->climate_temp = i;
         ch_printf( ch, "The climate temperature for %s is now %s.\n\r", area->name, temp_settings[i] );
         break;
      }

      if( i == MAX_CLIMATE )
      {
         ch_printf( ch, "Possible temperature settings:\n\r" );
         for( i = 0; i < MAX_CLIMATE; i++ )
            ch_printf( ch, "\t%s\n\r", temp_settings[i] );
      }
      return;
   }

   /*
    * set climate precipitation 
    */
   if( !str_cmp( arg, "precip" ) )
   {
      int i;
      argument = one_argument( argument, arg );

      for( i = 0; i < MAX_CLIMATE; i++ )
      {
         if( str_cmp( arg, precip_settings[i] ) )
            continue;

         area->weather->climate_precip = i;
         ch_printf( ch, "The climate precipitation for %s is now %s.\n\r", area->name, precip_settings[i] );
         break;
      }

      if( i == MAX_CLIMATE )
      {
         send_to_char( "Possible precipitation settings:\n\r", ch );
         for( i = 0; i < MAX_CLIMATE; i++ )
            ch_printf( ch, "\t%s\n\r", precip_settings[i] );
      }
      return;
   }

   /*
    * set climate wind 
    */
   if( !str_cmp( arg, "wind" ) )
   {
      int i;
      argument = one_argument( argument, arg );

      for( i = 0; i < MAX_CLIMATE; i++ )
      {
         if( str_cmp( arg, wind_settings[i] ) )
            continue;

         area->weather->climate_wind = i;
         ch_printf( ch, "The climate wind for %s is now %s.\n\r", area->name, wind_settings[i] );
         break;
      }

      if( i == MAX_CLIMATE )
      {
         send_to_char( "Possible wind settings:\n\r", ch );
         for( i = 0; i < MAX_CLIMATE; i++ )
            ch_printf( ch, "\t%s\n\r", wind_settings[i] );
      }
      return;
   }

   /*
    * add or remove neighboring weather systems 
    */
   if( !str_cmp( arg, "neighbor" ) )
   {
      NEIGHBOR_DATA *neigh;
      AREA_DATA *tarea;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Add or remove which area?\n\r", ch );
         return;
      }

      /*
       * look for a matching list item 
       */
      for( neigh = area->weather->first_neighbor; neigh; neigh = neigh->next )
      {
         if( nifty_is_name( argument, neigh->name ) )
            break;
      }

      /*
       * if the a matching list entry is found, remove it 
       */
      if( neigh )
      {
         /*
          * look for the neighbor area in question 
          */
         if( !( tarea = neigh->address ) )
            tarea = get_area( neigh->name );

         /*
          * if there is an actual neighbor area remove its entry to this area 
          */
         if( tarea )
         {
            NEIGHBOR_DATA *tneigh;

            tarea = neigh->address;
            for( tneigh = tarea->weather->first_neighbor; tneigh; tneigh = tneigh->next )
            {
               if( !str_cmp( area->name, tneigh->name ) )
                  break;
            }
            UNLINK( tneigh, tarea->weather->first_neighbor, tarea->weather->last_neighbor, next, prev );
            STRFREE( tneigh->name );
            DISPOSE( tneigh );
         }
         UNLINK( neigh, area->weather->first_neighbor, area->weather->last_neighbor, next, prev );
         ch_printf( ch, "The weather in %s and %s no longer affect each other.\n\r", neigh->name, area->name );
         STRFREE( neigh->name );
         DISPOSE( neigh );
      }
      /*
       * otherwise add an entry 
       */
      else
      {
         tarea = get_area( argument );

         if( !tarea )
         {
            send_to_char( "No such area exists.\n\r", ch );
            return;
         }

         if( tarea == area )
         {
            ch_printf( ch, "%s already affects its own weather.\n\r", area->name );
            return;
         }

         /*
          * add the entry 
          */
         CREATE( neigh, NEIGHBOR_DATA, 1 );
         neigh->name = STRALLOC( tarea->name );
         neigh->address = tarea;
         LINK( neigh, area->weather->first_neighbor, area->weather->last_neighbor, next, prev );

         /*
          * add an entry to the neighbor's list 
          */
         CREATE( neigh, NEIGHBOR_DATA, 1 );
         neigh->name = STRALLOC( area->name );
         neigh->address = area;
         LINK( neigh, tarea->weather->first_neighbor, tarea->weather->last_neighbor, next, prev );
         ch_printf( ch, "The weather in %s and %s now affect one another.\n\r", tarea->name, area->name );
      }
      return;
   }
   send_to_char( "Climate may only be followed by one of the following fields:\n\r", ch );
   send_to_char( "temp precip wind tneighbor\n\r", ch );
   return;
}

/*  
 *  Mobile and Object Program Copying 
 *  Last modified Feb. 24 1999
 *  Mystaric
 */

void mpcopy( MPROG_DATA * source, MPROG_DATA * destination )
{
   destination->type = source->type;
   destination->triggered = source->triggered;
   destination->resetdelay = source->resetdelay;
   destination->arglist = STRALLOC( source->arglist );
   destination->comlist = STRALLOC( source->comlist );
   destination->next = NULL;
}

CMDF do_opcopy( CHAR_DATA * ch, char *argument )
{
   char sobj[MIL], prog[MIL], num[MIL], dobj[MIL];
   OBJ_DATA *source = NULL, *destination = NULL;
   MPROG_DATA *source_oprog = NULL, *dest_oprog = NULL, *source_oprg = NULL, *dest_oprg = NULL;
   int value = -1, optype = -1, cnt = 0;
   bool COPY = FALSE;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mob's can't opcopy\n\r", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\n\r", ch );
      return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, sobj );
   argument = one_argument( argument, prog );

   if( sobj[0] == '\0' || prog[0] == '\0' )
   {
      send_to_char( "Syntax: opcopy <source object> <program> [number] <destination object>\n\r", ch );
      send_to_char( "        opcopy <source object> all <destination object>\n\r", ch );
      send_to_char( "        opcopy <source object> all <destination object> <program>\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Program being one of:\n\r", ch );
      send_to_char( "  act speech rand wear remove sac zap get\n\r", ch );
      send_to_char( "  drop damage repair greet exa use\n\r", ch );
      send_to_char( "  pull push (for levers,pullchains,buttons)\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Object should be in your inventory to edit.\n\r", ch );
      return;
   }

   if( !str_cmp( prog, "all" ) )
   {
      argument = one_argument( argument, dobj );
      argument = one_argument( argument, prog );
      optype = get_mpflag( prog );
      COPY = TRUE;
   }
   else
   {
      argument = one_argument( argument, num );
      argument = one_argument( argument, dobj );
      value = atoi( num );
   }

   if( get_trust( ch ) < LEVEL_GOD )
   {
      if( !( source = get_obj_carry( ch, sobj ) ) )
      {
         send_to_char( "You aren't carrying source object.\n\r", ch );
         return;
      }

      if( !( destination = get_obj_carry( ch, dobj ) ) )
      {
         send_to_char( "You aren't carrying destination object.\n\r", ch );
         return;
      }
   }
   else
   {
      if( !( source = get_obj_world( ch, sobj ) ) )
      {
         send_to_char( "Can't find source object in all the realms.\n\r", ch );
         return;
      }

      if( !( destination = get_obj_world( ch, dobj ) ) )
      {
         send_to_char( "Can't find destination object in all the realms.\n\r", ch );
         return;
      }
   }

   if( source == destination )
   {
      send_to_char( "Source and destination objects cannot be the same\n\r", ch );
      return;
   }

   if( !can_omodify( ch, destination ) )
   {
      send_to_char( "You cannot modify destination object.\n\r", ch );
      return;
   }

   if( !IS_OBJ_FLAG( destination, ITEM_PROTOTYPE ) )
   {
      send_to_char( "Destination object must have prototype flag.\n\r", ch );
      return;
   }

   source_oprog = source->pIndexData->mudprogs;
   dest_oprog = destination->pIndexData->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !source_oprog )
   {
      send_to_char( "Source object has no mob programs.\n\r", ch );
      return;
   }

   if( COPY )
   {
      for( source_oprg = source_oprog; source_oprg; source_oprg = source_oprg->next )
      {
         if( optype == source_oprg->type || optype == -1 )
         {
            if( dest_oprog != NULL )
               for( ; dest_oprog->next; dest_oprog = dest_oprog->next );
            CREATE( dest_oprg, MPROG_DATA, 1 );
            if( dest_oprog )
               dest_oprog->next = dest_oprg;
            else
            {
               destination->pIndexData->mudprogs = dest_oprg;
               dest_oprog = dest_oprg;
            }
            mpcopy( source_oprg, dest_oprg );
            xSET_BIT( destination->pIndexData->progtypes, dest_oprg->type );
            cnt++;
         }
      }

      if( cnt == 0 )
      {
         send_to_char( "No such program in source object\n\r", ch );
         return;
      }
      ch_printf( ch, "%d programs successfully copied from %s to %s.\n\r", cnt, sobj, dobj );
      return;
   }

   if( value < 1 )
   {
      send_to_char( "No such program in source object.\n\r", ch );
      return;
   }

   optype = get_mpflag( prog );

   for( source_oprg = source_oprog; source_oprg; source_oprg = source_oprg->next )
   {
      if( ++cnt == value && source_oprg->type == optype )
      {
         if( dest_oprog != NULL )
            for( ; dest_oprog->next; dest_oprog = dest_oprog->next );
         CREATE( dest_oprg, MPROG_DATA, 1 );
         if( dest_oprog )
            dest_oprog->next = dest_oprg;
         else
            destination->pIndexData->mudprogs = dest_oprg;
         mpcopy( source_oprg, dest_oprg );
         xSET_BIT( destination->pIndexData->progtypes, dest_oprg->type );
         ch_printf( ch, "%s program %d from %s successfully copied to %s.\n\r", prog, value, sobj, dobj );
         return;
      }
   }
   send_to_char( "No such program in source object.\n\r", ch );
   return;
}

CMDF do_mpcopy( CHAR_DATA * ch, char *argument )
{
   char smob[MIL], prog[MIL], num[MIL], dmob[MIL];
   CHAR_DATA *source = NULL, *destination = NULL;
   MPROG_DATA *source_mprog = NULL, *dest_mprog = NULL, *source_mprg = NULL, *dest_mprg = NULL;
   int value = -1, mptype = -1, cnt = 0;
   bool COPY = FALSE;

   set_char_color( AT_PLAIN, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mob's can't opcop\n\r", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\n\r", ch );
      return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, smob );
   argument = one_argument( argument, prog );

   if( smob[0] == '\0' || prog[0] == '\0' )
   {
      send_to_char( "Syntax: mpcopy <source mobile> <program> [number] <destination mobile>\n\r", ch );
      send_to_char( "        mpcopy <source mobile> all <destination mobile>\n\r", ch );
      send_to_char( "        mpcopy <source mobile> all <destination mobile> <program>\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Program being one of:\n\r", ch );
      send_to_char( "  act speech rand fight hitprcnt greet allgreet\n\r", ch );
      send_to_char( "  entry give bribe death time hour script\n\r", ch );
      return;
   }

   if( !str_cmp( prog, "all" ) )
   {
      argument = one_argument( argument, dmob );
      argument = one_argument( argument, prog );
      mptype = get_mpflag( prog );
      COPY = TRUE;
   }
   else
   {
      argument = one_argument( argument, num );
      argument = one_argument( argument, dmob );
      value = atoi( num );
   }

   if( get_trust( ch ) < LEVEL_GOD )
   {
      if( !( source = get_char_room( ch, smob ) ) )
      {
         send_to_char( "Source mobile is not present.\n\r", ch );
         return;
      }

      if( !( destination = get_char_room( ch, dmob ) ) )
      {
         send_to_char( "Destination mobile is not present.\n\r", ch );
         return;
      }
   }
   else
   {
      if( !( source = get_char_world( ch, smob ) ) )
      {
         send_to_char( "Can't find source mobile\n\r", ch );
         return;
      }

      if( !( destination = get_char_world( ch, dmob ) ) )
      {
         send_to_char( "Can't find destination mobile\n\r", ch );
         return;
      }
   }
   if( source == destination )
   {
      send_to_char( "Source and destination mobiles cannot be the same\n\r", ch );
      return;
   }

   if( get_trust( ch ) < source->level || !IS_NPC( source ) || get_trust( ch ) < destination->level
       || !IS_NPC( destination ) )
   {
      send_to_char( "You can't do that!\n\r", ch );
      return;
   }

   if( !can_mmodify( ch, destination ) )
   {
      send_to_char( "You cannot modify destination mobile.\n\r", ch );
      return;
   }

   if( !IS_ACT_FLAG( destination, ACT_PROTOTYPE ) )
   {
      send_to_char( "Destination mobile must have a prototype flag to mpcopy.\n\r", ch );
      return;
   }

   source_mprog = source->pIndexData->mudprogs;
   dest_mprog = destination->pIndexData->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !source_mprog )
   {
      send_to_char( "Source mobile has no mob programs.\n\r", ch );
      return;
   }

   if( COPY )
   {
      for( source_mprg = source_mprog; source_mprg; source_mprg = source_mprg->next )
      {
         if( mptype == source_mprg->type || mptype == -1 )
         {
            if( dest_mprog != NULL )
               for( ; dest_mprog->next; dest_mprog = dest_mprog->next );
            CREATE( dest_mprg, MPROG_DATA, 1 );

            if( dest_mprog )
               dest_mprog->next = dest_mprg;
            else
            {
               destination->pIndexData->mudprogs = dest_mprg;
               dest_mprog = dest_mprg;
            }
            mpcopy( source_mprg, dest_mprg );
            xSET_BIT( destination->pIndexData->progtypes, dest_mprg->type );
            cnt++;
         }
      }
      if( cnt == 0 )
      {
         ch_printf( ch, "No such program in source mobile\n\r" );
         return;
      }
      ch_printf( ch, "%d programs successfully copied from %s to %s.\n\r", cnt, smob, dmob );
      return;
   }

   if( value < 1 )
   {
      send_to_char( "No such program in source mobile.\n\r", ch );
      return;
   }

   mptype = get_mpflag( prog );

   for( source_mprg = source_mprog; source_mprg; source_mprg = source_mprg->next )
   {
      if( ++cnt == value && source_mprg->type == mptype )
      {
         if( dest_mprog != NULL )
            for( ; dest_mprog->next; dest_mprog = dest_mprog->next );
         CREATE( dest_mprg, MPROG_DATA, 1 );
         if( dest_mprog )
            dest_mprog->next = dest_mprg;
         else
            destination->pIndexData->mudprogs = dest_mprg;
         mpcopy( source_mprg, dest_mprg );
         xSET_BIT( destination->pIndexData->progtypes, dest_mprg->type );
         ch_printf( ch, "%s program %d from %s successfully copied to %s.\n\r", prog, value, smob, dmob );
         return;
      }
   }
   send_to_char( "No such program in source mobile.\n\r", ch );
   return;
}

CMDF do_makerooms( CHAR_DATA * ch, char *argument )
{
   ROOM_INDEX_DATA *location;
   AREA_DATA *pArea;
   int vnum, x, room_count;

   pArea = ch->pcdata->area;

   if( !pArea )
   {
      send_to_char( "You must have an area assigned to do this.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Create a block of rooms.\n\r", ch );
      send_to_char( "Usage: makerooms <# of rooms>\n\r", ch );
      return;
   }
   x = atoi( argument );

   ch_printf( ch, "Attempting to create a block of %d rooms.\n\r", x );

   if( x > 1000 )
   {
      ch_printf( ch, "The maximum number of rooms this mud can create at once is 1000.\n\r" );
      return;
   }

   room_count = 0;

   if( pArea->low_vnum + x > pArea->hi_vnum )
   {
      send_to_char( "You don't even have that many rooms assigned to you.\n\r", ch );
      return;
   }

   for( vnum = pArea->low_vnum; vnum <= pArea->hi_vnum; ++vnum )
   {
      if( get_room_index( vnum ) == NULL )
         ++room_count;

      if( room_count >= x )
         break;
   }

   if( room_count < x )
   {
      send_to_char( "There aren't enough free rooms in your assigned range!\n\r", ch );
      return;
   }

   send_to_char( "Creating the rooms...\n\r", ch );

   room_count = 0;
   vnum = pArea->low_vnum;

   while( room_count < x )
   {
      if( get_room_index( vnum ) == NULL )
      {
         ++room_count;

         location = make_room( vnum, pArea );
         if( !location )
         {
            bug( "%s: make_room failed", __FUNCTION__ );
            return;
         }
      }
      ++vnum;
   }
   ch_printf( ch, "%d rooms created.\n\r", room_count );
   return;
}

/* Consolidated *assign function. 
 * Assigns room/obj/mob ranges and initializes new zone - Samson 2-12-99 
 */
CMDF do_vassign( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL], arg3[MIL];
   int lo, hi;
   CHAR_DATA *victim, *mob;
   ROOM_INDEX_DATA *room;
   MOB_INDEX_DATA *pMobIndex;
   OBJ_INDEX_DATA *pObjIndex;
   OBJ_DATA *obj;
   AREA_DATA *tarea;
   char filename[256];

   set_char_color( AT_IMMORT, ch );

#ifdef MULTIPORT
   if( port == MAINPORT )
   {
      send_to_char( "Vassign is disabled on this port.\n\r", ch );
      return;
   }
#endif

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   lo = atoi( arg2 );
   hi = atoi( arg3 );

   if( arg1[0] == '\0' || lo < 0 || hi < 0 )
   {
      send_to_char( "Syntax: vassign <who> <low> <high>\n\r", ch );
      return;
   }
   if( ( victim = get_char_world( ch, arg1 ) ) == NULL )
   {
      send_to_char( "They don't seem to be around.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) || get_trust( victim ) < LEVEL_CREATOR )
   {
      send_to_char( "They wouldn't know what to do with a vnum range.\n\r", ch );
      return;
   }
   if( lo == 0 && hi == 0 )
   {
      if( victim->pcdata->area )
         close_area( victim->pcdata->area );
      victim->pcdata->area = NULL;
      victim->pcdata->low_vnum = 0;
      victim->pcdata->hi_vnum = 0;
      ch_printf( victim, "%s has removed your vnum range.\n\r", ch->name );
      save_char_obj( victim );
      return;
   }
   if( lo >= sysdata.maxvnum || hi >= sysdata.maxvnum )
   {
      ch_printf( ch, "Cannot assign this range, maximum allowable vnum is currently %d.\n\r", sysdata.maxvnum );
      return;
   }
   if( lo == 0 && hi != 0 )
   {
      send_to_char( "Unacceptable vnum range, low vnum cannot be 0 when hi vnum is not.\n\r", ch );
      return;
   }
   if( lo > hi )
   {
      send_to_char( "Unacceptable vnum range, low vnum must be smaller than high vnum.\n\r", ch );
      return;
   }
   if( victim->pcdata->area || victim->pcdata->low_vnum || victim->pcdata->hi_vnum )
   {
      send_to_char( "You cannot assign them a range, they already have one!\n\r", ch );
      return;
   }
   victim->pcdata->low_vnum = lo;
   victim->pcdata->hi_vnum = hi;
   assign_area( victim );
   send_to_char( "Done.\n\r", ch );
   assign_area( victim );  /* Put back by Thoric on 02/07/96 */
   ch_printf( victim, "&Y%s has assigned you the vnum range %d - %d.\n\r", ch->name, lo, hi );

   if( !victim->pcdata->area )
   {
      bug( "%s: assign_area failed", __FUNCTION__ );
      return;
   }

   tarea = victim->pcdata->area;

   /*
    * Initialize first and last rooms in range 
    */
   if( !( room = make_room( lo, tarea ) ) )
   {
      bug( "%s: make_room failed to initialize first room.", __FUNCTION__ );
      return;
   }

   if( !( room = make_room( hi, tarea ) ) )
   {
      bug( "%s: make_room failed to initialize last room.", __FUNCTION__ );
      return;
   }

   /*
    * Initialize first mob in range 
    */
   if( !( pMobIndex = make_mobile( lo, 0, "first mob" ) ) )
   {
      bug( "%s: make_mobile failed to initialize first mob.", __FUNCTION__ );
      return;
   }
   mob = create_mobile( pMobIndex );
   if( !char_to_room( mob, room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   /*
    * Initialize last mob in range 
    */
   if( !( pMobIndex = make_mobile( hi, 0, "last mob" ) ) )
   {
      bug( "%s: make_mobile failed to initialize last mob.", __FUNCTION__ );
      return;
   }
   mob = create_mobile( pMobIndex );
   if( !char_to_room( mob, room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   /*
    * Initialize first obj in range 
    */
   if( !( pObjIndex = make_object( lo, 0, "first obj" ) ) )
   {
      bug( "%s: make_object failed to initialize first obj.", __FUNCTION__ );
      return;
   }
   if( !( obj = create_object( pObjIndex, 0 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   obj_to_room( obj, room, NULL );

   /*
    * Initialize last obj in range 
    */
   if( !( pObjIndex = make_object( hi, 0, "last obj" ) ) )
   {
      bug( "%s: make_object failed to initialize last obj.", __FUNCTION__ );
      return;
   }
   if( !( obj = create_object( pObjIndex, 0 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   obj_to_room( obj, room, NULL );

   /*
    * Save character and newly created zone 
    */
   save_char_obj( victim );

   snprintf( filename, 256, "%s%s", BUILD_DIR, tarea->filename );
   fold_area( tarea, filename, FALSE );

   SET_AREA_FLAG( tarea, AFLAG_PROTOTYPE );
   set_char_color( AT_IMMORT, ch );
   ch_printf( ch, "Vnum range set for %s and initialized.\n\r", victim->name );
   return;
}
