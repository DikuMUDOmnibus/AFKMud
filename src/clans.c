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
 *                           Special clan module                            *
 ****************************************************************************/

#include "mud.h"
#include "auction.h"
#include "clans.h"
#include "shops.h"

#define MAX_NEST	100
static OBJ_DATA *rgObjNest[MAX_NEST];

CLAN_DATA *first_clan;
CLAN_DATA *last_clan;
COUNCIL_DATA *first_council;
COUNCIL_DATA *last_council;
MEMBER_LIST *first_member_list;
MEMBER_LIST *last_member_list;

void fwrite_obj( CHAR_DATA * ch, OBJ_DATA * obj, CLAN_DATA * clan, FILE * fp, int iNest, short os_type, bool hotboot );
void fread_obj( CHAR_DATA * ch, FILE * fp, short os_type );
void save_shop( CHAR_DATA * mob );
bool exists_player( char *name );
char *wear_bit_name( int wear_flags );

/*
 * Get pointer to clan structure from clan name.
 */
CLAN_DATA *get_clan( char *name )
{
   CLAN_DATA *clan;

   for( clan = first_clan; clan; clan = clan->next )
      if( !str_cmp( name, clan->name ) )
         return clan;
   return NULL;
}

COUNCIL_DATA *get_council( char *name )
{
   COUNCIL_DATA *council;

   for( council = first_council; council; council = council->next )
      if( !str_cmp( name, council->name ) )
         return council;
   return NULL;
}

/*
 * Save items in a clan storage room			-Scryn & Thoric
 */
void save_clan_storeroom( CHAR_DATA * ch, CLAN_DATA * clan )
{
   FILE *fp;
   char filename[256];
   short templvl;
   OBJ_DATA *contents;

   if( !clan )
   {
      bug( "%s", "save_clan_storeroom: Null clan pointer!" );
      return;
   }

   if( !ch )
   {
      bug( "%s", "save_clan_storeroom: Null ch pointer!" );
      return;
   }

   snprintf( filename, 256, "%s%s.vault", CLAN_DIR, clan->filename );
   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "%s", "save_clan_storeroom: fopen" );
      perror( filename );
   }
   else
   {
      templvl = ch->level;
      ch->level = LEVEL_AVATAR;  /* make sure EQ doesn't get lost */
      contents = ch->in_room->last_content;
      if( contents )
         fwrite_obj( ch, contents, clan, fp, 0, OS_CARRY, FALSE );
      fprintf( fp, "%s", "#END\n" );
      ch->level = templvl;
      FCLOSE( fp );
      return;
   }
   return;
}

void check_clan_storeroom( CHAR_DATA * ch )
{
   CLAN_DATA *clan;

   for( clan = first_clan; clan; clan = clan->next )
      if( clan->storeroom == ch->in_room->vnum )
         save_clan_storeroom( ch, clan );

   return;
}

void check_clan_shop( CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj )
{
   CLAN_DATA *clan = NULL;
   bool cfound = FALSE;

   if( IS_NPC( ch ) )
      return;

   for( clan = first_clan; clan; clan = clan->next )
   {
      if( victim->pIndexData->vnum == clan->shopkeeper )
      {
         cfound = TRUE;
         break;
      }
   }

   if( ( cfound && clan != ch->pcdata->clan ) || IS_OBJ_FLAG( obj, ITEM_PERSONAL ) || obj->rent >= sysdata.minrent )
   {
      ch_printf( ch, "%s says 'I cannot accept this from you.'\n\r", victim->short_descr );
      ch_printf( ch, "%s hands %s back to you.\n\r", victim->short_descr, obj->short_descr );
      obj_from_char( obj );
      obj = obj_to_char( obj, ch );
      save_char_obj( ch );
      return;
   }

   ch_printf( ch, "%s puts %s into inventory.\n\r", victim->short_descr, obj->short_descr );
   save_shop( victim );
   return;
}

void free_one_clan( CLAN_DATA * clan )
{
   MEMBER_LIST *members_list, *list_next;
   MEMBER_DATA *member;

   for( members_list = first_member_list; members_list; members_list = list_next )
   {
      list_next = members_list->next;
      if( !str_cmp( clan->name, members_list->name ) )
      {
         while( members_list->first_member )
         {
            member = members_list->first_member;
            UNLINK( member, members_list->first_member, members_list->last_member, next, prev );
            STRFREE( member->name );
            STRFREE( member->since );
            DISPOSE( member );
         }
         UNLINK( members_list, first_member_list, last_member_list, next, prev );
         STRFREE( members_list->name );
         DISPOSE( members_list );
      }
   }

   UNLINK( clan, first_clan, last_clan, next, prev );
   DISPOSE( clan->filename );
   STRFREE( clan->name );
   DISPOSE( clan->motto );
   DISPOSE( clan->clandesc );
   STRFREE( clan->deity );
   STRFREE( clan->leader );
   STRFREE( clan->number1 );
   STRFREE( clan->number2 );
   STRFREE( clan->leadrank );
   STRFREE( clan->onerank );
   STRFREE( clan->tworank );
   STRFREE( clan->badge );
   DISPOSE( clan );

   return;
}

void free_clans( void )
{
   CLAN_DATA *clan, *clan_next;

   for( clan = first_clan; clan; clan = clan_next )
   {
      clan_next = clan->next;
      free_one_clan( clan );
   }
   return;
}

void save_member_list( MEMBER_LIST * members_list )
{
   MEMBER_DATA *member;
   FILE *fp;
   CLAN_DATA *clan;
   char buf[256];

   if( !members_list )
   {
      bug( "%s", "save_member_list: NULL members_list" );
      return;
   }

   if( ( clan = get_clan( members_list->name ) ) == NULL )
   {
      bug( "save_member_list: no such clan: %s", members_list->name );
      return;
   }

   snprintf( buf, 256, "%s%s.mem", CLAN_DIR, clan->filename );

   if( ( fp = fopen( buf, "w" ) ) == NULL )
   {
      bug( "%s", "Cannot open clan.mem file for writing" );
      perror( buf );
      return;
   }

   fprintf( fp, "Name          %s~\n", members_list->name );
   for( member = members_list->first_member; member; member = member->next )
      fprintf( fp, "Member        %s %s %d %d %d %d\n", member->name, member->since,
               member->kills, member->deaths, member->level, member->Class );
   fprintf( fp, "%s", "End\n\n" );
   FCLOSE( fp );
}

void show_members( CHAR_DATA * ch, char *argument, char *format )
{
   MEMBER_LIST *members_list;
   MEMBER_DATA *member;
   CLAN_DATA *clan;

   for( members_list = first_member_list; members_list; members_list = members_list->next )
   {
      if( !str_cmp( members_list->name, argument ) )
         break;
   }

   if( !members_list )
      return;

   clan = get_clan( argument );

   if( !clan )
      return;

   pager_printf( ch, "\n\rMembers of %s\n\r", clan->name );
   send_to_pager( "------------------------------------------------------------\n\r", ch );
   pager_printf( ch, "Leader: %s\n\r", clan->leader );
   pager_printf( ch, "Number1: %s\n\r", clan->number1 );
   pager_printf( ch, "Number2: %s\n\r", clan->number2 );
   pager_printf( ch, "Deity: %s\n\r", clan->deity );
   send_to_pager( "------------------------------------------------------------\n\r", ch );
   send_to_pager( "Lvl  Name                 Class  Kills   Deaths Since\n\r\n\r", ch );

   if( format && format[0] != '\0' )
   {
      if( !str_cmp( format, "kills" ) || !str_cmp( format, "deaths" ) || !str_cmp( format, "alpha" ) )
      {
         MS_DATA *insert = NULL;
         MS_DATA *sort;
         MS_DATA *first_member = NULL;
         MS_DATA *last_member = NULL;

         CREATE( sort, MS_DATA, 1 );
         sort->member = members_list->first_member;
         LINK( sort, first_member, last_member, next, prev );

         for( member = members_list->first_member->next; member; member = member->next )
         {
            insert = NULL;
            for( sort = first_member; sort; sort = sort->next )
            {
               if( !str_cmp( format, "kills" ) )
               {
                  if( member->kills > sort->member->kills )
                  {
                     CREATE( insert, MS_DATA, 1 );
                     insert->member = member;
                     INSERT( insert, sort, first_member, next, prev );
                     break;
                  }
               }
               else if( !str_cmp( format, "deaths" ) )
               {
                  if( member->deaths > sort->member->deaths )
                  {
                     CREATE( insert, MS_DATA, 1 );
                     insert->member = member;
                     INSERT( insert, sort, first_member, next, prev );
                     break;
                  }
               }
               else if( !str_cmp( format, "alpha" ) )
               {
                  if( str_cmp( member->name, sort->member->name ) )
                  {
                     CREATE( insert, MS_DATA, 1 );
                     insert->member = member;
                     INSERT( insert, sort, first_member, next, prev );
                     break;
                  }
               }
            }
            if( insert == NULL )
            {
               CREATE( insert, MS_DATA, 1 );
               insert->member = member;
               LINK( insert, first_member, last_member, next, prev );
            }
         }

         for( sort = first_member; sort; sort = sort->next )
            if( str_cmp( sort->member->name, clan->leader )
                && str_cmp( sort->member->name, clan->number1 ) && str_cmp( sort->member->name, clan->number2 ) )
               pager_printf( ch, "[%2d] %-12s %13s %6d %8d %10s\n\r",
                             sort->member->level, capitalize( sort->member->name ),
                             class_table[sort->member->Class]->who_name, sort->member->kills, sort->member->deaths,
                             sort->member->since );
      }

      for( member = members_list->first_member; member; member = member->next )
         if( !str_prefix( format, member->name ) )
            pager_printf( ch, "[%2d] %-12s %13s %6d %8d %10s\n\r",
                          member->level, capitalize( member->name ), class_table[member->Class]->who_name,
                          member->kills, member->deaths, member->since );
   }
   else
   {
      for( member = members_list->first_member; member; member = member->next )
         pager_printf( ch, "[%2d] %-12s %13s %6d  %7d %10s\n\r", member->level, capitalize( member->name ),
                       class_table[member->Class]->who_name, member->kills, member->deaths, member->since );
   }
   send_to_pager( "------------------------------------------------------------\n\r", ch );
}

void remove_member( char *clan_name, char *name )
{
   MEMBER_LIST *members_list;
   MEMBER_DATA *member;

   if( !clan_name )
   {
      bug( "%s", "remove_member: NULL clan name being passed!" );
      return;
   }

   if( !name )
   {
      bug( "%s", "remove_member: NULL name being passed!" );
      return;
   }

   for( members_list = first_member_list; members_list; members_list = members_list->next )
   {
      if( !str_cmp( members_list->name, clan_name ) )
         break;
   }

   if( !members_list )
      return;

   for( member = members_list->first_member; member; member = member->next )
   {
      if( !str_cmp( member->name, name ) )
      {
         UNLINK( member, members_list->first_member, members_list->last_member, next, prev );
         STRFREE( member->name );
         STRFREE( member->since );
         DISPOSE( member );
         save_member_list( members_list );
         break;
      }
   }
}

CMDF do_roster( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) || !ch->pcdata->clan
       || ( str_cmp( ch->name, ch->pcdata->clan->leader )
            && str_cmp( ch->name, ch->pcdata->clan->number1 )
            && str_cmp( ch->name, ch->pcdata->clan->number2 ) && str_cmp( ch->name, ch->pcdata->clan->deity ) ) )
   {
      send_to_char( "But you aren't in a guild or clan.\n\r", ch );
      if( IS_IMP( ch ) )
         send_to_char( "Use the 'members' command to view the roster.\n\r", ch );
      return;
   }

   show_members( ch, ch->pcdata->clan->name, argument );
   return;
}

