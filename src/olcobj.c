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
 *                          Oasis OLC Module                                *
 ****************************************************************************/

/****************************************************************************
 * ResortMUD 4.0 Beta by Ntanel, Garinan, Badastaz, Josh, Digifuzz, Senir,  *
 * Kratas, Scion, Shogar and Tagith.  Special thanks to Thoric, Nivek,      *
 * Altrag, Arlorn, Justice, Samson, Dace, HyperEye and Yakkov.              *
 ****************************************************************************
 * Copyright (C) 1996 - 2001 Haslage Net Electronics: MudWorld              *
 * of Lorain, Ohio - ALL RIGHTS RESERVED                                    *
 * The text and pictures of this publication, or any part thereof, may not  *
 * be reproduced or transmitted in any form or by any means, electronic or  *
 * mechanical, includes photocopying, recording, storage in a information   *
 * retrieval system, or otherwise, without the prior written or e-mail      *
 * consent from the publisher.                                              *
 ****************************************************************************
 * GREETING must mention ResortMUD programmers and the help file named      *
 * CREDITS must remain completely intact as listed in the SMAUG license.    *
 ****************************************************************************/

/**************************************************************************\
 *                                                                        *
 *     OasisOLC II for Smaug 1.40 written by Evan Cortens(Tagith)         *
 *                                                                        *
 *   Based on OasisOLC for CircleMUD3.0bpl9 written by Harvey Gilpin      *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 *                    Object editing module (oedit.c) v1.0                *
 *                                                                        *
\**************************************************************************/

#include "mud.h"
#include "liquids.h"
#include "treasure.h"

/* External functions */
extern int top_affect;
extern int top_ed;
extern char *const liquid_types[];

