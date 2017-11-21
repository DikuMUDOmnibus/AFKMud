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
 *                    Shop, Banking, and Repair Module                      *
 ****************************************************************************/

/* 

Clan shopkeeper section installed by Samson 7-16-00 
Code used is based on the Vendor snippet provided by:

Vendor Snippit V1.01
By: Jimmy M. (Meckteck and Legonas, depending on where i am)
E-Mail: legonas@netzero.net
ICQ #28394032
Released 02/28/00

Tested and Created on the Smaug1.4a code base.

----------------------------------------
Northwind: home of Legends             +
http://www.kilnar.com/~northwind/      +
telnet://northwind.kilnar.com:5555/    +
----------------------------------------

*/

#include <string.h>
#include <dirent.h>
#include "mud.h"
#include "clans.h"
#include "mxp.h"
#include "shops.h"

void fwrite_obj( CHAR_DATA * ch, OBJ_DATA * obj, CLAN_DATA * clan, FILE * fp, int iNest, short os_type, bool hotboot );
void fread_obj( CHAR_DATA * ch, FILE * fp, short os_type );
char *mxp_obj_str( CHAR_DATA * ch, OBJ_DATA * obj );
char *mxp_obj_str_close( CHAR_DATA * ch, OBJ_DATA * obj );
void save_clan( CLAN_DATA * clan );
void auction_sell( CHAR_DATA * ch, CHAR_DATA * auc, char *argument );
void auction_buy( CHAR_DATA * ch, CHAR_DATA * auc, char *argument );
void auction_value( CHAR_DATA * ch, CHAR_DATA * auc, char *argument );
void sound_to_char( const char *fname, int volume, CHAR_DATA * ch, bool toroom );
int can_wear_obj( CHAR_DATA * ch, OBJ_DATA * obj );

SHOP_DATA *first_shop;
SHOP_DATA *last_shop;
REPAIR_DATA *first_repair;
REPAIR_DATA *last_repair;

#define ROOM_VNUM_TEMPSHOP 1290

/*
 * Local functions
 */
int get_cost( CHAR_DATA * ch, CHAR_DATA * keeper, OBJ_DATA * obj, bool fBuy );
int get_repaircost( CHAR_DATA * keeper, OBJ_DATA * obj );
void fwrite_mobile( CHAR_DATA * mob, FILE * fp, bool shopmob );
char_data *fread_mobile( FILE * fp, bool shopmob );

void save_shop( CHAR_DATA * mob )
{
   FILE *fp = NULL;
   char filename[256];

   if( !IS_NPC( mob ) )
      return;

   snprintf( filename, 256, "%s%s", SHOP_DIR, capitalize( mob->short_descr ) );

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s", "save_shop: fopen" );
      perror( filename );
   }
   fwrite_mobile( mob, fp, true );
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
   return;
}

void load_shopkeepers( void )
{
   DIR *dp;
   FILE *fp;
   CLAN_DATA *clan;
   CHAR_DATA *mob = NULL;
   OBJ_DATA *obj, *obj_next;
   struct dirent *dentry;
   char directory_name[100];
   char filename[256];

   snprintf( directory_name, 100, "%s", SHOP_DIR );
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
         snprintf( filename, 256, "%s%s", directory_name, dentry->d_name );
         if( ( fp = fopen( filename, "r" ) ) != NULL )
         {
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
                  bug( "%s: # not found.", __FUNCTION__ );
                  break;
               }
               word = fread_word( fp );
               if( !strcmp( word, "SHOP" ) )
                  mob = fread_mobile( fp, true );
               else if( !strcmp( word, "OBJECT" ) )
               {
                  mob->tempnum = -9999;
                  fread_obj( mob, fp, OS_CARRY );
               }
               else if( !strcmp( word, "END" ) )
                  break;
            }
            FCLOSE( fp );
            if( mob )
            {
               for( clan = first_clan; clan; clan = clan->next )
               {
                  if( clan->shopkeeper == mob->pIndexData->vnum && clan->bank )
                  {
                     clan->balance += mob->gold;
                     mob->gold = 0;
                     save_shop( mob );
                  }
               }
               for( obj = mob->first_carrying; obj; obj = obj_next )
               {
                  obj_next = obj->next_content;

                  if( obj->rent >= sysdata.minrent )
                     extract_obj( obj );
               }
            }
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );
   return;
}

/*
 * Shopping commands.
 */
/* Modified to make keepers say when they open/close - Samson 4-23-98 */
CHAR_DATA *find_keeper( CHAR_DATA * ch )
{
   CHAR_DATA *keeper, *whof;
   SHOP_DATA *pShop;
   int speakswell;

   pShop = NULL;
   for( keeper = ch->in_room->first_person; keeper; keeper = keeper->next_in_room )
      if( IS_NPC( keeper ) && ( pShop = keeper->pIndexData->pShop ) != NULL )
         break;

   if( !pShop )
   {
      send_to_char( "You can't do that here.\n\r", ch );
      return NULL;
   }

   /*
    * Disallow sales during battle
    */
   if( ( whof = who_fighting( keeper ) ) != NULL )
   {
      if( whof == ch )
         send_to_char( "I don't think that's a good idea...\n\r", ch );
      else
         interpret( keeper, "say I'm too busy for that!" );
      return NULL;
   }

   if( who_fighting( ch ) )
   {
      ch_printf( ch, "%s doesn't seem to want to get involved.\n\r", PERS( keeper, ch, FALSE ) );
      return NULL;
   }

   /*
    * Check to see if show is open.
    * Supports closing times after midnight
    */
   if( pShop->open_hour > pShop->close_hour )
   {
      if( time_info.hour < pShop->open_hour && time_info.hour > pShop->close_hour )
      {
         if( pShop->open_hour < sysdata.hournoon )
            cmdf( keeper, "say Sorry, come back later. I open at %dam.",
                  ( pShop->open_hour == 0 ) ? ( pShop->open_hour + sysdata.hournoon ) : ( pShop->open_hour ) );
         else
            cmdf( keeper, "say Sorry, come back later. I open at %dpm.",
                  ( pShop->open_hour ==
                    sysdata.hournoon ) ? ( pShop->open_hour ) : ( pShop->open_hour - sysdata.hournoon ) );
         return NULL;
      }
   }
   else  /* Severely hacked up to work with Alsherok's 28hr clock - Samson 5-13-99 */
   {
      if( time_info.hour < pShop->open_hour )
      {
         if( pShop->open_hour < sysdata.hournoon )
            cmdf( keeper, "say Sorry, come back later. I open at %dam.",
                  ( pShop->open_hour == 0 ) ? ( pShop->open_hour + sysdata.hournoon ) : ( pShop->open_hour ) );
         else
            cmdf( keeper, "say Sorry, come back later. I open at %dpm.",
                  ( pShop->open_hour ==
                    sysdata.hournoon ) ? ( pShop->open_hour ) : ( pShop->open_hour - sysdata.hournoon ) );
         return NULL;
      }
      if( time_info.hour > pShop->close_hour )
      {
         if( pShop->close_hour < sysdata.hournoon )
            cmdf( keeper, "say Sorry, come back tomorrow. I close at %dam.",
                  ( pShop->close_hour == 0 ) ? ( pShop->close_hour + sysdata.hournoon ) : ( pShop->close_hour ) );
         else
            cmdf( keeper, "say Sorry, come back tomorrow. I close at %dpm.",
                  ( pShop->close_hour ==
                    sysdata.hournoon ) ? ( pShop->close_hour ) : ( pShop->close_hour - sysdata.hournoon ) );
         return NULL;
      }
   }

   if( keeper->position == POS_SLEEPING )
   {
      send_to_char( "While they're asleep?\n\r", ch );
      return NULL;
   }

   if( keeper->position < POS_SLEEPING )
   {
      send_to_char( "I don't think they can hear you...\n\r", ch );
      return NULL;
   }

   /*
    * Invisible or hidden people.
    */
   if( !can_see( keeper, ch, FALSE ) )
   {
      interpret( keeper, "say I don't trade with folks I can't see." );
      return NULL;
   }

   speakswell = UMIN( knows_language( keeper, ch->speaking, ch ), knows_language( ch, ch->speaking, keeper ) );

   if( ( number_percent(  ) % 65 ) > speakswell )
   {
      if( speakswell > 60 )
         cmdf( keeper, "say %s, Could you repeat that? I didn't quite catch it.", ch->name );
      else if( speakswell > 50 )
         cmdf( keeper, "say %s, Could you say that a little more clearly please?", ch->name );
      else if( speakswell > 40 )
         cmdf( keeper, "say %s, Sorry... What was that you wanted?", ch->name );
      else
         cmdf( keeper, "say %s, I can't understand you.", ch->name );
      return NULL;
   }

   return keeper;
}

