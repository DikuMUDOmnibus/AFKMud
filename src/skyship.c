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
 *  Skyship Module for Overland - based off of Ymris' Dragonflight Module   *
 *        Original Dragonflight Module written by Ymris of Terahoun         *
 ****************************************************************************/

#include "mud.h"
#include "overland.h"
#include "skyship.h"

LANDING_DATA *first_landing;
LANDING_DATA *last_landing;

void send_to_room( const char *txt, CHAR_DATA * ch, ROOM_INDEX_DATA * rm )
{
   CHAR_DATA *rch;

   if( !txt || !rm )
      return;

   for( rch = rm->first_person; rch; rch = rch->next_in_room )
      if( !char_died( rch ) && rch->desc )
      {
         if( ch->x == rch->x && ch->y == rch->y && ch->map == rch->map )
            send_to_char( txt, rch );
      }
}

/*
 * Remove a skyship when it is no longer needed
 */
void purge_skyship( CHAR_DATA * ch, CHAR_DATA * skyship )
{
   send_to_room( "The skyship pilot ascends and takes to the wind.\n\r", ch, ch->in_room );
   skyship->x = 0;
   skyship->y = 0;

   /*
    * Release the player from the skyship 
    */
   REMOVE_PLR_FLAG( ch, PLR_BOARDED );
   ch->has_skyship = FALSE;
   ch->my_skyship = NULL;

   /*
    * After this short timer runs out, it'll be extracted properly 
    */
   skyship->timer = 10;
   return;
}

/*
 * Skyship landing function
 */
void land_skyship( CHAR_DATA * ch, CHAR_DATA * skyship, bool arrived )
{
   if( !IS_NPC( ch ) && arrived )
   {
      ch->x = skyship->dcoordx;
      ch->y = skyship->dcoordy;
      ch->inflight = FALSE;
   }

   skyship->map = skyship->my_rider->map;
   skyship->x = skyship->my_rider->x;
   skyship->y = skyship->my_rider->y;
   skyship->backtracking = FALSE;
   skyship->inflight = FALSE;

   if( IS_ACT_FLAG( skyship, ACT_BOARDED ) )
   {
      interpret( ch, "look" );
      send_to_room( "&CThe skyship descends and lands on the platform.\n\r", skyship, skyship->in_room );
      send_to_room( "Everyone aboard the ship disembarks.\n\r", skyship, skyship->in_room );
      purge_skyship( ch, skyship );
      return;
   }
   else
   {
      send_to_room( "&CA skyship descends from above and lands on the platform.\n\r", skyship, skyship->in_room );
      return;
   }
}

/*
 * Skyship flight function
 */
