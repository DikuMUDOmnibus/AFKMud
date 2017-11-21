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
 *                             Player ships                                 *
 *                   Brought over from Lands of Altanos                     *
 ***************************************************************************/

/* This code is still in the development stages and probably has bugs - be warned */

#include "mud.h"
#include "overland.h"
#include "ships.h"

void check_sneaks( CHAR_DATA * ch );

char *const ship_type[] = {
   "None", "Skiff", "Coaster", "Caravel", "Galleon", "Warship"
};

char *const ship_flags[] = {
   "anchored", "onmap", "airship"
};

SHIP_DATA *first_ship;
SHIP_DATA *last_ship;

int get_shiptype( char *type )
{
   int x;

   for( x = 0; x < SHIP_MAX; x++ )
      if( !str_cmp( type, ship_type[x] ) )
         return x;
   return -1;
}

int get_shipflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( ship_flags ) / sizeof( ship_flags[0] ) ); x++ )
      if( !str_cmp( flag, ship_flags[x] ) )
         return x;
   return -1;
}

SHIP_DATA *ship_lookup_by_vnum( int vnum )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( ship->vnum == vnum )
         return ship;
   return NULL;
}

SHIP_DATA *ship_lookup( char *name )
{
   SHIP_DATA *ship;

   for( ship = first_ship; ship; ship = ship->next )
      if( !str_cmp( name, ship->name ) )
         return ship;

   return NULL;
}

CMDF do_shiplist( CHAR_DATA * ch, char *argument )
{
   SHIP_DATA *ship;
   int count = 1;

   send_to_char( "&RNumbr Ship Name          Ship Type        Owner        Hull Status\n\r", ch );
   send_to_char( "&r===== =========          =========        =====        ===========\n\r", ch );
   for( ship = first_ship; ship; ship = ship->next )
   {
      ch_printf( ch, "&Y&W%4d&b&w>&g %-15s&Y    %-15s &G %-15s &Y%d/%d&g\n\r",
                 count, capitalize( ship->name ), ship_type[ship->type], ship->owner, ship->hull, ship->max_hull );
      count += 1;
   }
   return;
}

CMDF do_shipstat( CHAR_DATA * ch, char *argument )
{
   SHIP_DATA *ship;

   if( !( ship = ship_lookup( argument ) ) )
   {
      send_to_char( "Stat which ship?\n\r", ch );
      return;
   }

   ch_printf( ch, "Name:  %s\n\r", ship->name );
   ch_printf( ch, "Owner: %s\n\r", ship->owner );
   ch_printf( ch, "Vnum:  %d\n\r", ship->vnum );
   if( IS_SHIP_FLAG( ship, SHIP_ONMAP ) )
   {
      ch_printf( ch, "On map: %s\n\r", map_names[ship->map] );
      ch_printf( ch, "Coords: %4dX %4dy\n\r", ship->x, ship->y );
   }
   else
      ch_printf( ch, "In room: %d\n\r", ship->room );
   ch_printf( ch, "Hull:  %d/%d\n\r", ship->hull, ship->max_hull );
   ch_printf( ch, "Fuel:  %d/%d\n\r", ship->fuel, ship->max_fuel );
   ch_printf( ch, "Type:  %d (%s)\n\r", ship->type, ship_type[ship->type] );
   ch_printf( ch, "Flags: %s\n\r", ext_flag_string( &ship->flags, ship_flags ) );
   return;
}

