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
 *                          Auction House module                            *
 ****************************************************************************/

#include <stdarg.h>
#include <ctype.h>
#include <dirent.h>
#include "mud.h"
#include "auction.h"
#include "bet.h"
#include "clans.h"
#include "event.h"

SALE_DATA *first_sale;
SALE_DATA *last_sale;

#define MAX_NEST	100
static OBJ_DATA *rgObjNest[MAX_NEST];

void bid( CHAR_DATA * ch, CHAR_DATA * buyer, char *argument );
bool exists_player( char *name );
void save_clan( CLAN_DATA * clan );
void save_clan_storeroom( CHAR_DATA * ch, CLAN_DATA * clan );
void fwrite_obj( CHAR_DATA * ch, OBJ_DATA * obj, CLAN_DATA * clan, FILE * fp, int iNest, short os_type, bool hotboot );
void fread_obj( CHAR_DATA * ch, FILE * fp, short os_type );
char *wear_bit_name( int wear_flags );

void save_sales( void )
{
   SALE_DATA *sale;
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%s%s", AUC_DIR, SALES_FILE );

   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "%s", "save_sales: fopen" );
      perror( filename );
   }
   else
   {
      for( sale = first_sale; sale; sale = sale->next )
      {
         fprintf( fp, "%s", "#SALE\n" );
         fprintf( fp, "Aucmob		%s~\n", sale->aucmob );
         fprintf( fp, "Seller		%s~\n", sale->seller );
         fprintf( fp, "Buyer		%s~\n", sale->buyer );
         fprintf( fp, "Item		%s~\n", sale->item );
         fprintf( fp, "Bid		%d\n", sale->bid );
         fprintf( fp, "Collected	%d\n", sale->collected );
         fprintf( fp, "%s", "End\n\n" );
      }
      fprintf( fp, "%s", "#END\n" );
      FCLOSE( fp );
   }
   return;
}

void add_sale( char *aucmob, char *seller, char *buyer, char *item, int bidamt, bool collected )
{
   SALE_DATA *sale;

   CREATE( sale, SALE_DATA, 1 );
   LINK( sale, first_sale, last_sale, next, prev );
   sale->aucmob = STRALLOC( aucmob );
   sale->seller = STRALLOC( seller );
   sale->buyer = STRALLOC( buyer );
   sale->item = STRALLOC( item );
   sale->bid = bidamt;
   sale->collected = collected;

   save_sales(  );
   return;
}

void remove_sale( SALE_DATA * sold )
{
   UNLINK( sold, first_sale, last_sale, next, prev );
   STRFREE( sold->aucmob );
   STRFREE( sold->seller );
   STRFREE( sold->buyer );
   STRFREE( sold->item );
   DISPOSE( sold );

   if( !mud_down )
      save_sales(  );
   return;
}

void free_sales( void )
{
   SALE_DATA *sale, *sale_next;

   for( sale = first_sale; sale; sale = sale_next )
   {
      sale_next = sale->next;
      remove_sale( sale );
   }
   return;
}

SALE_DATA *check_sale( char *aucmob, char *pcname, char *objname )
{
   SALE_DATA *sale;

   for( sale = first_sale; sale; sale = sale->next )
   {
      if( !str_cmp( aucmob, sale->aucmob ) )
      {
         if( !str_cmp( pcname, sale->seller ) )
         {
            if( !str_cmp( objname, sale->item ) )
               return sale;
         }
      }
   }
   return NULL;
}

void sale_count( CHAR_DATA * ch )
{
   SALE_DATA *sale;
   short salecount = 0;

   for( sale = first_sale; sale; sale = sale->next )
      if( !str_cmp( sale->seller, ch->name ) )
         salecount++;

   if( salecount > 0 )
   {
      ch_printf( ch, "&[auction]While you were gone, auctioneers sold %d items for you.\n\r", salecount );
      send_to_char( "Use the 'saleslist' command to see which auctioneer sold what.\n\r", ch );
   }
   return;
}

CMDF do_saleslist( CHAR_DATA * ch, char *argument )
{
   SALE_DATA *sale;
   short salecount = 0;

   for( sale = first_sale; sale; sale = sale->next )
   {
      if( !str_cmp( sale->seller, ch->name ) || IS_IMP( ch ) )
      {
         ch_printf( ch, "&[auction]%s sold %s for %s while %s were away.\n\r",
                    sale->aucmob, sale->item, ( !str_cmp( sale->seller, ch->name ) ? "you" : sale->seller ),
                    ( !str_cmp( sale->seller, ch->name ) ? "you" : "they" ) );
         salecount++;
      }
   }

   if( salecount == 0 )
      send_to_char( "&[auction]You haven't sold any items at auction yet.\n\r", ch );

   return;
}

void prune_sales( void )
{
   SALE_DATA *sale, *sale_next;

   for( sale = first_sale; sale; sale = sale_next )
   {
      sale_next = sale->next;

      if( !exists_player( sale->buyer ) && !exists_player( sale->seller ) )
      {
         remove_sale( sale );
         continue;
      }
      if( !exists_player( sale->seller ) && sale->collected )
      {
         remove_sale( sale );
         continue;
      }
      if( !exists_player( sale->buyer ) && !sale->collected && str_cmp( sale->buyer, "The Code" ) )
      {
         STRFREE( sale->buyer );
         sale->buyer = STRALLOC( "The Code" );
         save_sales(  );
         continue;
      }
   }
   return;
}