void fly_skyship( CHAR_DATA * ch, CHAR_DATA * skyship )
{
   CHAR_DATA *pair;
   double dist, angle;
   int speed = 10;   /* Speed of the skyships on the overland */

   /*
    * Reset the boredom counter 
    */
   skyship->zzzzz = 0;

   if( ch && IS_ACT_FLAG( skyship, ACT_BOARDED ) )
      pair = ch;
   else
   {
      pair = skyship;
      speed *= 5;
   }

   /*
    * If skyship is close to the landing site... 
    */
   if( ( ( skyship->y - skyship->dcoordy ) <= speed )
       && ( skyship->y - skyship->dcoordy ) >= -speed
       && ( ( skyship->x - skyship->dcoordx ) <= speed ) && ( skyship->x - skyship->dcoordx ) >= -speed )
   {
      land_skyship( pair, skyship, TRUE );
      return;
   }

   /*
    * up up and away 
    */

   dist = distance( skyship->x, skyship->y, skyship->dcoordx, skyship->dcoordy );
   angle = calc_angle( skyship->x, skyship->y, skyship->dcoordx, skyship->dcoordy, &dist );

   if( angle == -1 )
      skyship->heading = -1;
   else if( angle >= 360 )
      skyship->heading = DIR_NORTH;
   else if( angle >= 315 )
      skyship->heading = DIR_NORTHWEST;
   else if( angle >= 270 )
      skyship->heading = DIR_WEST;
   else if( angle >= 225 )
      skyship->heading = DIR_SOUTHWEST;
   else if( angle >= 180 )
      skyship->heading = DIR_SOUTH;
   else if( angle >= 135 )
      skyship->heading = DIR_SOUTHEAST;
   else if( angle >= 90 )
      skyship->heading = DIR_EAST;
   else if( angle >= 45 )
      skyship->heading = DIR_NORTHEAST;
   else if( angle >= 0 )
      skyship->heading = DIR_NORTH;

   /*
    * move towards dest in steps of "speed" rooms  (salt to taste) 
    */
   switch ( skyship->heading )
   {
      case DIR_NORTH:
         pair->y = pair->y - speed;
         if( pair == ch )
            skyship->y = skyship->y - speed;
         break;

      case DIR_EAST:
         pair->x = pair->x + speed;
         if( pair == ch )
            skyship->x = skyship->x + speed;
         break;

      case DIR_SOUTH:
         pair->y = pair->y + speed;
         if( pair == ch )
            skyship->y = skyship->y + speed;
         break;

      case DIR_WEST:
         pair->x = pair->x - speed;
         if( pair == ch )
            skyship->x = skyship->x - speed;
         break;

      case DIR_NORTHEAST:
         pair->x = pair->x + speed;
         pair->y = pair->y - speed;
         if( pair == ch )
         {
            skyship->x = skyship->x + speed;
            skyship->y = skyship->y - speed;
         }
         break;
      case DIR_NORTHWEST:
         pair->x = pair->x - speed;
         pair->y = pair->y - speed;
         if( pair == ch )
         {
            skyship->x = skyship->x - speed;
            skyship->y = skyship->y - speed;
         }
         break;
      case DIR_SOUTHEAST:
         pair->x = pair->x + speed;
         pair->y = pair->y + speed;
         if( pair == ch )
         {
            skyship->x = skyship->x + speed;
            skyship->y = skyship->y + speed;
         }
         break;
      case DIR_SOUTHWEST:
         pair->x = pair->x - speed;
         pair->y = pair->y + speed;
         if( pair == ch )
         {
            skyship->x = skyship->x - speed;
            skyship->y = skyship->y + speed;
         }
         break;
      default:
         break;
   }

   collect_followers( ch, ch->in_room, ch->in_room );

   /*
    * Reversed the order of these calls because of how the Overland clears the screen - Samson 
    */
   if( IS_ACT_FLAG( skyship, ACT_BOARDED ) )
      interpret( pair, "look" );

   /*
    * If skyship is looking for a better spot....
    */
   if( skyship->backtracking )
      land_skyship( pair, skyship, FALSE );

   return;
}

/*
 * Create a skyship 
 */
