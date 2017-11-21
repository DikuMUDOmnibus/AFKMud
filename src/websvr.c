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
 *                            Webserver Module                              *
 ****************************************************************************/

/* ROM 2.4 Integrated Web Server - Version 1.0
 *
 * This is my first major snippet... Please be kind. ;-)
 * Copyright 1998 -- Defiant -- Rob Siemborski -- mud@towers.crusoe.net
 *
 * Many thanks to Russ and the rest of the developers of ROM for creating
 * such an excellent codebase to program on.
 *
 * If you use this code on your mud, I simply ask that you place my name
 * someplace in the credits.  You can put it where you feel it is
 * appropriate.
 *
 * I offer no guarantee that this will work on any mud except my own, and
 * if you can't get it to work, please don't bother me.  I wrote and tested
 * this only on a Linux 2.0.30 system.  Comments about bugs, are, however,
 * appreciated.
 *
 * Now... On to the installation!
 */

/*
 * Insanity v0.9a pre-release Modifications
 * By Chris Fewtrell (Trax) <C.J.Fewtrell@bcs.org.uk>
 *
 * - Added functionailiy for Secure Web server pages, using standard HTTP
 *   Basic authentication, comparing with pass list generated with command
 *   from within the MUD itself. 
 * - Started work on web interface to help files, allowing them to be browsed
 *   from a web browser rather than being in MUD to read them.
 * - Seperated out the HTTP codes and content type to seperate functions
 *   (intending to allow more than HTML to be served via this)
 * - Adjusted the descriptor handling to prevent anyone from prematurely
 *   stopping a transfer causing a fd exception and the system to exit()
 * - Created a sorta "virtual" web directory for the webserver files to be
 *   actually served. This contains the usual images dir if any images are
 *   needed to be served from a central repository rather than generated.
 *   Be warned though! It WON'T follow any symlinks, I'll add that later
 *   with the stat function.. (maybe :) 
 *
 * Future Possbile additions:
 * - Access to general boards though web interface, prolly prevent posting but
 *   being able to browse and read notes to 'all' would be allowed
 */

/*
 * Additional Modifications based upon with with Insanity Codebase
 * By Chris Fewtrell (Trax) <C.J.Fewtrell@bcs.org.uk>
 *
 * - Web server root directory created, all URLs that use internal code
 *   generated HTML intercept first, then path is handed off to webroot
 *   handler allowing a directory tree to be built up there. Should allow
 *   easier management of the file behind the internal web server.
 */

#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#if defined(__CYGWIN__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include <cstdarg>
#include <list>
#include "mud.h"
#include "help.h"
#ifdef I3
#include "i3.h"
#endif
#ifdef IMC
#include "imc.h"
#endif
#include "web.h"

using namespace std;

CMDF do_who( CHAR_DATA * ch, char *argument );
char *in_dns_cache( char *ip );
#ifdef IMC
void web_imc_list( WEB_DESCRIPTOR * wdesc );
#endif

#define WEB_ROOMS "../public_html/"

/*
 * Content type stuff
 * This should let us use multiple filetypes
 * behind the server (graphics, html, text etc..)
 * all based on suffix matching   
 */
#define CONTENT_HTML    1
#define CONTENT_TEXT    2
#define CONTENT_GIF     3
#define CONTENT_JPEG    4
#define CONTENT_GZIP    5
#define CONTENT_WAV     6
#define CONTENT_VRML    7
#define CONTENT_CLASS   8

struct type_data
{
   char *suffix;
   int type;
};

struct type_data content_types[] = {
   {".html", CONTENT_HTML},
   {".htm", CONTENT_HTML},
   {".gif", CONTENT_GIF},
   {".txt", CONTENT_TEXT},
   {".text", CONTENT_TEXT},
   {".jpg", CONTENT_JPEG},
   {".jpeg", CONTENT_JPEG},
   {".gz", CONTENT_GZIP},
   {".gzip", CONTENT_GZIP},
   {".wav", CONTENT_WAV},
   {".wrl", CONTENT_VRML},
   {".class", CONTENT_CLASS},

   {"", CONTENT_TEXT}
};

/* The mark of the end of a HTTP/1.x request */
const char ENDREQUEST[5] = { 13, 10, 13, 10, 0 };  /* (CRLFCRLF) */

/* Locals */
list < WEB_DESCRIPTOR * >weblist;
int top_web_desc;
int web_socket;

/* Generic Utility Function */
/* Whoever the goober was who originally made this an int return.... WHY? YOU NEVER USED IT! */
void send_buf( int fd, const char *buf )
{
   send( fd, buf, strlen( buf ), 0 );
   return;
}

void web_printf( int fd, const char *buf, ... )
{
   char wbuf[MSL * 2];
   va_list args;

   va_start( args, buf );
   vsnprintf( wbuf, MSL * 2, buf, args );
   va_end( args );

   send_buf( fd, wbuf );
}

void shutdown_web( bool hotboot )
{
   list < WEB_DESCRIPTOR * >::iterator curr;

   /*
    * Close All Current Connections 
    */
   for( curr = weblist.begin(  ); curr != weblist.end(  ); )
   {
      WEB_DESCRIPTOR *current = ( *curr );
      curr++;

      close( current->fd );
      weblist.remove( current );
      delete current;
   }

   if( hotboot )
      return;

   /*
    * Stop Listening 
    */
   log_string( "Closing webserver..." );
   close( web_socket );
   web_socket = -1;
   sysdata.webrunning = false;
}