void fread_sale( SALE_DATA * sale, FILE * fp )
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

         case 'A':
            KEY( "Aucmob", sale->aucmob, fread_string( fp ) );
            break;

         case 'B':
            KEY( "Bid", sale->bid, fread_number( fp ) );
            KEY( "Buyer", sale->buyer, fread_string( fp ) );
            break;

         case 'C':
            KEY( "Collected", sale->collected, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'I':
            KEY( "Item", sale->item, fread_string( fp ) );
            break;

         case 'S':
            KEY( "Seller", sale->seller, fread_string( fp ) );
            break;
      }
      if( !fMatch )
         bug( "Fread_sale: no match: %s", word );
   }
}

void load_sales( void )
{
   char filename[256];
   SALE_DATA *sale;
   FILE *fp;

   first_sale = NULL;
   last_sale = NULL;

   snprintf( filename, 256, "%s%s", AUC_DIR, SALES_FILE );

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
            bug( "%s", "Load_sales: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "SALE" ) )
         {
            CREATE( sale, SALE_DATA, 1 );
            fread_sale( sale, fp );
            LINK( sale, first_sale, last_sale, next, prev );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_sales: bad section: %s.", word );
            continue;
         }
      }
      FCLOSE( fp );
   }

   prune_sales(  );

   return;
}

void read_aucvault( char *dirname, char *filename )
{
   ROOM_INDEX_DATA *aucvault;
   CHAR_DATA *aucmob;
   FILE *fp;
   char fname[256];

   if( !( aucmob = get_char_world( supermob, filename ) ) )
   {
      bug( "Um. Missing mob for %s's auction vault.", filename );
      return;
   }

   aucvault = get_room_index( aucmob->in_room->vnum + 1 );

   if( !aucvault )
   {
      bug( "Ooops! The vault room for %s's auction house is missing!", aucmob->short_descr );
      return;
   }

   snprintf( fname, 256, "%s%s", dirname, filename );
   if( ( fp = fopen( fname, "r" ) ) != NULL )
   {
      short iNest;
      bool found;
      OBJ_DATA *tobj, *tobj_next;

      log_printf( "Loading auction house vault: %s", filename );
      rset_supermob( aucvault );
      for( iNest = 0; iNest < MAX_NEST; iNest++ )
         rgObjNest[iNest] = NULL;

      found = TRUE;
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
            bug( "read_aucvault: # not found. %s", aucmob->short_descr );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "OBJECT" ) ) /* Objects  */
            fread_obj( supermob, fp, OS_CARRY );
         else if( !str_cmp( word, "END" ) )  /* Done     */
            break;
         else
         {
            bug( "read_aucvault: bad section. %s", aucmob->short_descr );
            break;
         }
      }
      FCLOSE( fp );
      for( tobj = supermob->first_carrying; tobj; tobj = tobj_next )
      {
         tobj_next = tobj->next_content;
         if( tobj->year == 0 )
         {
            tobj->day = time_info.day;
            tobj->month = time_info.month;
            tobj->year = time_info.year + 1;
         }
         obj_from_char( tobj );
         obj_to_room( tobj, aucvault, supermob );
      }
      release_supermob(  );
   }
   else
      log_string( "Cannot open auction vault" );

   return;
}

void load_aucvaults( void )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];

   mudstrlcpy( directory_name, AUC_DIR, 100 );
   dp = opendir( directory_name );
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( str_cmp( dentry->d_name, "CVS" ) )
         {
            if( str_cmp( dentry->d_name, "sales.dat" ) )
               read_aucvault( directory_name, dentry->d_name );
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );
   return;
}

void save_aucvault( CHAR_DATA * ch, char *aucmob )
{
   ROOM_INDEX_DATA *aucvault;
   OBJ_DATA *contents;
   FILE *fp;
   char filename[256];
   short templvl;

   if( !ch )
   {
      bug( "%s", "save_aucvault: NULL ch!" );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   snprintf( filename, 256, "%s%s", AUC_DIR, aucmob );
   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "%s", "save_aucvault: fopen" );
      perror( filename );
   }
   else
   {
      templvl = ch->level;
      ch->level = LEVEL_AVATAR;  /* make sure EQ doesn't get lost */
      contents = aucvault->last_content;
      if( contents )
         fwrite_obj( ch, contents, NULL, fp, 0, OS_CARRY, FALSE );
      fprintf( fp, "%s", "#END\n" );
      ch->level = templvl;
      FCLOSE( fp );
      return;
   }

   return;
}

/*
 * this function sends raw argument over the AUCTION: channel
 * I am not too sure if this method is right..
 */
