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
 *                       Intermud-3 Network Module                          *
 ****************************************************************************/

/*
 * Copyright (c) 2000 Fatal Dimensions
 *
 * See the file "LICENSE" or information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/* Ported to Smaug 1.4a by Samson of Alsherok.
 * Consolidated for cross-codebase compatibility by Samson of Alsherok.
 * Modifications and enhancements to the code
 * Copyright (c)2001-2003 Roger Libiez ( Samson )
 * Registered with the United States Copyright Office
 * TX 5-562-404
 *
 * Contains codebase specific defines to make the rest of it all work - hopefully.
 * Anything your codebase needs to alter is more than likely going to be stored in here.
 * This should be the only file you need to edit to solve unforseen compiling problems
 * if I've done this properly. And remember, this is all based on what these defines mean
 * in your STOCK code. If you've made adjustments to any of it, then you'll need to adjust
 * them here too.
 */

#define i3strlcpy mudstrlcpy  /* Yes yes, I know. But this makes it easier to diff for generic client releases. */
#define i3strlcat mudstrlcat  /* Yes yes, I know. But this makes it easier to diff for generic client releases. */
#define I3STRALLOC str_dup
#define I3STRFREE DISPOSE
#define I3DISPOSE DISPOSE
#define I3LINK LINK
#define I3UNLINK UNLINK
#define I3INSERT INSERT
#define I3CREATE CREATE
#define I3FCLOSE FCLOSE
#define CH_I3DATA(ch)		((ch)->pcdata->i3chardata)
#define CH_I3LEVEL(ch)        ((ch)->level)
#define CH_I3NAME(ch)         ((ch)->name)
#define CH_I3TITLE(ch)        ((ch)->pcdata->title)
#define CH_I3RANK(ch)         ((ch)->pcdata->rank)
#define CH_I3SEX(ch)          ((ch)->sex)
