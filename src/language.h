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
 *                      Player communication module                         *
 ****************************************************************************/

typedef struct lcnv_data LCNV_DATA;
typedef struct lang_data LANG_DATA;

extern LANG_DATA *first_lang;
extern LANG_DATA *last_lang;

/*
 * Tongues / Languages structures
 */
struct lcnv_data
{
   LCNV_DATA *next;
   LCNV_DATA *prev;
   char *old;
   int olen;
   char *lnew;
   int nlen;
};

struct lang_data
{
   LANG_DATA *next;
   LANG_DATA *prev;
   char *name;
   LCNV_DATA *first_precnv;
   LCNV_DATA *last_precnv;
   char *alphabet;
   LCNV_DATA *first_cnv;
   LCNV_DATA *last_cnv;
};