void save_ships( void )
{
   FILE *fp;
   SHIP_DATA *ship;

   if( !( fp = fopen( SHIP_FILE, "w" ) ) )
   {
      perror( SHIP_FILE );
      bug( "%s", "save_ships: can't open ship file" );
      return;
   }
   for( ship = first_ship; ship; ship = ship->next )
   {
      fprintf( fp, "%s", "#SHIP\n" );
      fprintf( fp, "Name      %s~\n", ship->name );
      fprintf( fp, "Owner     %s~\n", ship->owner );
      fprintf( fp, "Flags     %s~\n", ext_flag_string( &ship->flags, ship_flags ) );
      fprintf( fp, "Vnum      %d\n", ship->vnum );
      fprintf( fp, "Room      %d\n", ship->room );
      fprintf( fp, "Type      %s~\n", ship_type[ship->type] );
      fprintf( fp, "Hull      %d\n", ship->hull );
      fprintf( fp, "Max_hull  %d\n", ship->max_hull );
      fprintf( fp, "Fuel      %d\n", ship->fuel );
      fprintf( fp, "Max_fuel  %d\n", ship->max_fuel );
      fprintf( fp, "Coordinates %d %d %d\n", ship->map, ship->x, ship->y );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
   return;
}

void fread_ship( SHIP_DATA * ship, FILE * fp )
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

         case 'C':
            if( !str_cmp( word, "Coordinates" ) )
            {
               ship->map = fread_number( fp );
               ship->x = fread_number( fp );
               ship->y = fread_number( fp );
               fMatch = TRUE;
               break;
            }
         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               char *shipflags = NULL;
               char flag[MIL];
               int value;

               shipflags = fread_flagstring( fp );

               while( shipflags[0] != '\0' )
               {
                  shipflags = one_argument( shipflags, flag );
                  value = get_shipflag( flag );
                  if( value < 0 || value > MAX_BITS )
                     bug( "Unknown shipflag: %s\n\r", flag );
                  else
                     SET_SHIP_FLAG( ship, value );
               }
               fMatch = TRUE;
               break;
            }
            KEY( "Fuel", ship->fuel, fread_number( fp ) );
            break;

         case 'H':
            KEY( "Hull", ship->hull, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Max_fuel", ship->max_fuel, fread_number( fp ) );
            KEY( "Max_hull", ship->max_hull, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", ship->name, fread_string_nohash( fp ) );
            break;

         case 'O':
            KEY( "Owner", ship->owner, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Room", ship->room, fread_number( fp ) );
            break;

         case 'T':
            if( !str_cmp( word, "Type" ) )
            {
               int type = -1;

               type = get_shiptype( fread_flagstring( fp ) );

               if( type < 0 || type >= SHIP_MAX )
               {
                  bug( "load_ships: %s has invalid ship type - setting to none", ship->name );
                  type = 0;
               }
               ship->type = type;
               fMatch = TRUE;
               break;
            }
            break;

         case 'V':
            KEY( "Vnum", ship->vnum, fread_number( fp ) );
            break;
      }
      if( !fMatch )
      {
         bug( "Fread_ship: no match: %s", word );
         fread_to_eol( fp );
      }
   }
}

void load_ships( void )
{
   FILE *fp;
   SHIP_DATA *ship;
   bool found = FALSE;

   first_ship = NULL;
   last_ship = NULL;

   if( ( fp = fopen( SHIP_FILE, "r" ) ) != NULL )
   {

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
            bug( "%s", "ship_load: # Not found" );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "SHIP" ) )
         {
            CREATE( ship, SHIP_DATA, 1 );
            fread_ship( ship, fp );
            LINK( ship, first_ship, last_ship, next, prev );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "load_ships: bad section - %s", word );
            break;
         }
      }
      FCLOSE( fp );
   }
}

void delete_ship( SHIP_DATA * ship )
{
   UNLINK( ship, first_ship, last_ship, next, prev );
   DISPOSE( ship->name );
   STRFREE( ship->owner );
   DISPOSE( ship );
   return;
}

void free_ships( void )
{
   SHIP_DATA *ship, *ship_next;

   for( ship = first_ship; ship; ship = ship_next )
   {
      ship_next = ship->next;
      delete_ship( ship );
   }
   return;
}

