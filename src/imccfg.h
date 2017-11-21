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
 *                             IMC2 Freedom Client                          *
 ****************************************************************************/

/* Codebase Macros - AFKMud
 *
 * This should cover the following derivatives:
 *
 * AFKMud: Versions 1.6 and up.
 *
 * Other derivatives should work too, please submit any needed changes to imc@muddomain.com and be sure
 * you mention it's for AFKMud so that the email can be properly forwarded to me. -- Samson
 */

#define imcstrlcpy mudstrlcpy /* Yes yes. Cheesy hack. I know. */
#define imcstrlcat mudstrlcat /* Yes yes. Cheesy hack. I know. */
#define IMCSTRALLOC str_dup
#define IMCSTRFREE DISPOSE
#define IMCDISPOSE DISPOSE
#define IMCLINK LINK
#define IMCUNLINK UNLINK
#define IMCINSERT INSERT
#define IMCCREATE CREATE
#define IMCRECREATE RECREATE
#define IMCFCLOSE FCLOSE
#define CH_IMCDATA(ch)        ((ch)->pcdata->imcchardata)
#define CH_IMCLEVEL(ch)       ((ch)->level)
#define CH_IMCNAME(ch)        ((ch)->name)
#define CH_IMCTITLE(ch)       ((ch)->pcdata->title)
#define CH_IMCRANK(ch)        ((ch)->pcdata->rank)
#define CH_IMCSEX(ch)         ((ch)->sex)