LIQ_TABLE *get_liq_vnum( int vnum );
void medit_disp_aff_flags( DESCRIPTOR_DATA * d );
void medit_disp_ris( DESCRIPTOR_DATA * d );
void olc_log( DESCRIPTOR_DATA * d, const char *format, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void armorgen( OBJ_DATA * obj );
void weapongen( OBJ_DATA * obj );
int set_obj_rent( OBJ_INDEX_DATA * obj );
int get_trapflag( char *flag );
int get_traptype( char *flag );

/* Internal functions */
void oedit_disp_menu( DESCRIPTOR_DATA * d );

void ostat_plus( CHAR_DATA * ch, OBJ_DATA * obj, bool olc )
{
   LIQ_TABLE *liq = NULL;
   SKILLTYPE *sktmp;
   int dam;
   char buf[MSL], lbuf[10];
   char *armortext = NULL;
   int x;

   /******
    * A more informative ostat, so You actually know what those obj->value[x] mean
    * without looking in the code for it. Combines parts of look, examine, the
    * identification spell, and things that were never seen.. Probably overkill
    * on most things, but I'm lazy and hate digging through code to see what
    * value[x] means... -Druid
    ******/
   send_to_char( "&wAdditional Object information:\n\r", ch );
   switch ( obj->item_type )
   {
      default:
         send_to_char( "Additional details not available. Perhaps this item has no values??\n\r", ch );
         send_to_char( "Report this to your Kingdom Lord so that it can be looked into.\n\r", ch );
         break;
      case ITEM_LIGHT:
         ch_printf( ch, "%sValue[2] - Hours left: ", olc ? "&gG&w) " : "&w" );
         if( obj->value[2] >= 0 )
            ch_printf( ch, "&c%d\n\r", obj->value[2] );
         else
            send_to_char( "&cInfinite\n\r", ch );
         break;
      case ITEM_POTION:
      case ITEM_PILL:
      case ITEM_SCROLL:
         ch_printf( ch, "%sValue[0] - Spell Level: %d\n\r", olc ? "&gE&w) " : "&w", obj->value[0] );
         for( x = 1; x <= 3; x++ )
         {
            snprintf( lbuf, 10, "&g%c&w) ", 'E' + x );
            if( obj->value[x] >= 0 && ( sktmp = get_skilltype( obj->value[x] ) ) != NULL )
               ch_printf( ch, "%sValue[%d] - Spell (%d): &c%s\n\r", olc ? lbuf : "", x, obj->value[x], sktmp->name );
            else
               ch_printf( ch, "%sValue[%d] - Spell: None\n\r", olc ? lbuf : "&w", x );
         }
         if( obj->item_type == ITEM_PILL )
            ch_printf( ch, "%sValue[4] - Food Value: &c%d\n\r", olc ? "&gI&w) " : "&w", obj->value[4] );
         break;
      case ITEM_SALVE:
      case ITEM_WAND:
      case ITEM_STAFF:
         ch_printf( ch, "%sValue[0] - Spell Level: &c%d\n\r", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch_printf( ch, "%sValue[1] - Max Charges: &c%d\n\r", olc ? "&gF&w) " : "&w", obj->value[1] );
         ch_printf( ch, "%sValue[2] - Charges Remaining: &c%d\n\r", olc ? "&gG&w) " : "&w", obj->value[2] );
         if( obj->item_type != ITEM_SALVE )
         {
            if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != NULL )
               ch_printf( ch, "%sValue[3] - Spell (%d): &c%s\n\r", olc ? "&gH&w) " : "&w", obj->value[3], sktmp->name );
            else
               ch_printf( ch, "%sValue[3] - Spell: &cNone\n\r", olc ? "&gH&w) " : "&w" );
            break;
         }
         ch_printf( ch, "%sValue[3] - Delay (beats): &c%d\n\r", olc ? "&gH&w) " : "&w", obj->value[3] );
         for( x = 4; x <= 5; x++ )
         {
            snprintf( lbuf, 10, "&g%c&w) ", 'E' + x );
            if( obj->value[x] >= 0 && ( sktmp = get_skilltype( obj->value[x] ) ) != NULL )
               ch_printf( ch, "%sValue[%d] - Spell (%d): &c%s\n\r", olc ? lbuf : "&w", x, obj->value[x], sktmp->name );
            else
               ch_printf( ch, "%sValue[%d] - Spell: None\n\r", olc ? lbuf : "&w", x );
         }
         break;
      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
         ch_printf( ch, "%sValue[0] - Base Condition: &c%d\n\r", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch_printf( ch, "%sValue[1] - Min. Damage: &c%d\n\r", olc ? "&gF&w) " : "&w", obj->value[1] );
         ch_printf( ch, "%sValue[2] - Max Damage: &c%d\n\r", olc ? "&gG&w) " : "&w", obj->value[2] );
         ch_printf( ch, "&RAverage Hit: %d\n\r", ( obj->value[1] + obj->value[2] ) / 2 );
         if( obj->item_type != ITEM_MISSILE_WEAPON )
            ch_printf( ch, "%sValue[3] - Damage Type (%d): &c%s\n\r", olc ? "&gH&w) " : "&w", obj->value[3],
                       attack_table[obj->value[3]] );
         ch_printf( ch, "%sValue[4] - Skill Required (%d): &c%s\n\r", olc ? "&gI&w) " : "&w", obj->value[4],
                    weapon_skills[obj->value[4]] );
         if( obj->item_type == ITEM_MISSILE_WEAPON )
         {
            ch_printf( ch, "%sValue[5] - Projectile Fired (%d): &c%s\n\r", olc ? "&gJ&w) " : "&w", obj->value[5],
                       projectiles[obj->value[5]] );
            send_to_char( "Projectile fired must match on the projectiles this weapon fires.\n\r", ch );
         }
         ch_printf( ch, "%sValue[6] - Current Condition: &c%d\n\r", olc ? "&gK&w) " : "&w", obj->value[6] );
         ch_printf( ch, "Condition: %s\n\r", condtxt( obj->value[6], obj->value[0] ) );
         ch_printf( ch, "%sValue[7] - Available sockets: &c%d\n\r", olc ? "&gL&w) " : "&w", obj->value[7] );
         ch_printf( ch, "Socket 1: %s\n\r", obj->socket[0] );
         ch_printf( ch, "Socket 2: %s\n\r", obj->socket[1] );
         ch_printf( ch, "Socket 3: %s\n\r", obj->socket[2] );
         send_to_char( "&WThe following 3 settings only apply to automatically generated weapons.\n\r", ch );
         ch_printf( ch, "%sValue[8] - Weapon Type (%d): &c%s\n\r", olc ? "&gM&w) " : "&w",
                    obj->value[8], weapon_type[obj->value[8]].name );
         ch_printf( ch, "%sValue[9] - Weapon Material (%d): &c%s\n\r", olc ? "&gN&w) " : "&w",
                    obj->value[9], armor_materials[obj->value[9]].name );
         ch_printf( ch, "%sValue[10] - Weapon Quality (%d): &c%s\n\r", olc ? "&gO&w) " : "&w",
                    obj->value[10], weapon_quality[obj->value[10]] );
         break;
      case ITEM_ARMOR:
         ch_printf( ch, "%sValue[0] - Current AC: &c%d\n\r", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch_printf( ch, "%sValue[1] - Base AC: &c%d\n\r", olc ? "&gF&w) " : "&w", obj->value[1] );
         send_to_char( "Condition: ", ch );
         armortext = condtxt( obj->value[1], obj->value[0] );
         ch_printf( ch, "%s\n\r", armortext );
         ch_printf( ch, "%sValue[2] - Available sockets( applies only to body armor ): &c%d\n\r", olc ? "&gG&w) " : "&w",
                    obj->value[2] );
         ch_printf( ch, "Socket 1: %s\n\r", obj->socket[0] );
         ch_printf( ch, "Socket 2: %s\n\r", obj->socket[1] );
         ch_printf( ch, "Socket 3: %s\n\r", obj->socket[2] );
         send_to_char( "&WThe following 2 settings only apply to automatically generated armors.\n\r", ch );
         ch_printf( ch, "%sValue[3] - Armor Type (%d): &c%s\n\r", olc ? "&gH&w) " : "&w",
                    obj->value[3], armor_type[obj->value[3]].name );
         ch_printf( ch, "%sValue[4] - Armor Material (%d): &c%s\n\r", olc ? "&gI&w) " : "&w",
                    obj->value[4], armor_materials[obj->value[4]].name );
         break;
         /*
          * Bug Fix 7/9/00 -Druid
          */
      case ITEM_COOK:
      case ITEM_FOOD:
         ch_printf( ch, "%sValue[0] - Food Value: &c%d\n\r", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch_printf( ch, "%sValue[1] - Condition (%d): &c", olc ? "&gF&w) " : "&w", obj->value[1] );
         if( obj->timer > 0 && obj->value[1] > 0 )
            dam = ( obj->timer * 10 ) / obj->value[1];
         else
            dam = 10;
         if( dam >= 10 )
            mudstrlcpy( buf, "It is fresh.", MSL );
         else if( dam == 9 )
            mudstrlcpy( buf, "It is nearly fresh.", MSL );
         else if( dam == 8 )
            mudstrlcpy( buf, "It is perfectly fine.", MSL );
         else if( dam == 7 )
            mudstrlcpy( buf, "It looks good.", MSL );
         else if( dam == 6 )
            mudstrlcpy( buf, "It looks ok.", MSL );
         else if( dam == 5 )
            mudstrlcpy( buf, "It is a little stale.", MSL );
         else if( dam == 4 )
            mudstrlcpy( buf, "It is a bit stale.", MSL );
         else if( dam == 3 )
            mudstrlcpy( buf, "It smells slightly off.", MSL );
         else if( dam == 2 )
            mudstrlcpy( buf, "It smells quite rank.", MSL );
         else if( dam == 1 )
            mudstrlcpy( buf, "It smells revolting!", MSL );
         else if( dam <= 0 )
            mudstrlcpy( buf, "It is crawling with maggots!", MSL );
         mudstrlcat( buf, "\n\r", MSL );
         send_to_char( buf, ch );
         if( obj->item_type == ITEM_COOK )
         {
            ch_printf( ch, "%sValue[2] - Condition (%d): &c", olc ? "&gG&w) " : "&w", obj->value[2] );
            dam = obj->value[2];
            if( dam >= 3 )
               mudstrlcpy( buf, "It is burned to a crisp.", MSL );
            else if( dam == 2 )
               mudstrlcpy( buf, "It is a little over cooked.", MSL );
            else if( dam == 1 )
               mudstrlcpy( buf, "It is perfectly roasted.", MSL );
            else
               mudstrlcpy( buf, "It is raw.", MSL );
            mudstrlcat( buf, "\n\r", MSL );
            send_to_char( buf, ch );
         }
         if( obj->value[3] != 0 )
         {
            ch_printf( ch, "%sValue[3] - Poisoned (%d): &cYes\n\r", olc ? "&gH&w) " : "&w", obj->value[3] );
            x = 2 * obj->value[0] * ( obj->value[3] > 0 ? obj->value[3] : 1 );
            ch_printf( ch, "Duration: %d\n\r", x );
         }
         if( obj->timer > 0 && obj->value[1] > 0 )
            dam = ( obj->timer * 10 ) / obj->value[1];
         else
            dam = 10;
         if( obj->value[3] <= 0 && ( ( dam < 4 && number_range( 0, dam + 1 ) == 0 )
                                     || ( obj->item_type == ITEM_COOK && obj->value[2] == 0 ) ) )
         {
            send_to_char( "Poison: Yes\n\r", ch );
            x = 2 * obj->value[0] * ( obj->value[3] > 0 ? obj->value[3] : 1 );
            ch_printf( ch, "Duration: %d\n\r", x );
         }
         if( obj->value[4] )
            ch_printf( ch, "%sValue[4] - Timer: &c%d\n\r", olc ? "&gI&w) " : "&w", obj->value[4] );
         break;
      case ITEM_DRINK_CON:
         if( ( liq = get_liq_vnum( obj->value[2] ) ) == NULL )
            bug( "Do_look: bad liquid number %d.", obj->value[2] );

         ch_printf( ch, "%sValue[0] - Capacity: &c%d\n\r", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch_printf( ch, "%sValue[1] - Quantity Left (%d): &c", olc ? "&gF&w) " : "&w", obj->value[1] );
         if( obj->value[1] > obj->value[0] )
            send_to_char( "More than Full\n\r", ch );
         else if( obj->value[1] == obj->value[0] )
            send_to_char( "Full\n\r", ch );
         else if( obj->value[1] >= ( 3 * obj->value[0] / 4 ) )
            send_to_char( "Almost Full\n\r", ch );
         else if( obj->value[1] > ( obj->value[0] / 2 ) )
            send_to_char( "More than half full\n\r", ch );
         else if( obj->value[1] == ( obj->value[0] / 2 ) )
            send_to_char( "Half full\n\r", ch );
         else if( obj->value[1] >= ( obj->value[0] / 4 ) )
            send_to_char( "Less than half full\n\r", ch );
         else if( obj->value[1] >= 1 )
            send_to_char( "Almost Empty\n\r", ch );
         else
            send_to_char( "Empty\n\r", ch );
         ch_printf( ch, "%sValue[2] - Liquid (%d): &c%s\n\r", olc ? "&gG&w) " : "&w", obj->value[2], liq->name );
         ch_printf( ch, "Liquid type : %s\n\r", liquid_types[liq->type] );
         ch_printf( ch, "Liquid color: %s\n\r", liq->color );
         if( liq->mod[COND_DRUNK] != 0 )
            ch_printf( ch, "Affects Drunkeness by: %d\n\r", liq->mod[COND_DRUNK] );
         if( liq->mod[COND_FULL] != 0 )
            ch_printf( ch, "Affects Hunger by: %d\n\r", liq->mod[COND_FULL] );
         if( liq->mod[COND_THIRST] != 0 )
            ch_printf( ch, "Affects Thirst by: %d\n\r", liq->mod[COND_THIRST] );
         ch_printf( ch, "Poisoned: &c%s\n\r", liq->type == LIQTYPE_POISON ? "Yes" : "No" );
         break;
      case ITEM_HERB:
         ch_printf( ch, "%sValue[1] - Charges: &c%d\n\r", olc ? "&gF&w) " : "&w", obj->value[1] );
         ch_printf( ch, "%sValue[2] - Herb #: &cY%d\n\r", olc ? "&gG&w) " : "&w", obj->value[2] );
         break;
      case ITEM_CONTAINER:
         ch_printf( ch, "%sValue[0] - Capacity (%d): &c", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch_printf( ch, "%s\n\r",
                    obj->value[0] < 76 ? "Small capacity" :
                    obj->value[0] < 150 ? "Small to medium capacity" :
                    obj->value[0] < 300 ? "Medium capacity" :
                    obj->value[0] < 550 ? "Medium to large capacity" :
                    obj->value[0] < 751 ? "Large capacity" : "Giant capacity" );
         ch_printf( ch, "%sValue[1] - Flags (%d): &c", olc ? "&gF&w) " : "&w", obj->value[1] );
         if( obj->value[1] <= 0 )
            send_to_char( " None\n\r", ch );
         else
         {
            if( IS_SET( obj->value[1], CONT_CLOSEABLE ) )
               send_to_char( " Closeable", ch );
            if( IS_SET( obj->value[1], CONT_PICKPROOF ) )
               send_to_char( " PickProof", ch );
            if( IS_SET( obj->value[1], CONT_CLOSED ) )
               send_to_char( " Closed", ch );
            if( IS_SET( obj->value[1], CONT_LOCKED ) )
               send_to_char( " Locked", ch );
            if( IS_SET( obj->value[1], CONT_EATKEY ) )
               send_to_char( " EatKey", ch );
            send_to_char( "\n\r", ch );
         }
         ch_printf( ch, "%sValue[2] - Key (%d): &c", olc ? "&gG&w) " : "&w", obj->value[2] );
         if( obj->value[2] <= 0 )
            send_to_char( "None\n\r", ch );
         else
         {
            OBJ_INDEX_DATA *key = get_obj_index( obj->value[2] );

            if( key )
               ch_printf( ch, "%s\n\r", key->short_descr );
            else
               send_to_char( "ERROR: Key does not exist!\n\r", ch );
         }
         ch_printf( ch, "%sValue[3] - Condition: &c%d\n\r", olc ? "&gH&w) " : "&w", obj->value[3] );
         if( obj->timer )
            ch_printf( ch, "Object Timer, Time Left: %d\n\r", obj->timer );
         break;
      case ITEM_PROJECTILE:
         ch_printf( ch, "%sValue[0] - Condition: &c%d\n\r", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch_printf( ch, "%sValue[1] - Min. Damage: &c%d\n\r", olc ? "&gF&w) " : "&w", obj->value[1] );
         ch_printf( ch, "%sValue[2] - Max Damage: &c%d\n\r", olc ? "&gG&w) " : "&w", obj->value[2] );
         ch_printf( ch, "%sValue[3] - Damage Type (%d): &c%s\n\r", olc ? "&gH&w) " : "&w", obj->value[3],
                    attack_table[obj->value[3]] );
         ch_printf( ch, "%sValue[4] - Projectile Type (%d): &c%s\n\r", olc ? "&gI&w) " : "&w", obj->value[4],
                    projectiles[obj->value[4]] );
         send_to_char( "Projectile type must match on the missileweapon which fires it.\n\r", ch );
         ch_printf( ch, "%sValue[5] - Current Condition: &c%d\n\r", olc ? "&gJ&w) " : "&w", obj->value[5] );
         ch_printf( ch, "Condition: %s", condtxt( obj->value[5], obj->value[0] ) );
         break;
      case ITEM_MONEY:
         ch_printf( ch, "%sValue[0] - # of Coins: &c%d\n\r", olc ? "&gE&w) " : "&w", obj->value[0] );
         break;
      case ITEM_FURNITURE:
         ch_printf( ch, "%sValue[2] - Furniture Flags: &c%s\n\r", olc ? "&gG&w) " : "&w",
                    flag_string( obj->value[2], furniture_flags ) );
         break;
      case ITEM_TRAP:
         ch_printf( ch, "%sValue[0] - Charges Remaining: &c%d\n\r", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch_printf( ch, "%sValue[1] - Type (%d): &c%s\n\r", olc ? "&gF&w) " : "&w",
                    obj->value[1], trap_types[obj->value[1]] );
         switch ( obj->value[1] )
         {
            default:
               mudstrlcpy( buf, "Hit by a trap", MSL );
               ch_printf( ch, "Does Damage from (%d) to (%d)\n\r", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_POISON_GAS:
               mudstrlcpy( buf, "Surrounded by a green cloud of gas", MSL );
               send_to_char( "Casts spell: Poison\n\r", ch );
               break;
            case TRAP_TYPE_POISON_DART:
               mudstrlcpy( buf, "Hit by a dart", MSL );
               send_to_char( "Casts spell: Poison\n\r", ch );
               ch_printf( ch, "Does Damage from (%d) to (%d)\n\r", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_POISON_NEEDLE:
               mudstrlcpy( buf, "Pricked by a needle", MSL );
               send_to_char( "Casts spell: Poison\n\r", ch );
               ch_printf( ch, "Does Damage from (%d) to (%d)\n\r", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_POISON_DAGGER:
               mudstrlcpy( buf, "Stabbed by a dagger", MSL );
               send_to_char( "Casts spell: Poison\n\r", ch );
               ch_printf( ch, "Does Damage from (%d) to (%d)\n\r", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_POISON_ARROW:
               mudstrlcpy( buf, "Struck with an arrow", MSL );
               send_to_char( "Casts spell: Poison\n\r", ch );
               ch_printf( ch, "Does Damage from (%d) to (%d)\n\r", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_BLINDNESS_GAS:
               mudstrlcpy( buf, "Surrounded by a red cloud of gas", MSL );
               send_to_char( "Casts spell: Blind\n\r", ch );
               break;
            case TRAP_TYPE_SLEEPING_GAS:
               mudstrlcpy( buf, "Surrounded by a yellow cloud of gas", MSL );
               send_to_char( "Casts spell: Sleep\n\r", ch );
               break;
            case TRAP_TYPE_FLAME:
               mudstrlcpy( buf, "Struck by a burst of flame", MSL );
               send_to_char( "Casts spell: Flamestrike\n\r", ch );
               break;
            case TRAP_TYPE_EXPLOSION:
               mudstrlcpy( buf, "Hit by an explosion", MSL );
               send_to_char( "Casts spell: Fireball\n\r", ch );
               break;
            case TRAP_TYPE_ACID_SPRAY:
               mudstrlcpy( buf, "Covered by a spray of acid", MSL );
               send_to_char( "Casts spell: Acid Blast\n\r", ch );
               break;
            case TRAP_TYPE_ELECTRIC_SHOCK:
               mudstrlcpy( buf, "Suddenly shocked", MSL );
               send_to_char( "Casts spell: Lightning Bolt\n\r", ch );
               break;
            case TRAP_TYPE_BLADE:
               mudstrlcpy( buf, "Sliced by a razor sharp blade", MSL );
               ch_printf( ch, "Does Damage from (%d) to (%d)\n\r", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_SEX_CHANGE:
               mudstrlcpy( buf, "Surrounded by a mysterious aura", MSL );
               send_to_char( "Casts spell: Change Sex\n\r", ch );
               break;
         }
         ch_printf( ch, "Text Displayed: %s\n\r", buf );
         ch_printf( ch, "%sValue[3] - Trap Flags (%d): &c", olc ? "&gH&w) " : "&w", obj->value[3] );
         ch_printf( ch, "%s\n\r", flag_string( obj->value[3], trap_flags ) );
         ch_printf( ch, "%sValue[4] - Min. Damage: &c%d\n\r", olc ? "&gI&w) " : "&w", obj->value[4] );
         ch_printf( ch, "%sValue[5] - Max Damage: &c%d\n\r", olc ? "&gJ&w) " : "&w", obj->value[5] );
         break;
      case ITEM_KEY:
         ch_printf( ch, "%sValue[0] - Lock #: &c%d\n\r", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch_printf( ch, "%sValue[4] - Durability: &c%d\n\r", olc ? "&gI&w) " : "&w", obj->value[4] );
         ch_printf( ch, "%sValue[5] - Container Lock Number: &c%d\n\r", olc ? "&gJ&w) " : "&w", obj->value[5] );
         break;
      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
      case ITEM_BUTTON:
         ch_printf( ch, "%sValue[0] - Flags (%d): &c", olc ? "&gE&w) " : "&w", obj->value[0] );
         if( IS_SET( obj->value[0], TRIG_UP ) )
            send_to_char( " UP", ch );
         if( IS_SET( obj->value[0], TRIG_UNLOCK ) )
            send_to_char( " Unlock", ch );
         if( IS_SET( obj->value[0], TRIG_LOCK ) )
            send_to_char( " Lock", ch );
         if( IS_SET( obj->value[0], TRIG_D_NORTH ) )
            send_to_char( " North", ch );
         if( IS_SET( obj->value[0], TRIG_D_SOUTH ) )
            send_to_char( " South", ch );
         if( IS_SET( obj->value[0], TRIG_D_EAST ) )
            send_to_char( " East", ch );
         if( IS_SET( obj->value[0], TRIG_D_WEST ) )
            send_to_char( " West", ch );
         if( IS_SET( obj->value[0], TRIG_D_UP ) )
            send_to_char( " Up", ch );
         if( IS_SET( obj->value[0], TRIG_D_DOWN ) )
            send_to_char( " Down", ch );
         if( IS_SET( obj->value[0], TRIG_DOOR ) )
            send_to_char( " Door", ch );
         if( IS_SET( obj->value[0], TRIG_CONTAINER ) )
            send_to_char( " Container", ch );
         if( IS_SET( obj->value[0], TRIG_OPEN ) )
            send_to_char( " Open", ch );
         if( IS_SET( obj->value[0], TRIG_CLOSE ) )
            send_to_char( " Close", ch );
         if( IS_SET( obj->value[0], TRIG_PASSAGE ) )
            send_to_char( " Passage", ch );
         if( IS_SET( obj->value[0], TRIG_OLOAD ) )
            send_to_char( " Oload", ch );
         if( IS_SET( obj->value[0], TRIG_MLOAD ) )
            send_to_char( " Mload", ch );
         if( IS_SET( obj->value[0], TRIG_TELEPORT ) )
            send_to_char( " Teleport", ch );
         if( IS_SET( obj->value[0], TRIG_TELEPORTALL ) )
            send_to_char( " TeleportAll", ch );
         if( IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
            send_to_char( " TeleportPlus", ch );
         if( IS_SET( obj->value[0], TRIG_DEATH ) )
            send_to_char( " Death", ch );
         if( IS_SET( obj->value[0], TRIG_CAST ) )
            send_to_char( " Cast", ch );
         if( IS_SET( obj->value[0], TRIG_FAKEBLADE ) )
            send_to_char( " FakeBlade", ch );
         if( IS_SET( obj->value[0], TRIG_RAND4 ) )
            send_to_char( " Rand4", ch );
         if( IS_SET( obj->value[0], TRIG_RAND6 ) )
            send_to_char( " Rand6", ch );
         if( IS_SET( obj->value[0], TRIG_TRAPDOOR ) )
            send_to_char( " Trapdoor", ch );
         if( IS_SET( obj->value[0], TRIG_ANOTHEROOM ) )
            send_to_char( " Anotheroom", ch );
         if( IS_SET( obj->value[0], TRIG_USEDIAL ) )
            send_to_char( " UseDial", ch );
         if( IS_SET( obj->value[0], TRIG_ABSOLUTEVNUM ) )
            send_to_char( " AbsoluteVnum", ch );
         if( IS_SET( obj->value[0], TRIG_SHOWROOMDESC ) )
            send_to_char( " ShowRoomDesc", ch );
         if( IS_SET( obj->value[0], TRIG_AUTORETURN ) )
            send_to_char( " AutoReturn", ch );
         send_to_char( "\n\r", ch );
         send_to_char( "Trigger Position: ", ch );
         if( obj->item_type != ITEM_BUTTON )
         {
            if( IS_SET( obj->value[0], TRIG_UP ) )
               send_to_char( "UP\n\r", ch );
            else
               send_to_char( "Down\n\r", ch );
         }
         else
         {
            if( IS_SET( obj->value[0], TRIG_UP ) )
               send_to_char( "IN\n\r", ch );
            else
               send_to_char( "Out\n\r", ch );
         }
         ch_printf( ch, "Automatically Reset Trigger?: %s\n\r", IS_SET( obj->value[0], TRIG_AUTORETURN ) ? "Yes" : "No" );
         if( HAS_PROG( obj->pIndexData, PULL_PROG ) || HAS_PROG( obj->pIndexData, PUSH_PROG ) )
            send_to_char( "Object Has: ", ch );
         if( HAS_PROG( obj->pIndexData, PULL_PROG ) && HAS_PROG( obj->pIndexData, PUSH_PROG ) )
            send_to_char( "Push and Pull Programs\n\r", ch );
         else if( HAS_PROG( obj->pIndexData, PULL_PROG ) )
            send_to_char( "Pull Program\n\r", ch );
         else if( HAS_PROG( obj->pIndexData, PUSH_PROG ) )
            send_to_char( "Push Program\n\r", ch );
         if( IS_SET( obj->value[0], TRIG_TELEPORT )
             || IS_SET( obj->value[0], TRIG_TELEPORTALL ) || IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
         {
            ch_printf( ch, "Triggers: Teleport %s\n\r",
                       IS_SET( obj->value[0], TRIG_TELEPORT ) ? "Character <actor>" :
                       IS_SET( obj->value[0], TRIG_TELEPORTALL ) ? "All Characters in room" :
                       "All Characters and Objects in room" );
            ch_printf( ch, "%sValue[1] - Teleport to Room: &c%d\n\r", olc ? "&gF&w) " : "&w", obj->value[1] );
            ch_printf( ch, "Show Room Description on Teleport? %s\n\r",
                       IS_SET( obj->value[0], TRIG_SHOWROOMDESC ) ? "Yes" : "No" );
         }
         if( IS_SET( obj->value[0], TRIG_RAND4 ) || IS_SET( obj->value[0], TRIG_RAND6 ) )
            ch_printf( ch, "Triggers: Randomize Exits (%s)\n\r", IS_SET( obj->value[0], TRIG_RAND4 ) ? "3" : "5" );
         if( IS_SET( obj->value[0], TRIG_DOOR ) )
         {
            send_to_char( "Triggers: Door\n\r", ch );
            if( IS_SET( obj->value[0], TRIG_PASSAGE ) )
               send_to_char( "Triggers: Create Passage\n\r", ch );
            if( IS_SET( obj->value[0], TRIG_UNLOCK ) )
               send_to_char( "Triggers: Unlock Door\n\r", ch );
            if( IS_SET( obj->value[0], TRIG_LOCK ) )
               send_to_char( "Triggers: Lock Door\n\r", ch );
            if( IS_SET( obj->value[0], TRIG_OPEN ) )
               send_to_char( "Triggers: Open Door\n\r", ch );
            if( IS_SET( obj->value[0], TRIG_CLOSE ) )
               send_to_char( "Triggers: Close Door\n\r", ch );
            ch_printf( ch, "%sValue[1] - In Room: &c%d\n\r", olc ? "&gF&w) " : "&w", obj->value[1] );
            ch_printf( ch, "To the: %s\n\r",
                       IS_SET( obj->value[0], TRIG_D_NORTH ) ? "North" :
                       IS_SET( obj->value[0], TRIG_D_SOUTH ) ? "South" :
                       IS_SET( obj->value[0], TRIG_D_EAST ) ? "East" :
                       IS_SET( obj->value[0], TRIG_D_WEST ) ? "West" :
                       IS_SET( obj->value[0], TRIG_D_UP ) ? "UP" :
                       IS_SET( obj->value[0], TRIG_D_DOWN ) ? "Down" : "Unknown" );
            if( IS_SET( obj->value[0], TRIG_PASSAGE ) )
               ch_printf( ch, "%sValue[2] - To Room: &c%d\n\r", olc ? "&gG&w) " : "&w", obj->value[2] );
         }
         break;
      case ITEM_BLOOD:
         ch_printf( ch, "%sValue[1] - Amount Remaining: &c%d\n\r", olc ? "&gF&w) " : "&w", obj->value[1] );
         if( obj->timer )
            ch_printf( ch, "Object Timer, Time Left: %d\n\r", obj->timer );
         break;
      case ITEM_CAMPGEAR:
         ch_printf( ch, "%sValue[0] - Type of Gear (%d): &c", olc ? "&gE&w) " : "&w", obj->value[0] );
         switch ( obj->value[0] )
         {
            default:
               send_to_char( "Unknown\n\r", ch );
               break;

            case 1:
               send_to_char( "Bedroll\n\r", ch );
               break;

            case 2:
               send_to_char( "Misc. Gear\n\r", ch );
               break;

            case 3:
               send_to_char( "Firewood\n\r", ch );
               break;
         }
         break;
      case ITEM_ORE:
         ch_printf( ch, "%sValue[0] - Type of Ore (%d): &c", olc ? "&gE&w) " : "&w", obj->value[0] );
         switch ( obj->value[0] )
         {
            default:
               send_to_char( "Unknown\n\r", ch );
               break;

            case 1:
               send_to_char( "Iron\n\r", ch );
               break;

            case 2:
               send_to_char( "Gold\n\r", ch );
               break;

            case 3:
               send_to_char( "Silver\n\r", ch );
               break;

            case 4:
               send_to_char( "Adamantite\n\r", ch );
               break;

            case 5:
               send_to_char( "Mithril\n\r", ch );
               break;

            case 6:
               send_to_char( "Blackmite\n\r", ch );
               break;
         }
         ch_printf( ch, "%sValue[1] - Purity: &c%d", olc ? "&gF&w) " : "&w", obj->value[1] );
         break;
      case ITEM_PIECE:
         ch_printf( ch, "%sValue[0] - Obj Vnum for Other Half: &c%d", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch_printf( ch, "%sValue[1] - Obj Vnum for Combined Object: &c%d", olc ? "&gF&w) " : "&w", obj->value[1] );
         break;
   }
}

void cleanup_olc( DESCRIPTOR_DATA * d )
{
   if( d->olc )
   {
      if( d->character )
      {
         d->character->pcdata->dest_buf = NULL;
         act( AT_ACTION, "$n stops using OLC.", d->character, NULL, NULL, TO_CANSEE );
      }
      d->connected = CON_PLAYING;
      DISPOSE( d->olc );
   }
   return;
}

/*
 * Starts it all off
 */
CMDF do_ooedit( CHAR_DATA * ch, char *argument )
{
   DESCRIPTOR_DATA *d;
   OBJ_DATA *obj;

   if( IS_NPC( ch ) )
   {
      send_to_char( "I don't think so...\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "OEdit what?\n\r", ch );
      return;
   }
   if( !( obj = get_obj_world( ch, argument ) ) )
   {
      send_to_char( "Nothing like that in hell, earth, or heaven.\n\r", ch );
      return;
   }

   /*
    * Make sure the object isnt already being edited 
    */
   for( d = first_descriptor; d; d = d->next )
      if( d->connected == CON_OEDIT )
         if( d->olc && OLC_VNUM( d ) == obj->pIndexData->vnum )
         {
            ch_printf( ch, "That object is currently being edited by %s.\n\r", d->character->name );
            return;
         }

   if( !can_omodify( ch, obj ) )
      return;

   d = ch->desc;
   CREATE( d->olc, OLC_DATA, 1 );
   OLC_VNUM( d ) = obj->pIndexData->vnum;
   OLC_CHANGE( d ) = FALSE;
   OLC_VAL( d ) = 0;
   d->character->pcdata->dest_buf = obj;
   d->connected = CON_OEDIT;
   oedit_disp_menu( d );

   act( AT_ACTION, "$n starts using OLC.", ch, NULL, NULL, TO_CANSEE );
   return;
}

CMDF do_ocopy( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   int ovnum, cvnum;
   OBJ_INDEX_DATA *orig;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: ocopy <original> <new>\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: ocopy <original> <new>\n\r", ch );
      return;
   }

   if( !is_number( arg1 ) || !is_number( argument ) )
   {
      send_to_char( "Values must be numeric.\n\r", ch );
      return;
   }

   ovnum = atoi( arg1 );
   cvnum = atoi( argument );

   if( get_trust( ch ) < LEVEL_GREATER )
   {
      AREA_DATA *pArea;

      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         send_to_char( "You must have an assigned area to copy objects.\n\r", ch );
         return;
      }
      if( cvnum < pArea->low_vnum || cvnum > pArea->hi_vnum )
      {
         send_to_char( "That number is not in your allocated range.\n\r", ch );
         return;
      }
   }

   if( get_obj_index( cvnum ) )
   {
      send_to_char( "Target vnum already exists.\n\r", ch );
      return;
   }

   if( ( orig = get_obj_index( ovnum ) ) == NULL )
   {
      send_to_char( "Source vnum does not exist.\n\r", ch );
      return;
   }
   make_object( cvnum, ovnum, orig->name );
   send_to_char( "Object copied.\n\r", ch );
   return;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/* For container flags */
void oedit_disp_container_flags_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   int i;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( i = 0; i < MAX_CONT_FLAG; i++ )
      ch_printf( d->character, "&g%d&w) %s\n\r", i + 1, container_flags[i] );
   ch_printf( d->character, "Container flags: &c%s&w\n\r", flag_string( obj->value[1], container_flags ) );

   send_to_char( "Enter flag, 0 to quit : ", d->character );
}

/* For furniture flags */
void oedit_disp_furniture_flags_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   int i;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( i = 0; i < MAX_FURNFLAG; i++ )
      ch_printf( d->character, "&g%d&w) %s\n\r", i + 1, furniture_flags[i] );
   ch_printf( d->character, "Furniture flags: &c%s&w\n\r", flag_string( obj->value[2], furniture_flags ) );

   send_to_char( "Enter flag, 0 to quit : ", d->character );
}

/*
 * Display lever flags menu
 */
void oedit_disp_lever_flags_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   int counter;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < MAX_TRIGFLAG; counter++ )
      ch_printf( d->character, "&g%2d&w) %s\n\r", counter + 1, trig_flags[counter] );
   ch_printf( d->character, "Lever flags: &c%s&w\n\rEnter flag, 0 to quit: ", flag_string( obj->value[0], trig_flags ) );
   return;
}

/*
 * Fancy layering stuff, trying to lessen confusion :)
 */
void oedit_disp_layer_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;

   OLC_MODE( d ) = OEDIT_LAYERS;
   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   send_to_char( "Choose which layer, or combination of layers fits best: \n\r\n\r", d->character );
   ch_printf( d->character, "[&c%s&w] &g1&w) Nothing Layers\n\r", ( obj->pIndexData->layers == 0 ) ? "X" : " " );
   ch_printf( d->character, "[&c%s&w] &g2&w) Silk Shirt\n\r", IS_SET( obj->pIndexData->layers, 1 ) ? "X" : " " );
   ch_printf( d->character, "[&c%s&w] &g3&w) Leather Vest\n\r", IS_SET( obj->pIndexData->layers, 2 ) ? "X" : " " );
   ch_printf( d->character, "[&c%s&w] &g4&w) Light Chainmail\n\r", IS_SET( obj->pIndexData->layers, 4 ) ? "X" : " " );
   ch_printf( d->character, "[&c%s&w] &g5&w) Leather Jacket\n\r", IS_SET( obj->pIndexData->layers, 8 ) ? "X" : " " );
   ch_printf( d->character, "[&c%s&w] &g6&w) Light Cloak\n\r", IS_SET( obj->pIndexData->layers, 16 ) ? "X" : " " );
   ch_printf( d->character, "[&c%s&w] &g7&w) Loose Cloak\n\r", IS_SET( obj->pIndexData->layers, 32 ) ? "X" : " " );
   ch_printf( d->character, "[&c%s&w] &g8&w) Cape\n\r", IS_SET( obj->pIndexData->layers, 64 ) ? "X" : " " );
   ch_printf( d->character, "[&c%s&w] &g9&w) Magical Effects\n\r", IS_SET( obj->pIndexData->layers, 128 ) ? "X" : " " );
   send_to_char( "\n\rLayer or 0 to exit: ", d->character );
}

/* For extra descriptions */
void oedit_disp_extradesc_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   int count = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   if( obj->pIndexData->first_extradesc )
   {
      EXTRA_DESCR_DATA *ed;
      for( ed = obj->pIndexData->first_extradesc; ed; ed = ed->next )
         ch_printf( d->character, "&g%2d&w) Keyword: &O%s\n\r", ++count, ed->keyword );
   }
   if( obj->first_extradesc )
   {
      EXTRA_DESCR_DATA *ed;
      for( ed = obj->first_extradesc; ed; ed = ed->next )
         ch_printf( d->character, "&g%2d&w) Keyword: &O%s\n\r", ++count, ed->keyword );
   }

   if( obj->pIndexData->first_extradesc || obj->first_extradesc )
      send_to_char( "\n\r", d->character );

   send_to_char( "&gA&w) Add a new description\n\r", d->character );
   send_to_char( "&gR&w) Remove a description\n\r", d->character );
   send_to_char( "&gQ&w) Quit\n\r", d->character );
   send_to_char( "\n\rEnter choice: ", d->character );

   OLC_MODE( d ) = OEDIT_EXTRADESC_MENU;
}

void oedit_disp_extra_choice( DESCRIPTOR_DATA * d )
{
   EXTRA_DESCR_DATA *ed = ( EXTRA_DESCR_DATA * ) d->character->pcdata->spare_ptr;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   ch_printf( d->character, "&g1&w) Keyword: &O%s\n\r", ed->keyword );
   ch_printf( d->character, "&g2&w) Description: \n\r&O%s&w\n\r",
              ( ed->extradesc && ed->extradesc[0] != '\0' ) ? ed->extradesc : "(none)" );
   send_to_char( "&gQ&w) Quit\n\r", d->character );

   OLC_MODE( d ) = OEDIT_EXTRADESC_CHOICE;
}

/* Ask for *which* apply to edit and prompt for some other options */
void oedit_disp_prompt_apply_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   AFFECT_DATA *paf;
   int counter = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
   {
      ch_printf( d->character, " &g%2d&w) ", counter++ );
      showaffect( d->character, paf );
   }

   for( paf = obj->first_affect; paf; paf = paf->next )
   {
      ch_printf( d->character, " &g%2d&w) ", counter++ );
      showaffect( d->character, paf );
   }
   send_to_char( " \n\r &gA&w) Add an affect\n\r", d->character );
   send_to_char( " &gR&w) Remove an affect\n\r", d->character );
   send_to_char( " &gQ&w) Quit\n\r", d->character );

   send_to_char( "\n\rEnter option or affect#: ", d->character );
   OLC_MODE( d ) = OEDIT_AFFECT_MENU;

   return;
}

/*. Ask for liquid type .*/
void oedit_liquid_type( DESCRIPTOR_DATA * d )
{
   int counter, i, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );

   counter = 0;
   for( i = 0; i < MAX_CONDS; i++ )
      if( liquid_table[i] )
      {
         ch_printf( d->character, " &w%2d&g ) &c%-20.20s ", counter, liquid_table[i]->name );

         if( ++col % 3 == 0 )
            send_to_char( "\n\r", d->character );
         counter++;
      }

   send_to_char( "\n\r&wEnter drink type: ", d->character );
   OLC_MODE( d ) = OEDIT_VALUE_2;

   return;
}

/*
 * Display the menu of apply types
 */
void oedit_disp_affect_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < MAX_APPLY_TYPE; counter++ )
   {
      /*
       * Don't want people choosing these ones 
       */
      if( counter == 0 || counter == APPLY_EXT_AFFECT )
         continue;

      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, a_types[counter] );
      if( ++col % 3 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter apply type (0 to quit): ", d->character );
   OLC_MODE( d ) = OEDIT_AFFECT_LOCATION;

   return;
}

/*
 * Display menu of projectile types
 */
void oedit_disp_proj_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < PROJ_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, projectiles[counter] );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter projectile type: ", d->character );
   return;
}

/*
 * Display menu of weapongen types
 */
void oedit_disp_wgen_type_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < TWTP_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, weapon_type[counter].name );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter weapon type: ", d->character );
   return;
}

/*
 * Display menu of armorgen types
 */
void oedit_disp_agen_type_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < TATP_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, armor_type[counter].name );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter armor type: ", d->character );
   return;
}

/*
 * Display menu of weapon/armorgen materials
 */
void oedit_disp_gen_material_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < TMAT_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, armor_materials[counter].name );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter material type: ", d->character );
   return;
}

/*
 * Display menu of weapongen qualities
 */
void oedit_disp_wgen_qual_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < TQUAL_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, weapon_quality[counter] );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter quality: ", d->character );
   return;
}