CMDF do_members( CHAR_DATA * ch, char *argument )
{
   char arg1[MSL];
   argument = one_argument( argument, arg1 );

   if( argument[0] == '\0' || arg1[0] == '\0' )
   {
      send_to_char( "Usage: members show <guild/clan>\n\r", ch );
      send_to_char( "Usage: members show all\n\r", ch );
      send_to_char( "Usage: members create <roster name>\n\r", ch );
      send_to_char( "Usage: members delete <roster name>\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "show" ) )
   {
      if( !str_cmp( argument, "all" ) )
      {
         MEMBER_LIST *members_list;
         for( members_list = first_member_list; members_list; members_list = members_list->next )
            show_members( ch, members_list->name, NULL );
         return;
      }
      show_members( ch, argument, NULL );
      return;
   }

   if( !str_cmp( arg1, "create" ) )
   {
      MEMBER_LIST *members_list;

      CREATE( members_list, MEMBER_LIST, 1 );
      members_list->name = STRALLOC( argument );
      LINK( members_list, first_member_list, last_member_list, next, prev );
      save_member_list( members_list );
      ch_printf( ch, "Member lists \"%s\" created.\n\r", argument );
      return;
   }

   if( !str_cmp( arg1, "delete" ) )
   {
      MEMBER_LIST *members_list;
      MEMBER_DATA *member;

      for( members_list = first_member_list; members_list; members_list = members_list->next )
         if( !str_cmp( argument, members_list->name ) )
         {
            while( members_list->first_member )
            {
               member = members_list->first_member;
               STRFREE( member->name );
               STRFREE( member->since );
               UNLINK( member, members_list->first_member, members_list->last_member, next, prev );
               DISPOSE( member );
            }
            STRFREE( members_list->name );
            UNLINK( members_list, first_member_list, last_member_list, next, prev );
            DISPOSE( members_list );
            ch_printf( ch, "Member list \"%s\" destroyed.\n\r", argument );
            return;
         }
      send_to_char( "No such list.\n\r", ch );
      return;
   }
}

bool load_member_list( char *filename )
{
   FILE *fp;
   char buf[256];
   MEMBER_LIST *members_list;
   MEMBER_DATA *member;

   snprintf( buf, 256, "%s%s.mem", CLAN_DIR, filename );

   if( ( fp = fopen( buf, "r" ) ) == NULL )
   {
      bug( "%s", "Cannot open member list for reading" );
      return FALSE;
   }

   CREATE( members_list, MEMBER_LIST, 1 );

   for( ;; )
   {
      char *word;

      word = fread_word( fp );

      if( !str_cmp( word, "Name" ) )
      {
         members_list->name = fread_string( fp );
         continue;
      }
      else if( !str_cmp( word, "Member" ) )
      {
         CREATE( member, MEMBER_DATA, 1 );
         member->name = STRALLOC( fread_word( fp ) );
         member->since = STRALLOC( fread_word( fp ) );
         member->kills = fread_number( fp );
         member->deaths = fread_number( fp );
         member->level = fread_number( fp );
         member->Class = fread_number( fp );
         LINK( member, members_list->first_member, members_list->last_member, next, prev );
         continue;
      }
      else if( !str_cmp( word, "End" ) )
      {
         LINK( members_list, first_member_list, last_member_list, next, prev );
         FCLOSE( fp );
         return TRUE;
      }
      else
      {
         bug( "load_members_list: bad section %s", word );
         FCLOSE( fp );
         return FALSE;
      }
   }

}

void update_member( CHAR_DATA * ch )
{
   MEMBER_LIST *members_list;
   MEMBER_DATA *member;

   if( IS_NPC( ch ) || !ch->pcdata->clan )
      return;

   for( members_list = first_member_list; members_list; members_list = members_list->next )
      if( !str_cmp( members_list->name, ch->pcdata->clan_name ) )
      {
         for( member = members_list->first_member; member; member = member->next )
            if( !str_cmp( member->name, ch->name ) )
            {
               if( ch->pcdata->clan->clan_type == CLAN_PLAIN )
               {
                  member->kills = ch->pcdata->pkills;
                  member->deaths = ch->pcdata->pdeaths;
               }
               else
               {
                  member->kills = ch->pcdata->mkills;
                  member->deaths = ch->pcdata->mdeaths;
               }

               member->level = ch->level;
               return;
            }

         if( member == NULL )
         {
            struct tm *t = localtime( &current_time );

            CREATE( member, MEMBER_DATA, 1 );
            member->name = STRALLOC( ch->name );
            member->level = ch->level;
            member->Class = ch->Class;
            stralloc_printf( &member->since, "[%02d|%02d|%04d]", t->tm_mon + 1, t->tm_mday, t->tm_year + 1900 );
            if( ch->pcdata->clan->clan_type == CLAN_PLAIN )
            {
               member->kills = ch->pcdata->pkills;
               member->deaths = ch->pcdata->pdeaths;
            }
            else
            {
               member->kills = ch->pcdata->mkills;
               member->deaths = ch->pcdata->mdeaths;
            }
            LINK( member, members_list->first_member, members_list->last_member, next, prev );
            save_member_list( members_list );
         }
      }
}

void delete_clan( CHAR_DATA * ch, CLAN_DATA * clan )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *vch;
   ROOM_INDEX_DATA *room = NULL;
   MOB_INDEX_DATA *mob = NULL;
   char filename[256], storeroom[256], record[256], roster[256];
   char clanname[MIL];

   mudstrlcpy( filename, clan->filename, 256 );
   mudstrlcpy( clanname, clan->name, 256 );
   snprintf( storeroom, 256, "%s.vault", filename );
   snprintf( record, 256, "%s.record", filename );
   snprintf( roster, 256, "%s.mem", filename );

   for( d = first_descriptor; d; d = d->next )
   {
      vch = d->original ? d->original : d->character;

      if( !vch->pcdata->clan )
         continue;

      if( vch->pcdata->clan == clan )
      {
         STRFREE( vch->pcdata->clan_name );
         vch->pcdata->clan = NULL;
         ch_printf( ch, "The %s known as &W%s&D has been destroyed by the gods!\n\r",
                    clan->clan_type == CLAN_ORDER ? "guild" : "clan", clan->name );
      }
   }

   if( !remove( roster ) )
   {
      if( !ch )
         log_string( "Clan roster file destroyed." );
      else
         send_to_char( "Clan roster file destroyed.\n\r", ch );
   }

   if( !remove( record ) )
   {
      if( !ch )
         log_string( "Clan Pkill record file destroyed." );
      else
         send_to_char( "Clan Pkill record file destroyed.\n\r", ch );
   }

   if( clan->storeroom )
   {
      room = get_room_index( clan->storeroom );

      if( room )
      {
         REMOVE_ROOM_FLAG( room, ROOM_CLANSTOREROOM );
         if( !remove( storeroom ) )
         {
            if( !ch )
               log_string( "Clan storeroom file destroyed." );
            else
               send_to_char( "Clan storeroom file destroyed.\n\r", ch );
         }
      }
   }

   if( clan->idmob )
   {
      mob = get_mob_index( clan->idmob );

      if( mob )
      {
         REMOVE_ACT_FLAG( mob, ACT_GUILDIDMOB );

         for( vch = first_char; vch; vch = vch->next )
         {
            if( IS_NPC( vch ) && vch->pIndexData == mob )
               REMOVE_ACT_FLAG( vch, ACT_GUILDIDMOB );
         }
      }
   }

   if( clan->auction )
   {
      mob = get_mob_index( clan->auction );

      if( mob )
      {
         char aucfile[256];

         REMOVE_ACT_FLAG( mob, ACT_GUILDAUC );

         for( vch = first_char; vch; vch = vch->next )
         {
            if( IS_NPC( vch ) && vch->pIndexData == mob )
            {
               REMOVE_ACT_FLAG( vch, ACT_GUILDAUC );
               REMOVE_ROOM_FLAG( vch->in_room, ROOM_AUCTION );
            }
         }
         snprintf( aucfile, 256, "%s%s", AUC_DIR, mob->short_descr );
         if( !remove( aucfile ) )
         {
            if( !ch )
               log_string( "Clan auction house file destroyed." );
            else
               send_to_char( "Clan auction house file destroyed.\n\r", ch );
         }
      }
   }

   if( clan->shopkeeper )
   {
      mob = get_mob_index( clan->shopkeeper );

      if( mob )
      {
         char shopfile[256];

         REMOVE_ACT_FLAG( mob, ACT_GUILDVENDOR );

         for( vch = first_char; vch; vch = vch->next )
         {
            if( IS_NPC( vch ) && vch->pIndexData == mob )
               REMOVE_ACT_FLAG( vch, ACT_GUILDVENDOR );
         }
         snprintf( shopfile, 256, "%s%s", SHOP_DIR, mob->short_descr );
         if( !remove( shopfile ) )
         {
            if( !ch )
               log_string( "Clan shop file destroyed." );
            else
               send_to_char( "Clan shop file destroyed.\n\r", ch );
         }
      }
   }

   free_one_clan( clan );

   if( !ch )
   {
      if( !remove( filename ) )
         log_printf( "Clan data for %s destroyed - no members left.", clanname );
      return;
   }

   if( !remove( filename ) )
   {
      ch_printf( ch, "&RClan data for %s has been destroyed.\n\r", clanname );
      log_printf( "Clan data for %s has been destroyed by %s.", clanname, ch->name );
   }
   return;
}

void write_clan_list( void )
{
   CLAN_DATA *tclan;
   FILE *fpout;
   char filename[256];

   snprintf( filename, 256, "%s%s", CLAN_DIR, CLAN_LIST );
   fpout = fopen( filename, "w" );
   if( !fpout )
   {
      bug( "%s", "FATAL: cannot open clan.lst for writing!" );
      return;
   }
   for( tclan = first_clan; tclan; tclan = tclan->next )
      fprintf( fpout, "%s\n", tclan->filename );
   fprintf( fpout, "%s", "$\n" );
   FCLOSE( fpout );
}

void write_council_list( void )
{
   COUNCIL_DATA *tcouncil;
   FILE *fpout;
   char filename[256];

   snprintf( filename, 256, "%s%s", COUNCIL_DIR, COUNCIL_LIST );
   fpout = fopen( filename, "w" );
   if( !fpout )
   {
      bug( "%s", "FATAL: cannot open council.lst for writing!" );
      return;
   }
   for( tcouncil = first_council; tcouncil; tcouncil = tcouncil->next )
      fprintf( fpout, "%s\n", tcouncil->filename );
   fprintf( fpout, "%s", "$\n" );
   FCLOSE( fpout );
}

/*
 * Save a clan's data to its data file
 */
void save_clan( CLAN_DATA * clan )
{
   FILE *fp;
   char filename[256];

   if( !clan )
   {
      bug( "%s", "save_clan: null clan pointer!" );
      return;
   }

   if( !clan->filename || clan->filename[0] == '\0' )
   {
      bug( "save_clan: %s has no filename", clan->name );
      return;
   }

   snprintf( filename, 256, "%s%s", CLAN_DIR, clan->filename );

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s", "save_clan: fopen" );
      perror( filename );
   }
   else
   {
      fprintf( fp, "%s", "#CLAN\n" );
      fprintf( fp, "Name         %s~\n", clan->name );
      fprintf( fp, "Filename     %s~\n", clan->filename );
      fprintf( fp, "Motto        %s~\n", clan->motto );
      fprintf( fp, "Description  %s~\n", clan->clandesc );
      fprintf( fp, "Deity        %s~\n", clan->deity );
      fprintf( fp, "Leader       %s~\n", clan->leader );
      fprintf( fp, "NumberOne    %s~\n", clan->number1 );
      fprintf( fp, "NumberTwo    %s~\n", clan->number2 );
      fprintf( fp, "Badge        %s~\n", clan->badge );
      fprintf( fp, "Leadrank     %s~\n", clan->leadrank );
      fprintf( fp, "Onerank      %s~\n", clan->onerank );
      fprintf( fp, "Tworank      %s~\n", clan->tworank );
      fprintf( fp, "PKills       %d %d %d %d %d %d %d %d %d %d\n",
               clan->pkills[0], clan->pkills[1], clan->pkills[2],
               clan->pkills[3], clan->pkills[4], clan->pkills[5],
               clan->pkills[6], clan->pkills[7], clan->pkills[8], clan->pkills[9] );
      fprintf( fp, "PDeaths      %d %d %d %d %d %d %d %d %d %d\n",
               clan->pdeaths[0], clan->pdeaths[1], clan->pdeaths[2],
               clan->pdeaths[3], clan->pdeaths[4], clan->pdeaths[5],
               clan->pdeaths[6], clan->pdeaths[7], clan->pdeaths[8], clan->pdeaths[9] );
      fprintf( fp, "MKills       %d\n", clan->mkills );
      fprintf( fp, "MDeaths      %d\n", clan->mdeaths );
      fprintf( fp, "IllegalPK    %d\n", clan->illegal_pk );
      fprintf( fp, "Score        %d\n", clan->score );
      fprintf( fp, "Type         %d\n", clan->clan_type );
      fprintf( fp, "Class        %d\n", clan->Class );
      fprintf( fp, "Favour       %d\n", clan->favour );
      fprintf( fp, "Strikes      %d\n", clan->strikes );
      fprintf( fp, "Members      %d\n", clan->members );
      fprintf( fp, "MemLimit     %d\n", clan->mem_limit );
      fprintf( fp, "Alignment    %d\n", clan->alignment );
      fprintf( fp, "Board        %d\n", clan->board );
      fprintf( fp, "ClanObjOne   %d\n", clan->clanobj1 );
      fprintf( fp, "ClanObjTwo   %d\n", clan->clanobj2 );
      fprintf( fp, "ClanObjThree %d\n", clan->clanobj3 );
      fprintf( fp, "ClanObjFour  %d\n", clan->clanobj4 );
      fprintf( fp, "ClanObjFive  %d\n", clan->clanobj5 );
      fprintf( fp, "Recall       %d\n", clan->recall );
      fprintf( fp, "Storeroom    %d\n", clan->storeroom );
      fprintf( fp, "GuardOne     %d\n", clan->guard1 );
      fprintf( fp, "GuardTwo     %d\n", clan->guard2 );
      fprintf( fp, "Tithe	   %d\n", clan->tithe );
      fprintf( fp, "Balance	   %d\n", clan->balance );
      fprintf( fp, "Idmob	   %d\n", clan->idmob );
      fprintf( fp, "Inn		   %d\n", clan->inn );
      fprintf( fp, "Shopkeeper   %d\n", clan->shopkeeper );
      fprintf( fp, "Auction	   %d\n", clan->auction );
      fprintf( fp, "Bank	   %d\n", clan->bank );
      fprintf( fp, "Repair	   %d\n", clan->repair );
      fprintf( fp, "Forge	   %d\n", clan->forge );
      fprintf( fp, "%s", "End\n\n" );
      fprintf( fp, "%s", "#END\n" );
   }
   FCLOSE( fp );
   return;
}

