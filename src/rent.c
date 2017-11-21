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
 *                        Rent and Camping Module                           *
 ****************************************************************************/

#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include "mud.h"
#include "auction.h"
#include "clans.h"
#include "connhist.h"
#include "hotboot.h"
#include "new_auth.h"
#include "overland.h"



/* Change this to an appropriate vnum on your mud, this is where players will be
   loaded for the code to update their rent charges - Samson 1-24-00 */


#ifdef MULTIPORT
extern bool compilelock;
#endif
extern bool bootlock;
int num_quotes;   /* for quotes */
#define QUOTE_FILE "quotes.dat"

AUTH_LIST *get_auth_name( char *name );
void check_pfiles( time_t reset );
void save_clan( CLAN_DATA * clan );
void update_connhistory( DESCRIPTOR_DATA * d, int type );
void show_stateflags( CHAR_DATA * ch );

typedef struct quote_data QUOTE_DATA;

struct quote_data
{
   QUOTE_DATA *next;
   QUOTE_DATA *prev;
   char *quote;
   int number;
};

QUOTE_DATA *first_quote;
QUOTE_DATA *last_quote;

/* Calculates rent value for object affects, used for automatic rent calculation */
int calc_aff_rent( int location, int mod )
{
   int calc = 0, x;

   /*
    * No sense in going through all this for a modifier that does nothing, eh? 
    */
   if( mod == 0 || location == APPLY_NONE )
      return 0;

   /*
    * ooOOooOOooOOoo, yes, I know, another awful looking switch! :P
    * * Don't change those multipliers for mods of < 0 either, since the mod itself is negative,
    * * it will produce the desired affect of returning a negative rent value.
    */
   switch ( location )
   {
      default:
         return 0;

      case APPLY_STR:
      case APPLY_DEX:
         if( mod < 0 )
            return ( 5000 * mod );
         else
            return ( 6000 * mod );

      case APPLY_INT:
      case APPLY_WIS:
         if( mod < 0 )
            return ( 3000 * mod );
         else
            return ( 4000 * mod );

      case APPLY_CON:
         if( mod < 0 )
            return ( 4000 * mod );
         else
            return ( 5000 * mod );

      case APPLY_AGE:
         return ( 1000 * ( mod / 20 ) );

      case APPLY_MANA:
         if( mod > 75 )
            return -2;
         if( mod > 20 && mod <= 75 )
            return ( 2000 * mod );
         return ( UMAX( sysdata.minrent * -1, 1000 * mod ) );

      case APPLY_HIT:
         if( mod > 50 )
            return -2;
         if( mod > 20 && mod <= 50 )
            return ( 3000 * mod );
         if( mod > 10 && mod <= 20 )
            return ( 2000 * mod );
         return ( UMAX( sysdata.minrent * -1, 1000 * mod ) );

      case APPLY_MOVE:
         if( mod < 0 )
            return ( 1000 * ( mod / 10 ) );
         else
            return ( 1000 * ( mod / 5 ) );

      case APPLY_AC:
         if( mod > 12 )
            return -2;
         else
            return ( -1000 * mod ); /* Negative AC is good.... */

      case APPLY_BACKSTAB:
      case APPLY_GOUGE:
      case APPLY_DISARM:
      case APPLY_BASH:
      case APPLY_STUN:
      case APPLY_GRIP:
         if( mod > 10 )
            return -2;
         else
            return ( 1000 * mod );

      case APPLY_SF:   /* spellfail */
         if( mod < -25 )
            return -2;
         else
            return ( -1000 * mod ); /* Not a typo */

      case APPLY_KICK:
      case APPLY_PUNCH:
         if( mod > 20 )
            return -2;
         else
            return ( 1000 * mod );

      case APPLY_HIT_REGEN:
      case APPLY_MANA_REGEN:
         if( mod > 25 )
            return -2;
         else
            return ( 1000 * mod );

      case APPLY_HITROLL:
         if( mod > 6 )
            return -2;
         else
            return ( 2000 * mod );

      case APPLY_DODGE:
      case APPLY_PARRY:
         if( mod > 5 )
            return -2;
         else
            return ( 2000 * mod );

      case APPLY_SCRIBE:
      case APPLY_BREW:
         if( mod > 10 )
            return -2;
         else
            return ( 2000 * mod );

      case APPLY_DAMROLL:
         if( mod > 6 )
            return -2;
         else
            return ( 3000 * mod );

      case APPLY_HITNDAM:
         if( mod > 6 )
            return -2;
         else
            return ( 5000 * mod );

      case APPLY_SAVING_ALL:
         return ( -10000 * mod );   /* Not a typo */

      case APPLY_ATTACKS:
      case APPLY_ALLSTATS:
         return ( 20000 * mod );

      case APPLY_SAVING_POISON:
      case APPLY_SAVING_ROD:
      case APPLY_SAVING_PARA:
      case APPLY_SAVING_BREATH:
      case APPLY_SAVING_SPELL:
         if( mod < 0 )
            return ( -3000 * mod ); /* No, this wasn't a typo */
         else
            return ( -2000 * mod ); /* No, this wasn't a typo either */

      case APPLY_CHA:
         if( mod < 0 )
            return ( 2000 * mod );
         else
            return ( 3000 * mod );

      case APPLY_AFFECT:  /* Otta be some way to fiddle here too */
         return 0;

      case APPLY_RESISTANT:
         for( x = 0; x < 32; x++ )
            if( IS_SET( mod, 1 << x ) )
               calc += 20000;
         if( calc > 40000 )
            return -2;
         else
            return calc;

      case APPLY_IMMUNE:
      case APPLY_STRIPSN:
         return -1;

      case APPLY_SUSCEPTIBLE:
         for( x = 0; x < 32; x++ )
            if( IS_SET( mod, 1 << x ) )
               calc++;
         if( calc == 1 )
            return -15000;
         else if( calc == 2 )
            return -25000;
         else
            return 0;

      case APPLY_ABSORB:
         for( x = 0; x < 32; x++ )
            if( IS_SET( mod, 1 << x ) )
               calc++;
         if( calc == 1 )
            return 50000;
         else if( calc > 1 )
            return -2;
         else
            return 0;

      case APPLY_WEAPONSPELL:
      case APPLY_WEARSPELL:
      case APPLY_REMOVESPELL:
      case APPLY_EAT_SPELL:
         if( IS_VALID_SN( mod ) )
            return ( skill_table[mod]->rent );
         else
            return 0;

      case APPLY_LCK:
         if( mod < 0 )
            return ( 4000 * mod );
         else
            return ( 5000 * mod );

      case APPLY_PICK:
      case APPLY_TRACK:
      case APPLY_STEAL:
      case APPLY_SNEAK:
      case APPLY_HIDE:
      case APPLY_DETRAP:
      case APPLY_SEARCH:
      case APPLY_CLIMB:
      case APPLY_DIG:
      case APPLY_COOK:
      case APPLY_PEEK:
      case APPLY_EXTRAGOLD:
         if( mod > 20 )
            return -2;
         else
            return ( 1000 * ( mod / 2 ) );

      case APPLY_MOUNT:
         if( mod > 50 )
            return -2;
         else
            return ( 1000 * ( mod / 5 ) );

      case APPLY_MOVE_REGEN:
         if( mod > 50 )
            return -2;
         else
            return ( 1000 * ( mod / 5 ) );
   }
}