/*
 * Display menu of damage types
 */
void oedit_disp_damage_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < DAM_MAX_TYPE; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, attack_table[counter] );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter damage type: ", d->character );
   return;
}

/*
 * Display menu of trap types
 */
void oedit_disp_traptype_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter <= TRAP_TYPE_SEX_CHANGE; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, trap_types[counter] );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter trap type: ", d->character );
   return;
}

/*
 * Display trap flags menu
 */
void oedit_disp_trapflags( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter <= TRAPFLAG_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter + 1, capitalize( trap_flags[counter] ) );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   ch_printf( d->character, "\n\rTrap flags: &c%s&w\n\rEnter trap flag, 0 to quit:  ",
              flag_string( obj->value[3], trap_flags ) );
   return;
}

/*
 * Display menu of weapon types
 */
void oedit_disp_weapon_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < WEP_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, weapon_skills[counter] );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter weapon type: ", d->character );
   return;
}

/*
 * Display menu of campgear types
 */
void oedit_disp_gear_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < GEAR_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, campgear[counter] );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter gear type: ", d->character );
   return;
}

/*
 * Display menu of ore types
 */
void oedit_disp_ore_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < ORE_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, ores[counter] );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter ore type: ", d->character );
   return;
}

/* spell type */
void oedit_disp_spells_menu( DESCRIPTOR_DATA * d )
{
   send_to_char( "Enter the name of the spell: ", d->character );
}