void talk_auction( char *fmt, ... )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *original;
   char buf[MSL];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL, fmt, args );
   va_end( args );

   for( d = first_descriptor; d; d = d->next )
   {
      original = d->original ? d->original : d->character;  /* if switched */
      if( d->connected == CON_PLAYING && hasname( original->pcdata->chan_listen, "auction" )
          && !IS_ROOM_FLAG( original->in_room, ROOM_SILENCE ) && original->level > 1 )
         act_printf( AT_AUCTION, original, NULL, NULL, TO_CHAR, "Auction: %s", buf );
   }
}

/* put an item on auction, or see the stats on the current item or bet */
void bid( CHAR_DATA * ch, CHAR_DATA * buyer, char *argument )
{
   OBJ_DATA *obj;
   char arg1[MIL], arg2[MIL];

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   set_char_color( AT_AUCTION, ch );

   /*
    * Normal NPCs not allowed to auction - Samson 6-23-99 
    */
   if( IS_NPC( ch ) && ch != supermob )
   {
      if( !IS_ACT_FLAG( ch, ACT_GUILDAUC ) && !IS_ACT_FLAG( ch, ACT_AUCTION ) )
         return;
   }

   if( !arg1 || arg1[0] == '\0' )
   {
      if( auction->item != NULL )
      {
         obj = auction->item;

         /*
          * show item data here 
          */
         if( auction->bet > 0 )
            ch_printf( ch, "\n\rCurrent bid on this item is %d gold.\n\r", auction->bet );
         else
            ch_printf( ch, "\n\rNo bids on this item have been received.\n\r" );

         if( IS_IMMORTAL( ch ) )
            ch_printf( ch, "Seller: %s.  Bidder: %s.  Round: %d.\n\r",
                       auction->seller->name, auction->buyer->name, ( auction->going + 1 ) );
         return;
      }
      else
      {
         send_to_char( "\n\rThere is nothing being auctioned right now.\n\r", ch );
         return;
      }
   }

   if( ( IS_IMMORTAL( ch ) || ch == supermob ) && !str_cmp( arg1, "stop" ) )
   {
      if( auction->item == NULL )
      {
         send_to_char( "There is no auction to stop.\n\r", ch );
         return;
      }
      else  /* stop the auction */
      {
         ROOM_INDEX_DATA *aucvault;

         aucvault = get_room_index( auction->seller->in_room->vnum + 1 );

         talk_auction( "Sale of %s has been stopped.", auction->item->short_descr );

         obj_to_room( auction->item, aucvault, auction->seller );
         /*
          * Make sure vault is saved when we stop an auction - Samson 6-23-99 
          */
         save_aucvault( auction->seller, auction->seller->short_descr );

         auction->item = NULL;
         if( auction->buyer != NULL && auction->buyer != auction->seller )
            send_to_char( "Your bid has been cancelled.\n\r", auction->buyer );
         return;
      }
   }

   if( !str_cmp( arg1, "bid" ) )
   {
      if( auction->item != NULL )
      {
         int newbet;

         if( ch == auction->seller )
         {
            send_to_char( "You can't bid on your own item!\n\r", ch );
            return;
         }

         if( !str_cmp( ch->name, auction->item->seller ) )
         {
            send_to_char( "You cannot bid on your own item!\n\r", ch );
            return;
         }

         /*
          * make - perhaps - a bet now 
          */
         if( !arg2 || arg2[0] == '\0' )
         {
            send_to_char( "Bid how much?\n\r", ch );
            return;
         }

         newbet = parsebet( auction->bet, arg2 );

         if( newbet < auction->starting + 500 )
         {
            send_to_char( "You must place a bid that is higher than the starting bid.\n\r", ch );
            return;
         }

         /*
          * to avoid slow auction, use a bigger amount than 100 if the bet
          * is higher up - changed to 10000 for our high economy
          */

         if( newbet < ( auction->bet + 500 ) )
         {
            send_to_char( "You must at least bid 500 coins over the current bid.\n\r", ch );
            return;
         }

         if( newbet > ch->gold )
         {
            send_to_char( "You don't have that much money!\n\r", ch );
            return;
         }

         if( newbet > 2000000000 )
         {
            send_to_char( "You can't bid over 2 billion coins.\n\r", ch );
            return;
         }

         /*
          * Is it the item they really want to bid on? --Shaddai 
          */
         if( !argument || argument[0] != '\0' || !nifty_is_name( argument, auction->item->name ) )
         {
            send_to_char( "That item is not being auctioned right now.\n\r", ch );
            return;
         }
         /*
          * the actual bet is OK! 
          */

         auction->buyer = ch;
         auction->bet = newbet;
         auction->going = 0;

         talk_auction( "A bid of %d gold has been received on %s.\n\r", newbet, auction->item->short_descr );

         return;
      }
      else
      {
         send_to_char( "There isn't anything being auctioned right now.\n\r", ch );
         return;
      }
   }
/* finally... */

   if( !IS_NPC( ch ) )  /* Blocks PCs from bypassing the auctioneer - Samson 6-23-99 */
   {
      send_to_char( "Only an auctioneer can start the bidding for an item!\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   obj = get_obj_carry( ch, arg1 ); /* does char have the item ? */

   if( obj == NULL )
   {
      bug( "bid: Auctioneer %s isn't carrying the item!", ch->short_descr );
      return;
   }

   if( auction->item == NULL )
   {
      switch ( obj->item_type )
      {

         default:
            bug( "bid: Auctioneer %s tried to auction invalid item type!", ch->short_descr );
            return;

            /*
             * insert any more item types here... items with a timer MAY NOT BE AUCTIONED! 
             */
         case ITEM_LIGHT:
         case ITEM_TREASURE:
         case ITEM_POTION:
         case ITEM_CONTAINER:
         case ITEM_INSTRUMENT:
         case ITEM_KEYRING:
         case ITEM_QUIVER:
         case ITEM_DRINK_CON:
         case ITEM_FOOD:
         case ITEM_COOK:
         case ITEM_PEN:
         case ITEM_CAMPGEAR:
         case ITEM_BOAT:
         case ITEM_PILL:
         case ITEM_PIPE:
         case ITEM_HERB_CON:
         case ITEM_INCENSE:
         case ITEM_FIRE:
         case ITEM_RUNEPOUCH:
         case ITEM_MAP:
         case ITEM_BOOK:
         case ITEM_RUNE:
         case ITEM_MATCH:
         case ITEM_HERB:
         case ITEM_WEAPON:
         case ITEM_MISSILE_WEAPON:
         case ITEM_ARMOR:
         case ITEM_STAFF:
         case ITEM_WAND:
         case ITEM_SCROLL:
         case ITEM_ORE:
         case ITEM_PROJECTILE:
            separate_obj( obj );
            obj_from_char( obj );
            auction->item = obj;
            auction->bet = 0;
            if( buyer == NULL )
               auction->buyer = ch;
            else
               auction->buyer = buyer;
            auction->seller = ch;
            auction->going = 0;
            auction->starting = obj->bid;

            if( auction->starting > 0 )
               auction->bet = auction->starting;

            talk_auction( "Bidding begins on %s at %d gold.", obj->short_descr, auction->starting );

            /*
             * Setup the auction event 
             */
            add_event( sysdata.auctionseconds, ev_auction, NULL );
            return;
      }  /* switch */
   }
   else
   {
      act( AT_TELL, "Try again later - $p is being auctioned right now!", ch, auction->item, NULL, TO_CHAR );
      if( !IS_IMMORTAL( ch ) )
         WAIT_STATE( ch, sysdata.pulseviolence );
      return;
   }
}

CMDF do_bid( CHAR_DATA * ch, char *argument )
{
   char buf[MIL];

   if( IS_IMMORTAL( ch ) )
   {
      if( argument[0] == '\0' )
      {
         bid( ch, NULL, "" );
         return;
      }

      if( !str_cmp( argument, "stop" ) )
      {
         bid( ch, NULL, "stop" );
         return;
      }
   }
   snprintf( buf, MIL, "bid %s", argument );
   bid( ch, NULL, buf );
   return;
}

CMDF do_identify( CHAR_DATA * ch, char *argument )
{
   CLAN_DATA *clan = NULL;
   ROOM_INDEX_DATA *aucvault, *original;
   OBJ_DATA *obj;
   CHAR_DATA *auc;
   AFFECT_DATA *paf;
   SKILLTYPE *sktmp;
   double idcost;
   bool aucmob = FALSE;
   bool found = FALSE;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use this command.\n\r", ch );
      return;
   }

   set_char_color( AT_AUCTION, ch );

   for( auc = ch->in_room->first_person; auc; auc = auc->next_in_room )
      if( IS_NPC( auc ) && ( IS_ACT_FLAG( auc, ACT_AUCTION ) || IS_ACT_FLAG( auc, ACT_GUILDAUC ) ) )
      {
         aucmob = TRUE;
         break;
      }

   if( !aucmob )
   {
      send_to_char( "You must go to an auction house to use this command.\n\r", ch );
      return;
   }

   if( !IS_ROOM_FLAG( ch->in_room, ROOM_AUCTION ) )
   {
      send_to_char( "This is not an auction house!\n\r", ch );
      bug( "do_identify: Auction mob in non-auction room %d!", ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      send_to_char( "This is not an auction house!\n\r", ch );
      bug( "do_identify: Missing auction vault for room %d!", ch->in_room->vnum );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "What would you like identified?\n\r", ch );
      return;
   }

   original = ch->in_room;

   char_from_room( ch );
   if( !char_to_room( ch, aucvault ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->first_content );
   char_from_room( ch );
   if( !char_to_room( ch, original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   if( !obj || ( obj->buyer != NULL && str_cmp( obj->buyer, "" ) ) )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument );
      return;
   }

   if( !str_cmp( obj->seller, "" ) || obj->seller == NULL )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument );
      bug( "auction_value: Object with no seller - %s", obj->short_descr );
      return;
   }

   idcost = 5000 + ( obj->cost * 0.1 );

   if( ch->gold - idcost < 0 )
   {
      act( AT_TELL, "$n tells you 'You cannot afford to identify that!'", auc, NULL, ch, TO_VICT );
      return;
   }

   if( IS_ACT_FLAG( auc, ACT_GUILDAUC ) )
   {
      for( clan = first_clan; clan; clan = clan->next )
      {
         if( clan->auction == auc->pIndexData->vnum )
         {
            found = TRUE;
            break;
         }
      }
   }

   if( found && clan == ch->pcdata->clan )
      ;
   else
   {
      act_printf( AT_AUCTION, auc, NULL, ch, TO_VICT, "$n charges you %d gold for the identification.", idcost );
      ch->gold -= ( int )idcost;
      if( found && clan->bank )
      {
         clan->balance += ( int )idcost;
         save_clan( clan );
      }
   }

   ch_printf( ch, "\n\rObject: %s\n\r", obj->short_descr );
   if( ch->level >= LEVEL_IMMORTAL )
      ch_printf( ch, "Vnum: %d\n\r", obj->pIndexData->vnum );
   ch_printf( ch, "Keywords: %s\n\r", obj->name );
   ch_printf( ch, "Type: %s\n\r", item_type_name( obj ) );
   if( ch->level >= LEVEL_IMMORTAL )
      ch_printf( ch, "Wear Flags : %s\n\r", wear_bit_name( obj->wear_flags ) );
   ch_printf( ch, "Extra Flags: %s\n\r", extra_bit_name( &obj->extra_flags ) );
   ch_printf( ch, "Magic Flags: %s\n\r", magic_bit_name( obj->magic_flags ) );
   ch_printf( ch, "Weight: %d  Value: %d  Rent Cost: %d\n\r", obj->weight, obj->cost, obj->rent );
   set_char_color( AT_MAGIC, ch );

   switch ( obj->item_type )
   {
      case ITEM_CONTAINER:
         ch_printf( ch, "%s appears to be %s.\n\r", capitalize( obj->short_descr ),
                    obj->value[0] < 76 ? "of a small capacity" :
                    obj->value[0] < 150 ? "of a small to medium capacity" :
                    obj->value[0] < 300 ? "of a medium capacity" :
                    obj->value[0] < 550 ? "of a medium to large capacity" :
                    obj->value[0] < 751 ? "of a large capacity" : "of a giant capacity" );
         break;

      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
         ch_printf( ch, "Level %d spells of:", obj->value[0] );

         if( obj->value[1] >= 0 && ( sktmp = get_skilltype( obj->value[1] ) ) != NULL )
         {
            send_to_char( " '", ch );
            send_to_char( sktmp->name, ch );
            send_to_char( "'", ch );
         }

         if( obj->value[2] >= 0 && ( sktmp = get_skilltype( obj->value[2] ) ) != NULL )
         {
            send_to_char( " '", ch );
            send_to_char( sktmp->name, ch );
            send_to_char( "'", ch );
         }

         if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != NULL )
         {
            send_to_char( " '", ch );
            send_to_char( sktmp->name, ch );
            send_to_char( "'", ch );
         }

         send_to_char( ".\n\r", ch );
         break;

      case ITEM_SALVE:
         ch_printf( ch, "Has %d(%d) applications of level %d", obj->value[1], obj->value[2], obj->value[0] );
         if( obj->value[4] >= 0 && ( sktmp = get_skilltype( obj->value[4] ) ) != NULL )
         {
            send_to_char( " '", ch );
            send_to_char( sktmp->name, ch );
            send_to_char( "'", ch );
         }
         if( obj->value[5] >= 0 && ( sktmp = get_skilltype( obj->value[5] ) ) != NULL )
         {
            send_to_char( " '", ch );
            send_to_char( sktmp->name, ch );
            send_to_char( "'", ch );
         }
         send_to_char( ".\n\r", ch );
         break;

      case ITEM_WAND:
      case ITEM_STAFF:
         ch_printf( ch, "Has %d(%d) charges of level %d", obj->value[1], obj->value[2], obj->value[0] );

         if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != NULL )
         {
            send_to_char( " '", ch );
            send_to_char( sktmp->name, ch );
            send_to_char( "'", ch );
         }

         send_to_char( ".\n\r", ch );
         break;

      case ITEM_WEAPON:
         ch_printf( ch, "Damage is %d to %d (average %d)%s\n\r",
                    obj->value[1], obj->value[2],
                    ( obj->value[1] + obj->value[2] ) / 2, IS_OBJ_FLAG( obj, ITEM_POISONED ) ? ", and is poisonous." : "." );
         break;

      case ITEM_ARMOR:
         ch_printf( ch, "Base Armor Class is -%d.\n\r", obj->value[0] );
         break;
   }
   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      showaffect( ch, paf );

   for( paf = obj->first_affect; paf; paf = paf->next )
      showaffect( ch, paf );

   return;
}