CMDF do_setprice( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *keeper;
   CLAN_DATA *clan;
   char arg1[MIL];
   OBJ_DATA *obj;
   bool found = FALSE;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use this command.\n\r", ch );
      return;
   }

   if( !ch->pcdata->clan )
   {
      send_to_char( "But you aren't even in a clan or guild!\n\r", ch );
      return;
   }

   if( !( keeper = find_keeper( ch ) ) )
   {
      send_to_char( "What shopkeeper?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );

   if( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: setprice <item> <price>\n\r", ch );
      send_to_char( "Item should be something the shopkeeper already has.\n\r", ch );
      send_to_char( "Give the shopkeeper an item to list it for sale.\n\r", ch );
      send_to_char( "To remove an item from his inventory, set the price to -1.\n\r", ch );
      return;
   }

   if( ch->fighting )
   {
      send_to_char( "Not during combat!\n\r", ch );
      return;
   }

   if( !IS_ACT_FLAG( keeper, ACT_GUILDVENDOR ) )
   {
      send_to_char( "This shopkeeper doesn't work for a clan or guild!\n\r", ch );
      return;
   }

   for( clan = first_clan; clan; clan = clan->next )
   {
      if( clan->shopkeeper == keeper->pIndexData->vnum )
      {
         found = TRUE;
         break;
      }
   }

   if( found && clan != ch->pcdata->clan )
   {
      ch_printf( ch, "Beat it! I don't work for your %s!\n\r", clan->clan_type == CLAN_ORDER ? "guild" : "clan" );
      return;
   }

   if( str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number1 ) && str_cmp( ch->name, clan->number2 ) )
   {
      ch_printf( ch, "Only the %s admins can set prices on inventory.\n\r",
                 clan->clan_type == CLAN_ORDER ? "guild" : "clan" );
      return;
   }

   if( ( obj = get_obj_carry( keeper, arg1 ) ) != NULL )
   {
      int price;

      price = atoi( argument );

      if( price < -1 || price > 2000000000 )
      {
         send_to_char( "Valid price range is between -1 and 2 billion.\n\r", ch );
         return;
      }

      if( price == -1 )
      {
         obj_from_char( obj );
         obj_to_char( obj, ch );
         obj->cost = obj->pIndexData->cost;
         ch_printf( ch, "%s has been removed from sale.\n\r", obj->short_descr );
         save_char_obj( ch );
      }
      else
      {
         obj->cost = price;
         ch_printf( ch, "The price for %s has been changed to %d.\n\r", obj->short_descr, price );
      }
      save_shop( keeper );
      return;
   }
   send_to_char( "He doesnt have that item!\n\r", ch );
   return;
}

/*
 * repair commands.
 */
/* Modified to make keeper show open/close hour - Samson 4-23-98 */
CHAR_DATA *find_fixer( CHAR_DATA * ch )
{
   CHAR_DATA *keeper, *whof;
   REPAIR_DATA *rShop;
   int speakswell;

   rShop = NULL;
   for( keeper = ch->in_room->first_person; keeper; keeper = keeper->next_in_room )
      if( IS_NPC( keeper ) && ( rShop = keeper->pIndexData->rShop ) != NULL )
         break;

   if( !rShop )
   {
      send_to_char( "You can't do that here.\n\r", ch );
      return NULL;
   }

   /*
    * Disallow sales during battle
    */
   if( ( whof = who_fighting( keeper ) ) != NULL )
   {
      if( whof == ch )
         send_to_char( "I don't think that's a good idea...\n\r", ch );
      else
         interpret( keeper, "say I'm too busy for that!" );
      return NULL;
   }

   /*
    * According to rlog, this is the second time I've done this
    * * so mobiles can repair in combat.  -- Blod, 1/98
    */
   if( !IS_NPC( ch ) && who_fighting( ch ) )
   {
      ch_printf( ch, "%s doesn't seem to want to get involved.\n\r", PERS( keeper, ch, FALSE ) );
      return NULL;
   }

   /*
    * Check to see if show is open.
    * Supports closing times after midnight
    */
   if( rShop->open_hour > rShop->close_hour )
   {
      if( time_info.hour < rShop->open_hour && time_info.hour > rShop->close_hour )
      {
         if( rShop->open_hour < sysdata.hournoon )
            cmdf( keeper, "say Sorry, come back later. I open at %dam.",
                  ( rShop->open_hour == 0 ) ? ( rShop->open_hour + sysdata.hournoon ) : ( rShop->open_hour ) );
         else
            cmdf( keeper, "say Sorry, come back later. I open at %dpm.",
                  ( rShop->open_hour ==
                    sysdata.hournoon ) ? ( rShop->open_hour ) : ( rShop->open_hour - sysdata.hournoon ) );
         return NULL;
      }
   }
   else  /* Severely hacked up to work with Alsherok's 28hr clock - Samson 5-13-99 */
   {
      if( time_info.hour < rShop->open_hour )
      {
         if( rShop->open_hour < sysdata.hournoon )
            cmdf( keeper, "say Sorry, come back later. I open at %dam.",
                  ( rShop->open_hour == 0 ) ? ( rShop->open_hour + sysdata.hournoon ) : ( rShop->open_hour ) );
         else
            cmdf( keeper, "say Sorry, come back later. I open at %dpm.",
                  ( rShop->open_hour ==
                    sysdata.hournoon ) ? ( rShop->open_hour ) : ( rShop->open_hour - sysdata.hournoon ) );
         return NULL;
      }
      if( time_info.hour > rShop->close_hour )
      {
         if( rShop->close_hour < sysdata.hournoon )
            cmdf( keeper, "say Sorry, come back tomorrow. I close at %dam.",
                  ( rShop->close_hour == 0 ) ? ( rShop->close_hour + sysdata.hournoon ) : ( rShop->close_hour ) );
         else
            cmdf( keeper, "say Sorry, come back tomorrow. I close at %dpm.",
                  ( rShop->close_hour ==
                    sysdata.hournoon ) ? ( rShop->close_hour ) : ( rShop->close_hour - sysdata.hournoon ) );
         return NULL;
      }
   }

   if( keeper->position == POS_SLEEPING )
   {
      send_to_char( "While they're asleep?\n\r", ch );
      return NULL;
   }

   if( keeper->position < POS_SLEEPING )
   {
      send_to_char( "I don't think they can hear you...\n\r", ch );
      return NULL;
   }

   /*
    * Invisible or hidden people.
    */
   if( !can_see( keeper, ch, FALSE ) )
   {
      interpret( keeper, "say I don't trade with folks I can't see." );
      return NULL;
   }

   speakswell = UMIN( knows_language( keeper, ch->speaking, ch ), knows_language( ch, ch->speaking, keeper ) );

   if( ( number_percent(  ) % 65 ) > speakswell )
   {
      if( speakswell > 60 )
         cmdf( keeper, "say %s, Could you repeat that? I didn't quite catch it.", ch->name );
      else if( speakswell > 50 )
         cmdf( keeper, "say %s, Could you say that a little more clearly please?", ch->name );
      else if( speakswell > 40 )
         cmdf( keeper, "say %s, Sorry... What was that you wanted?", ch->name );
      else
         cmdf( keeper, "say %s I can't understand you.", ch->name );
      return NULL;
   }

   return keeper;
}