int set_obj_rent( OBJ_INDEX_DATA * obj )
{
   AFFECT_DATA *paf;
   int rent = 0, calc = 0;

   if( IS_OBJ_FLAG( obj, ITEM_DEATHROT ) )
      return 0;

   for( paf = obj->first_affect; paf; paf = paf->next )
   {
      calc = calc_aff_rent( paf->location, paf->modifier );
      if( calc == -2 )
      {
         switch ( paf->location )
         {
            default:
               if( sysdata.RENT )
               {
                  bug( "set_obj_rent: Affect %s exceeds maximum rent specs. Object %d",
                       affect_loc_name( paf->location ), obj->vnum );
               }
               break;

            case APPLY_WEAPONSPELL:
            case APPLY_WEARSPELL:
            case APPLY_REMOVESPELL:
            case APPLY_EAT_SPELL:
               if( sysdata.RENT )
               {
                  bug( "set_obj_rent: Item spell %s exceeds allowable rent specs. Object %d",
                       IS_VALID_SN( paf->modifier ) ? skill_table[paf->modifier]->name : "unknown", obj->vnum );
               }
               break;

            case APPLY_RESISTANT:
            case APPLY_IMMUNE:
            case APPLY_SUSCEPTIBLE:
            case APPLY_ABSORB:
               if( sysdata.RENT )
                  bug( "set_obj_rent: Item RISA flags exceed allowable rent specs. Object %d", obj->vnum );
               break;
         }
         return -2;
      }
      if( calc == -1 )
         return -1;
      rent += calc;
   }

   if( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_MISSILE_WEAPON || obj->item_type == ITEM_PROJECTILE )
   {
      calc = ( obj->value[1] + obj->value[2] ) / 2;
      if( calc == 15 || calc == 16 )
         rent += 20000;
      else if( calc > 16 )
         rent += 25000;
   }

   if( rent >= sysdata.minrent - 5000 && rent <= sysdata.minrent - 1 )
      rent = sysdata.minrent - 1;

   if( rent < 0 )
      rent = 0;

   return rent;
}

/*
 * Calculates rent for rare items, no display to player. Installed 1-14-98 by Samson
 * Code courtesy of Mudmen - Desolation of the Dragon 2 Mud
 * Used by comm.c to calculate rent cost when player logs on
 */
void rent_calculate( CHAR_DATA * ch, OBJ_DATA * obj, int *rent )
{
   OBJ_DATA *tobj;
   if( obj->rent >= sysdata.minrent )
      *rent += obj->rent * obj->count;
   for( tobj = obj->first_content; tobj; tobj = tobj->next_content )
      rent_calculate( ch, tobj, rent );
}

/*
 * Calculates rent for rare items, displays cost to player. 
 * Installed 1-14-98 by Samson. Code courtesy of Mudmen - Desolation of the Dragon 2 Mud
 * Used in do_offer to tell player how much rent they're going to be charged.
 */
void rent_display( CHAR_DATA * ch, OBJ_DATA * obj, int *rent )
{
   OBJ_DATA *tobj;
   if( obj->rent >= sysdata.minrent )
   {
      *rent += obj->rent * obj->count;
      ch_printf( ch, "%s:\t%d coins per day.\n\r", obj->short_descr, obj->rent );
   }
   for( tobj = obj->first_content; tobj; tobj = tobj->next_content )
      rent_display( ch, tobj, rent );
}

/* Removes rare items the player cannot afford to maintain.
   Installed 1-14-98 by Samson. Code courtesy of Mudmen - Desolation of the Dragon 2 Mud
   Used during login */
void rent_check( CHAR_DATA * ch, OBJ_DATA * obj )
{
   OBJ_DATA *tobj;
   if( obj->rent >= sysdata.minrent )
      extract_obj( obj );
   for( tobj = obj->first_content; tobj; tobj = tobj->next_content )
      rent_check( ch, tobj );
}

/*
 * Calculates rent for rare items, displays cost to player, removes -1 rent items from
 * player's inventory. Added to rent system by Samson on 2-5-98.
 * Used by do_rent when player is renting from the game.
 */
void rent_leaving( CHAR_DATA * ch, OBJ_DATA * obj, int *rent )
{
   OBJ_DATA *tobj;

   if( obj->rent >= sysdata.minrent )
   {
      *rent += obj->rent * obj->count;
      ch_printf( ch, "%s:\t%d coins per day.\n\r", obj->short_descr, obj->rent );
   }
   if( obj->rent == -1 )
   {
      if( obj->wear_loc != WEAR_NONE )
         unequip_char( ch, obj );
      separate_obj( obj );
      ch_printf( ch, "%s dissapears in a cloud of smoke!\n\r", obj->short_descr );
      extract_obj( obj );
   }
   for( tobj = obj->first_content; tobj; tobj = tobj->next_content )
      rent_leaving( ch, tobj, rent );
}

void free_quotes( void )
{
   QUOTE_DATA *quote, *quote_next;

   for( quote = first_quote; quote; quote = quote_next )
   {
      quote_next = quote->next;

      DISPOSE( quote->quote );
      UNLINK( quote, first_quote, last_quote, next, prev );
      DISPOSE( quote );
   }
}

QUOTE_DATA *get_quote( int q )
{
   QUOTE_DATA *quote;

   for( quote = first_quote; quote; quote = quote->next )
      if( quote->number == q )
         return quote;
   return NULL;
}

void save_quotes( void )
{
   QUOTE_DATA *quote;
   FILE *fp;
   char filename[256];
   int q = 0;

   snprintf( filename, 256, "%s%s", SYSTEM_DIR, QUOTE_FILE );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s", "Unable to open quote file for writing!" );
      perror( filename );
      return;
   }

   for( quote = first_quote; quote; quote = quote->next )
   {
      q++;
      fprintf( fp, "%s", "#QUOTE\n" );
      fprintf( fp, "Quote: %s~\n", quote->quote );
      fprintf( fp, "%s", "End\n\n" );
      quote->number = q;
   }
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
   num_quotes = q;
   return;
}

void fread_quote( QUOTE_DATA * quote, FILE * fp )
{
   const char *word;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'Q':
            KEY( "Quote:", quote->quote, fread_string_nohash( fp ) );
            break;
      }
      if( !fMatch )
         bug( "Fread_quote: no match: %s", word );
   }
}

/** Function: load_quotes
  * Descr   : Determines how many (if any) quote files are located within
  *           QUOTE_DIR, for later use by "do_quotes".
  * Returns : (void)
  * Syntax  : (none)
  * Written : v1.0 12/97
  * Author  : Gary McNickle <gary@dharvest.com>
  *
  * Completely rewritten by Samson so it can be OLC'able. 10-15-03
  */
void load_quotes( void )
{
   char filename[256];
   QUOTE_DATA *quote;
   FILE *fp;

   first_quote = NULL;
   last_quote = NULL;
   num_quotes = 0;

   snprintf( filename, 256, "%s%s", SYSTEM_DIR, QUOTE_FILE );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      log_string( "Loading quotes file..." );
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s", "load_quotes: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "QUOTE" ) )
         {
            CREATE( quote, QUOTE_DATA, 1 );
            fread_quote( quote, fp );
            if( !quote->quote || quote->quote[0] == '\0' )
            {
               DISPOSE( quote->quote );
               DISPOSE( quote );
               continue;
            }
            quote->number = ++num_quotes;
            LINK( quote, first_quote, last_quote, next, prev );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "load_quotes: bad section: %s", word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   return;
}