CMDF do_collect( CHAR_DATA * ch, char *argument )
{
   CLAN_DATA *clan = NULL;
   SALE_DATA *sold, *sold_next;
   ROOM_INDEX_DATA *aucvault, *original;
   OBJ_DATA *obj;
   CHAR_DATA *auc;
   bool aucmob = FALSE;
   bool found = FALSE;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use this command.\n\r", ch );
      return;
   }

   for( auc = ch->in_room->first_person; auc; auc = auc->next_in_room )
   {
      if( IS_NPC( auc ) && ( IS_ACT_FLAG( auc, ACT_AUCTION ) || IS_ACT_FLAG( auc, ACT_GUILDAUC ) ) )
      {
         aucmob = TRUE;
         break;
      }
   }

   if( !aucmob )
   {
      send_to_char( "You must go to an auction house to collect for an auction.\n\r", ch );
      return;
   }

   if( !IS_ROOM_FLAG( ch->in_room, ROOM_AUCTION ) )
   {
      send_to_char( "This is not an auction house!\n\r", ch );
      bug( "do_collect: Auction mob in non-auction room %d!", ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      send_to_char( "This is not an auction house!\n\r", ch );
      bug( "do_collect: Missing auction vault for room %d!", ch->in_room->vnum );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Are you here to collect an item, or some money?\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "money" ) )
   {
      double totalfee = 0, totalnet = 0, fee, net;
      bool getsome = FALSE;

      for( sold = first_sale; sold; sold = sold_next )
      {
         sold_next = sold->next;

         if( str_cmp( sold->seller, ch->name ) )
            continue;

         if( sold->collected == FALSE && str_cmp( sold->buyer, "The Code" ) )
         {
            act_printf( AT_AUCTION, auc, NULL, ch, TO_VICT, "%s has not collected %s yet.", sold->buyer, sold->item );
            continue;
         }

         getsome = TRUE;

         fee = ( sold->bid * 0.05 );
         net = sold->bid - fee;

         act_printf( AT_AUCTION, auc, NULL, ch, TO_VICT, "$n sold %s to %s for %d gold.", sold->item, sold->buyer,
                     sold->bid );

         totalfee += fee;
         totalnet += net;

         remove_sale( sold );
      }

      if( !getsome )
      {
         act( AT_TELL, "$n tells you 'But you have not sold anything here!'", auc, NULL, ch, TO_VICT );
         return;
      }

      act_printf( AT_AUCTION, auc, NULL, ch, TO_VICT, "$n collects his fee of %d, and hands you %d gold.", totalfee,
                  totalnet );
      act( AT_AUCTION, "$n collects his fees and hands $N some gold.", auc, NULL, ch, TO_NOTVICT );

      ch->gold += ( int )totalnet;
      save_char_obj( ch );

      if( IS_ACT_FLAG( auc, ACT_GUILDAUC ) )
      {
         for( clan = first_clan; clan; clan = clan->next )
         {
            if( clan->auction == auc->pIndexData->vnum )
            {
               found = TRUE;
               break;
            }
         }
      }

      if( found && clan->bank )
      {
         clan->balance += ( int )totalfee;
         save_clan( clan );
      }

      return;
   }

   original = ch->in_room;

   char_from_room( ch );
   if( !char_to_room( ch, aucvault ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->first_content );
   char_from_room( ch );
   if( !char_to_room( ch, original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   if( !obj )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being sold.'", argument );
      return;
   }

   if( !str_cmp( obj->seller, ch->name ) && ( !str_cmp( obj->buyer, "" ) || obj->buyer == NULL ) )
   {
      double fee = ( obj->cost * .05 );

      if( obj->rent >= sysdata.minrent )
         fee = ( obj->rent * .20 ); /* 20% handling charge for rare goods being returned */

      act( AT_AUCTION, "$n returns $p to $N.", auc, obj, ch, TO_NOTVICT );
      act( AT_AUCTION, "$n returns $p to you.", auc, obj, ch, TO_VICT );

      obj_from_room( obj );
      obj_to_char( obj, ch );
      save_aucvault( auc, auc->short_descr );

      if( IS_ACT_FLAG( auc, ACT_GUILDAUC ) )
      {
         for( clan = first_clan; clan; clan = clan->next )
         {
            if( clan->auction == auc->pIndexData->vnum )
            {
               found = TRUE;
               break;
            }
         }
      }

      if( found && clan == ch->pcdata->clan )
         ;
      else
      {
         ch->gold -= ( int )fee;
         save_char_obj( ch );
         act_printf( AT_AUCTION, auc, NULL, ch, TO_VICT, "$n charges you a fee of %d for $s services.", fee );
         if( found && clan->bank )
         {
            clan->balance += ( int )fee;
            save_clan( clan );
         }
      }

      return;
   }

   if( str_cmp( obj->buyer, ch->name ) )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'But you didn't win the bidding on %s!'",
                  obj->short_descr );
      return;
   }

   if( ch->gold < obj->bid )
   {
      act( AT_TELL, "$n tells you 'You can't afford the bid, come back when you have the gold.", auc, NULL, ch, TO_VICT );
      return;
   }

   ch->gold -= obj->bid;
   separate_obj( obj );
   obj_from_room( obj );
   obj_to_char( obj, ch );
   REMOVE_OBJ_FLAG( obj, ITEM_AUCTION );
   save_aucvault( auc, auc->short_descr );
   save_char_obj( ch );

   sold = check_sale( auc->short_descr, obj->seller, obj->short_descr );

   if( sold )
   {
      sold->collected = TRUE;
      save_sales(  );
   }

   act( AT_ACTION, "$n collects your money, and hands you $p.", auc, obj, ch, TO_VICT );
   act( AT_ACTION, "$n collects $N's money, and hands $M $p.", auc, obj, ch, TO_NOTVICT );

   STRFREE( obj->seller );
   STRFREE( obj->buyer );
   obj->bid = 0;

   return;
}