int get_cost( CHAR_DATA * ch, CHAR_DATA * keeper, OBJ_DATA * obj, bool fBuy )
{
   SHOP_DATA *pShop;
   int cost;
   int profitmod;

   if( !obj || ( pShop = keeper->pIndexData->pShop ) == NULL )
      return 0;

   if( fBuy )
   {
      profitmod = 13 - get_curr_cha( ch ) + ( ( URANGE( 5, ch->level, LEVEL_AVATAR ) - 20 ) / 2 );
      cost = ( int )( obj->cost * UMAX( ( pShop->profit_sell + 1 ), pShop->profit_buy + profitmod ) ) / 100;
      cost = ( int )( cost * ( 80 + UMIN( ch->level, LEVEL_AVATAR ) ) ) / 100;
   }
   else
   {
      int itype;

      profitmod = get_curr_cha( ch ) - 13;
      cost = 0;
      for( itype = 0; itype < MAX_TRADE; itype++ )
      {
         if( obj->item_type == pShop->buy_type[itype] )
         {
            cost = ( int )( obj->cost * UMIN( ( pShop->profit_buy - 1 ), pShop->profit_sell + profitmod ) ) / 100;
            break;
         }
      }
   }

   if( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
   {
      if( obj->value[2] < 0 && obj->value[1] < 0 )
         ;
      else
         cost = ( int )( cost * obj->value[2] / ( obj->value[1] > 0 ? obj->value[1] : 1 ) );
   }

   /*
    * alright - now that all THAT crap is over and done with.... lets simplify for
    * * guild vendors since the guild determines what they want to sell it for :)
    */
   if( IS_ACT_FLAG( keeper, ACT_GUILDVENDOR ) )
      cost = obj->cost;

   return cost;
}

int get_repaircost( CHAR_DATA * keeper, OBJ_DATA * obj )
{
   REPAIR_DATA *rShop;
   int cost;
   int itype;
   bool found;

   if( !obj || ( rShop = keeper->pIndexData->rShop ) == NULL )
      return 0;

   cost = 0;
   found = FALSE;
   for( itype = 0; itype < MAX_FIX; itype++ )
   {
      if( obj->item_type == rShop->fix_type[itype] )
      {
         cost = ( int )( obj->cost * rShop->profit_fix / 1000 );
         found = TRUE;
         break;
      }
   }

   if( !found )
      cost = -1;

   if( cost == 0 )
      cost = 1;

   if( found && cost > 0 )
   {
      switch ( obj->item_type )
      {
         case ITEM_ARMOR:
            if( obj->value[0] >= obj->value[1] )
               cost = -2;
            else
               cost *= ( obj->value[1] - obj->value[0] );
            break;
         case ITEM_WEAPON:
         case ITEM_MISSILE_WEAPON:
            if( obj->value[6] >= obj->value[0] )
               cost = -2;
            else
               cost *= ( obj->value[0] - obj->value[6] );
            break;
         case ITEM_PROJECTILE:
            if( obj->value[5] >= obj->value[0] )
               cost = -2;
            else
               cost *= ( obj->value[0] - obj->value[5] );
            break;
         case ITEM_WAND:
         case ITEM_STAFF:
            if( obj->value[2] >= obj->value[1] )
               cost = -2;
            else
               cost *= ( obj->value[1] - obj->value[2] );
      }
   }
   return cost;
}

CMDF do_buy( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *auc;
   char arg[MIL];
   double maxgold;
   bool aucmob = FALSE;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Sorry, Mobs cannot buy items!\n\r", ch );
      return;
   }

   for( auc = ch->in_room->first_person; auc; auc = auc->next_in_room )
   {
      if( IS_NPC( auc ) && ( IS_ACT_FLAG( auc, ACT_AUCTION ) || IS_ACT_FLAG( auc, ACT_GUILDAUC ) ) )
      {
         auction_buy( ch, auc, argument );
         aucmob = TRUE;
         break;
      }
   }

   if( aucmob )
      return;

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Buy what?\n\r", ch );
      return;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_PET_SHOP ) )
   {
      CHAR_DATA *pet;
      ROOM_INDEX_DATA *pRoomIndexNext, *in_room;

      if( IS_NPC( ch ) )
         return;

      pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
      if( !pRoomIndexNext )
      {
         bug( "Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum );
         send_to_char( "Sorry, you can't buy that here.\n\r", ch );
         return;
      }

      in_room = ch->in_room;
      ch->in_room = pRoomIndexNext;
      pet = get_char_room( ch, arg );
      ch->in_room = in_room;

      if( pet == NULL || !IS_ACT_FLAG( pet, ACT_PET ) )
      {
         send_to_char( "Sorry, you can't buy that here.\n\r", ch );
         return;
      }

      if( ch->gold < 10 * pet->level * pet->level )
      {
         send_to_char( "You can't afford it.\n\r", ch );
         return;
      }

      if( ch->level < pet->level )
      {
         send_to_char( "You're not ready for this pet.\n\r", ch );
         return;
      }

      if( !can_charm( ch ) )
      {
         send_to_char( "You already have too many followers to support!\n\r", ch );
         return;
      }

      maxgold = 10 * pet->level * pet->level;

      if( number_percent(  ) < ch->pcdata->learned[gsn_bargain] )
      {
         double x = ( ( double )ch->pcdata->learned[gsn_bargain] / 3 ) / 100;
         double pct = maxgold * x;
         maxgold = maxgold - pct;

         ch_printf( ch, "&[skill]Your bargaining skills have reduced the price by %d gold!\n\r", ( int )pct );
      }
      else
      {
         if( ch->pcdata->learned[gsn_bargain] > 0 )
         {
            send_to_char( "&[skill]Your bargaining skills failed to reduce the price.\n\r", ch );
            learn_from_failure( ch, gsn_bargain );
         }
      }

      ch->gold -= ( int )maxgold;
      pet = create_mobile( pet->pIndexData );

      argument = one_argument( argument, arg );
      if( arg && arg[0] != '\0' )
         stralloc_printf( &pet->name, "%s %s", pet->name, arg );

      stralloc_printf( &pet->chardesc, "%sA neck tag says 'I belong to %s'.\n\r", pet->chardesc, ch->name );

      if( !char_to_room( pet, ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      bind_follower( pet, ch, -1, -1 );
      send_to_char( "Enjoy your pet.\n\r", ch );
      act( AT_ACTION, "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM );
      return;
   }
   else
   {
      CHAR_DATA *keeper;
      CLAN_DATA *clan = NULL;
      OBJ_DATA *obj;
      double cost;
      int noi = 1;   /* Number of items */
      short mnoi = 50; /* Max number of items to be bought at once */
      bool found = FALSE;

      if( ( keeper = find_keeper( ch ) ) == NULL )
         return;

      maxgold = keeper->level * keeper->level * 50000;

      if( IS_ACT_FLAG( keeper, ACT_GUILDVENDOR ) )
      {
         for( clan = first_clan; clan; clan = clan->next )
         {
            if( keeper->pIndexData->vnum == clan->shopkeeper )
            {
               found = TRUE;
               break;
            }
         }
      }

      if( is_number( arg ) )
      {
         noi = atoi( arg );
         argument = one_argument( argument, arg );
         if( noi > mnoi )
         {
            act( AT_TELL, "$n tells you 'I don't sell that many items at once.'", keeper, NULL, ch, TO_VICT );
            ch->reply = keeper;
            return;
         }
      }

      obj = get_obj_carry( keeper, arg );
      cost = ( get_cost( ch, keeper, obj, TRUE ) * noi );

      if( cost <= 0 || !can_see_obj( ch, obj, FALSE ) )
      {
         act( AT_TELL, "$n tells you 'I don't sell that -- try 'list'.'", keeper, NULL, ch, TO_VICT );
         ch->reply = keeper;
         return;
      }

      /*
       * Moved down here to see if this solves the crash bug 
       */
      if( found )
         cost = ( obj->cost * noi );

      if( !IS_OBJ_FLAG( obj, ITEM_INVENTORY ) && ( noi > 1 ) )
      {
         interpret( keeper, "laugh" );
         act( AT_TELL, "$n tells you 'I don't have enough of those in stock to sell more than one at a time.'",
              keeper, NULL, ch, TO_VICT );
         ch->reply = keeper;
         return;
      }

      if( ch->gold < cost )
      {
         act( AT_TELL, "$n tells you 'You can't afford to buy $p.'", keeper, obj, ch, TO_VICT );
         ch->reply = keeper;
         return;
      }

      if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) && get_trust( ch ) < LEVEL_IMMORTAL )
      {
         act( AT_TELL, "$n tells you 'This is a only a prototype!  I can't sell you that...'", keeper, NULL, ch, TO_VICT );
         ch->reply = keeper;
         return;
      }

      if( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
      {
         send_to_char( "You can't carry that many items.\n\r", ch );
         return;
      }

      if( ch->carry_weight + ( get_obj_weight( obj ) * noi ) + ( noi > 1 ? 2 : 0 ) > can_carry_w( ch ) )
      {
         send_to_char( "You can't carry that much weight.\n\r", ch );
         return;
      }

      if( noi == 1 )
      {
         act( AT_ACTION, "$n buys $p.", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You buy $p.", ch, obj, NULL, TO_CHAR );
      }
      else
      {
         act_printf( AT_ACTION, ch, obj, NULL, TO_ROOM, "$n buys %d $p%s.", noi,
                     ( obj->short_descr[strlen( obj->short_descr ) - 1] == 's' ? "" : "s" ) );
         act_printf( AT_ACTION, ch, obj, NULL, TO_CHAR,
                     "You buy %d $p%s.", noi, ( obj->short_descr[strlen( obj->short_descr ) - 1] == 's' ? "" : "s" ) );
         act( AT_ACTION, "$N puts them into a bag and hands it to you.", ch, NULL, keeper, TO_CHAR );
      }

      if( number_percent(  ) < ch->pcdata->learned[gsn_bargain] )
      {
         double x = ( ( double )ch->pcdata->learned[gsn_bargain] / 3 ) / 100;
         double pct = cost * x;
         cost = cost - pct;

         ch_printf( ch, "Your bargaining skills have reduced the price by %d gold!\n\r", ( int )pct );
      }
      else
      {
         if( ch->pcdata->learned[gsn_bargain] > 0 )
         {
            send_to_char( "Your bargaining skills failed to reduce the price.\n\r", ch );
            learn_from_failure( ch, gsn_bargain );
         }
      }

      ch->gold -= ( int )cost;

      if( IS_OBJ_FLAG( obj, ITEM_INVENTORY ) )
      {
         OBJ_DATA *buy_obj, *bag;

         if( !( buy_obj = create_object( obj->pIndexData, obj->level ) ) )
         {
            ch->gold += ( int )cost;
            log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            return;
         }

         /*
          * Due to grouped objects and carry limitations in SMAUG
          * The shopkeeper gives you a bag with multiple-buy,
          * and also, only one object needs be created with a count
          * set to the number bought.    -Thoric
          */
         if( noi > 1 )
         {
            if( !( bag = create_object( get_obj_index( OBJ_VNUM_SHOPPING_BAG ), 1 ) ) )
            {
               ch->gold += ( int )cost;
               log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
               return;
            }
            SET_OBJ_FLAG( bag, ITEM_GROUNDROT );
            bag->timer = 10;  /* Blodkai, 4/97 */
            /*
             * perfect size bag ;) 
             */
            bag->value[0] = bag->weight + ( buy_obj->weight * noi );
            buy_obj->count = noi;
            obj->pIndexData->count += ( noi - 1 );
            numobjsloaded += ( noi - 1 );
            obj_to_obj( buy_obj, bag );
            obj_to_char( bag, ch );
         }
         else
            obj_to_char( buy_obj, ch );
      }
      else
      {
         obj_from_char( obj );
         obj_to_char( obj, ch );
         if( obj->pIndexData->vnum == OBJ_VNUM_TREASURE )
            obj->timer = 0;
      }

      if( found && clan->bank )
      {
         clan->balance += ( int )cost;
         save_clan( clan );
         save_shop( keeper );
      }
      else
      {
         keeper->gold += ( int )cost;
         if( IS_ACT_FLAG( keeper, ACT_GUILDVENDOR ) )
            save_shop( keeper );
      }
      return;
   }
}

/* Ok, sod all that crap about levels and dividers and stuff.
 * Lets just have a nice, simple list command.
 * Samson 8-27-03
 */
CMDF do_list( CHAR_DATA * ch, char *argument )
{
   if( IS_ROOM_FLAG( ch->in_room, ROOM_AUCTION ) )
   {
      ROOM_INDEX_DATA *aucvault, *original;
      CHAR_DATA *auc;
      bool found = FALSE;

      aucvault = get_room_index( ch->in_room->vnum + 1 );
      if( !aucvault )
      {
         bug( "do_list: bad auction house at vnum %d.", ch->in_room->vnum );
         send_to_char( "You can't do that here.\n\r", ch );
         return;
      }

      for( auc = ch->in_room->first_person; auc; auc = auc->next_in_room )
      {
         if( IS_NPC( auc ) && ( IS_ACT_FLAG( auc, ACT_AUCTION ) || IS_ACT_FLAG( auc, ACT_GUILDAUC ) ) )
            found = TRUE;
      }

      if( !found )
      {
         send_to_char( "There is no auctioneer here!\n\r", ch );
         return;
      }

      send_to_pager( "Items available for bidding:\n\r", ch );

      original = ch->in_room;

      char_from_room( ch );
      if( !char_to_room( ch, aucvault ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      show_list_to_char( ch->in_room->first_content, ch, TRUE, FALSE );
      char_from_room( ch );
      if( !char_to_room( ch, original ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      return;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_PET_SHOP ) )
   {
      ROOM_INDEX_DATA *pRoomIndexNext;
      CHAR_DATA *pet;
      bool found;

      pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
      if( !pRoomIndexNext )
      {
         bug( "Do_list: bad pet shop at vnum %d.", ch->in_room->vnum );
         send_to_char( "You can't do that here.\n\r", ch );
         return;
      }

      found = FALSE;

      for( pet = pRoomIndexNext->first_person; pet; pet = pet->next_in_room )
      {
         if( IS_ACT_FLAG( pet, ACT_PET ) )
         {
            if( !found )
            {
               found = TRUE;
               send_to_pager( "Pets for sale:\n\r", ch );
            }
            pager_printf( ch, "[%2d] %8d - %s\n\r", pet->level, 10 * pet->level * pet->level, pet->short_descr );
         }
      }
      if( !found )
         send_to_char( "Sorry, we're out of pets right now.\n\r", ch );
      return;
   }
   else
   {
      CHAR_DATA *keeper;
      OBJ_DATA *obj;
      int cost;

      if( ( keeper = find_keeper( ch ) ) == NULL )
         return;

      send_to_pager( "&w[Price] Item\n\r", ch );

      for( obj = keeper->first_carrying; obj; obj = obj->next_content )
      {
         if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj, FALSE )
             && ( cost = get_cost( ch, keeper, obj, TRUE ) ) > 0 )
         {
            mxpobjmenu = MXP_SHOP;
            mxptail[0] = '\0';
            pager_printf( ch, "[%6d] %s%s%s%s\n\r", cost,
                          mxp_obj_str( ch, obj ), obj->short_descr, mxp_obj_str_close( ch, obj ),
                          can_wear_obj( ch, obj ) ? "" : " &R*&w" );
         }
      }
      send_to_char( "A &R*&w indicates an item you are not able to use.\n\r", ch );
      return;
   }
}

CMDF do_sell( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *keeper, *auc;
   CLAN_DATA *clan = NULL;
   OBJ_DATA *obj;
   int cost;
   bool aucmob = FALSE, found = FALSE;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Sorry, Mobs cannot sell items!\n\r", ch );
      return;
   }

   for( auc = ch->in_room->first_person; auc; auc = auc->next_in_room )
   {
      if( IS_NPC( auc ) && ( IS_ACT_FLAG( auc, ACT_AUCTION ) || IS_ACT_FLAG( auc, ACT_GUILDAUC ) ) )
      {
         auction_sell( ch, auc, argument );
         aucmob = TRUE;
         break;
      }
   }

   if( aucmob )
      return;

   one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Sell what?\n\r", ch );
      return;
   }

   if( !( keeper = find_keeper( ch ) ) )
      return;

   if( IS_ACT_FLAG( keeper, ACT_GUILDVENDOR ) )
   {
      for( clan = first_clan; clan; clan = clan->next )
         if( keeper->pIndexData->vnum == clan->shopkeeper )
         {
            found = TRUE;
            break;
         }
   }

   if( ( obj = get_obj_carry( ch, arg ) ) == NULL )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      ch->reply = keeper;
      return;
   }

   /*
    * Bug report and solution thanks to animal@netwin.co.nz 
    */
   if( !can_see_obj( keeper, obj, FALSE ) )
   {
      send_to_char( "What are you trying to sell me? I don't buy thin air!\n\r", ch );
      return;
   }

   if( !can_drop_obj( ch, obj ) )
   {
      send_to_char( "You can't let go of it!\n\r", ch );
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_DONATION ) )
   {
      send_to_char( "You cannot sell donated goods!\n\r", ch );
      return;
   }

   if( obj->timer > 0 )
   {
      act( AT_TELL, "$n tells you, '$p is depreciating in value too quickly...'", keeper, obj, ch, TO_VICT );
      return;
   }

   if( ( cost = get_cost( ch, keeper, obj, FALSE ) ) <= 0 )
   {
      act( AT_ACTION, "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
      return;
   }

   if( found && clan->bank && cost >= clan->balance )
   {
      act_printf( AT_TELL, keeper, obj, ch, TO_VICT, "$n tells you, '$p is worth more than %s can afford...'", clan->name );
      return;
   }
   else
   {
      if( cost >= keeper->gold )
      {
         act( AT_TELL, "$n tells you, '$p is worth more than I can afford...'", keeper, obj, ch, TO_VICT );
         return;
      }
   }

   separate_obj( obj );
   act( AT_ACTION, "$n sells $p.", ch, obj, NULL, TO_ROOM );
   act_printf( AT_ACTION, ch, obj, NULL, TO_CHAR, "You sell $p for %d gold piece%s.", cost, cost == 1 ? "" : "s" );
   ch->gold += cost;

   if( obj->item_type == ITEM_TRASH )
      extract_obj( obj );
   else
   {
      obj_from_char( obj );
      obj_to_char( obj, keeper );
      if( obj->pIndexData->vnum == OBJ_VNUM_TREASURE )
         obj->timer = 50;
   }

   if( found && clan->bank )
   {
      clan->balance -= cost;
      if( clan->balance < 0 )
         clan->balance = 0;
      save_clan( clan );
      save_shop( keeper );
   }
   else
   {
      keeper->gold -= cost;
      if( keeper->gold < 0 )
         keeper->gold = 0;
      if( IS_ACT_FLAG( keeper, ACT_GUILDVENDOR ) )
         save_shop( keeper );
   }

   return;
}

CMDF do_value( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *keeper, *auc;
   OBJ_DATA *obj;
   int cost;
   bool aucmob = FALSE;

   for( auc = ch->in_room->first_person; auc; auc = auc->next_in_room )
   {
      if( IS_NPC( auc ) && ( IS_ACT_FLAG( auc, ACT_AUCTION ) || IS_ACT_FLAG( auc, ACT_GUILDAUC ) ) )
      {
         auction_value( ch, auc, argument );
         aucmob = TRUE;
         break;
      }
   }

   if( aucmob )
      return;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Value what?\n\r", ch );
      return;
   }

   if( !( keeper = find_keeper( ch ) ) )
      return;

   if( ( obj = get_obj_carry( ch, argument ) ) == NULL )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      ch->reply = keeper;
      return;
   }

   if( !can_drop_obj( ch, obj ) )
   {
      send_to_char( "You can't let go of it!\n\r", ch );
      return;
   }

   if( ( cost = get_cost( ch, keeper, obj, FALSE ) ) <= 0 )
   {
      act( AT_ACTION, "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
      return;
   }
   act_printf( AT_TELL, keeper, obj, ch, TO_VICT, "$n tells you 'I'd give you %d gold coins for $p.'", cost );
   ch->reply = keeper;
   return;
}

