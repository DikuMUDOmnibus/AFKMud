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
 *                   Main structure manipulation module                     *
 ****************************************************************************/

#include <string.h>
#include "mud.h"
#include "auction.h"
#include "deity.h"
#include "fight.h"
#include "mud_prog.h"
#include "polymorph.h"

extern int top_exit;
extern int top_ed;
extern int top_affect;
extern int cur_qobjs;
extern int cur_qchars;
extern CHAR_DATA *gch_prev;
extern OBJ_DATA *gobj_prev;

ch_ret global_retcode;
int falling;

void write_corpses( CHAR_DATA * ch, char *name, OBJ_DATA * objrem );
void update_visits( CHAR_DATA * ch, ROOM_INDEX_DATA * room );
void die_follower( CHAR_DATA * ch );
void obj_fall( OBJ_DATA * obj, bool through );
void set_attacks( CHAR_DATA * ch );
void free_obj( OBJ_DATA * obj );
ROOM_INDEX_DATA *check_room( CHAR_DATA * ch, ROOM_INDEX_DATA * dieroom );
void delete_reset( RESET_DATA * pReset );

typedef struct extracted_char_data EXTRACT_CHAR_DATA;

struct extracted_char_data
{
   EXTRACT_CHAR_DATA *next;
   CHAR_DATA *ch;
   ROOM_INDEX_DATA *room;
   ch_ret retcode;
   bool extract;
};

OBJ_DATA *extracted_obj_queue;
EXTRACT_CHAR_DATA *extracted_char_queue;
extern REL_DATA *first_relation;
extern REL_DATA *last_relation;

void free_teleports( void )
{
   TELEPORT_DATA *tele, *tele_next;

   for( tele = first_teleport; tele; tele = tele_next )
   {
      tele_next = tele->next;

      UNLINK( tele, first_teleport, last_teleport, next, prev );
      DISPOSE( tele );
   }
}

/* Brought over from DOTD code, caclulates such things as the number of
   attacks a PC gets, as well as monk barehand damage and some other
   RIS flags - Samson 4-6-99 
*/
void ClassSpecificStuff( CHAR_DATA * ch )
{
   if( IS_NPC( ch ) )
      return;

   if( IsDragon( ch ) )
      ch->armor = UMIN( -150, ch->armor );

   set_attacks( ch );

   if( ch->Class == CLASS_MONK )
   {
      switch ( ch->level )
      {
         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
            ch->barenumdie = 1;
            ch->baresizedie = 3;
            break;
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
            ch->barenumdie = 1;
            ch->baresizedie = 4;
            break;
         case 11:
         case 12:
         case 13:
         case 14:
         case 15:
            ch->barenumdie = 1;
            ch->baresizedie = 6;
            break;
         case 16:
         case 17:
         case 18:
         case 19:
         case 20:
            ch->barenumdie = 1;
            ch->baresizedie = 8;
            break;
         case 21:
         case 22:
         case 23:
         case 24:
         case 25:
            ch->barenumdie = 2;
            ch->baresizedie = 3;
            break;
         case 26:
         case 27:
         case 28:
         case 29:
         case 30:
            ch->barenumdie = 2;
            ch->baresizedie = 4;
            break;
         case 31:
         case 32:
         case 33:
         case 34:
         case 35:
            ch->barenumdie = 1;
            ch->baresizedie = 10;
            break;
         case 36:
         case 37:
         case 38:
         case 39:
         case 40:
            ch->barenumdie = 1;
            ch->baresizedie = 12;
            break;
         case 41:
         case 42:
         case 43:
         case 44:
         case 45:
            ch->barenumdie = 1;
            ch->baresizedie = 15;
            break;
         case 46:
         case 47:
         case 48:
         case 49:
         case 50:
            ch->barenumdie = 2;
            ch->baresizedie = 5;
            break;
         case 51:
         case 52:
         case 53:
         case 54:
         case 55:
            ch->barenumdie = 2;
            ch->baresizedie = 6;
            break;
         case 56:
         case 57:
         case 58:
         case 59:
         case 60:
            ch->barenumdie = 3;
            ch->baresizedie = 5;
            break;
         case 61:
         case 62:
         case 63:
         case 64:
         case 65:
            ch->barenumdie = 3;
            ch->baresizedie = 6;
            break;
         case 66:
         case 67:
         case 68:
         case 69:
         case 70:
            ch->barenumdie = 1;
            ch->baresizedie = 20;
            break;
         case 71:
         case 72:
         case 73:
         case 74:
         case 75:
            ch->barenumdie = 4;
            ch->baresizedie = 5;
            break;
         case 76:
         case 77:
         case 78:
         case 79:
         case 80:
            ch->barenumdie = 5;
            ch->baresizedie = 4;
            break;
         case 81:
         case 82:
         case 83:
         case 84:
         case 85:
            ch->barenumdie = 5;
            ch->baresizedie = 5;
            break;
         case 86:
         case 87:
         case 88:
         case 89:
         case 90:
            ch->barenumdie = 5;
            ch->baresizedie = 6;
            break;
         case 91:
         case 92:
         case 93:
         case 94:
         case 95:
            ch->barenumdie = 6;
            ch->baresizedie = 6;
            break;
         default:
            ch->barenumdie = 7;
            ch->baresizedie = 6;
            break;
      }

      if( ch->level >= 20 )
         SET_RESIS( ch, RIS_HOLD );
      if( ch->level >= 36 )
         SET_RESIS( ch, RIS_CHARM );
      if( ch->level >= 44 )
         SET_RESIS( ch, RIS_POISON );
      if( ch->level >= 62 )
      {
         SET_IMMUNE( ch, RIS_CHARM );
         REMOVE_RESIS( ch, RIS_CHARM );
      }
      if( ch->level >= 70 )
      {
         SET_IMMUNE( ch, RIS_POISON );
         REMOVE_RESIS( ch, RIS_POISON );
      }
      ch->armor = 100 - UMIN( 150, ch->level * 5 );
      ch->max_move = UMAX( ch->max_move, ( 150 + ( ch->level * 5 ) ) );
   }

   if( ch->Class == CLASS_DRUID )
   {
      if( ch->level >= 28 )
         SET_IMMUNE( ch, RIS_CHARM );
      if( ch->level >= 64 )
         SET_RESIS( ch, RIS_POISON );
   }

   if( ch->Class == CLASS_NECROMANCER )
   {
      if( ch->level >= 20 )
         SET_RESIS( ch, RIS_COLD );
      if( ch->level >= 40 )
         SET_RESIS( ch, RIS_FIRE );
      if( ch->level >= 45 )
         SET_RESIS( ch, RIS_ENERGY );
      if( ch->level >= 85 )
      {
         SET_IMMUNE( ch, RIS_COLD );
         REMOVE_RESIS( ch, RIS_COLD );
      }
      if( ch->level >= 90 )
      {
         SET_IMMUNE( ch, RIS_FIRE );
         REMOVE_RESIS( ch, RIS_FIRE );
      }
   }
}

/*								-Thoric
 * Return how much experience is required for ch to get to a certain level
 */
long exp_level( int level )
{
   long x, y = 0;

   for( x = 1; x < level; x++ )
      y += ( x ) * ( x * 625 );

   return y;
}

/*
 * Retrieve a character's trusted level for permission checking.
 */
short get_trust( CHAR_DATA * ch )
{
   if( ch->desc && ch->desc->original )
      ch = ch->desc->original;

   if( ch->trust != 0 )
      return ch->trust;

   if( IS_NPC( ch ) && ch->level >= LEVEL_AVATAR )
      return LEVEL_AVATAR;

   if( ch->level >= LEVEL_IMMORTAL && IS_RETIRED( ch ) )
      return LEVEL_IMMORTAL;

   return ch->level;
}

/*
 * Retrieve a character's age.
 */
/* Function modified from original form by Samson - unknown date.
   Aging advances at the same rate as the calendar date now */
short get_age( CHAR_DATA * ch )
{
   if( IS_NPC( ch ) )
      return -1;

   return ch->pcdata->age + ch->pcdata->age_bonus; /* There, now isn't this simpler? :P */
}

/* One hopes this will do as planned and determine how old a PC is based on the birthdate
   we record at creation. - Samson 10-25-99 */
short calculate_age( CHAR_DATA * ch )
{
   short age, num_days, ch_days;

   if( IS_NPC( ch ) )
      return -1;

   num_days = ( time_info.month + 1 ) * sysdata.dayspermonth;
   num_days += time_info.day;

   ch_days = ( ch->pcdata->month + 1 ) * sysdata.dayspermonth;
   ch_days += ch->pcdata->day;

   age = time_info.year - ch->pcdata->year;

   if( ch_days - num_days > 0 )
      age -= 1;

   return age;
}

/*
 * Retrieve character's current strength.
 */
