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

/*	
 *	Web.h was made because some webmasters don't know the first thing
 *	about code.  So I saw it as a chance to dip my quill into a little
 *	more coding.
 *	
 *	What this is for is use with 'show_web_file' in websvr.c, it will
 *	do about the exact same as 'show_file' but for the WWW.  What this
 *	will do is show HTML files from a directory rather from code.
 *	
 *	The file types go like this:
 *
 *	*.ti	These are files that are just one solid page with no code
 *		before or after it.  Such as an index page or error page.
 *
 *	*.tih	This will be a header (Telnet Interface Header), that will
 *	start out a HTML file with tags or what not, before the code.
 *	Placing graphics for headers is not uncommon and leave a nice touch.
 *	
 *	*.tif	This will be a footer (Telnet Interface Footer), it will clean
 *	up the mess made by the code, and finish the HTML, providing links
 *	and so on and so forth.  This is your interface, be creative :)
 *
 *	This should make those rudy poo webmasters happy, somewhat :)
 *	Just explain that they will have to account for the middle of the html
 *	to be code generated and to format accordingly before and after.
 *
 *	-- Christopher Aaron Haslage (Yakkov) 6/3/99 (No Help)
 */

#ifndef __WEB_H__
#define __WEB_H__

#define PUBLIC_WEB	"../public_html/"

#define PUB_INDEX		PUBLIC_WEB "index.html"
#define PUB_ERROR		PUBLIC_WEB "error.html"
#define PUB_WIZLIST_H	PUBLIC_WEB "wizlist.tih"
#define PUB_WIZLIST_F	PUBLIC_WEB "wizlist.tif"
#define PUB_WIZLIST2_F  PUBLIC_WEB "wizlist2.tif"
#define PUB_WHOLIST_H	PUBLIC_WEB "wholist.tih"
#define PUB_WHOLIST_F	PUBLIC_WEB "wholist.tif"
#define PUB_WHOLIST2_F  PUBLIC_WEB "wholist2.tif"
#define PUB_AREALIST_H	PUBLIC_WEB "arealist.tih"
#define PUB_AREALIST_F	PUBLIC_WEB "arealist.tif"
#define PUB_AREALIST2_F PUBLIC_WEB "arealist2.tif"
#define PUB_HELP_H	PUBLIC_WEB "help.tih"
#define PUB_HELP_F	PUBLIC_WEB "help.tif"
#define PUB_I3LIST_H	PUBLIC_WEB "i3list.tih"
#define PUB_I3LIST_F	PUBLIC_WEB "i3list.tif"
#define PUB_I3LIST2_F	PUBLIC_WEB "i3list2.tif"
#define PUB_IMCLIST_H	PUBLIC_WEB "imclist.tih"
#define PUB_IMCLIST_F	PUBLIC_WEB "imclist.tif"
#define PUB_IMCLIST2_F	PUBLIC_WEB "imclist2.tif"

/*
 * Added: 21 April 2001 (Trax)
 * Moved to a .h file as some of this info is gonna be useful elsewhere
 * (and I don't like bloating merc.h/mud.h, its bad enough as it is)
 *
 * Define various places where web pages will reside for the server to serve
 * as well as any other misc files req.
 */
#define MAXDATA 1024

#define WEB_ROOT        PUBLIC_WEB  /* General file storage if not caught by a special case */
#define MAX_WEBHELP_LEVEL LEVEL_AVATAR /* The level that webhelps are displayed for (inclusive) */

typedef struct web_descriptor WEB_DESCRIPTOR;

struct web_descriptor
{
   struct sockaddr_in their_addr;
   char request[MAXDATA * 2];
   socklen_t sin_size;
   int fd;
};
#endif

void send_buf( int fd, const char *buf );