char *add_linebreak( const char *str )
{
   static char newstr[MSL];
   int i, j;

   for( i = j = 0; str[i] != '\0'; i++ )
   {
      if( str[i] == '~' )
      {
         newstr[j++] = '\n';
         newstr[j++] = '\r';
      }
      else
         newstr[j++] = str[i];
   }
   newstr[j] = '\0';
   return newstr;
}

/** Function: do_quotes
  * Descr   : Outputs an ascii file "quote.#" to the player. The number (#)
  *           is determined at random, based on how many quote files are
  *           stored in the QUOTE_DIR directory.
  * Returns : (void)
  * Syntax  : (none)
  * Written : v1.0 12/97
  * Author  : Gary McNickle <gary@dharvest.com>    
  *
  * Completely rewritten by Samson so it can be OLC'able. 10-15-03
  */
CMDF do_quoteset( CHAR_DATA * ch, char *argument )
{
   QUOTE_DATA *quote;
   char arg[MIL];
   int q;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: quoteset add <quote>\n\r", ch );
      send_to_char( "Usage: quoteset remove <quote#>\n\r", ch );
      send_to_char( "Usage: quoteset list\n\r", ch );
      send_to_char( "\n\rTo add a line break, insert a tilde (~) to signify it.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "list" ) )
   {
      if( num_quotes == 0 )
      {
         send_to_char( "There are no quotes to list.\n\r", ch );
         return;
      }
      for( quote = first_quote; quote; quote = quote->next )
         pager_printf( ch, "&cQuote #%d:\n\r &W%s&D\n\r\n\r", quote->number, quote->quote );
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "add" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         do_quoteset( ch, "" );
         return;
      }
      argument = add_linebreak( argument );
      CREATE( quote, QUOTE_DATA, 1 );
      quote->quote = str_dup( argument );
      LINK( quote, first_quote, last_quote, next, prev );
      save_quotes(  );
      ch_printf( ch, "Quote #%d has been added to the list.\n\r", quote->number );
      return;
   }

   if( str_cmp( arg, "remove" ) || !is_number( argument ) )
   {
      do_quoteset( ch, "" );
      return;
   }

   q = atoi( argument );
   quote = get_quote( q );
   if( !quote )
   {
      send_to_char( "No quote by that number exists!\n\r", ch );
      return;
   }
   UNLINK( quote, first_quote, last_quote, next, prev );
   DISPOSE( quote->quote );
   save_quotes(  );
   ch_printf( ch, "Quote #%d has been removed from the list.\n\r", q );
   return;
}

void quotes( CHAR_DATA * ch )
{
   QUOTE_DATA *quote;
   int q;

   if( num_quotes == 0 )
      return;

   /*
    * Lamers! How can you not like quotes! 
    */
   if( IS_PCFLAG( ch, PCFLAG_NOQUOTE ) )
      return;

   q = number_range( 1, num_quotes );
   quote = get_quote( q );
   if( !quote )
   {
      bug( "Missing quote #%d ?!?", q );
      return;
   }
   ch_printf( ch, "&W\n\r%s&d\n\r", quote->quote );
   return;
}

CMDF do_quotes( CHAR_DATA * ch, char *argument )
{
   quotes( ch );
   return;
}

void char_leaving( CHAR_DATA * ch, int howleft, int cost )
{
   OBJ_DATA *obj, *obj_next;  /* If they quit to leave */
   int x, y;
   AUTH_LIST *old_auth = NULL;

   /*
    * new auth 
    */
   old_auth = get_auth_name( ch->name );
   if( old_auth != NULL )
      if( old_auth->state == AUTH_ONLINE )
         old_auth->state = AUTH_OFFLINE;  /* Logging off */

   /*
    * Default conditions 
    */
   ch->pcdata->rent = 0;
   ch->pcdata->norares = FALSE;
   ch->pcdata->autorent = FALSE;

   if( howleft == 2 )   /* Failed autorent */
      ch->pcdata->autorent = TRUE;

   ch->pcdata->camp = howleft;

   if( howleft == 0 )   /* Rented at an inn */
   {
      switch ( ch->in_room->area->continent )
      {
         case ACON_ALSHEROK:
            ch->pcdata->alsherok = ch->in_room->vnum;
            break;
         case ACON_ELETAR:
            ch->pcdata->eletar = ch->in_room->vnum;
            break;
         case ACON_ALATIA:
            ch->pcdata->alatia = ch->in_room->vnum;
            break;
         default:
            break;
      }
   }
   else if( howleft > 1 )  /* Either type of autorent or quit puts recalls back to default */
   {
      ch->pcdata->alsherok = ROOM_VNUM_TEMPLE;
      ch->pcdata->eletar = ROOM_VNUM_ELETAR_RECALL;
      ch->pcdata->alatia = ROOM_VNUM_ALATIA_RECALL;
   }

   /*
    * Get 'em dismounted until we finish mount saving -- Blodkai, 4/97 
    */
   if( ch->position == POS_MOUNTED )
      interpret( ch, "dismount" );

   if( ch->morph )
      interpret( ch, "revert" );

   if( howleft == 4 && cost > 0 )   /* They quit, strip their rent items down! Only used if Rent system is active. */
   {
      /*
       * New code added here by Samson on 1-14-98: Character is unequipped here 
       */
      for( obj = ch->first_carrying; obj != NULL; obj = obj_next )
      {
         obj_next = obj->next_content;
         if( obj->wear_loc != WEAR_NONE && obj->rent >= sysdata.minrent )
            unequip_char( ch, obj );
      }

      /*
       * New code added here by Samson on 1-14-98: Character now drops all rent level eq 
       */
      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         if( obj->rent < sysdata.minrent )
            continue;

         separate_obj( obj );
         obj_from_char( obj );
         if( !obj_next )
            obj_next = ch->first_carrying;
         obj = obj_to_room( obj, ch->in_room, ch );
      }
   }

   if( ch->desc )
   {
      if( ch->timer > 24 )
         update_connhistory( ch->desc, CONNTYPE_IDLE );
      else
         update_connhistory( ch->desc, CONNTYPE_QUIT );
   }

   quotes( ch );
   quitting_char = ch;
   save_char_obj( ch );

   if( sysdata.save_pets )
   {
      CHAR_DATA *pet, *pet_next;

      for( pet = ch->first_pet; pet; pet = pet_next )
      {
         pet_next = pet->next_pet;

         unbind_follower( pet, ch );
         extract_char( pet, TRUE );
      }
   }

   /*
    * Synch clandata up only when clan member quits now. --Shaddai 
    */
   if( ch->pcdata->clan )
      save_clan( ch->pcdata->clan );

   saving_char = NULL;

   extract_char( ch, TRUE );
   for( x = 0; x < MAX_WEAR; x++ )
      for( y = 0; y < MAX_LAYERS; y++ )
         save_equipment[x][y] = NULL;
   return;
}