void create_skyship( CHAR_DATA * ch )
{
   MOB_INDEX_DATA *vskyship = NULL;
   CHAR_DATA *skyship;

   /*
    * Set this to the vnum assigned to your skyship 
    */
   vskyship = get_mob_index( 11072 );

   if( !vskyship )
   {
      bug( "%s", "create_skyship: Vnum not found for skyship" );
      return;
   }

   skyship = create_mobile( vskyship );

   /*
    * pick a random set of coordinates 
    */
   /*
    * and create skyship at the coords 
    */
   /*
    * skyship can be safely spawned using the same room as the PC calling it - Samson 
    */
   if( !char_to_room( skyship, ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   SET_ACT_FLAG( skyship, ACT_ONMAP );
   skyship->inflight = TRUE;
   skyship->heading = -1;
   skyship->map = ch->map;
   skyship->x = ch->x;
   skyship->y = ch->y;

   /*
    * Bond the player and skyship together 
    */
   ch->my_skyship = skyship;
   skyship->my_rider = ch;

   /*
    * Set the launch coords for backtracking, if needed later 
    */
   skyship->lcoordx = ch->x;
   skyship->lcoordy = ch->y;

   /*
    * fly skyship to player location 
    */
   skyship->dcoordx = ch->x;
   skyship->dcoordy = ch->y;

   fly_skyship( NULL, skyship );

   return;
}

/*
 * Call a skyship 
 */
CMDF do_call( CHAR_DATA * ch, char *argument )
{
   short terrain = get_terrain( ch->map, ch->x, ch->y );

   /*
    * Sanity checks Reasons why a skyship wouldn't want to answer
    */
   /*
    * You a smelly mobbie?? 
    */
   if( IS_NPC( ch ) )
   {
      send_to_char( "Sorry, mobs cannot use skyships to get around.\n\r", ch );
      return;

   }

   /*
    * Simplifies things to keep skyships on the Overland only - Samson 
    */
   if( !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "Skyships may only be called on the Overland.\n\r", ch );
      return;
   }

   /*
    * already has a skyship 
    */
   if( ch->my_skyship )
   {
      send_to_char( "You have already sent for a skyship, be patient.\n\r", ch );
      return;
   }

   if( terrain != SECT_LANDING )
   {
      send_to_char( "A skyship will not land except at a designated landing site.\n\r", ch );
      return;
   }
   send_to_char( "You send for a skyship.\n\r", ch );
   act( AT_PLAIN, "$n sends for a skyship.", ch, NULL, NULL, TO_ROOM );
   create_skyship( ch );
   return;
}

LANDING_DATA *check_landing_site( short map, short x, short y )
{
   LANDING_DATA *landing;

   for( landing = first_landing; landing; landing = landing->next )
   {
      if( landing->map == map )
      {
         if( landing->x == x && landing->y == y )
            return landing;
      }
   }
   return NULL;
}

/*
 * Command to Fly a skyship 
 */
CMDF do_fly( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *skyship = NULL;
   LANDING_DATA *landing, *lsite = NULL;
   int cost = 0;

   if( !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "This command can only be used from the Overland.\n\r", ch );
      return;
   }

   if( !ch->my_skyship )
   {
      send_to_char( "You have not called for a skyship, how do you expect to do that?\n\r", ch );
      return;
   }

   if( !IS_PLR_FLAG( ch, PLR_BOARDED ) )
   {
      send_to_char( "You aren't on a skyship.\n\r", ch );
      return;
   }

   skyship = ch->my_skyship;

   if( skyship->map != ch->map && skyship->x != ch->x && skyship->y != ch->y )
   {
      send_to_char( "The skyship you boarded is not here?!?!?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "You need to specify a destination first.\n\r", ch );
      return;
   }

   lsite = check_landing_site( ch->map, ch->x, ch->y );

   for( landing = first_landing; landing; landing = landing->next )
   {
      if( landing->area && !str_prefix( argument, landing->area ) )
      {
         if( lsite && !str_cmp( landing->area, lsite->area ) )
         {
            ch_printf( ch, "You are already at %s though!\n\r", argument );
            return;
         }

         if( landing->map != ch->map )
         {
            /*
             * Simplifies things. Especially since it would look funny to see alien terrain below you - Samson 
             */
            send_to_char( "The skyship pilot refuses to fly you to another continent.\n\r", ch );
            return;
         }
         cost = landing->cost;

         if( ch->gold < cost )
         {
            ch_printf( ch, "A flight to %s will cost you %d, which you cannot afford right now.\n\r",
                       landing->area, landing->cost );
            return;
         }
         ch->gold -= cost;

         ch_printf( ch, "The skyship pilot takes your gold and charts a course to %s\n\r", landing->area );
         skyship->dcoordx = landing->x;
         skyship->dcoordy = landing->y;
         skyship->backtracking = FALSE;
         ch->inflight = TRUE;
         send_to_room( "The skyship ascends and takes to the wind.\n\r", ch, ch->in_room );
         return;
      }
   }
   ch_printf( ch, "There is no landing site in the vicinity of %s.\n\r", argument );
   skyship->dcoordx = ch->x;
   skyship->dcoordy = ch->y;
   return;
}

CMDF do_board( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *skyship = ch->my_skyship;

   if( !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "This command can only be used from the Overland.\n\r", ch );
      return;
   }

   if( !skyship )
   {
      send_to_char( "You have not called for a skyship, how do you expect to do that?\n\r", ch );
      return;
   }

   if( ch->map != skyship->map && ch->x != skyship->x && ch->y != skyship->y )
   {
      send_to_char( "You cannot board yet, the skyship hasn't arrived.\n\r", ch );
      return;
   }

   if( IS_PLR_FLAG( ch, PLR_BOARDED ) )
   {
      send_to_char( "You have already boarded the skyship.\n\r", ch );
      return;
   }

   SET_ACT_FLAG( skyship, ACT_BOARDED );
   SET_PLR_FLAG( ch, PLR_BOARDED );
   send_to_char( "You climb abord the skyship and settle into your seat.\n\r", ch );
   return;
}