/* object value 0 */
void oedit_disp_val0_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_0;

   switch ( obj->item_type )
   {
      case ITEM_SALVE:
      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_WAND:
      case ITEM_STAFF:
      case ITEM_POTION:
         send_to_char( "Spell level : ", d->character );
         break;
      case ITEM_MISSILE_WEAPON:
      case ITEM_WEAPON:
      case ITEM_PROJECTILE:
         send_to_char( "Condition : ", d->character );
         break;
      case ITEM_ARMOR:
         send_to_char( "Current AC : ", d->character );
         break;
      case ITEM_QUIVER:
      case ITEM_KEYRING:
      case ITEM_PIPE:
      case ITEM_CONTAINER:
      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
         send_to_char( "Capacity : ", d->character );
         break;
      case ITEM_FOOD:
      case ITEM_COOK:
         send_to_char( "Hours to fill stomach : ", d->character );
         break;
      case ITEM_MONEY:
         send_to_char( "Amount of gold coins : ", d->character );
         break;
      case ITEM_LEVER:
      case ITEM_SWITCH:
         oedit_disp_lever_flags_menu( d );
         break;
      case ITEM_TRAP:
         send_to_char( "Charges: ", d->character );
         break;
      case ITEM_KEY:
         send_to_char( "Room lock vnum: ", d->character );
         break;
      case ITEM_ORE:
         oedit_disp_ore_menu( d );
         break;
      case ITEM_CAMPGEAR:
         oedit_disp_gear_menu( d );
         break;
      case ITEM_PIECE:
         send_to_char( "Vnum for second half of object: ", d->character );
         break;
      default:
         oedit_disp_menu( d );
   }
}