/*
 * Repair a single object. Used when handling "repair all" - Gorog
 */
void repair_one_obj( CHAR_DATA * ch, CHAR_DATA * keeper, OBJ_DATA * obj, char *arg, int maxgold, char *fixstr,
                     char *fixstr2 )
{
   CLAN_DATA *clan = NULL;
   int cost;
   bool found = FALSE;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the repair command.\n\r", ch );
      return;
   }

   if( !can_drop_obj( ch, obj ) )
   {
      ch_printf( ch, "You can't let go of %s.\n\r", obj->name );
      return;
   }

   if( ( cost = get_repaircost( keeper, obj ) ) < 0 )
   {
      if( cost != -2 )
         act( AT_TELL, "$n tells you, 'Sorry, I can't do anything with $p.'", keeper, obj, ch, TO_VICT );
      else
         act( AT_TELL, "$n tells you, '$p looks fine to me!'", keeper, obj, ch, TO_VICT );
      return;
   }

   if( IS_ACT_FLAG( keeper, ACT_GUILDREPAIR ) )
   {
      for( clan = first_clan; clan; clan = clan->next )
      {
         if( clan->repair == keeper->pIndexData->vnum )
         {
            found = TRUE;
            break;
         }
      }
   }

   if( found && clan == ch->pcdata->clan )
      cost = 0;

   /*
    * "repair all" gets a 10% surcharge - Gorog 
    */
   if( ( cost = strcmp( "all", arg ) ? cost : 11 * cost / 10 ) > ch->gold )
   {
      act_printf( AT_TELL, ch, NULL, keeper, TO_CHAR,
                  "$N tells you, 'It will cost %d piece%s of gold to %s %s...'", cost,
                  cost == 1 ? "" : "s", fixstr, obj->name );
      act( AT_TELL, "$N tells you, 'Which I see you can't afford.'", ch, NULL, keeper, TO_CHAR );
   }
   else
   {
      act_printf( AT_ACTION, ch, obj, keeper, TO_ROOM, "$n gives $p to $N, who quickly %s it.", fixstr2 );
      act_printf( AT_ACTION, ch, obj, keeper, TO_CHAR,
                  "$N charges you %d gold piece%s to %s $p.", cost, cost == 1 ? "" : "s", fixstr );
      ch->gold -= cost;

      if( found && clan->bank )
      {
         clan->balance += cost;
         save_clan( clan );
      }
      else
      {
         keeper->gold += cost;
         if( keeper->gold < 0 )
            keeper->gold = 0;
      }

      switch ( obj->item_type )
      {
         default:
            send_to_char( "For some reason, you think you got ripped off...\n\r", ch );
            break;
         case ITEM_ARMOR:
            obj->value[0] = obj->value[1];
            break;
         case ITEM_WEAPON:
         case ITEM_MISSILE_WEAPON:
            obj->value[6] = obj->value[0];
            break;
         case ITEM_PROJECTILE:
            obj->value[5] = obj->value[0];
            break;
         case ITEM_WAND:
         case ITEM_STAFF:
            obj->value[2] = obj->value[1];
            break;
      }
      oprog_repair_trigger( ch, obj );
   }
}