void auction_value( CHAR_DATA * ch, CHAR_DATA * auc, char *argument )
{
   ROOM_INDEX_DATA *aucvault, *original;
   OBJ_DATA *obj;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Check bid on what item?\n\r", ch );
      return;
   }

   if( !IS_ROOM_FLAG( ch->in_room, ROOM_AUCTION ) )
   {
      send_to_char( "This is not an auction house!\n\r", ch );
      bug( "auction_value: Auction mob in non-auction room %d!", ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      send_to_char( "This is not an auction house!\n\r", ch );
      bug( "auction_value: Missing auction vault for room %d!", ch->in_room->vnum );
      return;
   }

   original = ch->in_room;

   char_from_room( ch );
   if( !char_to_room( ch, aucvault ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->first_content );
   char_from_room( ch );
   if( !char_to_room( ch, original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   if( !obj || ( obj->buyer != NULL && str_cmp( obj->buyer, "" ) ) )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument );
      return;
   }

   if( !str_cmp( obj->seller, "" ) || obj->seller == NULL )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument );
      bug( "auction_value: Object with no seller - %s", obj->short_descr );
      return;
   }
   ch_printf( ch, "&[auction]%s : Offered by %s. Minimum bid: %d\n\r", obj->short_descr, obj->seller, obj->bid );
   return;
}