/* object value 1 */
void oedit_disp_val1_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_1;

   switch ( obj->item_type )
   {
      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
         oedit_disp_spells_menu( d );
         break;

      case ITEM_SALVE:
      case ITEM_HERB:
         send_to_char( "Charges: ", d->character );
         break;

      case ITEM_PIPE:
         send_to_char( "Number of draws: ", d->character );
         break;

      case ITEM_WAND:
      case ITEM_STAFF:
         send_to_char( "Max number of charges : ", d->character );
         break;

      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
      case ITEM_PROJECTILE:
         send_to_char( "Number of damage dice : ", d->character );
         break;
      case ITEM_FOOD:
      case ITEM_COOK:
         send_to_char( "Condition: ", d->character );
         break;

      case ITEM_CONTAINER:
         oedit_disp_container_flags_menu( d );
         break;

      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
         send_to_char( "Quantity : ", d->character );
         break;

      case ITEM_ARMOR:
         send_to_char( "Original AC: ", d->character );
         break;

      case ITEM_LEVER:
      case ITEM_SWITCH:
         if( IS_SET( obj->value[0], TRIG_CAST ) )
            oedit_disp_spells_menu( d );
         else
            send_to_char( "Vnum: ", d->character );
         break;

      case ITEM_ORE:
         send_to_char( "Purity: ", d->character );
         break;

      case ITEM_PIECE:
         send_to_char( "Vnum for complete assembled object: ", d->character );
         break;

      case ITEM_TRAP:
         oedit_disp_traptype_menu( d );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 2 */
void oedit_disp_val2_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_2;

   switch ( obj->item_type )
   {
      case ITEM_LIGHT:
         send_to_char( "Number of hours (0 = burnt, -1 is infinite) : ", d->character );
         break;
      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
         oedit_disp_spells_menu( d );
         break;
      case ITEM_WAND:
      case ITEM_STAFF:
         send_to_char( "Number of charges remaining : ", d->character );
         break;
      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
      case ITEM_PROJECTILE:
         send_to_char( "Size of damage dice : ", d->character );
         break;
      case ITEM_CONTAINER:
         send_to_char( "Vnum of key to open container (-1 for no key) : ", d->character );
         break;
      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
         oedit_liquid_type( d );
         break;
      case ITEM_ARMOR:
         send_to_char( "Available sockets: ", d->character );
         break;
      case ITEM_FURNITURE:
         oedit_disp_furniture_flags_menu( d );
         break;
      default:
         oedit_disp_menu( d );
   }
}

/* object value 3 */
void oedit_disp_val3_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_3;

   switch ( obj->item_type )
   {
      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_WAND:
      case ITEM_STAFF:
         oedit_disp_spells_menu( d );
         break;
      case ITEM_WEAPON:
      case ITEM_PROJECTILE:
         oedit_disp_damage_menu( d );
         break;

      case ITEM_ARMOR:
         oedit_disp_agen_type_menu( d );
         break;

      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
      case ITEM_FOOD:
         send_to_char( "Poisoned (0 = not poisoned) : ", d->character );
         break;

      case ITEM_TRAP:
         oedit_disp_trapflags( d );
         OLC_MODE( d ) = OEDIT_TRAPFLAGS;
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 4 */
void oedit_disp_val4_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_4;

   switch ( obj->item_type )
   {
      case ITEM_SALVE:
         oedit_disp_spells_menu( d );
         break;
      case ITEM_FOOD:
      case ITEM_COOK:
         send_to_char( "Food value: ", d->character );
         break;
      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
         oedit_disp_weapon_menu( d );
         break;
      case ITEM_PROJECTILE:
         oedit_disp_proj_menu( d );
         break;

      case ITEM_ARMOR:
         oedit_disp_gen_material_menu( d );
         break;

      case ITEM_KEY:
         send_to_char( "Durability: ", d->character );
         break;

      case ITEM_TRAP:
         send_to_char( "Minimum Damage: ", d->character );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 5 */
void oedit_disp_val5_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_5;

   switch ( obj->item_type )
   {
      case ITEM_SALVE:
         oedit_disp_spells_menu( d );
         break;
      case ITEM_MISSILE_WEAPON:
         oedit_disp_proj_menu( d );
         break;
      case ITEM_KEY:
         send_to_char( "Container lock vnum: ", d->character );
         break;
      case ITEM_PROJECTILE:
         send_to_char( "Current condition: ", d->character );
         break;

      case ITEM_TRAP:
         send_to_char( "Maximum Damage: ", d->character );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 6 */
void oedit_disp_val6_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_6;

   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
         send_to_char( "Current condition: ", d->character );
         break;
      default:
         oedit_disp_menu( d );
   }
}

/* object value 7 */
void oedit_disp_val7_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_7;

   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
         send_to_char( "Available sockets: ", d->character );
         break;
      default:
         oedit_disp_menu( d );
   }
}

/* object value 8 */
void oedit_disp_val8_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_8;

   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
         oedit_disp_wgen_type_menu( d );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 9 */
void oedit_disp_val9_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_9;

   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
         oedit_disp_gen_material_menu( d );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 10 */
void oedit_disp_val10_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_10;

   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
         oedit_disp_wgen_qual_menu( d );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object type */
void oedit_disp_type_menu( DESCRIPTOR_DATA * d )
{
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < MAX_ITEM_TYPE; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, o_types[counter] );
      if( ++col % 3 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter object type: ", d->character );
   return;
}

/* object extra flags */
void oedit_disp_extra_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < MAX_ITEM_FLAG; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter + 1, capitalize( o_flags[counter] ) );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   ch_printf( d->character, "\n\rObject flags: &c%s&w\n\rEnter object extra flag (0 to quit): ",
              ext_flag_string( &obj->extra_flags, o_flags ) );
   return;
}

/*
 * Display wear flags menu
 */
void oedit_disp_wear_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   int counter, col = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter <= MAX_WEAR_FLAG; counter++ )
   {
      if( 1 << counter == ITEM_DUAL_WIELD )
         continue;

      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter + 1, capitalize( w_flags[counter] ) );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   ch_printf( d->character, "\n\rWear flags: &c%s&w\n\rEnter wear flag, 0 to quit:  ",
              flag_string( obj->wear_flags, w_flags ) );
   return;
}

/* display main menu */
void oedit_disp_menu( DESCRIPTOR_DATA * d )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;

   /*
    * Ominous looking ANSI code of some sort - perhaps there's some way to use this elsewhere ? 
    */
   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );

   /*
    * . Build first half of menu .
    */
   ch_printf( d->character,
              "&w-- Item number : [&c%d&w]\r\n"
              "&g1&w) Name     : &O%s\r\n"
              "&g2&w) S-Desc   : &O%s\r\n"
              "&g3&w) L-Desc   :-\r\n&O%s\r\n"
              "&g4&w) A-Desc   :-\r\n&O%s\n\r"
              "&g5&w) Type        : &c%s\r\n"
              "&g6&w) Extra flags : &c%s\r\n",
              obj->pIndexData->vnum, obj->name, obj->short_descr, obj->objdesc,
              obj->action_desc ? obj->action_desc : "<not set>\r\n",
              capitalize( item_type_name( obj ) ), ext_flag_string( &obj->extra_flags, o_flags ) );

   /*
    * Build second half of the menu 
    */
   ch_printf( d->character, "&g7&w) Wear flags  : &c%s\n\r" "&g8&w) Weight      : &c%d\n\r" "&g9&w) Cost        : &c%d\n\r" "&gA&w) Rent        : &c%d\n\r" "&gB&w) Timer       : &c%d\n\r" "&gC&w) Level       : &c%d\n\r"   /* -- Object level . */
              "&gD&w) Layers      : &c%d\n\r",
              flag_string( obj->wear_flags, w_flags ),
              obj->weight, obj->cost, obj->rent, obj->timer, obj->level, obj->pIndexData->layers );

   ostat_plus( d->character, obj, TRUE );

   send_to_char( "&gP&w) Affect menu\n\r"
                 "&gR&w) Extra descriptions menu\n\r" "&gQ&w) Quit\n\r" "Enter choice : ", d->character );

   OLC_MODE( d ) = OEDIT_MAIN_MENU;

   return;
}