CMDF do_repair( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *keeper;
   OBJ_DATA *obj;
   char *fixstr, *fixstr2;
   int maxgold;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Repair what?\n\r", ch );
      return;
   }

   if( !( keeper = find_fixer( ch ) ) )
      return;

   maxgold = keeper->level * keeper->level * 100000;
   switch ( keeper->pIndexData->rShop->shop_type )
   {
      default:
      case SHOP_FIX:
         fixstr = "repair";
         fixstr2 = "repairs";
         break;
      case SHOP_RECHARGE:
         fixstr = "recharge";
         fixstr2 = "recharges";
         break;
   }

   if( !strcmp( argument, "all" ) )
   {
      for( obj = ch->first_carrying; obj; obj = obj->next_content )
      {
         if( can_see_obj( ch, obj, FALSE ) && can_see_obj( keeper, obj, FALSE )
             && ( obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON
                  || obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF ) )
            repair_one_obj( ch, keeper, obj, argument, maxgold, fixstr, fixstr2 );
      }
      return;
   }

   if( !( obj = get_obj_carry( ch, argument ) ) )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      ch->reply = keeper;
      return;
   }

   repair_one_obj( ch, keeper, obj, argument, maxgold, fixstr, fixstr2 );
   return;
}

void appraise_all( CHAR_DATA * ch, CHAR_DATA * keeper, char *fixstr )
{
   OBJ_DATA *obj;
   int cost = 0, total = 0;

   for( obj = ch->first_carrying; obj != NULL; obj = obj->next_content )
   {
      if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj, FALSE )
          && ( obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON
               || obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF ) )
      {
         if( !can_drop_obj( ch, obj ) )
            ch_printf( ch, "You can't let go of %s.\n\r", obj->name );
         else if( ( cost = get_repaircost( keeper, obj ) ) < 0 )
         {
            if( cost != -2 )
               act( AT_TELL, "$n tells you, 'Sorry, I can't do anything with $p.'", keeper, obj, ch, TO_VICT );
            else
               act( AT_TELL, "$n tells you, '$p looks fine to me!'", keeper, obj, ch, TO_VICT );
         }
         else
         {
            act_printf( AT_TELL, ch, NULL, keeper, TO_CHAR,
                        "$N tells you, 'It will cost %d piece%s of gold to %s %s'",
                        cost, cost == 1 ? "" : "s", fixstr, obj->name );
            total += cost;
         }
      }
   }
   if( total > 0 )
   {
      send_to_char( "\n\r", ch );
      act_printf( AT_TELL, ch, NULL, keeper, TO_CHAR,
                  "$N tells you, 'It will cost %d piece%s of gold in total.'", total, cost == 1 ? "" : "s" );
      act_printf( AT_TELL, ch, NULL, keeper, TO_CHAR, "$N tells you, 'Remember there is a 10%% surcharge for repair all.'" );
   }
}