void auction_buy( CHAR_DATA * ch, CHAR_DATA * auc, char *argument )
{
   ROOM_INDEX_DATA *aucvault, *original;
   OBJ_DATA *obj;
   char buf[MIL];

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Start bidding on what item?\n\r", ch );
      return;
   }

   if( !IS_ROOM_FLAG( ch->in_room, ROOM_AUCTION ) )
   {
      send_to_char( "This is not an auction house!\n\r", ch );
      bug( "auction_buy: Auction mob in non-auction room %d!", ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      send_to_char( "This is not an auction house!\n\r", ch );
      bug( "auction_buy: Missing auction vault for room %d!", ch->in_room->vnum );
      return;
   }

   if( auction->item )
   {
      act( AT_TELL, "$n tells you 'Wait until the current item has been sold.'", auc, NULL, ch, TO_VICT );
      return;
   }

   original = ch->in_room;

   char_from_room( ch );
   if( !char_to_room( ch, aucvault ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->first_content );
   char_from_room( ch );
   if( !char_to_room( ch, original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   if( !obj )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument );
      return;
   }

   if( !str_cmp( obj->seller, "" ) || obj->seller == NULL )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument );
      bug( "auction_buy: Object with no seller - %s", obj->short_descr );
      return;
   }

   if( obj->buyer != NULL && str_cmp( obj->buyer, "" ) )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'That item has already been sold to %s.'", obj->buyer );
      return;
   }

   if( !str_cmp( obj->seller, ch->name ) )
   {
      act( AT_TELL, "$n tells you 'You can't buy your own item!'", auc, NULL, ch, TO_VICT );
      return;
   }

   if( ch->gold < obj->bid )
   {
      act( AT_TELL, "$n tells you 'You don't have the money to back that bid!'", auc, NULL, ch, TO_VICT );
      return;
   }

   separate_obj( obj );
   obj_from_room( obj );
   obj_to_char( obj, auc );
   /*
    * Could lose the item, but prevents cheaters from duplicating items - Samson 6-23-99 
    */
   save_aucvault( auc, auc->short_descr );

   snprintf( buf, MIL, "%s %d", obj->name, obj->bid );
   bid( auc, ch, buf );
   return;
}