/***************************************************************************
 Object affect editing/removing functions
 ***************************************************************************/
void edit_object_affect( DESCRIPTOR_DATA * d, int number )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   int count = 0;
   AFFECT_DATA *paf;

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
   {
      if( count == number )
      {
         d->character->pcdata->spare_ptr = paf;
         OLC_VAL( d ) = TRUE;
         oedit_disp_affect_menu( d );
         return;
      }
      count++;
   }
   for( paf = obj->first_affect; paf; paf = paf->next )
   {
      if( count == number )
      {
         d->character->pcdata->spare_ptr = paf;
         OLC_VAL( d ) = TRUE;
         oedit_disp_affect_menu( d );
         return;
      }
      count++;
   }
   send_to_char( "Affect not found.\n\r", d->character );
   return;
}

void remove_affect_from_obj( OBJ_DATA * obj, int number )
{
   int count = 0;
   AFFECT_DATA *paf;

   if( obj->pIndexData->first_affect )
   {
      for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      {
         if( count == number )
         {
            UNLINK( paf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next, prev );
            DISPOSE( paf );
            --top_affect;
            return;
         }
         count++;
      }
   }

   if( obj->first_affect )
   {
      for( paf = obj->first_affect; paf; paf = paf->next )
      {
         if( count == number )
         {
            UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
            DISPOSE( paf );
            --top_affect;
            return;
         }
         count++;
      }
   }
   return;
}

EXTRA_DESCR_DATA *oedit_find_extradesc( OBJ_DATA * obj, int number )
{
   int count = 0;
   EXTRA_DESCR_DATA *ed;

   for( ed = obj->pIndexData->first_extradesc; ed; ed = ed->next )
   {
      if( ++count == number )
         return ed;
   }

   for( ed = obj->first_extradesc; ed; ed = ed->next )
   {
      if( ++count == number )
         return ed;
   }

   return NULL;
}

/*
 * Bogus command for resetting stuff
 */
CMDF do_oedit_reset( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) ch->pcdata->dest_buf;
   EXTRA_DESCR_DATA *ed = ( EXTRA_DESCR_DATA * ) ch->pcdata->spare_ptr;

   switch ( ch->substate )
   {
      default:
         return;

      case SUB_OBJ_EXTRA:
         if( !ch->pcdata->dest_buf )
         {
            send_to_char( "Fatal error, report to Samson.\n\r", ch );
            bug( "%s", "do_oedit_reset: sub_obj_extra: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            return;
         }
         STRFREE( ed->extradesc );
         ed->extradesc = copy_buffer( ch );
         stop_editing( ch );
         ch->pcdata->dest_buf = obj;
         ch->pcdata->spare_ptr = ed;
         ch->substate = SUB_NONE;
         ch->desc->connected = CON_OEDIT;
         OLC_MODE( ch->desc ) = OEDIT_EXTRADESC_CHOICE;
         oedit_disp_extra_choice( ch->desc );
         return;

      case SUB_OBJ_LONG:
         if( !ch->pcdata->dest_buf )
         {
            send_to_char( "Fatal error, report to Samson.\n\r", ch );
            bug( "%s", "do_oedit_reset: sub_obj_long: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            return;
         }
         STRFREE( obj->objdesc );
         obj->objdesc = copy_buffer( ch );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->objdesc );
            obj->pIndexData->objdesc = QUICKLINK( obj->objdesc );
         }
         stop_editing( ch );
         ch->pcdata->dest_buf = obj;
         ch->desc->connected = CON_OEDIT;
         ch->substate = SUB_NONE;
         OLC_MODE( ch->desc ) = OEDIT_MAIN_MENU;
         oedit_disp_menu( ch->desc );
         return;
   }
}

/*
 * This function interprets the arguments that the character passed
 * to it based on which OLC mode you are in at the time
 */
