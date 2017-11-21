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
 *                           Help System Module                             *
 ****************************************************************************/

#include <string.h>
#include <ctype.h>
#include "mud.h"
#include "help.h"

HELP_DATA *first_help;
HELP_DATA *last_help;

int top_help;

void load_area_file( AREA_DATA * tarea, char *filename, bool isproto );

void free_help( HELP_DATA * pHelp )
{
   if( !fBootDb )
      UNLINK( pHelp, first_help, last_help, next, prev );
   DISPOSE( pHelp->text );
   DISPOSE( pHelp->keyword );
   DISPOSE( pHelp );
   return;
}

void free_helps( void )
{
   HELP_DATA *pHelp, *pHelp_next;

   for( pHelp = first_help; pHelp; pHelp = pHelp_next )
   {
      pHelp_next = pHelp->next;
      free_help( pHelp );
   }
   return;
}

/*
 * Adds a help page to the list if it is not a duplicate of an existing page.
 * Page is insert-sorted by keyword.			-Thoric
 * (The reason for sorting is to keep do_hlist looking nice)
 */
void add_help( HELP_DATA * pHelp )
{
   HELP_DATA *tHelp;
   int match;

   for( tHelp = first_help; tHelp; tHelp = tHelp->next )
      if( pHelp->level == tHelp->level && !str_cmp( pHelp->keyword, tHelp->keyword ) )
      {
         bug( "add_help: duplicate: %s.  Deleting.", pHelp->keyword );
         free_help( pHelp );
         return;
      }
      else if( ( match = strcmp( pHelp->keyword[0] == '\'' ? pHelp->keyword + 1 : pHelp->keyword,
                                 tHelp->keyword[0] == '\'' ? tHelp->keyword + 1 : tHelp->keyword ) ) < 0
               || ( match == 0 && pHelp->level > tHelp->level ) )
      {
         if( !tHelp->prev )
            first_help = pHelp;
         else
            tHelp->prev->next = pHelp;
         pHelp->prev = tHelp->prev;
         pHelp->next = tHelp;
         tHelp->prev = pHelp;
         break;
      }

   if( !tHelp )
      LINK( pHelp, first_help, last_help, next, prev );

   top_help++;
}

/*
 * Moved into a separate function so it can be used for other things
 * ie: online help editing				-Thoric
 */
HELP_DATA *get_help( CHAR_DATA * ch, char *argument )
{
   char argall[MIL], argone[MIL], argnew[MIL];
   HELP_DATA *pHelp;
   int lev;

   if( !argument || argument[0] == '\0' )
      argument = "summary";

   if( isdigit( argument[0] ) && !is_number( argument ) )
   {
      lev = number_argument( argument, argnew );
      argument = argnew;
   }
   else
      lev = -2;
   /*
    * Tricky argument handling so 'help a b' doesn't match a.
    */
   argall[0] = '\0';
   while( argument[0] != '\0' )
   {
      argument = one_argument( argument, argone );
      if( argall[0] != '\0' )
         mudstrlcat( argall, " ", MIL );
      mudstrlcat( argall, argone, MIL );
   }

   for( pHelp = first_help; pHelp; pHelp = pHelp->next )
   {
      if( pHelp->level > get_trust( ch ) )
         continue;
      if( lev != -2 && pHelp->level != lev )
         continue;

      if( is_name( argall, pHelp->keyword ) )
         return pHelp;
   }
   return NULL;
}

/*
 * Now this is cleaner
 */