bool init_web( int webport, int wsocket, bool hotboot )
{
   struct sockaddr_in my_addr;
   int x = 1;

   weblist.clear(  );
   top_web_desc = 0;
   web_socket = wsocket;

   log_printf( "Attaching Internal Web Server to Port %d", webport );

   if( !hotboot )
   {
      if( ( web_socket = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
      {
         log_string( "----> Web Server: Error getting socket" );
         perror( "web-socket" );
         return false;
      }

      if( setsockopt( web_socket, SOL_SOCKET, SO_REUSEADDR, ( void * )&x, sizeof( x ) ) < 0 )
      {
         perror( "init_web: SO_REUSEADDR" );
         close( web_socket );
         web_socket = -1;
         return false;
      }

#if defined(SO_LINGER) && !defined(SYSV)
      {
         struct linger ld;

         ld.l_onoff = 1;
         ld.l_linger = 1000;

         if( setsockopt( web_socket, SOL_SOCKET, SO_LINGER, ( void * )&ld, sizeof( ld ) ) < 0 )
         {
            perror( "Init_web: SO_LINGER" );
            close( web_socket );
            web_socket = -1;
            return false;
         }
      }
#endif

      memset( &my_addr, '\0', sizeof( my_addr ) );
      my_addr.sin_family = AF_INET;
      my_addr.sin_port = htons( webport );

      if( ( bind( web_socket, ( struct sockaddr * )&my_addr, sizeof( my_addr ) ) ) == -1 )
      {
         log_string( "----> Web Server: Error binding socket" );
         perror( "web-bind" );
         close( web_socket );
         web_socket = -1;
         return false;
      }

      /*
       * Only listen for 5 connects at once, do we really need more? 
       */
      /*
       * We might now since I've attached this to the live web page - Samson 
       */
      if( listen( web_socket, 20 ) < 0 )
      {
         perror( "web-listen" );
         close( web_socket );
         web_socket = -1;
         return false;
      }
   }
   return true;
}

CMDF do_web( CHAR_DATA * ch, char *argument )
{
   list < WEB_DESCRIPTOR * >::iterator current;

   if( IS_NPC( ch ) )
   {
      send_to_char( "No way. Mobs can't do that.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "start" ) )
   {
      if( sysdata.webrunning )
      {
         send_to_char( "The webserver is already running!\n\r", ch );
         return;
      }
      else
      {
         if( !init_web( port + 1, -1, false ) ) /* mud_port is a global variable, remember? */
            send_to_char( "&RUnable to start web server.\n\r", ch );
         else
            sysdata.webrunning = true;
      }
      return;
   }

   if( !str_cmp( argument, "stop" ) )
   {
      if( !sysdata.webrunning )
      {
         send_to_char( "&YThe webserver isn't running!\n\r", ch );
         return;
      }
      else
      {
         shutdown_web( false );
         send_to_char( "&RWebserver has been shut down.\n\r", ch );
      }
      return;
   }

   if( !str_cmp( argument, "sockets" ) )
   {
      int count = 0;
      if( !sysdata.webrunning )
      {
         send_to_char( "&YThe webserver isn't running!\n\r", ch );
         return;
      }

      for( current = weblist.begin(  ); current != weblist.end(  ); current++ )
      {
         count++;
         ch_printf( ch, "Web descriptor: %d, Originating IP: %s\n\r", ( *current )->fd,
                    inet_ntoa( ( *current )->their_addr.sin_addr ) );
      }

      ch_printf( ch, "Total Web descriptors: %d\n\r", count );
      return;
   }

   send_to_char( "Usage: web start\n\r", ch );
   send_to_char( "Usage: web stop\n\r", ch );
   send_to_char( "Usage: web sockets\n\r", ch );
   return;
}

struct timeval ZERO_TIME = { 0, 0 };

int web_colour( char type, char *string, bool & firsttag )
{
   char code[50];
   char *p = '\0';
   bool validcolor = false;

   switch ( type )
   {
      default:
/*	    mudstrlcpy( code, "", 50 ); */
         break;
      case '&':
         mudstrlcpy( code, "&amp;", 50 );
         break;
      case 'x':
         mudstrlcpy( code, "<span class=\"black\">", 50 );
         validcolor = true;
         break;
      case 'b':
         mudstrlcpy( code, "<span class=\"dblue\">", 50 );
         validcolor = true;
         break;
      case 'c':
         mudstrlcpy( code, "<span class=\"cyan\">", 50 );
         validcolor = true;
         break;
      case 'g':
         mudstrlcpy( code, "<span class=\"dgreen\">", 50 );
         validcolor = true;
         break;
      case 'r':
         mudstrlcpy( code, "<span class=\"dred\">", 50 );
         validcolor = true;
         break;
      case 'w':
         mudstrlcpy( code, "<span class=\"grey\">", 50 );
         validcolor = true;
         break;
      case 'y':
         mudstrlcpy( code, "<span class=\"yellow\">", 50 );
         validcolor = true;
         break;
      case 'Y':
         mudstrlcpy( code, "<span class=\"yellow\">", 50 );
         validcolor = true;
         break;
      case 'B':
         mudstrlcpy( code, "<span class=\"blue\">", 50 );
         validcolor = true;
         break;
      case 'C':
         mudstrlcpy( code, "<span class=\"lblue\">", 50 );
         validcolor = true;
         break;
      case 'G':
         mudstrlcpy( code, "<span class=\"green\">", 50 );
         validcolor = true;
         break;
      case 'R':
         mudstrlcpy( code, "<span class=\"red\">", 50 );
         validcolor = true;
         break;
      case 'W':
         mudstrlcpy( code, "<span class=\"white\">", 50 );
         validcolor = true;
         break;
      case 'z':
         mudstrlcpy( code, "<span class=\"dgrey\">", 50 );
         validcolor = true;
         break;
      case 'o':
         mudstrlcpy( code, "<span class=\"yellow\">", 50 );
         validcolor = true;
         break;
      case 'O':
         mudstrlcpy( code, "<span class=\"orange\">", 50 );
         validcolor = true;
         break;
      case 'p':
         mudstrlcpy( code, "<span class=\"purple\">", 50 );
         validcolor = true;
         break;
      case 'P':
         mudstrlcpy( code, "<span class=\"pink\">", 50 );
         validcolor = true;
         break;
      case '<':
         mudstrlcpy( code, "&lt;", 50 );
         break;
      case '>':
         mudstrlcpy( code, "&gt;", 50 );
         break;
      case '/':
         mudstrlcpy( code, "<br />", 50 );
         break;
      case '{':
         snprintf( code, 50, "%c", '{' );
         break;
      case '-':
         snprintf( code, 50, "%c", '~' );
         break;
   }

   if( !firsttag && validcolor )
   {
      char newcode[50];

      snprintf( newcode, 50, "</span>%s", code );
      mudstrlcpy( code, newcode, 50 );
   }

   if( firsttag && validcolor )
      firsttag = false;

   p = code;
   while( *p != '\0' )
   {
      *string = *p++;
      *++string = '\0';
   }

   return ( strlen( code ) );
}

void web_colourconv( char *buffer, const char *txt )
{
   const char *point;
   int skip = 0;
   bool firsttag = true;

   if( txt )
   {
      for( point = txt; *point; point++ )
      {
         if( *point == '&' )
         {
            point++;
            skip = web_colour( *point, buffer, firsttag );
            while( skip-- > 0 )
               ++buffer;
            continue;
         }
         *buffer = *point;
         *++buffer = '\0';
      }
      *buffer = '\0';
   }
   if( !firsttag )
      mudstrlcat( buffer, "</span>", MSL );
   return;
}

/*
 *	This was added because of webmasters complaining on how they don't
 *	know how to code.  So web.h was added as well as extra directories
 *	in ../web (public_html and staff_html).     [readme.txt in ../web]
 *
 *	The file:  *.tih means 'Telnet Interface Header' (for beginning
 *	the html files) and *.tif 'Telnet Interface Footer' (for ending
 *	the html files).  The middle is filled in with generated code.
 *	
 *	-- Christopher Aaron Haslage (Yakkov) -- 6/3/99 (No Help Needed)
 */
/* Hey, what's wrong with being a Webmaster and not knowing how to code?
 * Hell. I'm a coder and I don't know HTML for squat, so I imagine the reverse
 * is also true :) - Samson
 */
void show_web_file( char *filename, WEB_DESCRIPTOR * wdesc )
{
   char buf[MSL];
   FILE *fp;
   int num = 0;
   char let;

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      while( !feof( fp ) )
      {
         while( ( let = fgetc( fp ) ) != EOF && num < ( MSL - 2 ) )
         {
            if( let != '\r' )
               buf[num++] = let;
         }

      }
      buf[num] = '\0';
      FCLOSE( fp );
   }
   else
      mudstrlcpy( buf, "\n\r<p><font color=\"red\">\n"
                  "ERROR: Missing or corrupted file in the Web Interface!\n" "</font></p><font color=\"white\">\n\r", MSL );

   send_buf( wdesc->fd, buf );
}

void send_200OK( WEB_DESCRIPTOR * wdesc )
{
   send_buf( wdesc->fd, "HTTP/1.1 200 OK\n" );
}

void send_404UNFOUND( WEB_DESCRIPTOR * wdesc )
{
   send_buf( wdesc->fd, "HTTP/1.1 404 Not Found\n" );
}

void send_content( WEB_DESCRIPTOR * wdesc, int type )
{
   switch ( type )
   {
      case CONTENT_HTML:
         send_buf( wdesc->fd, "Content-type: text/html\n\n" );
         break;
      default:
      case CONTENT_TEXT:
         send_buf( wdesc->fd, "Content-type: text/plain\n\n" );
         break;
      case CONTENT_GIF:
         send_buf( wdesc->fd, "Content-type: image/gif\n\n" );
         break;
      case CONTENT_WAV:
         send_buf( wdesc->fd, "Content-type: audio/x-wav\n\n" );
         break;
      case CONTENT_GZIP:
         send_buf( wdesc->fd, "Content-type: application/x-zip-compressed\n\n" );
         break;
      case CONTENT_VRML:
         send_buf( wdesc->fd, "Content-type: x-world/x-vrml\n\n" );
         break;
      case CONTENT_CLASS:
         send_buf( wdesc->fd, "Content-type: application/octet-stream\n\n" );
         break;
   }
}

/* point 1 */
void handle_web_main( WEB_DESCRIPTOR * wdesc )
{
   send_200OK( wdesc );
   send_content( wdesc, CONTENT_HTML );

   show_web_file( PUB_INDEX, wdesc );
}

/* point 3 */
void handle_web_wizlist( WEB_DESCRIPTOR * wdesc, bool choice )
{
   char buf[MSL];
   char colbuf[2 * MSL];
   FILE *fp;
   int num = 0;
   char let;

   send_200OK( wdesc );
   send_content( wdesc, CONTENT_HTML );

   show_web_file( PUB_WIZLIST_H, wdesc );

   if( ( fp = fopen( WEBWIZ_FILE, "r" ) ) != NULL )
   {
      while( !feof( fp ) )
      {
         while( ( let = fgetc( fp ) ) != EOF && num < ( MSL - 2 ) )
         {
            if( let != '\r' )
               buf[num++] = let;
         }
      }
      buf[num] = '\0';
      FCLOSE( fp );
   }
   else
      mudstrlcpy( buf, "Error opening Wizlist file<br />\n\r", MSL );

   web_colourconv( colbuf, buf );

   send_buf( wdesc->fd, colbuf );

   if( choice )
      show_web_file( PUB_WIZLIST2_F, wdesc );
   else
      show_web_file( PUB_WIZLIST_F, wdesc );
}

void handle_who_routine( WEB_DESCRIPTOR * wdesc )
{
   FILE *fp;
   char buf[MSL], col_buf[MSL];
   int c;
   int num = 0;

   if( ( fp = fopen( WEBWHO_FILE, "r" ) ) != NULL )
   {
      while( !feof( fp ) )
      {
         while( ( buf[num] = fgetc( fp ) ) != EOF && buf[num] != '\n' && buf[num] != '\r' && num < ( MSL - 2 ) )
            num++;
         c = fgetc( fp );
         if( ( c != '\n' && c != '\r' ) || c == buf[num] )
            ungetc( c, fp );
         buf[num++] = '\n';
         buf[num] = '\0';

         if( strlen( buf ) > 32000 )
         {
            bug( "%s", "handle_who_routine: Strlen Greater then 32000" );
            buf[32000] = '\0';
         }
         num = 0;
         web_colourconv( col_buf, buf );
         send_buf( wdesc->fd, col_buf );
      }
      FCLOSE( fp );
   }
   return;
}

/* point 4 */
void handle_web_who_request( WEB_DESCRIPTOR * wdesc, bool choice )
{
   send_200OK( wdesc );
   send_content( wdesc, CONTENT_HTML );

   show_web_file( PUB_WHOLIST_H, wdesc );

   do_who( NULL, "www" );
   handle_who_routine( wdesc );

   if( choice )
      show_web_file( PUB_WHOLIST2_F, wdesc );
   else
      show_web_file( PUB_WHOLIST_F, wdesc );
}

#ifdef IMC
/* Added: 21 April 2001 (Trax)
 * Function to handle imc web request (the imclist output)
 * Will also handle if IMC is not present in the codebase (through #define) */
void handle_web_imc( WEB_DESCRIPTOR * wdesc, char *path, bool choice )
{
   send_200OK( wdesc );
   send_content( wdesc, CONTENT_HTML );
   show_web_file( PUB_IMCLIST_H, wdesc );
   web_imc_list( wdesc );
   if( choice )
      show_web_file( PUB_IMCLIST2_F, wdesc );
   else
      show_web_file( PUB_IMCLIST_F, wdesc );
   return;
}
#endif

#ifdef I3
/*
 * Code to display the web based list of I3 muds
 * This isn't pretty and I'm not proud of it, I just want to get it working
 * first. Again a ripoff of I3_mudlist pulled apart a little
 */
void web_I3_mudlist( WEB_DESCRIPTOR * wdesc )
{
   I3_MUD *mud;
   int mudcount = 0;
   bool all = false;

   if( first_mud == NULL )
   {
      send_buf( wdesc->fd, "There are no muds to list!?" );
      return;
   }

   for( mud = first_mud; mud; mud = mud->next )
   {
      if( mud == NULL )
         continue;

      if( mud->name == NULL )
         continue;

      if( !all && mud->status == 0 )
         continue;

      mudcount++;

      switch ( mud->status )
      {
         case -1:
            web_printf( wdesc->fd,
                        "<tr><td><font color=\"red\">%s</font></td><td><font color=\"yellow\">%s</font></td><td><font color=\"green\">%s</font></td><td><font color=\"blue\">%s</font></td><td><font color=\"purple\">%d</font></td></tr>",
                        mud->name, mud->mud_type, mud->mudlib, mud->ipaddress, mud->player_port );
            break;
         case 0:
            web_printf( wdesc->fd, "<tr><td colspan=\"5\">%s (down)</td></tr>\n\r", mud->name );
            break;
         default:
            web_printf( wdesc->fd, "<tr><td colspan=\"5\">%s (rebooting, back in %d seconds)</td></tr>", mud->name,
                        mud->status );
            break;
      }
   }
   web_printf( wdesc->fd, "<tr><td colspan=\"5\">%d total muds listed.</td></tr>", mudcount );
   return;
}

/*
 * Added: 21 April 2001 (Trax)
 * Function to handle I3 mudlist web requests
 */
void handle_web_i3( WEB_DESCRIPTOR * wdesc, char *path, bool choice )
{
   send_200OK( wdesc );
   send_content( wdesc, CONTENT_HTML );

   show_web_file( PUB_I3LIST_H, wdesc );

   send_buf( wdesc->fd,
             "<table><tr><td><font color=\"red\">Name   </font></td><td><font color=\"yellow\">Type</font></td><td><font color=\"green\">Mudlib   </font></td><td><font color=\"blue\">Address    </font></td><td><font color=\"purple\">Port</font></td></tr>" );

   web_I3_mudlist( wdesc );

   send_buf( wdesc->fd, "</table>" );

   if( choice )
      show_web_file( PUB_I3LIST2_F, wdesc );
   else
      show_web_file( PUB_I3LIST_F, wdesc );
}
#endif

/*
 * Added: 21 April 2001 (Trax)
 * Function for sending a simple basic index for accessing the helpfiles via the web     
 */
void send_help_index( WEB_DESCRIPTOR * wdesc )
{
   char let;

   send_buf( wdesc->fd,
             "<hr />\n\r"
             "<form action=\"/help\" method=\"get\">\n\r"
             "<p align=\"center\">Keyword Lookup:&nbsp;"
             "<input type=\"text\" size=\"15\" name=\"keyword\" />&nbsp;\n\r"
             "<input type=\"submit\" value=\"Lookup\" />\n\r" "</p></form>\n\r<p align=\"center\">[" );

   for( let = 'a'; let <= 'z'; let++ )
      web_printf( wdesc->fd, "&nbsp;<a href=\"/help/%c/\">%c</a>&nbsp;%s", let, UPPER( let ), let == 'z' ? "" : "|" );

   send_buf( wdesc->fd, "]</p><hr />\n\r" );
   show_web_file( PUB_HELP_F, wdesc );
}

/*
 * Added: 21 April 2001 (Trax)
 * Function to display the list of helpfiles matching a single prefixed letter
 */
void send_help_list( WEB_DESCRIPTOR * wdesc, char let )
{
   char buf[1024], *left, word[MIL], *ptr;
   HELP_DATA *pHelp;
   int col = 0, entries = 0;

   snprintf( buf, 1024, "%c", let );

   send_buf( wdesc->fd,
             "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n\r" );
   send_buf( wdesc->fd, "<html>\n\r<head>\n\r" );
   send_buf( wdesc->fd, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\" />\n\r" );
   web_printf( wdesc->fd, "<title>AFKMud Web Help: %c</title>\n\r</head>\n\r"
               "<body bgcolor=\"#000000\" text=\"#FFFFFF\" link=\"#FF0000\">\n\r"
               "<font size=\"+2\"><b><i>\n\rWeb Based Help: Listing for letter '%c'\n\r"
               "</i></b></font>\n\r<hr /><p></p>\n\r<table>\n\r<tr>", let, let );

   for( pHelp = first_help; pHelp; pHelp = pHelp->next )
   {
      if( pHelp->level > MAX_WEBHELP_LEVEL )
         continue;

      for( left = one_argument( pHelp->keyword, word ); word[0] != '\0'; left = one_argument( left, word ) )
      {
         if( !str_prefix( buf, word ) )
         {
            char nospaces[MIL], name[MIL], name2[MIL];

            mudstrlcpy( nospaces, word, MIL );
            for( ptr = nospaces; *ptr != '\0'; ptr++ )
               if( *ptr == ' ' )
                  *ptr = '_';

            web_colourconv( name, nospaces );
            web_colourconv( name2, word );
            web_printf( wdesc->fd, "<td><a href=\"/help/%c/%s\">%s</a></td>", let, name, name2 );
            entries++;

            if( ++col % 5 == 0 )
               send_buf( wdesc->fd, "</tr>\n\r<tr>" );
         }
      }
   }
   if( col % 5 == 0 )
      send_buf( wdesc->fd, "<td></td>\n\r" );
   web_printf( wdesc->fd, "</tr>\n\r<tr>"
               "<td colspan=\"5\" align=\"center\">\n\r"
               "<b>Total of %d entries found matching '%c'</b>\n\r" "</td></tr></table><p></p>\n\r", entries, let );
}

/*
 * Added: 21 April 2001 (Trax)
 * Function to display a particular helpfile via the web interface
 */
void display_web_help( WEB_DESCRIPTOR * wdesc, char *path )
{
   HELP_DATA *pHelp;
   char buf[MSL], *ptr;

   send_200OK( wdesc );
   send_content( wdesc, CONTENT_HTML );
   send_buf( wdesc->fd,
             "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n\r" );
   send_buf( wdesc->fd, "<html>\n\r<head>\n\r" );
   send_buf( wdesc->fd, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\" />\n\r" );
   web_printf( wdesc->fd,
               "<title>AFKMud Web Help</title>\n\r</head>\n\r"
               "<body bgcolor=\"#000000\" text=\"#FFFFFF\" link=\"#FF0000\">\n\r"
               "<font size=\"+2\"><b>\n\rWeb Based Help: %s</b></font><br />\n\r<hr />\n\r", path );

   mudstrlcpy( buf, path, MSL );

   for( ptr = buf; *ptr != '\0'; ptr++ )
   {
      if( *ptr == '_' )
         *ptr = ' ';
      if( *ptr == '+' )
         *ptr = ' ';
   }

   for( pHelp = first_help; pHelp; pHelp = pHelp->next )
   {
      if( pHelp->level > MAX_WEBHELP_LEVEL )
         continue;

      if( is_name( buf, pHelp->keyword ) )
      {
         char col_buf[MSL];
         char pre_buf[MSL];

         mudstrlcpy( pre_buf, pHelp->text, MSL );

         for( ptr = pre_buf; *ptr != '\0'; ptr++ )
         {
            if( *ptr == '<' )
               *ptr = '[';
            if( *ptr == '>' )
               *ptr = ']';
         }

         web_colourconv( col_buf, pre_buf /* pHelp->text */  );

         for( ptr = col_buf; *ptr != '\0'; ptr++ )
         {
            if( *ptr == '\r' )
               *ptr = ' ';
         }
         send_buf( wdesc->fd, "<pre><tt>\n\r" );
         send_buf( wdesc->fd, col_buf );
         send_buf( wdesc->fd, "\n\r</tt></pre>\n\r" );
         break;
      }
   }

   if( !pHelp )
   {
      send_buf( wdesc->fd, "Unable to locate help on " );
      send_buf( wdesc->fd, buf );
   }
   send_help_index( wdesc );
   return;
}

/*
 * Added: 21 April 2001 (Trax)
 * Function to handle the incoming requests for the web based help system
 * This parses checks and passes the information as required
 */
void handle_web_help( WEB_DESCRIPTOR * wdesc, char *path )
{
   char *ptr;

   if( *path == '?' )
   {
      /*
       * Okay we are dealing with the form query.. 
       */
      char keyword[MIL], newpath[MSL];

      mudstrlcpy( keyword, path + 2 + strlen( "keyword" ), MIL );

      snprintf( newpath, MSL, "/%c/%s", keyword[0], keyword );

      handle_web_help( wdesc, newpath );
      return;
   }

   if( path[0] == '\0' || !strcmp( path, "/" ) || !str_cmp( path, "/index.html" ) )
   {
      send_200OK( wdesc );
      send_content( wdesc, CONTENT_HTML );
      send_buf( wdesc->fd,
                "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n\r" );
      send_buf( wdesc->fd, "<html>\n\r<head>\n\r" );
      send_buf( wdesc->fd, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\" />\n\r" );

      send_buf( wdesc->fd,
                "<title>AFKMud Web Help</title>\n\r</head>\n\r"
                "<body bgcolor=\"#000000\" text=\"#FFFFFF\" link=\"#FF0000\">\n\r"
                "<font size=\"+2\"><b>Web Based Help</b></font>\n\r"
                "<p>This section of the web server allows you to browse the helpfiles.</p>\n\r"
                "<p>Please select a letter from the below list to start browsing the "
                "list of helpfiles or enter a keyword to look for in the lookup text box below.\n\r</p>\n\r" );

      send_help_index( wdesc );
      return;
   }

   ptr = path + 1;

   if( *ptr >= 'a' && *ptr <= 'z' && *( ptr + 1 ) == '/' && *( ptr + 2 ) == '\0' )
   {
      send_200OK( wdesc );
      send_content( wdesc, CONTENT_HTML );
      send_help_list( wdesc, *ptr );
      send_help_index( wdesc );
      return;
   }

   display_web_help( wdesc, ptr + 2 );
   return;
}

void handle_web_arealist( WEB_DESCRIPTOR * wdesc, char *path, bool choice )
{
   char *print_string =
      "<tr><td><font color=\"red\">%s   </font></td><td><font color=\"yellow\">%s</font></td><td><font color=\"green\">%d - %d   </font></td><td><font color=\"blue\">%d - %d</font></td></tr>";
   AREA_DATA *pArea;

   send_200OK( wdesc );
   send_content( wdesc, CONTENT_HTML );

   show_web_file( PUB_AREALIST_H, wdesc );

   send_buf( wdesc->fd,
             "<table><tr><td><font color=\"red\">Author   </font></td><td><font color=\"yellow\">Area</font></td><td><font color=\"green\">Recommened   </font></td><td><font color=\"blue\">Enforced</font></td></tr>" );

   for( pArea = first_area_nsort; pArea; pArea = pArea->next )
   {
      web_printf( wdesc->fd, print_string, pArea->author, pArea->name, pArea->low_soft_range, pArea->hi_soft_range,
                  pArea->low_hard_range, pArea->hi_hard_range );
   }

   send_buf( wdesc->fd, "</table>" );

   if( choice )
      show_web_file( PUB_AREALIST2_F, wdesc );
   else
      show_web_file( PUB_AREALIST_F, wdesc );

   return;
}

int determine_type( char *path )
{
   int i;

   for( i = 0; *content_types[i].suffix; i++ )
   {
      if( !str_suffix( content_types[i].suffix, path ) )
         return content_types[i].type;
   }

   /*
    * If we dunno, we'll use plain text then 
    */
   return CONTENT_TEXT;
}

/*
 * Added: 21 April 2001 (Trax)
 * Function to handle general requests for files which have no special handler
 * This maps the path requested onto the HTML root defined higher up and returns that
 * file if it exists (with correct http MIME header if found defined) Will work
 * with binary or text files
 *
 * Modified: 28 March 2003 (Tomas Mecir)
 * Fixed a security bug, that allowed anyone to read any file on the server that was
 * accessible with server's UID
 */
void handle_web_root( WEB_DESCRIPTOR * wdesc, char *path )
{
   char file[MIL];
   int type, fd;
   void *buffer;

   /*
    * do not allow leaving webroot 
    */
   if( strstr( path, "../" ) )   /* not 100% clean way of doing this, but it works :) */
   {
      /*
       * will send 404 
       */
      send_404UNFOUND( wdesc );
      show_web_file( PUB_ERROR, wdesc );
      return;
   }

   if( !str_cmp( path, "" ) || !str_cmp( path, "/" ) )
      snprintf( file, MIL, "%s%s", WEB_ROOT, "/index.html" );
   else
      snprintf( file, MIL, "%s%s", WEB_ROOT, path );

   if( file[strlen( file ) - 2] == '/' )
      mudstrlcat( file, "index.html", MIL );

   /*
    * Work out the filetype so we know what we are doing 
    */
   type = determine_type( file );

   if( ( fd = open( file, O_RDONLY | O_NONBLOCK ) ) == -1 )
   {
      send_404UNFOUND( wdesc );
      show_web_file( PUB_ERROR, wdesc );
   }
   else
   {
      int readlen = 0;

      buffer = calloc( 1, 1024 );
      send_200OK( wdesc );
      send_content( wdesc, type );

      while( ( readlen = read( fd, buffer, 1024 ) ) > 0 )
         send( wdesc->fd, buffer, readlen, 0 );

      close( fd );
      free( buffer );
   }
   return;
}

void send_401UNAUTHORISED( WEB_DESCRIPTOR * wdesc, char *realm )
{
   send_buf( wdesc->fd, "HTTP/1.1 401 Unauthorised\n" );
   web_printf( wdesc->fd, "WWW-Authenticate: Basic realm=\"%s\"\n", realm );
}

void handle_web_request( WEB_DESCRIPTOR * wdesc )
{
   char buf[MSL], web_buf[MSL], path[MSL];
   char *stuff;
   int addr;

   stuff = one_argument( wdesc->request, path );
   one_argument( stuff, path );

   /*
    * process request 
    */
   /*
    * are we using HTTP/1.x? If so, write out header stuff.. 
    */
   if( !strstr( wdesc->request, "GET" ) )
   {
      send_buf( wdesc->fd, "HTTP/1.1 501 Not Implemented" );
      return;
   }
   addr = ntohl( wdesc->their_addr.sin_addr.s_addr );

   mudstrlcpy( web_buf, inet_ntoa( wdesc->their_addr.sin_addr ), MSL );

   if( !sysdata.NO_NAME_RESOLVING )
   {
      mudstrlcpy( buf, in_dns_cache( web_buf ), MSL );

      if( buf && buf[0] != '\0' )
         mudstrlcpy( web_buf, buf, MSL );
   }

   /*
    * Handle the actual request 
    */
   if( !str_cmp( path, "/wholist" ) || !str_cmp( path, "/wholist.html" ) )
   {
      handle_web_who_request( wdesc, false );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - WhoList from webpage", web_buf );
   }
   else if( !str_cmp( path, "/wholist2" ) || !str_cmp( path, "/wholist2.html" ) )
   {
      handle_web_who_request( wdesc, true );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - WhoList from port", web_buf );
   }
   else if( !str_cmp( path, "/" ) || !str_cmp( path, "/index.html" ) )
   {
      handle_web_main( wdesc );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - Index", web_buf );
   }
   else if( !str_cmp( path, "/wizlist" ) || !str_cmp( path, "/wizlist.html" ) )
   {
      handle_web_wizlist( wdesc, false );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - WizList from webpage", web_buf );
   }
   else if( !str_cmp( path, "/wizlist2" ) || !str_cmp( path, "/wizlist2.html" ) )
   {
      handle_web_wizlist( wdesc, true );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - WizList from port", web_buf );
   }

#ifdef I3
   else if( !str_cmp( path, "/i3list" ) || !str_cmp( path, "/i3list.html" ) )
   {
      handle_web_i3( wdesc, path, false );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - I3List from webpage", web_buf );
   }
   else if( !str_cmp( path, "/i3list2" ) || !str_cmp( path, "/i3list2.html" ) )
   {
      handle_web_i3( wdesc, path, true );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - I3List from port", web_buf );
   }
#endif
#ifdef IMC
   else if( !str_cmp( path, "/imclist" ) || !str_cmp( path, "/imclist.html" ) )
   {
      handle_web_imc( wdesc, path, false );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - IMCList from webpage", web_buf );
   }
   else if( !str_cmp( path, "/imclist2" ) || !str_cmp( path, "/imclist2.html" ) )
   {
      handle_web_imc( wdesc, path, true );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - IMCList from port", web_buf );
   }
#endif
   else if( !str_prefix( "/help", path ) )
   {
      handle_web_help( wdesc, path + strlen( "/help" ) );
      /*
       * Trust me when I say DONT log your help hits! 
       */
   }
   else if( !str_cmp( path, "/arealist" ) || !str_cmp( path, "/arealist.html" ) )
   {
      handle_web_arealist( wdesc, path, false );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - Area List from webpage", web_buf );
   }
   else if( !str_cmp( path, "/arealist2" ) || !str_cmp( path, "/arealist2.html" ) )
   {
      handle_web_arealist( wdesc, path, true );
      if( sysdata.webcounter == true )
         log_printf( "Web Interface Hit: %s - Area List from port", web_buf );
   }
   else
      handle_web_root( wdesc, path );  /* Now pass to root dir handler */
}

void handle_web( void )
{
   int max_fd;
   WEB_DESCRIPTOR *current;
   list < WEB_DESCRIPTOR * >::iterator curr;
   fd_set readfds;

   FD_ZERO( &readfds );
   FD_SET( web_socket, &readfds );

   /*
    * updated on June 6, 2003 by Tomas Mecir:
    * - fixed buffer overflow causing server crash when too long request comes
    */
   /*
    * it *will* be atleast web_socket 
    */
   max_fd = web_socket;

   /*
    * add in all the current web descriptors 
    */
   for( curr = weblist.begin(  ); curr != weblist.end(  ); curr++ )
   {
      FD_SET( ( *curr )->fd, &readfds );
      if( max_fd < ( *curr )->fd )
         max_fd = ( *curr )->fd;
   }

   /*
    * Wait for ONE descriptor to have activity 
    */
   select( max_fd + 1, &readfds, NULL, NULL, &ZERO_TIME );

   if( FD_ISSET( web_socket, &readfds ) )
   {
      /*
       * NEW CONNECTION -- INIT & ADD TO LIST 
       */

      current = new WEB_DESCRIPTOR;
      current->sin_size = sizeof( struct sockaddr_in );
      current->request[0] = '\0';

      if( ( current->fd =
            accept( web_socket, ( struct sockaddr * )&( current->their_addr ), &( current->sin_size ) ) ) == -1 )
      {
         log_string( "----> Web Server: Error accepting connection" );
         perror( "web-accept" );
         delete current;
         FD_CLR( web_socket, &readfds );
         return;
      }

      weblist.push_back( current );
      /*
       * END ADDING NEW DESC 
       */
   }

   /*
    * DATA IN! 
    */
   for( curr = weblist.begin(  ); curr != weblist.end(  ); curr++ )
   {
      if( FD_ISSET( ( *curr )->fd, &readfds ) ) /* We Got Data! */
      {
         char buf[MAXDATA];
         int numbytes;
         int curlen;

         if( ( numbytes = read( ( *curr )->fd, buf, sizeof( buf ) ) ) == -1 )
         {
            perror( "web-read" );
            continue;
         }

         buf[numbytes] = '\0';

         /*
          * ensure that we won't crash on a buffer overflow 
          */
         curlen = strlen( ( *curr )->request );
         mudstrlcat( ( *curr )->request, buf, 2 * MAXDATA - 6 );
         /*
          * we don't want to miss the trailing ENDREQUEST 
          */
         if( strlen( ( *curr )->request ) == 2 * MAXDATA - 7 ) /* buffer size reached */
            /*
             * we could miss the end of request, but we don't need it anyway 
             */
            mudstrlcat( ( *curr )->request, ENDREQUEST, MAXDATA * 2 );
      }
   }  /* DONE WITH DATA IN */

   /*
    * DATA OUT 
    */
   /*
    * Hmm we want to delay this if possible, to prevent it prematurely 
    */
   for( curr = weblist.begin(  ); curr != weblist.end(  ); )
   {
      current = ( *curr );
      curr++;

      if( strstr( current->request, "HTTP/1." ) /* 1.x request (vernum on FIRST LINE) */
          && strstr( current->request, ENDREQUEST ) )
         handle_web_request( current );
      else if( !strstr( current->request, "HTTP/1." ) && strchr( current->request, '\n' ) )  /* HTTP/0.9 (no ver number) */
         handle_web_request( current );
      else
         continue;   /* Don't have full request yet! */

      close( current->fd );
      weblist.remove( current );
      delete current;
   }
   /*
    * END DATA-OUT 
    */
}

/* Aurora's room-to-web toy - this could be quite fun to mess with */
void room_to_html( ROOM_INDEX_DATA * room, bool complete )
{
   FILE *fp = NULL;
   EXIT_DATA *pexit;
   char filename[256];
   bool found = false;

   if( !room )
      return;

   snprintf( filename, 256, "%s%d.html", WEB_ROOMS, room->vnum );

   if( ( fp = fopen( filename, "w" ) ) != NULL )
   {
      char roomdesc[MSL];

      fprintf( fp, "%s", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n" );
      fprintf( fp, "<html>\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n" );
      fprintf( fp, "<title>%s: %s</title>\n</head>\n\n<body bgcolor=\"#000000\">\n", room->area->name, room->name );
      fprintf( fp, "%s", "<font face=\"Fixedsys\" size=\"3\">\n" );
      fprintf( fp, "<font color=\"#FF0000\">%s</font><br />\n", room->name );
      fprintf( fp, "%s", "<font color=\"#33FF33\">[Exits:" );
      for( pexit = room->first_exit; pexit; pexit = pexit->next )
      {
         if( pexit->to_room ) /* Set any controls you want here, ie: not closed doors, etc */
         {
            found = true;
            fprintf( fp, " <a href=\"%d.html\">%s</a>", pexit->to_room->vnum, dir_name[pexit->vdir] );
         }
      }
      if( !found )
         fprintf( fp, "%s", " None.]</font><br />\n" );
      else
         fprintf( fp, "%s", "]</font><br />\n" );
      web_colourconv( roomdesc, room->roomdesc );
      fprintf( fp, "<font color=\"#999999\">%s</font><br />\n", roomdesc );

      if( complete )
      {
         obj_data *obj;
         char_data *rch;

         for( obj = room->first_content; obj; obj = obj->next_content )
         {
            if( IS_OBJ_FLAG( obj, ITEM_AUCTION ) )
               continue;

            if( obj->objdesc && obj->objdesc[0] != '\0' )
               fprintf( fp, "<font color=\"#0000EE\">%s</font><br />\n", obj->objdesc );
         }

         for( rch = room->first_person; rch; rch = rch->next_in_room )
         {
            if( IS_NPC( rch ) )
               fprintf( fp, "<font color=\"#FF00FF\">%s</font><br />\n", rch->long_descr );
            else
            {
               char pctitle[MSL];

               web_colourconv( pctitle, rch->pcdata->title );
               fprintf( fp, "<font color=\"#FF00FF\">%s %s</font><br />\n", rch->name, pctitle );
            }
         }
      }
      fprintf( fp, "%s", "</font>\n</body>\n</html>\n" );
      FCLOSE( fp );
   }
   else
      bug( "%s", "Error Opening room to html index stream!" );

   return;
}

CMDF do_webroom( CHAR_DATA * ch, char *argument )
{
   ROOM_INDEX_DATA *room;
   int hash;
   bool complete = false;

   if( !str_cmp( argument, "complete" ) )
      complete = true;

   /*
    * Limiting this to Bywater since I don't want the whole mud "webized" 
    */
   for( hash = 0; hash < MAX_KEY_HASH; hash++ )
   {
      for( room = room_index_hash[hash]; room; room = room->next )
      {
         if( IS_ROOM_FLAG( room, ROOM_AUCTION ) || IS_ROOM_FLAG( room, ROOM_PET_SHOP ) )
            continue;

         if( !str_cmp( room->area->filename, "bywater.are" ) )
            room_to_html( room, complete );
      }
   }
   send_to_char( "Bywater rooms dumped to web.\n\r", ch );
   return;
}