/*
 * Save a council's data to its data file
 */
void save_council( COUNCIL_DATA * council )
{
   FILE *fp;
   char filename[256];

   if( !council )
   {
      bug( "%s", "save_council: null council pointer!" );
      return;
   }

   if( !council->filename || council->filename[0] == '\0' )
   {
      bug( "save_council: %s has no filename", council->name );
      return;
   }

   snprintf( filename, 256, "%s%s", COUNCIL_DIR, council->filename );

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s", "save_council: fopen" );
      perror( filename );
   }
   else
   {
      fprintf( fp, "%s", "#COUNCIL\n" );
      if( council->name )
         fprintf( fp, "Name         %s~\n", council->name );
      if( council->filename )
         fprintf( fp, "Filename     %s~\n", council->filename );
      if( council->councildesc )
         fprintf( fp, "Description  %s~\n", council->councildesc );
      if( council->head )
         fprintf( fp, "Head         %s~\n", council->head );
      if( council->head2 != NULL )
         fprintf( fp, "Head2        %s~\n", council->head2 );
      fprintf( fp, "Members      %d\n", council->members );
      fprintf( fp, "Board        %d\n", council->board );
      fprintf( fp, "Meeting      %d\n", council->meeting );
      if( council->powers )
         fprintf( fp, "Powers       %s~\n", council->powers );
      fprintf( fp, "%s", "End\n\n" );
      fprintf( fp, "%s", "#END\n" );
   }
   FCLOSE( fp );
   return;
}

/*
 * Read in actual clan data.
 */

/*
 * Reads in PKill and PDeath still for backward compatibility but now it
 * should be written to PKillRange and PDeathRange for multiple level pkill
 * tracking support. --Shaddai
 *
 * Redone properly by Samson, breaks compatibility, but hey. It had to be done.
 *
 * Added a hardcoded limit memlimit to the amount of members a clan can 
 * have set using setclan.  --Shaddai
 */
void fread_clan( CLAN_DATA * clan, FILE * fp )
{
   const char *word;
   char *ln;
   int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10;
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
            KEY( "Alignment", clan->alignment, fread_number( fp ) );
            KEY( "Auction", clan->auction, fread_number( fp ) );
            break;

         case 'B':
            KEY( "Badge", clan->badge, fread_string( fp ) );
            KEY( "Board", clan->board, fread_number( fp ) );
            KEY( "Balance", clan->balance, fread_number( fp ) );
            KEY( "Bank", clan->bank, fread_number( fp ) );
            break;

         case 'C':
            KEY( "ClanObjOne", clan->clanobj1, fread_number( fp ) );
            KEY( "ClanObjTwo", clan->clanobj2, fread_number( fp ) );
            KEY( "ClanObjThree", clan->clanobj3, fread_number( fp ) );
            KEY( "ClanObjFour", clan->clanobj4, fread_number( fp ) );
            KEY( "ClanObjFive", clan->clanobj5, fread_number( fp ) );
            KEY( "Class", clan->Class, fread_number( fp ) );
            break;

         case 'D':
            KEY( "Deity", clan->deity, fread_string( fp ) );
            KEY( "Description", clan->clandesc, fread_string_nohash( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'F':
            KEY( "Favour", clan->favour, fread_number( fp ) );
            KEY( "Filename", clan->filename, fread_string_nohash( fp ) );
            KEY( "Forge", clan->forge, fread_number( fp ) );
            break;

         case 'G':
            KEY( "GuardOne", clan->guard1, fread_number( fp ) );
            KEY( "GuardTwo", clan->guard2, fread_number( fp ) );
            break;

         case 'I':
            KEY( "Idmob", clan->idmob, fread_number( fp ) );
            KEY( "IllegalPK", clan->illegal_pk, fread_number( fp ) );
            KEY( "Inn", clan->inn, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Leader", clan->leader, fread_string( fp ) );
            KEY( "Leadrank", clan->leadrank, fread_string( fp ) );
            break;

         case 'M':
            KEY( "MDeaths", clan->mdeaths, fread_number( fp ) );
            KEY( "Members", clan->members, fread_number( fp ) );
            KEY( "MemLimit", clan->mem_limit, fread_number( fp ) );
            KEY( "MKills", clan->mkills, fread_number( fp ) );
            KEY( "Motto", clan->motto, fread_string_nohash( fp ) );
            break;

         case 'N':
            KEY( "Name", clan->name, fread_string( fp ) );
            KEY( "NumberOne", clan->number1, fread_string( fp ) );
            KEY( "NumberTwo", clan->number2, fread_string( fp ) );
            break;

         case 'O':
            KEY( "Onerank", clan->onerank, fread_string( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "PDeaths" ) )
            {
               ln = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = x9 = x10 = 0;

               sscanf( ln, "%d %d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9, &x10 );
               clan->pdeaths[0] = x1;
               clan->pdeaths[1] = x2;
               clan->pdeaths[2] = x3;
               clan->pdeaths[3] = x4;
               clan->pdeaths[4] = x5;
               clan->pdeaths[5] = x6;
               clan->pdeaths[6] = x7;
               clan->pdeaths[7] = x8;
               clan->pdeaths[8] = x9;
               clan->pdeaths[9] = x10;
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "PKills" ) )
            {
               ln = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = x9 = x10 = 0;

               sscanf( ln, "%d %d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9, &x10 );
               clan->pkills[0] = x1;
               clan->pkills[1] = x2;
               clan->pkills[2] = x3;
               clan->pkills[3] = x4;
               clan->pkills[4] = x5;
               clan->pkills[5] = x6;
               clan->pkills[6] = x7;
               clan->pkills[7] = x8;
               clan->pkills[8] = x9;
               clan->pkills[9] = x10;
               fMatch = TRUE;
               break;
            }
            break;

         case 'R':
            KEY( "Recall", clan->recall, fread_number( fp ) );
            KEY( "Repair", clan->repair, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Score", clan->score, fread_number( fp ) );
            KEY( "Strikes", clan->strikes, fread_number( fp ) );
            KEY( "Storeroom", clan->storeroom, fread_number( fp ) );
            KEY( "Shopkeeper", clan->shopkeeper, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Tithe", clan->tithe, fread_number( fp ) );
            KEY( "Tworank", clan->tworank, fread_string( fp ) );
            KEY( "Type", clan->clan_type, fread_number( fp ) );
            break;
      }
      if( !fMatch )
         bug( "Fread_clan: no match: %s", word );
   }
}

/*
 * Read in actual council data.
 */
void fread_council( COUNCIL_DATA * council, FILE * fp )
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

         case 'B':
            KEY( "Board", council->board, fread_number( fp ) );
            break;

         case 'D':
            KEY( "Description", council->councildesc, fread_string_nohash( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               return;
            }
            break;

         case 'F':
            KEY( "Filename", council->filename, fread_string_nohash( fp ) );
            break;

         case 'H':
            KEY( "Head", council->head, fread_string( fp ) );
            KEY( "Head2", council->head2, fread_string( fp ) );
            break;

         case 'M':
            KEY( "Members", council->members, fread_number( fp ) );
            KEY( "Meeting", council->meeting, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", council->name, fread_string( fp ) );
            break;

         case 'P':
            KEY( "Powers", council->powers, fread_string( fp ) );
            break;
      }

      if( !fMatch )
         bug( "Fread_council: no match: %s", word );
   }
}

/* Sets up a bunch of default values for new clans or during loadup so we don't get wierd stuff - Samson 7-16-00 */
void clean_clan( CLAN_DATA * clan )
{
   clan->getleader = FALSE;
   clan->getone = FALSE;
   clan->gettwo = FALSE;
   clan->members = 1;
   clan->mem_limit = 15;
   clan->mkills = 0;
   clan->mdeaths = 0;
   clan->illegal_pk = 0;
   clan->score = 0;
   clan->clan_type = CLAN_ORDER;
   clan->favour = 0;
   clan->strikes = 0;
   clan->alignment = 0;
   clan->recall = 0;
   clan->storeroom = 0;
   clan->guard1 = 0;
   clan->guard2 = 0;
   clan->tithe = 0;
   clan->balance = 0;
   clan->inn = 0;
   clan->idmob = 0;
   clan->shopkeeper = 0;
   clan->auction = 0;
   clan->bank = 0;
   clan->repair = 0;
   clan->forge = 0;
   clan->pkills[0] = 0;
   clan->pkills[1] = 0;
   clan->pkills[2] = 0;
   clan->pkills[3] = 0;
   clan->pkills[4] = 0;
   clan->pkills[5] = 0;
   clan->pkills[6] = 0;
   clan->pkills[7] = 0;
   clan->pkills[8] = 0;
   clan->pkills[9] = 0;
   clan->pdeaths[0] = 0;
   clan->pdeaths[1] = 0;
   clan->pdeaths[2] = 0;
   clan->pdeaths[3] = 0;
   clan->pdeaths[4] = 0;
   clan->pdeaths[5] = 0;
   clan->pdeaths[6] = 0;
   clan->pdeaths[7] = 0;
   clan->pdeaths[8] = 0;
   clan->pdeaths[9] = 0;

   return;
}

/*
 * Load a clan file
 */
bool load_clan_file( const char *clanfile )
{
   char filename[256];
   CLAN_DATA *clan;
   FILE *fp;
   bool found;

   CREATE( clan, CLAN_DATA, 1 );

   clean_clan( clan );  /* Default settings so we don't get wierd ass stuff */

   found = FALSE;
   snprintf( filename, 256, "%s%s", CLAN_DIR, clanfile );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
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
            bug( "%s", "Load_clan_file: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "CLAN" ) )
         {
            fread_clan( clan, fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_clan_file: bad section: %s.", word );
            break;
         }
      }
      FCLOSE( fp );
   }

   if( found )
   {
      ROOM_INDEX_DATA *storeroom;

      LINK( clan, first_clan, last_clan, next, prev );

      if( !load_member_list( clan->filename ) )
      {
         MEMBER_LIST *members_list;

         log_string( "No memberlist found, creating new list" );
         CREATE( members_list, MEMBER_LIST, 1 );
         members_list->name = STRALLOC( clan->name );
         LINK( members_list, first_member_list, last_member_list, next, prev );
         save_member_list( members_list );
      }

      if( clan->storeroom == 0 || ( storeroom = get_room_index( clan->storeroom ) ) == NULL )
      {
         log_string( "Storeroom not found" );
         return found;
      }

      snprintf( filename, 256, "%s%s.vault", CLAN_DIR, clan->filename );
      if( ( fp = fopen( filename, "r" ) ) != NULL )
      {
         int iNest;
         bool cfound;
         OBJ_DATA *tobj, *tobj_next;

         log_string( "Loading clan storage room" );
         rset_supermob( storeroom );
         for( iNest = 0; iNest < MAX_NEST; iNest++ )
            rgObjNest[iNest] = NULL;

         cfound = TRUE;
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
               bug( "Load_clan_vault: # not found. %s", clan->name );
               break;
            }

            word = fread_word( fp );
            if( !str_cmp( word, "OBJECT" ) ) /* Objects  */
               fread_obj( supermob, fp, OS_CARRY );
            else if( !str_cmp( word, "END" ) )  /* Done     */
               break;
            else
            {
               bug( "Load_clan_vault: %s bad section.", clan->name );
               break;
            }
         }
         FCLOSE( fp );
         for( tobj = supermob->first_carrying; tobj; tobj = tobj_next )
         {
            tobj_next = tobj->next_content;
            obj_from_char( tobj );
            if( tobj->rent >= sysdata.minrent )
               extract_obj( tobj );
            else
               obj_to_room( tobj, storeroom, supermob );
         }
         release_supermob(  );
      }
      else
         log_string( "Cannot open clan vault" );
   }
   else
      DISPOSE( clan );

   return found;
}

/*
 * Load a council file
 */
bool load_council_file( const char *councilfile )
{
   char filename[256];
   COUNCIL_DATA *council;
   FILE *fp;
   bool found;

   CREATE( council, COUNCIL_DATA, 1 );

   found = FALSE;
   snprintf( filename, 256, "%s%s", COUNCIL_DIR, councilfile );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
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
            bug( "%s", "Load_council_file: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "COUNCIL" ) )
         {
            fread_council( council, fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s", "Load_council_file: bad section." );
            break;
         }
      }
      FCLOSE( fp );
   }

   if( found )
      LINK( council, first_council, last_council, next, prev );

   else
      DISPOSE( council );

   return found;
}

void verify_clans( void )
{
   CLAN_DATA *clan, *clan_next;
   bool change = FALSE;

   log_string( "Cleaning up clan data..." );
   for( clan = first_clan; clan; clan = clan_next )
   {
      clan_next = clan->next;

      if( !clan->leader && !clan->number1 && !clan->number2 )
      {
         delete_clan( NULL, clan );
         continue;
      }

      if( clan->members < 1 )
      {
         delete_clan( NULL, clan );
         continue;
      }

      if( clan->members < 3 )
      {
         if( clan->members == 2 && clan->number2 )
         {
            STRFREE( clan->number2 );
            save_clan( clan );
         }
         if( clan->members == 1 && clan->number1 && clan->number2 )
         {
            STRFREE( clan->number1 );
            STRFREE( clan->number2 );
            save_clan( clan );
         }
      }

      if( !exists_player( clan->leader ) )
      {
         if( !exists_player( clan->number1 ) )
         {
            if( !exists_player( clan->number2 ) )
            {
               STRFREE( clan->leader );
               STRFREE( clan->number1 );
               STRFREE( clan->number2 );
               clan->getleader = TRUE;
               clan->getone = TRUE;
               clan->gettwo = TRUE;
               save_clan( clan );
               continue;
            }
         }
      }

      change = FALSE;
      log_printf( "Checking data for %s.....", clan->name );

      if( !exists_player( clan->leader ) )
      {
         STRFREE( clan->leader );
         if( exists_player( clan->number1 ) )
         {
            change = TRUE;
            clan->leader = STRALLOC( clan->number1 );
            STRFREE( clan->number1 );
            if( exists_player( clan->number2 ) )
            {
               clan->number1 = STRALLOC( clan->number2 );
               STRFREE( clan->number2 );
               clan->gettwo = TRUE;
            }
            else
            {
               STRFREE( clan->number2 );
               clan->getone = TRUE;
               clan->gettwo = TRUE;
            }
         }
         else if( exists_player( clan->number2 ) )
         {
            change = TRUE;
            clan->leader = STRALLOC( clan->number2 );
            STRFREE( clan->number1 );
            STRFREE( clan->number2 );
            clan->getone = TRUE;
            clan->gettwo = TRUE;
         }
         else
            clan->getleader = TRUE;
      }
      if( !exists_player( clan->number1 ) )
      {
         STRFREE( clan->number1 );
         if( exists_player( clan->number2 ) )
         {
            change = TRUE;
            clan->number1 = STRALLOC( clan->number2 );
            STRFREE( clan->number2 );
            clan->gettwo = TRUE;
         }
         else
         {
            STRFREE( clan->number2 );
            clan->getone = TRUE;
            clan->gettwo = TRUE;
         }
      }
      if( !exists_player( clan->number2 ) )
      {
         STRFREE( clan->number2 );
         clan->gettwo = TRUE;
      }
      if( change == TRUE )
         log_printf( "Administration data for %s has changed.", clan->name );

      save_clan( clan );
   }
   write_clan_list(  );
   return;
}

/*
 * Load in all the clan files.
 */
void load_clans( void )
{
   FILE *fpList;
   const char *filename;
   char clanlist[256];

   first_clan = NULL;
   last_clan = NULL;

   log_string( "Loading clans..." );

   snprintf( clanlist, 256, "%s%s", CLAN_DIR, CLAN_LIST );

   if( !( fpList = fopen( clanlist, "r" ) ) )
   {
      perror( clanlist );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fpList ) ? "$" : fread_word( fpList );
      log_string( filename );
      if( filename[0] == '$' )
         break;

      if( !load_clan_file( filename ) )
         bug( "Cannot load clan file: %s", filename );
   }
   FCLOSE( fpList );
   verify_clans(  ); /* Check against pfiles to see if clans should still exist */
   log_string( "Done clans" );
   return;
}