void auction_sell( CHAR_DATA * ch, CHAR_DATA * auc, char *argument )
{
   ROOM_INDEX_DATA *aucvault;
   OBJ_DATA *obj, *sobj;
   char arg1[MIL];
   int minbid;
   short sellcount = 0;

   argument = one_argument( argument, arg1 );

   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Offer what for auction?\n\r", ch );
      return;
   }

   if( !IS_ROOM_FLAG( ch->in_room, ROOM_AUCTION ) )
   {
      send_to_char( "This is not an auction house!\n\r", ch );
      bug( "auction_sell: Auction mob in non-auction room %d!", ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      send_to_char( "This is not an auction house!\n\r", ch );
      bug( "auction_sell: Missing auction vault for room %d!", ch->in_room->vnum );
      return;
   }

   if( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", auc, NULL, ch, TO_VICT );
      return;
   }

   if( !can_drop_obj( ch, obj ) )
   {
      send_to_char( "You can't let go of it, it's cursed!\n\r", ch );
      return;
   }

   if( obj->timer > 0 )
   {
      act( AT_TELL, "$n tells you, '$p is decaying too rapidly...'", auc, obj, ch, TO_VICT );
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_NOAUCTION ) )
   {
      send_to_char( "That item cannot be auctioned!\n\r", ch );
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_DONATION ) )
   {
      send_to_char( "You cannot auction off donated goods!\n\r", ch );
      return;
   }

   if( obj->rent >= sysdata.minrent || obj->rent == -1 )
   {
      send_to_char( "Sorry, rare items cannot be sold here.\n\r", ch );
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_PERSONAL ) )
   {
      send_to_char( "Personal items may not be sold here.\n\r", ch );
      return;
   }

   minbid = atoi( argument );

   if( minbid < 1 )
   {
      send_to_char( "You must specify a bid greater than 0.\n\r", ch );
      return;
   }

   for( sobj = aucvault->first_content; sobj; sobj = sobj->next_content )
   {
      if( sobj && sobj->pIndexData->vnum == obj->pIndexData->vnum )
      {
         act( AT_TELL, "$n tells you '$p is already being offered. Come back later.'", auc, obj, ch, TO_VICT );
         return;
      }
   }

   for( sobj = aucvault->first_content; sobj; sobj = sobj->next_content )
   {
      if( sobj && !str_cmp( sobj->seller, ch->name ) )
         sellcount++;
   }

   if( sellcount > 9 )
   {
      act( AT_TELL, "$n tells you 'You may not have more than 10 items on sale at once.'", auc, NULL, ch, TO_VICT );
      return;
   }

   separate_obj( obj );

   STRFREE( obj->seller );
   obj->seller = STRALLOC( ch->name );
   STRFREE( obj->buyer );
   obj->bid = minbid;
   act( AT_AUCTION, "$n offers $p up for auction.", ch, obj, NULL, TO_ROOM );
   act( AT_AUCTION, "You put $p up for auction.", ch, obj, NULL, TO_CHAR );

   talk_auction( "%s accepts %s at a minimum bid of %d.", auc->short_descr, obj->short_descr, obj->bid );

   obj->day = time_info.day;
   obj->month = time_info.month;
   obj->year = time_info.year + 1;

   obj_from_char( obj );
   obj_to_room( obj, aucvault, ch );
   save_char_obj( ch );
   save_aucvault( auc, auc->short_descr );

   return;
}

