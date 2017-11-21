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
 *         Slay V2.0 - Online editable configurable slay options            *
 ****************************************************************************/

/* Capability to create, edit and delete slaytypes added to original code by Samson 8-3-98 */

typedef struct slay_data SLAY_DATA;

extern SLAY_DATA *first_slay;
extern SLAY_DATA *last_slay;

/* Improved data structure for online slay editing - Samson 8-3-98 */
struct slay_data
{
   SLAY_DATA *next;
   SLAY_DATA *prev;
   char *owner;
   char *type;
   int color;
   char *cmsg;
   char *vmsg;
   char *rmsg;
};

#define SLAY_FILE		"slay.dat"  /* Slay data file for online editing - Samson 8-3-98 */

/* Maxslaytypes variable - 50 should be WAY more than enough :P */
#define MAX_SLAY_TYPES		50