short get_curr_str( CHAR_DATA * ch )
{
   short max;

   if( IS_NPC( ch ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, ch->perm_str + ch->mod_str, max );
}

/*
 * Retrieve character's current intelligence.
 */
short get_curr_int( CHAR_DATA * ch )
{
   short max;

   if( IS_NPC( ch ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, ch->perm_int + ch->mod_int, max );
}

/*
 * Retrieve character's current wisdom.
 */
short get_curr_wis( CHAR_DATA * ch )
{
   short max;

   if( IS_NPC( ch ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, ch->perm_wis + ch->mod_wis, max );
}

/*
 * Retrieve character's current dexterity.
 */
short get_curr_dex( CHAR_DATA * ch )
{
   short max;

   if( IS_NPC( ch ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, ch->perm_dex + ch->mod_dex, max );
}

/*
 * Retrieve character's current constitution.
 */
short get_curr_con( CHAR_DATA * ch )
{
   short max;

   if( IS_NPC( ch ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, ch->perm_con + ch->mod_con, max );
}

/*
 * Retrieve character's current charisma.
 */
short get_curr_cha( CHAR_DATA * ch )
{
   short max;

   if( IS_NPC( ch ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, ch->perm_cha + ch->mod_cha, max );
}

/*
 * Retrieve character's current luck.
 */
short get_curr_lck( CHAR_DATA * ch )
{
   short max;

   if( IS_NPC( ch ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, ch->perm_lck + ch->mod_lck, max );
}

/*
 * Retrieve a character's carry capacity.
 * Vastly reduced (finally) due to containers		-Thoric
 */
/* Function modified from original form, capacity formula changed + pets can carry - Samson 4-12-98 */
int can_carry_n( CHAR_DATA * ch )
{
   int penalty = 0;

   if( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
      return ch->level * 200;

   if( !IS_NPC( ch ) && ch->Class == CLASS_MONK )
      return 20;  /* Monks can only carry 20 items total */

   /*
    * Come now, never heard of pack animals people? Altered so pets can hold up to 10 - Samson 4-12-98 
    */
   if( IS_ACT_FLAG( ch, ACT_PET ) )
      return 10;

   if( IS_ACT_FLAG( ch, ACT_IMMORTAL ) )
      return ch->level * 200;

   if( get_eq_char( ch, WEAR_WIELD ) )
      ++penalty;
   if( get_eq_char( ch, WEAR_DUAL_WIELD ) )
      ++penalty;
   if( get_eq_char( ch, WEAR_MISSILE_WIELD ) )
      ++penalty;
   if( get_eq_char( ch, WEAR_HOLD ) )
      ++penalty;
   if( get_eq_char( ch, WEAR_SHIELD ) )
      ++penalty;

   /*
    * Removed old formula, added something a bit more sensible here
    * Samson 4-12-98. Minimum of 15, (dex+str+level)-10 - penalty, or a max of 40. 
    */
   return URANGE( 15, ( get_curr_dex( ch ) + get_curr_str( ch ) + ch->level ) - 10 - penalty, 40 );
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w( CHAR_DATA * ch )
{
   if( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
      return 1000000;

   if( IS_ACT_FLAG( ch, ACT_IMMORTAL ) )
      return 1000000;

   return str_app[get_curr_str( ch )].carry;
}

/*
 * See if a player/mob can take a piece of prototype eq		-Thoric
 */
bool can_take_proto( CHAR_DATA * ch )
{
   if( IS_IMMORTAL( ch ) )
      return TRUE;
   else if( IS_ACT_FLAG( ch, ACT_PROTOTYPE ) )
      return TRUE;
   else
      return FALSE;
}

/*
 * See if a string is one of the names of an object.
 */
bool is_name( const char *str, char *namelist )
{
   char name[MIL];

   for( ;; )
   {
      namelist = one_argument( namelist, name );
      if( name[0] == '\0' )
         return FALSE;
      if( !str_cmp( str, name ) )
         return TRUE;
   }
}

bool is_name_prefix( const char *str, char *namelist )
{
   char name[MIL];

   for( ;; )
   {
      namelist = one_argument( namelist, name );
      if( name[0] == '\0' )
         return FALSE;
      if( !str_prefix( str, name ) )
         return TRUE;
   }
}

/*
 * See if a string is one of the names of an object.		-Thoric
 * Treats a dash as a word delimiter as well as a space
 */
bool is_name2( const char *str, char *namelist )
{
   char name[MIL];

   for( ;; )
   {
      namelist = one_argument2( namelist, name );
      if( name[0] == '\0' )
         return FALSE;
      if( !str_cmp( str, name ) )
         return TRUE;
   }
}

bool is_name2_prefix( const char *str, char *namelist )
{
   char name[MIL];

   for( ;; )
   {
      namelist = one_argument2( namelist, name );
      if( name[0] == '\0' )
         return FALSE;
      if( !str_prefix( str, name ) )
         return TRUE;
   }
}

/*								-Thoric
 * Checks if str is a name in namelist supporting multiple keywords
 */
bool nifty_is_name( char *str, const char *namelist )
{
   char name[MIL], nlist[MIL];

   if( !str || str[0] == '\0' || !namelist || namelist[0] == '\0' )
      return FALSE;

   mudstrlcpy( nlist, namelist, MIL );
   for( ;; )
   {
      str = one_argument2( str, name );
      if( name[0] == '\0' )
         return TRUE;
      if( !is_name2( name, nlist ) )
         return FALSE;
   }
}

bool nifty_is_name_prefix( char *str, char *namelist )
{
   char name[MIL];

   if( !str || str[0] == '\0' || !namelist || namelist[0] == '\0' )
      return FALSE;

   for( ;; )
   {
      str = one_argument2( str, name );
      if( name[0] == '\0' )
         return TRUE;
      if( !is_name2_prefix( name, namelist ) )
         return FALSE;
   }
}

void room_affect( ROOM_INDEX_DATA * pRoomIndex, AFFECT_DATA * paf, bool fAdd )
{
   if( fAdd )
   {
      switch ( paf->location )
      {
         case APPLY_ROOMFLAG:
         case APPLY_SECTORTYPE:
            break;
         case APPLY_ROOMLIGHT:
            pRoomIndex->light += paf->modifier;
            break;
         case APPLY_TELEVNUM:
         case APPLY_TELEDELAY:
            break;
      }
   }
   else
   {
      switch ( paf->location )
      {
         case APPLY_ROOMFLAG:
         case APPLY_SECTORTYPE:
            break;
         case APPLY_ROOMLIGHT:
            pRoomIndex->light -= paf->modifier;
            break;
         case APPLY_TELEVNUM:
         case APPLY_TELEDELAY:
            break;
      }
   }
}

/*
 * Modify a skill (hopefully) properly			-Thoric
 *
 * On "adding" a skill modifying affect, the value set is unimportant
 * upon removing the affect, the skill it enforced to a proper range.
 */
void modify_skill( CHAR_DATA * ch, int sn, int mod, bool fAdd )
{
   if( !IS_NPC( ch ) )
   {
      if( fAdd )
         ch->pcdata->learned[sn] += mod;
      else
         ch->pcdata->learned[sn] = URANGE( 0, ch->pcdata->learned[sn] + mod, GET_ADEPT( ch, sn ) );
   }
}

/*
 * Apply or remove an affect to a character.
 */
void affect_modify( CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd )
{
   OBJ_DATA *wield;
   int mod;
   struct skill_type *skill;
   ch_ret retcode;

   mod = paf->modifier;

   if( fAdd )
   {
      if( paf->bit >= 0 && paf->bit < MAX_AFFECTED_BY )
         SET_AFFECTED( ch, paf->bit );
      if( paf->location % REVERSE_APPLY == APPLY_RECURRINGSPELL )
      {
         mod = abs( mod );
         if( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL && skill->type == SKILL_SPELL )
            SET_AFFECTED( ch, AFF_RECURRINGSPELL );
         else
            bug( "affect_modify(%s) APPLY_RECURRINGSPELL with bad sn %d", ch->name, mod );
         return;
      }
   }
   else
   {
      if( paf->bit >= 0 && paf->bit < MAX_AFFECTED_BY )
         REMOVE_AFFECTED( ch, paf->bit );
      /*
       * might be an idea to have a duration removespell which returns
       * the spell after the duration... but would have to store
       * the removed spell's information somewhere...     -Thoric
       */
      if( paf->location % REVERSE_APPLY == APPLY_RECURRINGSPELL )
      {
         mod = abs( mod );
         if( !IS_VALID_SN( mod ) || ( skill = skill_table[mod] ) == NULL || skill->type != SKILL_SPELL )
            bug( "affect_modify(%s) APPLY_RECURRINGSPELL with bad sn %d", ch->name, mod );
         REMOVE_AFFECTED( ch, AFF_RECURRINGSPELL );
         return;
      }

      switch ( paf->location % REVERSE_APPLY )
      {
         case APPLY_AFFECT:
            REMOVE_BIT( ch->affected_by.bits[0], mod );
            return;
         case APPLY_EXT_AFFECT:
            REMOVE_AFFECTED( ch, mod );
            return;
         case APPLY_RESISTANT:
            REMOVE_RESIS( ch, mod );
            return;
         case APPLY_IMMUNE:
            REMOVE_IMMUNE( ch, mod );
            return;
         case APPLY_SUSCEPTIBLE:
            REMOVE_SUSCEP( ch, mod );
            return;
         case APPLY_ABSORB:
            REMOVE_ABSORB( ch, mod );
            return;
         case APPLY_REMOVE:
            SET_BIT( ch->affected_by.bits[0], mod );
            return;
         default:
            break;
      }
      mod = 0 - mod;
   }

   switch ( paf->location % REVERSE_APPLY )
   {
      default:
         bug( "Affect_modify: unknown location %d.", paf->location );
         return;

      case APPLY_NONE:
         break;
      case APPLY_STR:
         ch->mod_str += mod;
         break;
      case APPLY_DEX:
         ch->mod_dex += mod;
         break;
      case APPLY_INT:
         ch->mod_int += mod;
         break;
      case APPLY_WIS:
         ch->mod_wis += mod;
         break;
      case APPLY_CON:
         ch->mod_con += mod;
         break;
      case APPLY_CHA:
         ch->mod_cha += mod;
         break;
      case APPLY_LCK:
         ch->mod_lck += mod;
         break;
      case APPLY_ALLSTATS:
         ch->mod_str += mod;
         ch->mod_dex += mod;
         ch->mod_int += mod;
         ch->mod_wis += mod;
         ch->mod_con += mod;
         ch->mod_cha += mod;
         ch->mod_lck += mod;
         break;
      case APPLY_SEX:
         ch->sex = ( ch->sex + mod ) % 3;
         if( ch->sex < 0 )
            ch->sex += 2;
         ch->sex = URANGE( 0, ch->sex, 2 );
         break;
      case APPLY_ATTACKS:
         ch->numattacks += mod;
         break;

      case APPLY_CLASS:
         break;
      case APPLY_LEVEL:
         break;
      case APPLY_GOLD:
         break;
      case APPLY_EXP:
         break;
      case APPLY_SF:
         ch->spellfail += mod;
         break;
      case APPLY_RACE:
         ch->race += mod;
         break;
      case APPLY_AGE:
         if( !IS_NPC( ch ) )
            ch->pcdata->age_bonus += mod;
         break;
      case APPLY_HEIGHT:
         ch->height += mod;
         break;
      case APPLY_WEIGHT:
         ch->weight += mod;
         break;
      case APPLY_MANA:
         ch->max_mana += mod;
         break;
      case APPLY_HIT:
         ch->max_hit += mod;
         break;
      case APPLY_MOVE:
         ch->max_move += mod;
         break;
      case APPLY_MANA_REGEN:
         ch->mana_regen += mod;
         break;
      case APPLY_HIT_REGEN:
         ch->hit_regen += mod;
         break;
      case APPLY_MOVE_REGEN:
         ch->move_regen += mod;
         break;
      case APPLY_ANTIMAGIC:
         ch->amp += mod;
         break;
      case APPLY_AC:
         ch->armor += mod;
         break;
      case APPLY_HITROLL:
         ch->hitroll += mod;
         break;
      case APPLY_DAMROLL:
         ch->damroll += mod;
         break;
      case APPLY_HITNDAM:
         ch->hitroll += mod;
         ch->damroll += mod;
         break;
      case APPLY_SAVING_POISON:
         ch->saving_poison_death += mod;
         break;
      case APPLY_SAVING_ROD:
         ch->saving_wand += mod;
         break;
      case APPLY_SAVING_PARA:
         ch->saving_para_petri += mod;
         break;
      case APPLY_SAVING_BREATH:
         ch->saving_breath += mod;
         break;
      case APPLY_SAVING_SPELL:
         ch->saving_spell_staff += mod;
         break;
      case APPLY_SAVING_ALL:
         ch->saving_poison_death += mod;
         ch->saving_para_petri += mod;
         ch->saving_breath += mod;
         ch->saving_spell_staff += mod;
         ch->saving_wand += mod;
         break;
      case APPLY_AFFECT:
         SET_BIT( ch->affected_by.bits[0], mod );
         break;
      case APPLY_EXT_AFFECT:
         SET_AFFECTED( ch, mod );
         break;
      case APPLY_RESISTANT:
         SET_RESIS( ch, mod );
         break;
      case APPLY_IMMUNE:
         SET_IMMUNE( ch, mod );
         break;
      case APPLY_SUSCEPTIBLE:
         SET_SUSCEP( ch, mod );
         break;
      case APPLY_ABSORB:
         SET_ABSORB( ch, mod );
         break;
      case APPLY_WEAPONSPELL:   /* see fight.c */
         break;
      case APPLY_REMOVE:
         REMOVE_BIT( ch->affected_by.bits[0], mod );
         break;

      case APPLY_FULL:
         if( !IS_NPC( ch ) )
            ch->pcdata->condition[COND_FULL] = URANGE( 0, ch->pcdata->condition[COND_FULL] + mod, sysdata.maxcondval );
         break;

      case APPLY_THIRST:
         if( !IS_NPC( ch ) )
            ch->pcdata->condition[COND_THIRST] = URANGE( 0, ch->pcdata->condition[COND_THIRST] + mod, sysdata.maxcondval );
         break;

      case APPLY_DRUNK:
         if( !IS_NPC( ch ) )
            ch->pcdata->condition[COND_DRUNK] = URANGE( 0, ch->pcdata->condition[COND_DRUNK] + mod, sysdata.maxcondval );
         break;

      case APPLY_MENTALSTATE:
         ch->mental_state = URANGE( -100, ch->mental_state + mod, 100 );
         break;

      case APPLY_CONTAGIOUS:
         break;
      case APPLY_ODOR:
         break;
      case APPLY_STRIPSN:
         if( IS_VALID_SN( mod ) )
            affect_strip( ch, mod );
         else
            bug( "affect_modify: APPLY_STRIPSN invalid sn %d", mod );
         break;

         /*
          * spell cast upon wear/removal of an object  -Thoric 
          */
      case APPLY_WEARSPELL:
      case APPLY_REMOVESPELL:
         if( IS_ROOM_FLAG( ch->in_room, ROOM_NO_MAGIC ) || IS_IMMUNE( ch, RIS_MAGIC ) || ( ( paf->location % REVERSE_APPLY ) == APPLY_WEARSPELL && !fAdd ) || ( ( paf->location % REVERSE_APPLY ) == APPLY_REMOVESPELL && !fAdd ) || saving_char == ch   /* so save/quit doesn't trigger */
             || loading_char == ch )   /* so loading doesn't trigger */
            return;

         mod = abs( mod );
         if( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL && skill->type == SKILL_SPELL )
            if( ( retcode = ( *skill->spell_fun ) ( mod, ch->level, ch, ch ) ) == rCHAR_DIED || char_died( ch ) )
               return;
         break;


         /*
          * skill apply types  -Thoric 
          */

      case APPLY_PALM: /* not implemented yet */
         break;
      case APPLY_PEEK:
         modify_skill( ch, gsn_peek, mod, fAdd );
         break;
      case APPLY_TRACK:
         modify_skill( ch, gsn_track, mod, fAdd );
         break;
      case APPLY_HIDE:
         modify_skill( ch, gsn_hide, mod, fAdd );
         break;
      case APPLY_STEAL:
         modify_skill( ch, gsn_steal, mod, fAdd );
         break;
      case APPLY_SNEAK:
         modify_skill( ch, gsn_sneak, mod, fAdd );
         break;
      case APPLY_PICK:
         modify_skill( ch, gsn_pick_lock, mod, fAdd );
         break;
      case APPLY_BACKSTAB:
         modify_skill( ch, gsn_backstab, mod, fAdd );
         break;
         /*
          * case APPLY_DETRAP:  modify_skill(ch, gsn_find_traps,mod, fAdd);  break; 
          */
      case APPLY_DODGE:
         modify_skill( ch, gsn_dodge, mod, fAdd );
         break;
      case APPLY_SCAN:
         break;
      case APPLY_GOUGE:
         modify_skill( ch, gsn_gouge, mod, fAdd );
         break;
      case APPLY_SEARCH:
         modify_skill( ch, gsn_search, mod, fAdd );
         break;
      case APPLY_DIG:
         modify_skill( ch, gsn_dig, mod, fAdd );
         break;
      case APPLY_MOUNT:
         modify_skill( ch, gsn_mount, mod, fAdd );
         break;
      case APPLY_DISARM:
         modify_skill( ch, gsn_disarm, mod, fAdd );
         break;
      case APPLY_KICK:
         modify_skill( ch, gsn_kick, mod, fAdd );
         break;
      case APPLY_PARRY:
         modify_skill( ch, gsn_parry, mod, fAdd );
         break;
      case APPLY_BASH:
         modify_skill( ch, gsn_bash, mod, fAdd );
         break;
      case APPLY_STUN:
         modify_skill( ch, gsn_stun, mod, fAdd );
         break;
      case APPLY_PUNCH:
         modify_skill( ch, gsn_punch, mod, fAdd );
         break;
      case APPLY_CLIMB:
         modify_skill( ch, gsn_climb, mod, fAdd );
         break;
      case APPLY_GRIP:
         modify_skill( ch, gsn_grip, mod, fAdd );
         break;
      case APPLY_SCRIBE:
         modify_skill( ch, gsn_scribe, mod, fAdd );
         break;
      case APPLY_BREW:
         modify_skill( ch, gsn_brew, mod, fAdd );
         break;
      case APPLY_COOK:
         modify_skill( ch, gsn_cook, mod, fAdd );
         break;

      case APPLY_EXTRAGOLD:  /* SHUT THE HELL UP LOCATION 89! */
         if( !IS_NPC( ch ) )
            ch->pcdata->exgold += mod;
         break;

         /*
          *  Applys that dont generate effects on pcs
          */
      case APPLY_EAT_SPELL:
      case APPLY_ALIGN_SLAYER:
      case APPLY_RACE_SLAYER:
         break;

         /*
          * Room apply types
          */
      case APPLY_ROOMFLAG:
      case APPLY_SECTORTYPE:
      case APPLY_ROOMLIGHT:
      case APPLY_TELEVNUM:
         break;
   }

   /*
    * Check for weapon wielding.
    * Guard against recursion (for weapons with affects).
    */
   if( !IS_NPC( ch ) && saving_char != ch
       && ( wield = get_eq_char( ch, WEAR_WIELD ) ) != NULL && get_obj_weight( wield ) > str_app[get_curr_str( ch )].wield )
   {
      static int depth;

      if( depth == 0 )
      {
         depth++;
         act( AT_ACTION, "You are too weak to wield $p any longer.", ch, wield, NULL, TO_CHAR );
         act( AT_ACTION, "$n stops wielding $p.", ch, wield, NULL, TO_ROOM );
         unequip_char( ch, wield );
         depth--;
      }
   }
   return;
}

/*
 * Give an affect to a char.
 */
void affect_to_char( CHAR_DATA * ch, AFFECT_DATA * paf )
{
   AFFECT_DATA *paf_new;

   if( !ch )
   {
      bug( "Affect_to_char(NULL, %d)", paf ? paf->type : 0 );
      return;
   }

   if( !paf )
   {
      bug( "Affect_to_char(%s, NULL)", ch->name );
      return;
   }

   CREATE( paf_new, AFFECT_DATA, 1 );
   LINK( paf_new, ch->first_affect, ch->last_affect, next, prev );
   paf_new->type = paf->type;
   paf_new->duration = paf->duration;
   paf_new->location = paf->location;
   paf_new->modifier = paf->modifier;
   paf_new->bit = paf->bit;
   paf_new->rismod = paf->rismod;

   affect_modify( ch, paf_new, TRUE );
   return;
}

/*
 * Remove an affect from a char.
 */
void affect_remove( CHAR_DATA * ch, AFFECT_DATA * paf )
{
   if( !ch->first_affect )
   {
      bug( "Affect_remove(%s, %d): no affect.", ch->name, paf ? paf->type : 0 );
      return;
   }

   affect_modify( ch, paf, FALSE );

   UNLINK( paf, ch->first_affect, ch->last_affect, next, prev );
   DISPOSE( paf );
   return;
}

/*
 * Strip all affects of a given sn.
 */
void affect_strip( CHAR_DATA * ch, int sn )
{
   AFFECT_DATA *paf;
   AFFECT_DATA *paf_next;

   for( paf = ch->first_affect; paf; paf = paf_next )
   {
      paf_next = paf->next;
      if( paf->type == sn )
         affect_remove( ch, paf );
   }

   return;
}

/*
 * Return TRUE if a char is affected by a spell.
 */
bool is_affected( CHAR_DATA * ch, int sn )
{
   AFFECT_DATA *paf;

   for( paf = ch->first_affect; paf; paf = paf->next )
      if( paf->type == sn )
         return TRUE;

   return FALSE;
}

/*
 * Add or enhance an affect.
 * Limitations put in place by Thoric, they may be high... but at least
 * they're there :)
 */
void affect_join( CHAR_DATA * ch, AFFECT_DATA * paf )
{
   AFFECT_DATA *paf_old;

   for( paf_old = ch->first_affect; paf_old; paf_old = paf_old->next )
      if( paf_old->type == paf->type )
      {
         paf->duration = UMIN( 32500, paf->duration + paf_old->duration );
         if( paf->modifier )
            paf->modifier = UMIN( 5000, paf->modifier + paf_old->modifier );
         else
            paf->modifier = paf_old->modifier;
         affect_remove( ch, paf_old );
         break;
      }

   affect_to_char( ch, paf );
   return;
}

/*
 * Apply only affected and RIS on a char
 */
void aris_affect( CHAR_DATA * ch, AFFECT_DATA * paf )
{
   /*
    * How the hell have you SmaugDevs gotten away with this for so long! 
    */
   if( paf->bit >= 0 && paf->bit < MAX_AFFECTED_BY )
      SET_AFFECTED( ch, paf->bit );
   switch ( paf->location % REVERSE_APPLY )
   {
      case APPLY_AFFECT:
         SET_BIT( ch->affected_by.bits[0], paf->modifier );
         break;
      case APPLY_RESISTANT:
         xSET_BITS( ch->resistant, paf->rismod );
         break;
      case APPLY_IMMUNE:
         xSET_BITS( ch->immune, paf->rismod );
         break;
      case APPLY_SUSCEPTIBLE:
         xSET_BITS( ch->susceptible, paf->rismod );
         break;
      case APPLY_ABSORB:
         xSET_BITS( ch->absorb, paf->rismod );
         break;
   }
}

/*
 * Update affecteds and RIS for a character in case things get messed.
 * This should only really be used as a quick fix until the cause
 * of the problem can be hunted down. - FB
 * Last modified: June 30, 1997
 *
 * Quick fix?  Looks like a good solution for a lot of problems.
 */

/* Temp mod to bypass immortals so they can keep their mset affects,
 * just a band-aid until we get more time to look at it -- Blodkai */
void update_aris( CHAR_DATA * ch )
{
   AFFECT_DATA *paf;
   OBJ_DATA *obj;
   int hiding;

   if( IS_IMMORTAL( ch ) )
      return;

   /*
    * So chars using hide skill will continue to hide 
    */
   hiding = IS_AFFECTED( ch, AFF_HIDE );

   if( !IS_NPC( ch ) )  /* Because we don't want NPCs to be purged of EVERY effect the have */
   {
      xCLEAR_BITS( ch->affected_by );
      xCLEAR_BITS( ch->resistant );
      xCLEAR_BITS( ch->immune );
      xCLEAR_BITS( ch->susceptible );
      xCLEAR_BITS( ch->absorb );
      xCLEAR_BITS( ch->no_affected_by );
      xCLEAR_BITS( ch->no_resistant );
      xCLEAR_BITS( ch->no_immune );
      xCLEAR_BITS( ch->no_susceptible );
   }

   /*
    * Add in effects from race 
    * Because NPCs can have races MUCH higher than this that the table doesn't define yet 
    */
   if( ch->race <= RACE_19 )
   {
      xSET_BITS( ch->affected_by, race_table[ch->race]->affected );
      xSET_BITS( ch->resistant, race_table[ch->race]->resist );
      xSET_BITS( ch->susceptible, race_table[ch->race]->suscept );
   }

   /*
    * Add in effects from Class 
    * Because NPCs can have classes higher than the table allows 
    */
   if( ch->Class <= CLASS_BARD )
   {
      xSET_BITS( ch->affected_by, class_table[ch->Class]->affected );
      xSET_BITS( ch->resistant, class_table[ch->Class]->resist );
      xSET_BITS( ch->susceptible, class_table[ch->Class]->suscept );
   }
   ClassSpecificStuff( ch );   /* Brought over from DOTD code - Samson 4-6-99 */

   /*
    * Add in effects from deities 
    */
   if( !IS_NPC( ch ) && ch->pcdata->deity )
   {
      if( ch->pcdata->favor > ch->pcdata->deity->affectednum )
      {
         if( !xIS_EMPTY( ch->pcdata->deity->affected ) )
            xSET_BITS( ch->affected_by, ch->pcdata->deity->affected );
      }
      if( ch->pcdata->favor > ch->pcdata->deity->affectednum2 )
      {
         if( !xIS_EMPTY( ch->pcdata->deity->affected2 ) )
            xSET_BITS( ch->affected_by, ch->pcdata->deity->affected2 );
      }
      if( ch->pcdata->favor > ch->pcdata->deity->affectednum3 )
      {
         if( !xIS_EMPTY( ch->pcdata->deity->affected3 ) )
            xSET_BITS( ch->affected_by, ch->pcdata->deity->affected3 );
      }
      if( ch->pcdata->favor > ch->pcdata->deity->elementnum )
      {
         if( ch->pcdata->deity->element != 0 )
            SET_RESIS( ch, ch->pcdata->deity->element );
      }
      if( ch->pcdata->favor > ch->pcdata->deity->elementnum2 )
      {
         if( ch->pcdata->deity->element2 != 0 )
            SET_RESIS( ch, ch->pcdata->deity->element2 );
      }
      if( ch->pcdata->favor > ch->pcdata->deity->elementnum3 )
      {
         if( ch->pcdata->deity->element3 != 0 )
            SET_RESIS( ch, ch->pcdata->deity->element3 );
      }
      if( ch->pcdata->favor < ch->pcdata->deity->susceptnum )
      {
         if( ch->pcdata->deity->suscept != 0 )
            SET_SUSCEP( ch, ch->pcdata->deity->suscept );
      }
      if( ch->pcdata->favor < ch->pcdata->deity->susceptnum2 )
      {
         if( ch->pcdata->deity->suscept2 != 0 )
            SET_SUSCEP( ch, ch->pcdata->deity->suscept2 );
      }
      if( ch->pcdata->favor < ch->pcdata->deity->susceptnum3 )
      {
         if( ch->pcdata->deity->suscept3 != 0 )
            SET_SUSCEP( ch, ch->pcdata->deity->suscept3 );
      }
   }

   /*
    * Add in effect from spells 
    */
   for( paf = ch->first_affect; paf; paf = paf->next )
      aris_affect( ch, paf );

   /*
    * Add in effects from equipment 
    */
   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->wear_loc != WEAR_NONE )
      {
         for( paf = obj->first_affect; paf; paf = paf->next )
            aris_affect( ch, paf );

         for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
            aris_affect( ch, paf );
      }
   }

   /*
    * Add in effects from the room 
    */
   if( ch->in_room ) /* non-existant char booboo-fix --TRI */
   {
      for( paf = ch->in_room->first_affect; paf; paf = paf->next )
         aris_affect( ch, paf );
   }

   /*
    * Add in effects for polymorph 
    */
   if( !IS_NPC( ch ) && ch->morph )
   {
      xSET_BITS( ch->affected_by, ch->morph->affected_by );
      xSET_BITS( ch->immune, ch->morph->immune );
      xSET_BITS( ch->resistant, ch->morph->resistant );
      xSET_BITS( ch->susceptible, ch->morph->suscept );
      xSET_BITS( ch->absorb, ch->morph->absorb );
      /*
       * Right now only morphs have no_ things --Shaddai 
       */
      xSET_BITS( ch->no_affected_by, ch->morph->no_affected_by );
      xSET_BITS( ch->no_immune, ch->morph->no_immune );
      xSET_BITS( ch->no_resistant, ch->morph->no_resistant );
      xSET_BITS( ch->no_susceptible, ch->morph->no_suscept );
   }

   /*
    * If they were hiding before, make them hiding again 
    */
   if( hiding )
      SET_AFFECTED( ch, AFF_HIDE );

   return;
}

/*
 * Move a char out of a room.
 */
void char_from_room( CHAR_DATA * ch )
{
   OBJ_DATA *obj;
   AFFECT_DATA *paf;

   if( !ch->in_room )
   {
      bug( "Char_from_room: %s not in a room!", ch->name );
      return;
   }

   if( !IS_NPC( ch ) )
      --ch->in_room->area->nplayer;

   if( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL && obj->item_type == ITEM_LIGHT
       && obj->value[2] != 0 && ch->in_room->light > 0 )
      --ch->in_room->light;

   /*
    * Character's affect on the room
    */
   for( paf = ch->first_affect; paf; paf = paf->next )
      room_affect( ch->in_room, paf, FALSE );

   /*
    * Room's affect on the character
    */
   if( !char_died( ch ) )
   {
      for( paf = ch->in_room->first_affect; paf; paf = paf->next )
         affect_modify( ch, paf, FALSE );

      if( char_died( ch ) )   /* could die from removespell, etc */
         return;
   }

   UNLINK( ch, ch->in_room->first_person, ch->in_room->last_person, next_in_room, prev_in_room );

   ch->was_in_room = ch->in_room;
   ch->in_room = NULL;
   ch->next_in_room = NULL;
   ch->prev_in_room = NULL;

   if( !IS_NPC( ch ) && get_timer( ch, TIMER_SHOVEDRAG ) > 0 )
      remove_timer( ch, TIMER_SHOVEDRAG );

   return;
}

/*
 * Move a char into a room.
 */
bool char_to_room( CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex )
{
   OBJ_DATA *obj;
   AFFECT_DATA *paf;

   if( !ch )
   {
      bug( "%s: NULL ch!", __FUNCTION__ );
      return FALSE;
   }

   if( !pRoomIndex || !get_room_index( pRoomIndex->vnum ) )
   {
      bug( "Char_to_room: %s -> NULL room!  Putting char in limbo (%d)", ch->name, ROOM_VNUM_LIMBO );
      /*
       * This used to just return, but there was a problem with crashing
       * and I saw no reason not to just put the char in limbo.  -Narn
       */
      pRoomIndex = get_room_index( ROOM_VNUM_LIMBO );
      if( !pRoomIndex )
      {
         bug( "FATAL: char_to_room: Limbo room is MISSING! Expect crash! %s:%s, line %d", __FILE__, __FUNCTION__, __LINE__ );
         return FALSE;
      }
   }

   ch->in_room = pRoomIndex;
   if( ch->home_vnum < 1 )
      ch->home_vnum = ch->in_room->vnum;

   /*
    * Yep - you guessed it. Everything that needs to happen to add Zone X to Player Y's list
    * * takes place in this one teeny weeny little function found in afk.c :) 
    */
   update_visits( ch, pRoomIndex );

   LINK( ch, pRoomIndex->first_person, pRoomIndex->last_person, next_in_room, prev_in_room );

   if( !IS_NPC( ch ) )
      ++pRoomIndex->area->nplayer;

   if( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL && obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
      ++pRoomIndex->light;

   /*
    * Room's effect on the character
    */
   if( !char_died( ch ) )
   {
      for( paf = pRoomIndex->first_affect; paf; paf = paf->next )
         affect_modify( ch, paf, TRUE );

      if( char_died( ch ) )   /* could die from a wearspell, etc */
         return TRUE;
   }

   /*
    * Character's effect on the room
    */
   for( paf = ch->first_affect; paf; paf = paf->next )
      room_affect( pRoomIndex, paf, TRUE );

   if( !IS_NPC( ch ) && IS_ROOM_FLAG( pRoomIndex, ROOM_SAFE ) && get_timer( ch, TIMER_SHOVEDRAG ) <= 0 )
      add_timer( ch, TIMER_SHOVEDRAG, 10, NULL, 0 );
                                                   /*-30 Seconds-*/

   /*
    * Delayed Teleport rooms             -Thoric
    * Should be the last thing checked in this function
    */
   if( IS_ROOM_FLAG( pRoomIndex, ROOM_TELEPORT ) && pRoomIndex->tele_delay > 0 )
   {
      TELEPORT_DATA *tele;

      for( tele = first_teleport; tele; tele = tele->next )
         if( tele->room == pRoomIndex )
            return TRUE;

      CREATE( tele, TELEPORT_DATA, 1 );
      LINK( tele, first_teleport, last_teleport, next, prev );
      tele->room = pRoomIndex;
      tele->timer = pRoomIndex->tele_delay;
   }
   if( !ch->was_in_room )
      ch->was_in_room = ch->in_room;

   if( ch->on )
   {
      ch->on = NULL;
      ch->position = POS_STANDING;
   }
   if( ch->position != POS_STANDING && ch->tempnum != -9998 )  /* Hackish hotboot fix! WOO! */
      ch->position = POS_STANDING;
   return TRUE;
}

/*
 * If possible group obj2 into obj1				-Thoric
 * This code, along with clone_object, obj->count, and special support
 * for it implemented throughout handler.c and save.c should show improved
 * performance on MUDs with players that hoard tons of potions and scrolls
 * as this will allow them to be grouped together both in memory, and in
 * the player files.
 */
OBJ_DATA *group_object( OBJ_DATA * obj1, OBJ_DATA * obj2 )
{
   if( !obj1 || !obj2 )
      return NULL;
   if( obj1 == obj2 )
      return obj1;

   if( obj1->pIndexData->vnum == OBJ_VNUM_TREASURE || obj2->pIndexData->vnum == OBJ_VNUM_TREASURE )
      return obj2;

   if( obj1->pIndexData == obj2->pIndexData
    && ( obj1->name && obj2->name && !str_cmp( obj1->name, obj2->name ) )
    && ( obj1->short_descr && obj2->short_descr && !str_cmp( obj1->short_descr, obj2->short_descr ) )
    && ( obj1->objdesc && obj2->objdesc && !str_cmp( obj1->objdesc, obj2->objdesc ) )
    && ( obj1->action_desc && obj2->action_desc && !str_cmp( obj1->action_desc, obj2->action_desc ) )
    && !str_cmp( obj1->socket[0], obj2->socket[0] )
    && !str_cmp( obj1->socket[1], obj2->socket[1] )
    && !str_cmp( obj1->socket[2], obj2->socket[2] )
    && obj1->item_type == obj2->item_type
    && xSAME_BITS( obj1->extra_flags, obj2->extra_flags )
    && obj1->magic_flags == obj2->magic_flags
    && obj1->wear_flags == obj2->wear_flags
    && obj1->wear_loc == obj2->wear_loc
    && obj1->weight == obj2->weight
    && obj1->cost == obj2->cost
    && obj1->level == obj2->level
    && obj1->timer == obj2->timer
    && obj1->value[0] == obj2->value[0]
    && obj1->value[1] == obj2->value[1]
    && obj1->value[2] == obj2->value[2]
    && obj1->value[3] == obj2->value[3]
    && obj1->value[4] == obj2->value[4]
    && obj1->value[5] == obj2->value[5]
    && obj1->value[6] == obj2->value[6]
    && obj1->value[7] == obj2->value[7]
    && obj1->value[8] == obj2->value[8]
    && obj1->value[9] == obj2->value[9]
    && obj1->value[10] == obj2->value[10]
    && !obj1->first_extradesc && !obj2->first_extradesc
    && !obj1->first_affect && !obj2->first_affect
    && !obj1->first_content && !obj2->first_content
    && obj1->count + obj2->count > 0
    && obj1->map == obj2->map
    && obj1->x == obj2->x
    && obj1->y == obj2->y
    && !str_cmp( obj1->seller, obj2->seller )
    && !str_cmp( obj1->buyer, obj2->buyer )
    && obj1->bid == obj2->bid )   /* prevent count overflow */
   {
      obj1->count += obj2->count;
      obj1->pIndexData->count += obj2->count;   /* to be decremented in */
      numobjsloaded += obj2->count; /* extract_obj */
      extract_obj( obj2 );
      return obj1;
   }
   return obj2;
}

/*
 * Give an obj to a char.
 */
OBJ_DATA *obj_to_char( OBJ_DATA * obj, CHAR_DATA * ch )
{
   OBJ_DATA *otmp;
   OBJ_DATA *oret = obj;
   bool skipgroup, grouped;
   int oweight = get_obj_weight( obj );
   int onum = get_obj_number( obj );
   int wear_loc = obj->wear_loc;
   EXT_BV extra_flags = obj->extra_flags;

   skipgroup = FALSE;
   grouped = FALSE;

   if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
   {
      if( !IS_IMMORTAL( ch ) && ( !IS_NPC(ch) || !IS_ACT_FLAG( ch, ACT_PROTOTYPE ) ) )
         return obj_to_room( obj, ch->in_room, ch );
   }

   if( loading_char == ch )
   {
      int x, y;
      for( x = 0; x < MAX_WEAR; x++ )
         for( y = 0; y < MAX_LAYERS; y++ )
            if( IS_NPC( ch ) )
            {
               if( mob_save_equipment[x][y] == obj )
               {
                  skipgroup = TRUE;
                  break;
               }
            }
            else
            {
               if( save_equipment[x][y] == obj )
               {
                  skipgroup = TRUE;
                  break;
               }
            }
   }

   if( IS_NPC( ch ) && ch->pIndexData->pShop )
      skipgroup = TRUE;

   if( !skipgroup )
      for( otmp = ch->first_carrying; otmp; otmp = otmp->next_content )
         if( ( oret = group_object( otmp, obj ) ) == otmp )
         {
            grouped = TRUE;
            break;
         }
   if( !grouped )
   {
      if( !IS_NPC( ch ) || !ch->pIndexData->pShop )
      {
         LINK( obj, ch->first_carrying, ch->last_carrying, next_content, prev_content );
         obj->carried_by = ch;
         obj->in_room = NULL;
         obj->in_obj = NULL;
         if( ch != supermob )
         {
            REMOVE_OBJ_FLAG( obj, ITEM_ONMAP );
            obj->map = -1;
            obj->x = -1;
            obj->y = -1;
         }
      }
      else
      {
         /*
          * If ch is a shopkeeper, add the obj using an insert sort 
          */
         for( otmp = ch->first_carrying; otmp; otmp = otmp->next_content )
         {
            if( obj->level > otmp->level )
            {
               INSERT( obj, otmp, ch->first_carrying, next_content, prev_content );
               break;
            }
            else if( obj->level == otmp->level && !str_cmp( obj->short_descr, otmp->short_descr ) )
            {
               INSERT( obj, otmp, ch->first_carrying, next_content, prev_content );
               break;
            }
         }
         if( !otmp )
            LINK( obj, ch->first_carrying, ch->last_carrying, next_content, prev_content );

         obj->carried_by = ch;
         obj->in_room = NULL;
         obj->in_obj = NULL;
         if( ch != supermob )
         {
            REMOVE_OBJ_FLAG( obj, ITEM_ONMAP );
            obj->map = -1;
            obj->x = -1;
            obj->y = -1;
         }
      }
   }
   if( wear_loc == WEAR_NONE )
   {
      ch->carry_number += onum;
      ch->carry_weight += oweight;
   }
   else if( !xIS_SET( extra_flags, ITEM_MAGIC ) )
      ch->carry_weight += oweight;
   return ( oret ? oret : obj );
}

/*
 * Take an obj from its character.
 */
void obj_from_char( OBJ_DATA * obj )
{
   CHAR_DATA *ch;
   if( ( ch = obj->carried_by ) == NULL )
   {
      bug( "%s", "Obj_from_char: null ch." );
      bug( "Object was vnum %d - %s", obj->pIndexData->vnum, obj->short_descr );
      return;
   }

   if( obj->wear_loc != WEAR_NONE )
      unequip_char( ch, obj );

   /*
    * obj may drop during unequip... 
    */
   if( !obj->carried_by )
      return;

   UNLINK( obj, ch->first_carrying, ch->last_carrying, next_content, prev_content );

   if( IS_OBJ_FLAG( obj, ITEM_COVERING ) && obj->first_content )
      empty_obj( obj, NULL, NULL );

   obj->in_room = NULL;
   obj->carried_by = NULL;
   ch->carry_number -= get_obj_number( obj );
   ch->carry_weight -= get_obj_weight( obj );
   return;
}

/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac( OBJ_DATA * obj, int iWear )
{
   if( obj->item_type != ITEM_ARMOR )
      return 0;

   switch ( iWear )
   {
      case WEAR_BODY:
         return 3 * obj->value[0];
      case WEAR_HEAD:
         return 2 * obj->value[0];
      case WEAR_LEGS:
         return 2 * obj->value[0];
      case WEAR_FEET:
         return obj->value[0];
      case WEAR_HANDS:
         return obj->value[0];
      case WEAR_ARMS:
         return obj->value[0];
      case WEAR_SHIELD:
         return obj->value[0];
      case WEAR_FINGER_L:
         return obj->value[0];
      case WEAR_FINGER_R:
         return obj->value[0];
      case WEAR_NECK_1:
         return obj->value[0];
      case WEAR_NECK_2:
         return obj->value[0];
      case WEAR_ABOUT:
         return 2 * obj->value[0];
      case WEAR_WAIST:
         return obj->value[0];
      case WEAR_WRIST_L:
         return obj->value[0];
      case WEAR_WRIST_R:
         return obj->value[0];
      case WEAR_HOLD:
         return obj->value[0];
      case WEAR_EYES:
         return obj->value[0];
      case WEAR_FACE:
         return obj->value[0];
      case WEAR_BACK:
         return obj->value[0];
      case WEAR_ANKLE_L:
         return obj->value[0];
      case WEAR_ANKLE_R:
         return obj->value[0];
   }

   return 0;
}

/*
 * Find a piece of eq on a character.
 * Will pick the top layer if clothing is layered.		-Thoric
 */
OBJ_DATA *get_eq_char( CHAR_DATA * ch, int iWear )
{
   OBJ_DATA *obj, *maxobj = NULL;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->wear_loc == iWear )
      {
         if( !obj->pIndexData->layers )
            return obj;
         else if( !maxobj || obj->pIndexData->layers > maxobj->pIndexData->layers )
            maxobj = obj;
      }
   return maxobj;
}

/*
 * Equip a char with an obj.
 */
void equip_char( CHAR_DATA * ch, OBJ_DATA * obj, int iWear )
{
   AFFECT_DATA *paf;
   OBJ_DATA *otmp;

   if( ( otmp = get_eq_char( ch, iWear ) ) != NULL && ( !otmp->pIndexData->layers || !obj->pIndexData->layers ) )
   {
      bug( "Equip_char: already equipped (%d).", iWear );
      return;
   }

   if( obj->carried_by != ch )
   {
      bug( "equip_char: obj (%s) not being carried by ch (%s)!", obj->name, ch->name );
      return;
   }

   separate_obj( obj ); /* just in case */
   if( ( IS_OBJ_FLAG( obj, ITEM_ANTI_EVIL ) && IS_EVIL( ch ) )
       || ( IS_OBJ_FLAG( obj, ITEM_ANTI_GOOD ) && IS_GOOD( ch ) )
       || ( IS_OBJ_FLAG( obj, ITEM_ANTI_NEUTRAL ) && IS_NEUTRAL( ch ) ) )
   {
      /*
       * Thanks to Morgenes for the bug fix here!
       */
      if( loading_char != ch )
      {
         act( AT_MAGIC, "You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR );
         act( AT_MAGIC, "$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM );
      }
      if( obj->carried_by )
         obj_from_char( obj );
      obj_to_room( obj, ch->in_room, ch );
      oprog_zap_trigger( ch, obj );
      if( IS_SAVE_FLAG( SV_ZAPDROP ) && !char_died( ch ) )
         save_char_obj( ch );
      return;
   }

   ch->armor -= apply_ac( obj, iWear );
   obj->wear_loc = iWear;

   ch->carry_number -= get_obj_number( obj );
   if( IS_OBJ_FLAG( obj, ITEM_MAGIC ) )
      ch->carry_weight -= get_obj_weight( obj );

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      affect_modify( ch, paf, TRUE );
   for( paf = obj->first_affect; paf; paf = paf->next )
      affect_modify( ch, paf, TRUE );

   if( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room )
      ++ch->in_room->light;

   return;
}

/*
 * Unequip a char with an obj.
 */
void unequip_char( CHAR_DATA * ch, OBJ_DATA * obj )
{
   AFFECT_DATA *paf;

   if( obj->wear_loc == WEAR_NONE )
   {
      bug( "Unequip_char: %s already unequipped.", ch->name );
      return;
   }

   ch->carry_number += get_obj_number( obj );
   if( IS_OBJ_FLAG( obj, ITEM_MAGIC ) )
      ch->carry_weight += get_obj_weight( obj );

   ch->armor += apply_ac( obj, obj->wear_loc );
   obj->wear_loc = -1;

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      affect_modify( ch, paf, FALSE );
   if( obj->carried_by )
      for( paf = obj->first_affect; paf; paf = paf->next )
         affect_modify( ch, paf, FALSE );

   update_aris( ch );

   if( !obj->carried_by )
      return;

   if( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room && ch->in_room->light > 0 )
      --ch->in_room->light;

   return;
}

/*
 * Move an obj out of a room.
 */
void obj_from_room( OBJ_DATA * obj )
{
   ROOM_INDEX_DATA *in_room;
   AFFECT_DATA *paf;

   if( ( in_room = obj->in_room ) == NULL )
   {
      bug( "obj_from_room: %s not in a room!", obj->name );
      return;
   }

   /*
    * Should handle all cases of picking stuff up from maps - Samson 
    */
   REMOVE_OBJ_FLAG( obj, ITEM_ONMAP );
   obj->x = -1;
   obj->y = -1;
   obj->map = -1;

   for( paf = obj->first_affect; paf; paf = paf->next )
      room_affect( in_room, paf, FALSE );

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      room_affect( in_room, paf, FALSE );

   UNLINK( obj, in_room->first_content, in_room->last_content, next_content, prev_content );

   /*
    * uncover contents 
    */
   if( IS_OBJ_FLAG( obj, ITEM_COVERING ) && obj->first_content )
      empty_obj( obj, NULL, obj->in_room );

   if( obj->item_type == ITEM_FIRE )
      obj->in_room->light -= obj->count;

   obj->carried_by = NULL;
   obj->in_obj = NULL;
   obj->in_room = NULL;
   if( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && falling < 1 )
      write_corpses( NULL, obj->short_descr + 14, obj );
   return;
}

/*
 * Move an obj into a room.
 */
OBJ_DATA *obj_to_room( OBJ_DATA * obj, ROOM_INDEX_DATA * pRoomIndex, CHAR_DATA * ch )
{
   OBJ_DATA *otmp, *oret;
   short count = obj->count;
   short item_type = obj->item_type;
   AFFECT_DATA *paf;

   for( paf = obj->first_affect; paf; paf = paf->next )
      room_affect( pRoomIndex, paf, TRUE );

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      room_affect( pRoomIndex, paf, TRUE );

   for( otmp = pRoomIndex->first_content; otmp; otmp = otmp->next_content )
      if( ( oret = group_object( otmp, obj ) ) == otmp )
      {
         if( item_type == ITEM_FIRE )
            pRoomIndex->light += count;
         return oret;
      }

   LINK( obj, pRoomIndex->first_content, pRoomIndex->last_content, next_content, prev_content );
   obj->in_room = pRoomIndex;
   obj->carried_by = NULL;
   obj->in_obj = NULL;
   obj->room_vnum = pRoomIndex->vnum;  /* hotboot tracker */
   if( item_type == ITEM_FIRE )
      pRoomIndex->light += count;
   falling++;
   obj_fall( obj, FALSE );
   falling--;

   /*
    * Hoping that this will cover all instances of objects from character to room - Samson 8-22-99 
    */
   if( ch != NULL )
   {
      if( IS_ACT_FLAG( ch, ACT_ONMAP ) || IS_PLR_FLAG( ch, PLR_ONMAP ) )
      {
         SET_OBJ_FLAG( obj, ITEM_ONMAP );
         obj->map = ch->map;
         obj->x = ch->x;
         obj->y = ch->y;
      }
      else
      {
         REMOVE_OBJ_FLAG( obj, ITEM_ONMAP );
         obj->map = -1;
         obj->x = -1;
         obj->y = -1;
      }
   }

   if( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && falling < 1 )
      write_corpses( NULL, obj->short_descr + 14, NULL );
   return obj;
}

/*
 * Who's carrying an item -- recursive for nested objects	-Thoric
 */
CHAR_DATA *carried_by( OBJ_DATA * obj )
{
   if( obj->in_obj )
      return carried_by( obj->in_obj );

   return obj->carried_by;
}

/*
 * Return TRUE if an object is, or nested inside a magic container
 */
bool in_magic_container( OBJ_DATA * obj )
{
   if( obj->item_type == ITEM_CONTAINER && IS_OBJ_FLAG( obj, ITEM_MAGIC ) )
      return TRUE;
   if( obj->in_obj )
      return in_magic_container( obj->in_obj );
   return FALSE;
}

/*
 * Move an object into an object.
 */
OBJ_DATA *obj_to_obj( OBJ_DATA * obj, OBJ_DATA * obj_to )
{
   OBJ_DATA *otmp, *oret;
   CHAR_DATA *who;

   if( obj == obj_to )
   {
      bug( "Obj_to_obj: trying to put object inside itself: vnum %d", obj->pIndexData->vnum );
      return obj;
   }

   if( !in_magic_container( obj_to ) && ( who = carried_by( obj_to ) ) != NULL )
      who->carry_weight += get_obj_weight( obj );

   for( otmp = obj_to->first_content; otmp; otmp = otmp->next_content )
      if( ( oret = group_object( otmp, obj ) ) == otmp )
         return oret;

   LINK( obj, obj_to->first_content, obj_to->last_content, next_content, prev_content );

   obj->in_obj = obj_to;
   obj->in_room = NULL;
   obj->carried_by = NULL;

   return obj;
}

/*
 * Move an object out of an object.
 */
void obj_from_obj( OBJ_DATA * obj )
{
   OBJ_DATA *obj_from;
   bool magic;

   if( !( obj_from = obj->in_obj ) )
   {
      bug( "Obj_from_obj: %s null obj_from.", obj->name );
      return;
   }

   magic = in_magic_container( obj_from );

   UNLINK( obj, obj_from->first_content, obj_from->last_content, next_content, prev_content );

   /*
    * uncover contents 
    */
   if( IS_OBJ_FLAG( obj, ITEM_COVERING ) && obj->first_content )
      empty_obj( obj, obj->in_obj, NULL );

   obj->in_obj = NULL;
   obj->in_room = NULL;
   obj->carried_by = NULL;

   /*
    * This will hopefully cover all objs coming from containers going to the maps - Samson 8-22-99 
    */
   if( IS_OBJ_FLAG( obj_from, ITEM_ONMAP ) )
   {
      SET_OBJ_FLAG( obj, ITEM_ONMAP );
      obj->map = obj_from->map;
      obj->x = obj_from->x;
      obj->y = obj_from->y;
   }
   if( !magic )
      for( ; obj_from; obj_from = obj_from->in_obj )
         if( obj_from->carried_by )
            obj_from->carried_by->carry_weight -= get_obj_weight( obj );
   return;
}

/*
 * Stick obj onto extraction queue
 */
void queue_extracted_obj( OBJ_DATA * obj )
{
   ++cur_qobjs;
   obj->next = extracted_obj_queue;
   extracted_obj_queue = obj;
}

/*
 * Extract an obj from the world.
 */
void extract_obj( OBJ_DATA * obj )
{
   OBJ_DATA *obj_content;

   if( obj_extracted( obj ) )
   {
      bug( "%s: obj %d already extracted!", __FUNCTION__, obj->pIndexData->vnum );
      /*
       * return; Seeing if we can get it to either extract it for real, or die trying! 
       */
   }

   if( obj->item_type == ITEM_PORTAL )
      remove_portal( obj );

   if( auction->item && auction->item == obj )
      interpret( supermob, "bid stop" );

   if( obj->carried_by )
      obj_from_char( obj );
   else if( obj->in_room )
      obj_from_room( obj );
   else if( obj->in_obj )
      obj_from_obj( obj );

   while( ( obj_content = obj->last_content ) != NULL )
      extract_obj( obj_content );

   if( obj == gobj_prev )
      gobj_prev = obj->prev;

   UNLINK( obj, first_object, last_object, next, prev );

   /*
    * shove onto extraction queue 
    */
   queue_extracted_obj( obj );

   obj->pIndexData->count -= obj->count;
   numobjsloaded -= obj->count;
   --physicalobjects;
   return;
}

/* Automatic corpse retrieval for < 10 characters.
 * Adapted from the Undertaker snippet by Cyrus and Robcon (Rage of Carnage 2).
 */
void retrieve_corpse( CHAR_DATA * ch, CHAR_DATA * healer )
{
   char buf[MSL];
   OBJ_DATA *obj, *outer_obj;
   bool found = FALSE;

   /*
    * Avoids the potential for filling the room with hundreds of mob corpses 
    */
   if( IS_NPC( ch ) )
      return;

   mudstrlcpy( buf, "the corpse of ", MSL );
   mudstrlcat( buf, ch->name, MSL );
   for( obj = first_object; obj; obj = obj->next )
   {
      if( !nifty_is_name( buf, obj->short_descr ) )
         continue;

      /*
       * This will prevent NPC corpses from being retreived if the person has a mob's name 
       */
      if( obj->item_type == ITEM_CORPSE_NPC )
         continue;

      found = TRUE;

      /*
       * Could be carried by act_scavengers, or other idiots so ... 
       */
      outer_obj = obj;
      while( outer_obj->in_obj )
         outer_obj = outer_obj->in_obj;

      separate_obj( outer_obj );
      obj_from_room( outer_obj );
      obj_to_room( outer_obj, ch->in_room, ch );

      if( healer )
      {
         act( AT_MAGIC, "$n closes $s eyes in deep prayer....", healer, NULL, NULL, TO_ROOM );
         act( AT_MAGIC, "A moment later $T appears in the room!", healer, NULL, buf, TO_ROOM );
      }
      else
      {
         act( AT_MAGIC, "From out of nowhere, $T appears in a bright flash!", ch, NULL, buf, TO_ROOM );
      }

      if( ch->level > 7 )
      {
         send_to_char( "&RReminder: Automatic corpse retrieval ceases once you've reached level 10.\n\r", ch );
         send_to_char( "Upon reaching level 10 it is strongly advised that you choose a deity to devote to!\n\r", ch );
      }
   }

   if( !found )
   {
      send_to_char( "&RThere is no corpse to retrieve. Perhaps you've fallen victim to a Death Trap?\n\r", ch );
      if( ch->level > 7 )
      {
         send_to_char( "&RReminder: Automatic corpse retrieval ceases once you've reached level 10.\n\r", ch );
         send_to_char( "Upon reaching level 10 it is strongly advised that you choose a deity to devote to!\n\r", ch );
      }
   }

   return;
}

/* Check a char for ITEM_MUSTMOUNT eq and remove it - Samson 3-18-01 */
void check_mount_objs( CHAR_DATA * ch, bool fell )
{
   OBJ_DATA *obj, *obj_next;

   for( obj = ch->first_carrying; obj; obj = obj_next )
   {
      obj_next = obj->next_content;
      if( obj->wear_loc == WEAR_NONE )
         continue;

      if( !IS_OBJ_FLAG( obj, ITEM_MUSTMOUNT ) )
         continue;

      if( fell )
      {
         act( AT_ACTION, "As you fall, $p drops to the ground.", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n drops $p as $e falls to the ground.", ch, obj, NULL, TO_ROOM );

         obj_from_char( obj );
         obj = obj_to_room( obj, ch->in_room, ch );
         oprog_drop_trigger( ch, obj );   /* mudprogs */
         if( char_died( ch ) )
            return;
      }
      else
      {
         act( AT_ACTION, "As you dismount, you remove $p.", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n removes $p as $e dismounts.", ch, obj, NULL, TO_ROOM );
         unequip_char( ch, obj );
      }
   }
   return;
}

/*
 * Add ch to the queue of recently extracted characters		-Thoric
 */
void queue_extracted_char( CHAR_DATA * ch, bool extract )
{
   EXTRACT_CHAR_DATA *ccd;

   if( !ch )
   {
      bug( "%s", "queue_extracted char: ch = NULL" );
      return;
   }
   CREATE( ccd, EXTRACT_CHAR_DATA, 1 );
   ccd->ch = ch;
   ccd->room = ch->in_room;
   ccd->extract = extract;
   ccd->retcode = rCHAR_DIED;
   ccd->next = extracted_char_queue;
   extracted_char_queue = ccd;
   cur_qchars++;
}

/*
 * Extract a char from the world.
 */
/* Function modified from original form - Samson 3-29-98 */
/* If something has gone awry with your *ch pointers, there's a fairly good chance this thing will trip over it and
 * bring things crashing down around you. Which is why there are so many bug log points.
 */
void extract_char( CHAR_DATA * ch, bool fPull )
{
   CHAR_DATA *wch;
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *location, *dieroom;   /* Added for checking where to send you at death - Samson */
   REL_DATA *RQueue, *rq_next;

   if( !ch )
   {
      bug( "%s: NULL ch.", __FUNCTION__ );
      return;
   }

   if( !ch->in_room )
   {
      bug( "%s: %s in NULL room. Transferring to Limbo.", __FUNCTION__, ch->name ? ch->name : "???" );
      if( !char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   if( ch == supermob && !mud_down )
   {
      bug( "%s: ch == supermob!", __FUNCTION__ );
      return;
   }

   if( char_died( ch ) )
   {
      bug( "%s: %s already died!", __FUNCTION__, ch->name );
      /*
       * return; This return is commented out in the hops of allowing the dead mob to be extracted anyway 
       */
   }

   /*
    * shove onto extraction queue 
    */
   queue_extracted_char( ch, fPull );

   for( RQueue = first_relation; RQueue; RQueue = rq_next )
   {
      rq_next = RQueue->next;
      if( fPull && RQueue->Type == relMSET_ON )
      {
         if( ch == RQueue->Subject )
            ( ( CHAR_DATA * ) RQueue->Actor )->pcdata->dest_buf = NULL;
         else if( ch != RQueue->Actor )
            continue;
         UNLINK( RQueue, first_relation, last_relation, next, prev );
         DISPOSE( RQueue );
      }
   }

   if( gch_prev == ch )
      gch_prev = ch->prev;

   if( fPull && !mud_down )
      die_follower( ch );

   if( !mud_down )
      stop_fighting( ch, TRUE );

   if( ch->mount )
   {
      REMOVE_ACT_FLAG( ch->mount, ACT_MOUNTED );
      ch->mount = NULL;
      ch->position = POS_STANDING;
   }

   /*
    * check if this NPC was a mount or a pet
    */
   if( IS_NPC( ch ) && !mud_down )
   {
      REMOVE_ACT_FLAG( ch, ACT_MOUNTED );
      for( wch = first_char; wch; wch = wch->next )
      {
         if( wch->mount == ch )
         {
            wch->mount = NULL;
            wch->position = POS_SITTING;
            if( wch->in_room == ch->in_room )
            {
               act( AT_SOCIAL, "Your faithful mount, $N collapses beneath you...", wch, NULL, ch, TO_CHAR );
               act( AT_SOCIAL, "You hit the ground with a thud.", wch, NULL, NULL, TO_CHAR );
               act( AT_PLAIN, "$n falls from $N as $N is slain.", wch, NULL, ch, TO_ROOM );
               check_mount_objs( ch, TRUE ); /* Check to see if they have ITEM_MUSTMOUNT stuff */
            }
         }

         if( !IS_NPC( wch ) && wch->first_pet )
         {
            if( ch->master == wch )
            {
               unbind_follower( ch, wch );
               if( wch->in_room == ch->in_room )
                  act( AT_SOCIAL, "You mourn for the loss of $N.", wch, NULL, ch, TO_CHAR );
            }
         }
      }
   }

   /*
    * Bug fix loop to stop PCs from being hunted after death - Samson 8-22-99 
    */
   if( !mud_down )
   {
      for( wch = first_char; wch; wch = wch->next )
      {
         if( IS_NPC( wch ) && wch->hating )
         {
            if( wch->hating->who == ch )
               stop_hating( wch );
         }

         if( IS_NPC( wch ) && wch->fearing )
         {
            if( wch->fearing->who == ch )
               stop_fearing( wch );
         }

         if( IS_NPC( wch ) && wch->hunting )
         {
            if( wch->hunting->who == ch )
               stop_hunting( wch );
         }
      }
   }

   while( ( obj = ch->last_carrying ) != NULL )
   {
      if( obj->rent >= sysdata.minrent && IS_ROOM_FLAG( ch->in_room, ROOM_DEATH ) )
         obj->pIndexData->count += obj->count;
      extract_obj( obj );
   }

   dieroom = ch->in_room;
   char_from_room( ch );

   if( !fPull )
   {
      location = check_room( ch, dieroom );  /* added check to see what continent PC is on - Samson 3-29-98 */

      if( !location )
         location = get_room_index( ROOM_VNUM_TEMPLE );

      if( !location )
         location = get_room_index( ROOM_VNUM_LIMBO );

      if( !char_to_room( ch, location ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      if( IS_PLR_FLAG( ch, PLR_ONMAP ) )
      {
         REMOVE_PLR_FLAG( ch, PLR_ONMAP );
         REMOVE_PLR_FLAG( ch, PLR_MAPEDIT ); /* Just in case they were editing */

         ch->x = -1;
         ch->y = -1;
         ch->map = -1;
      }

      /*
       * Make things a little fancier           -Thoric
       */
      if( ( wch = get_char_room( ch, "healer" ) ) != NULL )
      {
         act( AT_MAGIC, "$n mutters a few incantations, waves $s hands and points $s finger.", wch, NULL, NULL, TO_ROOM );
         act( AT_MAGIC, "$n appears from some strange swirling mists!", ch, NULL, NULL, TO_ROOM );
         act_printf( AT_MAGIC, wch, NULL, NULL, TO_ROOM,
                     "$n says 'Welcome back to the land of the living, %s.'", capitalize( ch->name ) );
         if( ch->level < 10 )
         {
            retrieve_corpse( ch, wch );
            collect_followers( ch, dieroom, location );
         }
      }
      else
      {
         act( AT_MAGIC, "$n appears from some strange swirling mists!", ch, NULL, NULL, TO_ROOM );
         if( ch->level < 10 )
         {
            retrieve_corpse( ch, NULL );
            collect_followers( ch, dieroom, location );
         }
      }

      ch->position = POS_RESTING;
      return;
   }

   if( IS_NPC( ch ) )
   {
      --ch->pIndexData->count;
      --nummobsloaded;
   }

   if( ch->desc && ch->desc->original )
      interpret( ch, "return" );

   if( ch->switched && ch->switched->desc )
      interpret( ch->switched, "return" );

   if( !mud_down )
   {
      for( wch = first_char; wch; wch = wch->next )
      {
         if( wch->reply == ch )
            wch->reply = NULL;
      }
   }

   UNLINK( ch, first_char, last_char, next, prev );

   if( ch->desc )
   {
      if( ch->desc->character != ch )
         bug( "Extract_char: %s's descriptor points to another char", ch->name );
      else
      {
         ch->desc->character = NULL;
         close_socket( ch->desc, FALSE );
         ch->desc = NULL;
      }
   }
   return;
}

/*
 * Find a char in the room.
 */
CHAR_DATA *get_char_room( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *rch;
   int number, count, vnum;

   number = number_argument( argument, arg );

   if( !str_cmp( arg, "self" ) )
      return ch;

   if( get_trust( ch ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   count = 0;

   for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
      if( can_see( ch, rch, FALSE ) && !is_ignoring( rch, ch ) && ( nifty_is_name( arg, rch->name )
                                                                    || ( IS_NPC( rch ) && vnum == rch->pIndexData->vnum ) ) )
      {
         if( number == 0 && !IS_NPC( rch ) )
            return rch;
         else if( ++count == number )
            return rch;
      }

   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of characters
    * again looking for prefix matching, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
   {
      if( !can_see( ch, rch, FALSE ) || !nifty_is_name_prefix( arg, rch->name ) || is_ignoring( rch, ch ) )
         continue;

      if( number == 0 && !IS_NPC( rch ) )
         return rch;
      else if( ++count == number )
         return rch;
   }

   return NULL;
}

/*
 * Find a char in the world.
 */
CHAR_DATA *get_char_world( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *wch;
   int number, count, vnum;

   number = number_argument( argument, arg );
   count = 0;
   if( !str_cmp( arg, "self" ) )
      return ch;

   /*
    * Allow reference by vnum for saints+         -Thoric
    */
   if( get_trust( ch ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   /*
    * check the room for an exact match 
    */
   for( wch = ch->in_room->first_person; wch; wch = wch->next_in_room )
      if( can_see( ch, wch, TRUE ) && !is_ignoring( wch, ch ) && ( nifty_is_name( arg, wch->name )
                                                                   || ( IS_NPC( wch ) && vnum == wch->pIndexData->vnum ) ) )
      {
         if( number == 0 && !IS_NPC( wch ) )
            return wch;
         else if( ++count == number )
            return wch;
      }

   count = 0;

   /*
    * check the world for an exact match 
    */
   for( wch = first_char; wch; wch = wch->next )
      if( can_see( ch, wch, TRUE ) && !is_ignoring( wch, ch )
          && ( nifty_is_name( arg, wch->name ) || ( IS_NPC( wch ) && vnum == wch->pIndexData->vnum ) ) )
      {
         if( number == 0 && !IS_NPC( wch ) )
            return wch;
         else if( ++count == number )
            return wch;
      }

   /*
    * bail out if looking for a vnum match 
    */
   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, check the room for
    * for a prefix match, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( wch = ch->in_room->first_person; wch; wch = wch->next_in_room )
   {
      if( !can_see( ch, wch, TRUE ) || !nifty_is_name_prefix( arg, wch->name ) || is_ignoring( wch, ch ) )
         continue;
      if( number == 0 && !IS_NPC( wch ) )
         return wch;
      else if( ++count == number )
         return wch;
   }

   /*
    * If we didn't find a prefix match in the room, run through the full list
    * of characters looking for prefix matching, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( wch = first_char; wch; wch = wch->next )
   {
      if( !can_see( ch, wch, TRUE ) || !nifty_is_name_prefix( arg, wch->name ) || is_ignoring( wch, ch ) )
         continue;
      if( number == 0 && !IS_NPC( wch ) )
         return wch;
      else if( ++count == number )
         return wch;
   }

   return NULL;
}

/*
 * Find an obj in a list.
 */
OBJ_DATA *get_obj_list( CHAR_DATA * ch, char *argument, OBJ_DATA * list )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int number;
   int count;

   number = number_argument( argument, arg );
   count = 0;
   for( obj = list; obj; obj = obj->next_content )
      if( can_see_obj( ch, obj, FALSE ) && nifty_is_name( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( obj = list; obj; obj = obj->next_content )
      if( can_see_obj( ch, obj, FALSE ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;

   return NULL;
}

/*
 * Find an obj in a list...going the other way			-Thoric
 */
OBJ_DATA *get_obj_list_rev( CHAR_DATA * ch, char *argument, OBJ_DATA * list )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int number;
   int count;

   number = number_argument( argument, arg );
   count = 0;
   for( obj = list; obj; obj = obj->prev_content )
      if( can_see_obj( ch, obj, FALSE ) && nifty_is_name( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( obj = list; obj; obj = obj->prev_content )
      if( can_see_obj( ch, obj, FALSE ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;

   return NULL;
}

/*
 * Find an obj in player's inventory or wearing via a vnum -Shaddai
 */
OBJ_DATA *get_obj_vnum( CHAR_DATA * ch, int vnum )
{
   OBJ_DATA *obj;

   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      if( can_see_obj( ch, obj, FALSE ) && obj->pIndexData->vnum == vnum )
         return obj;
   return NULL;
}

/*
 * Find an obj in player's inventory.
 */
OBJ_DATA *get_obj_carry( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int number, count, vnum;

   number = number_argument( argument, arg );
   if( get_trust( ch ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   count = 0;
   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj, FALSE )
          && ( nifty_is_name( arg, obj->name ) || obj->pIndexData->vnum == vnum ) )
         if( ( count += obj->count ) >= number )
            return obj;

   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj, FALSE ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;

   return NULL;
}

/*
 * Find an obj in player's equipment.
 */
OBJ_DATA *get_obj_wear( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int number, count, vnum;

   number = number_argument( argument, arg );

   if( get_trust( ch ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   count = 0;
   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      if( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj, FALSE )
          && ( nifty_is_name( arg, obj->name ) || obj->pIndexData->vnum == vnum ) )
         if( ++count == number )
            return obj;

   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      if( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj, FALSE ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ++count == number )
            return obj;

   return NULL;
}

/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA *get_obj_here( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;

   obj = get_obj_list_rev( ch, argument, ch->in_room->last_content );
   if( obj )
      return obj;

   if( ( obj = get_obj_carry( ch, argument ) ) != NULL )
      return obj;

   if( ( obj = get_obj_wear( ch, argument ) ) != NULL )
      return obj;

   return NULL;
}

/*
 * Find an obj in the world.
 */
OBJ_DATA *get_obj_world( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int number, count, vnum;

   if( ( obj = get_obj_here( ch, argument ) ) != NULL )
      return obj;

   number = number_argument( argument, arg );

   /*
    * Allow reference by vnum for saints+         -Thoric
    */
   if( get_trust( ch ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   count = 0;
   for( obj = first_object; obj; obj = obj->next )
      if( can_see_obj( ch, obj, TRUE ) && ( nifty_is_name( arg, obj->name ) || vnum == obj->pIndexData->vnum ) )
         if( ( count += obj->count ) >= number )
            return obj;

   /*
    * bail out if looking for a vnum 
    */
   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( obj = first_object; obj; obj = obj->next )
      if( can_see_obj( ch, obj, TRUE ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;

   return NULL;
}

/*
 * How mental state could affect finding an object		-Thoric
 * Used by get/drop/put/quaff/recite/etc
 * Increasingly freaky based on mental state and drunkeness
 */
bool ms_find_obj( CHAR_DATA * ch )
{
   int ms = ch->mental_state;
   int drunk = IS_NPC( ch ) ? 0 : ch->pcdata->condition[COND_DRUNK];
   char *t;

   /*
    * we're going to be nice and let nothing weird happen unless
    * you're a tad messed up
    */
   drunk = UMAX( 1, drunk );
   if( abs( ms ) + ( drunk / 3 ) < 30 )
      return FALSE;
   if( ( number_percent(  ) + ( ms < 0 ? 15 : 5 ) ) > abs( ms ) / 2 + drunk / 4 )
      return FALSE;
   if( ms > 15 )  /* range 1 to 20 -- feel free to add more */
      switch ( number_range( UMAX( 1, ( ms / 5 - 15 ) ), ( ms + 4 ) / 5 ) )
      {
         default:
         case 1:
            t = "As you reach for it, you forgot what it was...\n\r";
            break;
         case 2:
            t = "As you reach for it, something inside stops you...\n\r";
            break;
         case 3:
            t = "As you reach for it, it seems to move out of the way...\n\r";
            break;
         case 4:
            t = "You grab frantically for it, but can't seem to get a hold of it...\n\r";
            break;
         case 5:
            t = "It disappears as soon as you touch it!\n\r";
            break;
         case 6:
            t = "You would if it would stay still!\n\r";
            break;
         case 7:
            t = "Whoa!  It's covered in blood!  Ack!  Ick!\n\r";
            break;
         case 8:
            t = "Wow... trails!\n\r";
            break;
         case 9:
            t = "You reach for it, then notice the back of your hand is growing something!\n\r";
            break;
         case 10:
            t = "As you grasp it, it shatters into tiny shards which bite into your flesh!\n\r";
            break;
         case 11:
            t = "What about that huge dragon flying over your head?!?!?\n\r";
            break;
         case 12:
            t = "You stratch yourself instead...\n\r";
            break;
         case 13:
            t = "You hold the universe in the palm of your hand!\n\r";
            break;
         case 14:
            t = "You're too scared.\n\r";
            break;
         case 15:
            t = "Your mother smacks your hand... 'NO!'\n\r";
            break;
         case 16:
            t = "Your hand grasps the worst pile of revoltingness that you could ever imagine!\n\r";
            break;
         case 17:
            t = "You stop reaching for it as it screams out at you in pain!\n\r";
            break;
         case 18:
            t = "What about the millions of burrow-maggots feasting on your arm?!?!\n\r";
            break;
         case 19:
            t = "That doesn't matter anymore... you've found the TRUE answer to everything!\n\r";
            break;
         case 20:
            t = "A supreme entity has no need for that.\n\r";
            break;
      }
   else
   {
      int sub = URANGE( 1, abs( ms ) / 2 + drunk, 60 );
      switch ( number_range( 1, sub / 10 ) )
      {
         default:
         case 1:
            t = "In just a second...\n\r";
            break;
         case 2:
            t = "You can't find that...\n\r";
            break;
         case 3:
            t = "It's just beyond your grasp...\n\r";
            break;
         case 4:
            t = "...but it's under a pile of other stuff...\n\r";
            break;
         case 5:
            t = "You go to reach for it, but pick your nose instead.\n\r";
            break;
         case 6:
            t = "Which one?!?  I see two... no three...\n\r";
            break;
      }
   }
   send_to_char( t, ch );
   return TRUE;
}

/*
 * Generic get obj function that supports optional containers.	-Thoric
 * currently only used for "eat" and "quaff".
 */
OBJ_DATA *find_obj( CHAR_DATA * ch, char *argument, bool carryonly )
{
   char arg1[MIL], arg2[MIL];
   OBJ_DATA *obj = NULL;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !str_cmp( arg2, "from" ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( !arg2 || arg2[0] == '\0' )
   {
      if( carryonly && ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
      {
         send_to_char( "You do not have that item.\n\r", ch );
         return NULL;
      }
      else if( !carryonly && ( obj = get_obj_here( ch, arg1 ) ) == NULL )
      {
         act( AT_PLAIN, "I see no $T here.", ch, NULL, arg1, TO_CHAR );
         return NULL;
      }
      return obj;
   }
   else
   {
      OBJ_DATA *container = NULL;

      if( carryonly && ( container = get_obj_carry( ch, arg2 ) ) == NULL
          && ( container = get_obj_wear( ch, arg2 ) ) == NULL )
      {
         send_to_char( "You do not have that item.\n\r", ch );
         return NULL;
      }
      if( !carryonly && ( container = get_obj_here( ch, arg2 ) ) == NULL )
      {
         act( AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR );
         return NULL;
      }
      if( !IS_OBJ_FLAG( container, ITEM_COVERING ) && IS_SET( container->value[1], CONT_CLOSED ) )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR );
         return NULL;
      }
      obj = get_obj_list( ch, arg1, container->first_content );
      if( !obj )
         act( AT_PLAIN, IS_OBJ_FLAG( container, ITEM_COVERING ) ?
              "I see nothing like that beneath $p." : "I see nothing like that in $p.", ch, container, NULL, TO_CHAR );
      return obj;
   }
}

int get_obj_number( OBJ_DATA * obj )
{
   return obj->count;
}

/*
 * Return weight of an object, including weight of contents (unless magic).
 */
int get_obj_weight( OBJ_DATA * obj )
{
   int weight;

   weight = obj->count * obj->weight;

   /*
    * magic containers 
    */
   if( obj->item_type != ITEM_CONTAINER || !IS_OBJ_FLAG( obj, ITEM_MAGIC ) )
      for( obj = obj->first_content; obj; obj = obj->next_content )
         weight += get_obj_weight( obj );

   return weight;
}

/*
 * Return real weight of an object, including weight of contents.
 */
int get_real_obj_weight( OBJ_DATA * obj )
{
   int weight;

   weight = obj->count * obj->weight;

   for( obj = obj->first_content; obj; obj = obj->next_content )
      weight += get_real_obj_weight( obj );

   return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark( ROOM_INDEX_DATA * pRoomIndex, CHAR_DATA * ch )
{
   if( !pRoomIndex )
   {
      char buf[MSL];

      if( IS_NPC( ch ) )
         snprintf( buf, MSL, "Mob #%d", ch->pIndexData->vnum );
      else
         snprintf( buf, MSL, "Player %s", ch->name );

      bug( "room_is_dark: NULL pRoomIndex. Occupant: %s", ch->name );

      if( char_died( ch ) )
         bug( "%s was probably dead when this happened.", buf );

      return TRUE;
   }

   if( pRoomIndex->light > 0 )
      return FALSE;

   if( IS_ROOM_FLAG( pRoomIndex, ROOM_DARK ) )
      return TRUE;

   if( pRoomIndex->sector_type == SECT_INDOORS || pRoomIndex->sector_type == SECT_CITY )
      return FALSE;

   if( time_info.sunlight == SUN_SET || time_info.sunlight == SUN_DARK )
      return TRUE;

   return FALSE;
}

/*
 * True if room is private.
 */
bool room_is_private( ROOM_INDEX_DATA * pRoomIndex )
{
   CHAR_DATA *rch;
   int count;

   if( !pRoomIndex )
   {
      bug( "%s", "room_is_private: NULL pRoomIndex" );
      return FALSE;
   }

   count = 0;
   for( rch = pRoomIndex->first_person; rch; rch = rch->next_in_room )
      count++;

   if( IS_ROOM_FLAG( pRoomIndex, ROOM_PRIVATE ) && count >= 2 )
      return TRUE;

   if( IS_ROOM_FLAG( pRoomIndex, ROOM_SOLITARY ) && count >= 1 )
      return TRUE;

   return FALSE;
}

/*
 * True if char can see victim.
 */
bool can_see( CHAR_DATA * ch, CHAR_DATA * victim, bool override )
{
   if( !victim )  /* Gorog - panicked attempt to stop crashes */
   {
      bug( "can_see: NULL victim! CH %s tried to see it.", ch ? ch->name : "Unknown?" );
      return FALSE;
   }

   if( !ch )
   {
      if( IS_AFFECTED( victim, AFF_INVISIBLE ) || IS_AFFECTED( victim, AFF_HIDE ) || IS_PLR_FLAG( victim, PLR_WIZINVIS ) )
         return FALSE;
      else
         return TRUE;
   }

   if( !IS_IMP( ch ) && victim == supermob )
      return FALSE;

   if( ch == victim )
      return TRUE;

   if( IS_PLR_FLAG( victim, PLR_WIZINVIS ) && ch->level < victim->pcdata->wizinvis )
      return FALSE;

   /*
    * SB 
    */
   if( IS_ACT_FLAG( victim, ACT_MOBINVIS ) && ch->level < victim->mobinvis )
      return FALSE;

   /*
    * Deadlies link-dead over 2 ticks aren't seen by mortals -- Blodkai 
    */
   if( !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && !IS_NPC( victim ) && IS_PKILL( victim ) && victim->timer > 1 && !victim->desc )
      return FALSE;

   if( ( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) ) && override == FALSE )
   {
      if( !is_same_map( ch, victim ) )
         return FALSE;
   }

   /*
    * Unless you want to get spammed to death as an immortal on the map, DONT move this above here!!!! 
    */
   if( IS_PLR_FLAG( ch, PLR_HOLYLIGHT ) )
      return TRUE;

   /*
    * The miracle cure for blindness? -- Altrag 
    */
   if( !IS_AFFECTED( ch, AFF_TRUESIGHT ) )
   {
      if( IS_AFFECTED( ch, AFF_BLIND ) )
         return FALSE;

      if( room_is_dark( ch->in_room, ch ) && !IS_AFFECTED( ch, AFF_INFRARED ) )
         return FALSE;

      if( IS_AFFECTED( victim, AFF_INVISIBLE ) && !IS_AFFECTED( ch, AFF_DETECT_INVIS ) )
         return FALSE;

      if( IS_AFFECTED( victim, AFF_HIDE ) && !IS_AFFECTED( ch, AFF_DETECT_HIDDEN ) && !victim->fighting
          && ( IS_NPC( ch ) ? !IS_NPC( victim ) : IS_NPC( victim ) ) )
         return FALSE;
   }
   return TRUE;
}

/*
 * True if char can see obj.
 * Override boolean to bypass overland coordinate checks - Samson 10-17-03
 */
bool can_see_obj( CHAR_DATA * ch, OBJ_DATA * obj, bool override )
{
   if( IS_OBJ_FLAG( obj, ITEM_ONMAP ) && !override )
   {
      if( ch->map != obj->map || ch->x != obj->x || ch->y != obj->y )
         return FALSE;
   }

   if( IS_PLR_FLAG( ch, PLR_HOLYLIGHT ) )
      return TRUE;

   if( IS_NPC( ch ) && ch->pIndexData->vnum == MOB_VNUM_SUPERMOB )
      return TRUE;

   if( IS_OBJ_FLAG( obj, ITEM_BURIED ) )
      return FALSE;

   if( IS_OBJ_FLAG( obj, ITEM_HIDDEN ) )
      return FALSE;

   if( IS_AFFECTED( ch, AFF_TRUESIGHT ) )
      return TRUE;

   if( IS_AFFECTED( ch, AFF_BLIND ) )
      return FALSE;

   /*
    * can see lights in the dark 
    */
   if( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
      return TRUE;

   if( room_is_dark( ch->in_room, ch ) )
   {
      /*
       * can see glowing items in the dark... invisible or not 
       */
      if( IS_OBJ_FLAG( obj, ITEM_GLOW ) )
         return TRUE;
      if( !IS_AFFECTED( ch, AFF_INFRARED ) )
         return FALSE;
   }

   if( IS_OBJ_FLAG( obj, ITEM_INVIS ) && !IS_AFFECTED( ch, AFF_DETECT_INVIS ) )
      return FALSE;

   return TRUE;
}

/*
 * True if char can drop obj.
 */
bool can_drop_obj( CHAR_DATA * ch, OBJ_DATA * obj )
{
   if( !IS_OBJ_FLAG( obj, ITEM_NODROP ) )
      return TRUE;

   if( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
      return TRUE;

   if( IS_NPC( ch ) && ch->pIndexData->vnum == MOB_VNUM_SUPERMOB )
      return TRUE;

   return FALSE;
}

/*
 * Return ascii name of an item type.
 */
char *item_type_name( OBJ_DATA * obj )
{
   if( obj->item_type < 1 || obj->item_type >= MAX_ITEM_TYPE )
   {
      bug( "Item_type_name: unknown type %d.", obj->item_type );
      return "(unknown)";
   }

   return o_types[obj->item_type];
}

/*
 * Return ascii name of an affect location.
 */
char *affect_loc_name( int location )
{
   switch ( location )
   {
      case APPLY_NONE:
         return "none";
      case APPLY_STR:
         return "strength";
      case APPLY_DEX:
         return "dexterity";
      case APPLY_INT:
         return "intelligence";
      case APPLY_WIS:
         return "wisdom";
      case APPLY_CON:
         return "constitution";
      case APPLY_CHA:
         return "charisma";
      case APPLY_LCK:
         return "luck";
      case APPLY_SEX:
         return "sex";
      case APPLY_CLASS:
         return "class";
      case APPLY_LEVEL:
         return "level";
      case APPLY_AGE:
         return "age";
      case APPLY_MANA:
         return "mana";
      case APPLY_HIT:
         return "hp";
      case APPLY_MOVE:
         return "moves";
      case APPLY_MANA_REGEN:
         return "mana regen";
      case APPLY_HIT_REGEN:
         return "hp regen ";
      case APPLY_MOVE_REGEN:
         return "move regen";
      case APPLY_ANTIMAGIC:
         return "antimagic";
      case APPLY_GOLD:
         return "gold";
      case APPLY_EXP:
         return "experience";
      case APPLY_AC:
         return "armor class";
      case APPLY_HITROLL:
         return "hit roll";
      case APPLY_DAMROLL:
         return "damage roll";
      case APPLY_SAVING_POISON:
         return "save vs poison";
      case APPLY_SAVING_ROD:
         return "save vs rod";
      case APPLY_SAVING_PARA:
         return "save vs paralysis";
      case APPLY_SAVING_BREATH:
         return "save vs breath";
      case APPLY_SAVING_SPELL:
         return "save vs spell";
      case APPLY_HEIGHT:
         return "height";
      case APPLY_WEIGHT:
         return "weight";
      case APPLY_AFFECT:
         return "affected_by";
      case APPLY_RESISTANT:
         return "resistant";
      case APPLY_IMMUNE:
         return "immune";
      case APPLY_SUSCEPTIBLE:
         return "susceptible";
      case APPLY_ABSORB:
         return "absorbs";
      case APPLY_ATTACKS:
         return "attacks";
      case APPLY_BACKSTAB:
         return "backstab";
      case APPLY_PICK:
         return "pick";
      case APPLY_PEEK:
         return "peek";
      case APPLY_TRACK:
         return "track";
      case APPLY_STEAL:
         return "steal";
      case APPLY_SNEAK:
         return "sneak";
      case APPLY_HIDE:
         return "hide";
      case APPLY_PALM:
         return "palm";
      case APPLY_DETRAP:
         return "detrap";
      case APPLY_DODGE:
         return "dodge";
      case APPLY_SF:
         return "spellfail";
      case APPLY_SCAN:
         return "scan";
      case APPLY_GOUGE:
         return "gouge";
      case APPLY_SEARCH:
         return "search";
      case APPLY_MOUNT:
         return "mount";
      case APPLY_DISARM:
         return "disarm";
      case APPLY_KICK:
         return "kick";
      case APPLY_PARRY:
         return "parry";
      case APPLY_BASH:
         return "bash";
      case APPLY_STUN:
         return "stun";
      case APPLY_PUNCH:
         return "punch";
      case APPLY_CLIMB:
         return "climb";
      case APPLY_GRIP:
         return "grip";
      case APPLY_SCRIBE:
         return "scribe";
      case APPLY_BREW:
         return "brew";
      case APPLY_WEAPONSPELL:
         return "weapon spell";
      case APPLY_WEARSPELL:
         return "wear spell";
      case APPLY_REMOVESPELL:
         return "remove spell";
      case APPLY_RECURRINGSPELL:
         return "recurring spell";
      case APPLY_MENTALSTATE:
         return "mental state";
      case APPLY_STRIPSN:
         return "dispel";
      case APPLY_REMOVE:
         return "remove";
      case APPLY_DIG:
         return "dig";
      case APPLY_FULL:
         return "hunger";
      case APPLY_THIRST:
         return "thirst";
      case APPLY_DRUNK:
         return "drunk";
      case APPLY_ROOMFLAG:
         return "roomflag";
      case APPLY_SECTORTYPE:
         return "sectortype";
      case APPLY_ROOMLIGHT:
         return "roomlight";
      case APPLY_TELEVNUM:
         return "teleport vnum";
      case APPLY_TELEDELAY:
         return "teleport delay";
      case APPLY_COOK:
         return "cook";
      case APPLY_RACE:
         return "race";
      case APPLY_HITNDAM:
         return "hit-n-dam";
      case APPLY_SAVING_ALL:
         return "all saves";
      case APPLY_EAT_SPELL:
         return "eat spell";
      case APPLY_RACE_SLAYER:
         return "race slayer";
      case APPLY_ALIGN_SLAYER:
         return "alignment slayer";
      case APPLY_EXTRAGOLD:
         return "extragold";
      case APPLY_ALLSTATS:
         return "allstats";
      case APPLY_EXT_AFFECT:
         return "ext_affected";
   }
   bug( "Affect_location_name: unknown location %d.", location );
   return "(unknown)";
}

/*
 * Return ascii name of an affect bit vector.
 */
const char *affect_bit_name( EXT_BV * vector )
{
   static char buf[MSL];

   buf[0] = '\0';
   if( xIS_SET( *vector, AFF_BLIND ) )
      mudstrlcat( buf, " blind", MSL );
   if( xIS_SET( *vector, AFF_INVISIBLE ) )
      mudstrlcat( buf, " invisible", MSL );
   if( xIS_SET( *vector, AFF_DETECT_EVIL ) )
      mudstrlcat( buf, " detect_evil", MSL );
   if( xIS_SET( *vector, AFF_DETECT_INVIS ) )
      mudstrlcat( buf, " detect_invis", MSL );
   if( xIS_SET( *vector, AFF_DETECT_MAGIC ) )
      mudstrlcat( buf, " detect_magic", MSL );
   if( xIS_SET( *vector, AFF_DETECT_HIDDEN ) )
      mudstrlcat( buf, " detect_hidden", MSL );
   if( xIS_SET( *vector, AFF_HOLD ) )
      mudstrlcat( buf, " hold", MSL );
   if( xIS_SET( *vector, AFF_SANCTUARY ) )
      mudstrlcat( buf, " sanctuary", MSL );
   if( xIS_SET( *vector, AFF_FAERIE_FIRE ) )
      mudstrlcat( buf, " faerie_fire", MSL );
   if( xIS_SET( *vector, AFF_INFRARED ) )
      mudstrlcat( buf, " infrared", MSL );
   if( xIS_SET( *vector, AFF_CURSE ) )
      mudstrlcat( buf, " curse", MSL );
   if( xIS_SET( *vector, AFF_SPY ) )
      mudstrlcat( buf, " spy", MSL );
   if( xIS_SET( *vector, AFF_POISON ) )
      mudstrlcat( buf, " poison", MSL );
   if( xIS_SET( *vector, AFF_PROTECT ) )
      mudstrlcat( buf, " protect", MSL );
   if( xIS_SET( *vector, AFF_PARALYSIS ) )
      mudstrlcat( buf, " paralysis", MSL );
   if( xIS_SET( *vector, AFF_SLEEP ) )
      mudstrlcat( buf, " sleep", MSL );
   if( xIS_SET( *vector, AFF_SNEAK ) )
      mudstrlcat( buf, " sneak", MSL );
   if( xIS_SET( *vector, AFF_HIDE ) )
      mudstrlcat( buf, " hide", MSL );
   if( xIS_SET( *vector, AFF_CHARM ) )
      mudstrlcat( buf, " charm", MSL );
   if( xIS_SET( *vector, AFF_POSSESS ) )
      mudstrlcat( buf, " possess", MSL );
   if( xIS_SET( *vector, AFF_FLYING ) )
      mudstrlcat( buf, " flying", MSL );
   if( xIS_SET( *vector, AFF_PASS_DOOR ) )
      mudstrlcat( buf, " passdoor", MSL );
   if( xIS_SET( *vector, AFF_FLOATING ) )
      mudstrlcat( buf, " floating", MSL );
   if( xIS_SET( *vector, AFF_TRUESIGHT ) )
      mudstrlcat( buf, " true_sight", MSL );
   if( xIS_SET( *vector, AFF_DETECTTRAPS ) )
      mudstrlcat( buf, " detect_traps", MSL );
   if( xIS_SET( *vector, AFF_SCRYING ) )
      mudstrlcat( buf, " scrying", MSL );
   if( xIS_SET( *vector, AFF_FIRESHIELD ) )
      mudstrlcat( buf, " fireshield", MSL );
   if( xIS_SET( *vector, AFF_ACIDMIST ) )
      mudstrlcat( buf, " acidmist", MSL );
   if( xIS_SET( *vector, AFF_VENOMSHIELD ) )
      mudstrlcat( buf, " venomshield", MSL );
   if( xIS_SET( *vector, AFF_SHOCKSHIELD ) )
      mudstrlcat( buf, " shockshield", MSL );
   if( xIS_SET( *vector, AFF_ICESHIELD ) )
      mudstrlcat( buf, " iceshield", MSL );
   if( xIS_SET( *vector, AFF_QUIV ) )
      mudstrlcat( buf, " quiv_palm", MSL );
   if( xIS_SET( *vector, AFF_POSSESS ) )
      mudstrlcat( buf, " possess", MSL );
   if( xIS_SET( *vector, AFF_BERSERK ) )
      mudstrlcat( buf, " berserk", MSL );
   if( xIS_SET( *vector, AFF_AQUA_BREATH ) )
      mudstrlcat( buf, " aqua_breath", MSL );
   if( xIS_SET( *vector, AFF_BLADEBARRIER ) )
      mudstrlcat( buf, " bladebarrier", MSL );
   if( xIS_SET( *vector, AFF_SILENCE ) )
      mudstrlcat( buf, " silence", MSL );
   if( xIS_SET( *vector, AFF_ANIMAL_INVIS ) )
      mudstrlcat( buf, " animal_invis", MSL );
   if( xIS_SET( *vector, AFF_HEAT_STUFF ) )
      mudstrlcat( buf, " heat_stuff", MSL );
   if( xIS_SET( *vector, AFF_LIFE_PROT ) )
      mudstrlcat( buf, " life_prot", MSL );
   if( xIS_SET( *vector, AFF_DRAGON_RIDE ) )
      mudstrlcat( buf, " dragon_ride", MSL );
   if( xIS_SET( *vector, AFF_GROWTH ) )
      mudstrlcat( buf, " growth", MSL );
   if( xIS_SET( *vector, AFF_TREE_TRAVEL ) )
      mudstrlcat( buf, " tree_travel", MSL );
   if( xIS_SET( *vector, AFF_TRAVELLING ) )
      mudstrlcat( buf, " travelling", MSL );
   if( xIS_SET( *vector, AFF_TELEPATHY ) )
      mudstrlcat( buf, " telepathy", MSL );
   if( xIS_SET( *vector, AFF_ETHEREAL ) )
      mudstrlcat( buf, " ethereal", MSL );
   if( xIS_SET( *vector, AFF_HASTE ) )
      mudstrlcat( buf, " haste", MSL );
   if( xIS_SET( *vector, AFF_SLOW ) )
      mudstrlcat( buf, " slow", MSL );
   if( xIS_SET( *vector, AFF_ELVENSONG ) )
      mudstrlcat( buf, " elvensong", MSL );
   if( xIS_SET( *vector, AFF_BLADESONG ) )
      mudstrlcat( buf, " bladesong", MSL );
   if( xIS_SET( *vector, AFF_REVERIE ) )
      mudstrlcat( buf, " reverie", MSL );
   if( xIS_SET( *vector, AFF_TENACITY ) )
      mudstrlcat( buf, " tenacity", MSL );
   if( xIS_SET( *vector, AFF_DEATHSONG ) )
      mudstrlcat( buf, " deathsong", MSL );
   if( xIS_SET( *vector, AFF_WIZARDEYE ) )
      mudstrlcat( buf, " wizardeye", MSL );
   if( xIS_SET( *vector, AFF_NOTRACK ) )
      mudstrlcat( buf, " notrack", MSL );
   if( xIS_SET( *vector, AFF_ENLIGHTEN ) )
      mudstrlcat( buf, " enlighten", MSL );
   if( xIS_SET( *vector, AFF_TREETALK ) )
      mudstrlcat( buf, " treetalk", MSL );
   if( xIS_SET( *vector, AFF_SPAMGUARD ) )
      mudstrlcat( buf, " spamguard", MSL );
   if( xIS_SET( *vector, AFF_BASH ) )
      mudstrlcat( buf, " bash", MSL );
   return ( buf[0] != '\0' ) ? buf + 1 : "none";
}

/*
 * Return ascii name of extra flags vector.
 */
const char *extra_bit_name( EXT_BV * extra_flags )
{
   static char buf[MSL];

   buf[0] = '\0';
   if( xIS_SET( *extra_flags, ITEM_GLOW ) )
      mudstrlcat( buf, " glow", MSL );
   if( xIS_SET( *extra_flags, ITEM_HUM ) )
      mudstrlcat( buf, " hum", MSL );
   if( xIS_SET( *extra_flags, ITEM_TWOHAND ) )
      mudstrlcat( buf, " twohand", MSL );
   if( xIS_SET( *extra_flags, ITEM_LOYAL ) )
      mudstrlcat( buf, " loyal", MSL );
   if( xIS_SET( *extra_flags, ITEM_EVIL ) )
      mudstrlcat( buf, " evil", MSL );
   if( xIS_SET( *extra_flags, ITEM_INVIS ) )
      mudstrlcat( buf, " invis", MSL );
   if( xIS_SET( *extra_flags, ITEM_MAGIC ) )
      mudstrlcat( buf, " magic", MSL );
   if( xIS_SET( *extra_flags, ITEM_NODROP ) )
      mudstrlcat( buf, " nodrop", MSL );
   if( xIS_SET( *extra_flags, ITEM_BLESS ) )
      mudstrlcat( buf, " bless", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_GOOD ) )
      mudstrlcat( buf, " antigood", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_EVIL ) )
      mudstrlcat( buf, " antievil", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_NEUTRAL ) )
      mudstrlcat( buf, " antineutral", MSL );
   if( xIS_SET( *extra_flags, ITEM_NOREMOVE ) )
      mudstrlcat( buf, " noremove", MSL );
   if( xIS_SET( *extra_flags, ITEM_INVENTORY ) )
      mudstrlcat( buf, " inventory", MSL );
   if( xIS_SET( *extra_flags, ITEM_DEATHROT ) )
      mudstrlcat( buf, " deathrot", MSL );
   if( xIS_SET( *extra_flags, ITEM_GROUNDROT ) )
      mudstrlcat( buf, " groundrot", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_MAGE ) )
      mudstrlcat( buf, " antimage", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_ROGUE ) )
      mudstrlcat( buf, " antirogue", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_WARRIOR ) )
      mudstrlcat( buf, " antiwarrior", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_CLERIC ) )
      mudstrlcat( buf, " anticleric", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_DRUID ) )
      mudstrlcat( buf, " antidruid", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_BARD ) )
      mudstrlcat( buf, " antibard", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_MONK ) )
      mudstrlcat( buf, " antimonk", MSL );
   if( xIS_SET( *extra_flags, ITEM_ORGANIC ) )
      mudstrlcat( buf, " organic", MSL );
   if( xIS_SET( *extra_flags, ITEM_METAL ) )
      mudstrlcat( buf, " metal", MSL );
   if( xIS_SET( *extra_flags, ITEM_DONATION ) )
      mudstrlcat( buf, " donated", MSL );
   if( xIS_SET( *extra_flags, ITEM_CLANOBJECT ) )
      mudstrlcat( buf, " clan", MSL );
   if( xIS_SET( *extra_flags, ITEM_CLANCORPSE ) )
      mudstrlcat( buf, " clanbody", MSL );
   if( xIS_SET( *extra_flags, ITEM_PROTOTYPE ) )
      mudstrlcat( buf, " prototype", MSL );
   if( xIS_SET( *extra_flags, ITEM_MINERAL ) )
      mudstrlcat( buf, " mineral", MSL );
   if( xIS_SET( *extra_flags, ITEM_BRITTLE ) )
      mudstrlcat( buf, " brittle", MSL );
   if( xIS_SET( *extra_flags, ITEM_RESISTANT ) )
      mudstrlcat( buf, " resistant", MSL );
   if( xIS_SET( *extra_flags, ITEM_IMMUNE ) )
      mudstrlcat( buf, " immune", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_MEN ) )
      mudstrlcat( buf, " antimen", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_WOMEN ) )
      mudstrlcat( buf, " antiwomen", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_NEUTER ) )
      mudstrlcat( buf, " antineuter", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_HERMA ) )
      mudstrlcat( buf, " antihermaphrodyte", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_SUN ) )
      mudstrlcat( buf, " antisun", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_RANGER ) )
      mudstrlcat( buf, " antiranger", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_PALADIN ) )
      mudstrlcat( buf, " antipaladin", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_NECRO ) )
      mudstrlcat( buf, " antinecro", MSL );
   if( xIS_SET( *extra_flags, ITEM_ANTI_APAL ) )
      mudstrlcat( buf, " antiapal", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_CLERIC ) )
      mudstrlcat( buf, " onlycleric", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_MAGE ) )
      mudstrlcat( buf, " onlymage", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_ROGUE ) )
      mudstrlcat( buf, " onlyrogue", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_WARRIOR ) )
      mudstrlcat( buf, " onlywarrior", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_BARD ) )
      mudstrlcat( buf, " onlybard", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_DRUID ) )
      mudstrlcat( buf, " onlydruid", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_MONK ) )
      mudstrlcat( buf, " onlymonk", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_RANGER ) )
      mudstrlcat( buf, " onlyranger", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_PALADIN ) )
      mudstrlcat( buf, " onlypaladin", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_NECRO ) )
      mudstrlcat( buf, " onlynecro", MSL );
   if( xIS_SET( *extra_flags, ITEM_ONLY_APAL ) )
      mudstrlcat( buf, " onlyapal", MSL );
   if( xIS_SET( *extra_flags, ITEM_PERSONAL ) )
      mudstrlcat( buf, " personal", MSL );
   if( xIS_SET( *extra_flags, ITEM_LODGED ) )
      mudstrlcat( buf, " lodged", MSL );
   if( xIS_SET( *extra_flags, ITEM_SINDHAE ) )
      mudstrlcat( buf, " sindhae", MSL );
   if( xIS_SET( *extra_flags, ITEM_MUSTMOUNT ) )
      mudstrlcat( buf, " mustmount", MSL );
   if( xIS_SET( *extra_flags, ITEM_NOAUCTION ) )
      mudstrlcat( buf, " noauction", MSL );
   return ( buf[0] != '\0' ) ? buf + 1 : "none";
}

/*
 * Return ascii name of magic flags vector. - Scryn
 */
const char *magic_bit_name( int magic_flags )
{
   static char buf[MSL];

   buf[0] = '\0';
   if( magic_flags & ITEM_RETURNING )
      mudstrlcat( buf, " returning", MSL );
   if( magic_flags & ITEM_BACKSTABBER )
      mudstrlcat( buf, " backstabber", MSL );
   if( magic_flags & ITEM_BANE )
      mudstrlcat( buf, " bane", MSL );
   if( magic_flags & ITEM_LOYAL )
      mudstrlcat( buf, " loyal", MSL );
   if( magic_flags & ITEM_HASTE )
      mudstrlcat( buf, " haste", MSL );
   if( magic_flags & ITEM_DRAIN )
      mudstrlcat( buf, " drain", MSL );
   if( magic_flags & ITEM_LIGHTNING_BLADE )
      mudstrlcat( buf, " lightningblade", MSL );

   return ( buf[0] != '\0' ) ? buf + 1 : "none";
}

/* Return ascii name of wear flags vector. - Added by Samson 2-8-98 */
const char *wear_bit_name( int wear_flags )
{
   static char buf[MSL];

   buf[0] = '\0';
   if( wear_flags & ITEM_TAKE )
      mudstrlcat( buf, " take", MSL );
   if( wear_flags & ITEM_WEAR_FINGER )
      mudstrlcat( buf, " finger", MSL );
   if( wear_flags & ITEM_WEAR_NECK )
      mudstrlcat( buf, " neck", MSL );
   if( wear_flags & ITEM_WEAR_BODY )
      mudstrlcat( buf, " body", MSL );
   if( wear_flags & ITEM_WEAR_HEAD )
      mudstrlcat( buf, " head", MSL );
   if( wear_flags & ITEM_WEAR_LEGS )
      mudstrlcat( buf, " legs", MSL );
   if( wear_flags & ITEM_WEAR_FEET )
      mudstrlcat( buf, " feet", MSL );
   if( wear_flags & ITEM_WEAR_HANDS )
      mudstrlcat( buf, " hands", MSL );
   if( wear_flags & ITEM_WEAR_ARMS )
      mudstrlcat( buf, " arms", MSL );
   if( wear_flags & ITEM_WEAR_SHIELD )
      mudstrlcat( buf, " shield", MSL );
   if( wear_flags & ITEM_WEAR_ABOUT )
      mudstrlcat( buf, " about", MSL );
   if( wear_flags & ITEM_WEAR_WAIST )
      mudstrlcat( buf, " waist", MSL );
   if( wear_flags & ITEM_WEAR_WRIST )
      mudstrlcat( buf, " wrist", MSL );
   if( wear_flags & ITEM_WIELD )
      mudstrlcat( buf, " wield", MSL );
   if( wear_flags & ITEM_HOLD )
      mudstrlcat( buf, " hold", MSL );
   if( wear_flags & ITEM_DUAL_WIELD )
      mudstrlcat( buf, " dual-wield", MSL );
   if( wear_flags & ITEM_WEAR_EARS )
      mudstrlcat( buf, " ears", MSL );
   if( wear_flags & ITEM_WEAR_EYES )
      mudstrlcat( buf, " eyes", MSL );
   if( wear_flags & ITEM_MISSILE_WIELD )
      mudstrlcat( buf, " missile", MSL );
   if( wear_flags & ITEM_WEAR_BACK )
      mudstrlcat( buf, " back", MSL );
   if( wear_flags & ITEM_WEAR_FACE )
      mudstrlcat( buf, " face", MSL );
   if( wear_flags & ITEM_WEAR_ANKLE )
      mudstrlcat( buf, " ankle", MSL );
   return ( buf[0] != '\0' ) ? buf + 1 : "none";
}

/*
 * Set off a trap (obj) upon character (ch)			-Thoric
 */
ch_ret spring_trap( CHAR_DATA * ch, OBJ_DATA * obj )
{
   int dam, typ, lev;
   char *txt;
   ch_ret retcode;

   typ = obj->value[1];
   lev = obj->value[2];

   retcode = rNONE;

   switch ( typ )
   {
      default:
         txt = "hit by a trap";
         break;
      case TRAP_TYPE_POISON_GAS:
         txt = "surrounded by a green cloud of gas";
         break;
      case TRAP_TYPE_POISON_DART:
         txt = "hit by a dart";
         break;
      case TRAP_TYPE_POISON_NEEDLE:
         txt = "pricked by a needle";
         break;
      case TRAP_TYPE_POISON_DAGGER:
         txt = "stabbed by a dagger";
         break;
      case TRAP_TYPE_POISON_ARROW:
         txt = "struck with an arrow";
         break;
      case TRAP_TYPE_BLINDNESS_GAS:
         txt = "surrounded by a red cloud of gas";
         break;
      case TRAP_TYPE_SLEEPING_GAS:
         txt = "surrounded by a yellow cloud of gas";
         break;
      case TRAP_TYPE_FLAME:
         txt = "struck by a burst of flame";
         break;
      case TRAP_TYPE_EXPLOSION:
         txt = "hit by an explosion";
         break;
      case TRAP_TYPE_ACID_SPRAY:
         txt = "covered by a spray of acid";
         break;
      case TRAP_TYPE_ELECTRIC_SHOCK:
         txt = "suddenly shocked";
         break;
      case TRAP_TYPE_BLADE:
         txt = "sliced by a razor sharp blade";
         break;
      case TRAP_TYPE_SEX_CHANGE:
         txt = "surrounded by a mysterious aura";
         break;
   }
   dam = number_range( obj->value[4], obj->value[5] );
   act_printf( AT_HITME, ch, NULL, NULL, TO_CHAR, "You are %s!", txt );
   act_printf( AT_ACTION, ch, NULL, NULL, TO_ROOM, "$n is %s.", txt );
   --obj->value[0];
   if( obj->value[0] <= 0 )
      extract_obj( obj );
   switch ( typ )
   {
      default:
      case TRAP_TYPE_BLADE:
         retcode = damage( ch, ch, dam, TYPE_UNDEFINED );
         break;

      case TRAP_TYPE_POISON_DART:
      case TRAP_TYPE_POISON_NEEDLE:
      case TRAP_TYPE_POISON_DAGGER:
      case TRAP_TYPE_POISON_ARROW:
         retcode = obj_cast_spell( gsn_poison, lev, ch, ch, NULL );
         if( retcode == rNONE )
            retcode = damage( ch, ch, dam, TYPE_UNDEFINED );
         break;

      case TRAP_TYPE_POISON_GAS:
         retcode = obj_cast_spell( gsn_poison, lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_BLINDNESS_GAS:
         retcode = obj_cast_spell( gsn_blindness, lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_SLEEPING_GAS:
         retcode = obj_cast_spell( skill_lookup( "sleep" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_ACID_SPRAY:
         retcode = obj_cast_spell( skill_lookup( "acid blast" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_SEX_CHANGE:
         retcode = obj_cast_spell( skill_lookup( "change sex" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_FLAME:
         retcode = obj_cast_spell( skill_lookup( "flamestrike" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_EXPLOSION:
         retcode = obj_cast_spell( gsn_fireball, lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_ELECTRIC_SHOCK:
         retcode = obj_cast_spell( skill_lookup( "lightning bolt" ), lev, ch, ch, NULL );
         break;
   }
   return retcode;
}

/*
 * Check an object for a trap					-Thoric
 */
ch_ret check_for_trap( CHAR_DATA * ch, OBJ_DATA * obj, int flag )
{
   OBJ_DATA *check;
   ch_ret retcode;

   if( !obj->first_content )
      return rNONE;

   retcode = rNONE;

   for( check = obj->first_content; check; check = check->next_content )
      if( check->item_type == ITEM_TRAP && IS_SET( check->value[3], flag ) )
      {
         retcode = spring_trap( ch, check );
         if( retcode != rNONE )
            return retcode;
      }
   return retcode;
}

/*
 * Check the room for a trap					-Thoric
 */
ch_ret check_room_for_traps( CHAR_DATA * ch, int flag )
{
   OBJ_DATA *check;
   ch_ret retcode;

   retcode = rNONE;

   if( !ch )
      return rERROR;
   if( !ch->in_room || !ch->in_room->first_content )
      return rNONE;

   for( check = ch->in_room->first_content; check; check = check->next_content )
   {
      if( check->item_type == ITEM_TRAP && IS_SET( check->value[3], flag ) )
      {
         retcode = spring_trap( ch, check );
         if( retcode != rNONE )
            return retcode;
      }
   }
   return retcode;
}

/*
 * return TRUE if an object contains a trap			-Thoric
 */
bool is_trapped( OBJ_DATA * obj )
{
   OBJ_DATA *check;

   if( !obj->first_content )
      return FALSE;

   for( check = obj->first_content; check; check = check->next_content )
      if( check->item_type == ITEM_TRAP )
         return TRUE;

   return FALSE;
}

/*
 * If an object contains a trap, return the pointer to the trap	-Thoric
 */
OBJ_DATA *get_trap( OBJ_DATA * obj )
{
   OBJ_DATA *check;

   if( !obj->first_content )
      return NULL;

   for( check = obj->first_content; check; check = check->next_content )
      if( check->item_type == ITEM_TRAP )
         return check;

   return NULL;
}

/*
 * Return a pointer to the first object of a certain type found that
 * a player is carrying/wearing
 */
OBJ_DATA *get_objtype( CHAR_DATA * ch, short type )
{
   OBJ_DATA *obj;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->item_type == type )
         return obj;

   return NULL;
}

/*
 * Remove an exit from a room					-Thoric
 */
void extract_exit( ROOM_INDEX_DATA * room, EXIT_DATA * pexit )
{
   UNLINK( pexit, room->first_exit, room->last_exit, next, prev );
   if( pexit->rexit )
      pexit->rexit->rexit = NULL;
   STRFREE( pexit->keyword );
   STRFREE( pexit->exitdesc );
   DISPOSE( pexit );
}

/*
 * clean out a room (leave list pointers intact )		-Thoric
 */
void clean_room( ROOM_INDEX_DATA * room )
{
   EXTRA_DESCR_DATA *ed, *ed_next;
   EXIT_DATA *pexit, *pexit_next;
   MPROG_DATA *mprog, *mprog_next;

   DISPOSE( room->roomdesc );
   DISPOSE( room->nitedesc );
   STRFREE( room->name );
   for( mprog = room->mudprogs; mprog; mprog = mprog_next )
   {
      mprog_next = mprog->next;
      STRFREE( mprog->arglist );
      STRFREE( mprog->comlist );
      DISPOSE( mprog );
   }
   for( ed = room->first_extradesc; ed; ed = ed_next )
   {
      ed_next = ed->next;
      STRFREE( ed->extradesc );
      STRFREE( ed->keyword );
      DISPOSE( ed );
      top_ed--;
   }
   room->first_extradesc = NULL;
   room->last_extradesc = NULL;
   for( pexit = room->first_exit; pexit; pexit = pexit_next )
   {
      pexit_next = pexit->next;
      extract_exit( room, pexit );
      top_exit--;
   }
   room->first_exit = NULL;
   room->last_exit = NULL;
   xCLEAR_BITS( room->room_flags );
   room->sector_type = 0;
   room->light = 0;
}

/*
 * clean out an object (index) (leave list pointers intact )	-Thoric
 */
void clean_obj( OBJ_INDEX_DATA * obj )
{
   AFFECT_DATA *paf, *paf_next;
   EXTRA_DESCR_DATA *ed, *ed_next;
   MPROG_DATA *mprog, *mprog_next;

   STRFREE( obj->name );
   STRFREE( obj->short_descr );
   STRFREE( obj->objdesc );
   STRFREE( obj->action_desc );
   STRFREE( obj->socket[0] );
   STRFREE( obj->socket[1] );
   STRFREE( obj->socket[2] );
   obj->item_type = 0;
   xCLEAR_BITS( obj->extra_flags );
   obj->wear_flags = 0;
   obj->count = 0;
   obj->weight = 0;
   obj->cost = 0;
   obj->value[0] = 0;
   obj->value[1] = 0;
   obj->value[2] = 0;
   obj->value[3] = 0;
   obj->value[4] = 0;
   obj->value[5] = 0;
   obj->value[6] = 0;
   obj->value[7] = 0;
   obj->value[8] = 0;
   obj->value[9] = 0;
   obj->value[10] = 0;
   for( paf = obj->first_affect; paf; paf = paf_next )
   {
      paf_next = paf->next;
      DISPOSE( paf );
      top_affect--;
   }
   obj->first_affect = NULL;
   obj->last_affect = NULL;
   for( ed = obj->first_extradesc; ed; ed = ed_next )
   {
      ed_next = ed->next;
      STRFREE( ed->extradesc );
      STRFREE( ed->keyword );
      DISPOSE( ed );
      top_ed--;
   }
   obj->first_extradesc = NULL;
   obj->last_extradesc = NULL;
   for( mprog = obj->mudprogs; mprog; mprog = mprog_next )
   {
      mprog_next = mprog->next;
      STRFREE( mprog->arglist );
      STRFREE( mprog->comlist );
      DISPOSE( mprog );
   }
}

/*
 * clean out a mobile (index) (leave list pointers intact )	-Thoric
 */
void clean_mob( MOB_INDEX_DATA * mob )
{
   MPROG_DATA *mprog, *mprog_next;

   STRFREE( mob->player_name );
   STRFREE( mob->short_descr );
   STRFREE( mob->long_descr );
   STRFREE( mob->chardesc );
   STRFREE( mob->spec_funname );
   mob->spec_fun = NULL;
   mob->pShop = NULL;
   mob->rShop = NULL;
   xCLEAR_BITS( mob->progtypes );

   for( mprog = mob->mudprogs; mprog; mprog = mprog_next )
   {
      mprog_next = mprog->next;
      STRFREE( mprog->arglist );
      STRFREE( mprog->comlist );
      DISPOSE( mprog );
   }
   mob->count = 0;
   mob->killed = 0;
   mob->sex = 0;
   mob->level = 0;
   xCLEAR_BITS( mob->act );
   xCLEAR_BITS( mob->affected_by );
   mob->alignment = 0;
   mob->mobthac0 = 0;
   mob->ac = 0;
   mob->hitnodice = 0;
   mob->hitsizedice = 0;
   mob->hitplus = 0;
   mob->damnodice = 0;
   mob->damsizedice = 0;
   mob->damplus = 0;
   mob->gold = 0;
   mob->position = 0;
   mob->defposition = 0;
   mob->height = 0;
   mob->weight = 0;
   xCLEAR_BITS( mob->attacks );
   xCLEAR_BITS( mob->defenses );
}

extern int top_reset;

/*
 * Remove all resets from a room -Thoric
 */
void clean_resets( ROOM_INDEX_DATA * room )
{
   RESET_DATA *pReset, *pReset_next;

   for( pReset = room->first_reset; pReset; pReset = pReset_next )
   {
      pReset_next = pReset->next;
      delete_reset( pReset );
      --top_reset;
   }
   room->first_reset = NULL;
   room->last_reset = NULL;
}

/*
 * Show an affect verbosely to a character			-Thoric
 */
void showaffect( CHAR_DATA * ch, AFFECT_DATA * paf )
{
   char buf[MSL];
   int x;

   if( !paf )
   {
      bug( "%s", "showaffect: NULL paf" );
      return;
   }
   if( paf->location != APPLY_NONE && paf->modifier != 0 )
   {
      switch ( paf->location )
      {
         default:
            snprintf( buf, MSL, "&wAffects: &B%15s&w by &B%3d&w.\n\r", affect_loc_name( paf->location ), paf->modifier );
            break;
         case APPLY_AFFECT:
            snprintf( buf, MSL, "&wAffects: &B%15s&w by &B", affect_loc_name( paf->location ) );
            for( x = 0; x < 32; x++ )
               if( IS_SET( paf->modifier, 1 << x ) )
               {
                  mudstrlcat( buf, " ", MSL );
                  mudstrlcat( buf, a_flags[x], MSL );
               }
            mudstrlcat( buf, "&w\n\r", MSL );
            break;
         case APPLY_WEAPONSPELL:
         case APPLY_WEARSPELL:
         case APPLY_REMOVESPELL:
         case APPLY_EAT_SPELL:
            snprintf( buf, MSL, "&wCasts spell: &B'%s'&w\n\r",
                      IS_VALID_SN( paf->modifier ) ? skill_table[paf->modifier]->name : "unknown" );
            break;
         case APPLY_RESISTANT:
         case APPLY_IMMUNE:
         case APPLY_SUSCEPTIBLE:
         case APPLY_ABSORB:
            snprintf( buf, MSL, "&wAffects: &B%15s&w by &B", affect_loc_name( paf->location ) );
            for( x = 0; x < 32; x++ )
               if( IS_SET( paf->modifier, 1 << x ) )
               {
                  mudstrlcat( buf, " ", MSL );
                  mudstrlcat( buf, ris_flags[x], MSL );
               }
            mudstrlcat( buf, "&w\n\r", MSL );
            break;
      }
      send_to_char( buf, ch );
   }
}

/*
 * Check the recently extracted object queue for obj		-Thoric
 */
bool obj_extracted( OBJ_DATA * obj )
{
   OBJ_DATA *cod;

   for( cod = extracted_obj_queue; cod; cod = cod->next )
      if( obj == cod )
         return TRUE;

   return FALSE;
}

/*
 * Clean out the extracted object queue
 */
void clean_obj_queue( void )
{
   OBJ_DATA *obj;

   while( extracted_obj_queue )
   {
      obj = extracted_obj_queue;
      extracted_obj_queue = extracted_obj_queue->next;
      free_obj( obj );
      --cur_qobjs;
   }
}

/*
 * clean out the extracted character queue
 */
void clean_char_queue( void )
{
   EXTRACT_CHAR_DATA *ccd;

   for( ccd = extracted_char_queue; ccd; ccd = extracted_char_queue )
   {
      extracted_char_queue = ccd->next;
      if( ccd->extract )
         free_char( ccd->ch );
      DISPOSE( ccd );
      --cur_qchars;
   }
}

/*
 * Check to see if ch died recently				-Thoric
 */
bool char_died( CHAR_DATA * ch )
{
   EXTRACT_CHAR_DATA *ccd;

   for( ccd = extracted_char_queue; ccd; ccd = ccd->next )
      if( ccd->ch == ch )
         return TRUE;
   return FALSE;
}

/*
 * Add a timer to ch						-Thoric
 * Support for "call back" time delayed commands
 */
void add_timer( CHAR_DATA * ch, short type, short count, DO_FUN * fun, int value )
{
   TIMER *timer;

   for( timer = ch->first_timer; timer; timer = timer->next )
      if( timer->type == type )
      {
         timer->count = count;
         timer->do_fun = fun;
         timer->value = value;
         break;
      }
   if( !timer )
   {
      CREATE( timer, TIMER, 1 );
      timer->count = count;
      timer->type = type;
      timer->do_fun = fun;
      timer->value = value;
      LINK( timer, ch->first_timer, ch->last_timer, next, prev );
   }
}

TIMER *get_timerptr( CHAR_DATA * ch, short type )
{
   TIMER *timer;

   for( timer = ch->first_timer; timer; timer = timer->next )
      if( timer->type == type )
         return timer;
   return NULL;
}

short get_timer( CHAR_DATA * ch, short type )
{
   TIMER *timer;

   if( ( timer = get_timerptr( ch, type ) ) != NULL )
      return timer->count;
   else
      return 0;
}

void extract_timer( CHAR_DATA * ch, TIMER * timer )
{
   if( !timer )
   {
      bug( "%s", "extract_timer: NULL timer" );
      return;
   }

   UNLINK( timer, ch->first_timer, ch->last_timer, next, prev );
   DISPOSE( timer );
   return;
}

void remove_timer( CHAR_DATA * ch, short type )
{
   TIMER *timer;

   for( timer = ch->first_timer; timer; timer = timer->next )
      if( timer->type == type )
         break;

   if( timer )
      extract_timer( ch, timer );
}

bool in_hard_range( CHAR_DATA * ch, AREA_DATA * tarea )
{
   if( IS_IMMORTAL( ch ) )
      return TRUE;
   else if( IS_NPC( ch ) )
      return TRUE;
   else if( ch->level >= tarea->low_hard_range && ch->level <= tarea->hi_hard_range )
      return TRUE;
   else
      return FALSE;
}

/*
 * Scryn, standard luck check 2/2/96
 */
bool chance( CHAR_DATA * ch, short percent )
{
/*  short clan_factor, ms;*/
   short deity_factor, ms;

   if( !ch )
   {
      bug( "%s", "Chance: null ch!" );
      return FALSE;
   }

/* Code for clan stuff put in by Narn, Feb/96.  The idea is to punish clan
members who don't keep their alignment in tune with that of their clan by
making it harder for them to succeed at pretty much everything.  Clan_factor
will vary from 1 to 3, with 1 meaning there is no effect on the player's
change of success, and with 3 meaning they have half the chance of doing
whatever they're trying to do. 

Note that since the neutral clannies can only be off by 1000 points, their
maximum penalty will only be half that of the other clan types.

  if ( IS_CLANNED( ch ) )
    clan_factor = 1 + abs( ch->alignment - ch->pcdata->clan->alignment ) / 1000; 
  else
    clan_factor = 1;
*/
/* Mental state bonus/penalty:  Your mental state is a ranged value with
 * zero (0) being at a perfect mental state (bonus of 10).
 * negative values would reflect how sedated one is, and
 * positive values would reflect how stimulated one is.
 * In most circumstances you'd do best at a perfectly balanced state.
 */

   if( IS_DEVOTED( ch ) )
      deity_factor = ch->pcdata->favor / -500;
   else
      deity_factor = 0;

   ms = 10 - abs( ch->mental_state );

   if( ( number_percent(  ) - get_curr_lck( ch ) + 13 - ms ) + deity_factor <= percent )
      return TRUE;
   else
      return FALSE;
}

/*
 * Make a simple clone of an object (no extras...yet)		-Thoric
 */
OBJ_DATA *clone_object( OBJ_DATA * obj )
{
   OBJ_DATA *clone;

   CREATE( clone, OBJ_DATA, 1 );
   clone->pIndexData = obj->pIndexData;
   clone->name = QUICKLINK( obj->name );
   clone->short_descr = QUICKLINK( obj->short_descr );
   clone->objdesc = QUICKLINK( obj->objdesc );
   if( obj->action_desc && obj->action_desc[0] != '\0' )
      clone->action_desc = QUICKLINK( obj->action_desc );
   if( obj->socket[0] && obj->socket[0] != '\0' )
      clone->socket[0] = QUICKLINK( obj->socket[0] );
   if( obj->socket[1] && obj->socket[1] != '\0' )
      clone->socket[1] = QUICKLINK( obj->socket[1] );
   if( obj->socket[2] && obj->socket[2] != '\0' )
      clone->socket[2] = QUICKLINK( obj->socket[2] );
   clone->item_type = obj->item_type;
   clone->extra_flags = obj->extra_flags;
   clone->magic_flags = obj->magic_flags;
   clone->wear_flags = obj->wear_flags;
   clone->wear_loc = obj->wear_loc;
   clone->weight = obj->weight;
   clone->cost = obj->cost;
   clone->level = obj->level;
   clone->timer = obj->timer;
   clone->map = obj->map;
   clone->x = obj->x;
   clone->y = obj->y;
   clone->value[0] = obj->value[0];
   clone->value[1] = obj->value[1];
   clone->value[2] = obj->value[2];
   clone->value[3] = obj->value[3];
   clone->value[4] = obj->value[4];
   clone->value[5] = obj->value[5];
   clone->value[6] = obj->value[6];
   clone->value[7] = obj->value[7];
   clone->value[8] = obj->value[8];
   clone->value[9] = obj->value[9];
   clone->value[10] = obj->value[10];
   clone->count = 1;
   ++obj->pIndexData->count;
   ++numobjsloaded;
   ++physicalobjects;
   LINK( clone, first_object, last_object, next, prev );
   return clone;
}

/*
 * Split off a grouped object					-Thoric
 * decreased obj's count to num, and creates a new object containing the rest
 */
void split_obj( OBJ_DATA * obj, int num )
{
   int count = obj->count;
   OBJ_DATA *rest;

   if( count <= num || num == 0 )
      return;

   rest = clone_object( obj );
   --obj->pIndexData->count;  /* since clone_object() ups this value */
   --numobjsloaded;
   rest->count = obj->count - num;
   obj->count = num;

   if( obj->carried_by )
   {
      LINK( rest, obj->carried_by->first_carrying, obj->carried_by->last_carrying, next_content, prev_content );
      rest->carried_by = obj->carried_by;
      rest->in_room = NULL;
      rest->in_obj = NULL;
   }
   else if( obj->in_room )
   {
      LINK( rest, obj->in_room->first_content, obj->in_room->last_content, next_content, prev_content );
      rest->carried_by = NULL;
      rest->in_room = obj->in_room;
      rest->in_obj = NULL;
   }
   else if( obj->in_obj )
   {
      LINK( rest, obj->in_obj->first_content, obj->in_obj->last_content, next_content, prev_content );
      rest->in_obj = obj->in_obj;
      rest->in_room = NULL;
      rest->carried_by = NULL;
   }
}

void separate_obj( OBJ_DATA * obj )
{
   split_obj( obj, 1 );
}

/*
 * Empty an obj's contents... optionally into another obj, or a room
 */
bool empty_obj( OBJ_DATA * obj, OBJ_DATA * destobj, ROOM_INDEX_DATA * destroom )
{
   OBJ_DATA *otmp, *otmp_next;
   CHAR_DATA *ch = obj->carried_by;
   bool movedsome = FALSE;

   if( !obj )
   {
      bug( "%s", "empty_obj: NULL obj" );
      return FALSE;
   }
   if( destobj || ( !destroom && !ch && ( destobj = obj->in_obj ) != NULL ) )
   {
      for( otmp = obj->first_content; otmp; otmp = otmp_next )
      {
         otmp_next = otmp->next_content;
         /*
          * only keys on a keyring 
          */
         if( destobj->item_type == ITEM_KEYRING && otmp->item_type != ITEM_KEY )
            continue;
         if( destobj->item_type == ITEM_QUIVER && otmp->item_type != ITEM_PROJECTILE )
            continue;
         if( ( destobj->item_type == ITEM_CONTAINER || destobj->item_type == ITEM_KEYRING
               || destobj->item_type == ITEM_QUIVER )
             && get_real_obj_weight( otmp ) + get_real_obj_weight( destobj ) > destobj->value[0] )
            continue;
         obj_from_obj( otmp );
         obj_to_obj( otmp, destobj );
         movedsome = TRUE;
      }
      return movedsome;
   }
   if( destroom || ( !ch && ( destroom = obj->in_room ) != NULL ) )
   {
      for( otmp = obj->first_content; otmp; otmp = otmp_next )
      {
         otmp_next = otmp->next_content;
         if( ch && HAS_PROG( otmp->pIndexData, DROP_PROG ) && otmp->count > 1 )
         {
            separate_obj( otmp );
            obj_from_obj( otmp );
            if( !otmp_next )
               otmp_next = obj->first_content;
         }
         else
            obj_from_obj( otmp );
         obj = obj_to_room( otmp, destroom, ch );
         if( ch )
         {
            oprog_drop_trigger( ch, otmp );  /* mudprogs */
            if( char_died( ch ) )
               ch = NULL;
         }
         movedsome = TRUE;
      }
      return movedsome;
   }
   if( ch )
   {
      for( otmp = obj->first_content; otmp; otmp = otmp_next )
      {
         otmp_next = otmp->next_content;
         obj_from_obj( otmp );
         obj_to_char( otmp, ch );
         movedsome = TRUE;
      }
      return movedsome;
   }
   bug( "empty_obj: could not determine a destination for vnum %d", obj->pIndexData->vnum );
   return FALSE;
}

/*
 * Improve mental state						-Thoric
 */
void better_mental_state( CHAR_DATA * ch, int mod )
{
   int c = URANGE( 0, abs( mod ), 20 );
   int con = get_curr_con( ch );

   if( IS_IMMORTAL( ch ) )
   {
      ch->mental_state = 0;
      return;
   }

   c += number_percent(  ) < con ? 1 : 0;

   if( ch->mental_state < 0 )
      ch->mental_state = URANGE( -100, ch->mental_state + c, 0 );
   else if( ch->mental_state > 0 )
      ch->mental_state = URANGE( 0, ch->mental_state - c, 100 );
}

/*
 * Deteriorate mental state					-Thoric
 */
void worsen_mental_state( CHAR_DATA * ch, int mod )
{
   int c = URANGE( 0, abs( mod ), 20 );
   int con = get_curr_con( ch );

   if( IS_IMMORTAL( ch ) )
   {
      ch->mental_state = 0;
      return;
   }

   c -= number_percent(  ) < con ? 1 : 0;
   if( c < 1 )
      return;

   if( ch->mental_state < 0 )
      ch->mental_state = URANGE( -100, ch->mental_state - c, 100 );
   else if( ch->mental_state > 0 )
      ch->mental_state = URANGE( -100, ch->mental_state + c, 100 );
   else
      ch->mental_state -= c;
}

/*
 * returns area with name matching input string
 * Last Modified : July 21, 1997
 * Fireblade
 */
AREA_DATA *get_area( char *name )
{
   AREA_DATA *pArea;

   if( !name || name[0] == '\0' )
   {
      bug( "%s", "get_area: NULL input string." );
      return NULL;
   }

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      if( nifty_is_name( name, pArea->name ) )
         return pArea;
   }
   return NULL;
}

void check_switches( bool possess )
{
   CHAR_DATA *ch;

   for( ch = first_char; ch; ch = ch->next )
      check_switch( ch, possess );
}

CMDF do_switch( CHAR_DATA * ch, char *argument );
void check_switch( CHAR_DATA *ch, bool possess )
{
   AFFECT_DATA *paf;
   CMDTYPE *cmd;
   int hash, trust = get_trust(ch);

   if( !ch->switched )
      return;

   if( !possess )
   {
      for( paf = ch->switched->first_affect; paf; paf = paf->next )
      {
         if( paf->duration == -1 )
            continue;
      }
   }

   for( hash = 0; hash < 126; hash++ )
   {
      for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
      {
         if( cmd->do_fun != do_switch )
            continue;
         if( cmd->level <= trust )
            return;

         if( !IS_NPC(ch) && ch->pcdata->bestowments && is_name( cmd->name, ch->pcdata->bestowments )
          && cmd->level <= trust + sysdata.bestow_dif )
            return;
      }
   }

   if( !possess )
   {
      set_char_color( AT_BLUE, ch->switched );
      send_to_char( "You suddenly forfeit the power to switch!\n\r", ch->switched );
   }
   interpret( ch->switched, "return" );
}