/* 
 * Timing function used by update.c
 */
void skyship_update( void )
{
   CHAR_DATA *ch;

   /*
    * Safer to scan the list from start to end 
    */
   for( ch = first_char; ch; ch = ch->next )
   {
      if( IS_NPC( ch ) && ch->inflight )
         fly_skyship( ch->my_rider, ch );

      if( !IS_NPC( ch ) && ch->inflight )
         fly_skyship( ch, ch->my_skyship );

      /*
       * skyship idling Function
       */
      if( IS_NPC( ch ) && ch->my_rider && !ch->inflight )
      {
         ch->zzzzz++;

         /*
          * First boredom warning   salt to taste 
          */
         if( ch->zzzzz == 100 )
            send_to_room( "The skyship pilot is growing restless. Perhaps you should decide where you want to go?\n\r",
                          ch, ch->in_room );

         /*
          * Second boredom warning   salt to taste 
          */
         if( ch->zzzzz == 200 )
            send_to_room( "The skyship pilot is making preparations to leave, you had better decide soon.\n\r",
                          ch, ch->in_room );

         /*
          * Third strike, yeeeer out!   salt to taste 
          */
         if( ch->zzzzz == 300 )
         {
            send_to_room( "The skyship pilot ascends to the winds without you.\n\r", ch, ch->in_room );
            purge_skyship( ch->my_rider, ch );
         }
      }
   }
}

/*
 * Landing Site Stuff
 *   I take no credit for originality here on down.  This is 
 *   DIRECTLY "ADAPTED" from Samson's landmark code in overland.c
 */
void fread_landing_sites( LANDING_DATA * landing, FILE * fp )
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
            KEY( "Area", landing->area, fread_string( fp ) );
            break;

         case 'C':
            if( !str_cmp( word, "Coordinates" ) )
            {
               landing->map = fread_number( fp );
               landing->x = fread_number( fp );
               landing->y = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            KEY( "Cost", landing->cost, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;
      }
      if( !fMatch )
         bug( "Fread_landing: no match: %s", word );
   }
}

void load_landing_sites( void )
{
   char filename[256];
   LANDING_DATA *landing;
   FILE *fp;

   first_landing = NULL;
   last_landing = NULL;

   snprintf( filename, 256, "%s%s", MAP_DIR, LANDING_SITE_FILE );

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
            bug( "%s", "Load_landing_sites: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "LANDING_SITE" ) )
         {
            CREATE( landing, LANDING_DATA, 1 );
            fread_landing_sites( landing, fp );
            LINK( landing, first_landing, last_landing, next, prev );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_landing_sites: bad section: %s.", word );
            continue;
         }
      }
      FCLOSE( fp );
   }

   return;
}

void save_landing_sites( void )
{
   LANDING_DATA *landing;
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%s%s", MAP_DIR, LANDING_SITE_FILE );

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s", "save_landing_sites: fopen" );
      perror( filename );
   }
   else
   {
      for( landing = first_landing; landing; landing = landing->next )
      {
         fprintf( fp, "%s", "#LANDING_SITE\n" );
         fprintf( fp, "Coordinates	%d %d %d\n", landing->map, landing->x, landing->y );
         if( landing->area && landing->area[0] != '\0' )
            fprintf( fp, "Area	%s~\n", landing->area );
         fprintf( fp, "Cost	%d\n", landing->cost );
         fprintf( fp, "%s", "End\n\n" );
      }
      fprintf( fp, "%s", "#END\n" );
      FCLOSE( fp );
   }
   return;
}