/* Updated do_help command provided by Remcon of The Lands of Pabulum 03/20/2004 */
CMDF do_help( CHAR_DATA * ch, char *argument )
{
   HELP_DATA *pHelp;
   char *keyword;
   char arg[MIL], oneword[MSL], lastmatch[MSL];
   short matched = 0, checked = 0, totalmatched = 0, found = 0;
   bool uselevel = FALSE;
   int value = 0;

   set_pager_color( AT_HELP, ch );

   if( !argument || argument[0] == '\0' )
      argument = "summary";
   if( !( pHelp = get_help( ch, argument ) ) )
   {
      pager_printf( ch, "&wNo help on '%s' found.&D\n\r", argument );
      /* Get an arg incase they do a number seperate */
      one_argument( argument, arg );
      /* See if arg is a number if so update argument */
      if( is_number( arg ) )
      {
        argument = one_argument( argument, arg );
        if( argument && argument[0] != '\0' )
        {
            value = atoi( arg );
            uselevel = TRUE;
        }
        else /* If no more argument put arg as argument */
          argument = arg;
      }
      if( value > 0 )
         pager_printf( ch, "&CChecking for suggested helps that are level %d.\n\r", value );
      send_to_pager( "Suggested Help Files:\n\r", ch );
      mudstrlcpy( lastmatch, " ", MSL );
      for( pHelp = first_help; pHelp; pHelp = pHelp->next )
      {
         matched = 0;
         if( !pHelp || !pHelp->keyword || pHelp->keyword[0] == '\0' || pHelp->level > get_trust( ch ) )
            continue;
         /* Check arg if its avaliable */
         if( uselevel && pHelp->level != value )
           continue;
         keyword = pHelp->keyword;
         while( keyword && keyword[0] != '\0' )
         {
            matched = 0;   /* Set to 0 for each time we check lol */
            keyword = one_argument( keyword, oneword );
            /*
             * Lets check only up to 10 spots
             */
            for( checked = 0; checked <= 10; checked++ )
            {
               if( !oneword[checked] || !argument[checked] )
                  break;
               if( LOWER( oneword[checked] ) == LOWER( argument[checked] ) )
                  matched++;
            }
            if( ( matched > 1 && matched > ( checked / 2 ) ) || ( matched > 0 && checked < 2 ) )
            {
               pager_printf( ch, "&G %-20s &D", oneword );
               if( ++found % 4 == 0 )
               {
                  found = 0;
                  send_to_pager( "\n\r", ch );
               }
               mudstrlcpy( lastmatch, oneword, MSL );
               totalmatched++;
               break;
            }
         }
      }
      if( found != 0 )
         send_to_pager( "\n\r", ch );
      if( totalmatched == 0 )
      {
         send_to_pager( "&GNo suggested help files.\n\r", ch );
         return;
      }
      if( totalmatched == 1 && lastmatch != NULL && lastmatch && lastmatch[0] != '\0' && str_cmp(lastmatch, argument))
      {
         send_to_pager( "&COpening only suggested helpfile.&D\n\r", ch );
         do_help( ch, lastmatch );
         return;
      }
      return;
   }
   /*
    * Make newbies do a help start. --Shaddai
    */
   if( !IS_NPC( ch ) && !str_cmp( argument, "start" ) )
      SET_PCFLAG( ch, PCFLAG_HELPSTART );

   if( IS_IMMORTAL( ch ) )
      pager_printf( ch, "Help level: %d\n\r", pHelp->level );

   set_pager_color( AT_HELP, ch );

   /*
    * Strip leading '.' to allow initial blanks.
    */
   if( pHelp->text[0] == '.' )
      send_to_pager( pHelp->text + 1, ch );
   else
      send_to_pager( pHelp->text, ch );
   return;
}

/*
 * Help editor - Thoric
 */
CMDF do_hedit( CHAR_DATA * ch, char *argument )
{
   HELP_DATA *pHelp;

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor.\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_HELP_EDIT:
         if( !( pHelp = ( HELP_DATA * ) ch->pcdata->dest_buf ) )
         {
            bug( "%s", "hedit: sub_help_edit: NULL ch->pcdata->dest_buf" );
            stop_editing( ch );
            return;
         }
         DISPOSE( pHelp->text );
         pHelp->text = copy_buffer_nohash( ch );
         stop_editing( ch );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting helpfile.\n\r", ch );
         return;
   }
   if( !( pHelp = get_help( ch, argument ) ) )  /* new help */
   {
      HELP_DATA *tHelp;
      char argnew[MIL];
      int lev;
      bool new_help = TRUE;

      for( tHelp = first_help; tHelp; tHelp = tHelp->next )
         if( !str_cmp( argument, tHelp->keyword ) )
         {
            pHelp = tHelp;
            new_help = FALSE;
            break;
         }
      if( new_help )
      {
         if( isdigit( argument[0] ) )
         {
            lev = number_argument( argument, argnew );
            argument = argnew;
         }
         else
            lev = 0;
         CREATE( pHelp, HELP_DATA, 1 );
         pHelp->keyword = str_dup( strupper( argument ) );
         pHelp->level = lev;
         add_help( pHelp );
      }
   }
   ch->substate = SUB_HELP_EDIT;
   ch->pcdata->dest_buf = pHelp;
   if( !pHelp->text || pHelp->text[0] == '\0' )
      pHelp->text = str_dup( "" );
   start_editing( ch, pHelp->text );
   editor_desc_printf( ch, "Help topic, keyword '%s', level %d.", pHelp->keyword, pHelp->level );
}