CMDF do_appraise( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *keeper;
   OBJ_DATA *obj;
   int cost;
   char *fixstr;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Appraise what?\n\r", ch );
      return;
   }

   if( !( keeper = find_fixer( ch ) ) )
      return;

   switch ( keeper->pIndexData->rShop->shop_type )
   {
      default:
      case SHOP_FIX:
         fixstr = "repair";
         break;
      case SHOP_RECHARGE:
         fixstr = "recharge";
         break;
   }

   if( !strcmp( argument, "all" ) )
   {
      appraise_all( ch, keeper, fixstr );
      return;
   }

   if( !( obj = get_obj_carry( ch, argument ) ) )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      ch->reply = keeper;
      return;
   }

   if( !can_drop_obj( ch, obj ) )
   {
      send_to_char( "You can't let go of it.\n\r", ch );
      return;
   }

   if( ( cost = get_repaircost( keeper, obj ) ) < 0 )
   {
      if( cost != -2 )
         act( AT_TELL, "$n tells you, 'Sorry, I can't do anything with $p.'", keeper, obj, ch, TO_VICT );
      else
         act( AT_TELL, "$n tells you, '$p looks fine to me!'", keeper, obj, ch, TO_VICT );
      return;
   }

   act_printf( AT_TELL, ch, NULL, keeper, TO_CHAR,
               "$N tells you, 'It will cost %d piece%s of gold to %s that...'", cost, cost == 1 ? "" : "s", fixstr );
   if( cost > ch->gold )
      act( AT_TELL, "$N tells you, 'Which I see you can't afford.'", ch, NULL, keeper, TO_CHAR );

   return;
}

/* ------------------ Shop Building and Editing Section ----------------- */
CMDF do_makeshop( CHAR_DATA * ch, char *argument )
{
   SHOP_DATA *shop;
   CHAR_DATA *keeper;
   MOB_INDEX_DATA *mob;
   int vnum;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makeshop <mobname>\n\r", ch );
      return;
   }

   if( !( keeper = get_char_world( ch, argument ) ) )
   {
      send_to_char( "Mobile not found.\n\r", ch );
      return;
   }
   if( !IS_NPC( keeper ) )
   {
      send_to_char( "PCs can't keep shops.\n\r", ch );
      return;
   }
   mob = keeper->pIndexData;
   vnum = mob->vnum;

   if( !can_mmodify( ch, keeper ) )
      return;

   if( mob->pShop )
   {
      send_to_char( "This mobile already has a shop.\n\r", ch );
      return;
   }

   CREATE( shop, SHOP_DATA, 1 );

   LINK( shop, first_shop, last_shop, next, prev );
   shop->keeper = vnum;
   shop->profit_buy = 120;
   shop->profit_sell = 90;
   shop->open_hour = 0;
   shop->close_hour = sysdata.hoursperday;
   mob->pShop = shop;
   send_to_char( "Done.\n\r", ch );
   return;
}

CMDF do_shopset( CHAR_DATA * ch, char *argument )
{
   SHOP_DATA *shop;
   CHAR_DATA *keeper;
   MOB_INDEX_DATA *mob, *mob2;
   char arg1[MIL], arg2[MIL];
   int value;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !arg1 || arg1[0] == '\0' || !arg2 || arg2[0] == '\0' )
   {
      send_to_char( "Usage: shopset <mob vnum> <field> value\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "  buy0 buy1 buy2 buy3 buy4 buy sell open close keeper\n\r", ch );
      return;
   }

   if( !( keeper = get_char_world( ch, arg1 ) ) )
   {
      send_to_char( "Mobile not found.\n\r", ch );
      return;
   }
   if( !IS_NPC( keeper ) )
   {
      send_to_char( "PCs can't keep shops.\n\r", ch );
      return;
   }
   mob = keeper->pIndexData;

   if( !can_mmodify( ch, keeper ) )
      return;

   if( !mob->pShop )
   {
      send_to_char( "This mobile doesn't keep a shop.\n\r", ch );
      return;
   }
   shop = mob->pShop;
   value = atoi( argument );

   if( !str_cmp( arg2, "buy0" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         send_to_char( "Invalid item type!\n\r", ch );
         return;
      }
      shop->buy_type[0] = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "buy1" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         send_to_char( "Invalid item type!\n\r", ch );
         return;
      }
      shop->buy_type[1] = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "buy2" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         send_to_char( "Invalid item type!\n\r", ch );
         return;
      }
      shop->buy_type[2] = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "buy3" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         send_to_char( "Invalid item type!\n\r", ch );
         return;
      }
      shop->buy_type[3] = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "buy4" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         send_to_char( "Invalid item type!\n\r", ch );
         return;
      }
      shop->buy_type[4] = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "buy" ) )
   {
      if( value <= ( shop->profit_sell + 5 ) || value > 1000 )
      {
         send_to_char( "Out of range.\n\r", ch );
         return;
      }
      shop->profit_buy = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "sell" ) )
   {
      if( value < 0 || value >= ( shop->profit_buy - 5 ) )
      {
         send_to_char( "Out of range.\n\r", ch );
         return;
      }
      shop->profit_sell = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "open" ) )
   {
      if( value < 0 || value > sysdata.hoursperday )
      {
         ch_printf( ch, "Out of range. Valid values are from 0 to %d.\n\r", sysdata.hoursperday );
         return;
      }
      shop->open_hour = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "close" ) )
   {
      if( value < 0 || value > sysdata.hoursperday )
      {
         ch_printf( ch, "Out of range. Valid values are from 0 to %d.\n\r", sysdata.hoursperday );
         return;
      }
      shop->close_hour = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "keeper" ) )
   {
      int vnum = atoi( argument );
      if( !( mob2 = get_mob_index( vnum ) ) )
      {
         send_to_char( "Mobile not found.\n\r", ch );
         return;
      }
      if( !can_mmodify( ch, keeper ) )
         return;
      if( mob2->pShop )
      {
         send_to_char( "That mobile already has a shop.\n\r", ch );
         return;
      }
      mob->pShop = NULL;
      mob2->pShop = shop;
      shop->keeper = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   do_shopset( ch, "" );
   return;
}