/*
 * Load in all the council files.
 */
void load_councils( void )
{
   FILE *fpList;
   const char *filename;
   char councillist[256];

   first_council = NULL;
   last_council = NULL;

   log_string( "Loading councils..." );

   snprintf( councillist, 256, "%s%s", COUNCIL_DIR, COUNCIL_LIST );
   if( ( fpList = fopen( councillist, "r" ) ) == NULL )
   {
      perror( councillist );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fpList ) ? "$" : fread_word( fpList );
      log_string( filename );
      if( filename[0] == '$' )
         break;

      if( !load_council_file( filename ) )
         bug( "Cannot load council file: %s", filename );
   }
   FCLOSE( fpList );
   log_string( "Done councils" );
   return;
}

void check_clan_info( CHAR_DATA * ch )
{
   CLAN_DATA *clan;

   if( IS_NPC( ch ) )
      return;

   if( !ch->pcdata->clan )
      return;

   clan = ch->pcdata->clan;

   if( clan->getleader == TRUE && str_cmp( ch->name, clan->number1 ) && str_cmp( ch->name, clan->number2 ) )
   {
      STRFREE( clan->leader );
      clan->leader = STRALLOC( ch->name );
      clan->getleader = FALSE;
      save_clan( clan );
      ch_printf( ch, "Your %s's leader no longer existed. You have been made the new leader.\n\r",
                 clan->clan_type == CLAN_ORDER ? "guild" : "clan" );
      if( clan->getone == TRUE || clan->gettwo == TRUE )
      {
         ch_printf( ch,
                    "Other admins of your %s are also missing, it is advised that you pick new ones to avoid\n\rhaving them chosen for you.\n\r",
                    clan->clan_type == CLAN_ORDER ? "guild" : "clan" );
      }
   }

   if( clan->getone == TRUE && str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number2 ) )
   {
      STRFREE( clan->number1 );
      clan->number1 = STRALLOC( ch->name );
      clan->getone = FALSE;
      save_clan( clan );
      ch_printf( ch, "Your %s's first officer no longer existed. You have been made the new first officer.\n\r",
                 clan->clan_type == CLAN_ORDER ? "guild" : "clan" );
   }

   if( clan->gettwo == TRUE && str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number1 ) )
   {
      STRFREE( clan->number2 );
      clan->number2 = STRALLOC( ch->name );
      clan->gettwo = FALSE;
      save_clan( clan );
      ch_printf( ch, "Your %s's second officer no longer existed. You have been made the new second officer.\n\r",
                 clan->clan_type == CLAN_ORDER ? "guild" : "clan" );
   }
   return;
}