CMDF do_shipset( CHAR_DATA * ch, char *argument )
{
   SHIP_DATA *ship;
   char arg[MIL];
   char mod[MIL];

   smash_tilde( argument );
   argument = one_argument( argument, arg );
   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Syntax: shipset <ship> create\n\r", ch );
      send_to_char( "        shipset <ship> delete\n\r", ch );
      send_to_char( "        shipset <ship> <field> <value>\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "  Where <field> is one of:\n\r", ch );
      send_to_char( "    name owner vnum type fuel max_fuel\n\r", ch );
      send_to_char( "    hull max_hull coords room flags\n\r", ch );
      return;
   }

   argument = one_argument( argument, mod );
   ship = ship_lookup( arg );

   if( !str_cmp( mod, "create" ) )
   {
      if( ship )
      {
         send_to_char( "Ship already exists.\n\r", ch );
         return;
      }
      CREATE( ship, SHIP_DATA, 1 );
      ship->map = -1;
      ship->x = -1;
      ship->y = -1;
      ship->name = str_dup( arg );
      LINK( ship, first_ship, last_ship, next, prev );
      save_ships(  );
      send_to_char( "Ship created.\n\r", ch );
      return;
   }

   if( !ship )
   {
      send_to_char( "Ship doesn't exist.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "delete" ) )
   {
      delete_ship( ship );
      save_ships(  );
      send_to_char( "Ship deleted.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "name" ) )
   {
      if( ship_lookup( argument ) )
      {
         send_to_char( "Another ship has that name.\n\r", ch );
         return;
      }
      DISPOSE( ship->name );
      ship->name = str_dup( argument );
      save_ships(  );
      send_to_char( "Name changed.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "owner" ) )
   {
      STRFREE( ship->owner );
      ship->owner = STRALLOC( argument );
      save_ships(  );
      send_to_char( "Ownership shifted.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "vnum" ) )
   {
      ship->vnum = atoi( argument );
      save_ships(  );
      send_to_char( "Vnum modified.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "type" ) )
   {
      ship->type = atoi( argument );
      save_ships(  );
      send_to_char( "Type modified.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "hull" ) )
   {
      ship->hull = atoi( argument );
      save_ships(  );
      send_to_char( "Hull changed.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "max_hull" ) )
   {
      ship->max_hull = atoi( argument );
      save_ships(  );
      send_to_char( "Maximum hull changed.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "fuel" ) )
   {
      ship->fuel = atoi( argument );
      save_ships(  );
      send_to_char( "Fuel changed.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "max_fuel" ) )
   {
      ship->max_fuel = atoi( argument );
      save_ships(  );
      send_to_char( "Maximum fuel changed.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "room" ) )
   {
      ship->room = atoi( argument );
      save_ships(  );
      send_to_char( "Room set.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "coords" ) )
   {
      char arg3[MIL];

      argument = one_argument( argument, arg3 );

      if( arg3[0] == '\0' || argument[0] == '\0' )
      {
         send_to_char( "You must specify both coordinates.\n\r", ch );
         return;
      }

      ship->x = atoi( arg3 );
      ship->y = atoi( argument );
      save_ships(  );
      send_to_char( "Coordinates changed.\n\r", ch );
      return;
   }

   if( !str_cmp( mod, "flags" ) )
   {
      char arg3[MIL];
      int value;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_shipflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( ship->flags, value );
      }
      save_ships(  );
      return;
   }
   do_shipset( ch, "" );
   return;
}

/* Cheap hack of process_exit from overland.c for ships */
ch_ret process_shipexit( CHAR_DATA * ch, short map, short x, short y, int dir )
{
   int sector = get_terrain( map, x, y );
   int move;
   char *txt;
   CHAR_DATA *fch;
   CHAR_DATA *nextinroom;
   ROOM_INDEX_DATA *from_room;
   SHIP_DATA *ship = ch->on_ship;
   ch_ret retcode;
   int chars = 0, count = 0;
   short fx, fy, fmap;

   from_room = ch->in_room;
   fx = ch->x;
   fy = ch->y;
   fmap = ch->map;

   retcode = rNONE;

   if( sector == SECT_EXIT )
   {
      ENTRANCE_DATA *enter;
      ROOM_INDEX_DATA *toroom = NULL;

      enter = check_entrance( map, x, y );

      if( enter != NULL && !IS_PLR_FLAG( ch, PLR_MAPEDIT ) )
      {
         if( enter->tomap != -1 )   /* Means exit goes to another map */
         {
            enter_map( ch, NULL, enter->therex, enter->therey, enter->tomap );
            if( ch->mount )
               enter_map( ch->mount, NULL, enter->therex, enter->therey, enter->tomap );

            for( fch = from_room->first_person; fch; fch = fch->next_in_room )
               chars++;

            for( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom )
            {
               nextinroom = fch->next_in_room;
               count++;
               if( fch != ch  /* loop room bug fix here by Thoric */
                   && fch->master == ch
                   && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED )
                   && fch->x == fx && fch->y == fy && fch->map == fmap )
               {
                  if( !IS_NPC( fch ) )
                  {
                     act( AT_ACTION, "The ship sails $T.", fch, NULL, dir_name[dir], TO_CHAR );
                     process_exit( fch, fch->map, x, y, dir, FALSE );
                  }
                  else
                     enter_map( fch, NULL, enter->therex, enter->therey, enter->tomap );
               }
            }
            return rSTOP;
         }

         if( ( toroom = get_room_index( enter->vnum ) ) == NULL )
         {
            bug( "Target vnum %d for map exit does not exist!", enter->vnum );
            send_to_char( "Ooops. Something bad happened. Contact the immortals ASAP.\n\r", ch );
            return rSTOP;
         }

         if( toroom->sector_type != SECT_WATER_NOSWIM
             && toroom->sector_type != SECT_RIVER && toroom->sector_type != SECT_OCEAN )
         {
            if( toroom->sector_type == SECT_WATER_SWIM )
            {
               send_to_char( "The waters are too shallow to sail into.\n\r", ch );
               return rSTOP;
            }

            send_to_char( "That's land! You'll run your ship aground!\n\r", ch );
            return rSTOP;
         }

         if( toroom->sector_type != SECT_OCEAN && ch->on_ship->type > SHIP_CARAVEL )
         {
            ch_printf( ch, "Your %s is too large to sail into anything but oceans.\n\r", ship_type[ch->on_ship->type] );
            return rSTOP;
         }

         if( !str_cmp( ch->name, ship->owner ) )
            act_printf( AT_ACTION, ch, NULL, dir_name[dir], TO_ROOM, "%s sails off to the $T.", ship->name );

         ch->on_ship->room = toroom->vnum;

         leave_map( ch, NULL, toroom );

         for( fch = from_room->first_person; fch; fch = fch->next_in_room )
            chars++;

         for( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom )
         {
            nextinroom = fch->next_in_room;
            count++;
            if( fch != ch  /* loop room bug fix here by Thoric */
                && fch->master == ch && fch->position == POS_STANDING && fch->x == fx && fch->y == fy && fch->map == fmap )
            {
               if( !IS_NPC( fch ) )
               {
                  act( AT_ACTION, "The ship sails $T.", fch, NULL, dir_name[dir], TO_CHAR );
                  process_shipexit( fch, fch->map, x, y, dir );
               }
               else
                  leave_map( fch, ch, toroom );
            }
         }
         return rSTOP;
      }

      if( enter != NULL && IS_PLR_FLAG( ch, PLR_MAPEDIT ) )
      {
         delete_entrance( enter );
         putterr( ch->map, x, y, ch->pcdata->secedit );
         send_to_char( "&RMap exit deleted.\n\r", ch );
      }

   }

   switch ( dir )
   {
      case DIR_NORTH:
         if( y == -1 )
         {
            send_to_char( "You cannot go any further north!\n\r", ch );
            return rSTOP;
         }
         break;

      case DIR_EAST:
         if( x == MAX_X )
         {
            send_to_char( "You cannot go any further east!\n\r", ch );
            return rSTOP;
         }
         break;

      case DIR_SOUTH:
         if( y == MAX_Y )
         {
            send_to_char( "You cannot go any further south!\n\r", ch );
            return rSTOP;
         }
         break;

      case DIR_WEST:
         if( x == -1 )
         {
            send_to_char( "You cannot go any further west!\n\r", ch );
            return rSTOP;
         }
         break;

      case DIR_NORTHEAST:
         if( x == MAX_X || y == -1 )
         {
            send_to_char( "You cannot go any further northeast!\n\r", ch );
            return rSTOP;
         }
         break;

      case DIR_NORTHWEST:
         if( x == -1 || y == -1 )
         {
            send_to_char( "You cannot go any further northwest!\n\r", ch );
            return rSTOP;
         }
         break;

      case DIR_SOUTHEAST:
         if( x == MAX_X || y == MAX_Y )
         {
            send_to_char( "You cannot go any further southeast!\n\r", ch );
            return rSTOP;
         }
         break;

      case DIR_SOUTHWEST:
         if( x == -1 || y == MAX_Y )
         {
            send_to_char( "You cannot go any further southwest!\n\r", ch );
            return rSTOP;
         }
         break;
   }

   if( sector != SECT_WATER_NOSWIM && sector != SECT_OCEAN && sector != SECT_RIVER )
   {
      if( sector == SECT_WATER_SWIM )
      {
         send_to_char( "The waters are too shallow to sail into.\n\r", ch );
         return rSTOP;
      }
      send_to_char( "That's land! You'll run your ship aground!\n\r", ch );
      return rSTOP;
   }

   if( sector != SECT_OCEAN && ch->on_ship->type > SHIP_CARAVEL )
   {
      ch_printf( ch, "Your %s is too large to sail into anything but oceans.\n\r", ship_type[ch->on_ship->type] );
      return rSTOP;
   }

   move = sect_show[sector].move;

   if( ship->fuel < move && !IS_IMMORTAL( ch ) )
   {
      send_to_char( "Your ship is too low on magical energy to sail further ahead.\n\r", ch );
      return rSTOP;
   }

   if( !IS_IMMORTAL( ch ) && !str_cmp( ch->name, ship->owner ) )
      ship->fuel -= move;

   if( !str_cmp( ch->name, ship->owner ) )
      act_printf( AT_ACTION, ch, NULL, dir_name[dir], TO_ROOM, "%s sails off to the $T.", ship->name );

   ch->x = x;
   ch->y = y;
   ship->x = x;
   ship->y = y;

   if( !str_cmp( ch->name, ship->owner ) )
   {
      txt = rev_exit( dir );
      act_printf( AT_ACTION, ch, NULL, NULL, TO_ROOM, "%s sails in from the %s.", ship->name, txt );
   }

   for( fch = from_room->first_person; fch; fch = fch->next_in_room )
      chars++;

   for( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom )
   {
      nextinroom = fch->next_in_room;
      count++;
      if( fch != ch  /* loop room bug fix here by Thoric */
          && fch->master == ch
          && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) && fch->x == fx && fch->y == fy )
      {
         if( !IS_NPC( fch ) )
         {
            act( AT_ACTION, "The ship sails $T.", fch, NULL, dir_name[dir], TO_CHAR );
            process_exit( fch, fch->map, x, y, dir, FALSE );
         }
         else
         {
            fch->x = x;
            fch->y = y;
         }
      }
   }

   interpret( ch, "look" );
   return retcode;
}

/* Hacked code from the move_char function -Shatai */
/* Rehacked by Samson - had to clean up the mess */
ch_ret move_ship( CHAR_DATA * ch, EXIT_DATA * pexit, int direction )
{
   ROOM_INDEX_DATA *in_room;
   ROOM_INDEX_DATA *to_room;
   ROOM_INDEX_DATA *from_room;
   SHIP_DATA *other;
   SHIP_DATA *ship = ch->on_ship;
   ch_ret retcode;
   char *txt;
   short door;
   int newx, newy, move;

   retcode = rNONE;
   txt = NULL;

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
   {
      newx = ch->x;
      newy = ch->y;

      switch ( direction )
      {
         default:
            break;
         case DIR_NORTH:
            newy = ch->y - 1;
            break;
         case DIR_EAST:
            newx = ch->x + 1;
            break;
         case DIR_SOUTH:
            newy = ch->y + 1;
            break;
         case DIR_WEST:
            newx = ch->x - 1;
            break;
         case DIR_NORTHEAST:
            newx = ch->x + 1;
            newy = ch->y - 1;
            break;
         case DIR_NORTHWEST:
            newx = ch->x - 1;
            newy = ch->y - 1;
            break;
         case DIR_SOUTHEAST:
            newx = ch->x + 1;
            newy = ch->y + 1;
            break;
         case DIR_SOUTHWEST:
            newx = ch->x - 1;
            newy = ch->y + 1;
            break;
      }
      if( newx == ch->x && newy == ch->y )
         return rSTOP;

      retcode = process_shipexit( ch, ch->map, newx, newy, direction );
      return retcode;
   }

   in_room = ch->in_room;
   from_room = in_room;
   if( !pexit || ( to_room = pexit->to_room ) == NULL )
   {
      send_to_char( "Alas, you cannot go that way.\n\r", ch );
      check_sneaks( ch );
      return rSTOP;
   }

   door = pexit->vdir;

   /*
    * Overland Map stuff - Samson 7-31-99 
    */
   /*
    * Upgraded 4-28-00 to allow mounts and charmies to follow PC - Samson 
    */
   if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
   {
      CHAR_DATA *fch;
      CHAR_DATA *nextinroom;
      int chars = 0, count = 0;

      if( pexit->x < 0 || pexit->x >= MAX_X || pexit->y < 0 || pexit->y >= MAX_Y )
      {
         bug( "move_ship: Room #%d - Invalid exit coordinates: %d %d", in_room->vnum, pexit->x, pexit->y );
         send_to_char( "Oops. Something is wrong with this map exit - notify the immortals.\n\r", ch );
         check_sneaks( ch );
         return rSTOP;
      }

      if( !IS_NPC( ch ) )
      {
         enter_map( ch, pexit, pexit->x, pexit->y, -1 );
         if( ch->mount )
            enter_map( ch->mount, pexit, pexit->x, pexit->y, -1 );

         for( fch = from_room->first_person; fch; fch = fch->next_in_room )
            chars++;

         for( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom )
         {
            nextinroom = fch->next_in_room;
            count++;
            if( fch != ch  /* loop room bug fix here by Thoric */
                && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
            {
               if( !IS_NPC( fch ) )
               {
                  if( !get_exit( from_room, direction ) )
                  {
                     act( AT_ACTION, "The entrance closes behind $N, preventing you from following!", fch, NULL, ch, TO_CHAR );
                     continue;
                  }
                  act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
                  move_char( fch, pexit, 0, direction, FALSE );
               }
               else
                  enter_map( fch, pexit, pexit->x, pexit->y, -1 );
            }
         }
      }
      else
      {
         if( !IS_EXIT_FLAG( pexit, EX_NOMOB ) )
         {
            enter_map( ch, pexit, pexit->x, pexit->y, -1 );
            if( ch->mount )
               enter_map( ch->mount, pexit, pexit->x, pexit->y, -1 );
            for( fch = from_room->first_person; fch; fch = fch->next_in_room )
               chars++;

            for( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom )
            {
               nextinroom = fch->next_in_room;
               count++;
               if( fch != ch  /* loop room bug fix here by Thoric */
                   && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
               {
                  if( !IS_NPC( fch ) )
                  {
                     if( !get_exit( from_room, direction ) )
                     {
                        act( AT_ACTION, "The entrance closes behind $N, preventing you from following!", fch, NULL, ch, TO_CHAR );
                        continue;
                     }
                     act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
                     move_char( fch, pexit, 0, direction, FALSE );
                  }
                  else
                     enter_map( fch, pexit, pexit->x, pexit->y, -1 );
               }
            }
         }
      }
      check_sneaks( ch );
      return rSTOP;
   }

   /*
    * Water shallow enough to swim in, and land ( duh! ) cannot be sailed into 
    */
   if( pexit->to_room->sector_type != SECT_WATER_NOSWIM
       && pexit->to_room->sector_type != SECT_OCEAN && pexit->to_room->sector_type != SECT_RIVER )
   {
      if( pexit->to_room->sector_type == SECT_WATER_SWIM )
      {
         send_to_char( "The waters are too shallow to sail into.\n\r", ch );
         check_sneaks( ch );
         return rSTOP;
      }

      send_to_char( "That's land! You'll run your ship aground!\n\r", ch );
      check_sneaks( ch );
      return rSTOP;
   }

   /*
    * Bigger than a caravel, you gotta stay in ocean sectors 
    */
   if( pexit->to_room->sector_type != SECT_OCEAN && ch->on_ship->type > SHIP_CARAVEL )
   {
      ch_printf( ch, "Your %s is too large to sail into anything but oceans.\n\r", ship_type[ch->on_ship->type] );
      check_sneaks( ch );
      return rSTOP;
   }

   if( IS_EXIT_FLAG( pexit, EX_PORTAL ) )
   {
      send_to_char( "You cannot sail a ship through that!!\n\r", ch );
      check_sneaks( ch );
      return rSTOP;
   }

   if( !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && ch->in_room->area != to_room->area )
   {
      if( ch->level < to_room->area->low_hard_range )
      {
         switch ( to_room->area->low_hard_range - ch->level )
         {
            case 1:
               send_to_char( "&[tell]A voice in your mind says, 'You are nearly ready to go that way...'", ch );
               break;
            case 2:
               send_to_char( "&[tell]A voice in your mind says, 'Soon you shall be ready to travel down this path... soon.'",
                             ch );
               break;
            case 3:
               send_to_char( "&[tell]A voice in your mind says, 'You are not ready to go down that path... yet.'.\n\r", ch );
               break;
            default:
               send_to_char( "&[tell]A voice in your mind says, 'You are not ready to go down that path.'.\n\r", ch );
         }
         check_sneaks( ch );
         return rSTOP;
      }
      else if( ch->level > to_room->area->hi_hard_range )
      {
         send_to_char( "&[tell]A voice in your mind says, 'There is nothing more for you down that path.'", ch );
         check_sneaks( ch );
         return rSTOP;
      }
   }

   /*
    * Tunnels in water sectors only check for ships, not people since they're generally too small to matter 
    */
   if( to_room->tunnel > 0 )
   {
      int count = 0;

      for( other = first_ship; other; other = other->next )
      {
         if( other->room == to_room->vnum )
            ++count;

         if( count >= to_room->tunnel )
         {
            send_to_char( "There are too many ships ahead to pass.\n\r", ch );
            check_sneaks( ch );
            return rSTOP;
         }
      }
   }

   move = sect_show[in_room->sector_type].move;

   if( ship->fuel < move && !IS_IMMORTAL( ch ) )
   {
      send_to_char( "Your ship is too low on magical energy to sail further ahead.\n\r", ch );
      return rSTOP;
   }

   if( !str_cmp( ch->name, ship->owner ) && !IS_IMMORTAL( ch ) )
      ship->fuel -= move;

   rprog_leave_trigger( ch );
   if( char_died( ch ) )
      return global_retcode;

   if( !str_cmp( ch->name, ship->owner ) )
      act_printf( AT_ACTION, ch, NULL, dir_name[door], TO_ROOM, "%s sails off to the $T.", ship->name );

   char_from_room( ch );
   if( ch->mount )
   {
      rprog_leave_trigger( ch->mount );
      if( char_died( ch ) )
         return global_retcode;
      if( ch->mount )
      {
         char_from_room( ch->mount );
         if( !char_to_room( ch->mount, to_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      }
   }
   if( !char_to_room( ch, to_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   ship->room = to_room->vnum;
   check_sneaks( ch );

   if( !str_cmp( ch->name, ship->owner ) )
   {
      txt = rev_exit( door );
      act_printf( AT_ACTION, ch, NULL, NULL, TO_ROOM, "%s sails in from the %s.", ship->name, txt );
   }

   {
      CHAR_DATA *fch;
      CHAR_DATA *nextinroom;
      int chars = 0, count = 0;

      for( fch = from_room->first_person; fch; fch = fch->next_in_room )
         chars++;

      for( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom )
      {
         nextinroom = fch->next_in_room;
         count++;
         if( fch != ch  /* loop room bug fix here by Thoric */
             && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
         {
            act( AT_ACTION, "The ship sails $T.", fch, NULL, dir_name[door], TO_CHAR );
            move_char( fch, pexit, 0, direction, FALSE );
         }
      }
   }

   interpret( ch, "look" );
   return retcode;
}