void sweep_house( ROOM_INDEX_DATA * aucroom )
{
   CLAN_DATA *clan = NULL;
   CHAR_DATA *aucmob;
   OBJ_DATA *aucobj;
   ROOM_INDEX_DATA *aucvault;
   bool mob = FALSE;
   bool found = FALSE;

   for( aucmob = aucroom->first_person; aucmob; aucmob = aucmob->next_in_room )
   {
      if( IS_NPC( aucmob ) && ( IS_ACT_FLAG( aucmob, ACT_AUCTION ) || IS_ACT_FLAG( aucmob, ACT_GUILDAUC ) ) )
      {
         mob = TRUE;
         break;
      }
   }

   if( !mob )
      return;

   aucvault = get_room_index( aucroom->vnum + 1 );

   if( !aucvault )
   {
      bug( "sweep_house: No vault room preset for auction house %d!", aucroom->vnum );
      return;
   }

   for( aucobj = aucvault->first_content; aucobj; aucobj = aucobj->next_content )
   {
      if( ( aucobj->day == time_info.day && aucobj->month == time_info.month && aucobj->year == time_info.year )
          || ( time_info.year - aucobj->year > 1 ) )
      {
         separate_obj( aucobj );
         obj_from_room( aucobj );
         obj_to_char( aucobj, aucmob );
         save_aucvault( aucmob, aucmob->short_descr );

         if( IS_ACT_FLAG( aucmob, ACT_GUILDAUC ) )
         {
            for( clan = first_clan; clan; clan = clan->next )
            {
               if( clan->auction == aucmob->pIndexData->vnum )
               {
                  found = TRUE;
                  break;
               }
            }
         }

         if( found && clan->storeroom )
         {
            ROOM_INDEX_DATA *clanroom = get_room_index( clan->storeroom );
            obj_from_char( aucobj );
            obj_to_room( aucobj, clanroom, NULL );
            char_from_room( aucmob );
            if( !char_to_room( aucmob, clanroom ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            save_clan_storeroom( aucmob, clan );
            char_from_room( aucmob );
            if( !char_to_room( aucmob, aucroom ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            talk_auction( "%s has turned %s over to %s.", aucmob->short_descr, aucobj->short_descr, clan->name );
         }
         else
         {
            SET_OBJ_FLAG( aucobj, ITEM_DONATION );
            obj_from_char( aucobj );
            obj_to_room( aucobj, get_room_index( ROOM_VNUM_DONATION ), NULL );
            talk_auction( "%s donated %s to charity.", aucmob->short_descr, aucobj->short_descr );
         }
      }
   }
}

/* Sweep old crap from auction houses on daily basis (game time)- Samson 11-1-99 */
void clean_auctions( void )
{
   ROOM_INDEX_DATA *pRoomIndex;
   int iHash;

   for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
   {
      for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      {
         if( pRoomIndex )
         {
            if( IS_ROOM_FLAG( pRoomIndex, ROOM_AUCTION ) )
               sweep_house( pRoomIndex );
         }
      }
   }
   return;
}