CMDF do_shopstat( CHAR_DATA * ch, char *argument )
{
   SHOP_DATA *shop;
   CHAR_DATA *keeper;
   MOB_INDEX_DATA *mob;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: shopstat <keeper name>\n\r", ch );
      return;
   }

   if( !( keeper = get_char_world( ch, argument ) ) )
   {
      send_to_char( "Mobile not found.\n\r", ch );
      return;
   }
   if( !IS_NPC( keeper ) )
   {
      send_to_char( "PCs can't keep shops.\n\r", ch );
      return;
   }
   mob = keeper->pIndexData;

   if( !mob->pShop )
   {
      send_to_char( "This mobile doesn't keep a shop.\n\r", ch );
      return;
   }
   shop = mob->pShop;

   ch_printf( ch, "Keeper: %d  %s\n\r", shop->keeper, mob->short_descr );
   ch_printf( ch, "buy0 [%s]  buy1 [%s]  buy2 [%s]  buy3 [%s]  buy4 [%s]\n\r",
              o_types[shop->buy_type[0]], o_types[shop->buy_type[1]],
              o_types[shop->buy_type[2]], o_types[shop->buy_type[3]], o_types[shop->buy_type[4]] );
   ch_printf( ch, "Profit:  buy %3d%%  sell %3d%%\n\r", shop->profit_buy, shop->profit_sell );
   ch_printf( ch, "Hours:   open %2d  close %2d\n\r", shop->open_hour, shop->close_hour );
   return;
}

CMDF do_shops( CHAR_DATA * ch, char *argument )
{
   SHOP_DATA *shop;
   MOB_INDEX_DATA *mob;

   if( !first_shop )
   {
      send_to_char( "There are no shops.\n\r", ch );
      return;
   }

   send_to_char( "&WFor details on each shopkeeper, use the shopstat command.\n\r", ch );
   for( shop = first_shop; shop; shop = shop->next )
   {
      if( !( mob = get_mob_index( shop->keeper ) ) )
      {
         bug( "Bad mob index on shopkeeper %d", shop->keeper );
         continue;
      }
      ch_printf( ch, "&WShopkeeper: &G%-20.20s &WVnum: &G%-5d &WArea: &G%s\n\r", mob->short_descr, mob->vnum,
                 mob->area->name );
   }
   return;
}

/* -------------- Repair Shop Building and Editing Section -------------- */
CMDF do_makerepair( CHAR_DATA * ch, char *argument )
{
   REPAIR_DATA *repair;
   CHAR_DATA *keeper;
   MOB_INDEX_DATA *mob;
   int vnum;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makerepair <mobvnum>\n\r", ch );
      return;
   }

   if( !( keeper = get_char_world( ch, argument ) ) )
   {
      send_to_char( "Mobile not found.\n\r", ch );
      return;
   }
   if( !IS_NPC( keeper ) )
   {
      send_to_char( "PCs can't keep shops.\n\r", ch );
      return;
   }
   mob = keeper->pIndexData;
   vnum = mob->vnum;

   if( !can_mmodify( ch, keeper ) )
      return;

   if( mob->rShop )
   {
      send_to_char( "This mobile already has a repair shop.\n\r", ch );
      return;
   }

   CREATE( repair, REPAIR_DATA, 1 );

   LINK( repair, first_repair, last_repair, next, prev );
   repair->keeper = vnum;
   repair->profit_fix = 100;
   repair->shop_type = SHOP_FIX;
   repair->open_hour = 0;
   repair->close_hour = sysdata.hoursperday;
   mob->rShop = repair;
   send_to_char( "Done.\n\r", ch );
   return;
}

CMDF do_repairset( CHAR_DATA * ch, char *argument )
{
   REPAIR_DATA *repair;
   CHAR_DATA *keeper;
   MOB_INDEX_DATA *mob, *mob2;
   char arg1[MIL], arg2[MIL];
   int value;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !arg1 || arg1[0] == '\0' || !arg2 || arg2[0] == '\0' )
   {
      send_to_char( "Usage: repairset <mob vnum> <field> value\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "  fix0 fix1 fix2 profit type open close keeper\n\r", ch );
      return;
   }

   if( !( keeper = get_char_world( ch, arg1 ) ) )
   {
      send_to_char( "Mobile not found.\n\r", ch );
      return;
   }
   if( !IS_NPC( keeper ) )
   {
      send_to_char( "PCs can't keep shops.\n\r", ch );
      return;
   }
   mob = keeper->pIndexData;

   if( !can_mmodify( ch, keeper ) )
      return;

   if( !mob->rShop )
   {
      send_to_char( "This mobile doesn't keep a repair shop.\n\r", ch );
      return;
   }
   repair = mob->rShop;
   value = atoi( argument );

   if( !str_cmp( arg2, "fix0" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         send_to_char( "Invalid item type!\n\r", ch );
         return;
      }
      repair->fix_type[0] = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "fix1" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         send_to_char( "Invalid item type!\n\r", ch );
         return;
      }
      repair->fix_type[1] = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "fix2" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         send_to_char( "Invalid item type!\n\r", ch );
         return;
      }
      repair->fix_type[2] = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "profit" ) )
   {
      if( value < 1 || value > 1000 )
      {
         send_to_char( "Out of range.\n\r", ch );
         return;
      }
      repair->profit_fix = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( value < 1 || value > 2 )
      {
         send_to_char( "Out of range.\n\r", ch );
         return;
      }
      repair->shop_type = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "open" ) )
   {
      if( value < 0 || value > sysdata.hoursperday )
      {
         ch_printf( ch, "Out of range. Valid values are from 0 to %d.\n\r", sysdata.hoursperday );
         return;
      }
      repair->open_hour = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "close" ) )
   {
      if( value < 0 || value > sysdata.hoursperday )
      {
         ch_printf( ch, "Out of range. Valid values are from 0 to %d.\n\r", sysdata.hoursperday );
         return;
      }
      repair->close_hour = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "keeper" ) )
   {
      int vnum = atoi( argument );
      if( !( mob2 = get_mob_index( vnum ) ) )
      {
         send_to_char( "Mobile not found.\n\r", ch );
         return;
      }
      if( !can_mmodify( ch, keeper ) )
         return;
      if( mob2->rShop )
      {
         send_to_char( "That mobile already has a repair shop.\n\r", ch );
         return;
      }
      mob->rShop = NULL;
      mob2->rShop = repair;
      repair->keeper = value;
      send_to_char( "Done.\n\r", ch );
      return;
   }

   do_repairset( ch, "" );
   return;
}

CMDF do_repairstat( CHAR_DATA * ch, char *argument )
{
   REPAIR_DATA *repair;
   CHAR_DATA *keeper;
   MOB_INDEX_DATA *mob;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: repairstat <keeper vnum>\n\r", ch );
      return;
   }

   if( !( keeper = get_char_world( ch, argument ) ) )
   {
      send_to_char( "Mobile not found.\n\r", ch );
      return;
   }
   if( !IS_NPC( keeper ) )
   {
      send_to_char( "PCs can't keep shops.\n\r", ch );
      return;
   }
   mob = keeper->pIndexData;

   if( !mob->rShop )
   {
      send_to_char( "This mobile doesn't keep a repair shop.\n\r", ch );
      return;
   }
   repair = mob->rShop;

   ch_printf( ch, "Keeper: %d  %s\n\r", repair->keeper, mob->short_descr );
   ch_printf( ch, "fix0 [%s]  fix1 [%s]  fix2 [%s]\n\r",
              o_types[repair->fix_type[0]], o_types[repair->fix_type[1]], o_types[repair->fix_type[2]] );
   ch_printf( ch, "Profit: %3d%%  Type: %d\n\r", repair->profit_fix, repair->shop_type );
   ch_printf( ch, "Hours:   open %2d  close %2d\n\r", repair->open_hour, repair->close_hour );
   return;
}