/*
 * Stupid leading space muncher fix				-Thoric
 */
char *help_fix( char *text )
{
   char *fixed;

   if( !text )
      return "";
   fixed = strip_cr( text );
   if( fixed[0] == ' ' )
      fixed[0] = '.';
   return fixed;
}

CMDF do_hset( CHAR_DATA * ch, char *argument )
{
   HELP_DATA *pHelp;
   char arg1[MIL], arg2[MIL];

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Syntax: hset <field> [value] [help page]\n\r", ch );
      send_to_char( "\n\r", ch );
      send_to_char( "Field being one of:\n\r", ch );
      ch_printf( ch, "  level keyword remove save%s\n\r", IS_IMP( ch ) ? " reload" : "" );
      return;
   }

   if( !str_cmp( arg1, "reload" ) && IS_IMP( ch ) )
   {
      log_string( "Unloading existing help files." );
      free_helps(  );
      log_string( "Reloading help files." );
      load_area_file( last_area, "help.are", FALSE );
      send_to_char( "Help files reloaded.\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      FILE *fpout;

      log_string_plus( "Saving help.are...", LOG_NORMAL, LEVEL_GREATER );

      rename( "help.are", "help.are.bak" );
      if( ( fpout = fopen( "help.are", "w" ) ) == NULL )
      {
         bug( "%s", "hset save: fopen" );
         perror( "help.are" );
         return;
      }

      fprintf( fpout, "%s", "#HELPS\n\n" );
      for( pHelp = first_help; pHelp; pHelp = pHelp->next )
         fprintf( fpout, "%d %s~\n%s~\n\n", pHelp->level, pHelp->keyword, help_fix( pHelp->text ) );

      fprintf( fpout, "%s", "0 $~\n\n\n#$\n" );
      FCLOSE( fpout );
      send_to_char( "Saved.\n\r", ch );
      return;
   }

   if( str_cmp( arg1, "remove" ) )
      argument = one_argument( argument, arg2 );

   if( !( pHelp = get_help( ch, argument ) ) )
   {
      send_to_char( "Cannot find help on that subject.\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "remove" ) )
   {
      free_help( pHelp );
      send_to_char( "Removed.\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "level" ) )
   {
      int lev;

      if( !is_number(arg2) )
      {
         send_to_char( "Level field must be numeric.\r\n", ch );
         return;
      }

      lev = atoi(arg2);
      if( lev < -1 || lev > get_trust(ch) )
      {
         send_to_char( "You can't set the level to that.\r\n", ch );
         return;
      }
      pHelp->level = lev;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg1, "keyword" ) )
   {
      DISPOSE( pHelp->keyword );
      pHelp->keyword = str_dup( strupper( arg2 ) );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   do_hset( ch, "" );
}

/*
 * Show help topics in a level range				-Thoric
 * Idea suggested by Gorog
 * prefix keyword indexing added by Fireblade
 */
CMDF do_hlist( CHAR_DATA * ch, char *argument )
{
   int min, max, minlimit, maxlimit, cnt;
   char arg[MIL];
   HELP_DATA *help;
   bool minfound, maxfound;
   char *idx;

   maxlimit = get_trust( ch );
   minlimit = maxlimit >= LEVEL_GREATER ? -1 : 0;

   min = minlimit;
   max = maxlimit;

   idx = NULL;
   minfound = FALSE;
   maxfound = FALSE;

   for( argument = one_argument( argument, arg ); arg[0] != '\0'; argument = one_argument( argument, arg ) )
   {
      if( !isdigit( arg[0] ) )
      {
         if( idx )
         {
            send_to_char( "You may only use a single keyword to index the list.\n\r", ch );
            return;
         }
         idx = STRALLOC( arg );
      }
      else
      {
         if( !minfound )
         {
            min = URANGE( minlimit, atoi( arg ), maxlimit );
            minfound = TRUE;
         }
         else if( !maxfound )
         {
            max = URANGE( minlimit, atoi( arg ), maxlimit );
            maxfound = TRUE;
         }
         else
         {
            send_to_char( "You may only use two level limits.\n\r", ch );
            return;
         }
      }
   }

   if( min > max )
   {
      int temp = min;

      min = max;
      max = temp;
   }

   set_pager_color( AT_GREEN, ch );
   pager_printf( ch, "Help Topics in level range %d to %d:\n\r\n\r", min, max );
   for( cnt = 0, help = first_help; help; help = help->next )
      if( help->level >= min && help->level <= max && ( !idx || nifty_is_name_prefix( idx, help->keyword ) ) )
      {
         pager_printf( ch, "  %3d %s\n\r", help->level, help->keyword );
         ++cnt;
      }
   if( cnt )
      pager_printf( ch, "\n\r%d pages found.\n\r", cnt );
   else
      send_to_char( "None found.\n\r", ch );

   STRFREE( idx );
   return;
}

/* 
 * Title : Help Check Plus v1.0
 * Author: Chris Coulter (aka Gabriel Androctus)
 * Email : krisco7@bigfoot.com
 * Mud   : Perils of Quiernin (perils.wolfpaw.net 6000)
 * Descr.: A ridiculously simple routine that runs through the command and skill tables
 *         checking for help entries of the same name. If not found, it outputs a line
 *         to the pager. Priceless tool for finding those pesky missing help entries.
*/
CMDF do_helpcheck( CHAR_DATA * ch, char *argument )
{
   CMDTYPE *command;
   HELP_DATA *help;
   int hash, sn;
   int total = 0;
   bool fSkills = FALSE;
   bool fCmds = FALSE;

   if( argument[0] == '\0' )
   {
      set_pager_color( AT_YELLOW, ch );
      send_to_pager( "Syntax: helpcheck [ skills | commands | all ]\n\r", ch );
      return;
   }

   /*
    * check arguments and set appropriate switches 
    */
   if( !str_cmp( argument, "skills" ) )
      fSkills = TRUE;
   if( !str_cmp( argument, "commands" ) )
      fCmds = TRUE;
   if( !str_cmp( argument, "all" ) )
   {
      fSkills = TRUE;
      fCmds = TRUE;
   }

   if( fCmds ) /* run through command table */
   {
      send_to_pager( "&CMissing Commands Helps\n\r\n\r", ch );
      for( hash = 0; hash < 126; hash++ )
         for( command = command_hash[hash]; command; command = command->next )
         {
            /*
             * No entry, or command is above person's level 
             */
            if( !( help = get_help( ch, command->name ) ) && command->level <= ch->level )
            {
               pager_printf( ch, "&cNot found: &C%s&w\n\r", command->name );
               total++;
            }
            else
               continue;
         }
   }

   if( fSkills )  /* run through skill table */
   {
      send_to_pager( "\n\r&CMissing Skill/Spell Helps\n\r\n\r", ch );
      for( sn = 0; sn < top_sn; sn++ )
      {
         if( !( help = get_help( ch, skill_table[sn]->name ) ) )  /* no help entry */
         {
            pager_printf( ch, "&gNot found: &G%s&w\n\r", skill_table[sn]->name );
            total++;
         }
         else
            continue;
      }
   }

   /*
    * tally up the total number of missing entries and finish up 
    */
   pager_printf( ch, "\n\r&Y%d missing entries found.&w\n\r", total );
   return;
}