void add_landing( int map, int x, int y )
{
   LANDING_DATA *landing;

   CREATE( landing, LANDING_DATA, 1 );
   LINK( landing, first_landing, last_landing, next, prev );
   landing->map = map;
   landing->x = x;
   landing->y = y;
   landing->cost = 50000;
   save_landing_sites(  );
   return;
}

void delete_landing_site( LANDING_DATA * landing )
{
   if( !landing )
   {
      bug( "%s", "delete_landing_site: Trying to delete NULL landing site." );
      return;
   }

   UNLINK( landing, first_landing, last_landing, next, prev );
   STRFREE( landing->area );
   DISPOSE( landing );

   if( !mud_down )
      save_landing_sites(  );
   return;
}

void free_landings( void )
{
   LANDING_DATA *lands, *lands_next;

   for( lands = first_landing; lands; lands = lands_next )
   {
      lands_next = lands->next;
      delete_landing_site( lands );
   }
   return;
}

/* Support command to list all landing sites currently loaded */
CMDF do_landing_sites( CHAR_DATA * ch, char *argument )
{
   LANDING_DATA *landing;

   if( !first_landing )
   {
      send_to_char( "No landing sites defined.\n\r", ch );
      return;
   }

   send_to_pager( "Continent | Coordinates | Area             | Cost     \n\r", ch );
   send_to_pager( "------------------------------------------------------\n\r", ch );

   for( landing = first_landing; landing; landing = landing->next )
   {
      pager_printf( ch, "%-10s  %-4dX %-4dY   %-15s   %d\n\r",
                    map_names[landing->map], landing->x, landing->y, landing->area, landing->cost );
   }
   return;
}

/* OLC command to add/delete/edit landing site information */
CMDF do_setlanding( CHAR_DATA * ch, char *argument )
{
   LANDING_DATA *landing = NULL;
   char arg[MIL];

   if( IS_NPC( ch ) )
   {
      send_to_char( "Sorry, NPCs have to walk.\n\r", ch );
      return;
   }

   if( !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "This command can only be used from the overland.\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' || !str_cmp( arg, "help" ) )
   {
      send_to_char( "Usage: setlanding add\n\r", ch );
      send_to_char( "Usage: setlanding delete\n\r", ch );
      send_to_char( "Usage: setlanding area <area name>\n\r", ch );
      send_to_char( "Usage: setlanding cost <cost>\n\r", ch );
      return;
   }

   landing = check_landing_site( ch->map, ch->x, ch->y );

   if( !str_cmp( arg, "add" ) )
   {
      if( landing )
      {
         send_to_char( "There's already a landing site at this location.\n\r", ch );
         return;
      }
      add_landing( ch->map, ch->x, ch->y );
      putterr( ch->map, ch->x, ch->y, SECT_LANDING );
      send_to_char( "Landing site added.\n\r", ch );
      return;
   }

   if( !landing )
   {
      send_to_char( "There is no landing site here.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "delete" ) )
   {
      delete_landing_site( landing );
      putterr( ch->map, ch->x, ch->y, SECT_OCEAN );
      send_to_char( "Landing site deleted.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "area" ) )
   {
      smash_tilde( argument );
      STRFREE( landing->area );
      landing->area = STRALLOC( argument );
      save_landing_sites(  );
      ch_printf( ch, "Area set to %s.\n\r", argument );
      return;
   }

   if( !str_cmp( arg, "cost" ) )
   {
      landing->cost = atoi( argument );
      save_landing_sites(  );
      ch_printf( ch, "Landing site cost set to %d\n\r", landing->cost );
      return;
   }
   do_setlanding( ch, "" );
   return;
}