void oedit_parse( DESCRIPTOR_DATA * d, char *arg )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   AFFECT_DATA *paf = ( AFFECT_DATA * ) d->character->pcdata->spare_ptr;
   AFFECT_DATA *npaf;
   EXTRA_DESCR_DATA *ed = ( EXTRA_DESCR_DATA * ) d->character->pcdata->spare_ptr;
   char arg1[MIL];
   int number = 0, max_val, min_val, value;
   /*
    * bool found; 
    */

   switch ( OLC_MODE( d ) )
   {
      case OEDIT_MAIN_MENU:
         /*
          * switch to whichever mode the user selected, display prompt or menu 
          */
         switch ( UPPER( arg[0] ) )
         {
            case 'Q':
               cleanup_olc( d );
               return;
            case '1':
               send_to_char( "Enter namelist : ", d->character );
               OLC_MODE( d ) = OEDIT_EDIT_NAMELIST;
               break;
            case '2':
               send_to_char( "Enter short desc : ", d->character );
               OLC_MODE( d ) = OEDIT_SHORTDESC;
               break;
            case '3':
               send_to_char( "Enter long desc :-\r\n| ", d->character );
               OLC_MODE( d ) = OEDIT_LONGDESC;
               break;
            case '4':
               /*
                * lets not 
                */
               send_to_char( "Enter action desc :-\r\n", d->character );
               OLC_MODE( d ) = OEDIT_ACTDESC;
               break;
            case '5':
               oedit_disp_type_menu( d );
               OLC_MODE( d ) = OEDIT_TYPE;
               break;
            case '6':
               oedit_disp_extra_menu( d );
               OLC_MODE( d ) = OEDIT_EXTRAS;
               break;
            case '7':
               oedit_disp_wear_menu( d );
               OLC_MODE( d ) = OEDIT_WEAR;
               break;
            case '8':
               send_to_char( "Enter weight : ", d->character );
               OLC_MODE( d ) = OEDIT_WEIGHT;
               break;
            case '9':
               send_to_char( "Enter cost : ", d->character );
               OLC_MODE( d ) = OEDIT_COST;
               break;
            case 'A':
               send_to_char( "Enter cost per day : ", d->character );
               OLC_MODE( d ) = OEDIT_COSTPERDAY;
               break;
            case 'B':
               send_to_char( "Enter timer : ", d->character );
               OLC_MODE( d ) = OEDIT_TIMER;
               break;
            case 'C':
               send_to_char( "Enter level : ", d->character );
               OLC_MODE( d ) = OEDIT_LEVEL;
               break;
            case 'D':
               if( IS_WEAR_FLAG( obj, ITEM_WEAR_BODY )
                   || IS_WEAR_FLAG( obj, ITEM_WEAR_ABOUT )
                   || IS_WEAR_FLAG( obj, ITEM_WEAR_ARMS )
                   || IS_WEAR_FLAG( obj, ITEM_WEAR_FEET )
                   || IS_WEAR_FLAG( obj, ITEM_WEAR_HANDS )
                   || IS_WEAR_FLAG( obj, ITEM_WEAR_LEGS ) || IS_WEAR_FLAG( obj, ITEM_WEAR_WAIST ) )
               {
                  oedit_disp_layer_menu( d );
                  OLC_MODE( d ) = OEDIT_LAYERS;
               }
               else
                  send_to_char( "The wear location of this object is not layerable.\n\r", d->character );
               break;
            case 'E':
               oedit_disp_val0_menu( d );
               break;
            case 'F':
               oedit_disp_val1_menu( d );
               break;
            case 'G':
               oedit_disp_val2_menu( d );
               break;
            case 'H':
               oedit_disp_val3_menu( d );
               break;
            case 'I':
               oedit_disp_val4_menu( d );
               break;
            case 'J':
               oedit_disp_val5_menu( d );
               break;
            case 'K':
               oedit_disp_val6_menu( d );
               break;
            case 'L':
               oedit_disp_val7_menu( d );
               break;
            case 'M':
               oedit_disp_val8_menu( d );
               break;
            case 'N':
               oedit_disp_val9_menu( d );
               break;
            case 'O':
               oedit_disp_val10_menu( d );
               break;
            case 'P':
               oedit_disp_prompt_apply_menu( d );
               break;
            case 'R':
               oedit_disp_extradesc_menu( d );
               break;
            default:
               oedit_disp_menu( d );
               break;
         }
         return;  /* end of OEDIT_MAIN_MENU */

      case OEDIT_EDIT_NAMELIST:
         STRFREE( obj->name );
         obj->name = STRALLOC( arg );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->name );
            obj->pIndexData->name = QUICKLINK( obj->name );
         }
         olc_log( d, "Changed name to %s", obj->name );
         break;

      case OEDIT_SHORTDESC:
         STRFREE( obj->short_descr );
         obj->short_descr = STRALLOC( arg );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->short_descr );
            obj->pIndexData->short_descr = QUICKLINK( obj->short_descr );
         }
         olc_log( d, "Changed short to %s", obj->short_descr );
         break;

      case OEDIT_LONGDESC:
         STRFREE( obj->objdesc );
         obj->objdesc = STRALLOC( arg );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->objdesc );
            obj->pIndexData->objdesc = QUICKLINK( obj->objdesc );
         }
         olc_log( d, "Changed longdesc to %s", obj->objdesc );
         break;

      case OEDIT_ACTDESC:
         STRFREE( obj->action_desc );
         obj->action_desc = STRALLOC( arg );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->action_desc );
            obj->pIndexData->action_desc = QUICKLINK( obj->action_desc );
         }
         olc_log( d, "Changed actiondesc to %s", obj->action_desc );
         break;

      case OEDIT_TYPE:
         if( is_number( arg ) )
            number = atoi( arg );
         else
            number = get_otype( arg );

         if( ( number < 1 ) || ( number >= MAX_ITEM_TYPE ) )
         {
            send_to_char( "Invalid choice, try again : ", d->character );
            return;
         }
         else
         {
            obj->item_type = number;
            if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
               obj->pIndexData->item_type = obj->item_type;
         }
         olc_log( d, "Changed object type to %s", o_types[number] );
         break;

      case OEDIT_EXTRAS:
         while( arg[0] != '\0' )
         {
            arg = one_argument( arg, arg1 );
            if( is_number( arg1 ) )
            {
               number = atoi( arg1 );

               if( number == 0 )
               {
                  oedit_disp_menu( d );
                  return;
               }
               number -= 1;   /* Offset for 0 */
               if( number < 0 || number > MAX_ITEM_FLAG )
               {
                  oedit_disp_extra_menu( d );
                  return;
               }
            }
            else
            {
               number = get_oflag( arg1 );
               if( number < 0 || number > MAX_BITS )
               {
                  oedit_disp_extra_menu( d );
                  return;
               }
            }

            if( number == ITEM_PROTOTYPE && get_trust( d->character ) < LEVEL_GREATER
                && !hasname( d->character->pcdata->bestowments, "protoflag" ) )
               send_to_char( "You cannot change the prototype flag.\n\r", d->character );
            else
            {
               xTOGGLE_BIT( obj->extra_flags, number );
               olc_log( d, "%s the flag %s", IS_OBJ_FLAG( obj, number ) ? "Added" : "Removed", o_flags[number] );
            }

            /*
             * If you used a number, you can only do one flag at a time 
             */
            if( is_number( arg ) )
               break;
         }
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->extra_flags = obj->extra_flags;
         oedit_disp_extra_menu( d );
         return;

      case OEDIT_WEAR:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
               break;
            else if( number < 0 || number > MAX_WEAR_FLAG )
            {
               send_to_char( "Invalid flag, try again: ", d->character );
               return;
            }
            else
            {
               number -= 1;   /* Offset to accomodate 0 */
               TOGGLE_BIT( obj->wear_flags, 1 << number );
               olc_log( d, "%s the wearloc %s", IS_WEAR_FLAG( obj, 1 << number ) ? "Added" : "Removed", w_flags[number] );
            }
         }
         else
         {
            while( arg[0] != '\0' )
            {
               arg = one_argument( arg, arg1 );
               number = get_wflag( arg1 );
               if( number != -1 )
               {
                  TOGGLE_BIT( obj->wear_flags, 1 << number );
                  olc_log( d, "%s the wearloc %s", IS_WEAR_FLAG( obj, 1 << number ) ? "Added" : "Removed", w_flags[number] );
               }
            }
         }
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->wear_flags = obj->wear_flags;
         oedit_disp_wear_menu( d );
         return;

      case OEDIT_WEIGHT:
         number = atoi( arg );
         obj->weight = number;
         olc_log( d, "Changed weight to %d", obj->weight );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->weight = obj->weight;
         break;

      case OEDIT_COST:
         number = atoi( arg );
         obj->cost = number;
         olc_log( d, "Changed cost to %d", obj->cost );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->cost = obj->cost;
         break;

      case OEDIT_COSTPERDAY:
         number = atoi( arg );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
         {
            obj->pIndexData->rent = number;
            obj->rent = number;
            olc_log( d, "Changed rent to %d", obj->pIndexData->rent );
            if( number == -2 )
               obj->rent = set_obj_rent( obj->pIndexData );
            if( obj->rent == -2 )
               olc_log( d, "%s", "&YWARNING: This object exceeds allowable rent specs.\n\r" );
         }
         else
         {
            obj->rent = number;
            if( number == -2 )
               obj->rent = set_obj_rent( obj->pIndexData );
            olc_log( d, "Changed rent to %d", obj->rent );
            if( obj->rent == -2 )
               olc_log( d, "%s", "&YWARNING: This object exceeds allowable rent specs.\n\r" );
         }
         break;

      case OEDIT_TIMER:
         number = atoi( arg );
         obj->timer = number;
         olc_log( d, "Changed timer to %d", obj->timer );
         break;

      case OEDIT_LEVEL:
         number = atoi( arg );
         obj->level = URANGE( 0, number, MAX_LEVEL );
         olc_log( d, "Changed object level to %d", obj->level );
         break;

      case OEDIT_LAYERS:
         /*
          * Like they say, easy on the user, hard on the programmer :) 
          */
         /*
          * Or did I just make that up.... 
          */
         number = atoi( arg );
         switch ( number )
         {
            case 0:
               oedit_disp_menu( d );
               return;
            case 1:
               obj->pIndexData->layers = 0;
               break;
            case 2:
               TOGGLE_BIT( obj->pIndexData->layers, 1 );
               break;
            case 3:
               TOGGLE_BIT( obj->pIndexData->layers, 2 );
               break;
            case 4:
               TOGGLE_BIT( obj->pIndexData->layers, 4 );
               break;
            case 5:
               TOGGLE_BIT( obj->pIndexData->layers, 8 );
               break;
            case 6:
               TOGGLE_BIT( obj->pIndexData->layers, 16 );
               break;
            case 7:
               TOGGLE_BIT( obj->pIndexData->layers, 32 );
               break;
            case 8:
               TOGGLE_BIT( obj->pIndexData->layers, 64 );
               break;
            case 9:
               TOGGLE_BIT( obj->pIndexData->layers, 128 );
               break;
            default:
               send_to_char( "Invalid selection, try again: ", d->character );
               return;
         }
         olc_log( d, "Changed layers to %d", obj->pIndexData->layers );
         oedit_disp_layer_menu( d );
         return;

      case OEDIT_TRAPFLAGS:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
               break;
            else if( number < 0 || number > TRAPFLAG_MAX + 1 )
            {
               send_to_char( "Invalid flag, try again: ", d->character );
               return;
            }
            else
            {
               number -= 1;   /* Offset to accomodate 0 */
               TOGGLE_BIT( obj->value[3], 1 << number );
               olc_log( d, "%s the trapflag %s",
                        IS_SET( obj->value[3], 1 << number ) ? "Added" : "Removed", trap_flags[number] );
            }
         }
         else
         {
            while( arg[0] != '\0' )
            {
               arg = one_argument( arg, arg1 );
               number = get_trapflag( arg1 );
               if( number != -1 )
               {
                  TOGGLE_BIT( obj->value[3], 1 << number );
                  olc_log( d, "%s the trapflag %s",
                           IS_SET( obj->value[3], 1 << number ) ? "Added" : "Removed", trap_flags[number] );
               }
            }
         }
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[3] = obj->value[3];
         oedit_disp_trapflags( d );
         return;

      case OEDIT_VALUE_0:
         number = atoi( arg );
         switch ( obj->item_type )
         {
            case ITEM_LEVER:
            case ITEM_SWITCH:
               if( number < 0 || number > 29 )
                  oedit_disp_lever_flags_menu( d );
               else
               {
                  if( number != 0 )
                  {
                     TOGGLE_BIT( obj->value[0], 1 << ( number - 1 ) );
                     if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
                        TOGGLE_BIT( obj->pIndexData->value[0], 1 << ( number - 1 ) );
                  }
               }
               break;

            default:
               obj->value[0] = number;
               if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
                  obj->pIndexData->value[0] = number;
         }
         olc_log( d, "Changed v0 to %d", obj->value[0] );
         break;

      case OEDIT_VALUE_1:
         number = atoi( arg );
         switch ( obj->item_type )
         {
            case ITEM_PILL:
            case ITEM_SCROLL:
            case ITEM_POTION:
               if( !is_number( arg ) )
                  number = skill_lookup( arg );
               obj->value[1] = number;
               if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
                  obj->pIndexData->value[1] = number;
               break;

            case ITEM_LEVER:
            case ITEM_SWITCH:
               if( IS_SET( obj->value[0], TRIG_CAST ) )
                  number = skill_lookup( arg );
               obj->value[1] = number;
               if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
                  obj->pIndexData->value[1] = number;
               break;

            case ITEM_CONTAINER:
               number = atoi( arg );
               if( number < 0 || number > 31 )
                  oedit_disp_container_flags_menu( d );
               else
               {
                  /*
                   * if 0, quit 
                   */
                  if( number != 0 )
                  {
                     number = 1 << ( number - 1 );
                     TOGGLE_BIT( obj->value[1], number );
                     if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
                        TOGGLE_BIT( obj->pIndexData->value[1], number );
                  }
               }
               break;

            default:
               obj->value[1] = number;
               if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
                  obj->pIndexData->value[1] = number;
               break;
         }
         olc_log( d, "Changed v1 to %d", obj->value[1] );
         break;

      case OEDIT_VALUE_2:
         number = atoi( arg );
         /*
          * Some error checking done here 
          */
         switch ( obj->item_type )
         {
            case ITEM_SCROLL:
            case ITEM_POTION:
            case ITEM_PILL:
               min_val = -1;
               max_val = top_sn - 1;
               if( !is_number( arg ) )
                  number = skill_lookup( arg );
               break;

            case ITEM_WEAPON:
               min_val = 1;
               max_val = 100;
               break;
            case ITEM_ARMOR:
               min_val = 0;
               max_val = 3;
               break;
            case ITEM_DRINK_CON:
            case ITEM_FOUNTAIN:
               min_val = 0;
               max_val = top_liquid - 1;
               break;

            case ITEM_FURNITURE:
               min_val = 0;
               max_val = 2147483647;
               number = atoi( arg );
               if( number < 0 || number > 31 )
                  oedit_disp_furniture_flags_menu( d );
               else
               {
                  /*
                   * if 0, quit 
                   */
                  if( number != 0 )
                  {
                     number = 1 << ( number - 1 );
                     TOGGLE_BIT( obj->value[2], number );
                     if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
                        TOGGLE_BIT( obj->pIndexData->value[2], number );
                  }
               }
               break;

            default:
               /*
                * Would require modifying if you have bvnum 
                */
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[2] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v2 to %d", obj->value[2] );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[2] = obj->value[2];
         break;

      case OEDIT_VALUE_3:
         number = atoi( arg );
         switch ( obj->item_type )
         {
            case ITEM_PILL:
            case ITEM_SCROLL:
            case ITEM_POTION:
            case ITEM_WAND:
            case ITEM_STAFF:
               min_val = -1;
               max_val = top_sn - 1;
               if( !is_number( arg ) )
                  number = skill_lookup( arg );
               break;

            case ITEM_WEAPON:
               min_val = 0;
               max_val = DAM_MAX_TYPE - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val3_menu( d );
                  return;
               }
               break;

            case ITEM_ARMOR:
               min_val = 0;
               max_val = TATP_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val3_menu( d );
                  return;
               }
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[3] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v3 to %d", obj->value[3] );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[3] = obj->value[3];
         if( obj->item_type == ITEM_ARMOR && obj->value[3] > 0 && obj->value[4] > 0 )
            armorgen( obj );
         break;

      case OEDIT_VALUE_4:
         number = atoi( arg );
         switch ( obj->item_type )
         {
            case ITEM_SALVE:
               if( !is_number( arg ) )
                  number = skill_lookup( arg );
               min_val = -1;
               max_val = top_sn - 1;
               break;
            case ITEM_FOOD:
               min_val = 0;
               max_val = 32000;
               break;
            case ITEM_WEAPON:
            case ITEM_MISSILE_WEAPON:
               min_val = 0;
               max_val = WEP_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val4_menu( d );
                  return;
               }
               break;

            case ITEM_ARMOR:
               min_val = 0;
               max_val = TMAT_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val4_menu( d );
                  return;
               }
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[4] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v4 to %d", obj->value[4] );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[4] = obj->value[4];
         if( obj->item_type == ITEM_ARMOR && obj->value[3] > 0 && obj->value[4] > 0 )
            armorgen( obj );
         break;

      case OEDIT_VALUE_5:
         number = atoi( arg );
         switch ( obj->item_type )
         {
            case ITEM_SALVE:
               if( !is_number( arg ) )
                  number = skill_lookup( arg );
               min_val = -1;
               max_val = top_sn - 1;
               break;

            case ITEM_MISSILE_WEAPON:
               min_val = 0;
               max_val = PROJ_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val5_menu( d );
                  return;
               }
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         if( obj->item_type == ITEM_CORPSE_PC )
         {
            olc_log( d, "Error - can't change skeleton value on corpses." );
            break;
         }
         obj->value[5] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v5 to %d", obj->value[5] );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[5] = obj->value[5];
         break;

      case OEDIT_VALUE_6:
         number = atoi( arg );
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
            case ITEM_MISSILE_WEAPON:
               min_val = 0;
               max_val = sysdata.initcond;
               if( number < min_val || number > max_val )
                  return;
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[6] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v6 to %d", obj->value[6] );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[6] = obj->value[6];
         break;

      case OEDIT_VALUE_7:
         number = atoi( arg );
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
               min_val = 0;
               max_val = 3;
               if( number < min_val || number > max_val )
                  return;
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[7] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v7 to %d", obj->value[7] );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[7] = obj->value[7];
         break;

      case OEDIT_VALUE_8:
         number = atoi( arg );
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
               min_val = 0;
               max_val = TWTP_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val8_menu( d );
                  return;
               }
               break;
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[8] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v8 to %d", obj->value[8] );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[8] = obj->value[8];
         if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
            weapongen( obj );
         break;

      case OEDIT_VALUE_9:
         number = atoi( arg );
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
               min_val = 0;
               max_val = TMAT_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val9_menu( d );
                  return;
               }
               break;
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[9] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v9 to %d", obj->value[9] );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[9] = obj->value[9];
         if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
            weapongen( obj );
         break;

      case OEDIT_VALUE_10:
         number = atoi( arg );
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
               min_val = 0;
               max_val = TQUAL_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val9_menu( d );
                  return;
               }
               break;
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[10] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v10 to %d", obj->value[10] );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[10] = obj->value[10];
         if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
            weapongen( obj );
         break;

      case OEDIT_AFFECT_MENU:
         number = atoi( arg );

         switch ( arg[0] )
         {
            default:   /* if its a number, then its prolly for editing an affect */
               if( is_number( arg ) )
                  edit_object_affect( d, number );
               else
                  oedit_disp_prompt_apply_menu( d );
               return;

            case 'r':
            case 'R':
               /*
                * Chop off the 'R', if theres a number following use it, otherwise prompt for input 
                */
               arg = one_argument( arg, arg1 );
               if( arg && arg[0] != '\0' )
               {
                  number = atoi( arg );
                  remove_affect_from_obj( obj, number );
                  oedit_disp_prompt_apply_menu( d );
               }
               else
               {
                  send_to_char( "Remove which affect? ", d->character );
                  OLC_MODE( d ) = OEDIT_AFFECT_REMOVE;
               }
               return;

            case 'a':
            case 'A':
               CREATE( paf, AFFECT_DATA, 1 );
               d->character->pcdata->spare_ptr = paf;
               oedit_disp_affect_menu( d );
               return;

            case 'q':
            case 'Q':
               d->character->pcdata->spare_ptr = NULL;
               break;
         }
         break;   /* If we reach here, we're done */

      case OEDIT_AFFECT_LOCATION:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
            {
               /*
                * Junk the affect 
                */
               d->character->pcdata->spare_ptr = NULL;
               DISPOSE( paf );
               break;
            }
         }
         else
            number = get_atype( arg );

         if( number < 0 || number >= MAX_APPLY_TYPE || number == APPLY_EXT_AFFECT )
         {
            send_to_char( "Invalid location, try again: ", d->character );
            return;
         }
         paf->location = number;
         OLC_MODE( d ) = OEDIT_AFFECT_MODIFIER;
         /*
          * Insert all special affect handling here ie: non numerical stuff 
          */
         /*
          * And add the apropriate case statement below 
          */
         if( number == APPLY_AFFECT )
         {
            d->character->tempnum = 0;
            medit_disp_aff_flags( d );
         }
         else if( number == APPLY_RESISTANT || number == APPLY_IMMUNE || number == APPLY_SUSCEPTIBLE )
         {
            d->character->tempnum = 0;
            medit_disp_ris( d );
         }
         else if( number == APPLY_WEAPONSPELL || number == APPLY_WEARSPELL || number == APPLY_REMOVESPELL )
            oedit_disp_spells_menu( d );
         else
            send_to_char( "\n\rModifier: ", d->character );
         return;

      case OEDIT_AFFECT_MODIFIER:
         switch ( paf->location )
         {
            case APPLY_AFFECT:
            case APPLY_RESISTANT:
            case APPLY_IMMUNE:
            case APPLY_SUSCEPTIBLE:
               if( is_number( arg ) )
               {
                  number = atoi( arg );
                  if( number == 0 )
                  {
                     value = d->character->tempnum;
                     break;
                  }
                  TOGGLE_BIT( d->character->tempnum, 1 << ( number - 1 ) );
               }
               else
               {
                  while( arg[0] != '\0' )
                  {
                     arg = one_argument( arg, arg1 );
                     if( paf->location == APPLY_AFFECT )
                        number = get_aflag( arg1 );
                     else
                        number = get_risflag( arg1 );
                     if( number < 0 )
                        ch_printf( d->character, "Invalid flag: %s\n\r", arg1 );
                     else
                        TOGGLE_BIT( d->character->tempnum, 1 << number );
                  }
               }
               if( paf->location == APPLY_AFFECT )
                  medit_disp_aff_flags( d );
               else
                  medit_disp_ris( d );
               return;

            case APPLY_WEAPONSPELL:
            case APPLY_WEARSPELL:
            case APPLY_REMOVESPELL:
               if( is_number( arg ) )
               {
                  number = atoi( arg );
                  if( IS_VALID_SN( number ) )
                     value = number;
                  else
                  {
                     send_to_char( "Invalid sn, try again: ", d->character );
                     return;
                  }
               }
               else
               {
                  value = bsearch_skill_exact( arg, gsn_first_spell, gsn_first_skill - 1 );
                  if( value < 0 )
                  {
                     ch_printf( d->character, "Invalid spell %s, try again: ", arg );
                     return;
                  }
               }
               break;
            default:
               value = atoi( arg );
               break;
         }
         /*
          * Link it in 
          */
         if( !value || OLC_VAL( d ) == TRUE )
         {
            paf->modifier = value;
            olc_log( d, "Modified affect to: %s by %d", a_types[paf->location], value );
            OLC_VAL( d ) = FALSE;
            oedit_disp_prompt_apply_menu( d );
            return;
         }
         CREATE( npaf, AFFECT_DATA, 1 );
         npaf->type = -1;
         npaf->duration = -1;
         npaf->location = URANGE( 0, paf->location, MAX_APPLY_TYPE );
         npaf->modifier = value;
         npaf->bit = 0;
         npaf->next = NULL;

         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            LINK( npaf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next, prev );
         else
            LINK( npaf, obj->first_affect, obj->last_affect, next, prev );
         ++top_affect;
         olc_log( d, "Added new affect: %s by %d", a_types[npaf->location], npaf->modifier );

         DISPOSE( paf );
         d->character->pcdata->spare_ptr = NULL;
         oedit_disp_prompt_apply_menu( d );
         return;

      case OEDIT_AFFECT_RIS:
         /*
          * Unnecessary atm 
          */
         number = atoi( arg );
         if( number < 0 || number > 31 )
         {
            send_to_char( "Unknown flag, try again: ", d->character );
            return;
         }
         return;

      case OEDIT_AFFECT_REMOVE:
         number = atoi( arg );
         remove_affect_from_obj( obj, number );
         olc_log( d, "Removed affect #%d", number );
         oedit_disp_prompt_apply_menu( d );
         return;

      case OEDIT_EXTRADESC_KEY:
         /*
          * if ( SetOExtra( obj, arg ) || SetOExtraProto( obj->pIndexData, arg ) )
          * {
          * send_to_char( "A extradesc with that keyword already exists.\n\r", d->character );
          * oedit_disp_extradesc_menu(d);
          * return;
          * } 
          */
         olc_log( d, "Changed exdesc %s to %s", ed->keyword, arg );
         STRFREE( ed->keyword );
         ed->keyword = STRALLOC( arg );
         oedit_disp_extra_choice( d );
         return;

      case OEDIT_EXTRADESC_DESCRIPTION:
         /*
          * Should never reach this 
          */
         break;

      case OEDIT_EXTRADESC_CHOICE:
         number = atoi( arg );
         switch ( number )
         {
            case 0:
               OLC_MODE( d ) = OEDIT_EXTRADESC_MENU;
               oedit_disp_extradesc_menu( d );
               return;
            case 1:
               OLC_MODE( d ) = OEDIT_EXTRADESC_KEY;
               send_to_char( "Enter keywords, speperated by spaces: ", d->character );
               return;
            case 2:
               OLC_MODE( d ) = OEDIT_EXTRADESC_DESCRIPTION;
               d->character->substate = SUB_OBJ_EXTRA;
               d->character->last_cmd = do_oedit_reset;

               send_to_char( "Enter new extra description - :\n\r", d->character );
               if( !ed->extradesc )
                  ed->extradesc = STRALLOC( "" );
               start_editing( d->character, ed->extradesc );
               return;
         }
         break;

      case OEDIT_EXTRADESC_DELETE:
         ed = oedit_find_extradesc( obj, atoi( arg ) );
         if( !ed )
         {
            send_to_char( "Extra description not found, try again: ", d->character );
            return;
         }
         olc_log( d, "Deleted exdesc %s", ed->keyword );
         if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
            UNLINK( ed, obj->pIndexData->first_extradesc, obj->pIndexData->last_extradesc, next, prev );
         else
            UNLINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
         STRFREE( ed->keyword );
         STRFREE( ed->extradesc );
         DISPOSE( ed );
         top_ed--;
         oedit_disp_extradesc_menu( d );
         return;

      case OEDIT_EXTRADESC_MENU:
         switch ( UPPER( arg[0] ) )
         {
            case 'Q':
               break;

            case 'A':
               CREATE( ed, EXTRA_DESCR_DATA, 1 );
               if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
                  LINK( ed, obj->pIndexData->first_extradesc, obj->pIndexData->last_extradesc, next, prev );
               else
                  LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
               top_ed++;
               d->character->pcdata->spare_ptr = ed;
               olc_log( d, "Added new exdesc" );
               oedit_disp_extra_choice( d );
               return;

            case 'R':
               OLC_MODE( d ) = OEDIT_EXTRADESC_DELETE;
               send_to_char( "Delete which extra description? ", d->character );
               return;

            default:
               if( is_number( arg ) )
               {
                  ed = oedit_find_extradesc( obj, atoi( arg ) );
                  if( !ed )
                  {
                     send_to_char( "Not found, try again: ", d->character );
                     return;
                  }
                  d->character->pcdata->spare_ptr = ed;
                  oedit_disp_extra_choice( d );
               }
               else
                  oedit_disp_extradesc_menu( d );
               return;
         }
         break;

      default:
         bug( "%s", "Oedit_parse: Reached default case!" );
         break;
   }

   /*
    * . If we get here, we have changed something .
    */
   OLC_CHANGE( d ) = TRUE; /*. Has changed flag . */
   oedit_disp_menu( d );
}
