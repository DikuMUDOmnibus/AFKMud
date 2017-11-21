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
 *                     Line Editor and String Functions                     *
 ****************************************************************************/

#include <stdarg.h>
#include <string.h>
#include "mud.h"
#include "editor.h"

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 *
 * Renamed so it can play itself system independent.
 * Samson 10-12-03
 */
size_t mudstrlcpy( char *dst, const char *src, size_t siz )
{
   register char *d = dst;
   register const char *s = src;
   register size_t n = siz;

   /*
    * Copy as many bytes as will fit 
    */
   if( n != 0 && --n != 0 )
   {
      do
      {
         if( ( *d++ = *s++ ) == 0 )
            break;
      }
      while( --n != 0 );
   }

   /*
    * Not enough room in dst, add NUL and traverse rest of src 
    */
   if( n == 0 )
   {
      if( siz != 0 )
         *d = '\0';  /* NUL-terminate dst */
      while( *s++ )
         ;
   }
   return ( s - src - 1 ); /* count does not include NUL */
}

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(initial dst) + strlen(src); if retval >= siz,
 * truncation occurred.
 *
 * Renamed so it can play itself system independent.
 * Samson 10-12-03
 */
size_t mudstrlcat( char *dst, const char *src, size_t siz )
{
   register char *d = dst;
   register const char *s = src;
   register size_t n = siz;
   size_t dlen;

   /*
    * Find the end of dst and adjust bytes left but don't go past end 
    */
   while( n-- != 0 && *d != '\0' )
      d++;
   dlen = d - dst;
   n = siz - dlen;

   if( n == 0 )
      return ( dlen + strlen( s ) );
   while( *s != '\0' )
   {
      if( n != 1 )
      {
         *d++ = *s;
         n--;
      }
      s++;
   }
   *d = '\0';
   return ( dlen + ( s - src ) );   /* count does not include NUL */
}

void stralloc_printf( char **pointer, char *fmt, ... )
{
   char buf[MSL * 4];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 4, fmt, args );
   va_end( args );

   STRFREE( *pointer );
   *pointer = STRALLOC( buf );
   return;
}

void strdup_printf( char **pointer, char *fmt, ... )
{
   char buf[MSL * 4];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 4, fmt, args );
   va_end( args );

   DISPOSE( *pointer );
   *pointer = str_dup( buf );
   return;
}

/*
 * custom str_dup using create - Thoric
 */
char *str_dup( const char *str )
{
   static char *ret;
   int len;

   if( !str )
      return NULL;

   len = strlen( str ) + 1;

   /*
    * Is this RIGHT?!? Or am I reading this WAY wrong? 
    */
   /*
    * ret = (char *)calloc( len, sizeof(char) ); 
    */
   CREATE( ret, char, len );
   mudstrlcpy( ret, str, len );
   return ret;
}

/*
 * Compare strings, case insensitive.
 * Return TRUE if different
 *   (compatibility with historical functions).
 */
