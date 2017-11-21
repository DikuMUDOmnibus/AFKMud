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
 *                         Battle & death module                            *
 ****************************************************************************/

#include "mud.h"
#include "clans.h"
#include "deity.h"
#include "fight.h"
#include "event.h"

extern char lastplayercmd[MIL * 2];
extern CHAR_DATA *gch_prev;
OBJ_DATA *used_weapon;  /* Used to figure out which weapon later */
bool dual_flip = FALSE;
bool alreadyUsedSkill = FALSE;

ROOM_INDEX_DATA *check_room( CHAR_DATA * ch, ROOM_INDEX_DATA * dieroom );  /* For Arena combat */
SPELLF spell_smaug( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_energy_drain( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_fire_breath( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_frost_breath( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_acid_breath( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_lightning_breath( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_gas_breath( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_spiral_blast( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_dispel_magic( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_dispel_evil( int sn, int level, CHAR_DATA * ch, void *vo );
void save_clan( CLAN_DATA * clan );
void adjust_favor( CHAR_DATA * ch, int field, int mod );
int recall( CHAR_DATA * ch, int target );
void update_member( CHAR_DATA * ch );
void stop_follower( CHAR_DATA * ch );
OBJ_DATA *create_money( int amount );
void generate_treasure( CHAR_DATA * ch, OBJ_DATA * corpse );
void mprog_fight_trigger( CHAR_DATA * mob, CHAR_DATA * ch );
void mprog_hitprcnt_trigger( CHAR_DATA * mob, CHAR_DATA * ch );
void mprog_death_trigger( CHAR_DATA * killer, CHAR_DATA * mob );
void do_unmorph_char( CHAR_DATA * ch );
bool check_parry( CHAR_DATA * ch, CHAR_DATA * victim );
bool check_dodge( CHAR_DATA * ch, CHAR_DATA * victim );
bool check_tumble( CHAR_DATA * ch, CHAR_DATA * victim );
void trip( CHAR_DATA * ch, CHAR_DATA * victim );
void make_scraps( OBJ_DATA * obj );

bool loot_coins_from_corpse( CHAR_DATA *ch, OBJ_DATA *corpse )
{
   OBJ_DATA *content, *content_next;
   int new_gold, gold_diff, old_gold = ch->gold;

   for( content = corpse->first_content; content; content = content_next )
   {
      content_next = content->next_content;

      if( content->item_type != ITEM_MONEY )
         continue;
      if( !can_see_obj( ch, content, true ) )
         continue;
      if( !CAN_WEAR( content, ITEM_TAKE ) && ch->level < sysdata.level_getobjnotake )
         continue;
      if( IS_OBJ_FLAG( content, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
         continue;

      act( AT_ACTION, "You get $p from $P", ch, content, corpse, TO_CHAR );
      act( AT_ACTION, "$n gets $p from $P", ch, content, corpse, TO_ROOM );
      obj_from_obj( content );
      check_for_trap( ch, content, TRAP_GET );
      if( char_died( ch ) )
         return FALSE;

      oprog_get_trigger( ch, content );
      if( char_died( ch ) )
         return FALSE;

      ch->gold += content->value[0] * content->count;
      extract_obj( content );
   }

   new_gold = ch->gold;
   gold_diff = ( new_gold - old_gold );
   if( IS_PLR_FLAG( ch, PLR_GUILDSPLIT ) && ch->position > POS_SLEEPING && ch->pcdata->clan && ch->pcdata->clan->bank )
   {
      int split = 0;

      if( gold_diff > 0 )
      {
         float xx = ( float )( ch->pcdata->clan->tithe ) / 100;
         float pct = ( float )( gold_diff ) * xx;
         split = ( int )( gold_diff - pct );

         ch->pcdata->clan->balance += split;
         save_clan( ch->pcdata->clan );
         ch_printf( ch, "The Wedgy comes to collect %d gold on behalf of your %s.\n\r", split,
            ch->pcdata->clan->clan_type == CLAN_ORDER ? "guild" : "clan" );
         gold_diff -= split;
         ch->gold -= split;
      }
   }
   if( gold_diff > 0 && IS_PLR_FLAG( ch, PLR_GROUPSPLIT ) && ch->position > POS_SLEEPING )
      cmdf( ch, "split %d", gold_diff );
   return TRUE;
}

int IsGiantish( CHAR_DATA * ch )
{
   if( !ch )
      return FALSE;

   switch ( GET_RACE( ch ) )
   {
      case RACE_ENFAN:
      case RACE_GOBLIN:
      case RACE_ORC:
      case RACE_GIANT:
      case RACE_GIANT_HILL:
      case RACE_GIANT_FROST:
      case RACE_GIANT_FIRE:
      case RACE_GIANT_CLOUD:
      case RACE_GIANT_STORM:
      case RACE_GIANT_STONE:
      case RACE_TYTAN:
      case RACE_TROLL:
      case RACE_DRAAGDIM:
      case RACE_HALF_ORC:
      case RACE_HALF_OGRE:
      case RACE_HALF_GIANT:
         return TRUE;
      default:
         return FALSE;
   }
}

/*
 * Check to see if the player is in an "Arena".
 */
bool in_arena( CHAR_DATA * ch )
{
   if( !ch )   /* Um..... Could THIS be why ? */
   {
      bug( "%s", "in_arena: NULL CH!!! Wedgy, you better spill the beans!" );
      return FALSE;
   }

   if( !ch->in_room )
   {
      bug( "in_arena: %s in NULL room. Only The Wedgy knows how though.", ch->name );
      bug( "%s", "Going to attempt to move them to Limbo to prevent a crash." );
      if( !char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return FALSE;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_ARENA ) )
      return TRUE;
   if( IS_AREA_FLAG( ch->in_room->area, AFLAG_ARENA ) )
      return TRUE;
   if( !str_cmp( ch->in_room->area->filename, "arena.are" ) )
      return TRUE;

   return FALSE;
}

void make_blood( CHAR_DATA * ch )
{
   OBJ_DATA *obj;

   if( !( obj = create_object( get_obj_index( OBJ_VNUM_BLOOD ), 0 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   obj->timer = number_range( 2, 4 );
   obj->value[1] = number_range( 3, UMIN( 5, ch->level ) );
   obj_to_room( obj, ch->in_room, ch );
}

/*
 * Make a corpse out of a character.
 */
OBJ_DATA *make_corpse( CHAR_DATA *ch, CHAR_DATA *killer )
{
   OBJ_DATA *corpse, *obj, *obj_next;
   char *name;

   if( IS_NPC( ch ) )
   {
      name = ch->short_descr;
      if( !( corpse = create_object( get_obj_index( OBJ_VNUM_CORPSE_NPC ), ch->level ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return NULL;
      }
      corpse->timer = 8;
      corpse->value[3] = 100; /* So the slice skill will work */
      /*
       * Just use the 4th value - cost is creating odd errors on compile - Samson 
       */
      /*
       * Used by do_slice and spell_animate_dead 
       */
      corpse->value[4] = ch->pIndexData->vnum;

      if( ch->gold > 0 )
      {
         obj_to_obj( create_money( ch->gold ), corpse );
         ch->gold = 0;
      }
      /*
       * Access random treasure generator, only if the killer was a player 
       */
      else if( ch->gold < 0 && !IS_NPC( killer ) )
         generate_treasure( killer, corpse );

      /*
       * Cannot use these! They are used.
       * corpse->value[0] = ch->pIndexData->vnum;
       * corpse->value[1] = ch->max_hit;
       */
      /*
       * Using corpse cost to cheat, since corpses not sellable 
       */
      corpse->cost = ( -( int )ch->pIndexData->vnum );
      corpse->value[2] = corpse->timer;
   }
   else
   {
      name = ch->name;
      if( !( corpse = create_object( get_obj_index( OBJ_VNUM_CORPSE_PC ), ch->level ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return NULL;
      }
      if( in_arena( ch ) )
         corpse->timer = 0;
      else
         corpse->timer = 45;  /* This will provide a slight window to resurrect at full health - Samson */
      if( ch->gold > 0 )
      {
         obj_to_obj( create_money( ch->gold ), corpse );
         ch->gold = 0;
      }

      /*
       * Removed special handling for PK corpses because it was messing up my evil plot - Samson 
       */
      corpse->value[2] = ( int )( corpse->timer / 8 );
      corpse->value[3] = 5;   /* Decay stage - 5 is best. */
      corpse->value[4] = ch->level;
      corpse->value[5] = 0;   /* Still flesh rotting */
   }

   if( CAN_PKILL( ch ) && CAN_PKILL( killer ) && ch != killer )
      stralloc_printf( &corpse->action_desc, "%s", killer->name );

   /*
    * Added corpse name - make locate easier, other skills 
    */
   stralloc_printf( &corpse->name, "corpse %s", name );
   stralloc_printf( &corpse->short_descr, corpse->short_descr, name );
   stralloc_printf( &corpse->objdesc, corpse->objdesc, name );

   /*
    * Used in spell_animate_dead to check for dragons, currently. -- Tarl 29 July 2002 
    */
   corpse->value[6] = ch->race;

   for( obj = ch->first_carrying; obj; obj = obj_next )
   {
      obj_next = obj->next_content;
      obj_from_char( obj );
      if( IS_OBJ_FLAG( obj, ITEM_INVENTORY ) || IS_OBJ_FLAG( obj, ITEM_DEATHROT ) )
         extract_obj( obj );
      else
         obj_to_obj( obj, corpse );
   }
   return obj_to_room( corpse, ch->in_room, ch );
}

/* 
 * New alignment shift computation ported from Sillymud code.
 * Samson 3-13-98
 */
int align_compute( CHAR_DATA * gch, CHAR_DATA * victim )
{
   int change, align;

   if( IS_NPC( gch ) )
      return gch->alignment;

   align = gch->alignment;

   if( IS_GOOD( gch ) && IS_GOOD( victim ) )
      change = ( victim->alignment / 200 ) * ( UMAX( 1, ( victim->level - gch->level ) ) );

   else if( IS_EVIL( gch ) && IS_GOOD( victim ) )
      change = ( victim->alignment / 30 ) * ( UMAX( 1, ( victim->level - gch->level ) ) );

   else if( IS_EVIL( victim ) && IS_GOOD( gch ) )
      change = ( victim->alignment / 30 ) * ( UMAX( 1, ( victim->level - gch->level ) ) );

   else if( IS_EVIL( gch ) && IS_EVIL( victim ) )
      change = ( ( victim->alignment / 200 ) + 1 ) * ( UMAX( 1, ( victim->level - gch->level ) ) );

   else
      change = ( victim->alignment / 200 ) * ( UMAX( 1, ( victim->level - gch->level ) ) );

   if( change == 0 )
   {
      if( victim->alignment > 0 )
         change = 1;
      else if( victim->alignment < 0 )
         change = -1;
   }

   align -= change;

   align = UMAX( align, -1000 );
   align = UMIN( align, 1000 );

   return align;
}

/* Alignment monitor for Paladin, Antipaladin and Druid classes - Samson 4-17-98
   Code provided by Sten */
void class_monitor( CHAR_DATA * gch )
{
   if( gch->Class == CLASS_RANGER && gch->alignment < -350 )
   {
      send_to_char( "You are no longer worthy of your powers as a ranger.....\n\r", gch );
      send_to_char( "A strange feeling comes over you as you become a mere warrior!\n\r", gch );
      gch->Class = CLASS_WARRIOR;
   }

   if( gch->Class == CLASS_ANTIPALADIN && gch->alignment > -350 )
   {
      send_to_char( "You are no longer worthy of your powers as an antipaladin.....\n\r", gch );
      send_to_char( "A strange feeling comes over you as you become a mere warrior!\n\r", gch );
      gch->Class = CLASS_WARRIOR;
   }

   if( gch->Class == CLASS_PALADIN && gch->alignment < 350 )
   {
      send_to_char( "You are no longer worthy of your powers as a paladin.....\n\r", gch );
      send_to_char( "A strange feeling comes over you as you become a mere warrior!\n\r", gch );
      gch->Class = CLASS_WARRIOR;
   }

   if( gch->Class == CLASS_DRUID && ( gch->alignment < -349 || gch->alignment > 349 ) )
   {
      send_to_char( "You are no longer worthy of your powers as a druid.....\n\r", gch );
      send_to_char( "A strange feeling comes over you as you become a mere cleric!\n\r", gch );
      gch->Class = CLASS_CLERIC;
   }

   if( gch->Class == CLASS_RANGER && ( gch->alignment < -249 && gch->alignment >= -350 ) )
      ch_printf( gch, "You are straying from your cause against evil %s!", gch->name );

   if( gch->Class == CLASS_ANTIPALADIN && ( gch->alignment > -449 && gch->alignment <= -351 ) )
      ch_printf( gch, "You are straying from your evil ways %s!", gch->name );

   if( gch->Class == CLASS_PALADIN && ( gch->alignment < 449 && gch->alignment >= 350 ) )
      ch_printf( gch, "You are straying from your rightious ways %s!", gch->name );

   if( gch->Class == CLASS_DRUID && ( gch->alignment < -249 || gch->alignment > 249 ) )
      ch_printf( gch, "You are straying from the balanced path %s!", gch->name );

   return;
}

CHAR_DATA *who_fighting( CHAR_DATA * ch )
{
   if( !ch )
   {
      bug( "%s", "who_fighting: null ch" );
      return NULL;
   }
   if( !ch->fighting )
      return NULL;
   return ch->fighting->who;
}

/* Aging attack for mobs - Samson 3-28-00 */
CMDF do_ageattack( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   int agechance;

   if( !IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
      return;

   agechance = number_range( 1, 100 );

   if( agechance < 92 )
      return;

   ch_printf( victim, "%s touches you, and you feel yourself aging!\n\r", ch->short_descr );
   victim->pcdata->age_bonus += 1;

   return;
}

/*
 * Check to see if player's attacks are (still?) suppressed
 * #ifdef TRI
 */
bool is_attack_supressed( CHAR_DATA * ch )
{
   TIMER *timer;

   if( IS_NPC( ch ) )
      return FALSE;

   timer = get_timerptr( ch, TIMER_ASUPRESSED );

   if( !timer )
      return FALSE;

   /*
    * perma-supression -- bard? (can be reset at end of fight, or spell, etc) 
    */
   if( timer->value == -1 )
      return TRUE;

   /*
    * this is for timed supressions 
    */
   if( timer->count >= 1 )
      return TRUE;

   return FALSE;
}

/*
 * Check to see if weapon is poisoned.
 */
bool is_wielding_poisoned( CHAR_DATA * ch )
{
   OBJ_DATA *obj;

   if( !used_weapon )
      return FALSE;

   if( ( obj = get_eq_char( ch, WEAR_WIELD ) ) != NULL && used_weapon == obj && IS_OBJ_FLAG( obj, ITEM_POISONED ) )
      return TRUE;
   if( ( obj = get_eq_char( ch, WEAR_DUAL_WIELD ) ) != NULL && used_weapon == obj && IS_OBJ_FLAG( obj, ITEM_POISONED ) )
      return TRUE;

   return FALSE;
}

/*
 * hunting, hating and fearing code				-Thoric
 */
bool is_hunting( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( !ch->hunting || ch->hunting->who != victim )
      return FALSE;

   return TRUE;
}

bool is_hating( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( !ch->hating || ch->hating->who != victim )
      return FALSE;

   return TRUE;
}

bool is_fearing( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( !ch->fearing || ch->fearing->who != victim )
      return FALSE;

   return TRUE;
}

void stop_hunting( CHAR_DATA * ch )
{
   if( ch->hunting )
   {
      STRFREE( ch->hunting->name );
      DISPOSE( ch->hunting );
      ch->hunting = NULL;
   }
   return;
}

void stop_hating( CHAR_DATA * ch )
{
   if( ch->hating )
   {
      STRFREE( ch->hating->name );
      DISPOSE( ch->hating );
      ch->hating = NULL;
   }
   return;
}

void stop_fearing( CHAR_DATA * ch )
{
   if( ch->fearing )
   {
      STRFREE( ch->fearing->name );
      DISPOSE( ch->fearing );
      ch->fearing = NULL;
   }
   return;
}

void start_hunting( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( ch->hunting )
      stop_hunting( ch );

   CREATE( ch->hunting, HHF_DATA, 1 );
   ch->hunting->name = QUICKLINK( victim->name );
   ch->hunting->who = victim;
   return;
}

void start_hating( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( ch->hating )
      stop_hating( ch );

   CREATE( ch->hating, HHF_DATA, 1 );
   ch->hating->name = QUICKLINK( victim->name );
   ch->hating->who = victim;
   return;
}

void start_fearing( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( ch->fearing )
      stop_fearing( ch );

   CREATE( ch->fearing, HHF_DATA, 1 );
   ch->fearing->name = QUICKLINK( victim->name );
   ch->fearing->who = victim;
   return;
}

int max_fight( CHAR_DATA * ch )
{
   return 8;
}

CMDF do_gfighting( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   DESCRIPTOR_DATA *d;
   char arg1[MIL], arg2[MIL];
   bool found = FALSE, pmobs = FALSE, phating = FALSE, phunting = FALSE;
   int low = 1, high = MAX_LEVEL, count = 0;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 && arg1[0] != '\0' )
   {
      if( !arg2 || arg2[0] == '\0' )
      {
         send_to_pager( "\n\r&wSyntax:  gfighting | gfighting <low> <high> | gfighting <low> <high> mobs\n\r", ch );
         return;
      }
      low = atoi( arg1 );
      high = atoi( arg2 );
   }
   if( low < 1 || high < low || low > high || high > MAX_LEVEL )
   {
      send_to_pager( "&wInvalid level range.\n\r", ch );
      return;
   }
   if( !str_cmp( argument, "mobs" ) )
      pmobs = TRUE;
   else if( !str_cmp( argument, "hating" ) )
      phating = TRUE;
   else if( !str_cmp( argument, "hunting" ) )
      phunting = TRUE;

   pager_printf( ch, "\n\r&cGlobal %s conflict:\n\r", pmobs ? "mob" : "character" );
   if( !pmobs && !phating && !phunting )
   {
      for( d = first_descriptor; d; d = d->next )
         if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
             && ( victim = d->character ) != NULL && !IS_NPC( victim ) && victim->in_room
             && can_see( ch, victim, FALSE ) && victim->fighting && victim->level >= low && victim->level <= high )
         {
            found = TRUE;
            pager_printf( ch, "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\n\r",
                          victim->name, victim->level, victim->fighting->who->level,
                          IS_NPC( victim->fighting->who ) ? victim->fighting->who->short_descr : victim->fighting->who->name,
                          IS_NPC( victim->fighting->who ) ? victim->fighting->who->pIndexData->vnum : 0,
                          victim->in_room->area->name, victim->in_room == NULL ? 0 : victim->in_room->vnum );
            count++;
         }
   }
   else if( !phating && !phunting )
   {
      for( victim = first_char; victim; victim = victim->next )
         if( IS_NPC( victim ) && victim->in_room && can_see( ch, victim, FALSE )
             && victim->fighting && victim->level >= low && victim->level <= high )
         {
            found = TRUE;
            pager_printf( ch, "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\n\r",
                          victim->name, victim->level, victim->fighting->who->level,
                          IS_NPC( victim->fighting->who ) ? victim->fighting->who->short_descr : victim->fighting->who->name,
                          IS_NPC( victim->fighting->who ) ? victim->fighting->who->pIndexData->vnum : 0,
                          victim->in_room->area->name, victim->in_room == NULL ? 0 : victim->in_room->vnum );
            count++;
         }
   }
   else if( !phunting && phating )
   {
      for( victim = first_char; victim; victim = victim->next )
         if( IS_NPC( victim ) && victim->in_room && can_see( ch, victim, FALSE )
             && victim->hating && victim->level >= low && victim->level <= high )
         {
            found = TRUE;
            pager_printf( ch, "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\n\r",
                          victim->name, victim->level, victim->hating->who->level, IS_NPC( victim->hating->who ) ?
                          victim->hating->who->short_descr : victim->hating->who->name, IS_NPC( victim->hating->who ) ?
                          victim->hating->who->pIndexData->vnum : 0, victim->in_room->area->name,
                          victim->in_room == NULL ? 0 : victim->in_room->vnum );
            count++;
         }
   }
   else if( phunting )
   {
      for( victim = first_char; victim; victim = victim->next )
         if( IS_NPC( victim ) && victim->in_room && can_see( ch, victim, FALSE )
             && victim->hunting && victim->level >= low && victim->level <= high )
         {
            found = TRUE;
            pager_printf( ch, "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\n\r",
                          victim->name, victim->level, victim->hunting->who->level, IS_NPC( victim->hunting->who ) ?
                          victim->hunting->who->short_descr : victim->hunting->who->name,
                          IS_NPC( victim->hunting->who ) ? victim->hunting->who->pIndexData->vnum : 0,
                          victim->in_room->area->name, victim->in_room == NULL ? 0 : victim->in_room->vnum );
            count++;
         }
   }
   pager_printf( ch, "&c%d %s conflicts located.\n\r", count, pmobs ? "mob" : "character" );
   return;
}

/*
 * Weapon types, haus
 */
int weapon_prof_bonus_check( CHAR_DATA * ch, OBJ_DATA * wield, int *gsn_ptr )
{
   int bonus;

   bonus = 0;
   *gsn_ptr = gsn_pugilism;   /* Change back to -1 if this fails horribly */

   if( !IS_NPC( ch ) && wield )
   {
      switch ( wield->value[4] )
      {
            /*
             * Restructured weapon system - Samson 11-20-99 
             */
         default:
            *gsn_ptr = -1;
            break;
         case WEP_BAREHAND:
            *gsn_ptr = gsn_pugilism;
            break;
         case WEP_SWORD:
            *gsn_ptr = gsn_swords;
            break;
         case WEP_DAGGER:
            *gsn_ptr = gsn_daggers;
            break;
         case WEP_WHIP:
            *gsn_ptr = gsn_whips;
            break;
         case WEP_TALON:
            *gsn_ptr = gsn_talonous_arms;
            break;
         case WEP_MACE:
            *gsn_ptr = gsn_maces_hammers;
            break;
         case WEP_ARCHERY:
            *gsn_ptr = gsn_archery;
            break;
         case WEP_BLOWGUN:
            *gsn_ptr = gsn_blowguns;
            break;
         case WEP_SLING:
            *gsn_ptr = gsn_slings;
            break;
         case WEP_AXE:
            *gsn_ptr = gsn_axes;
            break;
         case WEP_SPEAR:
            *gsn_ptr = gsn_spears;
            break;
         case WEP_STAFF:
            *gsn_ptr = gsn_staves;
            break;
      }
      if( *gsn_ptr != -1 )
         bonus = ( int )( ( LEARNED( ch, *gsn_ptr ) - 50 ) / 10 );
   }
   return bonus;
}

/*
 * Calculate to hit armor Class 0 versus armor.
 * Rewritten by Dwip, 5-11-01
 */
int calc_thac0( CHAR_DATA * ch, CHAR_DATA * victim, int dist )
{
   int base_thac0, tenacity_adj;
   float thac0, thac0_gain;

   if( IS_NPC( ch ) && ( ch->mobthac0 < 21 || !class_table[ch->Class] ) )
      thac0 = ch->mobthac0;
   else
   {
      base_thac0 = class_table[ch->Class]->base_thac0;   /* This is thac0_00 right now */
      thac0_gain = class_table[ch->Class]->thac0_gain;
      /*
       * thac0_gain is a new thing replacing thac0_32 in the classfiles.
       * *  It needs to be calced and set for all classes, but shouldn't be overly hard. 
       */
      thac0 = ( base_thac0 - ( ch->level * thac0_gain ) ) - GET_HITROLL( ch ) + ( dist * 2 );
   }

   /*
    * Tenacity skill affect for Dwarves - Samson 4-24-00 
    */
   tenacity_adj = 0;
   if( dist == 0 && victim )
   {
      if( ch->fighting && ch->fighting->who == victim && IS_AFFECTED( ch, AFF_TENACITY ) )
         tenacity_adj -= 4;
   }
   thac0 -= tenacity_adj;

   return ( int )thac0;
}

/*
 * Calculate the tohit bonus on the object and return RIS values.
 * -- Altrag
 */
int obj_hitroll( OBJ_DATA * obj )
{
   int tohit = 0;
   AFFECT_DATA *paf;

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      if( paf->location == APPLY_HITROLL || paf->location == APPLY_HITNDAM )
         tohit += paf->modifier;

   for( paf = obj->first_affect; paf; paf = paf->next )
      if( paf->location == APPLY_HITROLL || paf->location == APPLY_HITNDAM )
         tohit += paf->modifier;

   return tohit;
}

/*
 * Calculate damage based on resistances, immunities and suceptibilities
 *					-Thoric
 */
double ris_damage( CHAR_DATA * ch, double dam, int ris )
{
   short modifier;  /* FIND ME */

   modifier = 10;
   if( IS_IMMUNE( ch, ris ) && !xIS_SET( ch->no_immune, ris ) )
      modifier -= 10;
   if( IS_RESIS( ch, ris ) && !xIS_SET( ch->no_resistant, ris ) )
      modifier -= 2;
   if( IS_SUSCEP( ch, ris ) && !xIS_SET( ch->no_susceptible, ris ) )
   {
      if( IS_NPC( ch ) && IS_IMMUNE( ch, ris ) )
         modifier += 0;
      else
         modifier += 2;
   }
   if( modifier <= 0 )
      return -1;
   if( modifier == 10 )
      return dam;
   return ( dam * modifier ) / 10;
}

bool check_illegal_pk( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( !IS_NPC( victim ) && !IS_NPC( ch ) )
   {
      if( ( !IS_PCFLAG( victim, PCFLAG_DEADLY ) || !IS_PCFLAG( ch, PCFLAG_DEADLY ) )
          && !in_arena( ch ) && ch != victim && !( IS_IMMORTAL( ch ) && IS_IMMORTAL( victim ) ) )
      {
         log_printf( "&p%s on %s%s in &W***&rILLEGAL PKILL&W*** &pattempt at %d",
                     ( lastplayercmd ), ( IS_NPC( victim ) ? victim->short_descr : victim->name ),
                     ( IS_NPC( victim ) ? victim->name : "" ), victim->in_room->vnum );
         last_pkroom = victim->in_room->vnum;
         return TRUE;
      }
   }
   return FALSE;
}

bool is_safe( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( char_died( victim ) || char_died( ch ) )
      return TRUE;

   /*
    * Thx Josh! 
    */
   if( who_fighting( ch ) == ch )
      return FALSE;

   if( !victim )  /*Gonna find this is_safe crash bug -Blod */
   {
      bug( "Is_safe: %s opponent does not exist!", ch->name );
      return TRUE;
   }

   if( !victim->in_room )
   {
      bug( "Is_safe: %s has no physical location!", victim->name );
      return TRUE;
   }

   if( IS_ROOM_FLAG( victim->in_room, ROOM_SAFE ) )
   {
      send_to_char( "&[magic]A magical force prevents you from attacking.\n\r", ch );
      return TRUE;
   }

   if( IS_ACT_FLAG( ch, ACT_IMMORTAL ) )
   {
      send_to_char( "&[magic]You are protected by the Gods, and therefore cannot engage in combat.\n\r", ch );
      return TRUE;
   }

   if( IS_ACT_FLAG( victim, ACT_IMMORTAL ) )
   {
      ch_printf( ch, "&[magic]%s is protected by the Gods and cannot be killed!\n\r", capitalize( victim->short_descr ) );
      return TRUE;
   }

   if( IS_ACT_FLAG( ch, ACT_PACIFIST ) )  /* Fireblade */
   {
      send_to_char( "&[magic]You are a pacifist and will not fight.\n\r", ch );
      return TRUE;
   }

   if( IS_ACT_FLAG( victim, ACT_PACIFIST ) ) /* Gorog */
   {
      ch_printf( ch, "&[magic]%s is a pacifist and will not fight.\n\r", capitalize( victim->short_descr ) );
      return TRUE;
   }

   if( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
      return FALSE;

   if( !IS_NPC( ch ) && !IS_NPC( victim ) && ch != victim && IS_AREA_FLAG( victim->in_room->area, AFLAG_NOPKILL ) )
   {
      send_to_char( "&[immortal]The gods have forbidden player killing in this area.\n\r", ch );
      return TRUE;
   }

   if( IS_NPC( victim ) && IS_AFFECTED( victim, AFF_CHARM ) && IS_ACT_FLAG( victim, ACT_PET ) && victim->master != NULL )
   {
      if( check_illegal_pk( ch, victim->master ) )
      {
         ch_printf( ch, "You may not engage %s's followers in combat.\n\r", victim->master->name );
         return TRUE;
      }

      /*
       * If check added to stop slaughtering pets by accident. (ie: burning hands spell on a pony ;) 
       */
      /*
       * Added by Tarl, 18 Mar 02 
       */
      if( !str_cmp( ch->name, victim->master->name ) )
      {
         send_to_char( "You may not engage your followers in combat.\n\r", ch );
         return TRUE;
      }
   }

   if( IS_NPC( ch ) && IS_NPC( victim ) )
   {
      if( IS_AFFECTED( ch, AFF_CHARM ) && IS_AFFECTED( victim, AFF_CHARM )
          && IS_ACT_FLAG( ch, ACT_PET ) && IS_ACT_FLAG( victim, ACT_PET ) && ch->master != NULL && victim->master != NULL )
      {
         if( check_illegal_pk( ch->master, victim->master ) )
         {
            ch_printf( ch->master, "&RYou cannot have your followers engage %s's followers in combat.\n\r",
                       victim->master->name );
            return TRUE;
         }
      }
   }

   if( IS_NPC( ch ) || IS_NPC( victim ) )
      return FALSE;

   if( check_illegal_pk( ch, victim ) )
   {
      send_to_char( "You cannot attack that person.\n\r", ch );
      return TRUE;
   }

   if( get_timer( victim, TIMER_PKILLED ) > 0 )
   {
      send_to_char( "&GThat character has died within the last 5 minutes.\n\r", ch );
      return TRUE;
   }

   if( get_timer( ch, TIMER_PKILLED ) > 0 )
   {
      send_to_char( "&GYou have been killed within the last 5 minutes.\n\r", ch );
      return TRUE;
   }
   return FALSE;
}

void align_zap( CHAR_DATA * ch )
{
   OBJ_DATA *obj, *obj_next;

   for( obj = ch->first_carrying; obj; obj = obj_next )
   {
      obj_next = obj->next_content;
      if( obj->wear_loc == WEAR_NONE )
         continue;

      if( ( IS_OBJ_FLAG( obj, ITEM_ANTI_EVIL ) && IS_EVIL( ch ) )
          || ( IS_OBJ_FLAG( obj, ITEM_ANTI_GOOD ) && IS_GOOD( ch ) )
          || ( IS_OBJ_FLAG( obj, ITEM_ANTI_NEUTRAL ) && IS_NEUTRAL( ch ) ) )
      {
         act( AT_MAGIC, "You are zapped by $p.", ch, obj, NULL, TO_CHAR );
         act( AT_MAGIC, "$n is zapped by $p.", ch, obj, NULL, TO_ROOM );
         obj_from_char( obj );
         obj = obj_to_room( obj, ch->in_room, ch );
         oprog_zap_trigger( ch, obj ); /* mudprogs */
         if( char_died( ch ) )
            return;
      }
   }
}

/* New (or old depending on point of view :P) exp calculations.
 * Reformulated by Samson on 3-12-98. Lets hope this crap works better
 * than what came stock! Thanks to Sillymud for the ratio formula.
 */
double RatioExp( CHAR_DATA * gch, CHAR_DATA * victim, double xp )
{
   double ratio, fexp, chlevel = gch->level, viclevel = victim->level, tempexp = victim->exp;

   ratio = viclevel / chlevel;

   if( ratio < 1.0 )
      fexp = ratio * tempexp;
   else
      fexp = xp;

   return fexp;
}

int xp_compute( CHAR_DATA * gch, CHAR_DATA * victim )
{
   int xp, xp_ratio, gchlev = gch->level;

   xp = victim->exp;

   if( gch->level > 35 )
      xp = ( int )RatioExp( gch, victim, xp );

   /*
    * Supposed to prevent polies from abusing something, not sure what tho. 
    */
   if( IS_NPC( gch ) )
   {
      xp *= 3;
      xp /= 4;
   }

   xp = number_range( ( xp * 3 ) >> 2, ( xp * 5 ) >> 2 );

   /*
    * semi-intelligent experienced player vs. novice player xp gain
    * "bell curve"ish xp mod by Thoric
    * based on time played vs. level
    */
   if( !IS_NPC( gch ) && gchlev > 5 )
   {
      xp_ratio = ( int )gch->pcdata->played / gchlev;
      if( xp_ratio > 20000 )
         xp = ( xp * 5 ) >> 2;
      else if( xp_ratio < 16000 )
         xp = ( xp * 3 ) >> 2;
      else if( xp_ratio < 10000 )
         xp >>= 1;
      else if( xp_ratio < 5000 )
         xp >>= 2;
      else if( xp_ratio < 3500 )
         xp >>= 3;
      else if( xp_ratio < 2000 )
         xp >>= 4;
   }

   /*
    * New EXP cap for PK kills 
    */
   if( !IS_NPC( victim ) )
   {
      int diff = victim->level - gch->level;

      if( diff == 0 )
         diff = 1;

      if( diff < 0 )
         diff = 0;

      xp = diff * 2000; /* 2000 exp per level above the killer */
   }

   /*
    * Level based experience gain cap.  Cannot get more experience for
    * a kill than the amount for your current experience level   -Thoric
    */
   return URANGE( 0, xp, exp_level( gchlev ) );
}

/* Figures up a mob's exp value based on some obscure D&D formula.
   Not sure exactly where or how it was derived, but it worked well
   enough for Sillymud, so why not here? :P Samson - 5-15-98 */
/* This has been modified to calculate based on what the mob_xp function tells it to use - Samson 5-18-01 */
int calculate_mob_exp( CHAR_DATA * mob, int exp_flags )
{
   int base;
   int phit;
   int sab;

   switch ( mob->level )
   {

      case 0:
         base = 10;
         phit = 1;
         sab = 4;
         break;

      case 1:
         base = 20;
         phit = 2;
         sab = 8;
         break;

      case 2:
         base = 35;
         phit = 3;
         sab = 15;
         break;

      case 3:
         base = 60;
         phit = 4;
         sab = 25;
         break;

      case 4:
         base = 90;
         phit = 5;
         sab = 40;
         break;

      case 5:
         base = 150;
         phit = 6;
         sab = 75;
         break;

      case 6:
         base = 225;
         phit = 8;
         sab = 125;
         break;

      case 7:
         base = 375;
         phit = 10;
         sab = 175;
         break;

      case 8:
         base = 600;
         phit = 12;
         sab = 300;
         break;

      case 9:
         base = 900;
         phit = 14;
         sab = 450;
         break;

      case 10:
         base = 1100;
         phit = 15;
         sab = 575;
         break;

      case 11:
         base = 1300;
         phit = 16;
         sab = 700;
         break;

      case 12:
         base = 1550;
         phit = 17;
         sab = 825;
         break;

      case 13:
         base = 1800;
         phit = 18;
         sab = 950;
         break;

      case 14:
         base = 2100;
         phit = 19;
         sab = 1100;
         break;

      case 15:
         base = 2400;
         phit = 20;
         sab = 1250;
         break;

      case 16:
         base = 2700;
         phit = 23;
         sab = 1400;
         break;

      case 17:
         base = 3000;
         phit = 25;
         sab = 1550;
         break;

      case 18:
         base = 4000;
         phit = 28;
         sab = 2000;
         break;

      case 19:
         base = 5000;
         phit = 30;
         sab = 2500;
         break;

      case 20:
         base = 6000;
         phit = 33;
         sab = 3000;
         break;

      case 21:
         base = 7000;
         phit = 35;
         sab = 3500;
         break;

      case 22:
         base = 8000;
         phit = 38;
         sab = 4000;
         break;

      case 23:
         base = 9000;
         phit = 40;
         sab = 4500;
         break;

      case 24:
         base = 11000;
         phit = 45;
         sab = 5500;
         break;

      case 25:
         base = 13000;
         phit = 50;
         sab = 6500;
         break;

      case 26:
         base = 15000;
         phit = 55;
         sab = 7500;
         break;

      case 27:
         base = 17000;
         phit = 60;
         sab = 8500;
         break;

      case 28:
         base = 19000;
         phit = 65;
         sab = 9500;
         break;

      case 29:
         base = 21000;
         phit = 70;
         sab = 10500;
         break;

      case 30:
      case 31:
      case 32:
      case 33:
      case 34:
         base = 24000;
         phit = 80;
         sab = 12000;
         break;


      case 35:
      case 36:
      case 37:
      case 38:
      case 39:
         base = 27000;
         phit = 90;
         sab = 13500;
         break;

      case 40:
      case 41:
      case 42:
      case 43:
      case 44:
         base = 30000;
         phit = 100;
         sab = 15000;
         break;

      case 45:
      case 46:
      case 47:
      case 48:
      case 49:
         base = 33000;
         phit = 110;
         sab = 16500;
         break;

      case 50:
      case 51:
      case 52:
      case 53:
      case 54:
         base = 36000;
         phit = 120;
         sab = 18000;
         break;

      case 55:
      case 56:
      case 57:
      case 58:
      case 59:
         base = 39000;
         phit = 130;
         sab = 19500;
         break;

      case 60:
      case 61:
      case 62:
      case 63:
      case 64:
         base = 43000;
         phit = 150;
         sab = 21500;
         break;

      case 65:
      case 66:
      case 67:
      case 68:
      case 69:
         base = 47000;
         phit = 170;
         sab = 23500;
         break;

      case 70:
      case 71:
      case 72:
      case 73:
      case 74:
         base = 51000;
         phit = 190;
         sab = 25500;
         break;

      case 75:
      case 76:
      case 77:
      case 78:
      case 79:
         base = 55000;
         phit = 210;
         sab = 27500;
         break;

      case 80:
      case 81:
      case 82:
      case 83:
      case 84:
         base = 59000;
         phit = 230;
         sab = 29500;
         break;

      case 85:
      case 86:
      case 87:
      case 88:
      case 89:
         base = 63000;
         phit = 250;
         sab = 31500;
         break;

      case 90:
      case 91:
      case 92:
      case 93:
      case 94:
         base = 68000;
         phit = 280;
         sab = 34000;
         break;

      case 95:
      case 96:
      case 97:
      case 98:
      case 99:
         base = 73000;
         phit = 310;
         sab = 36500;
         break;

      case 100:
         base = 78000;
         phit = 350;
         sab = 39000;
         break;

      default:
         base = 85000;
         phit = 400;
         sab = 42500;
         break;
   }

   return ( base + ( phit * mob->max_hit ) + ( sab * exp_flags ) );
}

/* The new and improved mob xp_flag formula. Data used is according to information from Dwip.
 * Sue him if this isn't working right :)
 * This is gonna be one big, ugly set of ifchecks.
 * Samson 5-18-01
 */
int mob_xp( CHAR_DATA * mob )
{
   int flags = 0;

   if( mob->armor < 0 )
      flags++;

   if( mob->numattacks > 4 )
      flags += 2;

   if( IS_ACT_FLAG( mob, ACT_AGGRESSIVE ) || IS_ACT_FLAG( mob, ACT_META_AGGR ) )
      flags++;

   if( IS_ACT_FLAG( mob, ACT_WIMPY ) )
      flags -= 2;

   if( IS_AFFECTED( mob, AFF_DETECT_INVIS ) || IS_AFFECTED( mob, AFF_DETECT_HIDDEN ) || IS_AFFECTED( mob, AFF_TRUESIGHT ) )
      flags++;

   if( IS_AFFECTED( mob, AFF_SANCTUARY ) )
      flags++;

   if( IS_AFFECTED( mob, AFF_SNEAK ) || IS_AFFECTED( mob, AFF_HIDE ) || IS_AFFECTED( mob, AFF_INVISIBLE ) )
      flags++;

   if( IS_AFFECTED( mob, AFF_FIRESHIELD ) )
      flags++;

   if( IS_AFFECTED( mob, AFF_SHOCKSHIELD ) )
      flags++;

   if( IS_AFFECTED( mob, AFF_ICESHIELD ) )
      flags++;

   if( IS_AFFECTED( mob, AFF_VENOMSHIELD ) )
      flags += 2;

   if( IS_AFFECTED( mob, AFF_ACIDMIST ) )
      flags += 2;

   if( IS_AFFECTED( mob, AFF_BLADEBARRIER ) )
      flags += 2;

   if( IS_RESIS( mob, RIS_FIRE ) || IS_RESIS( mob, RIS_COLD ) || IS_RESIS( mob, RIS_ELECTRICITY )
       || IS_RESIS( mob, RIS_ENERGY ) || IS_RESIS( mob, RIS_ACID ) )
      flags++;

   if( IS_RESIS( mob, RIS_BLUNT ) || IS_RESIS( mob, RIS_SLASH ) || IS_RESIS( mob, RIS_PIERCE )
       || IS_RESIS( mob, RIS_HACK ) || IS_RESIS( mob, RIS_LASH ) )
      flags++;

   if( IS_RESIS( mob, RIS_SLEEP ) || IS_RESIS( mob, RIS_CHARM ) || IS_RESIS( mob, RIS_HOLD )
       || IS_RESIS( mob, RIS_POISON ) || IS_RESIS( mob, RIS_PARALYSIS ) )
      flags++;

   if( IS_RESIS( mob, RIS_PLUS1 ) || IS_RESIS( mob, RIS_PLUS2 ) || IS_RESIS( mob, RIS_PLUS3 )
       || IS_RESIS( mob, RIS_PLUS4 ) || IS_RESIS( mob, RIS_PLUS5 ) || IS_RESIS( mob, RIS_PLUS6 ) )
      flags++;

   if( IS_RESIS( mob, RIS_MAGIC ) )
      flags += 2;

   if( IS_RESIS( mob, RIS_NONMAGIC ) )
      flags++;

   if( IS_IMMUNE( mob, RIS_FIRE ) || IS_IMMUNE( mob, RIS_COLD ) || IS_IMMUNE( mob, RIS_ELECTRICITY )
       || IS_IMMUNE( mob, RIS_ENERGY ) || IS_IMMUNE( mob, RIS_ACID ) )
      flags += 2;

   if( IS_IMMUNE( mob, RIS_BLUNT ) || IS_IMMUNE( mob, RIS_SLASH ) || IS_IMMUNE( mob, RIS_PIERCE )
       || IS_IMMUNE( mob, RIS_HACK ) || IS_IMMUNE( mob, RIS_LASH ) )
      flags += 2;

   if( IS_IMMUNE( mob, RIS_SLEEP ) || IS_IMMUNE( mob, RIS_CHARM ) || IS_IMMUNE( mob, RIS_HOLD )
       || IS_IMMUNE( mob, RIS_POISON ) || IS_IMMUNE( mob, RIS_PARALYSIS ) )
      flags += 2;

   if( IS_IMMUNE( mob, RIS_PLUS1 ) || IS_IMMUNE( mob, RIS_PLUS2 ) || IS_IMMUNE( mob, RIS_PLUS3 )
       || IS_IMMUNE( mob, RIS_PLUS4 ) || IS_IMMUNE( mob, RIS_PLUS5 ) || IS_IMMUNE( mob, RIS_PLUS6 ) )
      flags += 2;

   if( IS_IMMUNE( mob, RIS_MAGIC ) )
      flags += 2;

   if( IS_IMMUNE( mob, RIS_NONMAGIC ) )
      flags++;

   if( IS_SUSCEP( mob, RIS_FIRE ) || IS_SUSCEP( mob, RIS_COLD ) || IS_SUSCEP( mob, RIS_ELECTRICITY )
       || IS_SUSCEP( mob, RIS_ENERGY ) || IS_SUSCEP( mob, RIS_ACID ) )
      flags -= 2;

   if( IS_SUSCEP( mob, RIS_BLUNT ) || IS_SUSCEP( mob, RIS_SLASH ) || IS_SUSCEP( mob, RIS_PIERCE )
       || IS_SUSCEP( mob, RIS_HACK ) || IS_SUSCEP( mob, RIS_LASH ) )
      flags -= 3;

   if( IS_SUSCEP( mob, RIS_SLEEP ) || IS_SUSCEP( mob, RIS_CHARM ) || IS_SUSCEP( mob, RIS_HOLD )
       || IS_SUSCEP( mob, RIS_POISON ) || IS_SUSCEP( mob, RIS_PARALYSIS ) )
      flags -= 3;

   if( IS_SUSCEP( mob, RIS_PLUS1 ) || IS_SUSCEP( mob, RIS_PLUS2 ) || IS_SUSCEP( mob, RIS_PLUS3 )
       || IS_SUSCEP( mob, RIS_PLUS4 ) || IS_SUSCEP( mob, RIS_PLUS5 ) || IS_SUSCEP( mob, RIS_PLUS6 ) )
      flags -= 2;

   if( IS_SUSCEP( mob, RIS_MAGIC ) )
      flags -= 3;

   if( IS_SUSCEP( mob, RIS_NONMAGIC ) )
      flags -= 2;

   if( IS_ABSORB( mob, RIS_FIRE ) || IS_ABSORB( mob, RIS_COLD ) || IS_ABSORB( mob, RIS_ELECTRICITY )
       || IS_ABSORB( mob, RIS_ENERGY ) || IS_ABSORB( mob, RIS_ACID ) )
      flags += 3;

   if( IS_ABSORB( mob, RIS_BLUNT ) || IS_ABSORB( mob, RIS_SLASH ) || IS_ABSORB( mob, RIS_PIERCE )
       || IS_ABSORB( mob, RIS_HACK ) || IS_ABSORB( mob, RIS_LASH ) )
      flags += 3;

/*
   if( IS_ABSORB( mob, RIS_SLEEP ) || IS_ABSORB( mob, RIS_CHARM ) || IS_ABSORB( mob, RIS_HOLD )
    || IS_ABSORB( mob, RIS_POISON ) || IS_ABSORB( mob, RIS_PARALYSIS ) )
	flags += 3;
*/

   if( IS_ABSORB( mob, RIS_PLUS1 ) || IS_ABSORB( mob, RIS_PLUS2 ) || IS_ABSORB( mob, RIS_PLUS3 )
       || IS_ABSORB( mob, RIS_PLUS4 ) || IS_ABSORB( mob, RIS_PLUS5 ) || IS_ABSORB( mob, RIS_PLUS6 ) )
      flags += 3;

   if( IS_ABSORB( mob, RIS_MAGIC ) )
      flags += 3;

   if( IS_ABSORB( mob, RIS_NONMAGIC ) )
      flags += 2;

   if( IS_ATTACK( mob, ATCK_BASH ) )
      flags++;

   if( IS_ATTACK( mob, ATCK_STUN ) )
      flags++;

   if( IS_ATTACK( mob, ATCK_BACKSTAB ) )
      flags += 2;

   if( IS_ATTACK( mob, ATCK_FIREBREATH ) || IS_ATTACK( mob, ATCK_FROSTBREATH )
       || IS_ATTACK( mob, ATCK_ACIDBREATH ) || IS_ATTACK( mob, ATCK_LIGHTNBREATH ) || IS_ATTACK( mob, ATCK_GASBREATH ) )
      flags += 2;

   if( IS_ATTACK( mob, ATCK_EARTHQUAKE ) || IS_ATTACK( mob, ATCK_FIREBALL ) || IS_ATTACK( mob, ATCK_COLORSPRAY ) )
      flags += 2;

   if( IS_ATTACK( mob, ATCK_SPIRALBLAST ) )
      flags += 2;

   if( IS_ATTACK( mob, ATCK_AGE ) )
      flags += 2;

   if( IS_ATTACK( mob, ATCK_BITE ) || IS_ATTACK( mob, ATCK_CLAWS ) || IS_ATTACK( mob, ATCK_TAIL )
       || IS_ATTACK( mob, ATCK_STING ) || IS_ATTACK( mob, ATCK_PUNCH ) || IS_ATTACK( mob, ATCK_KICK )
       || IS_ATTACK( mob, ATCK_TRIP ) || IS_ATTACK( mob, ATCK_GOUGE ) || IS_ATTACK( mob, ATCK_DRAIN )
       || IS_ATTACK( mob, ATCK_POISON ) || IS_ATTACK( mob, ATCK_NASTYPOISON ) || IS_ATTACK( mob, ATCK_GAZE )
       || IS_ATTACK( mob, ATCK_BLINDNESS ) || IS_ATTACK( mob, ATCK_CAUSESERIOUS ) || IS_ATTACK( mob, ATCK_CAUSECRITICAL )
       || IS_ATTACK( mob, ATCK_CURSE ) || IS_ATTACK( mob, ATCK_FLAMESTRIKE ) || IS_ATTACK( mob, ATCK_HARM )
       || IS_ATTACK( mob, ATCK_WEAKEN ) )
      flags++;

   if( IS_DEFENSE( mob, DFND_PARRY ) )
      flags++;

   if( IS_DEFENSE( mob, DFND_DODGE ) )
      flags++;

   if( IS_DEFENSE( mob, DFND_DISPELEVIL ) )
      flags++;

   if( IS_DEFENSE( mob, DFND_DISPELMAGIC ) )
      flags++;

   if( IS_DEFENSE( mob, DFND_DISARM ) )
      flags++;

   if( IS_DEFENSE( mob, DFND_SANCTUARY ) )
      flags++;

   if( IS_DEFENSE( mob, DFND_HEAL ) || IS_DEFENSE( mob, DFND_CURELIGHT ) || IS_DEFENSE( mob, DFND_CURESERIOUS )
       || IS_DEFENSE( mob, DFND_CURECRITICAL ) || IS_DEFENSE( mob, DFND_SHIELD ) || IS_DEFENSE( mob, DFND_BLESS )
       || IS_DEFENSE( mob, DFND_STONESKIN ) || IS_DEFENSE( mob, DFND_TELEPORT ) || IS_DEFENSE( mob, DFND_GRIP )
       || IS_DEFENSE( mob, DFND_TRUESIGHT ) )
      flags++;

   /*
    * Gotta do this because otherwise mobs with negative flags will take xp AWAY from the player. 
    */
   if( flags < 0 )
      flags = 0;

   /*
    * And cap all mobs to no more than 10 flags to keep xp from going haywire 
    */
   if( flags > 10 )
      flags = 10;

   return calculate_mob_exp( mob, flags );
}

void group_gain( CHAR_DATA * ch, CHAR_DATA * victim )
{
   CHAR_DATA *gch, *gch_next, *lch;
   int xp = 0, max_level = 0, members = 0, xpmod = 1;

   /*
    * Monsters don't get kill xp's or alignment changes ( exception: charmies )
    * Dying of mortal wounds or poison doesn't give xp to anyone!
    */
   if( victim == ch )
      return;

   /*
    * We hope this works of course 
    */
   if( IS_NPC( ch ) )
   {
      if( ch->leader )
      {
         if( !IS_NPC( ch->leader ) && ch->leader == ch->master && IS_AFFECTED( ch, AFF_CHARM )
             && ch->in_room == ch->leader->in_room )
            ch = ch->master;
      }
   }

   /*
    * See above. If this is STILL an NPC after that, then yes, we need to bail out now. 
    */
   if( IS_NPC( ch ) )
      return;

   members = 0;
   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( !is_same_group( gch, ch ) )
         continue;

      if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         if( IS_PLR_FLAG( gch, PLR_ONMAP ) || IS_ACT_FLAG( gch, ACT_ONMAP ) )
         {
            if( ch->x != gch->x || ch->y != gch->y )
               continue;
         }
      }

      /*
       * Count members only if they're PCs so charmies don't dillute the kill 
       */
      if( !IS_NPC( gch ) )
      {
         members++;
         max_level = UMAX( max_level, gch->level );
      }
   }
   if( members == 0 )
   {
      bug( "%s", "Group_gain: members." );
      members = 1;
   }

   lch = ch->leader ? ch->leader : ch;
   for( gch = ch->in_room->first_person; gch; gch = gch_next )
   {
      xpmod = 1;
      gch_next = gch->next_in_room;

      if( !is_same_group( gch, ch ) )
         continue;

      if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         if( IS_PLR_FLAG( gch, PLR_ONMAP ) || IS_ACT_FLAG( gch, ACT_ONMAP ) )
         {
            if( ch->x != gch->x || ch->y != gch->y )
               continue;
         }
      }

      if( gch->level - lch->level > 20 )
         xpmod = 5;

      if( gch->level - lch->level < -20 )
         xpmod = 5;

      xp = ( xp_compute( gch, victim ) / members ) / xpmod;

      GET_ALIGN( gch ) = align_compute( gch, victim );
      class_monitor( gch );   /* Alignment monitoring added - Samson 4-17-98 */
      ch_printf( gch, "%sYou receive %d experience points.\n\r", color_str( AT_PLAIN, gch ), xp );
      gain_exp( gch, xp ); /* group gain */
      align_zap( gch );
   }
   return;
}

/*
 * Revamped by Thoric to be more realistic
 * Added code to produce different messages based on weapon type - FB
 * Added better bug message so you can track down the bad dt's -Shaddai
 */
void new_dam_message( CHAR_DATA * ch, CHAR_DATA * victim, double dam, unsigned int dt, OBJ_DATA * obj )
{
   char buf1[256], buf2[256], buf3[256];
   const char *vs;
   const char *vp;
   const char *attack;
   char punct;
   double dampc, d_index;
   struct skill_type *skill = NULL;
   bool gcflag = FALSE;
   bool gvflag = FALSE;
   int w_index;
   ROOM_INDEX_DATA *was_in_room;

   if( !dam )
      dampc = 0;
   else
      dampc = ( ( dam * 1000 ) / victim->max_hit ) + ( 50 - ( ( victim->hit * 50 ) / victim->max_hit ) );

   if( ch->in_room != victim->in_room )
   {
      was_in_room = ch->in_room;
      char_from_room( ch );
      if( !char_to_room( ch, victim->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }
   else
      was_in_room = NULL;

   /*
    * Get the weapon index 
    */
   if( dt > 0 && dt < ( unsigned int )top_sn )
      w_index = 0;
   else if( dt >= TYPE_HIT && dt < TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
      w_index = dt - TYPE_HIT;
   else
   {
      bug( "%s: bad dt %d from %s in %d.", __FUNCTION__, dt, ch->name, ch->in_room->vnum );
      dt = TYPE_HIT;
      w_index = 0;
   }

   /*
    * get the damage index 
    */
   if( dam == 0 )
      d_index = 0;
   else if( dampc < 0 )
      d_index = 1;
   else if( dampc <= 100 )
      d_index = 1 + dampc / 10;
   else if( dampc <= 200 )
      d_index = 11 + ( dampc - 100 ) / 20;
   else if( dampc <= 900 )
      d_index = 16 + ( dampc - 200 ) / 100;
   else
      d_index = 23;

   /*
    * Lookup the damage message 
    */
   vs = s_message_table[w_index][( int )d_index];
   vp = p_message_table[w_index][( int )d_index];

   punct = ( dampc <= 30 ) ? '.' : '!';

   if( dam == 0 && ( IS_PCFLAG( ch, PCFLAG_GAG ) ) )
      gcflag = TRUE;

   if( dam == 0 && ( IS_PCFLAG( victim, PCFLAG_GAG ) ) )
      gvflag = TRUE;

   if( dt >= 0 && dt < ( unsigned int )top_sn )
      skill = skill_table[dt];
   if( dt == TYPE_HIT )
   {
      snprintf( buf1, 256, "$n %s $N%c", vp, punct );
      snprintf( buf2, 256, "You %s $N%c", vs, punct );
      snprintf( buf3, 256, "$n %s you%c", vp, punct );
   }
   else if( dt > TYPE_HIT && is_wielding_poisoned( ch ) )
   {
      if( dt < TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
         attack = attack_table[dt - TYPE_HIT];
      else
      {
         bug( "%s: bad dt %d from %s in %d.", __FUNCTION__, dt, ch->name, ch->in_room->vnum );
         dt = TYPE_HIT;
         attack = attack_table[0];
      }
      snprintf( buf1, 256, "$n's poisoned %s %s $N%c", attack, vp, punct );
      snprintf( buf2, 256, "Your poisoned %s %s $N%c", attack, vp, punct );
      snprintf( buf3, 256, "$n's poisoned %s %s you%c", attack, vp, punct );
   }
   else
   {
      if( skill )
      {
         attack = skill->noun_damage;
         if( dam == 0 )
         {
            bool found = FALSE;

            if( skill->miss_char && skill->miss_char[0] != '\0' )
            {
               act( AT_HIT, skill->miss_char, ch, NULL, victim, TO_CHAR );
               found = TRUE;
            }
            if( skill->miss_vict && skill->miss_vict[0] != '\0' )
            {
               act( AT_HITME, skill->miss_vict, ch, NULL, victim, TO_VICT );
               found = TRUE;
            }
            if( skill->miss_room && skill->miss_room[0] != '\0' )
            {
               if( str_cmp( skill->miss_room, "supress" ) )
                  act( AT_ACTION, skill->miss_room, ch, NULL, victim, TO_NOTVICT );
               found = TRUE;
            }
            if( found ) /* miss message already sent */
            {
               if( was_in_room )
               {
                  char_from_room( ch );
                  if( !char_to_room( ch, was_in_room ) )
                     log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
               }
               return;
            }
         }
         else
         {
            if( skill->hit_char && skill->hit_char[0] != '\0' )
               act( AT_HIT, skill->hit_char, ch, NULL, victim, TO_CHAR );
            if( skill->hit_vict && skill->hit_vict[0] != '\0' )
               act( AT_HITME, skill->hit_vict, ch, NULL, victim, TO_VICT );
            if( skill->hit_room && skill->hit_room[0] != '\0' )
               act( AT_ACTION, skill->hit_room, ch, NULL, victim, TO_NOTVICT );
         }
      }
      else if( dt >= TYPE_HIT && dt < TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
      {
         if( obj )
            attack = obj->short_descr;
         else
            attack = attack_table[dt - TYPE_HIT];
      }
      else
      {
         bug( "%s: bad dt %d from %s in %d.", __FUNCTION__, dt, ch->name, ch->in_room->vnum );
         dt = TYPE_HIT;
         attack = attack_table[0];
      }
      snprintf( buf1, 256, "$n's %s %s $N%c", attack, vp, punct );
      snprintf( buf2, 256, "Your %s %s $N%c", attack, vp, punct );
      snprintf( buf3, 256, "$n's %s %s you%c", attack, vp, punct );
   }

   act( AT_ACTION, buf1, ch, NULL, victim, TO_NOTVICT );
   if( !gcflag )
      act( AT_HIT, buf2, ch, NULL, victim, TO_CHAR );
   if( !gvflag )
      act( AT_HITME, buf3, ch, NULL, victim, TO_VICT );

   if( was_in_room )
   {
      char_from_room( ch );
      if( !char_to_room( ch, was_in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }
   return;
}

#ifndef dam_message
void dam_message( CHAR_DATA * ch, CHAR_DATA * victim, int dam, int dt )
{
   new_dam_message( ch, victim, dam, dt );
}
#endif

/*
 * See if an attack justifies a ATTACKER flag.
 */
void check_attacker( CHAR_DATA * ch, CHAR_DATA * victim )
{
   /*
    * Made some changes to this function Apr 6/96 to reduce the prolifiration
    * of attacker flags in the realms. -Narn
    */
   /*
    * NPC's are fair game.
    */
   if( IS_NPC( victim ) )
      return;

   /*
    * deadly char check 
    */
   if( !IS_NPC( ch ) && !IS_NPC( victim ) && CAN_PKILL( ch ) && CAN_PKILL( victim ) )
      return;

   /*
    * Charm-o-rama.
    */
   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      if( !ch->master )
      {
         bug( "Check_attacker: %s bad AFF_CHARM", IS_NPC( ch ) ? ch->short_descr : ch->name );
         affect_strip( ch, gsn_charm_person );
         REMOVE_AFFECTED( ch, AFF_CHARM );
         return;
      }
      return;
   }

   /*
    * NPC's are cool of course (as long as not charmed).
    * Hitting yourself is cool too (bleeding).
    * So is being immortal (Alander's idea).
    */
   if( IS_NPC( ch ) || ch == victim || ch->level >= LEVEL_IMMORTAL )
      return;

   return;
}

/*
 * Start fights.
 */
void set_fighting( CHAR_DATA * ch, CHAR_DATA * victim )
{
   FIGHT_DATA *fight;

   if( ch->fighting )
   {
      bug( "Set_fighting: %s -> %s (already fighting %s)", ch->name, victim->name, ch->fighting->who->name );
      return;
   }

   if( IS_AFFECTED( ch, AFF_SLEEP ) )
      affect_strip( ch, gsn_sleep );

   /*
    * Limit attackers -Thoric 
    */
   if( victim->num_fighting > max_fight( victim ) )
   {
      send_to_char( "There are too many people fighting for you to join in.\n\r", ch );
      return;
   }

   CREATE( fight, FIGHT_DATA, 1 );
   fight->who = victim;
   ch->num_fighting = 1;
   ch->fighting = fight;

   if( IS_NPC( ch ) )
      ch->position = POS_FIGHTING;
   else
      switch ( ch->style )
      {
         case ( STYLE_EVASIVE ):
            ch->position = POS_EVASIVE;
            break;

         case ( STYLE_DEFENSIVE ):
            ch->position = POS_DEFENSIVE;
            break;

         case ( STYLE_AGGRESSIVE ):
            ch->position = POS_AGGRESSIVE;
            break;

         case ( STYLE_BERSERK ):
            ch->position = POS_BERSERK;
            break;

         default:
            ch->position = POS_FIGHTING;
      }
   victim->num_fighting++;
   if( victim->switched && IS_AFFECTED( victim->switched, AFF_POSSESS ) )
   {
      send_to_char( "You are disturbed!\n\r", victim->switched );
      interpret( victim->switched, "return" );
   }
   add_event( 2, ev_violence, ch );
   return;
}

/*
 * Set position of a victim.
	     */
void update_pos( CHAR_DATA * victim )
{
   if( !victim )
   {
      bug( "%s", "update_pos: null victim" );
      return;
   }

   if( victim->hit > 0 )
   {
      if( victim->position <= POS_STUNNED )
         victim->position = POS_STANDING;
      if( IS_AFFECTED( victim, AFF_PARALYSIS ) )
         victim->position = POS_STUNNED;
      return;
   }

   if( IS_NPC( victim ) || victim->hit <= -11 )
   {
      if( victim->mount )
      {
         act( AT_ACTION, "$n falls from $N.", victim, NULL, victim->mount, TO_ROOM );
         REMOVE_ACT_FLAG( victim->mount, ACT_MOUNTED );
         victim->mount = NULL;
      }
      victim->position = POS_DEAD;
      return;
   }

   if( victim->hit <= -6 )
      victim->position = POS_MORTAL;
   else if( victim->hit <= -3 )
      victim->position = POS_INCAP;
   else
      victim->position = POS_STUNNED;

   if( victim->position > POS_STUNNED && IS_AFFECTED( victim, AFF_PARALYSIS ) )
      victim->position = POS_STUNNED;

   if( victim->mount )
   {
      act( AT_ACTION, "$n falls unconscious from $N.", victim, NULL, victim->mount, TO_ROOM );
      REMOVE_ACT_FLAG( victim->mount, ACT_MOUNTED );
      victim->mount = NULL;
   }
   return;
}

/*
 * See if an attack justifies a KILLER flag.
 *
 * Actually since the killer flag no longer exists, this is just tallying kill totals now - Samson
 */
void check_killer( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( IS_NPC( victim ) )
   {
      if( !IS_NPC( ch ) )
      {
         int level_ratio;

         /*
          * Fix for crashes when killing mobs of level 0
          * * by Joe Fabiano -rinthos@yahoo.com
          * * on 03-16-03.
          */
         if( victim->level < 1 )
            level_ratio = URANGE( 1, ch->level, MAX_LEVEL );
         else
            level_ratio = URANGE( 1, ch->level / victim->level, MAX_LEVEL );
         if( ch->pcdata->clan )
            ch->pcdata->clan->mkills++;
         ch->pcdata->mkills++;
         if( ch->pcdata->deity )
         {
            if( victim->race == ch->pcdata->deity->npcrace )
               adjust_favor( ch, 3, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcrace2 )
               adjust_favor( ch, 22, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcrace3 )
               adjust_favor( ch, 23, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcfoe )
               adjust_favor( ch, 17, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcfoe2 )
               adjust_favor( ch, 24, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcfoe3 )
               adjust_favor( ch, 25, level_ratio );
            else
               adjust_favor( ch, 2, level_ratio );
         }
      }
      return;
   }

   if( ch == victim || ch->level >= LEVEL_IMMORTAL )
      return;

   if( in_arena( ch ) )
   {
      if( !IS_NPC( ch ) && !IS_NPC( victim ) )
      {
         ch->pcdata->pkills++;
         victim->pcdata->pdeaths++;
      }
      return;
   }

   /*
    * clan checks               -Thoric 
    */
   if( IS_PCFLAG( ch, PCFLAG_DEADLY ) && IS_PCFLAG( victim, PCFLAG_DEADLY ) )
   {
      /*
       * not of same clan? 
       */
      if( !ch->pcdata->clan || !victim->pcdata->clan
          || ( ch->pcdata->clan->clan_type != CLAN_NOKILL
               && victim->pcdata->clan->clan_type != CLAN_NOKILL && ch->pcdata->clan != victim->pcdata->clan ) )
      {
         if( ch->pcdata->clan )
         {
            if( victim->level < ( LEVEL_AVATAR * 0.1 ) )
               ch->pcdata->clan->pkills[0]++;
            else if( victim->level < ( LEVEL_AVATAR * 0.2 ) )
               ch->pcdata->clan->pkills[1]++;
            else if( victim->level < ( LEVEL_AVATAR * 0.3 ) )
               ch->pcdata->clan->pkills[2]++;
            else if( victim->level < ( LEVEL_AVATAR * 0.4 ) )
               ch->pcdata->clan->pkills[3]++;
            else if( victim->level < ( LEVEL_AVATAR * 0.5 ) )
               ch->pcdata->clan->pkills[4]++;
            else if( victim->level < ( LEVEL_AVATAR * 0.6 ) )
               ch->pcdata->clan->pkills[5]++;
            else if( victim->level < ( LEVEL_AVATAR * 0.7 ) )
               ch->pcdata->clan->pkills[6]++;
            else if( victim->level < ( LEVEL_AVATAR * 0.8 ) )
               ch->pcdata->clan->pkills[7]++;
            else if( victim->level < ( LEVEL_AVATAR * 0.9 ) )
               ch->pcdata->clan->pkills[8]++;
            else
               ch->pcdata->clan->pkills[9]++;
         }
         ch->pcdata->pkills++;
         ch->hit = ch->max_hit;
         ch->mana = ch->max_mana;
         ch->move = ch->max_move;
         update_pos( victim );
         if( victim != ch )
         {
            act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into $n.", ch, victim->name, NULL, TO_ROOM );
            act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into you.", ch, victim->name, NULL, TO_CHAR );
         }
         if( victim->pcdata->clan )
         {
            if( ch->level < ( LEVEL_AVATAR * 0.1 ) )
               victim->pcdata->clan->pdeaths[0]++;
            else if( ch->level < ( LEVEL_AVATAR * 0.2 ) )
               victim->pcdata->clan->pdeaths[1]++;
            else if( ch->level < ( LEVEL_AVATAR * 0.3 ) )
               victim->pcdata->clan->pdeaths[2]++;
            else if( ch->level < ( LEVEL_AVATAR * 0.4 ) )
               victim->pcdata->clan->pdeaths[3]++;
            else if( ch->level < ( LEVEL_AVATAR * 0.5 ) )
               victim->pcdata->clan->pdeaths[4]++;
            else if( ch->level < ( LEVEL_AVATAR * 0.6 ) )
               victim->pcdata->clan->pdeaths[5]++;
            else if( ch->level < ( LEVEL_AVATAR * 0.7 ) )
               victim->pcdata->clan->pdeaths[6]++;
            else if( ch->level < ( LEVEL_AVATAR * 0.8 ) )
               victim->pcdata->clan->pdeaths[7]++;
            else if( ch->level < ( LEVEL_AVATAR * 0.9 ) )
               victim->pcdata->clan->pdeaths[8]++;
            else
               victim->pcdata->clan->pdeaths[9]++;
         }
         victim->pcdata->pdeaths++;
         adjust_favor( victim, 11, 1 );
         adjust_favor( ch, 2, 1 );
         add_timer( victim, TIMER_PKILLED, 115, NULL, 0 );
         WAIT_STATE( victim, 3 * sysdata.pulseviolence );
         return;
      }
   }

   /*
    * Charm-o-rama.
    */
   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      if( !ch->master )
      {
         bug( "Check_killer: %s bad AFF_CHARM", IS_NPC( ch ) ? ch->short_descr : ch->name );
         affect_strip( ch, gsn_charm_person );
         REMOVE_AFFECTED( ch, AFF_CHARM );
         return;
      }

      /*
       * stop_follower( ch ); 
       */
      if( ch->master )
         check_killer( ch->master, victim );
      return;
   }

   if( IS_NPC( ch ) )
   {
      if( !IS_NPC( victim ) )
      {
         int level_ratio;

         if( victim->pcdata->clan )
            victim->pcdata->clan->mdeaths++;
         victim->pcdata->mdeaths++;
         level_ratio = URANGE( 1, ch->level / victim->level, LEVEL_AVATAR );
         if( victim->pcdata->deity )
         {
            if( ch->race == victim->pcdata->deity->npcrace )
               adjust_favor( victim, 12, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcrace2 )
               adjust_favor( victim, 26, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcrace3 )
               adjust_favor( victim, 27, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcfoe )
               adjust_favor( victim, 15, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcfoe2 )
               adjust_favor( victim, 28, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcfoe3 )
               adjust_favor( victim, 29, level_ratio );
            else
               adjust_favor( victim, 11, level_ratio );
         }
      }
      return;
   }

   if( !IS_NPC( ch ) )
   {
      if( ch->pcdata->clan )
         ch->pcdata->clan->illegal_pk++;
      ch->pcdata->illegal_pk++;
   }
   if( !IS_NPC( victim ) )
   {
      if( victim->pcdata->clan )
      {
         if( ch->level < ( LEVEL_AVATAR * 0.1 ) )
            victim->pcdata->clan->pdeaths[0]++;
         else if( ch->level < ( LEVEL_AVATAR * 0.2 ) )
            victim->pcdata->clan->pdeaths[1]++;
         else if( ch->level < ( LEVEL_AVATAR * 0.3 ) )
            victim->pcdata->clan->pdeaths[2]++;
         else if( ch->level < ( LEVEL_AVATAR * 0.4 ) )
            victim->pcdata->clan->pdeaths[3]++;
         else if( ch->level < ( LEVEL_AVATAR * 0.5 ) )
            victim->pcdata->clan->pdeaths[4]++;
         else if( ch->level < ( LEVEL_AVATAR * 0.6 ) )
            victim->pcdata->clan->pdeaths[5]++;
         else if( ch->level < ( LEVEL_AVATAR * 0.7 ) )
            victim->pcdata->clan->pdeaths[6]++;
         else if( ch->level < ( LEVEL_AVATAR * 0.8 ) )
            victim->pcdata->clan->pdeaths[7]++;
         else if( ch->level < ( LEVEL_AVATAR * 0.9 ) )
            victim->pcdata->clan->pdeaths[8]++;
         else
            victim->pcdata->clan->pdeaths[9]++;
      }
      victim->pcdata->pdeaths++;
   }

   save_char_obj( ch );
   return;
}

/*
 * just verify that a corpse looting is legal
 */
bool legal_loot( CHAR_DATA * ch, CHAR_DATA * victim )
{
   /*
    * anyone can loot mobs 
    */
   if( IS_NPC( victim ) )
      return TRUE;

   /*
    * non-charmed mobs can loot anything 
    */
   if( IS_NPC( ch ) && !ch->master )
      return TRUE;

   /*
    * members of different clans can loot too! -Thoric 
    */
   if( IS_PCFLAG( ch, PCFLAG_DEADLY ) && IS_PCFLAG( victim, PCFLAG_DEADLY ) )
      return TRUE;

   return FALSE;
}

void free_fight( CHAR_DATA * ch )
{
   if( !ch )
   {
      bug( "%s", "Free_fight: null ch!" );
      return;
   }
   if( ch->fighting )
   {
      if( !char_died( ch->fighting->who ) )
         --ch->fighting->who->num_fighting;
      DISPOSE( ch->fighting );
   }
   ch->fighting = NULL;
   if( ch->mount )
      ch->position = POS_MOUNTED;
   else
      ch->position = POS_STANDING;
   /*
    * Berserk wears off after combat. -- Altrag 
    */
   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      affect_strip( ch, gsn_berserk );
      set_char_color( AT_WEAROFF, ch );
      send_to_char( skill_table[gsn_berserk]->msg_off, ch );
      send_to_char( "\n\r", ch );
   }
   return;
}

/*
 * Stop fights.
 */
void stop_fighting( CHAR_DATA * ch, bool fBoth )
{
   CHAR_DATA *fch;

   cancel_event( ev_violence, ch );
   free_fight( ch );
   update_pos( ch );

   if( !fBoth )   /* major short cut here by Thoric */
      return;

   for( fch = first_char; fch; fch = fch->next )
   {
      if( who_fighting( fch ) == ch )
      {
         cancel_event( ev_violence, fch );
         free_fight( fch );
         update_pos( fch );
      }
   }
   return;
}

/* Vnums for the various bodyparts */
int part_vnums[] = { OBJ_VNUM_SEVERED_HEAD,  /* Head */
   OBJ_VNUM_SLICED_ARM, /* arms */
   OBJ_VNUM_SLICED_LEG, /* legs */
   OBJ_VNUM_TORN_HEART, /* heart */
   OBJ_VNUM_BRAINS,  /* brains */
   OBJ_VNUM_SPILLED_GUTS,  /* guts */
   OBJ_VNUM_HANDS,   /* hands */
   OBJ_VNUM_FOOT, /* feet */
   OBJ_VNUM_FINGERS, /* fingers */
   OBJ_VNUM_EAR,  /* ear */
   OBJ_VNUM_EYE,  /* eye */
   OBJ_VNUM_TONGUE,  /* long_tongue */
   OBJ_VNUM_EYESTALK,   /* eyestalks */
   OBJ_VNUM_TENTACLE,   /* tentacles */
   OBJ_VNUM_FINS, /* fins */
   OBJ_VNUM_WINGS,   /* wings */
   OBJ_VNUM_TAIL, /* tail */
   OBJ_VNUM_SCALES,  /* scales */
   OBJ_VNUM_CLAWS,   /* claws */
   OBJ_VNUM_FANGS,   /* fangs */
   OBJ_VNUM_HORNS,   /* horns */
   OBJ_VNUM_TUSKS,   /* tusks */
   OBJ_VNUM_TAIL, /* tailattack */
   OBJ_VNUM_SHARPSCALE, /* sharpscales */
   OBJ_VNUM_BEAK, /* beak */
   OBJ_VNUM_HAUNCHES,   /* haunches */
   OBJ_VNUM_HOOVES,  /* hooves */
   OBJ_VNUM_PAWS, /* paws */
   OBJ_VNUM_FORELEG, /* forelegs */
   OBJ_VNUM_FEATHERS,   /* feathers */
   0, /* r1 */
   0  /* r2 */
};

/* Messages for flinging off the various bodyparts */
char *part_messages[] = {
   "$n's severed head plops from its neck.",
   "$n's arm is sliced from $s dead body.",
   "$n's leg is sliced from $s dead body.",
   "$n's heart is torn from $s chest.",
   "$n's brains spill grotesquely from $s head.",
   "$n's guts spill grotesquely from $s torso.",
   "$n's hand is sliced from $s dead body.",
   "$n's foot is sliced from $s dead body.",
   "A finger is sliced from $n's dead body.",
   "$n's ear is sliced from $s dead body.",
   "$n's eye is gouged from its socket.",
   "$n's tongue is torn from $s mouth.",
   "An eyestalk is sliced from $n's dead body.",
   "A tentacle is severed from $n's dead body.",
   "A fin is sliced from $n's dead body.",
   "A wing is severed from $n's dead body.",
   "$n's tail is sliced from $s dead body.",
   "A scale falls from the body of $n.",
   "A claw is torn from $n's dead body.",
   "$n's fangs are torn from $s mouth.",
   "A horn is wrenched from the body of $n.",
   "$n's tusk is torn from $s dead body.",
   "$n's tail is sliced from $s dead body.",
   "A ridged scale falls from the body of $n.",
   "$n's beak is sliced from $s dead body.",
   "$n's haunches are sliced from $s dead body.",
   "A hoof is sliced from $n's dead body.",
   "A paw is sliced from $n's dead body.",
   "$n's foreleg is sliced from $s dead body.",
   "Some feathers fall from $n's dead body.",
   "r1 message.",
   "r2 message."
};

/*
 * Improved Death_cry contributed by Diavolo.
 * Additional improvement by Thoric (and removal of turds... sheesh!)  
 * Support for additional bodyparts by Fireblade
 */
void death_cry( CHAR_DATA * ch )
{
   char *msg;
   int vnum, shift, rmindex, i;

   if( !ch )
   {
      bug( "%s", "DEATH_CRY: null ch!" );
      return;
   }

   vnum = 0;
   msg = NULL;

   switch ( number_range( 0, 5 ) )
   {
      default:
         msg = "You hear $n's death cry.";
         break;
      case 0:
         msg = "$n screams furiously as $e falls to the ground in a heap!";
         break;
      case 1:
         msg = "$n hits the ground ... DEAD.";
         break;
      case 2:
         msg = "$n catches $s guts in $s hands as they pour through $s fatal wound!";
         break;
      case 3:
         msg = "$n splatters blood on your armor.";
         break;
      case 4:
         msg = "$n gasps $s last breath and blood spurts out of $s mouth and ears.";
         break;
      case 5:
         shift = number_range( 0, 31 );
         rmindex = 1 << shift;

         for( i = 0; i < 32 && ch->xflags; i++ )
         {
            if( HAS_BODYPART( ch, rmindex ) )
            {
               msg = part_messages[shift];
               vnum = part_vnums[shift];
               break;
            }
            else
            {
               shift = number_range( 0, 31 );
               rmindex = 1 << shift;
            }
         }
         if( !msg )
            msg = "You hear $n's death cry.";
         break;
   }

   act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );

   if( vnum )
   {
      OBJ_DATA *obj;
      char *name;

      if( !get_obj_index( vnum ) )
      {
         bug( "death_cry: invalid vnum %d", vnum );
         return;
      }

      name = IS_NPC( ch ) ? ch->short_descr : ch->name;
      if( !( obj = create_object( get_obj_index( vnum ), 0 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      obj->timer = number_range( 4, 7 );
      if( IS_AFFECTED( ch, AFF_POISON ) )
         obj->value[3] = 10;

      stralloc_printf( &obj->short_descr, obj->short_descr, name );
      stralloc_printf( &obj->objdesc, obj->objdesc, name );
      obj_to_room( obj, ch->in_room, ch );
   }
   return;
}

OBJ_DATA *raw_kill( CHAR_DATA * ch, CHAR_DATA * victim )
{
   OBJ_DATA *corpse_to_return = NULL;

   if( !victim )
   {
      bug( "%s: null victim! CH: %s", __FUNCTION__, ch->name );
      return NULL;
   }

   /*
    * backup in case hp goes below 1 
    */
   if( !IS_NPC( victim ) && victim->level == 1 )
   {
      bug( "%s: killing level 1", __FUNCTION__ );
      return NULL;
   }

   stop_fighting( victim, TRUE );

   if( in_arena( victim ) )
   {
      ROOM_INDEX_DATA *location = NULL;

      log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s bested %s in the arena.", ch->name, victim->name );
      ch_printf( ch, "You bested %s in arena combat!\n\r", victim->name );
      ch_printf( victim, "%s bested you in arena combat!\n\r", ch->name );
      victim->hit = 1;
      victim->position = POS_RESTING;

      location = check_room( victim, victim->in_room );  /* added check to see what continent PC is on - Samson 3-29-98 */

      if( !location )
         location = get_room_index( ROOM_VNUM_LIMBO );

      char_from_room( victim );
      if( !char_to_room( victim, location ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      if( IS_AFFECTED( victim, AFF_PARALYSIS ) )
         REMOVE_AFFECTED( victim, AFF_PARALYSIS );

      return NULL;
   }

   /*
    * Take care of morphed characters 
    */
   if( victim->morph )
   {
      do_unmorph_char( victim );
      return raw_kill( ch, victim );
   }

   /*
    * Deal with switched imms 
    */
   if( victim->desc && victim->desc->original )
   {
      CHAR_DATA *temp = victim;

      interpret( victim, "return" );
      return raw_kill( ch, temp );
   }

   mprog_death_trigger( ch, victim );
   if( char_died( victim ) )
      return NULL;

   rprog_death_trigger( ch, victim );
   if( char_died( victim ) )
      return NULL;

   death_cry( victim );
   corpse_to_return = make_corpse( victim, ch );
   if( victim->in_room->sector_type == SECT_OCEANFLOOR
       || victim->in_room->sector_type == SECT_UNDERWATER
       || victim->in_room->sector_type == SECT_WATER_SWIM
       || victim->in_room->sector_type == SECT_WATER_NOSWIM || victim->in_room->sector_type == SECT_RIVER )
      act( AT_BLOOD, "$n's blood slowly clouds the surrounding water.", victim, NULL, NULL, TO_ROOM );
   else if( victim->in_room->sector_type == SECT_AIR )
      act( AT_BLOOD, "$n's blood sprays wildly through the air.", victim, NULL, NULL, TO_ROOM );
   else
      make_blood( victim );

   if( IS_NPC( victim ) )
   {
      victim->pIndexData->killed++;
      extract_char( victim, TRUE );
      victim = NULL;
      return corpse_to_return;
   }

   set_char_color( AT_DIEMSG, victim );
   if( victim->pcdata->mdeaths + victim->pcdata->pdeaths < 3 )
      interpret( victim, "help new_death" );
   else
      interpret( victim, "help _DIEMSG_" );

   extract_char( victim, FALSE );
   if( !victim )
   {
      bug( "oops! %s: extract_char destroyed pc char", __FUNCTION__ );
      return corpse_to_return;
   }
   while( victim->first_affect )
      affect_remove( victim, victim->first_affect );
   victim->affected_by = race_table[victim->race]->affected;
   xCLEAR_BITS( victim->resistant );
   xCLEAR_BITS( victim->susceptible );
   xCLEAR_BITS( victim->immune );
   xCLEAR_BITS( victim->absorb );
   victim->carry_weight = 0;
   victim->armor = 100;
   victim->armor += race_table[victim->race]->ac_plus;
   victim->attacks = race_table[victim->race]->attacks;
   victim->defenses = race_table[victim->race]->defenses;
   victim->mod_str = 0;
   victim->mod_dex = 0;
   victim->mod_wis = 0;
   victim->mod_int = 0;
   victim->mod_con = 0;
   victim->mod_cha = 0;
   victim->mod_lck = 0;
   victim->damroll = 0;
   victim->hitroll = 0;
   victim->mental_state = -10;
   victim->alignment = URANGE( -1000, victim->alignment, 1000 );
   victim->saving_poison_death = race_table[victim->race]->saving_poison_death;
   victim->saving_wand = race_table[victim->race]->saving_wand;
   victim->saving_para_petri = race_table[victim->race]->saving_para_petri;
   victim->saving_breath = race_table[victim->race]->saving_breath;
   victim->saving_spell_staff = race_table[victim->race]->saving_spell_staff;
   victim->position = POS_RESTING;
   victim->hit = UMAX( 1, victim->hit );
   victim->mana = UMAX( 1, victim->mana );
   victim->move = UMAX( 1, victim->move );
   if( victim->pcdata->condition[COND_FULL] != -1 )
      victim->pcdata->condition[COND_FULL] = sysdata.maxcondval / 2;
   if( victim->pcdata->condition[COND_THIRST] != -1 )
      victim->pcdata->condition[COND_THIRST] = sysdata.maxcondval / 2;

   if( IS_SAVE_FLAG( SV_DEATH ) )
      save_char_obj( victim );
   return corpse_to_return;
}

/*
 * Damage an object.						-Thoric
 * Affect player's AC if necessary.
 * Make object into scraps if necessary.
 * Send message about damaged object.
 */
void damage_obj( OBJ_DATA * obj )
{
   CHAR_DATA *ch;

   ch = obj->carried_by;

   separate_obj( obj );
   if( !IS_PKILL( ch ) || ( IS_PKILL( ch ) && !IS_PCFLAG( ch, PCFLAG_GAG ) ) )
      act( AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_CHAR );
   else if( obj->in_room && ( ch = obj->in_room->first_person ) != NULL )
   {
      act( AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_ROOM );
      act( AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_CHAR );
      ch = NULL;
   }

   if( obj->item_type != ITEM_LIGHT )
      oprog_damage_trigger( ch, obj );
   else if( !in_arena( ch ) )
      oprog_damage_trigger( ch, obj );

   if( obj_extracted( obj ) )
      return;

   switch ( obj->item_type )
   {
      default:
         make_scraps( obj );
         break;
      case ITEM_CONTAINER:
      case ITEM_KEYRING:
      case ITEM_QUIVER:
         if( --obj->value[3] <= 0 )
         {
            if( !in_arena( ch ) )
               make_scraps( obj );
            else
               obj->value[3] = 1;
         }
         break;
      case ITEM_LIGHT:
         if( --obj->value[0] <= 0 )
         {
            if( !in_arena( ch ) )
               make_scraps( obj );
            else
               obj->value[0] = 1;
         }
         break;
      case ITEM_ARMOR:
         if( ch && obj->value[0] >= 1 )
            ch->armor += apply_ac( obj, obj->wear_loc );
         if( --obj->value[0] <= 0 )
         {
            if( !IS_PKILL( ch ) && !in_arena( ch ) )
               make_scraps( obj );
            else
            {
               obj->value[0] = 1;
               ch->armor -= apply_ac( obj, obj->wear_loc );
            }
         }
         else if( ch && obj->value[0] >= 1 )
            ch->armor -= apply_ac( obj, obj->wear_loc );
         break;
      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
         if( --obj->value[6] <= 0 )
         {
            if( !IS_PKILL( ch ) && !in_arena( ch ) )
               make_scraps( obj );
            else
               obj->value[6] = 1;
         }
         break;
      case ITEM_PROJECTILE:
         if( --obj->value[5] <= 0 )
         {
            if( !IS_PKILL( ch ) && !in_arena( ch ) )
               make_scraps( obj );
            else
               obj->value[5] = 1;
         }
         break;
   }
   if( ch != NULL )
      save_char_obj( ch ); /* Stop scrap duping - Samson 1-2-00 */

   return;
}

/*
 * Inflict damage from a hit.   This is one damn big function.
 */
/* Function modified from original form - Samson */
/* Eliminated clan_type GUILD 11-30-98 */
ch_ret damage( CHAR_DATA * ch, CHAR_DATA * victim, double dam, int dt )
{
   short dameq, maxdam, dampmod;
   bool npcvict, loot;
   OBJ_DATA *damobj;
   ch_ret retcode;
   /*
    * CHAR_DATA *gch, *lch; 
    */
   /*
    * short anopc = 0;  * # of (non-pkill) pc in a (ch) 
    */
   /*
    * short bnopc = 0;  * # of (non-pkill) pc in b (victim) 
    */

   retcode = rNONE;

   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return rERROR;
   }
   if( !victim )
   {
      bug( "%s: null victim!", __FUNCTION__ );
      return rVICT_DIED;
   }

   if( victim->position == POS_DEAD )
      return rVICT_DIED;

   npcvict = IS_NPC( victim );

   /*
    * Check Align types for RIS - Heath ;)
    */
   if( IS_GOOD( ch ) && IS_EVIL( victim ) )
      dam = ris_damage( victim, dam, RIS_GOOD );
   else if( IS_EVIL( ch ) && IS_GOOD( victim ) )
      dam = ris_damage( victim, dam, RIS_EVIL );

   /*
    * Check damage types for RIS            -Thoric
    */
   if( dam && dt != TYPE_UNDEFINED )
   {
      if( IS_FIRE( dt ) )
         dam = ris_damage( victim, dam, RIS_FIRE );
      else if( IS_COLD( dt ) )
         dam = ris_damage( victim, dam, RIS_COLD );
      else if( IS_ACID( dt ) )
         dam = ris_damage( victim, dam, RIS_ACID );
      else if( IS_ELECTRICITY( dt ) )
         dam = ris_damage( victim, dam, RIS_ELECTRICITY );
      else if( IS_ENERGY( dt ) )
         dam = ris_damage( victim, dam, RIS_ENERGY );
      else if( IS_DRAIN( dt ) )
         dam = ris_damage( victim, dam, RIS_DRAIN );
      else if( dt == gsn_poison || IS_POISON( dt ) )
         dam = ris_damage( victim, dam, RIS_POISON );
      else
         /*
          * Added checks for the 3 new dam types, and removed DAM_PEA - Grimm 
          * Removed excess duplication, added hack and lash RIS types - Samson 1-9-00 
          */
      if( dt == ( TYPE_HIT + DAM_CRUSH ) )
         dam = ris_damage( victim, dam, RIS_BLUNT );
      else if( dt == ( TYPE_HIT + DAM_STAB ) || dt == ( TYPE_HIT + DAM_PIERCE ) || dt == ( TYPE_HIT + DAM_THRUST ) )
         dam = ris_damage( victim, dam, RIS_PIERCE );
      else if( dt == ( TYPE_HIT + DAM_SLASH ) )
         dam = ris_damage( victim, dam, RIS_SLASH );
      else if( dt == ( TYPE_HIT + DAM_HACK ) )
         dam = ris_damage( victim, dam, RIS_HACK );
      else if( dt == ( TYPE_HIT + DAM_LASH ) )
         dam = ris_damage( victim, dam, RIS_LASH );

      if( dam == -1 )
      {
         if( dt >= 0 && dt < top_sn )
         {
            bool found = FALSE;
            SKILLTYPE *skill = skill_table[dt];

            if( skill->imm_char && skill->imm_char[0] != '\0' )
            {
               act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
               found = TRUE;
            }
            if( skill->imm_vict && skill->imm_vict[0] != '\0' )
            {
               act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
               found = TRUE;
            }
            if( skill->imm_room && skill->imm_room[0] != '\0' )
            {
               act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
               found = TRUE;
            }
            if( found )
               return rNONE;
         }
         dam = 0;
      }
   }

   /*
    * Precautionary step mainly to prevent people in Hell from finding
    * a way out. --Shaddai
    */
   if( IS_ROOM_FLAG( victim->in_room, ROOM_SAFE ) )
      dam = 0;

   if( dam && npcvict && ch != victim )
   {
      if( !IS_ACT_FLAG( victim, ACT_SENTINEL ) )
      {
         if( victim->hunting )
         {
            if( victim->hunting->who != ch )
            {
               STRFREE( victim->hunting->name );
               victim->hunting->name = QUICKLINK( ch->name );
               victim->hunting->who = ch;
            }
         }
         else if( !IS_ACT_FLAG( victim, ACT_PACIFIST ) ) /* Gorog */
            start_hunting( victim, ch );
      }

      if( victim->hating )
      {
         if( victim->hating->who != ch )
         {
            STRFREE( victim->hating->name );
            victim->hating->name = QUICKLINK( ch->name );
            victim->hating->who = ch;
         }
      }
      else if( !IS_ACT_FLAG( victim, ACT_PACIFIST ) ) /* Gorog */
         start_hating( victim, ch );
   }

   /*
    * Stop up any residual loopholes.
    */
   maxdam = ch->level * 30;
   if( dt == gsn_backstab )
      maxdam = ch->level * 80;
   if( dam > maxdam )
   {
      bug( "Damage: %d more than %d points!", ( int )dam, maxdam );
      bug( "** %s (lvl %d) -> %s **", ch->name, ch->level, victim->name );
      dam = maxdam;
   }

   if( victim != ch )
   {
      /*
       * Certain attacks are forbidden.
       * Most other attacks are returned.
       */
      if( is_safe( ch, victim ) )
         return rNONE;

      check_attacker( ch, victim );

      if( victim->position > POS_STUNNED )
      {
         if( !victim->fighting && victim->in_room == ch->in_room )
            set_fighting( victim, ch );

         /*
          * vwas: victim->position = POS_FIGHTING; 
          */
         if( IS_NPC( victim ) && victim->fighting )
            victim->position = POS_FIGHTING;
         else if( victim->fighting )
         {
            switch ( victim->style )
            {
               case ( STYLE_EVASIVE ):
                  victim->position = POS_EVASIVE;
                  break;
               case ( STYLE_DEFENSIVE ):
                  victim->position = POS_DEFENSIVE;
                  break;
               case ( STYLE_AGGRESSIVE ):
                  victim->position = POS_AGGRESSIVE;
                  break;
               case ( STYLE_BERSERK ):
                  victim->position = POS_BERSERK;
                  break;
               default:
                  victim->position = POS_FIGHTING;
            }

         }

      }

      if( victim->position > POS_STUNNED )
      {
         if( !ch->fighting && victim->in_room == ch->in_room )
            set_fighting( ch, victim );

         /*
          * If victim is charmed, ch might attack victim's master.
          */
         if( IS_NPC( ch ) && npcvict && IS_AFFECTED( victim, AFF_CHARM ) && victim->master
             && victim->master->in_room == ch->in_room && number_bits( 3 ) == 0 )
         {
            stop_fighting( ch, FALSE );
            retcode = multi_hit( ch, victim->master, TYPE_UNDEFINED );
            return retcode;
         }
      }

      /*
       * More charm stuff.
       */
      if( victim->master == ch )
         unbind_follower( victim, ch );

      /*
       * Pkill stuff.  If a deadly attacks another deadly or is attacked by
       * one, then ungroup any nondealies.  Disabled untill I can figure out
       * the right way to do it.
       * Someone enabled this? No no no, lets not do that. It's buggy and makes no sense! - Samson
       *
       
       * count the # of non-pkill pc in a ( not including == ch ) *
       for (gch = ch->in_room->first_person; gch; gch = gch->next_in_room)
       if (is_same_group(ch, gch) && !IS_NPC(gch)
       && !IS_PKILL(gch) && (ch != gch))
       anopc++;
       
       * count the # of non-pkill pc in b ( not including == victim ) *
       for( gch = victim->in_room->first_person; gch; gch = gch->next_in_room )
       if( is_same_group( victim, gch ) && !IS_NPC(gch) && !IS_PKILL(gch) && ( victim != gch ) )
       bnopc++;
       
       * only consider disbanding if both groups have 1(+) non-pk pc,
       or when one participant is pc, and the other group has 1(+)
       pk pc's (in the case that participant is only pk pc in group) 
       *
       if( ( bnopc > 0 && anopc > 0 ) || ( bnopc > 0 && !IS_NPC(ch) ) || (anopc > 0 && !IS_NPC( victim ) ) )
       {
       * Disband from same group first *
       if( is_same_group( ch, victim ) )
       {
       * Messages to char and master handled in stop_follower *
       act( AT_ACTION, "$n disbands from $N's group.", (ch->leader == victim) ? victim : ch, NULL,
       (ch->leader == victim) ? victim->master : ch->master, TO_NOTVICT );
       if( ch->leader == victim )
       stop_follower(victim);
       else
       stop_follower(ch);
       }
       
       * if leader isnt pkill, leave the group and disband ch *
       if( ch->leader != NULL && !IS_NPC(ch->leader) && !IS_PKILL( ch->leader ) )
       {
       act(AT_ACTION, "$n disbands from $N's group.", ch, NULL, ch->master, TO_NOTVICT);
       stop_follower(ch);
       }
       else
       {
       for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
       if (is_same_group(gch, ch) && !IS_NPC(gch) && !IS_PKILL(gch) && gch != ch )
       {
       act(AT_ACTION, "$n disbands from $N's group.", ch, NULL, gch->master, TO_NOTVICT);
       stop_follower(gch);
       }
       }
       * if leader isnt pkill, leave the group and disband victim *
       if( victim->leader != NULL && !IS_NPC(victim->leader) && !IS_PKILL( victim->leader ) )
       {
       act( AT_ACTION, "$n disbands from $N's group.", victim, NULL, victim->master, TO_NOTVICT );
       stop_follower(victim);
       }
       else
       {
       for( gch = victim->in_room->first_person; gch; gch = gch->next_in_room )
       if( is_same_group(gch, victim) && !IS_NPC(gch) && !IS_PKILL(gch) && gch != victim )
       {
       act( AT_ACTION, "$n disbands from $N's group.", gch, NULL, gch->master, TO_NOTVICT );
       stop_follower(gch);
       }
       }
       }
       END OF PKILL BLOCK */

      /*
       * Inviso attacks ... not.
       */
      if( IS_AFFECTED( ch, AFF_INVISIBLE ) )
      {
         affect_strip( ch, gsn_invis );
         affect_strip( ch, gsn_mass_invis );
         REMOVE_AFFECTED( ch, AFF_INVISIBLE );
         act( AT_MAGIC, "$n fades into existence.", ch, NULL, NULL, TO_ROOM );
      }

      /*
       * Take away Hide 
       */
      if( is_affected( ch, gsn_hide ) || IS_AFFECTED( ch, AFF_HIDE ) )
      {
         affect_strip( ch, gsn_hide );
         REMOVE_AFFECTED( ch, AFF_HIDE );
      }

      /*
       * Take away Sneak 
       */
      if( is_affected( ch, gsn_sneak ) || IS_AFFECTED( ch, AFF_SNEAK ) )
      {
         affect_strip( ch, gsn_sneak );
         REMOVE_AFFECTED( ch, AFF_SNEAK );
      }

      /*
       * Damage modifiers.
       */
      if( IS_AFFECTED( victim, AFF_SANCTUARY ) )
         dam /= 2;

      if( IS_AFFECTED( victim, AFF_PROTECT ) && IS_EVIL( ch ) )
         dam -= ( int )( dam / 4 );

      if( dam < 0 )
         dam = 0;


      /*
       * Check for disarm, trip, parry, and dodge.
       */
      if( dt >= TYPE_HIT && ch->in_room == victim->in_room )
      {
         if( IS_DEFENSE( ch, DFND_DISARM ) && ch->level > 9 && number_percent(  ) < ch->level / 3 )
            disarm( ch, victim );

         if( IS_ATTACK( ch, ATCK_TRIP ) && ch->level > 5 && number_percent(  ) < ch->level / 2 )
            trip( ch, victim );

         if( check_parry( ch, victim ) )
            return rNONE;
         if( check_dodge( ch, victim ) )
            return rNONE;
         if( check_tumble( ch, victim ) )
            return rNONE;
      }

      /*
       * Check control panel settings and modify damage
       */
      if( IS_NPC( ch ) )
      {
         if( npcvict )
            dampmod = sysdata.dam_mob_vs_mob;
         else
            dampmod = sysdata.dam_mob_vs_plr;
      }
      else
      {
         if( npcvict )
            dampmod = sysdata.dam_plr_vs_mob;
         else
            dampmod = sysdata.dam_plr_vs_plr;
      }
      if( dampmod > 0 )
         dam = ( dam * dampmod ) / 100;
   }

   /*
    * Code to handle equipment getting damaged, and also support  -Thoric
    * bonuses/penalties for having or not having equipment where hit
    */
   if( dam > 10 && dt != TYPE_UNDEFINED )
   {
      /*
       * get a random body eq part 
       */
      dameq = number_range( WEAR_LIGHT, WEAR_ANKLE_R );
      damobj = get_eq_char( victim, dameq );
      if( damobj )
      {
         if( dam > get_obj_resistance( damobj ) && number_bits( 1 ) == 0 )
            damage_obj( damobj );
         dam -= 5;   /* add a bonus for having something to block the blow */
      }
      else
         dam += 5;   /* add penalty for bare skin! */
   }

   if( ch != victim )
      dam_message( ch, victim, dam, dt );

   /*
    * Hurt the victim.
    * Inform the victim of his new state.
    */
   victim->hit -= ( int )dam;

   if( !IS_NPC( victim ) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1 )
      victim->hit = 1;

   /*
    * Make sure newbies dont die 
    */
   if( !IS_NPC( victim ) && victim->level == 1 && victim->hit < 1 )
      victim->hit = 1;

   if( dam > 0 && dt > TYPE_HIT && !IS_AFFECTED( victim, AFF_POISON ) && is_wielding_poisoned( ch )
       && !IS_IMMUNE( victim, RIS_POISON ) && !saves_poison_death( ch->level, victim ) )
   {
      AFFECT_DATA af;

      af.type = gsn_poison;
      af.duration = 20;
      af.location = APPLY_STR;
      af.modifier = -2;
      af.bit = AFF_POISON;
      affect_join( victim, &af );
      victim->mental_state = URANGE( 20, victim->mental_state + 2, 100 );
   }

   if( !npcvict && victim->level >= LEVEL_IMMORTAL && ch->level >= LEVEL_IMMORTAL && victim->hit < 1 )
      victim->hit = 1;

   update_pos( victim );

   switch ( victim->position )
   {
      case POS_MORTAL:
         act( AT_DYING, "$n is mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_ROOM );
         act( AT_DANGER, "You are mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_CHAR );
         break;

      case POS_INCAP:
         act( AT_DYING, "$n is incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_ROOM );
         act( AT_DANGER, "You are incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_CHAR );
         break;

      case POS_STUNNED:
         if( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
         {
            act( AT_ACTION, "$n is stunned, but will probably recover.", victim, NULL, NULL, TO_ROOM );
            act( AT_HURT, "You are stunned, but will probably recover.", victim, NULL, NULL, TO_CHAR );
         }
         break;

      case POS_DEAD:
         if( dt >= 0 && dt < top_sn )
         {
            SKILLTYPE *skill = skill_table[dt];

            if( skill->die_char && skill->die_char[0] != '\0' )
               act( AT_DEAD, skill->die_char, ch, NULL, victim, TO_CHAR );
            if( skill->die_vict && skill->die_vict[0] != '\0' )
               act( AT_DEAD, skill->die_vict, ch, NULL, victim, TO_VICT );
            if( skill->die_room && skill->die_room[0] != '\0' )
               act( AT_DEAD, skill->die_room, ch, NULL, victim, TO_NOTVICT );
         }
         act( AT_DEAD, "$n is DEAD!!", victim, 0, 0, TO_ROOM );
         act( AT_DEAD, "You have been KILLED!!\n\r", victim, 0, 0, TO_CHAR );
         break;

      default:
         if( dam > victim->max_hit / 4 )
         {
            act( AT_HURT, "That really did HURT!", victim, 0, 0, TO_CHAR );
            if( number_bits( 3 ) == 0 )
               worsen_mental_state( victim, 1 );   /* Bug fix - Samson 9-24-98 */
         }
         if( victim->hit < victim->max_hit / 4 )

         {
            act( AT_DANGER, "You wish that your wounds would stop BLEEDING so much!", victim, 0, 0, TO_CHAR );
            if( number_bits( 2 ) == 0 )
               worsen_mental_state( victim, 1 );   /* Bug fix - Samson 9-24-98 */
         }
         break;
   }

   /*
    * Payoff for killing things.
    */
   if( victim->position == POS_DEAD )
   {
      OBJ_DATA *new_corpse;

      if( !in_arena( victim ) )
         group_gain( ch, victim );

      if( !npcvict )
      {
         log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s (%d) killed by %s at %d",
                          victim->name, victim->level, ( IS_NPC( ch ) ? ch->short_descr : ch->name ),
                          victim->in_room->vnum );

         if( !IS_NPC( ch ) && !IS_IMMORTAL( ch )
             && ch->pcdata->clan && ch->pcdata->clan->clan_type != CLAN_ORDER && victim != ch )
         {
            if( victim->pcdata->clan && victim->pcdata->clan->name == ch->pcdata->clan->name )
               ;
            else
            {
               char filename[256];

               snprintf( filename, 256, "%s%s.record", CLAN_DIR, ch->pcdata->clan->name );
               append_to_file( filename, "&P(%2d) %-12s &wvs &P(%2d) %s &P%s ... &w%s",
                               ch->level, ch->name, victim->level, !CAN_PKILL( victim ) ? "&W<Peaceful>" :
                               victim->pcdata->clan ? victim->pcdata->clan->badge : "&P(&WUnclanned&P)",
                               victim->name, ch->in_room->area->name );
            }
         }

         /*
          * Dying penalty:
          * 1/4 way back to previous level.
          */
         if( !in_arena( victim ) )
         {
            if( victim->exp > exp_level( victim->level ) )
               gain_exp( victim, ( exp_level( victim->level ) - victim->exp ) / 4 );
         }

      }

      check_killer( ch, victim );

      if( !IS_NPC( ch ) && ch->pcdata->clan )
         update_member( ch );
      if( !IS_NPC( victim ) && victim->pcdata->clan )
         update_member( victim );

      if( ch->in_room == victim->in_room )
         loot = legal_loot( ch, victim );
      else
         loot = FALSE;

      new_corpse = raw_kill( ch, victim );
      victim = NULL;

      if( !IS_NPC( ch ) && loot && new_corpse && new_corpse->item_type == ITEM_CORPSE_NPC
       && new_corpse->in_room == ch->in_room && can_see_obj( ch, new_corpse, false ) && ch->position > POS_SLEEPING )
      {
         /*
          * Autogold by Scryn 8/12 
          */
         if( IS_PLR_FLAG( ch, PLR_AUTOGOLD ) && !loot_coins_from_corpse( ch, new_corpse ) )
            return rCHAR_DIED;

         if( !obj_extracted(new_corpse) )
         {
            if( IS_PLR_FLAG( ch, PLR_AUTOLOOT ) && victim != ch ) /* prevent nasty obj problems -- Blodkai */
               interpret( ch, "get all corpse" );

            if( IS_PLR_FLAG( ch, PLR_SMARTSAC ) )
            {
               OBJ_DATA *corpse;

               if( ( corpse = get_obj_here( ch, "corpse" ) ) != NULL )
               {
                  if( !corpse->first_content )
                     interpret( ch, "sacrifice corpse" );
                  else
                     interpret( ch, "look in corpse" );
               }
            }
            else if( IS_PLR_FLAG( ch, PLR_AUTOSAC ) && !IS_PLR_FLAG( ch, PLR_SMARTSAC ) )
               interpret( ch, "sacrifice corpse" );
         }
      }

      if( IS_SAVE_FLAG( SV_KILL ) )
         save_char_obj( ch );
      return rVICT_DIED;
   }

   if( victim == ch )
      return rNONE;

   /*
    * Take care of link dead people.
    */
   if( !npcvict && !victim->desc && !IS_PCFLAG( victim, PCFLAG_NORECALL ) )
   {
      if( number_range( 0, victim->wait ) == 0 )
      {
         recall( victim, -1 );
         return rNONE;
      }
   }

   /*
    * Wimp out?
    */
   if( npcvict && dam > 0 )
   {
      if( ( IS_ACT_FLAG( victim, ACT_WIMPY ) && number_bits( 1 ) == 0 && victim->hit < victim->max_hit / 2 )
          || ( IS_AFFECTED( victim, AFF_CHARM ) && victim->master && victim->master->in_room != victim->in_room ) )
      {
         start_fearing( victim, ch );
         stop_hunting( victim );
         interpret( victim, "flee" );
      }
   }

   if( !npcvict && victim->hit > 0 && victim->hit <= victim->wimpy && victim->wait == 0 )
      interpret( victim, "flee" );
   else if( !npcvict && IS_PLR_FLAG( victim, PLR_FLEE ) )
      interpret( victim, "flee" );

   tail_chain(  );
   return rNONE;
}

/*
 * Hit one guy once.
 */
ch_ret one_hit( CHAR_DATA * ch, CHAR_DATA * victim, int dt )
{
   OBJ_DATA *wield;
   double victim_ac, dam;
   int thac0, plusris, diceroll, attacktype, cnt, prof_bonus, prof_gsn = -1;
   ch_ret retcode = rNONE;
   AFFECT_DATA *aff;

   /*
    * Can't beat a dead char!
    * Guard against weird room-leavings.
    */

   if( victim->position == POS_DEAD || ch->in_room != victim->in_room )
      return rVICT_DIED;

   used_weapon = NULL;
   /*
    * Figure out the weapon doing the damage         -Thoric
    */
   if( ( wield = get_eq_char( ch, WEAR_DUAL_WIELD ) ) != NULL )
   {
      if( dual_flip == TRUE )
      {
         dual_flip = FALSE;
      }
      else
      {
         dual_flip = TRUE;
         wield = get_eq_char( ch, WEAR_WIELD );
      }
   }
   else
      wield = get_eq_char( ch, WEAR_WIELD );

   used_weapon = wield;

   if( wield )
      prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );
   else
      prof_bonus = 0;

   /*
    * Lets hope this works, the called function here defaults out to gsn_pugilism, and since
    * every Class can learn it, this should theoretically make it advance with use now 
    */
   prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );

   if( !( alreadyUsedSkill ) )
   {
      if( ch->fighting && dt == TYPE_UNDEFINED && IS_NPC( ch ) && !xIS_EMPTY( ch->attacks ) )
      {
         cnt = 0;
         for( ;; )
         {
            attacktype = number_range( 0, 6 );
            if( IS_ATTACK( ch, attacktype ) )
               break;
            if( cnt++ > 16 )
            {
               attacktype = -1;
               break;
            }
         }
         if( attacktype == ATCK_BACKSTAB )
            attacktype = -1;
         if( wield && number_percent(  ) > 25 )
            attacktype = -1;
         if( !wield && number_percent(  ) > 50 )
            attacktype = -1;
         switch ( attacktype )
         {
            default:
               break;
            case ATCK_BITE:
               interpret( ch, "bite" );
               retcode = global_retcode;
               break;
            case ATCK_CLAWS:
               interpret( ch, "claw" );
               retcode = global_retcode;
               break;
            case ATCK_TAIL:
               interpret( ch, "tail" );
               retcode = global_retcode;
               break;
            case ATCK_STING:
               interpret( ch, "sting" );
               retcode = global_retcode;
               break;
            case ATCK_PUNCH:
               interpret( ch, "punch" );
               retcode = global_retcode;
               break;
            case ATCK_KICK:
               interpret( ch, "kick" );
               retcode = global_retcode;
               break;
            case ATCK_TRIP:
               attacktype = 0;
               break;
         }
         alreadyUsedSkill = TRUE;
         if( attacktype >= 0 )
            return retcode;
      }
   }

   if( dt == TYPE_UNDEFINED )
   {
      dt = TYPE_HIT;
      if( wield && wield->item_type == ITEM_WEAPON )
         dt += wield->value[3];
   }

   /*
    * Go grab the thac0 value 
    */
   thac0 = calc_thac0( ch, victim, 0 );

   /*
    * Get the victim's armor Class 
    */
   victim_ac = UMAX( -19, ( int )( GET_AC( victim ) / 10 ) );  /* Use -19 or AC / 10 */

   /*
    * if you can't see what's coming... 
    */
   if( wield && !can_see_obj( victim, wield, FALSE ) )
      victim_ac += 1;
   if( !can_see( ch, victim, FALSE ) )
      victim_ac -= 4;

   /*
    * "learning" between combatients.  Takes the intelligence difference,
    * and multiplies by the times killed to make up a learning bonus
    * given to whoever is more intelligent            -Thoric
    */
   if( ch->fighting && ch->fighting->who == victim )
   {
      short times = ch->fighting->timeskilled;

      if( times )
      {
         short intdiff = get_curr_int( ch ) - get_curr_int( victim );

         if( intdiff != 0 )
            victim_ac += ( intdiff * times ) / 10;
      }
   }

   /*
    * Weapon proficiency bonus 
    */
   victim_ac += prof_bonus;

   /*
    * Faerie Fire Fix - Tarl 10 Dec 02 
    */
   if( IS_AFFECTED( victim, AFF_FAERIE_FIRE ) )
   {
      victim_ac = victim_ac + 20;
   }

   /*
    * The following section allows fighting style to modify AC. 
    * Added by Tarl 26 Mar 02   
    */

   if( victim->position == POS_BERSERK )
      victim_ac = 0.8 * victim_ac;
   else if( victim->position == POS_AGGRESSIVE )
      victim_ac = 0.85 * victim_ac;
   else if( victim->position == POS_DEFENSIVE )
      victim_ac = 1.1 * victim_ac;
   else if( victim->position == POS_EVASIVE )
      victim_ac = 1.2 * victim_ac;

   /*
    * The moment of excitement!
    */
   while( ( diceroll = number_bits( 5 ) ) >= 20 )
      ;

   if( diceroll == 0 || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
   {
      /*
       * Miss. 
       */
      if( prof_gsn != -1 )
         learn_from_failure( ch, prof_gsn );
      damage( ch, victim, 0, dt );
      tail_chain(  );
      return rNONE;
   }

   /*
    * Hit.
    * Calc damage.
    */

   if( !wield )   /* dice formula fixed by Thoric */
      dam = dice( ch->barenumdie, ch->baresizedie );
   else
   {
      dam = number_range( wield->value[1], wield->value[2] );
      if( ch->Class == CLASS_MONK ) /* Monks get extra damage - Samson 5-31-99 */
         dam += ( ch->level / 10 );
   }

   /*
    * Bonuses.
    */
   dam += GET_DAMROLL( ch );

   if( prof_bonus )
      dam += prof_bonus / 4;

   /*
    * Calculate Damage Modifiers from Victim's Fighting Style
    */
   if( victim->position == POS_BERSERK )
      dam = 1.2 * dam;
   else if( victim->position == POS_AGGRESSIVE )
      dam = 1.1 * dam;
   else if( victim->position == POS_DEFENSIVE )
      dam = .85 * dam;
   else if( victim->position == POS_EVASIVE )
      dam = .8 * dam;

   /*
    * Calculate Damage Modifiers from Attacker's Fighting Style
    */
   if( ch->position == POS_BERSERK )
      dam = 1.2 * dam;
   else if( ch->position == POS_AGGRESSIVE )
      dam = 1.1 * dam;
   else if( ch->position == POS_DEFENSIVE )
      dam = .85 * dam;
   else if( ch->position == POS_EVASIVE )
      dam = .8 * dam;

   if( !IS_NPC( ch ) && ch->pcdata->learned[gsn_enhanced_damage] > 0
       && number_percent(  ) < ch->pcdata->learned[gsn_enhanced_damage] )
   {
      dam += ( int )( dam * LEARNED( ch, gsn_enhanced_damage ) / 120 );
   }
   else
      learn_from_failure( ch, gsn_enhanced_damage );

   if( !IS_AWAKE( victim ) )
      dam *= 2;
   if( dt == gsn_backstab )
      dam *= ( 1 + ( ch->level / 4 ) );
   if( dt == gsn_circle )
      dam *= ( 1 + ( ch->level / 8 ) );

   dam = UMAX( dam, 1 );

   plusris = 0;

   if( wield )
   {
      if( IS_OBJ_FLAG( wield, ITEM_MAGIC ) )
         dam = ris_damage( victim, dam, RIS_MAGIC );
      else
         dam = ris_damage( victim, dam, RIS_NONMAGIC );

      /*
       * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll -Thoric
       */
      plusris = obj_hitroll( wield );
   }
   else if( ch->Class == CLASS_MONK )
   {
      dam = ris_damage( victim, dam, RIS_NONMAGIC );

      if( ch->level <= MAX_LEVEL )
         plusris = 3;

      if( ch->level < 80 )
         plusris = 2;

      if( ch->level < 50 )
         plusris = 1;

      if( ch->level < 30 )
         plusris = 0;
   }
   else
      dam = ris_damage( victim, dam, RIS_NONMAGIC );

   /*
    * check for RIS_PLUSx                -Thoric 
    */

   if( dam )
   {
      int x, res, imm, sus, mod;

      if( plusris )
         plusris = RIS_PLUS1 << UMIN( plusris, 7 );

      /*
       * initialize values to handle a zero plusris 
       */
      imm = res = -1;
      sus = 1;

      /*
       * find high ris 
       *//*
       * FIND ME 
       */
      for( x = RIS_PLUS1; x <= RIS_PLUS6; x <<= 1 )
      {
         if( IS_IMMUNE( victim, x ) )
            imm = x;
         if( IS_RESIS( victim, x ) )
            res = x;
         if( IS_SUSCEP( victim, x ) )
            sus = x;
      }
      mod = 10;
      if( imm >= plusris )
         mod -= 10;
      if( res >= plusris )
         mod -= 2;
      if( sus <= plusris )
         mod += 2;

      /*
       * check if immune 
       */
      if( mod <= 0 )
         dam = -1;
      if( mod != 10 )
         dam = ( dam * mod ) / 10;
   }

   /*
    * immune to damage 
    */
   if( dam == -1 )
   {
      if( dt >= 0 && dt < top_sn )
      {
         SKILLTYPE *skill = skill_table[dt];
         bool found = FALSE;

         if( skill->imm_char && skill->imm_char[0] != '\0' )
         {
            act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
            found = TRUE;
         }
         if( skill->imm_vict && skill->imm_vict[0] != '\0' )
         {
            act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
            found = TRUE;
         }
         if( skill->imm_room && skill->imm_room[0] != '\0' )
         {
            act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
            found = TRUE;
         }
         if( found )
            return rNONE;
      }
      dam = 0;
   }
   if( wield )
   {
      for( aff = wield->first_affect; aff; aff = aff->next )
      {
         if( aff->location == APPLY_RACE_SLAYER )
         {
            if( ( aff->modifier == GET_RACE( victim ) ) ||
                ( ( aff->modifier == RACE_UNDEAD ) && IsUndead( victim ) ) ||
                ( ( aff->modifier == RACE_DRAGON ) && IsDragon( victim ) ) ||
                ( ( aff->modifier == RACE_GIANT ) && IsGiantish( victim ) ) )
               dam *= 2;
         }
         if( aff->location == APPLY_ALIGN_SLAYER )
            switch ( aff->modifier )
            {
               case 0:
                  if( IS_EVIL( victim ) )
                     dam *= 2;
                  break;
               case 1:
                  if( IS_NEUTRAL( victim ) )
                     dam *= 2;
                  break;
               case 2:
                  if( IS_GOOD( victim ) )
                     dam *= 2;
                  break;
               default:
                  break;
            }
      }
   }

   if( ( retcode = damage( ch, victim, dam, dt ) ) != rNONE )
      return retcode;
   if( char_died( ch ) )
      return rCHAR_DIED;
   if( char_died( victim ) )
      return rVICT_DIED;

   retcode = rNONE;
   if( dam == 0 )
      return retcode;

   /*
    * Weapon spell support            -Thoric
    * Each successful hit casts a spell
    */
   if( wield && !IS_IMMUNE( victim, RIS_MAGIC ) && !IS_ROOM_FLAG( victim->in_room, ROOM_NO_MAGIC ) )
   {
      AFFECT_DATA *af;

      for( af = wield->pIndexData->first_affect; af; af = af->next )
         if( af->location == APPLY_WEAPONSPELL && IS_VALID_SN( af->modifier ) && skill_table[af->modifier]->spell_fun )
            retcode = ( *skill_table[af->modifier]->spell_fun ) ( af->modifier, 7, ch, victim );
      if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
         return retcode;
      for( af = wield->first_affect; af; af = af->next )
         if( af->location == APPLY_WEAPONSPELL && IS_VALID_SN( af->modifier ) && skill_table[af->modifier]->spell_fun )
            retcode = ( *skill_table[af->modifier]->spell_fun ) ( af->modifier, 7, ch, victim );
      if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
         return retcode;
   }

   /*
    * magic shields that retaliate          -Thoric
    */
   /*
    * Redone in dale fashion   -Heath, 1-18-98
    */

   if( IS_AFFECTED( victim, AFF_BLADEBARRIER ) && !IS_AFFECTED( ch, AFF_BLADEBARRIER ) )
      retcode = spell_smaug( skill_lookup( "blades" ), victim->level, victim, ch );
   if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
      return retcode;

   if( IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( ch, AFF_FIRESHIELD ) )
      retcode = spell_smaug( skill_lookup( "flare" ), victim->level, victim, ch );
   if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
      return retcode;

   if( IS_AFFECTED( victim, AFF_ICESHIELD ) && !IS_AFFECTED( ch, AFF_ICESHIELD ) )
      retcode = spell_smaug( skill_lookup( "iceshard" ), victim->level, victim, ch );
   if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
      return retcode;

   if( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) && !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
      retcode = spell_smaug( skill_lookup( "torrent" ), victim->level, victim, ch );
   if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
      return retcode;

   if( IS_AFFECTED( victim, AFF_ACIDMIST ) && !IS_AFFECTED( ch, AFF_ACIDMIST ) )
      retcode = spell_smaug( skill_lookup( "acidshot" ), victim->level, victim, ch );
   if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
      return retcode;

   if( IS_AFFECTED( victim, AFF_VENOMSHIELD ) && !IS_AFFECTED( ch, AFF_VENOMSHIELD ) )
      retcode = spell_smaug( skill_lookup( "venomshot" ), victim->level, victim, ch );
   if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
      return retcode;

   tail_chain(  );
   return retcode;
}

/*
 * Do one group of attacks.
 */
ch_ret multi_hit( CHAR_DATA * ch, CHAR_DATA * victim, int dt )
{
   float x = 0.0;
   int hchance;
   ch_ret retcode;

   /*
    * add timer to pkillers 
    */
   if( CAN_PKILL( ch ) && CAN_PKILL( victim ) )
   {
      add_timer( ch, TIMER_RECENTFIGHT, 11, NULL, 0 );
      add_timer( victim, TIMER_RECENTFIGHT, 11, NULL, 0 );
   }

   if( IS_NPC( ch ) )
      alreadyUsedSkill = FALSE;

   if( is_attack_supressed( ch ) )
      return rNONE;

   if( IS_ACT_FLAG( ch, ACT_NOATTACK ) )
      return rNONE;

   if( ( retcode = one_hit( ch, victim, dt ) ) != rNONE )
      return retcode;

   if( who_fighting( ch ) != victim || dt == gsn_backstab || dt == gsn_circle )
      return rNONE;

   /*
    * Very high chance of hitting compared to chance of going berserk 
    * 40% or higher is always hit.. don't learn anything here though. 
    * -- Altrag 
    */
   hchance = IS_NPC( ch ) ? 100 : ( LEARNED( ch, gsn_berserk ) * 5 / 2 );
   if( IS_AFFECTED( ch, AFF_BERSERK ) && number_percent(  ) < hchance )
      if( ( retcode = one_hit( ch, victim, dt ) ) != rNONE || who_fighting( ch ) != victim )
         return retcode;

   x = ch->numattacks;

   if( IS_AFFECTED( ch, AFF_HASTE ) )
      x *= 2;

   if( IS_AFFECTED( ch, AFF_SLOW ) )
      x /= 2;

   x -= 1.0;

   while( x > 0.9999 )
   {
      /*
       * Moved up here by Tarl, 14 April 02 
       */
      if( get_eq_char( ch, WEAR_DUAL_WIELD ) )
      {
         hchance = IS_NPC( ch ) ? ch->level : ch->pcdata->learned[gsn_dual_wield];
         if( number_percent(  ) < hchance )
         {
            if( !get_eq_char( ch, WEAR_WIELD ) )
            {
               bug( "!WEAR_WIELD in multi_hit in fight.c: %s", ch->name );
               return rNONE;
            }
            /*
             * dual_flip = TRUE;  
             */
            retcode = one_hit( ch, victim, dt );
            if( retcode != rNONE || who_fighting( ch ) != victim )
               return retcode;
         }
         else
            learn_from_failure( ch, gsn_dual_wield );
      }
      else
      {
         retcode = one_hit( ch, victim, dt );
         if( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
      }
      x -= 1.0;
   }

   if( x > 0.01 )
      if( number_percent(  ) > ( 100 * x ) )
      {
         retcode = one_hit( ch, victim, dt );
         if( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
      }
   retcode = rNONE;

   hchance = IS_NPC( ch ) ? ( int )( ch->level / 2 ) : 0;
   if( number_percent(  ) < hchance )
      retcode = one_hit( ch, victim, dt );

   return retcode;
}

CMDF do_assist( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim, *bob;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Assist whom?\n\r", ch );
      return;
   }

   if( ( bob = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( !( victim = who_fighting( bob ) ) )
   {
      send_to_char( "They aren't fighting anyone!\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You hit yourself.  Ouch!\n\r", ch );
      return;
   }

   if( !IS_NPC( victim ) && !in_arena( victim ) )
   {
      if( !CAN_PKILL( ch ) )
      {
         send_to_char( "You are not a pkiller!\n\r", ch );
         return;
      }
      if( !CAN_PKILL( victim ) )
      {
         send_to_char( "You cannot engage them in combat. They are not a pkiller.\n\r", ch );
         return;
      }
   }

   if( is_safe( ch, victim ) )
      return;

   if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
   {
      act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      send_to_char( "You do the best you can!\n\r", ch );
      return;
   }

   WAIT_STATE( ch, 1 * sysdata.pulseviolence );
   check_attacker( ch, victim );
   multi_hit( ch, victim, TYPE_UNDEFINED );
   return;
}

CMDF do_kill( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Kill whom?\n\r", ch );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) && victim->morph && !in_arena( victim ) )
   {
      send_to_char( "This creature appears strange to you. Look upon it more closely before attempting to kill it.", ch );
      return;
   }

   if( !IS_NPC( victim ) && !in_arena( victim ) )
   {
      if( !CAN_PKILL( ch ) )
      {
         send_to_char( "You are not a pkiller!\n\r", ch );
         return;
      }
      if( !CAN_PKILL( victim ) )
      {
         send_to_char( "You cannot engage them in combat. They are not a pkiller.\n\r", ch );
         return;
      }
   }

   if( victim == ch )
   {
      send_to_char( "You hit yourself. Ouch!\n\r", ch );
      multi_hit( ch, ch, TYPE_UNDEFINED );
      return;
   }

   if( is_safe( ch, victim ) )
      return;

   if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
   {
      act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
      return;
   }
   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      send_to_char( "You do the best you can!\n\r", ch );
      return;
   }
   WAIT_STATE( ch, 2 * sysdata.pulseviolence );
   check_attacker( ch, victim );
   multi_hit( ch, victim, TYPE_UNDEFINED );
   return;
}

CMDF do_flee( CHAR_DATA * ch, char *argument )
{
   ROOM_INDEX_DATA *was_in, *now_in;
   double los;
   int attempt, oldmap = ch->map, oldx = ch->x, oldy = ch->y;
   short door;
   EXIT_DATA *pexit;

   if( !who_fighting( ch ) )
   {
      if( ch->position > POS_SITTING && ch->position < POS_STANDING )
      {
         if( ch->mount )
            ch->position = POS_MOUNTED;
         else
            ch->position = POS_STANDING;
      }
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }
   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      send_to_char( "Flee while berserking?  You aren't thinking very clearly...\n\r", ch );
      return;
   }
   if( ch->move <= 0 )
   {
      send_to_char( "You're too exhausted to flee from combat!\n\r", ch );
      return;
   }
   /*
    * No fleeing while more aggressive than standard or hurt. - Haus 
    */
   if( !IS_NPC( ch ) && ch->position < POS_FIGHTING )
   {
      send_to_char( "You can't flee in an aggressive stance...\n\r", ch );
      return;
   }
   if( IS_NPC( ch ) && ch->position <= POS_SLEEPING )
      return;
   was_in = ch->in_room;
   for( attempt = 0; attempt < 8; attempt++ )
   {
      door = number_door(  );
      if( ( pexit = get_exit( was_in, door ) ) == NULL
          || !pexit->to_room || IS_EXIT_FLAG( pexit, EX_NOFLEE )
          || ( IS_EXIT_FLAG( pexit, EX_CLOSED ) && !IS_AFFECTED( ch, AFF_PASS_DOOR ) )
          || ( IS_NPC( ch ) && IS_ROOM_FLAG( pexit->to_room, ROOM_NO_MOB ) ) )
         continue;

      if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED ) || IS_EXIT_FLAG( pexit, EX_HEAVY )
            || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT ) || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) )
         continue;

      if( ch->mount && ch->mount->fighting )
         stop_fighting( ch->mount, TRUE );

      move_char( ch, pexit, 0, door, FALSE );

      if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         now_in = ch->in_room;
         if( ch->map == oldmap && ch->x == oldx && ch->y == oldy )
            continue;
      }
      else
      {
         if( ( now_in = ch->in_room ) == was_in )
            continue;
      }
      ch->in_room = was_in;
      act( AT_FLEE, "$n flees head over heels!", ch, NULL, NULL, TO_ROOM );
      ch->in_room = now_in;
      act( AT_FLEE, "$n glances around for signs of pursuit.", ch, NULL, NULL, TO_ROOM );
      if( !IS_NPC( ch ) )
      {
         CHAR_DATA *wf = who_fighting( ch );
         int fchance = 0;
         los = ( exp_level( ch->level + 1 ) - exp_level( ch->level ) ) * 0.03;

         if( wf )
            fchance = ( ( get_curr_dex( wf ) + get_curr_str( wf ) + wf->level )
                        - ( get_curr_dex( ch ) + get_curr_str( ch ) + ch->level ) );

         if( ( number_percent(  ) + fchance ) < ch->pcdata->learned[gsn_retreat] )
         {
            ch->in_room = was_in;
            act( AT_FLEE, "You skillfuly retreat from combat.", ch, NULL, NULL, TO_CHAR );
            ch->in_room = now_in;
            act( AT_FLEE, "$n skillfully retreats from combat.", ch, NULL, NULL, TO_ROOM );
         }
         else
         {
            if( ch->level < LEVEL_AVATAR )
               act_printf( AT_FLEE, ch, NULL, NULL, TO_CHAR,
                           "You flee head over heels from combat, losing %d experience.", los );
            else
               act( AT_FLEE, "You flee head over heels from combat!", ch, NULL, NULL, TO_CHAR );
            gain_exp( ch, 0 - los );
            if( ch->level >= skill_table[gsn_retreat]->skill_level[ch->Class] )
               learn_from_failure( ch, gsn_retreat );
         }

         if( wf && ch->pcdata->deity )
         {
            int level_ratio = URANGE( 1, wf->level / ch->level, LEVEL_AVATAR );

            if( wf && wf->race == ch->pcdata->deity->npcrace )
               adjust_favor( ch, 1, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcrace2 )
               adjust_favor( ch, 18, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcrace3 )
               adjust_favor( ch, 19, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcfoe )
               adjust_favor( ch, 16, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcfoe2 )
               adjust_favor( ch, 20, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcfoe3 )
               adjust_favor( ch, 21, level_ratio );

            else
               adjust_favor( ch, 0, level_ratio );
         }
      }
      stop_fighting( ch, TRUE );
      return;
   }
   los = ( exp_level( ch->level + 1 ) - exp_level( ch->level ) ) * 0.01;
   if( ch->level < LEVEL_AVATAR )
      act_printf( AT_FLEE, ch, NULL, NULL, TO_CHAR, "You attempt to flee from combat, losing %d experience.\n\r", los );
   else
      act( AT_FLEE, "You attempt to flee from combat, but can't escape!", ch, NULL, NULL, TO_CHAR );
   gain_exp( ch, 0 - los );
   if( ch->level >= skill_table[gsn_retreat]->skill_level[ch->Class] )
      learn_from_failure( ch, gsn_retreat );
   return;
}