/* Function modified from original form by Samson 1-14-98 */
CMDF do_quit( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;
   short room_chance;
   int level = ch->level, rentcost = 0;

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPCs cannot use the quit command.\n\r", ch );
      return;
   }

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      rent_calculate( ch, obj, &rentcost );

   if( !str_cmp( argument, "auto" ) && sysdata.RENT && rentcost > 0 )
   {
      room_chance = number_range( 1, 3 );

      if( room_chance > 2 )
      {
         log_printf_plus( LOG_COMM, level, "%s has failed autorent, setting autorent flag.", ch->name );
         char_leaving( ch, 2, rentcost );
      }
      else
      {
         log_printf_plus( LOG_COMM, level, "%s has autorented safely.", ch->name );
         char_leaving( ch, 3, rentcost );
      }
      return;
   }

   if( ( !argument || argument[0] == '\0' || str_cmp( argument, "yes" ) ) && sysdata.RENT && rentcost > 0 )
   {
      interpret( ch, "help quit" );
      return;
   }

   if( !IS_IMMORTAL( ch ) )
   {
      if( IS_AREA_FLAG( ch->in_room->area, AFLAG_NOQUIT ) )
      {
         send_to_char( "You may not quit in this area, it isn't safe!\n\r", ch );
         return;
      }

      if( IS_ROOM_FLAG( ch->in_room, ROOM_NOQUIT ) )
      {
         send_to_char( "You may not quit here, it isn't safe!\n\r", ch );
         return;
      }
   }

   if( ch->position == POS_FIGHTING )
   {
      send_to_char( "&RNo way! You are fighting.\n\r", ch );
      return;
   }

   if( ch->position < POS_STUNNED )
   {
      send_to_char( "&[blood]You're not DEAD yet.\n\r", ch );
      return;
   }

   if( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !IS_IMMORTAL( ch ) )
   {
      send_to_char( "&RYour adrenaline is pumping too hard to quit now!\n\r", ch );
      return;
   }

   if( auction->item != NULL && ( ( ch == auction->buyer ) || ( ch == auction->seller ) ) )
   {
      send_to_char( "&[auction]Wait until you have bought/sold the item on auction.\n\r", ch );
      return;
   }

   if( ch->inflight )
   {
      send_to_char( "&YSkyships are not equipped with parachutes. Wait until you land.\n\r", ch );
      return;
   }

   send_to_char( "&WYou make a hasty break for the confines of reality...&d\n\r", ch );
   if( sysdata.RENT && rentcost > 0 )
      send_to_char( "As you leave, your equipment falls to the floor!&d\n\r", ch );
   act( AT_SAY, "A strange voice says, 'We await your return, $n...'", ch, NULL, NULL, TO_CHAR );
   act( AT_BYE, "$n has left the game.", ch, NULL, NULL, TO_ROOM );

   log_printf_plus( LOG_COMM, ch->level, "%s has quit.", ch->name );
   if( sysdata.RENT )
      char_leaving( ch, 4, rentcost );
   else
      char_leaving( ch, 0, rentcost );
   return;
}

/* Checks room to see if an Innkeeper mob is present
   Code courtesy of the Smaug mailing list - Installed by Samson */
CHAR_DATA *find_innkeeper( CHAR_DATA * ch )
{
   CHAR_DATA *innkeeper;

   for( innkeeper = ch->in_room->first_person; innkeeper; innkeeper = innkeeper->next_in_room )
      if( IS_ACT_FLAG( innkeeper, ACT_INNKEEPER ) )
         break;

   return innkeeper;
}

/* New code to calculate player rent cost based on time stamp - Installed by Samson 1-14-98 */
/* Code courtesy of Garil - Desolation of the Dragon 2 Mud */
void scan_rent( CHAR_DATA * ch )
{
   OBJ_DATA *tobj;
   char buf[256];
   int rentcost = 0;
   double cost = 0, clancut = 0;
   CLAN_DATA *clan = NULL;
   bool found = FALSE;
   struct stat fst;

   snprintf( buf, 256, "%s%c/%s", PLAYER_DIR, tolower( ch->name[0] ), capitalize( ch->name ) );
   if( stat( buf, &fst ) != -1 )
   {
      for( tobj = ch->first_carrying; tobj; tobj = tobj->next_content )
         rent_calculate( ch, tobj, &rentcost );
      cost = ( int )( ( rentcost * ( double )( time( 0 ) - fst.st_mtime ) ) / 86400 );

      if( IS_IMMORTAL( ch ) || !sysdata.RENT )
         cost = 0;

      if( IS_ROOM_FLAG( ch->in_room, ROOM_GUILDINN ) )
      {
         for( clan = first_clan; clan; clan = clan->next )
         {
            if( clan->inn == ch->in_room->vnum )
            {
               found = TRUE;
               break;
            }
         }
      }

      if( found && clan == ch->pcdata->clan )
      {
         cost /= 2;
         clancut = 0;
      }
      else if( found )
      {
         cost *= 0.8;
         clancut = cost * 0.2;
      }

      /*
       * Drop your link - pay 5X the usual cost. 
       */
      if( ch->pcdata->autorent == TRUE )
         cost *= 5;

      if( ( ch->gold < cost || ch->pcdata->norares == TRUE ) && sysdata.RENT )
      {
         for( tobj = ch->first_carrying; tobj; tobj = tobj->next_content )
            rent_check( ch, tobj );

         if( found && clan->bank )
         {
            clan->balance += ch->gold;
            save_clan( clan );
         }

         ch->gold = 0;
         ch_printf( ch, "&BYou ran up charges of %d in rent, but could not afford it!\n\r",
                    ( int )( cost + ch->pcdata->rent ) );
         send_to_char( "Your rare items have been sold to cover the debt.\n\r", ch );
         if( ch->pcdata->autorent == TRUE )
            send_to_char( "Note: You autorented to leave the game - your cost was multiplied by 5.\n\r", ch );
         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL,
                          "%s ran up %d in rent costs, but ran out of money. Rare items recirculated.", ch->name,
                          cost + ch->pcdata->rent );
         ch->pcdata->rent = 0;
         ch->pcdata->norares = FALSE;
         ch->pcdata->autorent = FALSE;
      }
      else
      {
         ch->gold -= ( int )cost;

         if( !IS_IMMORTAL( ch ) )
         {
            if( sysdata.RENT )
            {
               ch_printf( ch, "&BYou ran up charges of %d in rent.\n\r", ( int )( cost + ch->pcdata->rent ) );
               if( ch->pcdata->autorent == TRUE )
                  send_to_char( "Note: You autorented to leave the game - your cost was multiplied by 5.\n\r", ch );
               log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s ran up %d in rent costs.", ch->name, cost + ch->pcdata->rent );
               ch->pcdata->rent = 0;
               ch->pcdata->norares = FALSE;
               ch->pcdata->autorent = FALSE;

               if( found && clan->bank && clancut > 0 )
               {
                  clan->balance += ( int )clancut;
                  save_clan( clan );
                  /*
                   * Modified by Tarl 12 Sept 03 to reflect the total cut the guild makes (i.e. clancut is simply the
                   * * amount the guild makes between the last rent update and when the player logs in. 
                   */
                  ch_printf( ch, "%d went to %s for maintenance costs.\n\r",
                             ( int )( ( cost + ch->pcdata->rent ) * 0.2 ), clan->name );
                  log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s made %d as their cut.", clan->name,
                                   ( int )( ( cost + ch->pcdata->rent ) * 0.2 ) );
               }
            }

            switch ( ch->pcdata->camp )
            {
               case 0:
                  log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s checks out of the inn.", ch->name );
                  break;
               case 2:
                  log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s reconnecting after failed autorent.", ch->name );
                  break;
               case 3:
                  log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s reconnecting after safe autorent.", ch->name );
                  break;
               case 4:
                  log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s reconnecting after quitting.", ch->name );
                  break;
            }
         }
         else
         {
            log_printf_plus( LOG_COMM, ch->level, "%s returns from beyond the void.", ch->name );
            show_stateflags( ch );
         }
      }
   }
   return;
}