/* Function modified from original form - Samson */
CMDF do_make( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   OBJ_INDEX_DATA *pObjIndex;
   OBJ_DATA *obj;
   CLAN_DATA *clan;
   int level;

   if( IS_NPC( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   clan = ch->pcdata->clan;

   if( str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->deity ) && str_cmp( ch->name, clan->number1 ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Make what?\n\r", ch );
      return;
   }

   pObjIndex = get_obj_index( clan->clanobj1 );
   level = 40;

   if( !pObjIndex || !is_name( arg, pObjIndex->name ) )
   {
      pObjIndex = get_obj_index( clan->clanobj2 );
      level = 45;
   }
   if( !pObjIndex || !is_name( arg, pObjIndex->name ) )
   {
      pObjIndex = get_obj_index( clan->clanobj3 );
      level = 50;
   }
   if( !pObjIndex || !is_name( arg, pObjIndex->name ) )
   {
      pObjIndex = get_obj_index( clan->clanobj4 );
      level = 35;
   }
   if( !pObjIndex || !is_name( arg, pObjIndex->name ) )
   {
      pObjIndex = get_obj_index( clan->clanobj5 );
      level = 1;
   }
   if( !pObjIndex || !is_name( arg, pObjIndex->name ) )
   {
      send_to_char( "You don't know how to make that.\n\r", ch );
      return;
   }

   if( !( obj = create_object( pObjIndex, level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   SET_OBJ_FLAG( obj, ITEM_CLANOBJECT );
   if( CAN_WEAR( obj, ITEM_TAKE ) )
      obj = obj_to_char( obj, ch );
   else
      obj = obj_to_room( obj, ch->in_room, ch );
   act( AT_MAGIC, "$n makes $p!", ch, obj, NULL, TO_ROOM );
   act( AT_MAGIC, "You make $p!", ch, obj, NULL, TO_CHAR );
   return;
}

/* Function modified from original form - Samson */
/* Eliminated clan_type GUILD - Samson 11-30-98 */
/* Clan Deity no longer able to issue command - Samson 12-6-98 */
CMDF do_induct( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   CLAN_DATA *clan;

   if( IS_NPC( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   clan = ch->pcdata->clan;

   if( ( ch->pcdata->bestowments && hasname( ch->pcdata->bestowments, "induct" ) )
       || !str_cmp( ch->name, clan->leader ) || !str_cmp( ch->name, clan->number1 ) || !str_cmp( ch->name, clan->number2 ) )
      ;
   else
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Induct whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "That player is not here.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   if( IS_IMMORTAL( victim ) )
   {
      send_to_char( "You can't induct such a godly presence.\n\r", ch );
      return;
   }

   if( victim->level < 10 )
   {
      send_to_char( "This player is not worthy of joining yet.\n\r", ch );
      return;
   }

   if( victim->pcdata->clan )
   {
      if( victim->pcdata->clan->clan_type == CLAN_ORDER )
      {
         if( victim->pcdata->clan == clan )
            send_to_char( "This player already belongs to your guild!\n\r", ch );
         else
            send_to_char( "This player already belongs to a guild!\n\r", ch );
         return;
      }
      else
      {
         if( victim->pcdata->clan == clan )
            send_to_char( "This player already belongs to your clan!\n\r", ch );
         else
            send_to_char( "This player already belongs to a clan!\n\r", ch );
         return;
      }
   }
   if( clan->mem_limit && clan->members >= clan->mem_limit )
   {
      send_to_char( "Your clan is too big to induct anymore players.\n\r", ch );
      return;
   }
   clan->members++;
   if( clan->clan_type != CLAN_ORDER )
      SET_BIT( victim->speaks, LANG_CLAN );

   if( clan->clan_type != CLAN_NOKILL && clan->clan_type != CLAN_ORDER )
      SET_PCFLAG( victim, PCFLAG_DEADLY );

   if( clan->clan_type != CLAN_ORDER && clan->clan_type != CLAN_NOKILL )
   {
      int sn;

      for( sn = 0; sn < top_sn; sn++ )
      {
         if( skill_table[sn]->guild == clan->Class && skill_table[sn]->name != NULL )
         {
            victim->pcdata->learned[sn] = GET_ADEPT( victim, sn );
            ch_printf( victim, "%s instructs you in the ways of %s.\n\r", ch->name, skill_table[sn]->name );
         }
      }
   }

   victim->pcdata->clan = clan;
   STRFREE( victim->pcdata->clan_name );
   victim->pcdata->clan_name = QUICKLINK( clan->name );
   update_member( victim );
   act( AT_MAGIC, "You induct $N into $t", ch, clan->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n inducts $N into $t", ch, clan->name, victim, TO_NOTVICT );
   act( AT_MAGIC, "$n inducts you into $t", ch, clan->name, victim, TO_VICT );
   save_char_obj( victim );
   save_clan( clan );
   return;
}

/* Function modified from original form - Samson */
/* Councils made open to immortals ONLY */
CMDF do_council_induct( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   COUNCIL_DATA *council;

   if( IS_NPC( ch ) || !ch->pcdata->council )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   council = ch->pcdata->council;

   if( ( council->head == NULL || str_cmp( ch->name, council->head ) )
       && ( council->head2 == NULL || str_cmp( ch->name, council->head2 ) ) && str_cmp( council->name, "mortal council" ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Induct whom into your council?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "That player is not here.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   if( victim->level < LEVEL_IMMORTAL )
   {
      send_to_char( "This player is not worthy of joining any council yet.\n\r", ch );
      return;
   }

   if( victim->pcdata->council )
   {
      send_to_char( "This player already belongs to a council!\n\r", ch );
      return;
   }

   council->members++;
   victim->pcdata->council = council;
   STRFREE( victim->pcdata->council_name );
   victim->pcdata->council_name = QUICKLINK( council->name );
   act( AT_MAGIC, "You induct $N into $t", ch, council->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n inducts $N into $t", ch, council->name, victim, TO_ROOM );
   act( AT_MAGIC, "$n inducts you into $t", ch, council->name, victim, TO_VICT );
   save_char_obj( victim );
   save_council( council );
   return;
}

/* Can the character outcast the victim? */
bool can_outcast( CLAN_DATA *clan, CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( !clan || !ch || !victim )
      return FALSE;
   if( !str_cmp( ch->name, clan->leader ) )
      return TRUE;
   if( !str_cmp( victim->name, clan->leader ) )
      return FALSE;
   if( !str_cmp( ch->name, clan->number1 ) )
      return TRUE;
   if( !str_cmp( victim->name, clan->number1 ) )
      return FALSE;
   if( !str_cmp( ch->name, clan->number2 ) )
      return TRUE;
   if( !str_cmp( victim->name, clan->number2 ) )
      return FALSE;
   return TRUE;
}

/* Function modified from original form - Samson */
/* Eliminated clan_type GUILD - Samson 11-30-98 */
/* Clan Deity no longer able to issue command - Samson 12-6-98 */
CMDF do_outcast( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   CLAN_DATA *clan;

   if( IS_NPC( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   clan = ch->pcdata->clan;

   if( ( ch->pcdata && ch->pcdata->bestowments && hasname( ch->pcdata->bestowments, "outcast" ) )
       || !str_cmp( ch->name, clan->leader ) || !str_cmp( ch->name, clan->number1 ) || !str_cmp( ch->name, clan->number2 ) )
      ;
   else
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument && argument[0] == '\0' )
   {
      send_to_char( "Outcast whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "That player is not here.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      if( ch->pcdata->clan->clan_type == CLAN_ORDER )
      {
         send_to_char( "Kick yourself out of your own guild?\n\r", ch );
         return;
      }
      else
      {
         send_to_char( "Kick yourself out of your own clan?\n\r", ch );
         return;
      }
   }

   if( victim->pcdata->clan != ch->pcdata->clan )
   {
      if( ch->pcdata->clan->clan_type == CLAN_ORDER )
      {
         send_to_char( "This player does not belong to your guild!\n\r", ch );
         return;
      }
      else
      {
         send_to_char( "This player does not belong to your clan!\n\r", ch );
         return;
      }
   }

   if( !can_outcast( clan, ch, victim ) )
   {
      send_to_char( "You are not able to outcast them.\r\n", ch );
      return;
   }

   if( clan->clan_type != CLAN_ORDER && clan->clan_type != CLAN_NOKILL )
   {
      int sn;

      for( sn = 0; sn < top_sn; sn++ )
         if( skill_table[sn]->guild == victim->pcdata->clan->Class && skill_table[sn]->name != NULL )
         {
            victim->pcdata->learned[sn] = 0;
            ch_printf( victim, "You forget the ways of %s.\n\r", skill_table[sn]->name );
         }
   }

   if( victim->speaking & LANG_CLAN )
      victim->speaking = LANG_COMMON;
   REMOVE_BIT( victim->speaks, LANG_CLAN );
   --clan->members;
   if( clan->members < 0 )
      clan->members = 0;
   if( !str_cmp( victim->name, ch->pcdata->clan->number1 ) )
      STRFREE( ch->pcdata->clan->number1 );
   if( !str_cmp( victim->name, ch->pcdata->clan->number2 ) )
      STRFREE( ch->pcdata->clan->number2 );
   if( !str_cmp( victim->name, ch->pcdata->clan->deity ) )
      STRFREE( ch->pcdata->clan->deity );
   victim->pcdata->clan = NULL;
   remove_member( clan->name, victim->name );
   STRFREE( victim->pcdata->clan_name );
   act( AT_MAGIC, "You outcast $N from $t", ch, clan->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n outcasts $N from $t", ch, clan->name, victim, TO_ROOM );
   act( AT_MAGIC, "$n outcasts you from $t", ch, clan->name, victim, TO_VICT );

   echo_all_printf( AT_MAGIC, ECHOTAR_ALL, "%s has been outcast from %s!", victim->name, clan->name );

   save_char_obj( victim );   /* clan gets saved when pfile is saved */
   save_clan( clan );
   return;
}

CMDF do_council_outcast( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   COUNCIL_DATA *council;

   if( IS_NPC( ch ) || !ch->pcdata->council )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   council = ch->pcdata->council;

   if( ( council->head == NULL || str_cmp( ch->name, council->head ) )
       && ( council->head2 == NULL || str_cmp( ch->name, council->head2 ) ) && str_cmp( council->name, "mortal council" ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Outcast whom from your council?\n\r", ch );
      return;
   }

   if( ( victim = get_char_world( ch, argument ) ) == NULL )
   {
      send_to_char( "That player is not here.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Kick yourself out of your own council?\n\r", ch );
      return;
   }

   if( victim->pcdata->council != ch->pcdata->council )
   {
      send_to_char( "This player does not belong to your council!\n\r", ch );
      return;
   }
   --council->members;
   victim->pcdata->council = NULL;
   STRFREE( victim->pcdata->council_name );
   check_switch( ch, FALSE );
   act( AT_MAGIC, "You outcast $N from $t", ch, council->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n outcasts $N from $t", ch, council->name, victim, TO_ROOM );
   act( AT_MAGIC, "$n outcasts you from $t", ch, council->name, victim, TO_VICT );
   save_char_obj( victim );
   save_council( council );
   return;
}

CMDF do_setclan( CHAR_DATA * ch, char *argument );

/* Subfunction of setclan for clan leaders and first officers - Samson 12-6-98 */
void pcsetclan( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   CLAN_DATA *clan;

   argument = one_argument( argument, arg1 );

   clan = get_clan( ch->pcdata->clan_name );
   if( !clan )
   {
      send_to_char( "But you are not a clan member?!?!?.\n\r", ch );
      return;
   }

   if( str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number1 ) )
   {
      send_to_char( "Only the clan leader or first officer can change clan information.\n\r", ch );
      return;
   }

   if( arg1[0] == '\0' )
   {
      send_to_char( "Usage: setclan <field> <player/text>\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "number2 ", ch );
      if( !str_cmp( ch->name, clan->leader ) )
      {
         send_to_char( "disband deity leader number1\n\r", ch );
         send_to_char( "leadrank onerank tworank\n\r", ch );
         send_to_char( "motto desc badge tithe\n\r", ch );
      }
      return;
   }

   if( !str_cmp( arg1, "number2" ) )
   {
      if( !exists_player( argument ) )
      {
         send_to_char( "That person does not exist!\n\r", ch );
         return;
      }
      STRFREE( clan->number2 );
      clan->number2 = STRALLOC( argument );
      clan->gettwo = FALSE;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( str_cmp( ch->name, clan->leader ) )
   {
      do_setclan( ch, "" );
      return;
   }

   if( !str_cmp( arg1, "disband" ) )
   {
      CHAR_DATA *vch;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "This requires confirmation!!!\n\r", ch );
         send_to_char( "If you're sure you want to disband, type 'setclan disband yes'.\n\r", ch );
         send_to_char
            ( "Weigh your decision CAREFULLY.\n\rDisbanding will permanently destroy ALL data related to your clan or guild!\n\r",
              ch );
         send_to_char( "This includes any shops, auction houses, storerooms etc you may own in a guildhall.\n\r", ch );
         send_to_char( "It is STRONGLY adivsed that you recover any goods left in such places BEFORE you disband.\n\r", ch );
         send_to_char( "Reimbursements for lost goods will NOT be made.\n\r", ch );
         return;
      }

      if( str_cmp( argument, "yes" ) )
      {
         do_setclan( ch, "disband" );
         return;
      }

      STRFREE( ch->pcdata->clan_name );
      ch->pcdata->clan = NULL;

      for( vch = first_char; vch; vch = vch->next )
      {
         if( !IS_NPC( vch ) && vch->pcdata->clan == clan )
         {
            STRFREE( vch->pcdata->clan_name );
            vch->pcdata->clan = NULL;
         }
      }
      echo_all_printf( AT_RED, ECHOTAR_ALL, "%s has dissolved %s!", ch->name, clan->name );
      log_printf( "%s has dissolved %s", ch->name, clan->name );
      delete_clan( ch, clan );
      write_clan_list(  );
      return;
   }

   if( !str_cmp( arg1, "leadrank" ) )
   {
      STRFREE( clan->leadrank );
      clan->leadrank = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "onerank" ) )
   {
      STRFREE( clan->onerank );
      clan->onerank = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "tworank" ) )
   {
      STRFREE( clan->tworank );
      clan->tworank = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "deity" ) )
   {
      STRFREE( clan->deity );
      clan->deity = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "leader" ) )
   {
      if( !exists_player( argument ) )
      {
         send_to_char( "That person does not exist!\n\r", ch );
         return;
      }
      STRFREE( clan->leader );
      clan->leader = STRALLOC( argument );
      clan->getleader = FALSE;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "number1" ) )
   {
      if( !exists_player( argument ) )
      {
         send_to_char( "That person does not exist!\n\r", ch );
         return;
      }
      STRFREE( clan->number1 );
      clan->number1 = STRALLOC( argument );
      clan->getone = FALSE;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "badge" ) )
   {
      STRFREE( clan->badge );
      clan->badge = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "motto" ) )
   {
      DISPOSE( clan->motto );
      clan->motto = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "desc" ) )
   {
      DISPOSE( clan->clandesc );
      clan->clandesc = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "tithe" ) )
   {
      int value = atoi( argument );

      if( value < 0 || value > 100 )
      {
         send_to_char( "Invalid tithe - range is from 0 to 100.\n\r", ch );
         return;
      }
      clan->tithe = value;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   do_setclan( ch, "" );
   return;
}

/* Function modified from original form - Samson */
/* Eliminated clan_type GUILD - Samson 11-30-98 */
CMDF do_setclan( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   char arg2[MIL];
   CLAN_DATA *clan;

   set_char_color( AT_PLAIN, ch );
   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !IS_IMMORTAL( ch ) )
   {
      pcsetclan( ch, argument );
      return;
   }

   if( !IS_IMP( ch ) )
   {
      send_to_char( "Only implementors can use this command.\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1[0] == '\0' )
   {
      send_to_char( "Usage: setclan <clan> <field> <deity|leader|number1|number2> <player>\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( " deity leader number1 number2\n\r", ch );
      send_to_char( " align (not functional)", ch );
      send_to_char( " leadrank onerank tworank\n\r", ch );
      send_to_char( " board recall storage guard1 guard2\n\r", ch );
      send_to_char( " obj1 obj2 obj3 obj4 obj5\n\r", ch );
      send_to_char( " name motto desc\n\r", ch );
      send_to_char( " favour strikes type Class tithe\n\r", ch );
      send_to_char( " balance inn idmob shopkeeper bank repair forge\n\r", ch );
      send_to_char( "filename members memlimit delete\n\r", ch );
      send_to_char( " pkill1-7 pdeath1-7\n\r", ch );
      return;
   }

   clan = get_clan( arg1 );
   if( !clan )
   {
      send_to_char( "No such clan.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "deity" ) )
   {
      STRFREE( clan->deity );
      clan->deity = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "leadrank" ) )
   {
      STRFREE( clan->leadrank );
      clan->leadrank = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "onerank" ) )
   {
      STRFREE( clan->onerank );
      clan->onerank = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "tworank" ) )
   {
      STRFREE( clan->tworank );
      clan->tworank = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "leader" ) )
   {
      STRFREE( clan->leader );
      clan->leader = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "number1" ) )
   {
      STRFREE( clan->number1 );
      clan->number1 = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "number2" ) )
   {
      STRFREE( clan->number2 );
      clan->number2 = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "badge" ) )
   {
      STRFREE( clan->badge );
      clan->badge = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "board" ) )
   {
      clan->board = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "obj1" ) )
   {
      clan->clanobj1 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "obj2" ) )
   {
      clan->clanobj2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "obj3" ) )
   {
      clan->clanobj3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "obj4" ) )
   {
      clan->clanobj4 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "obj5" ) )
   {
      clan->clanobj5 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "guard1" ) )
   {
      clan->guard1 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "guard2" ) )
   {
      clan->guard2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "recall" ) )
   {
      clan->recall = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "storage" ) )
   {
      clan->storeroom = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "tithe" ) )
   {
      int value = atoi( argument );

      if( value < 0 || value > 100 )
      {
         send_to_char( "Invalid tithe - range is from 0 to 100.\n\r", ch );
         return;
      }
      clan->tithe = value;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "balance" ) )
   {
      int value = atoi( argument );

      if( value < 0 || value > 2000000000 )
      {
         send_to_char( "Invalid balance - range is from 0 to 2 billion.\n\r", ch );
         return;
      }
      clan->balance = value;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "shopkeeper" ) )
   {
      MOB_INDEX_DATA *mob = NULL;
      int value = 0;

      if( atoi( argument ) != 0 )
      {
         mob = get_mob_index( atoi( argument ) );
         if( !mob )
         {
            send_to_char( "That mobile does not exist.\n\r", ch );
            return;
         }
         value = mob->vnum;
      }
      clan->shopkeeper = value;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "idmob" ) )
   {
      MOB_INDEX_DATA *mob = NULL;
      int value = 0;

      if( atoi( argument ) != 0 )
      {
         mob = get_mob_index( atoi( argument ) );
         if( !mob )
         {
            send_to_char( "That mobile does not exist.\n\r", ch );
            return;
         }
         value = mob->vnum;
      }
      clan->idmob = value;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "repair" ) )
   {
      MOB_INDEX_DATA *mob = NULL;
      int value = 0;

      if( atoi( argument ) != 0 )
      {
         mob = get_mob_index( atoi( argument ) );
         if( !mob )
         {
            send_to_char( "That mobile does not exist.\n\r", ch );
            return;
         }
         value = mob->vnum;
      }
      clan->repair = value;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "forge" ) )
   {
      MOB_INDEX_DATA *mob = NULL;
      int value = 0;

      if( atoi( argument ) != 0 )
      {
         mob = get_mob_index( atoi( argument ) );
         if( !mob )
         {
            send_to_char( "That mobile does not exist.\n\r", ch );
            return;
         }
         value = mob->vnum;
      }
      clan->forge = value;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "bank" ) )
   {
      MOB_INDEX_DATA *mob = NULL;
      int value = 0;

      if( atoi( argument ) != 0 )
      {
         mob = get_mob_index( atoi( argument ) );
         if( !mob )
         {
            send_to_char( "That mobile does not exist.\n\r", ch );
            return;
         }
         value = mob->vnum;
      }
      clan->bank = value;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "inn" ) )
   {
      ROOM_INDEX_DATA *room = NULL;
      int value = 0;

      if( atoi( argument ) != 0 )
      {
         room = get_room_index( atoi( argument ) );
         if( !room )
         {
            send_to_char( "That room does not exist.\n\r", ch );
            return;
         }
         value = room->vnum;
      }
      clan->inn = value;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "align" ) )
   {
      int value = atoi( argument );

      if( value < -100 || value > 1000 )
      {
         send_to_char( "Invalid alignment - range is from -1000 to 1000.\n\r", ch );
         return;
      }
      clan->alignment = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( !str_cmp( argument, "guild" ) )
         clan->clan_type = CLAN_ORDER;
      else
         clan->clan_type = CLAN_PLAIN;
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "Class" ) )
   {
      clan->Class = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      CLAN_DATA *uclan = NULL;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't name a clan nothing.\r\n", ch );
         return;
      }
      if( ( uclan = get_clan( argument ) ) )
      {
         send_to_char( "There is already another clan with that name.\r\n", ch );
         return;
      }
      STRFREE( clan->name );
      clan->name = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "motto" ) )
   {
      DISPOSE( clan->motto );
      clan->motto = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "desc" ) )
   {
      DISPOSE( clan->clandesc );
      clan->clandesc = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "memlimit" ) )
   {
      clan->mem_limit = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "members" ) )
   {
      clan->members = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, CLAN_DIR, argument ) )
         return;

      snprintf( filename, sizeof( filename ), "%s%s", CLAN_DIR, clan->filename );
      if( !remove( filename ) )
         send_to_char( "Old clan file deleted.\r\n", ch );

      DISPOSE( clan->filename );
      clan->filename = str_dup( argument );
      send_to_char( "Done.\r\n", ch );
      save_clan( clan );
      write_clan_list(  );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      delete_clan( ch, clan );
      write_clan_list(  );
      return;
   }

   if( !str_prefix( "pkill", arg2 ) )
   {
      int temp_value;
      if( !str_cmp( arg2, "pkill1" ) )
         temp_value = 0;
      else if( !str_cmp( arg2, "pkill2" ) )
         temp_value = 1;
      else if( !str_cmp( arg2, "pkill3" ) )
         temp_value = 2;
      else if( !str_cmp( arg2, "pkill4" ) )
         temp_value = 3;
      else if( !str_cmp( arg2, "pkill5" ) )
         temp_value = 4;
      else if( !str_cmp( arg2, "pkill6" ) )
         temp_value = 5;
      else if( !str_cmp( arg2, "pkill7" ) )
         temp_value = 6;
      else
      {
         do_setclan( ch, "" );
         return;
      }
      clan->pkills[temp_value] = atoi( argument );
      send_to_char( "Ok.\n\r", ch );
      return;
   }

   if( !str_prefix( "pdeath", arg2 ) )
   {
      int temp_value;
      if( !str_cmp( arg2, "pdeath1" ) )
         temp_value = 0;
      else if( !str_cmp( arg2, "pdeath2" ) )
         temp_value = 1;
      else if( !str_cmp( arg2, "pdeath3" ) )
         temp_value = 2;
      else if( !str_cmp( arg2, "pdeath4" ) )
         temp_value = 3;
      else if( !str_cmp( arg2, "pdeath5" ) )
         temp_value = 4;
      else if( !str_cmp( arg2, "pdeath6" ) )
         temp_value = 5;
      else if( !str_cmp( arg2, "pdeath7" ) )
         temp_value = 6;
      else
      {
         do_setclan( ch, "" );
         return;
      }
      clan->pdeaths[temp_value] = atoi( argument );
      send_to_char( "Ok.\n\r", ch );
      return;
   }
   do_setclan( ch, "" );
   return;
}

