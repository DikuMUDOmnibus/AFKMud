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
 *                       Shop and repair shop module                        *
 ****************************************************************************/

#define SHOP_DIR "../shops/"  /* Clan/PC shopkeepers - Samson 7-16-00 */

typedef struct shop_data SHOP_DATA;
typedef struct repairshop_data REPAIR_DATA;

extern SHOP_DATA *first_shop;
extern SHOP_DATA *last_shop;
extern REPAIR_DATA *first_repair;
extern REPAIR_DATA *last_repair;

/*
 * Shop types.
 */
#define MAX_TRADE	 5

struct shop_data
{
   SHOP_DATA *next;  /* Next shop in list    */
   SHOP_DATA *prev;  /* Previous shop in list   */
   int keeper; /* Vnum of shop keeper mob */
   short buy_type[MAX_TRADE];   /* Item types shop will buy   */
   short profit_buy;   /* Cost multiplier for buying */
   short profit_sell;  /* Cost multiplier for selling   */
   short open_hour; /* First opening hour      */
   short close_hour;   /* First closing hour      */
};

#define MAX_FIX		3
#define SHOP_FIX	1
#define SHOP_RECHARGE	2

struct repairshop_data
{
   REPAIR_DATA *next;   /* Next shop in list    */
   REPAIR_DATA *prev;   /* Previous shop in list   */
   int keeper; /* Vnum of shop keeper mob */
   short fix_type[MAX_FIX];  /* Item types shop will fix   */
   short profit_fix;   /* Cost multiplier for fixing */
   short shop_type; /* Repair shop type     */
   short open_hour; /* First opening hour      */
   short close_hour;   /* First closing hour      */
};
