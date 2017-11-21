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
 *                           Auction House module                           *
 ****************************************************************************/

#define AUC_DIR "../aucvaults/"
#define SALES_FILE "sales.dat"

typedef struct auction_data AUCTION_DATA; /* auction data */
typedef struct sale_data SALE_DATA;

extern AUCTION_DATA *auction;
extern SALE_DATA *first_sale;
extern SALE_DATA *last_sale;

struct auction_data
{
   OBJ_DATA *item;   /* a pointer to the item */
   CHAR_DATA *seller;   /* a pointer to the seller - which may NOT quit */
   CHAR_DATA *buyer; /* a pointer to the buyer - which may NOT quit */
   int bet; /* last bet - or 0 if noone has bet anything */
   short going;  /* 1,2, sold */
   int starting;
};

/* Holds data for sold items - Samson 6-23-99 */
struct sale_data
{
   SALE_DATA *next;
   SALE_DATA *prev;
   char *aucmob;
   char *seller;
   char *buyer;
   char *item;
   int bid;
   bool collected;
};