CMDF do_setcouncil( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   char arg2[MIL];
   COUNCIL_DATA *council;

   set_char_color( AT_PLAIN, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1[0] == '\0' )
   {
      send_to_char( "Usage: setcouncil <council> <field> <player>\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( " head head2 members board meeting name filename desc powers\n\r", ch );
      return;
   }

   council = get_council( arg1 );
   if( !council )
   {
      send_to_char( "No such council.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "head" ) )
   {
      STRFREE( council->head );
      council->head = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_council( council );
      return;
   }

   if( !str_cmp( arg2, "head2" ) )
   {
      STRFREE( council->head2 );
      if( !str_cmp( argument, "none" ) || !str_cmp( argument, "clear" ) )
         council->head2 = NULL;
      else
         council->head2 = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_council( council );
      return;
   }

   if( !str_cmp( arg2, "board" ) )
   {
      council->board = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_council( council );
      return;
   }

   if( !str_cmp( arg2, "members" ) )
   {
      council->members = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_council( council );
      return;
   }

   if( !str_cmp( arg2, "meeting" ) )
   {
      council->meeting = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_council( council );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      COUNCIL_DATA *ucouncil;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Can't set a council name to nothing.\r\n", ch );
         return;
      }
      if( ( ucouncil = get_council( argument ) ) )
      {
         send_to_char( "A council is already using that name.\r\n", ch );
         return;
      }
      STRFREE( council->name );
      council->name = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_council( council );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, COUNCIL_DIR, argument ) )
         return;

      snprintf( filename, sizeof( filename ), "%s%s", COUNCIL_DIR, council->filename );
      if( !remove( filename ) )
         send_to_char( "Old council file deleted.\r\n", ch );

      DISPOSE( council->filename );
      council->filename = str_dup( argument );
      send_to_char( "Done.\r\n", ch );
      save_council( council );
      write_council_list(  );
      return;
   }

   if( !str_cmp( arg2, "desc" ) )
   {
      DISPOSE( council->councildesc );
      council->councildesc = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      save_council( council );
      return;
   }

   if( !str_cmp( arg2, "powers" ) )
   {
      STRFREE( council->powers );
      council->powers = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      save_council( council );
      return;
   }

   do_setcouncil( ch, "" );
   return;
}

/*
 * Added multiple levels on pkills and pdeaths. -- Shaddai
 */
/* Function modified from original form - Samson */
/* Eliminated clan_type GUILD */
CMDF do_showclan( CHAR_DATA * ch, char *argument )
{
   CLAN_DATA *clan;

   set_char_color( AT_PLAIN, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }
   if( argument[0] == '\0' )
   {
      send_to_char( "Usage: showclan <clan>\n\r", ch );
      return;
   }

   clan = get_clan( argument );
   if( !clan )
   {
      send_to_char( "No such clan, or guild.\n\r", ch );
      return;
   }

   ch_printf( ch, "\n\r&w%s     : &W%s\t\tBadge: %s\n\r&wFilename : &W%s\n\r&wMotto    : &W%s\n\r",
              clan->clan_type == CLAN_ORDER ? "Guild" : "Clan",
              clan->name, clan->badge ? clan->badge : "(not set)", clan->filename, clan->motto );
   ch_printf( ch, "&wDesc     : &W%s\n\r&wDeity    : &W%s\n\r", clan->clandesc, clan->deity );
   ch_printf( ch, "&wLeader   : &W%-19.19s\t&wRank: &W%s\n\r", clan->leader, clan->leadrank );
   ch_printf( ch, "&wNumber1  : &W%-19.19s\t&wRank: &W%s\n\r", clan->number1, clan->onerank );
   ch_printf( ch, "&wNumber2  : &W%-19.19s\t&wRank: &W%s\n\r", clan->number2, clan->tworank );
   ch_printf( ch, "&wBalance  : &W%d\n\r", clan->balance );
   ch_printf( ch, "&wTithe    : &W%d\n\r", clan->tithe );
   ch_printf( ch,
              "&wPKills   : &w1-9:&W%-3d &w10-14:&W%-3d &w15-19:&W%-3d &w20-29:&W%-3d &w30-39:&W%-3d &w40-49:&W%-3d &w50:&W%-3d\n\r",
              clan->pkills[0], clan->pkills[1], clan->pkills[2], clan->pkills[3], clan->pkills[4], clan->pkills[5],
              clan->pkills[6] );
   ch_printf( ch,
              "&wPDeaths  : &w1-9:&W%-3d &w10-14:&W%-3d &w15-19:&W%-3d &w20-29:&W%-3d &w30-39:&W%-3d &w40-49:&W%-3d &w50:&W%-3d\n\r",
              clan->pdeaths[0], clan->pdeaths[1], clan->pdeaths[2], clan->pdeaths[3], clan->pdeaths[4], clan->pdeaths[5],
              clan->pdeaths[6] );
   ch_printf( ch, "&wIllegalPK: &W%-6d\n\r", clan->illegal_pk );
   ch_printf( ch, "&wMKills   : &W%-6d   &wMDeaths: &W%-6d\n\r", clan->mkills, clan->mdeaths );
   ch_printf( ch, "&wScore    : &W%-6d   &wFavor  : &W%-6d   &wStrikes: &W%d\n\r",
              clan->score, clan->favour, clan->strikes );
   ch_printf( ch, "&wMembers  : &W%-6d   &wMemLimit  : &W%-6d   &wAlign  : &W%-6d",
              clan->members, clan->mem_limit, clan->alignment );
   send_to_char( "\n\r", ch );
   ch_printf( ch, "&wBoard    : &W%-5d    &wRecall : &W%-5d    &wStorage: &W%-5d\n\r",
              clan->board, clan->recall, clan->storeroom );
   ch_printf( ch, "&wGuard1   : &W%-5d    &wGuard2 : &W%-5d    &wBanker : &W%-5d\n\r",
              clan->guard1, clan->guard2, clan->bank );
   ch_printf( ch, "&wInn      : &W%-5d    &wShop   : &W%-5d    &wAuction: &W%-5d\n\r",
              clan->inn, clan->shopkeeper, clan->auction );
   ch_printf( ch, "&wRepair   : &W%-5d    &wForge  : &W%-5d    &wIdmob  : &W%-5d\n\r",
              clan->repair, clan->forge, clan->idmob );
   ch_printf( ch, "&wObj1( &W%d &w)  Obj2( &W%d &w)  Obj3( &W%d &w)  Obj4( &W%d &w)  Obj5( &W%d &w)\n\r",
              clan->clanobj1, clan->clanobj2, clan->clanobj3, clan->clanobj4, clan->clanobj5 );
   return;
}

