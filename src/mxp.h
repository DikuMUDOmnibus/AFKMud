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
 *                           MXP Protocol Module                            *
 ****************************************************************************/

/* Mud eXtension Protocol 01/06/2001 Garil@DOTDII aka Jesse DeFer dotd@dotd.com */

#define MXP_VERSION	"0.3"

#define TELOPT_MXP 91

extern int mxpobjmenu;

typedef enum
{
   MXP_NONE = -1, MXP_GROUND = 0, MXP_INV, MXP_EQ, MXP_SHOP, MXP_STEAL, MXP_CONT, MAX_MXPOBJ
} mxp_objmenus;

extern char *mxp_obj_cmd[MAX_ITEM_TYPE][MAX_MXPOBJ];
extern char mxptail[MIL];

#define MXP_TAG_OPEN	"\033[0z"
#define MXP_TAG_SECURE	"\033[1z"
#define MXP_TAG_LOCKED	"\033[2z"

#define MXP_TAG_ROOMEXIT              MXP_TAG_SECURE"<RExits>"
#define MXP_TAG_ROOMEXIT_CLOSE        "</RExits>"MXP_TAG_LOCKED

#define MXP_TAG_ROOMNAME              MXP_TAG_SECURE"<RName>"
#define MXP_TAG_ROOMNAME_CLOSE        "</RName>"MXP_TAG_LOCKED

#define MXP_TAG_ROOMDESC              MXP_TAG_SECURE"<RDesc>"
#define MXP_TAG_ROOMDESC_CLOSE        MXP_TAG_SECURE"</RDesc>"MXP_TAG_LOCKED

#define MXP_TAG_PROMPT                MXP_TAG_SECURE"<Prompt>"
#define MXP_TAG_PROMPT_CLOSE          "</Prompt>"MXP_TAG_LOCKED
#define MXP_TAG_HP                    "<Hp>"
#define MXP_TAG_HP_CLOSE              "</Hp>"
#define MXP_TAG_MAXHP                 "<MaxHp>"
#define MXP_TAG_MAXHP_CLOSE           "</MaxHp>"
#define MXP_TAG_MANA                  "<Mana>"
#define MXP_TAG_MANA_CLOSE            "</Mana>"
#define MXP_TAG_MAXMANA               "<MaxMana>"
#define MXP_TAG_MAXMANA_CLOSE         "</MaxMana>"

#define MXP_SS_FILE     "../system/mxp.style"

#define MXP_ON(ch)      ( IS_PLR_FLAG( (ch), PLR_MXP ) && (ch)->desc && (ch)->desc->mxp_detected == TRUE )