CMDF do_repairshops( CHAR_DATA * ch, char *argument )
{
   REPAIR_DATA *repair;
   MOB_INDEX_DATA *mob;

   if( !first_repair )
   {
      send_to_char( "There are no repair shops.\n\r", ch );
      return;
   }

   send_to_char( "&WFor details on each repairsmith, use the repairstat command.\n\r", ch );
   for( repair = first_repair; repair; repair = repair->next )
   {
      if( !( mob = get_mob_index( repair->keeper ) ) )
      {
         bug( "Bad mob index on repairsmith %d", repair->keeper );
         continue;
      }
      ch_printf( ch, "&WRepairsmith: &G%-20.20s &WVnum: &G%-5d &WArea: &G%s\n\r", mob->short_descr, mob->vnum,
                 mob->area->name );
   }
   return;
}

/***************************************************************************  
 *                          SMAUG Banking Support Code                     *
 ***************************************************************************
 *                                                                         *
 * This code may be used freely, as long as credit is given in the help    *
 * file. Thanks.                                                           *
 *                                                                         *
 *                                        -= Minas Ravenblood =-           *
 *                                 Implementor of The Apocalypse Theatre   *
 *                                      (email: krisco7@hotmail.com)       *
 *                                                                         *
 ***************************************************************************/

/* Modifications to original source by Samson. Updated to include support for guild/clan bank accounts */

/* You can add this or just put it in the do_bank code. I don't really know
   why I made a seperate function for this, but I did. If you do add it,
   don't forget to declare it - Minas */

/* Finds banker mobs in a room. */
CHAR_DATA *find_banker( CHAR_DATA * ch )
{
   CHAR_DATA *banker = NULL;

   for( banker = ch->in_room->first_person; banker; banker = banker->next_in_room )
      if( IS_ACT_FLAG( banker, ACT_BANKER ) || IS_ACT_FLAG( banker, ACT_GUILDBANK ) )
         break;
   return banker;
}

/* SMAUG Bank Support
 * Coded by Minas Ravenblood for The Apocalypse Theatre
 * (email: krisco7@hotmail.com)
 */
/* Deposit, withdraw, balance and transfer commands */
CMDF do_deposit( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *banker;
   CLAN_DATA *clan = NULL;
   int amount;

   if( !( banker = find_banker( ch ) ) )
   {
      send_to_char( "You're not in a bank!\n\r", ch );
      return;
   }

   if( IS_NPC( ch ) )
   {
      interpret( banker, "say Sorry, we don't do business with mobs." );
      return;
   }

   if( IS_ACT_FLAG( banker, ACT_GUILDBANK ) )
   {
      for( clan = first_clan; clan; clan = clan->next )
      {
         if( clan->bank == banker->pIndexData->vnum )
            break;
      }
   }

   if( clan && ch->pcdata->clan != clan )
   {
      interpret( banker, "say Beat it! This is a private vault!" );
      return;
   }

   if( !argument || argument == '\0' )
   {
      send_to_char( "How much gold do you wish to deposit?\n\r", ch );
      return;
   }

   if( str_cmp( argument, "all" ) && !is_number( argument ) )
   {
      send_to_char( "How much gold do you wish to deposit?\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
      amount = ch->gold;
   else
      amount = atoi( argument );

   if( amount > ch->gold )
   {
      send_to_char( "You don't have that much gold to deposit.\n\r", ch );
      return;
   }

   if( amount <= 0 )
   {
      send_to_char( "Oh, I see.. your a comedian.\n\r", ch );
      return;
   }

   ch->gold -= amount;

   if( clan )
   {
      clan->balance += amount;
      save_clan( clan );
   }
   else
      ch->pcdata->balance += amount;

   ch_printf( ch, "&[gold]You deposit %d gold.\n\r", amount );
   act_printf( AT_GOLD, ch, NULL, NULL, TO_ROOM, "$n deposits %d gold.", amount );
   sound_to_char( "gold.wav", 100, ch, FALSE );
   save_char_obj( ch ); /* Prevent money duplication for clan accounts - Samson */
   return;
}

CMDF do_withdraw( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *banker;
   CLAN_DATA *clan = NULL;
   int amount;

   if( !( banker = find_banker( ch ) ) )
   {
      send_to_char( "You're not in a bank!\n\r", ch );
      return;
   }

   if( IS_NPC( ch ) )
   {
      interpret( banker, "say Sorry, we don't do business with mobs." );
      return;
   }

   if( IS_ACT_FLAG( banker, ACT_GUILDBANK ) )
   {
      for( clan = first_clan; clan; clan = clan->next )
      {
         if( clan->bank == banker->pIndexData->vnum )
            break;
      }
   }

   if( clan && ch->pcdata->clan != clan )
   {
      interpret( banker, "say Beat it! This is a private vault!" );
      return;
   }

   if( clan && str_cmp( ch->name, clan->leader )
       && str_cmp( ch->name, clan->number1 ) && str_cmp( ch->name, clan->number2 ) )
   {
      ch_printf( ch, "Sorry, only the %s officers are allowed to withdraw funds.\n\r",
                 clan->clan_type == CLAN_ORDER ? "guild" : "clan" );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "How much gold do you wish to withdraw?\n\r", ch );
      return;
   }
   if( str_cmp( argument, "all" ) && !is_number( argument ) )
   {
      send_to_char( "How much gold do you wish to withdraw?\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      if( clan )
         amount = clan->balance;
      else
         amount = ch->pcdata->balance;
   }
   else
      amount = atoi( argument );

   if( clan )
   {
      if( amount > clan->balance )
      {
         send_to_char( "There isn't that much gold in the vault!\n\r", ch );
         return;
      }
   }
   else
   {
      if( amount > ch->pcdata->balance )
      {
         send_to_char( "But you do not have that much gold in your account!\n\r", ch );
         return;
      }
   }

   if( amount <= 0 )
   {
      send_to_char( "Oh I see.. your a comedian.\n\r", ch );
      return;
   }

   if( clan )
   {
      clan->balance -= amount;
      save_clan( clan );
   }
   else
      ch->pcdata->balance -= amount;

   ch->gold += amount;
   ch_printf( ch, "&[gold]You withdraw %d gold.\n\r", amount );
   act_printf( AT_GOLD, ch, NULL, NULL, TO_ROOM, "$n withdraws %d gold.", amount );
   sound_to_char( "gold.wav", 100, ch, FALSE );
   save_char_obj( ch ); /* Prevent money duplication for clan accounts - Samson */
   return;
}

CMDF do_balance( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *banker;
   CLAN_DATA *clan = NULL;
   bool found = FALSE;

   if( !( banker = find_banker( ch ) ) )
   {
      send_to_char( "You're not in a bank!\n\r", ch );
      return;
   }

   if( IS_NPC( ch ) )
   {
      interpret( banker, "say Sorry, %s, we don't do business with mobs." );
      return;
   }

   if( IS_ACT_FLAG( banker, ACT_GUILDBANK ) )
   {
      for( clan = first_clan; clan; clan = clan->next )
      {
         if( clan->bank == banker->pIndexData->vnum )
         {
            found = TRUE;
            break;
         }
      }
   }

   if( found && ch->pcdata->clan != clan )
   {
      interpret( banker, "say Beat it! This is a private vault!" );
      return;
   }

   if( found )
   {
      ch_printf( ch, "&[gold]The %s has %d gold in the vault.\n\r",
                 clan->clan_type == CLAN_ORDER ? "guild" : "clan", clan->balance );
   }
   else
      ch_printf( ch, "&[gold]You have %d gold in the bank.\n\r", ch->pcdata->balance );

   return;
}

/* End of new bank support */