CMDF do_showcouncil( CHAR_DATA * ch, char *argument )
{
   COUNCIL_DATA *council;

   set_char_color( AT_PLAIN, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }
   if( argument[0] == '\0' )
   {
      send_to_char( "Usage: showcouncil <council>\n\r", ch );
      return;
   }

   council = get_council( argument );
   if( !council )
   {
      send_to_char( "No such council.\n\r", ch );
      return;
   }

   ch_printf( ch, "\n\r&wCouncil :  &W%s\n\r&wFilename:  &W%s\n\r", council->name, council->filename );
   ch_printf( ch, "&wHead:      &W%s\n\r", council->head );
   ch_printf( ch, "&wHead2:     &W%s\n\r", council->head2 );
   ch_printf( ch, "&wMembers:   &W%-d\n\r", council->members );
   ch_printf( ch, "&wBoard:     &W%-5d\n\r&wMeeting:   &W%-5d\n\r&wPowers:    &W%s\n\r",
              council->board, council->meeting, council->powers );
   ch_printf( ch, "&wDescription:\n\r&W%s\n\r", council->councildesc );
   return;
}

/* Function modified from original form - Samson */
/* Modified to set clan leader on use - Samson 12-6-98 */
CMDF do_makeclan( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   char arg2[MIL];
   char filename[256];
   CLAN_DATA *clan;
   CHAR_DATA *victim;
   bool found;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Usage: makeclan <filename> <clan leader> <clan name>\n\r", ch );
      send_to_char( "Filename must be a SINGLE word, no punctuation marks.\n\r", ch );
      send_to_char( "The clan leader must be present online to form a new clan.\n\r", ch );
      return;
   }

   if( ( victim = get_char_world( ch, arg2 ) ) == NULL )
   {
      send_to_char( "That player is not online.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "An NPC cannot lead a clan!\n\r", ch );
      return;
   }

   clan = get_clan( argument );
   if( clan )
   {
      send_to_char( "There is already a clan with that name.\n\r", ch );
      return;
   }

   found = FALSE;
   snprintf( filename, 256, "%s%s", CLAN_DIR, strlower( arg ) );

   CREATE( clan, CLAN_DATA, 1 );
   LINK( clan, first_clan, last_clan, next, prev );
   clean_clan( clan );  /* Setup default values - Samson 7-16-00 */

   clan->name = STRALLOC( argument );
   clan->filename = str_dup( filename );  /* Bug fix - Samson 5-31-99 */
   clan->leader = STRALLOC( victim->name );

   /*
    * Samson 12-6-98 
    */
   victim->pcdata->clan = clan;
   victim->pcdata->clan_name = QUICKLINK( clan->name );

   save_char_obj( victim );
   save_clan( victim->pcdata->clan );
   write_clan_list(  );

   ch_printf( ch, "Clan %s created with leader %s.\n\r", clan->name, clan->leader );
}

CMDF do_makecouncil( CHAR_DATA * ch, char *argument )
{
   char filename[256];
   COUNCIL_DATA *council;
   bool found;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makecouncil <council name>\n\r", ch );
      return;
   }

   found = FALSE;
   snprintf( filename, 256, "%s%s", COUNCIL_DIR, strlower( argument ) );

   CREATE( council, COUNCIL_DATA, 1 );
   LINK( council, first_council, last_council, next, prev );
   council->name = STRALLOC( argument );
}

/*
 * Added multiple level pkill and pdeath support. --Shaddai
 */

/* Function modified from original form - Samson */
/* Eliminated clan_type GUILD */
CMDF do_clans( CHAR_DATA * ch, char *argument )
{
   CLAN_DATA *clan;
   int count = 0;

   if( argument[0] == '\0' )
   {
      send_to_char
         ( "\n\r&RClan          Deity         Leader           Pkills:    Avatar      Other\n\r_________________________________________________________________________\n\r\n\r",
           ch );
      for( clan = first_clan; clan; clan = clan->next )
      {
         if( clan->clan_type == CLAN_ORDER )
            continue;
         ch_printf( ch, "&w%-13s %-13s %-13s", clan->name, clan->deity, clan->leader );
         ch_printf( ch, "&[blood]                %5d      %5d\n\r",
                    clan->pkills[6], ( clan->pkills[2] + clan->pkills[3] + clan->pkills[4] + clan->pkills[5] ) );
         count++;
      }
      if( !count )
         send_to_char( "&RThere are no Clans currently formed.\n\r", ch );
      else
         send_to_char
            ( "&R_________________________________________________________________________\n\r\n\rUse 'clans <clan>' for detailed information and a breakdown of victories.\n\r",
              ch );
      return;
   }

   clan = get_clan( argument );
   if( !clan || clan->clan_type == CLAN_ORDER )
   {
      send_to_char( "&RNo such clan.\n\r", ch );
      return;
   }
   ch_printf( ch, "\n\r&R%s, '%s'\n\r\n\r", clan->name, clan->motto );
   send_to_char( "&wVictories:&w\n\r", ch );
   ch_printf( ch, "    &w15-19...  &r%-4d\n\r    &w20-29...  &r%-4d\n\r    &w30-39...  &r%-4d\n\r    &w40-49...  &r%-4d\n\r",
              clan->pkills[2], clan->pkills[3], clan->pkills[4], clan->pkills[5] );
   ch_printf( ch, "   &wAvatar...  &r%-4d\n\r", clan->pkills[6] );
   ch_printf( ch, "&wClan Leader:  %s\n\rNumber One :  %s\n\rNumber Two :  %s\n\rClan Deity :  %s\n\r",
              clan->leader, clan->number1, clan->number2, clan->deity );
   if( !str_cmp( ch->name, clan->deity ? clan->deity : "" )
       || !str_cmp( ch->name, clan->leader ? clan->leader : "" )
       || !str_cmp( ch->name, clan->number1 ? clan->number1 : "" )
       || !str_cmp( ch->name, clan->number2 ? clan->number2 : "" ) )
      ch_printf( ch, "Members    :  %d\n\r", clan->members );
   ch_printf( ch, "\n\r&RDescription:  %s\n\r", clan->clandesc );
   return;
}

/* Function modified from original form - Samson */
/* Order referrences altered to read as guilds */
CMDF do_guilds( CHAR_DATA * ch, char *argument )
{
   CLAN_DATA *order;
   int count = 0;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char
         ( "\n\r&gGuild            Deity          Leader           Mkills      Mdeaths\n\r____________________________________________________________________\n\r\n\r",
           ch );
      for( order = first_clan; order; order = order->next )
         if( order->clan_type == CLAN_ORDER )
         {
            ch_printf( ch, "&G%-16s %-14s %-14s   %-7d       %5d\n\r",
                       order->name, order->deity, order->leader, order->mkills, order->mdeaths );
            count++;
         }
      if( !count )
         send_to_char( "&gThere are no Guilds currently formed.\n\r", ch );
      else
         send_to_char
            ( "&g____________________________________________________________________\n\r\n\rUse 'guilds <guild>' for more detailed information.\n\r",
              ch );
      return;
   }

   order = get_clan( argument );
   if( !order || order->clan_type != CLAN_ORDER )
   {
      send_to_char( "&gNo such Guild.\n\r", ch );
      return;
   }

   ch_printf( ch, "\n\r&gGuild of %s\n\r'%s'\n\r\n\r", order->name, order->motto );
   ch_printf( ch, "&GDeity      :  %s\n\rLeader     :  %s\n\rNumber One :  %s\n\rNumber Two :  %s\n\r",
              order->deity, order->leader, order->number1, order->number2 );
   if( !str_cmp( ch->name, order->deity ? order->deity : "" )
       || !str_cmp( ch->name, order->leader ? order->leader : "" )
       || !str_cmp( ch->name, order->number1 ? order->number1 : "" )
       || !str_cmp( ch->name, order->number2 ? order->number2 : "" ) )
      ch_printf( ch, "Members    :  %d\n\r", order->members );
   ch_printf( ch, "\n\r&gDescription:\n\r%s\n\r", order->clandesc );
   return;
}

CMDF do_councils( CHAR_DATA * ch, char *argument )
{
   COUNCIL_DATA *council;

   if( !first_council )
   {
      send_to_char( "There are no councils currently formed.\n\r", ch );
      return;
   }
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "\n\r&cTitle                    Head\n\r", ch );
      for( council = first_council; council; council = council->next )
      {
         if( council->head2 != NULL )
            ch_printf( ch, "&w%-24s %s and %s\n\r", council->name, council->head, council->head2 );
         else
            ch_printf( ch, "&w%-24s %-14s\n\r", council->name, council->head );
      }
      send_to_char( "&cUse 'councils <name of council>' for more detailed information.\n\r", ch );
      return;
   }
   council = get_council( argument );
   if( !council )
   {
      send_to_char( "&cNo such council exists...\n\r", ch );
      return;
   }
   ch_printf( ch, "&c\n\r%s\n\r", council->name );
   if( council->head2 == NULL )
      ch_printf( ch, "&cHead:     &w%s\n\r&cMembers:  &w%d\n\r", council->head, council->members );
   else
      ch_printf( ch, "&cCo-Heads:     &w%s &cand &w%s\n\r&cMembers:  &w%d\n\r",
                 council->head, council->head2, council->members );
   ch_printf( ch, "&cDescription:\n\r&w%s\n\r", council->councildesc );
   return;
}

