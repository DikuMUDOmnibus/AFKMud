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
 *                             Event Processing                             *
 *      This code is derived from the IMC2 0.10 event processing code.      *
 *      Originally written by Oliver Jowett. Licensed under the LGPL.       *
 *               See the document COPYING.LGPL for details.                 *
 ****************************************************************************/

#include "mud.h"
#include "event.h"

EVENT *first_event, *last_event;
long events_served = 0;

void free_event( EVENT * e )
{
   UNLINK( e, first_event, last_event, next, prev );
   DISPOSE( e );
}

void free_events( void )
{
   EVENT *e, *e_next;

   for( e = first_event; e; e = e_next )
   {
      e_next = e->next;

      free_event( e );
   }
}

void add_event( int when, void ( *callback ) ( void * ), void *data )
{
   EVENT *e, *cur = NULL;

   CREATE( e, EVENT, 1 );

   e->when = current_time + when;
   e->callback = callback;
   e->data = data;

   for( cur = first_event; cur; cur = cur->next )
   {
      if( cur->when > e->when )
         break;
   }

   if( !cur )
      LINK( e, first_event, last_event, next, prev );
   else
      INSERT( e, cur, first_event, next, prev );

   return;
}

void cancel_event( void ( *callback ) ( void * ), void *data )
{
   EVENT *e, *e_next;

   for( e = first_event; e; e = e_next )
   {
      e_next = e->next;

      if( ( !callback ) && e->data == data )
         free_event( e );

      else if( ( callback ) && e->data == data && data != NULL )
         free_event( e );

      else if( e->callback == callback && data == NULL )
         free_event( e );
   }
}

EVENT *find_event( void ( *callback ) ( void * ), void *data )
{
   EVENT *e;

   for( e = first_event; e; e = e->next )
      if( e->callback == callback && e->data == data )
         return e;

   return NULL;
}

time_t next_event( void ( *callback ) ( void * ), void *data )
{
   EVENT *e;

   for( e = first_event; e; e = e->next )
      if( e->callback == callback && e->data == data )
         return e->when - current_time;

   return -1;
}

void run_events( time_t newtime )
{
   EVENT *e;
   void ( *callback ) ( void * );
   void *data;

   while( first_event )
   {
      e = first_event;

      if( e->when > newtime )
         break;

      callback = e->callback;
      data = e->data;
      current_time = e->when;

      free_event( e );
      events_served++;

      if( callback )
         ( *callback ) ( data );
      else
         bug( "%s", "run_events: NULL callback" );
   }
   current_time = newtime;
}

CMDF do_eventinfo( CHAR_DATA * ch, char *argument )
{
   EVENT *ev;
   int evcount = 0;

   for( ev = first_event; ev; ev = ev->next )
      evcount++;

   ch_printf( ch, "&BPending events&c: %d\n\r", evcount );
   ch_printf( ch, "&BEvents served &c: %ld\n\r", events_served );
   return;
}