/* Offer function added by Samson 1-14-98 Tells player how much rent they will be charged */
CMDF do_offer( CHAR_DATA * ch, char *argument )
{
   CLAN_DATA *clan = NULL;
   bool found = FALSE;
   OBJ_DATA *obj;
   CHAR_DATA *innkeeper, *victim;
   int rentcost;

   if( !( innkeeper = find_innkeeper( ch ) ) )
   {
      send_to_char( "You can only offer at an inn.\n\r", ch );
      return;
   }

   victim = innkeeper;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Get Real! Mobs can't offer!\n\r", ch );
      return;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_GUILDINN ) )
   {
      for( clan = first_clan; clan; clan = clan->next )
      {
         if( clan->inn == ch->in_room->vnum )
         {
            found = TRUE;
            break;
         }
      }
   }

   rentcost = 0;

   if( sysdata.RENT )
   {
      act( AT_SOCIAL, "$n takes a look at your items.....&G", victim, NULL, ch, TO_VICT );
      for( obj = ch->first_carrying; obj; obj = obj->next_content )
         rent_display( ch, obj, &rentcost );

      act_printf( AT_SAY, victim, NULL, ch, TO_VICT, "$n says 'Your rent will cost you %d coins per day.'", rentcost );

      if( found && clan == ch->pcdata->clan )
      {
         rentcost /= 2;
         act_printf( AT_SAY, victim, NULL, ch, TO_VICT,
                     "$n says '%s membership discount applies! Your new total is %d per day.'",
                     clan->clan_type == CLAN_ORDER ? "guild" : "clan", rentcost );
      }
      else if( found )
      {
         act_printf( AT_SAY, victim, NULL, ch, TO_VICT,
                     "$n says 'For utilizing %s's inn, a discount applies! Your new total is %d per day.'", clan->name,
                     ( int )( rentcost * 0.8 ) );
      }

      if( IS_IMMORTAL( ch ) )
         act_printf( AT_SAY, victim, NULL, ch, TO_VICT, "$n says 'But for you, oh mighty %s, I shall waive my fees!",
                     ch->name );
   }
   else
      send_to_char( "Rent is disabled. No cost applies.\n\r", ch );
   return;
}

/* Modified do_quit function, calculates rent cost & saves player data. Added by Samson 1-14-98 */
CMDF do_rent( CHAR_DATA * ch, char *argument )
{
   CLAN_DATA *clan = NULL;
   bool found = FALSE;
   OBJ_DATA *obj;
   CHAR_DATA *innkeeper;
   CHAR_DATA *victim;
   int level = get_trust( ch );
   int rentcost;

   if( !( innkeeper = find_innkeeper( ch ) ) )
   {
      send_to_char( "You can only rent at an inn.\n\r", ch );
      return;
   }

   victim = innkeeper;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Get Real! Mobs can't rent!\n\r", ch );
      return;
   }

   if( auction->item != NULL && ( ( ch == auction->buyer ) || ( ch == auction->seller ) ) )
   {
      send_to_char( "Wait until you have bought/sold the item on auction.\n\r", ch );
      return;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_GUILDINN ) )
   {
      for( clan = first_clan; clan; clan = clan->next )
      {
         if( clan->inn == ch->in_room->vnum )
         {
            found = TRUE;
            break;
         }
      }
   }

   /*
    * New code added by Samson on 1-14-98 Rent cost calculation 
    */
   rentcost = 0;
   if( sysdata.RENT )
   {
      act( AT_SOCIAL, "$n takes a look at your items.....&G", victim, NULL, ch, TO_VICT );
      for( obj = ch->first_carrying; obj; obj = obj->next_content )
         rent_leaving( ch, obj, &rentcost );
      act_printf( AT_SAY, victim, NULL, ch, TO_VICT, "$n says 'Your rent will cost you %d coins per day.'", rentcost );

      if( found && clan == ch->pcdata->clan )
      {
         rentcost /= 2;
         act_printf( AT_SAY, victim, NULL, ch, TO_VICT,
                     "$n says '%s membership discount applies! Your new total is %d per day.'",
                     clan->clan_type == CLAN_ORDER ? "guild" : "clan", rentcost );
      }
      else if( found )
      {
         rentcost = ( int )( rentcost * 0.8 );
         act_printf( AT_SAY, victim, NULL, ch, TO_VICT,
                     "$n says 'For utilizing %s's inn, a discount applies! Your new total is %d per day.'",
                     clan->name, rentcost );
      }

      if( IS_IMMORTAL( ch ) )
      {
         act_printf( AT_SAY, victim, NULL, ch, TO_VICT,
                     "$n says 'But for you, oh mighty %s, I shall waive my fees!", ch->name );
         rentcost = 0;
      }

      if( ch->gold < rentcost )
      {
         act( AT_SAY, "$n says 'You cannot afford this much!!'", victim, NULL, ch, TO_VICT );
         return;
      }
   }
   act( AT_WHITE, "$n takes your equipment into storage, and shows you to your room.&d", victim, NULL, ch, TO_VICT );
   act( AT_SAY, "A strange voice says, 'We await your return, $n...'&d", ch, NULL, NULL, TO_CHAR );
   act( AT_BYE, "$n shows $N to $S room, and stores $S equipment.", victim, NULL, ch, TO_NOTVICT );

   log_printf_plus( LOG_COMM, level, "%s has rented, at a cost of %d per day.", ch->name, rentcost );
   log_printf_plus( LOG_COMM, level, "%s rented in: %s, %s", ch->name, ch->in_room->name, ch->in_room->area->name );

   char_leaving( ch, 0, rentcost );

   return;
}

/*
 * Make a campfire.
 */