CMDF do_victories( CHAR_DATA * ch, char *argument )
{
   char filename[256];

   if( IS_NPC( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( ch->pcdata->clan->clan_type != CLAN_ORDER )
   {
      snprintf( filename, 256, "%s%s.record", CLAN_DIR, ch->pcdata->clan->name );
      if( !str_cmp( ch->name, ch->pcdata->clan->leader ) && !str_cmp( argument, "clean" ) )
      {
         FILE *fp = fopen( filename, "w" );
         if( fp )
            FCLOSE( fp );
         send_to_pager( "\n\rVictories ledger has been cleared.\n\r", ch );
         return;
      }
      else
      {
         send_to_pager( "\n\rLVL  Character       LVL  Character\n\r", ch );
         show_file( ch, filename );
         return;
      }
   }
   else
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }
}

CMDF do_ident( CHAR_DATA * ch, char *argument )
{
   CLAN_DATA *clan = NULL;
   OBJ_DATA *obj;
   CHAR_DATA *mob;
   AFFECT_DATA *paf;
   SKILLTYPE *sktmp;
   double idcost;
   bool idmob = FALSE;
   bool found = FALSE;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use this command.\n\r", ch );
      return;
   }

   for( mob = ch->in_room->first_person; mob; mob = mob->next_in_room )
      if( IS_NPC( mob ) && ( IS_ACT_FLAG( mob, ACT_IDMOB ) || IS_ACT_FLAG( mob, ACT_GUILDIDMOB ) ) )
      {
         idmob = TRUE;
         break;
      }

   if( !idmob )
   {
      send_to_char( "You must go to someone who will identify your items to use this command.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      act( AT_TELL, "$n tells you 'What would you like identified?'\n\r", mob, NULL, ch, TO_VICT );
      return;
   }

   if( ( obj = get_obj_carry( ch, argument ) ) == NULL )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", mob, NULL, ch, TO_VICT );
      return;
   }

   if( IS_ACT_FLAG( mob, ACT_GUILDIDMOB ) )
   {
      for( clan = first_clan; clan; clan = clan->next )
         if( mob->pIndexData->vnum == clan->idmob )
         {
            found = TRUE;
            break;
         }
   }

   if( found && clan == ch->pcdata->clan )
      ;
   else
   {
      idcost = 5000 + ( obj->cost * 0.1 );

      if( ch->gold - idcost < 0 )
      {
         act( AT_TELL, "$n tells you 'You cannot afford to identify that!'", mob, NULL, ch, TO_VICT );
         return;
      }
      act_printf( AT_TELL, mob, NULL, ch, TO_VICT, "$n charges you %d gold for the identification.", idcost );
      ch->gold -= ( int )idcost;
      if( found && clan->bank )
      {
         clan->balance += ( int )idcost;
         save_clan( clan );
      }
   }

   act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT, "$n tells you 'Information on a %s:'", obj->short_descr );
   if( ch->level >= LEVEL_IMMORTAL )
      act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT, "$n tells you 'The objects Vnum is %d'", obj->pIndexData->vnum );

   act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT, "$n tells you 'The object has the Keywords: %s'", obj->name );
   act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT, "$n tells you 'The object is of Type: %s'", item_type_name( obj ) );
   if( ch->level >= LEVEL_IMMORTAL )
      act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT,
                  "$n tells you 'The object has Wear Flags : %s'", wear_bit_name( obj->wear_flags ) );
   act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT,
               "$n tells you 'The object has Extra Flags: %s'", extra_bit_name( &obj->extra_flags ) );
   act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT,
               "$n tells you 'The object has the following Magical properties: %s'", magic_bit_name( obj->magic_flags ) );
   act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT,
               "$n tells you the object Weighs: %d, is Valued at: %d, and will cost you: %d in rent.'", obj->weight,
               obj->cost, obj->rent );

   switch ( obj->item_type )
   {
      case ITEM_CONTAINER:
         act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT,
                     "$n tells you 'The %s appears to be %s.'", capitalize( obj->short_descr ),
                     obj->value[0] < 76 ? "of a small capacity" :
                     obj->value[0] < 150 ? "of a small to medium capacity" :
                     obj->value[0] < 300 ? "of a medium capacity" :
                     obj->value[0] < 550 ? "of a medium to large capacity" :
                     obj->value[0] < 751 ? "of a large capacity" : "of a giant capacity" );
         break;

      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
         act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT,
                     "$n tells you that the object holds level %d spells of:", obj->value[0] );
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
         act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT,
                     "$n tells you that the %s has %d(%d) applications of level %d", obj->short_descr,
                     obj->value[1], obj->value[2], obj->value[0] );
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
         act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT,
                     "$n tells you that obect has %d(%d) charges of level %d", obj->value[1], obj->value[2], obj->value[0] );

         if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != NULL )
         {
            send_to_char( " '", ch );
            send_to_char( sktmp->name, ch );
            send_to_char( "'", ch );
         }

         send_to_char( ".\n\r", ch );
         break;

      case ITEM_WEAPON:
         act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT,
                     "$n tells you that the object does damage of %d to %d (average %d)%s",
                     obj->value[1], obj->value[2], ( obj->value[1] + obj->value[2] ) / 2,
                     IS_OBJ_FLAG( obj, ITEM_POISONED ) ? ", and is poisonous." : "." );
         break;

      case ITEM_ARMOR:
         act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT,
                     "$n tells you that this object's Base Armor Class is -%d.", obj->value[0] );
         break;
   }
   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      showaffect( ch, paf );

   for( paf = obj->first_affect; paf; paf = paf->next )
      showaffect( ch, paf );

   return;
}

CMDF do_shove( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   char arg2[MIL];
   int exit_dir;
   EXIT_DATA *pexit;
   CHAR_DATA *victim;
   bool nogo;
   ROOM_INDEX_DATA *to_room;
   int schance = 0;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( !IS_PCFLAG( ch, PCFLAG_DEADLY ) )
   {
      send_to_char( "Only deadly characters can shove.\n\r", ch );
      return;
   }

   if( arg[0] == '\0' )
   {
      send_to_char( "Shove whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You shove yourself around, to no avail.\n\r", ch );
      return;
   }

   if( !IS_PCFLAG( victim, PCFLAG_DEADLY ) )
   {
      send_to_char( "You can only shove deadly characters.\n\r", ch );
      return;
   }

   if( ( victim->position ) != POS_STANDING )
   {
      act( AT_PLAIN, "$N isn't standing up.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( arg2[0] == '\0' )
   {
      send_to_char( "Shove them in which direction?\n\r", ch );
      return;
   }

   exit_dir = get_dir( arg2 );
   if( IS_ROOM_FLAG( victim->in_room, ROOM_SAFE ) && get_timer( victim, TIMER_SHOVEDRAG ) <= 0 )
   {
      send_to_char( "That character cannot be shoved right now.\n\r", ch );
      return;
   }
   victim->position = POS_SHOVE;
   nogo = FALSE;
   if( ( pexit = get_exit( ch->in_room, exit_dir ) ) == NULL )
      nogo = TRUE;
   else
      if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && ( !IS_AFFECTED( ch, AFF_PASS_DOOR ) || IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) )
          && !IS_PCFLAG( ch, PCFLAG_PASSDOOR ) )
      nogo = TRUE;

   if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED )
         || IS_EXIT_FLAG( pexit, EX_HEAVY )
         || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT ) || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) )
      nogo = TRUE;

   if( nogo )
   {
      send_to_char( "There's no exit in that direction.\n\r", ch );
      victim->position = POS_STANDING;
      return;
   }
   to_room = pexit->to_room;
   if( IS_ROOM_FLAG( to_room, ROOM_DEATH ) )
   {
      send_to_char( "You cannot shove someone into a death trap.\n\r", ch );
      victim->position = POS_STANDING;
      return;
   }

   if( ch->in_room->area != to_room->area && !in_hard_range( victim, to_room->area ) )
   {
      send_to_char( "That character cannot enter that area.\n\r", ch );
      victim->position = POS_STANDING;
      return;
   }

   /*
    * Check for Class, assign percentage based on that. 
    */
   if( ch->Class == CLASS_WARRIOR )
      schance = 70;
   if( ch->Class == CLASS_RANGER )
      schance = 60;
   if( ch->Class == CLASS_DRUID )
      schance = 45;
   if( ch->Class == CLASS_CLERIC )
      schance = 35;
   if( ch->Class == CLASS_ROGUE )
      schance = 30;
   if( ch->Class == CLASS_MAGE )
      schance = 15;
   if( ch->Class == CLASS_MONK )
      schance = 80;
   if( ch->Class == CLASS_PALADIN )
      schance = 65;
   if( ch->Class == CLASS_ANTIPALADIN )
      schance = 65;
   if( ch->Class == CLASS_BARD )
      schance = 30;
   if( ch->Class == CLASS_NECROMANCER )
      schance = 15;

   /*
    * Add 3 points to chance for every str point above 15, subtract for below 15 
    */

   schance += ( ( get_curr_str( ch ) - 15 ) * 3 );

   schance += ( ch->level - victim->level );

   if( schance < number_percent(  ) )
   {
      send_to_char( "You failed.\n\r", ch );
      victim->position = POS_STANDING;
      return;
   }
   act( AT_ACTION, "You shove $M.", ch, NULL, victim, TO_CHAR );
   act( AT_ACTION, "$n shoves you.", ch, NULL, victim, TO_VICT );
   move_char( victim, get_exit( ch->in_room, exit_dir ), 0, exit_dir, FALSE );
   if( !char_died( victim ) )
      victim->position = POS_STANDING;
   WAIT_STATE( ch, 12 );
   /*
    * Remove protection from shove/drag if char shoves -- Blodkai 
    */
   if( IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) && get_timer( ch, TIMER_SHOVEDRAG ) <= 0 )
      add_timer( ch, TIMER_SHOVEDRAG, 10, NULL, 0 );
}

CMDF do_drag( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   char arg2[MIL];
   int exit_dir;
   CHAR_DATA *victim;
   EXIT_DATA *pexit;
   ROOM_INDEX_DATA *to_room;
   bool nogo;
   int dchance = 0;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Only characters can drag.\n\r", ch );
      return;
   }

   if( arg[0] == '\0' )
   {
      send_to_char( "Drag whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You take yourself by the scruff of your neck, but go nowhere.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "You can only drag characters.\n\r", ch );
      return;
   }

   if( !IS_PLR_FLAG( victim, PLR_SHOVEDRAG ) && !IS_PCFLAG( victim, PCFLAG_DEADLY ) )
   {
      send_to_char( "That character doesn't seem to appreciate your attentions.\n\r", ch );
      return;
   }

   if( victim->fighting )
   {
      send_to_char( "You try, but can't get close enough.\n\r", ch );
      return;
   }

   if( !IS_PCFLAG( ch, PCFLAG_DEADLY ) && IS_PCFLAG( victim, PCFLAG_DEADLY ) )
   {
      send_to_char( "You cannot drag a deadly character.\n\r", ch );
      return;
   }

   if( !IS_PCFLAG( victim, PCFLAG_DEADLY ) && victim->position > POS_STUNNED )
   {
      send_to_char( "They don't seem to need your assistance.\n\r", ch );
      return;
   }

   if( arg2[0] == '\0' )
   {
      send_to_char( "Drag them in which direction?\n\r", ch );
      return;
   }

   exit_dir = get_dir( arg2 );

   if( IS_ROOM_FLAG( victim->in_room, ROOM_SAFE ) && get_timer( victim, TIMER_SHOVEDRAG ) <= 0 )
   {
      send_to_char( "That character cannot be dragged right now.\n\r", ch );
      return;
   }

   nogo = FALSE;
   if( ( pexit = get_exit( ch->in_room, exit_dir ) ) == NULL )
      nogo = TRUE;
   else
      if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && ( !IS_AFFECTED( ch, AFF_PASS_DOOR ) || IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) )
          && !IS_PCFLAG( ch, PCFLAG_PASSDOOR ) )
      nogo = TRUE;

   if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED )
         || IS_EXIT_FLAG( pexit, EX_HEAVY )
         || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT ) || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) )
      nogo = TRUE;

   if( nogo )
   {
      send_to_char( "There's no exit in that direction.\n\r", ch );
      return;
   }

   to_room = pexit->to_room;
   if( IS_ROOM_FLAG( to_room, ROOM_DEATH ) )
   {
      send_to_char( "You cannot drag someone into a death trap.\n\r", ch );
      return;
   }

   if( ch->in_room->area != to_room->area && !in_hard_range( victim, to_room->area ) )
   {
      send_to_char( "That character cannot enter that area.\n\r", ch );
      victim->position = POS_STANDING;
      return;
   }

   /*
    * Check for Class, assign percentage based on that. 
    */
   if( ch->Class == CLASS_WARRIOR )
      dchance = 70;
   if( ch->Class == CLASS_RANGER )
      dchance = 60;
   if( ch->Class == CLASS_DRUID )
      dchance = 45;
   if( ch->Class == CLASS_CLERIC )
      dchance = 35;
   if( ch->Class == CLASS_ROGUE )
      dchance = 30;
   if( ch->Class == CLASS_MAGE )
      dchance = 15;
   if( ch->Class == CLASS_MONK )
      dchance = 80;
   if( ch->Class == CLASS_PALADIN )
      dchance = 65;
   if( ch->Class == CLASS_ANTIPALADIN )
      dchance = 65;
   if( ch->Class == CLASS_BARD )
      dchance = 30;
   if( ch->Class == CLASS_NECROMANCER )
      dchance = 15;

   /*
    * Add 3 points to chance for every str point above 15, subtract for below 15 
    */

   dchance += ( ( get_curr_str( ch ) - 15 ) * 3 );

   dchance += ( ch->level - victim->level );

   if( dchance < number_percent(  ) )
   {
      send_to_char( "You failed.\n\r", ch );
      victim->position = POS_STANDING;
      return;
   }
   if( victim->position < POS_STANDING )
   {
      short temp;

      temp = victim->position;
      victim->position = POS_DRAG;
      act( AT_ACTION, "You drag $M into the next room.", ch, NULL, victim, TO_CHAR );
      act( AT_ACTION, "$n grabs your hair and drags you.", ch, NULL, victim, TO_VICT );
      move_char( victim, get_exit( ch->in_room, exit_dir ), 0, exit_dir, FALSE );
      if( !char_died( victim ) )
         victim->position = temp;
      /*
       * Move ch to the room too.. they are doing dragging - Scryn 
       */
      move_char( ch, get_exit( ch->in_room, exit_dir ), 0, exit_dir, FALSE );
      WAIT_STATE( ch, 12 );
      return;
   }
   send_to_char( "You cannot do that to someone who is standing.\n\r", ch );
   return;
}