bool str_cmp( const char *astr, const char *bstr )
{
   if( !astr )
   {
      bug( "%s", "Str_cmp: null astr." );
      if( bstr )
         bug( "str_cmp: astr: (null)  bstr: %s\n", bstr );
      return TRUE;
   }

   if( !bstr )
   {
      bug( "%s", "Str_cmp: null bstr." );
      if( astr )
         bug( "str_cmp: astr: %s  bstr: (null)\n", astr );
      return TRUE;
   }

   for( ; *astr || *bstr; astr++, bstr++ )
   {
      if( LOWER( *astr ) != LOWER( *bstr ) )
         return TRUE;
   }
   return FALSE;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix( const char *astr, const char *bstr )
{
   if( !astr )
   {
      bug( "%s", "Strn_cmp: null astr." );
      return TRUE;
   }

   if( !bstr )
   {
      bug( "%s", "Strn_cmp: null bstr." );
      return TRUE;
   }

   for( ; *astr; astr++, bstr++ )
   {
      if( LOWER( *astr ) != LOWER( *bstr ) )
         return TRUE;
   }
   return FALSE;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE if astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix( const char *astr, const char *bstr )
{
   int sstr1, sstr2, ichar;
   char c0;

   if( ( c0 = LOWER( astr[0] ) ) == '\0' )
      return FALSE;

   sstr1 = strlen( astr );
   sstr2 = strlen( bstr );

   for( ichar = 0; ichar <= sstr2 - sstr1; ichar++ )
      if( c0 == LOWER( bstr[ichar] ) && !str_prefix( astr, bstr + ichar ) )
         return FALSE;

   return TRUE;
}

/*
 * Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix( const char *astr, const char *bstr )
{
   int sstr1, sstr2;

   sstr1 = strlen( astr );
   sstr2 = strlen( bstr );
   if( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
      return FALSE;
   else
      return TRUE;
}

/*
 * Returns an initial-capped string.
 */
char *capitalize( const char *str )
{
   static char strcap[MSL];
   int i;

   for( i = 0; str[i] != '\0'; i++ )
      strcap[i] = LOWER( str[i] );
   strcap[i] = '\0';
   strcap[0] = UPPER( strcap[0] );
   return strcap;
}

/*
 * Returns a lowercase string.
 */
char *strlower( const char *str )
{
   static char strlow[MSL];
   int i;

   for( i = 0; str[i] != '\0'; i++ )
      strlow[i] = LOWER( str[i] );
   strlow[i] = '\0';
   return strlow;
}

/*
 * Returns an uppercase string.
 */
char *strupper( const char *str )
{
   static char strup[MSL];
   int i;

   for( i = 0; str[i] != '\0'; i++ )
      strup[i] = UPPER( str[i] );
   strup[i] = '\0';
   return strup;
}

/*
 * Returns TRUE or FALSE if a letter is a vowel			-Thoric
 */
bool isavowel( char letter )
{
   char c;

   c = LOWER( letter );
   if( c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' )
      return TRUE;
   else
      return FALSE;
}

/*
 * Shove either "a " or "an " onto the beginning of a string	-Thoric
 */
char *aoran( const char *str )
{
   static char temp[MSL];

   if( !str )
   {
      bug( "%s", "Aoran(): NULL str" );
      return "";
   }

   if( isavowel( str[0] ) || ( strlen( str ) > 1 && LOWER( str[0] ) == 'y' && !isavowel( str[1] ) ) )
      mudstrlcpy( temp, "an ", MSL );
   else
      mudstrlcpy( temp, "a ", MSL );
   mudstrlcat( temp, str, MSL );
   return temp;
}

/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde( char *str )
{
   for( ; *str != '\0'; str++ )
      if( *str == '~' )
         *str = '-';

   return;
}

/*
 * Encodes the tildes in a string.				-Thoric
 * Used for player-entered strings that go into disk files.
 */
void hide_tilde( char *str )
{
   for( ; *str != '\0'; str++ )
      if( *str == '~' )
         *str = HIDDEN_TILDE;

   return;
}

char *show_tilde( const char *str )
{
   static char buf[MSL];
   char *bufptr;

   bufptr = buf;
   for( ; *str != '\0'; str++, bufptr++ )
   {
      if( *str == HIDDEN_TILDE )
         *bufptr = '~';
      else
         *bufptr = *str;
   }
   *bufptr = '\0';
   return buf;
}

/*
   Original Code from SW:FotE 1.1
   Reworked strrep function. 
   Fixed a few glaring errors. It also will not overrun the bounds of a string.
   -- Xorith
*/
char *strrep( const char *src, const char *sch, const char *rep )
{
   int lensrc = strlen( src ), lensch = strlen( sch ), lenrep = strlen( rep ), x, y, in_p;
   static char newsrc[MSL];
   bool searching = FALSE;

   newsrc[0] = '\0';
   for( x = 0, in_p = 0; x < lensrc; x++, in_p++ )
   {
      if( src[x] == sch[0] )
      {
         searching = TRUE;
         for( y = 0; y < lensch; y++ )
            if( src[x + y] != sch[y] )
               searching = FALSE;

         if( searching )
         {
            for( y = 0; y < lenrep; y++, in_p++ )
            {
               if( in_p == ( MSL - 1 ) )
               {
                  newsrc[in_p] = '\0';
                  return newsrc;
               }
               newsrc[in_p] = rep[y];
            }
            x += lensch - 1;
            in_p--;
            searching = FALSE;
            continue;
         }
      }
      if( in_p == ( MSL - 1 ) )
      {
         newsrc[in_p] = '\0';
         return newsrc;
      }
      newsrc[in_p] = src[x];
   }
   newsrc[in_p] = '\0';
   return newsrc;
}

/* A rather dangerous function if passed with funky data - so just don't, ok? - Samson */
char *strrepa( const char *src, const char *sch[], const char *rep[] )
{
   static char newsrc[MSL];
   int x;

   mudstrlcpy( newsrc, src, MSL );
   for( x = 0; sch[x]; x++ )
      mudstrlcpy( newsrc, strrep( newsrc, sch[x], rep[x] ), MSL );

   return newsrc;
}

CMDF do_string( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], arg2[MIL];
   char *newstring;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   ch_printf( ch, "Old string: %s\n\r", argument );
   ch_printf( ch, "Replace   : %s\n\r", arg );
   ch_printf( ch, "With      : %s\n\r", arg2 );
   newstring = strrep( argument, arg, arg2 );
   ch_printf( ch, "Result    : %s\n\r", newstring );
   return;
}

void editor_print_info( CHAR_DATA * ch, EDITOR_DATA * edd, short max_size, char *argument )
{
   ch_printf( ch, "Currently editing: %s\n\r"
              "Total lines: %4d   On line:  %4d\n\r"
              "Buffer size: %4d   Max size: %4d\n\r",
              edd->desc ? edd->desc : "(Null description)", edd->numlines, edd->on_line, edd->size, max_size );
}

void set_editor_desc( CHAR_DATA * ch, char *new_desc )
{
   if( !ch || !ch->pcdata->editor )
      return;

   STRFREE( ch->pcdata->editor->desc );
   ch->pcdata->editor->desc = STRALLOC( new_desc );
}

void editor_desc_printf( CHAR_DATA * ch, char *desc_fmt, ... )
{
   char buf[MSL * 2];   /* umpf.. */
   va_list args;

   va_start( args, desc_fmt );
   vsnprintf( buf, MSL * 2, desc_fmt, args );
   va_end( args );

   set_editor_desc( ch, buf );
}

void stop_editing( CHAR_DATA * ch )
{
   DISPOSE( ch->pcdata->editor );
   send_to_char( "Done.\n\r", ch );
   ch->pcdata->dest_buf = NULL;
   ch->pcdata->spare_ptr = NULL;
   ch->substate = SUB_NONE;
   if( !ch->desc )
   {
      bug( "%s", "Fatal: stop_editing: no desc" );
      return;
   }
   ch->desc->connected = CON_PLAYING;
}

void start_editing( CHAR_DATA * ch, char *data )
{
   EDITOR_DATA *edit;
   short lines, size, lpos;
   char c;

   if( !ch->desc )
   {
      bug( "%s", "Fatal: start_editing: no desc" );
      return;
   }
   if( ch->substate == SUB_RESTRICTED )
      bug( "%s", "NOT GOOD: start_editing: ch->substate == SUB_RESTRICTED" );

   set_char_color( AT_GREEN, ch );
   send_to_char( "Begin entering your text now (/? = help /s = save /c = clear /l = list)\n\r", ch );
   send_to_char( "-----------------------------------------------------------------------\n\r> ", ch );
   if( ch->pcdata->editor )
      stop_editing( ch );

   CREATE( edit, EDITOR_DATA, 1 );
   edit->numlines = 0;
   edit->on_line = 0;
   edit->size = 0;
   size = 0;
   lpos = 0;
   lines = 0;
   if( !data )
      bug( "%s", "editor: data is NULL!" );
   else
      for( ;; )
      {
         c = data[size++];
         if( c == '\0' )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
         else if( c == '\r' );
         else if( c == '\n' || lpos > 79 )
         {
            edit->line[lines][lpos] = '\0';
            ++lines;
            lpos = 0;
         }
         else
            edit->line[lines][lpos++] = c;
         if( lines >= 49 || size > 4096 )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
      }

   if( lpos > 0 && lpos < 78 && lines < 49 )
   {
      edit->line[lines][lpos] = '~';
      edit->line[lines][lpos + 1] = '\0';
      ++lines;
      lpos = 0;
   }
   edit->numlines = lines;
   edit->size = size;
   edit->on_line = lines;
   ch->pcdata->editor = edit;
   ch->desc->connected = CON_EDITING;
}

char *copy_buffer( CHAR_DATA * ch )
{
   char buf[MSL];
   char tmp[100];
   short x, len;

   if( !ch )
   {
      bug( "%s", "copy_buffer: null ch" );
      return STRALLOC( "" );
   }

   if( !ch->pcdata->editor )
   {
      bug( "%s", "copy_buffer: null editor" );
      return STRALLOC( "" );
   }

   buf[0] = '\0';
   for( x = 0; x < ch->pcdata->editor->numlines; x++ )
   {
      mudstrlcpy( tmp, ch->pcdata->editor->line[x], 100 );
      len = strlen( tmp );
      if( tmp && tmp[len - 1] == '~' )
         tmp[len - 1] = '\0';
      else
         mudstrlcat( tmp, "\n\r", 100 );
      smash_tilde( tmp );
      mudstrlcat( buf, tmp, MSL );
   }
   return STRALLOC( buf );
}

char *copy_buffer_nohash( CHAR_DATA * ch )
{
   char buf[MSL];
   char tmp[100];
   short x, len;

   if( !ch )
   {
      bug( "%s", "copy_buffer_nohash: null ch" );
      return str_dup( "" );
   }

   if( !ch->pcdata->editor )
   {
      bug( "%s", "copy_buffer_nohash: null editor" );
      return str_dup( "" );
   }

   buf[0] = '\0';
   for( x = 0; x < ch->pcdata->editor->numlines; x++ )
   {
      mudstrlcpy( tmp, ch->pcdata->editor->line[x], 100 );
      len = strlen( tmp );
      if( tmp && tmp[len - 1] == '~' )
         tmp[len - 1] = '\0';
      else
         mudstrlcat( tmp, "\n\r", 100 );
      smash_tilde( tmp );
      mudstrlcat( buf, tmp, MSL );
   }
   return str_dup( buf );
}

/*
 * Simple but nice and handy line editor.			-Thoric
 */
void edit_buffer( CHAR_DATA * ch, char *argument )
{
   DESCRIPTOR_DATA *d;
   EDITOR_DATA *edit;
   char cmd[MIL];
   char buf[MIL];
   short x, line, max_buf_lines;
   bool save;

   if( ( d = ch->desc ) == NULL )
   {
      send_to_char( "You have no descriptor.\n\r", ch );
      return;
   }

   if( d->connected != CON_EDITING )
   {
      send_to_char( "You can't do that!\n\r", ch );
      bug( "%s", "Edit_buffer: d->connected != CON_EDITING" );
      return;
   }

   if( ch->substate <= SUB_PAUSE )
   {
      send_to_char( "You can't do that!\n\r", ch );
      bug( "Edit_buffer: illegal ch->substate (%d)", ch->substate );
      d->connected = CON_PLAYING;
      return;
   }

   if( !ch->pcdata->editor )
   {
      send_to_char( "You can't do that!\n\r", ch );
      bug( "%s", "Edit_buffer: null editor" );
      d->connected = CON_PLAYING;
      return;
   }

   edit = ch->pcdata->editor;
   save = FALSE;
   max_buf_lines = 50;

   if( ch->substate == SUB_MPROG_EDIT || ch->substate == SUB_HELP_EDIT )
      max_buf_lines = 60;

   if( argument[0] == '/' || argument[0] == '\\' )
   {
      one_argument( argument, cmd );
      if( !str_cmp( cmd + 1, "?" ) )
      {
         send_to_char( "Editing commands\n\r---------------------------------\n\r", ch );
         send_to_char( "/l              list buffer\n\r", ch );
         send_to_char( "/c              clear buffer\n\r", ch );
         send_to_char( "/d [line]       delete line\n\r", ch );
         send_to_char( "/g <line>       goto line\n\r", ch );
         send_to_char( "/i <line>       insert line\n\r", ch );
         send_to_char( "/f <format>     format text in buffer\n\r", ch );
         send_to_char( "/r <old> <new>  global replace\n\r", ch );
         send_to_char( "/a              abort editing\n\r", ch );
         send_to_char( "/p              show buffer information\n\r", ch );

         if( get_trust( ch ) > LEVEL_IMMORTAL )
            send_to_char( "/! <command>    execute command (do not use another editing command)\n\r", ch );
         send_to_char( "/s              save buffer\n\r\n\r> ", ch );
         return;
      }
      if( !str_cmp( cmd + 1, "c" ) )
      {
         memset( edit, '\0', sizeof( EDITOR_DATA ) );
         edit->numlines = 0;
         edit->on_line = 0;
         send_to_char( "Buffer cleared.\n\r> ", ch );
         return;
      }
      if( !str_cmp( cmd + 1, "r" ) )
      {
         char word1[MIL];
         char word2[MIL];
         char *sptr, *wptr, *lwptr;
         int count, wordln, word2ln, lineln;

         sptr = one_argument( argument, word1 );
         sptr = one_argument( sptr, word1 );
         sptr = one_argument( sptr, word2 );
         if( word1[0] == '\0' || word2[0] == '\0' )
         {
            send_to_char( "Need word to replace, and replacement.\n\r> ", ch );
            return;
         }
         /* Changed to a case-sensitive version of string compare --Cynshard */
         if( !strcmp( word1, word2 ) )
         {
            send_to_char( "Done.\n\r> ", ch );
            return;
         }
         count = 0;
         wordln = strlen( word1 );
         word2ln = strlen( word2 );
         ch_printf( ch, "Replacing all occurrences of %s with %s...\n\r", word1, word2 );
         for( x = 0; x < edit->numlines; x++ )
         {
            lwptr = edit->line[x];
            while( ( wptr = strstr( lwptr, word1 ) ) != NULL )
            {
               ++count;
               lineln = snprintf( buf, MIL, "%s%s", word2, wptr + wordln );
               if( lineln + wptr - edit->line[x] > 79 )
                  buf[lineln] = '\0';
               mudstrlcpy( wptr, buf, 81 - ( wptr - lwptr ) );
               lwptr = wptr + word2ln;
            }
         }
         ch_printf( ch, "Found and replaced %d occurrence(s).\n\r> ", count );
         return;
      }

      /*
       * added format command - shogar 
       */
      /*
       * This has been redone to be more efficient, and to make format
       * start at beginning of buffer, not whatever line you happened
       * to be on, at the time.   
       */

      if( !str_cmp( cmd + 1, "f" ) )
      {
         char temp_buf[MSL + max_buf_lines];
         int ep, old_p, end_mark;
         int p = 0;

         send_to_pager( "Reformating...\n\r", ch );

         for( x = 0; x < edit->numlines; x++ )
         {
            mudstrlcpy( temp_buf + p, edit->line[x], MSL + max_buf_lines - p );
            p += strlen( edit->line[x] );
            temp_buf[p] = ' ';
            p++;
         }

         temp_buf[p] = '\0';
         end_mark = p;
         p = 75;
         old_p = 0;
         edit->on_line = 0;
         edit->numlines = 0;

         while( old_p < end_mark )
         {
            while( temp_buf[p] != ' ' && p > old_p )
               p--;

            if( p == old_p )
               p += 75;

            if( p > end_mark )
               p = end_mark;

            ep = 0;
            for( x = old_p; x < p; x++ )
            {
               edit->line[edit->on_line][ep] = temp_buf[x];
               ep++;
            }
            edit->line[edit->on_line][ep] = '\0';

            edit->on_line++;
            edit->numlines++;

            old_p = p + 1;
            p += 75;

         }
         send_to_pager( "Reformating done.\n\r> ", ch );
         return;
      }

      if( !str_cmp( cmd + 1, "p" ) )
      {
         editor_print_info( ch, edit, max_buf_lines, argument );
         return;
      }

      if( !str_cmp( cmd + 1, "i" ) )
      {
         if( edit->numlines >= max_buf_lines )
            send_to_char( "Buffer is full.\n\r> ", ch );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument + 2 ) - 1;
            else
               line = edit->on_line;
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               send_to_char( "Out of range.\n\r> ", ch );
            else
            {
               for( x = ++edit->numlines; x > line; x-- )
                  mudstrlcpy( edit->line[x], edit->line[x - 1], 81 );
               mudstrlcpy( edit->line[line], "", 81 );
               send_to_char( "Line inserted.\n\r> ", ch );
            }
         }
         return;
      }

      if( !str_cmp( cmd + 1, "d" ) )
      {
         if( edit->numlines == 0 )
            send_to_char( "Buffer is empty.\n\r> ", ch );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument + 2 ) - 1;
            else
               line = edit->on_line;
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               send_to_char( "Out of range.\n\r> ", ch );
            else
            {
               if( line == 0 && edit->numlines == 1 )
               {
                  memset( edit, '\0', sizeof( EDITOR_DATA ) );
                  edit->numlines = 0;
                  edit->on_line = 0;
                  send_to_char( "Line deleted.\n\r> ", ch );
                  return;
               }
               for( x = line; x < ( edit->numlines - 1 ); x++ )
                  mudstrlcpy( edit->line[x], edit->line[x + 1], 81 );
               mudstrlcpy( edit->line[edit->numlines--], "", 81 );
               if( edit->on_line > edit->numlines )
                  edit->on_line = edit->numlines;
               send_to_char( "Line deleted.\n\r> ", ch );
            }
         }
         return;
      }
      if( !str_cmp( cmd + 1, "g" ) )
      {
         if( edit->numlines == 0 )
            send_to_char( "Buffer is empty.\n\r> ", ch );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument + 2 ) - 1;
            else
            {
               send_to_char( "Goto what line?\n\r> ", ch );
               return;
            }
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               send_to_char( "Out of range.\n\r> ", ch );
            else
            {
               edit->on_line = line;
               ch_printf( ch, "(On line %d)\n\r> ", line + 1 );
            }
         }
         return;
      }
      if( !str_cmp( cmd + 1, "l" ) )
      {
         if( edit->numlines == 0 )
            send_to_char( "Buffer is empty.\n\r> ", ch );
         else
         {
            send_to_char( "------------------\n\r", ch );
            for( x = 0; x < edit->numlines; x++ )
            {
               /*
                * Quixadhal - We cannot use ch_printf here, or we can't see
                * * what color codes exist in the strings!
                */
               char tmpline[MSL];

               snprintf( tmpline, MSL, "%2d> %s\n\r", x + 1, edit->line[x] );
               write_to_buffer( ch->desc, tmpline, 0 );
            }
            send_to_char( "------------------\n\r> ", ch );
         }
         return;
      }
      if( !str_cmp( cmd + 1, "a" ) )
      {
         if( !ch->last_cmd )
         {
            stop_editing( ch );
            return;
         }
         d->connected = CON_PLAYING;
         ch->substate = SUB_EDIT_ABORT;
         DISPOSE( ch->pcdata->editor );
         send_to_char( "Done.\n\r", ch );
         ch->pcdata->dest_buf = NULL;
         ch->pcdata->spare_ptr = NULL;
         ( *ch->last_cmd ) ( ch, "" );
         return;
      }
      if( get_trust( ch ) > LEVEL_IMMORTAL && !str_cmp( cmd + 1, "!" ) )
      {
         DO_FUN *last_cmd;
         int substate = ch->substate;

         last_cmd = ch->last_cmd;
         ch->substate = SUB_RESTRICTED;
         interpret( ch, argument + 3 );
         ch->substate = substate;
         ch->last_cmd = last_cmd;
         set_char_color( AT_GREEN, ch );
         send_to_char( "\n\r> ", ch );
         return;
      }
      if( !str_cmp( cmd + 1, "s" ) )
      {
         d->connected = CON_PLAYING;
         if( !ch->last_cmd )
            return;

         ( *ch->last_cmd ) ( ch, "" );
         return;
      }
   }

   if( edit->size + strlen( argument ) + 1 >= MSL - 1 )
      send_to_char( "You buffer is full.\n\r", ch );
   else
   {
      if( strlen( argument ) > 80 )
      {
         mudstrlcpy( buf, argument, 80 );
         send_to_char( "(Long line trimmed)\n\r> ", ch );
      }
      else
         mudstrlcpy( buf, argument, MIL );
      mudstrlcpy( edit->line[edit->on_line++], buf, 81 );
      if( edit->on_line > edit->numlines )
         edit->numlines++;
      if( edit->numlines > max_buf_lines )
      {
         edit->numlines = max_buf_lines;
         send_to_char( "Buffer full.\n\r", ch );
         save = TRUE;
      }
   }

   if( save )
   {
      d->connected = CON_PLAYING;
      if( !ch->last_cmd )
         return;
      ( *ch->last_cmd ) ( ch, "" );
      return;
   }
   send_to_char( "> ", ch );
}