void make_campfire( ROOM_INDEX_DATA * in_room, CHAR_DATA * ch, short timer )
{
   OBJ_DATA *fire;

   if( !( fire = create_object( get_obj_index( OBJ_VNUM_CAMPFIRE ), 0 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   fire->timer = number_fuzzy( timer );
   obj_to_room( fire, in_room, ch );
   return;
}

CMDF do_camp( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj, *campfire;
   bool fire, fbed, fgear, flint;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot camp!\n\r", ch );
      return;
   }

   if( !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      if( IS_ROOM_FLAG( ch->in_room, ROOM_NOCAMP ) )
      {
         send_to_char( "You may not setup camp in this spot, find another.\n\r", ch );
         return;
      }

      if( IS_AREA_FLAG( ch->in_room->area, AFLAG_NOCAMP ) )
      {
         send_to_char( "It is not safe to camp in this area!\n\r", ch );
         return;
      }

      if( IS_ROOM_FLAG( ch->in_room, ROOM_INDOORS )
          || IS_ROOM_FLAG( ch->in_room, ROOM_CAVE ) || IS_ROOM_FLAG( ch->in_room, ROOM_CAVERN ) )
      {
         send_to_char( "You must be outdoors to make camp.\n\r", ch );
         return;
      }

      switch ( ch->in_room->sector_type )
      {
         case SECT_UNDERWATER:
         case SECT_OCEANFLOOR:
            send_to_char( "You cannot camp underwater, you'd drown!\n\r", ch );
            return;
         case SECT_RIVER:
            send_to_char( "The river would sweep any such camp away!\n\r", ch );
            return;
         case SECT_WATER_SWIM:
         case SECT_WATER_NOSWIM:
         case SECT_OCEAN:
            send_to_char( "You cannot camp on the open water like that!\n\r", ch );
            /*
             * At some future date, add code to check for a large boat, assuming
             * we ever get code to support boats of any real size 
             */
            return;
         case SECT_AIR:
            send_to_char( "Yeah, sure, set camp in thin air???\n\r", ch );
            return;
         case SECT_CITY:
         case SECT_ROAD:
         case SECT_BRIDGE:
            send_to_char( "This spot is too heavily travelled to setup camp.\n\r", ch );
            return;
         case SECT_INDOORS:
            send_to_char( "You must be outdoors to make camp.\n\r", ch );
            return;
         case SECT_ICE:
            send_to_char( "It isn't safe to setup camp on the ice.\n\r", ch );
            return;
         case SECT_LAVA:
            send_to_char( "What? You want to barbecue yourself?\n\r", ch );
            return;
         default:
            break;
      }
   }
   else
   {
      short sector = map_sector[ch->map][ch->x][ch->y];

      switch ( sector )
      {
         case SECT_RIVER:
            send_to_char( "The river would sweep any such camp away!\n\r", ch );
            return;
         case SECT_WATER_SWIM:
         case SECT_WATER_NOSWIM:
            send_to_char( "You cannot camp on the open water like that!\n\r", ch );
            /*
             * At some future date, add code to check for a large boat, assuming
             * we ever get code to support boats of any real size 
             */
            return;
         case SECT_CITY:
         case SECT_ROAD:
         case SECT_BRIDGE:
            send_to_char( "This spot is too heavily travelled to setup camp.\n\r", ch );
            return;
         case SECT_ICE:
            send_to_char( "It isn't safe to setup camp on the ice.\n\r", ch );
            return;
         default:
            break;
      }
   }

   if( auction->item != NULL && ( ( ch == auction->buyer ) || ( ch == auction->seller ) ) )
   {
      send_to_char( "Wait until you have bought/sold the item on auction.\n\r", ch );
      return;
   }

   fbed = FALSE;
   fgear = FALSE;
   flint = FALSE;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->item_type == ITEM_CAMPGEAR )
      {
         if( obj->value[0] == 1 )
            fbed = TRUE;
         if( obj->value[0] == 2 )
            fgear = TRUE;
         if( obj->value[0] == 3 )
            flint = TRUE;
      }

   if( !fbed || !fgear )
   {
      send_to_char( "You must have a bedroll and camping gear before making camp.\n\r", ch );
      return;
   }

   fire = FALSE;

   for( campfire = ch->in_room->first_content; campfire; campfire = campfire->next_content )
   {
      if( campfire->item_type == ITEM_FIRE )
      {
         if( IS_PLR_FLAG( ch, PLR_ONMAP ) )
         {
            if( ch->map == campfire->map && ch->x == campfire->x && ch->y == campfire->y )
            {
               fire = TRUE;
               break;
            }
         }

         if( !IS_PLR_FLAG( ch, PLR_ONMAP ) )
         {
            fire = TRUE;
            break;
         }
      }
   }

   if( !fire )
      make_campfire( ch->in_room, ch, 40 );

   send_to_char( "After tending to your fire and securing your belongings, you make camp for the night.\n\r", ch );
   act( AT_GREEN, "$n secures $s belongings and makes camp for the night.\n\r", ch, NULL, NULL, TO_ROOM );

   log_printf( "%s has made camp for the night in %s.", ch->name, ch->in_room->area->name );
   char_leaving( ch, 1, 0 );
   return;
}

/* Take one rare item away once found - Samson 9-19-98 */
int thief_raid( CHAR_DATA * ch, OBJ_DATA * obj, int robbed )
{
   OBJ_DATA *tobj;

   if( robbed == 0 )
   {
      if( obj->rent >= sysdata.minrent )
      {
         ch_printf( ch, "&YThe thieves stole %s!\n\r", obj->short_descr );
         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Thieves stole %s from %s!", obj->short_descr, ch->name );
         separate_obj( obj );
         extract_obj( obj );
         robbed = 1;
      }
      for( tobj = obj->first_content; tobj; tobj = tobj->next_content )
         thief_raid( ch, tobj, robbed );
   }
   return robbed;
}

/* Take all rare items away - Samson 9-19-98 */
int bandit_raid( CHAR_DATA * ch, OBJ_DATA * obj, int robbed )
{
   OBJ_DATA *tobj;

   if( obj->rent >= sysdata.minrent )
   {
      ch_printf( ch, "&YThe bandits stole %s!\n\r", obj->short_descr );
      log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Bandits stole %s from %s!", obj->short_descr, ch->name );
      extract_obj( obj );
      robbed = 1;
   }
   for( tobj = obj->first_content; tobj; tobj = tobj->next_content )
      bandit_raid( ch, tobj, robbed );

   return robbed;
}

/* Run through and check player camp to see if they got robbed among other things.
   Samson 9-19-98 */
void break_camp( CHAR_DATA * ch )
{
   OBJ_DATA *obj;
   int robchance, robbed = 0;

   robchance = number_range( 1, 100 );

   if( robchance > 85 )
   {
      if( robchance < 98 )
      {
         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Thieves raided %s's camp!", ch->name );
         send_to_char( "&RYour camp was visited by thieves while you were away!\n\r", ch );
         send_to_char( "&RYour belongings have been rummaged through....\n\r", ch );

         for( obj = ch->first_carrying; obj; obj = obj->next_content )
            robbed = thief_raid( ch, obj, robbed );
      }
      else
      {
         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Bandits raided %s's camp!", ch->name );
         send_to_char( "&RYour camp was visited by bandits while you were away!\n\r", ch );
         send_to_char( "&RYour belongings have been rummaged through....\n\r", ch );

         for( obj = ch->first_carrying; obj; obj = obj->next_content )
            robbed = bandit_raid( ch, obj, robbed );
      }
   }
   log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s breaks camp and enters the game.", ch->name );
   ch->pcdata->camp = 0;
   return;
}

/*
 * Bring up the pfile for rent adjustments
 */
void rent_adjust_pfile( char *argument )
{
   CLAN_DATA *clan = NULL;
   bool found = FALSE, loaded;
   CHAR_DATA *temp, *ch;
   ROOM_INDEX_DATA *temproom, *original;
   OBJ_DATA *tobj;
   char fname[256], name[MIL];
   struct stat fst;
   DESCRIPTOR_DATA *d;
   int rentcost = 0, x, y;
   double cost = 0, clancut = 0;

   one_argument( argument, name );

   for( temp = first_char; temp; temp = temp->next )
   {
      if( IS_NPC( temp ) )
         continue;
      if( !str_cmp( name, temp->name ) )
         break;
   }
   if( temp != NULL )
   {
      log_printf( "Skipping rent adjustments for %s, player is online.", temp->name );
      if( IS_IMMORTAL( temp ) )  /* Get the rent items off the immortals */
      {
         log_printf( "Immortal: Removing rent items from %s.", temp->name );
         for( tobj = temp->first_carrying; tobj; tobj = tobj->next_content )
            rent_check( temp, tobj );
      }
      return;
   }

   temproom = get_room_index( ROOM_VNUM_RENTUPDATE );

   if( temproom == NULL )
   {
      bug( "%s", "Error in rent adjustment, temporary loading room is missing!" );
      return;
   }

   name[0] = UPPER( name[0] );
   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );

   if( stat( fname, &fst ) != -1 )
   {
      CREATE( d, DESCRIPTOR_DATA, 1 );
      d->next = NULL;
      d->prev = NULL;
      d->connected = CON_PLOADED;
      d->outsize = 2000;
      CREATE( d->outbuf, char, d->outsize );

      loaded = load_char_obj( d, name, FALSE, FALSE );
      LINK( d->character, first_char, last_char, next, prev );
      original = d->character->in_room;
      if( !char_to_room( d->character, temproom ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      ch = d->character;   /* Hopefully this will work, if not, we're SOL */
      d->character->desc = NULL;
      d->character = NULL;
      DISPOSE( d->outbuf );
      DISPOSE( d );

      log_printf( "Updating rent for %s", ch->name );
      if( IS_ROOM_FLAG( original, ROOM_GUILDINN ) )
      {
         for( clan = first_clan; clan; clan = clan->next )
         {
            if( clan->inn == original->vnum )
            {
               found = TRUE;
               break;
            }
         }
      }

      for( tobj = ch->first_carrying; tobj; tobj = tobj->next_content )
         rent_calculate( ch, tobj, &rentcost );

      cost = ( int )( ( rentcost * ( double )( time( 0 ) - fst.st_mtime ) ) / 86400 );

      if( IS_IMMORTAL( ch ) || !sysdata.RENT || ch->pcdata->camp == 1 )
         cost = 0;

      if( found && clan == ch->pcdata->clan )
         cost /= 2;
      else if( found )
      {
         cost *= 0.8;
         clancut = cost * 0.2;
      }

      /*
       * Drop link, pay 5X the normal cost 
       */
      if( ch->pcdata->autorent == TRUE )
      {
         cost *= 5;
         if( found )
            clancut *= 5;
      }

      if( ch->gold < cost )
      {
         for( tobj = ch->first_carrying; tobj; tobj = tobj->next_content )
            rent_check( ch, tobj );

         if( found && clan->bank )
         {
            clan->balance += ch->gold;
            save_clan( clan );
         }

         ch->gold = 0;
         ch->pcdata->rent += ( int )cost;
         ch->pcdata->norares = TRUE;
         log_printf( "%s ran up %d in rent costs, but ran out of money. Rare items recirculated.", ch->name, ( int )cost );
         log_printf( "%s accrued %d in charges before running out.", ch->name, ( int )( cost + ch->pcdata->rent ) );
         if( ch->pcdata->autorent == TRUE )
            log_printf( "%s autorented to leave the game - costs were quintupled.", ch->name );
      }
      else
      {
         if( !IS_IMMORTAL( ch ) && ch->pcdata->camp != 1 )
         {
            ch->gold -= ( int )cost;

            ch->pcdata->rent += ( int )cost;
            log_printf( "%s paid rent charges of %d for the day.", ch->name, ( int )cost );

            if( found && clan->bank && clancut > 0 )
            {
               clan->balance += ( int )clancut;
               save_clan( clan );
               ch_printf( ch, "%d went to %s for maintenance costs.\n\r", ( int )clancut, clan->name );
               log_printf( "%s made %d as their cut.", clan->name, ( int )clancut );
            }

            if( ch->pcdata->autorent == TRUE )
               log_printf( "%s autorented to leave the game - costs were quintupled.", ch->name );
         }
         else if( IS_IMMORTAL( ch ) )  /* Imms shouldn't be carrying rent items, period. */
         {
            log_printf( "Immortal: Removing rent items from %s.", ch->name );
            for( tobj = ch->first_carrying; tobj; tobj = tobj->next_content )
               rent_check( ch, tobj );
         }
      }

      char_from_room( ch );
      if( !char_to_room( ch, original ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      quitting_char = ch;
      save_char_obj( ch );


      if( sysdata.save_pets )
      {
         CHAR_DATA *pet;

         while( ( pet = ch->last_pet ) != NULL )
            extract_char( pet, TRUE );
      }

      /*
       * Synch clandata up only when clan member quits now. --Shaddai 
       */
      if( ch->pcdata->clan )
         save_clan( ch->pcdata->clan );

      saving_char = NULL;

      /*
       * After extract_char the ch is no longer valid!
       */
      extract_char( ch, TRUE );
      for( x = 0; x < MAX_WEAR; x++ )
         for( y = 0; y < MAX_LAYERS; y++ )
            save_equipment[x][y] = NULL;

      log_printf( "Rent totals for %s updated sucessfully.", name );
      return;
   }
   /*
    * else no player file 
    */
   return;
}

/* Rare item counting function taken from the Tartarus codebase, a
 * ROM 2.4b derivitive by Ceran
 */
/* Modified for Smaug compatibility by Samson */
int rent_scan_pfiles( char *dirname, char *filename, bool updating )
{
   FILE *fpChar;
   char fname[256];
   int adjust = 0;

   snprintf( fname, 256, "%s/%s", dirname, filename );

   if( !( fpChar = fopen( fname, "r" ) ) )
   {
      perror( fname );
      return 0;
   }

   for( ;; )
   {
      int vnum = 0, temp = 0, counter = 1;
      char letter;
      const char *word;
      char *tempstring;
      OBJ_INDEX_DATA *pObjIndex = NULL;

      letter = fread_letter( fpChar );

      if( ( letter != '#' ) && ( !feof( fpChar ) ) )
         continue;

      word = feof( fpChar ) ? "End" : fread_word( fpChar );

      if( !str_cmp( word, "End" ) )
         break;

      if( !str_cmp( word, "OBJECT" ) )
      {
         word = feof( fpChar ) ? "End" : fread_word( fpChar );

         if( !str_cmp( word, "End" ) )
            break;

         if( !str_cmp( word, "Nest" ) )
         {
            temp = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Count" ) )
         {
            counter = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Name" ) )
         {
            tempstring = fread_flagstring( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "ShortDescr" ) )
         {
            tempstring = fread_flagstring( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Description" ) )
         {
            tempstring = fread_flagstring( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "ActionDesc" ) )
         {
            tempstring = fread_flagstring( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Ovnum" ) )
         {
            vnum = fread_number( fpChar );
            if( ( pObjIndex = get_obj_index( vnum ) ) == NULL )
            {
               bug( "rent_scan_pfiles: %s has bad obj vnum.", filename );
               adjust = 1; /* So it can clean out the bad object - Samson 4-16-00 */
            }
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Rent" ) && pObjIndex )
         {
            int rent = fread_number( fpChar );
            if( rent >= sysdata.minrent )
            {
               if( !updating )
               {
                  pObjIndex->count += counter;
                  log_printf( "%s: Counted %d of Vnum %d", filename, counter, vnum );
               }
               else
                  adjust = 1;
            }
         }
      }
   }
   FCLOSE( fpChar );
   return ( adjust );
}

void corpse_scan( char *dirname, char *filename )
{
   FILE *fpChar;
   char fname[256];

   snprintf( fname, 256, "%s/%s", dirname, filename );

   if( !( fpChar = fopen( fname, "r" ) ) )
   {
      perror( fname );
      return;
   }

   for( ;; )
   {
      int vnum, counter = 1, nest = 0;
      char letter;
      const char *word;
      OBJ_INDEX_DATA *pObjIndex;

      letter = fread_letter( fpChar );

      if( ( letter != '#' ) && ( !feof( fpChar ) ) )
         continue;

      word = feof( fpChar ) ? "End" : fread_word( fpChar );

      if( !str_cmp( word, "End" ) )
         break;

      if( !str_cmp( word, "OBJECT" ) )
      {
         word = feof( fpChar ) ? "End" : fread_word( fpChar );

         if( !str_cmp( word, "End" ) )
            break;

         if( !str_cmp( word, "Nest" ) )
         {
            nest = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Count" ) )
         {
            counter = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Ovnum" ) )
         {
            vnum = fread_number( fpChar );
            if( ( get_obj_index( vnum ) ) == NULL )
               bug( "corpse_scan: %s's corpse has bad obj vnum.", filename );
            else
            {
               int rent = 0;
               pObjIndex = get_obj_index( vnum );
               if( pObjIndex->rent == -2 )
                  rent = set_obj_rent( pObjIndex );
               if( rent >= sysdata.minrent )
               {
                  pObjIndex->count += counter;
                  log_printf( "%s: Counted %d of Vnum %d", filename, counter, vnum );
               }
            }
         }
      }
   }
   FCLOSE( fpChar );
   return;
}

void mobfile_scan( void )
{
   FILE *fpChar;
   char fname[256];

   snprintf( fname, 256, "%s%s", SYSTEM_DIR, MOB_FILE );

   if( !( fpChar = fopen( fname, "r" ) ) )
   {
      perror( fname );
      return;
   }

   for( ;; )
   {
      int vnum, counter = 1, nest = 0;
      char letter;
      const char *word;
      OBJ_INDEX_DATA *pObjIndex;

      letter = fread_letter( fpChar );

      if( ( letter != '#' ) && ( !feof( fpChar ) ) )
         continue;

      word = feof( fpChar ) ? "End" : fread_word( fpChar );

      if( !str_cmp( word, "End" ) )
         break;

      if( !str_cmp( word, "OBJECT" ) )
      {
         word = feof( fpChar ) ? "End" : fread_word( fpChar );

         if( !str_cmp( word, "End" ) )
            break;

         if( !str_cmp( word, "Nest" ) )
         {
            nest = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Count" ) )
         {
            counter = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Ovnum" ) )
         {
            vnum = fread_number( fpChar );
            if( ( get_obj_index( vnum ) ) == NULL )
               bug( "mobfile_scan: bad obj vnum %d.", vnum );
            else
            {
               int rent = 0;
               pObjIndex = get_obj_index( vnum );
               if( pObjIndex->rent == -2 )
                  rent = set_obj_rent( pObjIndex );
               if( rent >= sysdata.minrent )
               {
                  pObjIndex->count += counter;
                  log_printf( "%s: Counted %d of Vnum %d", fname, counter, vnum );
               }
            }
         }
      }
   }
   FCLOSE( fpChar );
   return;
}

void objfile_scan( char *dirname, char *filename )
{
   FILE *fpChar;
   char fname[256];

   snprintf( fname, 256, "%s%s", dirname, filename );

   if( !( fpChar = fopen( fname, "r" ) ) )
   {
      perror( fname );
      return;
   }

   for( ;; )
   {
      int vnum, counter = 1, nest = 0;
      char letter;
      const char *word;
      OBJ_INDEX_DATA *pObjIndex;

      letter = fread_letter( fpChar );

      if( ( letter != '#' ) && ( !feof( fpChar ) ) )
         continue;

      word = feof( fpChar ) ? "End" : fread_word( fpChar );

      if( !str_cmp( word, "End" ) )
         break;

      if( !str_cmp( word, "OBJECT" ) )
      {
         word = feof( fpChar ) ? "End" : fread_word( fpChar );

         if( !str_cmp( word, "End" ) )
            break;

         if( !str_cmp( word, "Nest" ) )
         {
            nest = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Count" ) )
         {
            counter = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Ovnum" ) )
         {
            vnum = fread_number( fpChar );
            if( ( get_obj_index( vnum ) ) == NULL )
               bug( "objfile_scan: bad obj vnum %d.", vnum );
            else
            {
               int rent = 0;
               pObjIndex = get_obj_index( vnum );
               if( pObjIndex->rent == -2 )
                  rent = set_obj_rent( pObjIndex );
               if( rent >= sysdata.minrent )
               {
                  pObjIndex->count += counter;
                  log_printf( "%s: Counted %d of Vnum %d", fname, counter, vnum );
               }
            }
         }
      }
   }
   FCLOSE( fpChar );
   return;
}

void load_equipment_totals( bool fCopyOver )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];
   int adjust = 0;
   short alpha_loop;

   check_pfiles( 255 ); /* Clean up stragglers to get a better count - Samson 1-1-00 */

   log_string( "Updating rare item counts....." );

   log_string( "Checking player files...." );

   for( alpha_loop = 0; alpha_loop <= 25; alpha_loop++ )
   {
      snprintf( directory_name, 100, "%s%c", PLAYER_DIR, 'a' + alpha_loop );
      dp = opendir( directory_name );
      dentry = readdir( dp );
      while( dentry )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( !str_cmp( dentry->d_name, "CVS" ) )
         {
            dentry = readdir( dp );
            continue;
         }
         if( dentry->d_name[0] != '.' )
         {
            adjust = rent_scan_pfiles( directory_name, dentry->d_name, FALSE );
            adjust = 0;
         }
         dentry = readdir( dp );
      }
      closedir( dp );
   }

   log_string( "Checking corpses...." );

   snprintf( directory_name, 100, "%s", CORPSE_DIR );
   dp = opendir( directory_name );
   dentry = readdir( dp );
   while( dentry )
   {
      /*
       * Added by Tarl 3 Dec 02 because we are now using CVS 
       */
      if( !str_cmp( dentry->d_name, "CVS" ) )
      {
         dentry = readdir( dp );
         continue;
      }
      if( dentry->d_name[0] != '.' )
      {
         corpse_scan( directory_name, dentry->d_name );
      }
      dentry = readdir( dp );
   }
   closedir( dp );

   if( fCopyOver )
   {
      log_string( "Scanning world-state mob file...." );
      mobfile_scan(  );

      log_string( "Scanning world-state obj files...." );
      snprintf( directory_name, 100, "%s", HOTBOOT_DIR );
      dp = opendir( directory_name );
      dentry = readdir( dp );
      while( dentry )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( !str_cmp( dentry->d_name, "CVS" ) )
         {
            dentry = readdir( dp );
            continue;
         }
         if( dentry->d_name[0] != '.' )
         {
            objfile_scan( directory_name, dentry->d_name );
         }
         dentry = readdir( dp );
      }
      closedir( dp );
   }
   return;
}

void rent_update( void )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];
   int adjust = 0;
   short alpha_loop;

   if( !sysdata.RENT )
      return;

   log_string( "Checking daily rent for players...." );

   for( alpha_loop = 0; alpha_loop <= 25; alpha_loop++ )
   {
      snprintf( directory_name, 100, "%s%c", PLAYER_DIR, 'a' + alpha_loop );
      dp = opendir( directory_name );
      dentry = readdir( dp );
      while( dentry )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( !str_cmp( dentry->d_name, "CVS" ) )
         {
            dentry = readdir( dp );
            continue;
         }
         if( dentry->d_name[0] != '.' )
         {
            adjust = rent_scan_pfiles( directory_name, dentry->d_name, TRUE );
            if( adjust == 1 )
            {
               rent_adjust_pfile( dentry->d_name );
               adjust = 0;
            }
         }
         dentry = readdir( dp );
      }
      closedir( dp );
   }

   log_string( "Daily rent updates completed." );
   return;
}
