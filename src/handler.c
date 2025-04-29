/**************************************************************************r
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *	ROM 2.4 is copyright 1993-1998 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@hypercube.org)				   *
 *	    Gabrielle Taylor (gtaylor@hypercube.org)			   *
 *	    Brian Moore (zump@rom.org)					   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "interp.h"
#include "magic.h"
#include "merc.h"
#include "recycle.h"
#include "tables.h"

/*
 * Local functions.
 */
void affect_modify args((CHAR_DATA * ch, AFFECT_DATA *paf, bool fAdd));

/* friend stuff -- for NPC's mostly */
bool is_friend(CHAR_DATA *ch, CHAR_DATA *victim) {
  if (is_same_group(ch, victim)) return TRUE;

  if (!IS_NPC(ch)) return FALSE;

  if (!IS_NPC(victim)) {
    if (IS_SET(ch->off_flags, ASSIST_PLAYERS))
      return TRUE;
    else
      return FALSE;
  }

  if (IS_AFFECTED(ch, AFF_CHARM)) return FALSE;

  if (IS_SET(ch->off_flags, ASSIST_ALL)) return TRUE;

  if (ch->group && ch->group == victim->group) return TRUE;

  if (IS_SET(ch->off_flags, ASSIST_VNUM) &&
      ch->pIndexData == victim->pIndexData)
    return TRUE;

  if (IS_SET(ch->off_flags, ASSIST_RACE) && ch->race == victim->race)
    return TRUE;

  if (IS_SET(ch->off_flags, ASSIST_ALIGN) && !IS_SET(ch->act, ACT_NOALIGN) &&
      !IS_SET(victim->act, ACT_NOALIGN) &&
      ((IS_GOOD(ch) && IS_GOOD(victim)) || (IS_EVIL(ch) && IS_EVIL(victim)) ||
       (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))))
    return TRUE;

  return FALSE;
}

/* returns number of people on an object */
int count_users(OBJ_DATA *obj) {
  CHAR_DATA *fch;
  int count = 0;

  if (obj->in_room == NULL) return 0;

  for (fch = obj->in_room->people; fch != NULL; fch = fch->next_in_room)
    if (fch->on == obj) count++;

  return count;
}

/* returns material number */
int material_lookup(const char *name) {
  int mat;

  for (mat = 0;
       material_type[mat].name != NULL && material_type[mat].name[0] != '\0';
       mat++) {
    if (name[0] == material_type[mat].name[0] &&
        !str_prefix(name, material_type[mat].name))
      return mat;
  }

  return -1;
}

/* returns material name -- ROM OLC temp patch */
char *material_name(sh_int num) { return material_type[num].name; }

/* returns race number */
int race_lookup(const char *name) {
  int race;

  for (race = 0; race_table[race].name != NULL; race++) {
    if (LOWER(name[0]) == LOWER(race_table[race].name[0]) &&
        !str_prefix(name, race_table[race].name))
      return race;
  }

  return 0;
}

int liq_lookup(const char *name) {
  int liq;

  for (liq = 0; liq_table[liq].liq_name != NULL; liq++) {
    if (LOWER(name[0]) == LOWER(liq_table[liq].liq_name[0]) &&
        !str_prefix(name, liq_table[liq].liq_name))
      return liq;
  }

  return -1;
}

int weapon_lookup(const char *name) {
  int type;

  for (type = 0; weapon_table[type].name != NULL; type++) {
    if (LOWER(name[0]) == LOWER(weapon_table[type].name[0]) &&
        !str_prefix(name, weapon_table[type].name))
      return type;
  }

  return -1;
}

int weapon_type(const char *name) {
  int type;

  for (type = 0; weapon_table[type].name != NULL; type++) {
    if (LOWER(name[0]) == LOWER(weapon_table[type].name[0]) &&
        !str_prefix(name, weapon_table[type].name))
      return weapon_table[type].type;
  }

  return WEAPON_EXOTIC;
}

int item_lookup(const char *name) {
  int type;

  for (type = 0; item_table[type].name != NULL; type++) {
    if (LOWER(name[0]) == LOWER(item_table[type].name[0]) &&
        !str_prefix(name, item_table[type].name))
      return item_table[type].type;
  }

  return -1;
}

char *item_name(int item_type) {
  int type;

  for (type = 0; item_table[type].name != NULL; type++)
    if (item_type == item_table[type].type) return item_table[type].name;
  return "none";
}

char *weapon_name(int weapon_type) {
  int type;

  for (type = 0; weapon_table[type].name != NULL; type++)
    if (weapon_type == weapon_table[type].type) return weapon_table[type].name;
  return "exotic";
}

int attack_lookup(const char *name) {
  int att;

  for (att = 0; attack_table[att].name != NULL; att++) {
    if (LOWER(name[0]) == LOWER(attack_table[att].name[0]) &&
        !str_prefix(name, attack_table[att].name))
      return att;
  }

  return 0;
}

/* returns a flag for wiznet */
long wiznet_lookup(const char *name) {
  int flag;

  for (flag = 0; wiznet_table[flag].name != NULL; flag++) {
    if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0]) &&
        !str_prefix(name, wiznet_table[flag].name))
      return flag;
  }

  return -1;
}

/* returns class number */
int class_lookup(const char *name) {
  int class;

  for (class = 0; class < MAX_CLASS; class++) {
    if (LOWER(name[0]) == LOWER(class_table[class].name[0]) &&
        !str_prefix(name, class_table[class].name))
      return class;
  }

  return -1;
}

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.c */

int check_immune(CHAR_DATA *ch, int dam_type) {
  int immune, def;
  int bit;

  immune = -1;
  def = IS_NORMAL;

  if (dam_type == DAM_NONE) return immune;

  if (dam_type <= 3) {
    if (IS_SET(ch->imm_flags, IMM_WEAPON))
      def = IS_IMMUNE;
    else if (IS_SET(ch->res_flags, RES_WEAPON))
      def = IS_RESISTANT;
    else if (IS_SET(ch->vuln_flags, VULN_WEAPON))
      def = IS_VULNERABLE;
  } else /* magical attack */
  {
    if (IS_SET(ch->imm_flags, IMM_MAGIC))
      def = IS_IMMUNE;
    else if (IS_SET(ch->res_flags, RES_MAGIC))
      def = IS_RESISTANT;
    else if (IS_SET(ch->vuln_flags, VULN_MAGIC))
      def = IS_VULNERABLE;
  }

  /* set bits to check -- VULN etc. must ALL be the same or this will fail */
  switch (dam_type) {
    case (DAM_BASH):
      bit = IMM_BASH;
      break;
    case (DAM_PIERCE):
      bit = IMM_PIERCE;
      break;
    case (DAM_SLASH):
      bit = IMM_SLASH;
      break;
    case (DAM_FIRE):
      bit = IMM_FIRE;
      break;
    case (DAM_COLD):
      bit = IMM_COLD;
      break;
    case (DAM_LIGHTNING):
      bit = IMM_LIGHTNING;
      break;
    case (DAM_ACID):
      bit = IMM_ACID;
      break;
    case (DAM_POISON):
      bit = IMM_POISON;
      break;
    case (DAM_NEGATIVE):
      bit = IMM_NEGATIVE;
      break;
    case (DAM_HOLY):
      bit = IMM_HOLY;
      break;
    case (DAM_ENERGY):
      bit = IMM_ENERGY;
      break;
    case (DAM_MENTAL):
      bit = IMM_MENTAL;
      break;
    case (DAM_DISEASE):
      bit = IMM_DISEASE;
      break;
    case (DAM_DROWNING):
      bit = IMM_DROWNING;
      break;
    case (DAM_LIGHT):
      bit = IMM_LIGHT;
      break;
    case (DAM_CHARM):
      bit = IMM_CHARM;
      break;
    case (DAM_SOUND):
      bit = IMM_SOUND;
      break;
    default:
      return def;
  }

  if (IS_SET(ch->imm_flags, bit))
    immune = IS_IMMUNE;
  else if (IS_SET(ch->res_flags, bit) && immune != IS_IMMUNE)
    immune = IS_RESISTANT;
  else if (IS_SET(ch->vuln_flags, bit)) {
    if (immune == IS_IMMUNE)
      immune = IS_RESISTANT;
    else if (immune == IS_RESISTANT)
      immune = IS_NORMAL;
    else
      immune = IS_VULNERABLE;
  }

  if (immune == -1)
    return def;
  else
    return immune;
}

bool is_clan(CHAR_DATA *ch) { return ch->clan; }

bool is_same_clan(CHAR_DATA *ch, CHAR_DATA *victim) {
  if (clan_table[ch->clan].independent)
    return FALSE;
  else
    return (ch->clan == victim->clan);
}

/* checks mob format */
bool is_old_mob(CHAR_DATA *ch) {
  if (ch->pIndexData == NULL)
    return FALSE;
  else if (ch->pIndexData->new_format)
    return FALSE;
  return TRUE;
}

/* for returning skill information */
int get_skill(CHAR_DATA *ch, int sn) {
  int skill;

  if (sn == -1) /* shorthand for level based skills */
  {
    skill = ch->level * 5 / 2;
  }

  else if (sn < -1 || sn > MAX_SKILL) {
    bug("Bad sn %d in get_skill.", sn);
    skill = 0;
  }

  else if (!IS_NPC(ch)) {
    if (ch->level < skill_table[sn].skill_level[ch->class])
      skill = 0;
    else
      skill = ch->pcdata->learned[sn];
  }

  else /* mobiles */
  {
    if (skill_table[sn].spell_fun != spell_null)
      skill = 40 + 2 * ch->level;

    else if (sn == gsn_sneak || sn == gsn_hide)
      skill = ch->level * 2 + 20;

    else if ((sn == gsn_dodge && IS_SET(ch->off_flags, OFF_DODGE)) ||
             (sn == gsn_parry && IS_SET(ch->off_flags, OFF_PARRY)))
      skill = ch->level * 2;

    else if (sn == gsn_shield_block)
      skill = 10 + 2 * ch->level;

    else if (sn == gsn_second_attack &&
             (IS_SET(ch->act, ACT_WARRIOR) || IS_SET(ch->act, ACT_THIEF)))
      skill = 10 + 3 * ch->level;

    else if (sn == gsn_third_attack && IS_SET(ch->act, ACT_WARRIOR))
      skill = 4 * ch->level - 40;

    else if (sn == gsn_hand_to_hand)
      skill = 40 + 2 * ch->level;

    else if (sn == gsn_trip && IS_SET(ch->off_flags, OFF_TRIP))
      skill = 10 + 3 * ch->level;

    else if (sn == gsn_bash && IS_SET(ch->off_flags, OFF_BASH))
      skill = 10 + 3 * ch->level;

    else if (sn == gsn_disarm &&
             (IS_SET(ch->off_flags, OFF_DISARM) ||
              IS_SET(ch->act, ACT_WARRIOR) || IS_SET(ch->act, ACT_THIEF)))
      skill = 20 + 3 * ch->level;

    else if (sn == gsn_berserk && IS_SET(ch->off_flags, OFF_BERSERK))
      skill = 3 * ch->level;

    else if (sn == gsn_kick)
      skill = 10 + 3 * ch->level;

    else if (sn == gsn_backstab && IS_SET(ch->act, ACT_THIEF))
      skill = 20 + 2 * ch->level;

    else if (sn == gsn_rescue)
      skill = 40 + ch->level;

    else if (sn == gsn_recall)
      skill = 40 + ch->level;

    else if (sn == gsn_sword || sn == gsn_dagger || sn == gsn_spear ||
             sn == gsn_mace || sn == gsn_axe || sn == gsn_flail ||
             sn == gsn_whip || sn == gsn_polearm)
      skill = 40 + 5 * ch->level / 2;

    else
      skill = 0;
  }

  if (ch->daze > 0) {
    if (skill_table[sn].spell_fun != spell_null)
      skill /= 2;
    else
      skill = 2 * skill / 3;
  }

  if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
    skill = 9 * skill / 10;

  return URANGE(0, skill, 100);
}

/* for returning weapon information */
int get_weapon_sn(CHAR_DATA *ch) {
  OBJ_DATA *wield;
  int sn;

  wield = get_eq_char(ch, WEAR_WIELD);
  if (wield == NULL || wield->item_type != ITEM_WEAPON)
    sn = gsn_hand_to_hand;
  else
    switch (wield->value[0]) {
      default:
        sn = -1;
        break;
      case (WEAPON_SWORD):
        sn = gsn_sword;
        break;
      case (WEAPON_DAGGER):
        sn = gsn_dagger;
        break;
      case (WEAPON_SPEAR):
        sn = gsn_spear;
        break;
      case (WEAPON_MACE):
        sn = gsn_mace;
        break;
      case (WEAPON_AXE):
        sn = gsn_axe;
        break;
      case (WEAPON_FLAIL):
        sn = gsn_flail;
        break;
      case (WEAPON_WHIP):
        sn = gsn_whip;
        break;
      case (WEAPON_POLEARM):
        sn = gsn_polearm;
        break;
    }
  return sn;
}

int get_weapon_skill(CHAR_DATA *ch, int sn) {
  int skill;

  /* -1 is exotic */
  if (IS_NPC(ch)) {
    if (sn == -1)
      skill = 3 * ch->level;
    else if (sn == gsn_hand_to_hand)
      skill = 40 + 2 * ch->level;
    else
      skill = 40 + 5 * ch->level / 2;
  }

  else {
    if (sn == -1)
      skill = 3 * ch->level;
    else
      skill = ch->pcdata->learned[sn];
  }

  return URANGE(0, skill, 100);
}

/* used to de-screw characters */
void reset_char(CHAR_DATA *ch) {
  int loc, mod, stat;
  OBJ_DATA *obj;
  AFFECT_DATA *af;
  int i;

  if (IS_NPC(ch)) return;

  if (ch->pcdata->perm_hit == 0 || ch->pcdata->perm_mana == 0 ||
      ch->pcdata->perm_move == 0 || ch->pcdata->last_level == 0) {
    /* do a FULL reset */
    for (loc = 0; loc < MAX_WEAR; loc++) {
      obj = get_eq_char(ch, loc);
      if (obj == NULL) continue;
      if (!obj->enchanted)
        for (af = obj->pIndexData->affected; af != NULL; af = af->next) {
          mod = af->modifier;
          switch (af->location) {
            case APPLY_SEX:
              ch->sex -= mod;
              if (ch->sex < 0 || ch->sex > 2)
                ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
              break;
            case APPLY_MANA:
              ch->max_mana -= mod;
              break;
            case APPLY_HIT:
              ch->max_hit -= mod;
              break;
            case APPLY_MOVE:
              ch->max_move -= mod;
              break;
          }
        }

      for (af = obj->affected; af != NULL; af = af->next) {
        mod = af->modifier;
        switch (af->location) {
          case APPLY_SEX:
            ch->sex -= mod;
            break;
          case APPLY_MANA:
            ch->max_mana -= mod;
            break;
          case APPLY_HIT:
            ch->max_hit -= mod;
            break;
          case APPLY_MOVE:
            ch->max_move -= mod;
            break;
        }
      }
    }
    /* now reset the permanent stats */
    ch->pcdata->perm_hit = ch->max_hit;
    ch->pcdata->perm_mana = ch->max_mana;
    ch->pcdata->perm_move = ch->max_move;
    ch->pcdata->last_level = ch->played / 3600;
    if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2) {
      if (ch->sex > 0 && ch->sex < 3)
        ch->pcdata->true_sex = ch->sex;
      else
        ch->pcdata->true_sex = 0;
    }
  }

  /* now restore the character to his/her true condition */
  for (stat = 0; stat < MAX_STATS; stat++) ch->mod_stat[stat] = 0;

  if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
    ch->pcdata->true_sex = 0;
  ch->sex = ch->pcdata->true_sex;
  ch->max_hit = ch->pcdata->perm_hit;
  ch->max_mana = ch->pcdata->perm_mana;
  ch->max_move = ch->pcdata->perm_move;

  for (i = 0; i < 4; i++) ch->armor[i] = 100;

  ch->hitroll = 0;
  ch->damroll = 0;
  ch->saving_throw = 0;

  /* now start adding back the effects */
  for (loc = 0; loc < MAX_WEAR; loc++) {
    obj = get_eq_char(ch, loc);
    if (obj == NULL) continue;
    for (i = 0; i < 4; i++) ch->armor[i] -= apply_ac(obj, loc, i);

    if (!obj->enchanted)
      for (af = obj->pIndexData->affected; af != NULL; af = af->next) {
        mod = af->modifier;
        switch (af->location) {
          case APPLY_STR:
            ch->mod_stat[STAT_STR] += mod;
            break;
          case APPLY_DEX:
            ch->mod_stat[STAT_DEX] += mod;
            break;
          case APPLY_INT:
            ch->mod_stat[STAT_INT] += mod;
            break;
          case APPLY_WIS:
            ch->mod_stat[STAT_WIS] += mod;
            break;
          case APPLY_CON:
            ch->mod_stat[STAT_CON] += mod;
            break;

          case APPLY_SEX:
            ch->sex += mod;
            break;
          case APPLY_MANA:
            ch->max_mana += mod;
            break;
          case APPLY_HIT:
            ch->max_hit += mod;
            break;
          case APPLY_MOVE:
            ch->max_move += mod;
            break;

          case APPLY_AC:
            for (i = 0; i < 4; i++) ch->armor[i] += mod;
            break;
          case APPLY_HITROLL:
            ch->hitroll += mod;
            break;
          case APPLY_DAMROLL:
            ch->damroll += mod;
            break;

          case APPLY_SAVES:
            ch->saving_throw += mod;
            break;
          case APPLY_SAVING_ROD:
            ch->saving_throw += mod;
            break;
          case APPLY_SAVING_PETRI:
            ch->saving_throw += mod;
            break;
          case APPLY_SAVING_BREATH:
            ch->saving_throw += mod;
            break;
          case APPLY_SAVING_SPELL:
            ch->saving_throw += mod;
            break;
        }
      }

    for (af = obj->affected; af != NULL; af = af->next) {
      mod = af->modifier;
      switch (af->location) {
        case APPLY_STR:
          ch->mod_stat[STAT_STR] += mod;
          break;
        case APPLY_DEX:
          ch->mod_stat[STAT_DEX] += mod;
          break;
        case APPLY_INT:
          ch->mod_stat[STAT_INT] += mod;
          break;
        case APPLY_WIS:
          ch->mod_stat[STAT_WIS] += mod;
          break;
        case APPLY_CON:
          ch->mod_stat[STAT_CON] += mod;
          break;

        case APPLY_SEX:
          ch->sex += mod;
          break;
        case APPLY_MANA:
          ch->max_mana += mod;
          break;
        case APPLY_HIT:
          ch->max_hit += mod;
          break;
        case APPLY_MOVE:
          ch->max_move += mod;
          break;

        case APPLY_AC:
          for (i = 0; i < 4; i++) ch->armor[i] += mod;
          break;
        case APPLY_HITROLL:
          ch->hitroll += mod;
          break;
        case APPLY_DAMROLL:
          ch->damroll += mod;
          break;

        case APPLY_SAVES:
          ch->saving_throw += mod;
          break;
        case APPLY_SAVING_ROD:
          ch->saving_throw += mod;
          break;
        case APPLY_SAVING_PETRI:
          ch->saving_throw += mod;
          break;
        case APPLY_SAVING_BREATH:
          ch->saving_throw += mod;
          break;
        case APPLY_SAVING_SPELL:
          ch->saving_throw += mod;
          break;
      }
    }
  }

  /* now add back spell effects */
  for (af = ch->affected; af != NULL; af = af->next) {
    mod = af->modifier;
    switch (af->location) {
      case APPLY_STR:
        ch->mod_stat[STAT_STR] += mod;
        break;
      case APPLY_DEX:
        ch->mod_stat[STAT_DEX] += mod;
        break;
      case APPLY_INT:
        ch->mod_stat[STAT_INT] += mod;
        break;
      case APPLY_WIS:
        ch->mod_stat[STAT_WIS] += mod;
        break;
      case APPLY_CON:
        ch->mod_stat[STAT_CON] += mod;
        break;

      case APPLY_SEX:
        ch->sex += mod;
        break;
      case APPLY_MANA:
        ch->max_mana += mod;
        break;
      case APPLY_HIT:
        ch->max_hit += mod;
        break;
      case APPLY_MOVE:
        ch->max_move += mod;
        break;

      case APPLY_AC:
        for (i = 0; i < 4; i++) ch->armor[i] += mod;
        break;
      case APPLY_HITROLL:
        ch->hitroll += mod;
        break;
      case APPLY_DAMROLL:
        ch->damroll += mod;
        break;

      case APPLY_SAVES:
        ch->saving_throw += mod;
        break;
      case APPLY_SAVING_ROD:
        ch->saving_throw += mod;
        break;
      case APPLY_SAVING_PETRI:
        ch->saving_throw += mod;
        break;
      case APPLY_SAVING_BREATH:
        ch->saving_throw += mod;
        break;
      case APPLY_SAVING_SPELL:
        ch->saving_throw += mod;
        break;
    }
  }

  /* make sure sex is RIGHT!!!! */
  if (ch->sex < 0 || ch->sex > 2) ch->sex = ch->pcdata->true_sex;
}

/*
 * Retrieve a character's trusted level for permission checking.
 */
int get_trust(CHAR_DATA *ch) {
  if (ch->desc != NULL && ch->desc->original != NULL) ch = ch->desc->original;

  if (ch->trust) return ch->trust;

  if (IS_NPC(ch) && ch->level >= LEVEL_HERO)
    return LEVEL_HERO - 1;
  else
    return ch->level;
}

/*
 * Retrieve a character's age.
 */
int get_age(CHAR_DATA *ch) {
  return 17 + (ch->played + (int)(current_time - ch->logon)) / 72000;
}

/* command for retrieving stats */
int get_curr_stat(CHAR_DATA *ch, int stat) {
  int max;

  if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
    max = 25;

  else {
    max = pc_race_table[ch->race].max_stats[stat] + 4;

    if (class_table[ch->class].attr_prime == stat) max += 2;

    if (ch->race == race_lookup("human")) max += 1;

    max = UMIN(max, 25);
  }

  return URANGE(3, ch->perm_stat[stat] + ch->mod_stat[stat], max);
}

/* command for returning max training score */
int get_max_train(CHAR_DATA *ch, int stat) {
  int max;

  if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL) return 25;

  max = pc_race_table[ch->race].max_stats[stat];
  if (class_table[ch->class].attr_prime == stat) {
    if (ch->race == race_lookup("human"))
      max += 3;
    else
      max += 2;
  }
  return UMIN(max, 25);
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n(CHAR_DATA *ch) {
  if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL) return 1000;

  if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET)) return 0;

  return MAX_WEAR + 2 * get_curr_stat(ch, STAT_DEX) + ch->level;
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w(CHAR_DATA *ch) {
  if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL) return 10000000;

  if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET)) return 0;

  return str_app[get_curr_stat(ch, STAT_STR)].carry * 10 + ch->level * 25;
}

/*
 * See if a string is one of the names of an object.
 */

bool is_name(char *str, char *namelist) {
  char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
  char *list, *string;

  /* fix crash on NULL namelist */
  if (namelist == NULL || namelist[0] == '\0') return FALSE;

  /* fixed to prevent is_name on "" returning TRUE */
  if (str[0] == '\0') return FALSE;

  string = str;
  /* we need ALL parts of string to match part of namelist */
  for (;;) /* start parsing string */
  {
    str = one_argument(str, part);

    if (part[0] == '\0') return TRUE;

    /* check to see if this is part of namelist */
    list = namelist;
    for (;;) /* start parsing namelist */
    {
      list = one_argument(list, name);
      if (name[0] == '\0') /* this name was not found */
        return FALSE;

      if (!str_prefix(string, name)) return TRUE; /* full pattern match */

      if (!str_prefix(part, name)) break;
    }
  }
}

bool is_exact_name(char *str, char *namelist) {
  char name[MAX_INPUT_LENGTH];

  if (namelist == NULL) return FALSE;

  for (;;) {
    namelist = one_argument(namelist, name);
    if (name[0] == '\0') return FALSE;
    if (!str_cmp(str, name)) return TRUE;
  }
}

/* enchanted stuff for eq */
void affect_enchant(OBJ_DATA *obj) {
  /* okay, move all the old flags into new vectors if we have to */
  if (!obj->enchanted) {
    AFFECT_DATA *paf, *af_new;
    obj->enchanted = TRUE;

    for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) {
      af_new = new_affect();

      af_new->next = obj->affected;
      obj->affected = af_new;

      af_new->where = paf->where;
      af_new->type = UMAX(0, paf->type);
      af_new->level = paf->level;
      af_new->duration = paf->duration;
      af_new->location = paf->location;
      af_new->modifier = paf->modifier;
      af_new->bitvector = paf->bitvector;
    }
  }
}

/*
 * Apply or remove an affect to a character.
 */
void affect_modify(CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd) {
  OBJ_DATA *wield;
  int mod, i;

  mod = paf->modifier;

  if (fAdd) {
    switch (paf->where) {
      case TO_AFFECTS:
        SET_BIT(ch->affected_by, paf->bitvector);
        break;
      case TO_IMMUNE:
        SET_BIT(ch->imm_flags, paf->bitvector);
        break;
      case TO_RESIST:
        SET_BIT(ch->res_flags, paf->bitvector);
        break;
      case TO_VULN:
        SET_BIT(ch->vuln_flags, paf->bitvector);
        break;
    }
  } else {
    switch (paf->where) {
      case TO_AFFECTS:
        REMOVE_BIT(ch->affected_by, paf->bitvector);
        break;
      case TO_IMMUNE:
        REMOVE_BIT(ch->imm_flags, paf->bitvector);
        break;
      case TO_RESIST:
        REMOVE_BIT(ch->res_flags, paf->bitvector);
        break;
      case TO_VULN:
        REMOVE_BIT(ch->vuln_flags, paf->bitvector);
        break;
    }
    mod = 0 - mod;
  }

  switch (paf->location) {
    default:
      bug("Affect_modify: unknown location %d.", paf->location);
      return;

    case APPLY_NONE:
      break;
    case APPLY_STR:
      ch->mod_stat[STAT_STR] += mod;
      break;
    case APPLY_DEX:
      ch->mod_stat[STAT_DEX] += mod;
      break;
    case APPLY_INT:
      ch->mod_stat[STAT_INT] += mod;
      break;
    case APPLY_WIS:
      ch->mod_stat[STAT_WIS] += mod;
      break;
    case APPLY_CON:
      ch->mod_stat[STAT_CON] += mod;
      break;
    case APPLY_SEX:
      ch->sex += mod;
      break;
    case APPLY_CLASS:
      break;
    case APPLY_LEVEL:
      break;
    case APPLY_AGE:
      break;
    case APPLY_HEIGHT:
      break;
    case APPLY_WEIGHT:
      break;
    case APPLY_MANA:
      ch->max_mana += mod;
      break;
    case APPLY_HIT:
      ch->max_hit += mod;
      break;
    case APPLY_MOVE:
      ch->max_move += mod;
      break;
    case APPLY_GOLD:
      break;
    case APPLY_EXP:
      break;
    case APPLY_AC:
      for (i = 0; i < 4; i++) ch->armor[i] += mod;
      break;
    case APPLY_HITROLL:
      ch->hitroll += mod;
      break;
    case APPLY_DAMROLL:
      ch->damroll += mod;
      break;
    case APPLY_SAVES:
      ch->saving_throw += mod;
      break;
    case APPLY_SAVING_ROD:
      ch->saving_throw += mod;
      break;
    case APPLY_SAVING_PETRI:
      ch->saving_throw += mod;
      break;
    case APPLY_SAVING_BREATH:
      ch->saving_throw += mod;
      break;
    case APPLY_SAVING_SPELL:
      ch->saving_throw += mod;
      break;
    case APPLY_SPELL_AFFECT:
      break;
  }

  /*
   * Check for weapon wielding.
   * Guard against recursion (for weapons with affects).
   */
  if (!IS_NPC(ch) && (wield = get_eq_char(ch, WEAR_WIELD)) != NULL &&
      get_obj_weight(wield) >
          (str_app[get_curr_stat(ch, STAT_STR)].wield * 10)) {
    static int depth;

    if (depth == 0) {
      depth++;
      act("You drop $p.", ch, wield, NULL, TO_CHAR);
      act("$n drops $p.", ch, wield, NULL, TO_ROOM);
      obj_from_char(wield);
      obj_to_room(wield, ch->in_room);
      depth--;
    }
  }

  return;
}

/* find an effect in an affect list */
AFFECT_DATA *affect_find(AFFECT_DATA *paf, int sn) {
  AFFECT_DATA *paf_find;

  for (paf_find = paf; paf_find != NULL; paf_find = paf_find->next) {
    if (paf_find->type == sn) return paf_find;
  }

  return NULL;
}

/* fix object affects when removing one */
void affect_check(CHAR_DATA *ch, int where, int vector) {
  AFFECT_DATA *paf;
  OBJ_DATA *obj;

  if (where == TO_OBJECT || where == TO_WEAPON || vector == 0) return;

  for (paf = ch->affected; paf != NULL; paf = paf->next)
    if (paf->where == where && paf->bitvector == vector) {
      switch (where) {
        case TO_AFFECTS:
          SET_BIT(ch->affected_by, vector);
          break;
        case TO_IMMUNE:
          SET_BIT(ch->imm_flags, vector);
          break;
        case TO_RESIST:
          SET_BIT(ch->res_flags, vector);
          break;
        case TO_VULN:
          SET_BIT(ch->vuln_flags, vector);
          break;
      }
      return;
    }

  for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
    if (obj->wear_loc == -1) continue;

    for (paf = obj->affected; paf != NULL; paf = paf->next)
      if (paf->where == where && paf->bitvector == vector) {
        switch (where) {
          case TO_AFFECTS:
            SET_BIT(ch->affected_by, vector);
            break;
          case TO_IMMUNE:
            SET_BIT(ch->imm_flags, vector);
            break;
          case TO_RESIST:
            SET_BIT(ch->res_flags, vector);
            break;
          case TO_VULN:
            SET_BIT(ch->vuln_flags, vector);
        }
        return;
      }

    if (obj->enchanted) continue;

    for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
      if (paf->where == where && paf->bitvector == vector) {
        switch (where) {
          case TO_AFFECTS:
            SET_BIT(ch->affected_by, vector);
            break;
          case TO_IMMUNE:
            SET_BIT(ch->imm_flags, vector);
            break;
          case TO_RESIST:
            SET_BIT(ch->res_flags, vector);
            break;
          case TO_VULN:
            SET_BIT(ch->vuln_flags, vector);
            break;
        }
        return;
      }
  }
}

/*
 * Give an affect to a char.
 */
void affect_to_char(CHAR_DATA *ch, AFFECT_DATA *paf) {
  AFFECT_DATA *paf_new;

  paf_new = new_affect();

  *paf_new = *paf;

  VALIDATE(paf); /* in case we missed it when we set up paf */
  paf_new->next = ch->affected;
  ch->affected = paf_new;

  affect_modify(ch, paf_new, TRUE);
  return;
}

/* give an affect to an object */
void affect_to_obj(OBJ_DATA *obj, AFFECT_DATA *paf) {
  AFFECT_DATA *paf_new;

  paf_new = new_affect();

  *paf_new = *paf;

  VALIDATE(paf); /* in case we missed it when we set up paf */
  paf_new->next = obj->affected;
  obj->affected = paf_new;

  /* apply any affect vectors to the object's extra_flags */
  if (paf->bitvector) switch (paf->where) {
      case TO_OBJECT:
        SET_BIT(obj->extra_flags, paf->bitvector);
        break;
      case TO_WEAPON:
        if (obj->item_type == ITEM_WEAPON)
          SET_BIT(obj->value[4], paf->bitvector);
        break;
    }

  return;
}

/*
 * Remove an affect from a char.
 */
void affect_remove(CHAR_DATA *ch, AFFECT_DATA *paf) {
  int where;
  int vector;

  if (ch->affected == NULL) {
    bug("Affect_remove: no affect.", 0);
    return;
  }

  affect_modify(ch, paf, FALSE);
  where = paf->where;
  vector = paf->bitvector;

  if (paf == ch->affected) {
    ch->affected = paf->next;
  } else {
    AFFECT_DATA *prev;

    for (prev = ch->affected; prev != NULL; prev = prev->next) {
      if (prev->next == paf) {
        prev->next = paf->next;
        break;
      }
    }

    if (prev == NULL) {
      bug("Affect_remove: cannot find paf.", 0);
      return;
    }
  }

  free_affect(paf);

  affect_check(ch, where, vector);
  return;
}

void affect_remove_obj(OBJ_DATA *obj, AFFECT_DATA *paf) {
  int where, vector;
  if (obj->affected == NULL) {
    bug("Affect_remove_object: no affect.", 0);
    return;
  }

  if (obj->carried_by != NULL && obj->wear_loc != -1)
    affect_modify(obj->carried_by, paf, FALSE);

  where = paf->where;
  vector = paf->bitvector;

  /* remove flags from the object if needed */
  if (paf->bitvector) switch (paf->where) {
      case TO_OBJECT:
        REMOVE_BIT(obj->extra_flags, paf->bitvector);
        break;
      case TO_WEAPON:
        if (obj->item_type == ITEM_WEAPON)
          REMOVE_BIT(obj->value[4], paf->bitvector);
        break;
    }

  if (paf == obj->affected) {
    obj->affected = paf->next;
  } else {
    AFFECT_DATA *prev;

    for (prev = obj->affected; prev != NULL; prev = prev->next) {
      if (prev->next == paf) {
        prev->next = paf->next;
        break;
      }
    }

    if (prev == NULL) {
      bug("Affect_remove_object: cannot find paf.", 0);
      return;
    }
  }

  free_affect(paf);

  if (obj->carried_by != NULL && obj->wear_loc != -1)
    affect_check(obj->carried_by, where, vector);
  return;
}

/*
 * Strip all affects of a given sn.
 */
void affect_strip(CHAR_DATA *ch, int sn) {
  AFFECT_DATA *paf;
  AFFECT_DATA *paf_next;

  for (paf = ch->affected; paf != NULL; paf = paf_next) {
    paf_next = paf->next;
    if (paf->type == sn) affect_remove(ch, paf);
  }

  return;
}

/*
 * Return true if a char is affected by a spell.
 */
bool is_affected(CHAR_DATA *ch, int sn) {
  AFFECT_DATA *paf;

  for (paf = ch->affected; paf != NULL; paf = paf->next) {
    if (paf->type == sn) return TRUE;
  }

  return FALSE;
}

/*
 * Add or enhance an affect.
 */
void affect_join(CHAR_DATA *ch, AFFECT_DATA *paf) {
  AFFECT_DATA *paf_old;

  for (paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next) {
    if (paf_old->type == paf->type) {
      paf->level = (paf->level + paf_old->level) / 2;
      paf->duration += paf_old->duration;
      paf->modifier += paf_old->modifier;
      affect_remove(ch, paf_old);
      break;
    }
  }

  affect_to_char(ch, paf);
  return;
}

/*
 * Move a char out of a room.
 */
void char_from_room(CHAR_DATA *ch) {
  OBJ_DATA *obj;

  if (ch->in_room == NULL) {
    bug("Char_from_room: NULL.", 0);
    return;
  }

  if (!IS_NPC(ch)) --ch->in_room->area->nplayer;

  if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL &&
      obj->item_type == ITEM_LIGHT && obj->value[2] != 0 &&
      ch->in_room->light > 0)
    --ch->in_room->light;

  if (ch == ch->in_room->people) {
    ch->in_room->people = ch->next_in_room;
  } else {
    CHAR_DATA *prev;

    for (prev = ch->in_room->people; prev; prev = prev->next_in_room) {
      if (prev->next_in_room == ch) {
        prev->next_in_room = ch->next_in_room;
        break;
      }
    }

    if (prev == NULL) bug("Char_from_room: ch not found.", 0);
  }

  ch->in_room = NULL;
  ch->next_in_room = NULL;
  ch->on = NULL; /* sanity check! */
  return;
}

/*
 * Move a char into a room.
 */
void char_to_room(CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex) {
  OBJ_DATA *obj;

  if (pRoomIndex == NULL) {
    ROOM_INDEX_DATA *room;

    bug("Char_to_room: NULL.", 0);

    if ((room = get_room_index(ROOM_VNUM_TEMPLE)) != NULL)
      char_to_room(ch, room);

    return;
  }

  ch->in_room = pRoomIndex;
  ch->next_in_room = pRoomIndex->people;
  pRoomIndex->people = ch;

  if (!IS_NPC(ch)) {
    if (ch->in_room->area->empty) {
      ch->in_room->area->empty = FALSE;
      ch->in_room->area->age = 0;
    }
    ++ch->in_room->area->nplayer;
  }

  if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL &&
      obj->item_type == ITEM_LIGHT && obj->value[2] != 0)
    ++ch->in_room->light;

  if (IS_AFFECTED(ch, AFF_PLAGUE)) {
    AFFECT_DATA *af, plague;
    CHAR_DATA *vch;

    for (af = ch->affected; af != NULL; af = af->next) {
      if (af->type == gsn_plague) break;
    }

    if (af == NULL) {
      REMOVE_BIT(ch->affected_by, AFF_PLAGUE);
      return;
    }

    if (af->level == 1) return;

    plague.where = TO_AFFECTS;
    plague.type = gsn_plague;
    plague.level = af->level - 1;
    plague.duration = number_range(1, 2 * plague.level);
    plague.location = APPLY_STR;
    plague.modifier = -5;
    plague.bitvector = AFF_PLAGUE;

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
      if (!saves_spell(plague.level - 2, vch, DAM_DISEASE) &&
          !IS_IMMORTAL(vch) && !IS_AFFECTED(vch, AFF_PLAGUE) &&
          number_bits(6) == 0) {
        send_to_char("You feel hot and feverish.\n\r", vch);
        act("$n shivers and looks very ill.", vch, NULL, NULL, TO_ROOM);
        affect_join(vch, &plague);
      }
    }
  }

  return;
}

/*
 * Give an obj to a char.
 */
void obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch) {
  obj->next_content = ch->carrying;
  ch->carrying = obj;
  obj->carried_by = ch;
  obj->in_room = NULL;
  obj->in_obj = NULL;
  ch->carry_number += get_obj_number(obj);
  ch->carry_weight += get_obj_weight(obj);
}

/*
 * Take an obj from its character.
 */
void obj_from_char(OBJ_DATA *obj) {
  CHAR_DATA *ch;

  if ((ch = obj->carried_by) == NULL) {
    bug("Obj_from_char: null ch.", 0);
    return;
  }

  if (obj->wear_loc != WEAR_NONE) unequip_char(ch, obj);

  if (ch->carrying == obj) {
    ch->carrying = obj->next_content;
  } else {
    OBJ_DATA *prev;

    for (prev = ch->carrying; prev != NULL; prev = prev->next_content) {
      if (prev->next_content == obj) {
        prev->next_content = obj->next_content;
        break;
      }
    }

    if (prev == NULL) bug("Obj_from_char: obj not in list.", 0);
  }

  obj->carried_by = NULL;
  obj->next_content = NULL;
  ch->carry_number -= get_obj_number(obj);
  ch->carry_weight -= get_obj_weight(obj);
  return;
}

/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac(OBJ_DATA *obj, int iWear, int type) {
  if (obj->item_type != ITEM_ARMOR) return 0;

  switch (iWear) {
    case WEAR_BODY:
      return 3 * obj->value[type];
    case WEAR_HEAD:
      return 2 * obj->value[type];
    case WEAR_LEGS:
      return 2 * obj->value[type];
    case WEAR_FEET:
      return obj->value[type];
    case WEAR_HANDS:
      return obj->value[type];
    case WEAR_ARMS:
      return obj->value[type];
    case WEAR_SHIELD:
      return obj->value[type];
    case WEAR_NECK_1:
      return obj->value[type];
    case WEAR_NECK_2:
      return obj->value[type];
    case WEAR_ABOUT:
      return 2 * obj->value[type];
    case WEAR_WAIST:
      return obj->value[type];
    case WEAR_WRIST_L:
      return obj->value[type];
    case WEAR_WRIST_R:
      return obj->value[type];
    case WEAR_HOLD:
      return obj->value[type];
  }

  return 0;
}

/*
 * Find a piece of eq on a character.
 */
OBJ_DATA *get_eq_char(CHAR_DATA *ch, int iWear) {
  OBJ_DATA *obj;

  if (ch == NULL) return NULL;

  for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
    if (obj->wear_loc == iWear) return obj;
  }

  return NULL;
}

/*
 * Equip a char with an obj.
 */
void equip_char(CHAR_DATA *ch, OBJ_DATA *obj, int iWear) {
  AFFECT_DATA *paf;
  int i;

  if (get_eq_char(ch, iWear) != NULL) {
    bug("Equip_char: already equipped (%d).", iWear);
    return;
  }

  if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
    /*
     * Thanks to Morgenes for the bug fix here!
     */
    act("You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR);
    act("$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM);
    obj_from_char(obj);
    obj_to_room(obj, ch->in_room);
    return;
  }

  for (i = 0; i < 4; i++) ch->armor[i] -= apply_ac(obj, iWear, i);
  obj->wear_loc = iWear;

  if (!obj->enchanted)
    for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
      if (paf->location != APPLY_SPELL_AFFECT) affect_modify(ch, paf, TRUE);
  for (paf = obj->affected; paf != NULL; paf = paf->next)
    if (paf->location == APPLY_SPELL_AFFECT)
      affect_to_char(ch, paf);
    else
      affect_modify(ch, paf, TRUE);

  if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room != NULL)
    ++ch->in_room->light;

  return;
}

/*
 * Unequip a char with an obj.
 */
void unequip_char(CHAR_DATA *ch, OBJ_DATA *obj) {
  AFFECT_DATA *paf = NULL;
  AFFECT_DATA *lpaf = NULL;
  AFFECT_DATA *lpaf_next = NULL;
  int i;

  if (obj->wear_loc == WEAR_NONE) {
    bug("Unequip_char: already unequipped.", 0);
    return;
  }

  for (i = 0; i < 4; i++) ch->armor[i] += apply_ac(obj, obj->wear_loc, i);
  obj->wear_loc = -1;

  if (!obj->enchanted)
    for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) {
      if (paf->location == APPLY_SPELL_AFFECT) {
        for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next) {
          lpaf_next = lpaf->next;
          if ((lpaf->type == paf->type) && (lpaf->level == paf->level) &&
              (lpaf->location == APPLY_SPELL_AFFECT)) {
            affect_remove(ch, lpaf);
            lpaf_next = NULL;
          }
        }
      } else {
        affect_modify(ch, paf, FALSE);
        affect_check(ch, paf->where, paf->bitvector);
      }
    }
  for (paf = obj->affected; paf != NULL; paf = paf->next)
    if (paf->location == APPLY_SPELL_AFFECT) {
      bug("Norm-Apply: %d", 0);
      for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next) {
        lpaf_next = lpaf->next;
        if ((lpaf->type == paf->type) && (lpaf->level == paf->level) &&
            (lpaf->location == APPLY_SPELL_AFFECT)) {
          bug("location = %d", lpaf->location);
          bug("type = %d", lpaf->type);
          affect_remove(ch, lpaf);
          lpaf_next = NULL;
        }
      }
    } else {
      affect_modify(ch, paf, FALSE);
      affect_check(ch, paf->where, paf->bitvector);
    }

  if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0 &&
      ch->in_room != NULL && ch->in_room->light > 0)
    --ch->in_room->light;

  return;
}

/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list(OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list) {
  OBJ_DATA *obj;
  int nMatch;

  nMatch = 0;
  for (obj = list; obj != NULL; obj = obj->next_content) {
    if (obj->pIndexData == pObjIndex) nMatch++;
  }

  return nMatch;
}

/*
 * Move an obj out of a room.
 */
void obj_from_room(OBJ_DATA *obj) {
  ROOM_INDEX_DATA *in_room;
  CHAR_DATA *ch;

  if ((in_room = obj->in_room) == NULL) {
    bug("obj_from_room: NULL.", 0);
    return;
  }

  for (ch = in_room->people; ch != NULL; ch = ch->next_in_room)
    if (ch->on == obj) ch->on = NULL;

  if (obj == in_room->contents) {
    in_room->contents = obj->next_content;
  } else {
    OBJ_DATA *prev;

    for (prev = in_room->contents; prev; prev = prev->next_content) {
      if (prev->next_content == obj) {
        prev->next_content = obj->next_content;
        break;
      }
    }

    if (prev == NULL) {
      bug("Obj_from_room: obj not found.", 0);
      return;
    }
  }

  obj->in_room = NULL;
  obj->next_content = NULL;
  return;
}

/*
 * Move an obj into a room.
 */
void obj_to_room(OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex) {
  obj->next_content = pRoomIndex->contents;
  pRoomIndex->contents = obj;
  obj->in_room = pRoomIndex;
  obj->carried_by = NULL;
  obj->in_obj = NULL;
  return;
}

/*
 * Move an object into an object.
 */
void obj_to_obj(OBJ_DATA *obj, OBJ_DATA *obj_to) {
  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;
  obj->in_room = NULL;
  obj->carried_by = NULL;
  if (obj_to->pIndexData->vnum == OBJ_VNUM_PIT) obj->cost = 0;

  for (; obj_to != NULL; obj_to = obj_to->in_obj) {
    if (obj_to->carried_by != NULL) {
      obj_to->carried_by->carry_number += get_obj_number(obj);
      obj_to->carried_by->carry_weight +=
          get_obj_weight(obj) * WEIGHT_MULT(obj_to) / 100;
    }
  }

  return;
}

/*
 * Move an object out of an object.
 */
void obj_from_obj(OBJ_DATA *obj) {
  OBJ_DATA *obj_from;

  if ((obj_from = obj->in_obj) == NULL) {
    bug("Obj_from_obj: null obj_from.", 0);
    return;
  }

  if (obj == obj_from->contains) {
    obj_from->contains = obj->next_content;
  } else {
    OBJ_DATA *prev;

    for (prev = obj_from->contains; prev; prev = prev->next_content) {
      if (prev->next_content == obj) {
        prev->next_content = obj->next_content;
        break;
      }
    }

    if (prev == NULL) {
      bug("Obj_from_obj: obj not found.", 0);
      return;
    }
  }

  obj->next_content = NULL;
  obj->in_obj = NULL;

  for (; obj_from != NULL; obj_from = obj_from->in_obj) {
    if (obj_from->carried_by != NULL) {
      obj_from->carried_by->carry_number -= get_obj_number(obj);
      obj_from->carried_by->carry_weight -=
          get_obj_weight(obj) * WEIGHT_MULT(obj_from) / 100;
    }
  }

  return;
}

/*
 * Extract an obj from the world.
 */
void extract_obj(OBJ_DATA *obj) {
  OBJ_DATA *obj_content;
  OBJ_DATA *obj_next;

  if (obj->in_room != NULL)
    obj_from_room(obj);
  else if (obj->carried_by != NULL)
    obj_from_char(obj);
  else if (obj->in_obj != NULL)
    obj_from_obj(obj);

  for (obj_content = obj->contains; obj_content; obj_content = obj_next) {
    obj_next = obj_content->next_content;
    extract_obj(obj_content);
  }

  if (object_list == obj) {
    object_list = obj->next;
  } else {
    OBJ_DATA *prev;

    for (prev = object_list; prev != NULL; prev = prev->next) {
      if (prev->next == obj) {
        prev->next = obj->next;
        break;
      }
    }

    if (prev == NULL) {
      bug("Extract_obj: obj %d not found.", obj->pIndexData->vnum);
      return;
    }
  }

  --obj->pIndexData->count;
  free_obj(obj);
  return;
}

/*
 * Extract a char from the world.
 */
void extract_char(CHAR_DATA *ch, bool fPull) {
  CHAR_DATA *wch;
  OBJ_DATA *obj;
  OBJ_DATA *obj_next;

  /* doesn't seem to be necessary
  if ( ch->in_room == NULL )
  {
      bug( "Extract_char: NULL.", 0 );
      return;
  }
  */

  nuke_pets(ch);
  ch->pet = NULL; /* just in case */

  if (fPull) die_follower(ch);

  stop_fighting(ch, TRUE);

  for (obj = ch->carrying; obj != NULL; obj = obj_next) {
    obj_next = obj->next_content;
    extract_obj(obj);
  }

  if (ch->in_room != NULL) char_from_room(ch);

  /* Death room is set in the clan tabe now */
  if (!fPull) {
    char_to_room(ch, get_room_index(clan_table[ch->clan].hall));
    return;
  }

  if (IS_NPC(ch)) --ch->pIndexData->count;

  if (ch->desc != NULL && ch->desc->original != NULL) {
    do_function(ch, &do_return, "");
    ch->desc = NULL;
  }

  for (wch = char_list; wch != NULL; wch = wch->next) {
    if (wch->reply == ch) wch->reply = NULL;
  }

  if (ch == char_list) {
    char_list = ch->next;
  } else {
    CHAR_DATA *prev;

    for (prev = char_list; prev != NULL; prev = prev->next) {
      if (prev->next == ch) {
        prev->next = ch->next;
        break;
      }
    }

    if (prev == NULL) {
      bug("Extract_char: char not found.", 0);
      return;
    }
  }

  if (ch->desc != NULL) ch->desc->character = NULL;
  free_char(ch);
  return;
}

/*
 * Find a char in the room.
 */
CHAR_DATA *get_char_room(CHAR_DATA *ch, char *argument) {
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *rch;
  int number;
  int count;

  number = number_argument(argument, arg, sizeof(arg));
  count = 0;
  if (!str_cmp(arg, "self")) return ch;
  for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
    if (!can_see(ch, rch) || !is_name(arg, rch->name)) continue;
    if (++count == number) return rch;
  }

  return NULL;
}

/*
 * Find a char in the world.
 */
CHAR_DATA *get_char_world(CHAR_DATA *ch, char *argument) {
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *wch;
  int number;
  int count;

  if ((wch = get_char_room(ch, argument)) != NULL) return wch;

  number = number_argument(argument, arg, sizeof(arg));
  count = 0;
  for (wch = char_list; wch != NULL; wch = wch->next) {
    if (wch->in_room == NULL || !can_see(ch, wch) || !is_name(arg, wch->name))
      continue;
    if (++count == number) return wch;
  }

  return NULL;
}

/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
OBJ_DATA *get_obj_type(OBJ_INDEX_DATA *pObjIndex) {
  OBJ_DATA *obj;

  for (obj = object_list; obj != NULL; obj = obj->next) {
    if (obj->pIndexData == pObjIndex) return obj;
  }

  return NULL;
}

/*
 * Find an obj in a list.
 */
OBJ_DATA *get_obj_list(CHAR_DATA *ch, char *argument, OBJ_DATA *list) {
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  int number;
  int count;

  number = number_argument(argument, arg, sizeof(arg));
  count = 0;
  for (obj = list; obj != NULL; obj = obj->next_content) {
    if (can_see_obj(ch, obj) && is_name(arg, obj->name)) {
      if (++count == number) return obj;
    }
  }

  return NULL;
}

/*
 * Find an obj in player's inventory.
 */
OBJ_DATA *get_obj_carry(CHAR_DATA *ch, char *argument, CHAR_DATA *viewer) {
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  int number;
  int count;

  number = number_argument(argument, arg, sizeof(arg));
  count = 0;
  for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
    if (obj->wear_loc == WEAR_NONE && (can_see_obj(viewer, obj)) &&
        is_name(arg, obj->name)) {
      if (++count == number) return obj;
    }
  }

  return NULL;
}

/*
 * Find an obj in player's equipment.
 */
OBJ_DATA *get_obj_wear(CHAR_DATA *ch, char *argument) {
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  int number;
  int count;

  number = number_argument(argument, arg, sizeof(arg));
  count = 0;
  for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
    if (obj->wear_loc != WEAR_NONE && can_see_obj(ch, obj) &&
        is_name(arg, obj->name)) {
      if (++count == number) return obj;
    }
  }

  return NULL;
}

/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA *get_obj_here(CHAR_DATA *ch, char *argument) {
  OBJ_DATA *obj;

  obj = get_obj_list(ch, argument, ch->in_room->contents);
  if (obj != NULL) return obj;

  if ((obj = get_obj_carry(ch, argument, ch)) != NULL) return obj;

  if ((obj = get_obj_wear(ch, argument)) != NULL) return obj;

  return NULL;
}

/*
 * Find an obj in the world.
 */
OBJ_DATA *get_obj_world(CHAR_DATA *ch, char *argument) {
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  int number;
  int count;

  if ((obj = get_obj_here(ch, argument)) != NULL) return obj;

  number = number_argument(argument, arg, sizeof(arg));
  count = 0;
  for (obj = object_list; obj != NULL; obj = obj->next) {
    if (can_see_obj(ch, obj) && is_name(arg, obj->name)) {
      if (++count == number) return obj;
    }
  }

  return NULL;
}

/* deduct cost from a character */

void deduct_cost(CHAR_DATA *ch, int cost) {
  int silver = 0, gold = 0;

  silver = UMIN(ch->silver, cost);

  if (silver < cost) {
    gold = ((cost - silver + 99) / 100);
    silver = cost - 100 * gold;
  }

  ch->gold -= gold;
  ch->silver -= silver;

  if (ch->gold < 0) {
    bug("deduct costs: gold %d < 0", ch->gold);
    ch->gold = 0;
  }
  if (ch->silver < 0) {
    bug("deduct costs: silver %d < 0", ch->silver);
    ch->silver = 0;
  }
}
/*
 * Create a 'money' obj.
 */
OBJ_DATA *create_money(int gold, int silver) {
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA *obj;

  if (gold < 0 || silver < 0 || (gold == 0 && silver == 0)) {
    bug("Create_money: zero or negative money.", UMIN(gold, silver));
    gold = UMAX(1, gold);
    silver = UMAX(1, silver);
  }

  if (gold == 0 && silver == 1) {
    obj = create_object(get_obj_index(OBJ_VNUM_SILVER_ONE), 0);
  } else if (gold == 1 && silver == 0) {
    obj = create_object(get_obj_index(OBJ_VNUM_GOLD_ONE), 0);
  } else if (silver == 0) {
    obj = create_object(get_obj_index(OBJ_VNUM_GOLD_SOME), 0);
    snprintf(buf, sizeof(buf), obj->short_descr, gold);
    free_string(obj->short_descr);
    obj->short_descr = str_dup(buf);
    obj->value[1] = gold;
    obj->cost = gold;
    obj->weight = gold / 5;
  } else if (gold == 0) {
    obj = create_object(get_obj_index(OBJ_VNUM_SILVER_SOME), 0);
    snprintf(buf, sizeof(buf), obj->short_descr, silver);
    free_string(obj->short_descr);
    obj->short_descr = str_dup(buf);
    obj->value[0] = silver;
    obj->cost = silver;
    obj->weight = silver / 20;
  }

  else {
    obj = create_object(get_obj_index(OBJ_VNUM_COINS), 0);
    snprintf(buf, sizeof(buf), obj->short_descr, silver, gold);
    free_string(obj->short_descr);
    obj->short_descr = str_dup(buf);
    obj->value[0] = silver;
    obj->value[1] = gold;
    obj->cost = 100 * gold + silver;
    obj->weight = gold / 5 + silver / 20;
  }

  return obj;
}

/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number(OBJ_DATA *obj) {
  int number;

  if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MONEY ||
      obj->item_type == ITEM_GEM || obj->item_type == ITEM_JEWELRY)
    number = 0;
  else
    number = 1;

  for (obj = obj->contains; obj != NULL; obj = obj->next_content)
    number += get_obj_number(obj);

  return number;
}

/*
 * Return weight of an object, including weight of contents.
 */
int get_obj_weight(OBJ_DATA *obj) {
  int weight;
  OBJ_DATA *tobj;

  weight = obj->weight;
  for (tobj = obj->contains; tobj != NULL; tobj = tobj->next_content)
    weight += get_obj_weight(tobj) * WEIGHT_MULT(obj) / 100;

  return weight;
}

int get_true_weight(OBJ_DATA *obj) {
  int weight;

  weight = obj->weight;
  for (obj = obj->contains; obj != NULL; obj = obj->next_content)
    weight += get_obj_weight(obj);

  return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark(ROOM_INDEX_DATA *pRoomIndex) {
  if (pRoomIndex->light > 0) return FALSE;

  if (IS_SET(pRoomIndex->room_flags, ROOM_DARK)) return TRUE;

  if (pRoomIndex->sector_type == SECT_INSIDE ||
      pRoomIndex->sector_type == SECT_CITY)
    return FALSE;

  if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
    return TRUE;

  return FALSE;
}

bool is_room_owner(CHAR_DATA *ch, ROOM_INDEX_DATA *room) {
  if (room->owner == NULL || room->owner[0] == '\0') return FALSE;

  return is_name(ch->name, room->owner);
}

/*
 * True if room is private.
 */
bool room_is_private(ROOM_INDEX_DATA *pRoomIndex) {
  CHAR_DATA *rch;
  int count;

  if (pRoomIndex->owner != NULL && pRoomIndex->owner[0] != '\0') return TRUE;

  count = 0;
  for (rch = pRoomIndex->people; rch != NULL; rch = rch->next_in_room) count++;

  if (IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE) && count >= 2) return TRUE;

  if (IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY) && count >= 1) return TRUE;

  if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY)) return TRUE;

  return FALSE;
}

/* visibility on a room -- for entering and exits */
bool can_see_room(CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex) {
  if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY) &&
      get_trust(ch) < MAX_LEVEL)
    return FALSE;

  if (IS_SET(pRoomIndex->room_flags, ROOM_GODS_ONLY) && !IS_IMMORTAL(ch))
    return FALSE;

  if (IS_SET(pRoomIndex->room_flags, ROOM_HEROES_ONLY) && !IS_IMMORTAL(ch))
    return FALSE;

  if (IS_SET(pRoomIndex->room_flags, ROOM_NEWBIES_ONLY) && ch->level > 5 &&
      !IS_IMMORTAL(ch))
    return FALSE;

  if (!IS_IMMORTAL(ch) && pRoomIndex->clan && ch->clan != pRoomIndex->clan)
    return FALSE;

  return TRUE;
}

/*
 * True if char can see victim.
 */
bool can_see(CHAR_DATA *ch, CHAR_DATA *victim) {
  /* RT changed so that WIZ_INVIS has levels */
  if (ch == victim) return TRUE;

  if (get_trust(ch) < victim->invis_level) return FALSE;

  if (get_trust(ch) < victim->incog_level && ch->in_room != victim->in_room)
    return FALSE;

  if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)) ||
      (IS_NPC(ch) && IS_IMMORTAL(ch)))
    return TRUE;

  if (IS_AFFECTED(ch, AFF_BLIND)) return FALSE;

  if (room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_INFRARED)) return FALSE;

  if (IS_AFFECTED(victim, AFF_INVISIBLE) && !IS_AFFECTED(ch, AFF_DETECT_INVIS))
    return FALSE;

  /* sneaking */
  if (IS_AFFECTED(victim, AFF_SNEAK) && !IS_AFFECTED(ch, AFF_DETECT_HIDDEN) &&
      victim->fighting == NULL) {
    int chance;
    chance = get_skill(victim, gsn_sneak);
    chance += get_curr_stat(victim, STAT_DEX) * 3 / 2;
    chance -= get_curr_stat(ch, STAT_INT) * 2;
    chance -= ch->level - victim->level * 3 / 2;

    if (number_percent() < chance) return FALSE;
  }

  if (IS_AFFECTED(victim, AFF_HIDE) && !IS_AFFECTED(ch, AFF_DETECT_HIDDEN) &&
      victim->fighting == NULL)
    return FALSE;

  return TRUE;
}

/*
 * True if char can see obj.
 */
bool can_see_obj(CHAR_DATA *ch, OBJ_DATA *obj) {
  if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)) return TRUE;

  if (IS_SET(obj->extra_flags, ITEM_VIS_DEATH)) return FALSE;

  if (IS_AFFECTED(ch, AFF_BLIND) && obj->item_type != ITEM_POTION) return FALSE;

  if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0) return TRUE;

  if (IS_SET(obj->extra_flags, ITEM_INVIS) &&
      !IS_AFFECTED(ch, AFF_DETECT_INVIS))
    return FALSE;

  if (IS_OBJ_STAT(obj, ITEM_GLOW)) return TRUE;

  if (room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_DARK_VISION))
    return FALSE;

  return TRUE;
}

/*
 * True if char can drop obj.
 */
bool can_drop_obj(CHAR_DATA *ch, OBJ_DATA *obj) {
  if (!IS_SET(obj->extra_flags, ITEM_NODROP)) return TRUE;

  if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL) return TRUE;

  return FALSE;
}

/*
 * Return ascii name of an affect location.
 */
char *affect_loc_name(int location) {
  switch (location) {
    case APPLY_NONE:
      return "none";
    case APPLY_STR:
      return "strength";
    case APPLY_DEX:
      return "dexterity";
    case APPLY_INT:
      return "intelligence";
    case APPLY_WIS:
      return "wisdom";
    case APPLY_CON:
      return "constitution";
    case APPLY_SEX:
      return "sex";
    case APPLY_CLASS:
      return "class";
    case APPLY_LEVEL:
      return "level";
    case APPLY_AGE:
      return "age";
    case APPLY_MANA:
      return "mana";
    case APPLY_HIT:
      return "hp";
    case APPLY_MOVE:
      return "moves";
    case APPLY_GOLD:
      return "gold";
    case APPLY_EXP:
      return "experience";
    case APPLY_AC:
      return "armor class";
    case APPLY_HITROLL:
      return "hit roll";
    case APPLY_DAMROLL:
      return "damage roll";
    case APPLY_SAVES:
      return "saves";
    case APPLY_SAVING_ROD:
      return "save vs rod";
    case APPLY_SAVING_PETRI:
      return "save vs petrification";
    case APPLY_SAVING_BREATH:
      return "save vs breath";
    case APPLY_SAVING_SPELL:
      return "save vs spell";
    case APPLY_SPELL_AFFECT:
      return "none";
  }

  bug("Affect_location_name: unknown location %d.", location);
  return "(unknown)";
}

#define AFF_BIT_NAME_NONE 0
#define AFF_BIT_NAME_BLIND 1
#define AFF_BIT_NAME_INVISIBLE 2
#define AFF_BIT_NAME_DETECT_EVIL 3
#define AFF_BIT_NAME_DETECT_GOOD 4
#define AFF_BIT_NAME_DETECT_INVIS 5
#define AFF_BIT_NAME_DETECT_MAGIC 6
#define AFF_BIT_NAME_DETECT_HIDDEN 7
#define AFF_BIT_NAME_SANCTUARY 8
#define AFF_BIT_NAME_FAERIE_FIRE 9
#define AFF_BIT_NAME_INFRARED 10
#define AFF_BIT_NAME_CURSE 11
#define AFF_BIT_NAME_POISON 12
#define AFF_BIT_NAME_PROTECT_EVIL 13
#define AFF_BIT_NAME_PROTECT_GOOD 14
#define AFF_BIT_NAME_SLEEP 15
#define AFF_BIT_NAME_SNEAK 16
#define AFF_BIT_NAME_HIDE 17
#define AFF_BIT_NAME_CHARM 18
#define AFF_BIT_NAME_FLYING 19
#define AFF_BIT_NAME_PASS_DOOR 20
#define AFF_BIT_NAME_BERSERK 21
#define AFF_BIT_NAME_CALM 22
#define AFF_BIT_NAME_HASTE 23
#define AFF_BIT_NAME_SLOW 24
#define AFF_BIT_NAME_PLAGUE 25
#define AFF_BIT_NAME_DARK_VISION 26

const char *AFFECT_BIT_NAMES[] = {
  "none",
  " blind",
  " invisible",
  " detect_evil",
  " detect_good",
  " detect_invis",
  " detect_magic",
  " detect_hidden",
  " sanctuary",
  " faerie_fire",
  " infrared",
  " curse",
  " poison",
  " prot_evil",
  " prot_good",
  " sleep",
  " sneak",
  " hide",
  " charm",
  " flying",
  " pass_door",
  " berserk",
  " calm",
  " haste",
  " slow",
  " plague",
  " dark_vision"
};
/*
 * Return ascii name of an affect bit vector.
 */
char *affect_bit_name(int vector) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';
  if (vector & AFF_BLIND) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_BLIND], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_BLIND]) - 2; }
  if (vector & AFF_INVISIBLE) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_INVISIBLE], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_INVISIBLE]) - 2; }
  if (vector & AFF_DETECT_EVIL) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_DETECT_EVIL], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_DETECT_EVIL]) - 2; }
  if (vector & AFF_DETECT_GOOD) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_DETECT_GOOD], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_DETECT_GOOD]) - 2; }
  if (vector & AFF_DETECT_INVIS) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_DETECT_INVIS], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_DETECT_INVIS]) - 2; }
  if (vector & AFF_DETECT_MAGIC) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_DETECT_MAGIC], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_DETECT_MAGIC]) - 2; }
  if (vector & AFF_DETECT_HIDDEN) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_DETECT_HIDDEN], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_DETECT_HIDDEN]) - 2; }
  if (vector & AFF_SANCTUARY) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_SANCTUARY], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_SANCTUARY]) - 2; }
  if (vector & AFF_FAERIE_FIRE) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_FAERIE_FIRE], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_FAERIE_FIRE]) - 2; }
  if (vector & AFF_INFRARED) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_INFRARED], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_INFRARED]) - 2; }
  if (vector & AFF_CURSE) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_CURSE], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_CURSE]) - 2; }
  if (vector & AFF_POISON) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_POISON], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_POISON]) - 2; }
  if (vector & AFF_PROTECT_EVIL) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_PROTECT_EVIL], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_PROTECT_EVIL]) - 2; }
  if (vector & AFF_PROTECT_GOOD) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_PROTECT_GOOD], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_PROTECT_GOOD]) - 2; }
  if (vector & AFF_SLEEP) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_SLEEP], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_SLEEP]) - 2; }
  if (vector & AFF_SNEAK) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_SNEAK], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_SNEAK]) - 2; }
  if (vector & AFF_HIDE) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_HIDE], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_HIDE]) - 2; }
  if (vector & AFF_CHARM) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_CHARM], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_CHARM]) - 2; }
  if (vector & AFF_FLYING) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_FLYING], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_FLYING]) - 2; }
  if (vector & AFF_PASS_DOOR) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_PASS_DOOR], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_PASS_DOOR]) - 2; }
  if (vector & AFF_BERSERK) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_BERSERK], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_BERSERK]) - 2; }
  if (vector & AFF_CALM) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_CALM], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_CALM]) - 2; }
  if (vector & AFF_HASTE) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_HASTE], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_HASTE]) - 2; }
  if (vector & AFF_SLOW) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_SLOW], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_SLOW]) - 2; }
  if (vector & AFF_PLAGUE) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_PLAGUE], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_PLAGUE]) - 2; }
  if (vector & AFF_DARK_VISION) { strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_DARK_VISION], sizeof(buf) - offset - 1); offset += sizeof(AFFECT_BIT_NAMES[AFF_BIT_NAME_DARK_VISION]) - 2; }
  return (buf[0] != '\0') ? buf + 1 : strncat(buf, AFFECT_BIT_NAMES[AFF_BIT_NAME_NONE], sizeof(buf)-1);
}

#define EXTRA_BIT_NAME_NONE 0
#define EXTRA_BIT_NAME_GLOW 1
#define EXTRA_BIT_NAME_HUM 2
#define EXTRA_BIT_NAME_DARK 3
#define EXTRA_BIT_NAME_LOCK 4
#define EXTRA_BIT_NAME_EVIL 5
#define EXTRA_BIT_NAME_INVIS 6
#define EXTRA_BIT_NAME_MAGIC 7
#define EXTRA_BIT_NAME_NODROP 8
#define EXTRA_BIT_NAME_BLESS 9
#define EXTRA_BIT_NAME_ANTI_GOOD 10
#define EXTRA_BIT_NAME_ANTI_EVIL 11
#define EXTRA_BIT_NAME_ANTI_NEUTRAL 12
#define EXTRA_BIT_NAME_NOREMOVE 13
#define EXTRA_BIT_NAME_INVENTORY 14
#define EXTRA_BIT_NAME_NOPURGE 15
#define EXTRA_BIT_NAME_VIS_DEATH 16
#define EXTRA_BIT_NAME_ROT_DEATH 17
#define EXTRA_BIT_NAME_NOLOCATE 18
#define EXTRA_BIT_NAME_SELL_EXTRACT 19
#define EXTRA_BIT_NAME_BURN_PROOF 20
#define EXTRA_BIT_NAME_NOUNCURSE 21

const char *EXTRA_BIT_NAMES[] = {
  "none",
  " glow",
  " hum",
  " dark",
  " lock",
  " evil",
  " invis",
  " magic",
  " nodrop",
  " bless",
  " anti-good",
  " anti-evil",
  " anti-neutral",
  " noremove",
  " inventory",
  " nopurge",
  " vis_death",
  " rot_death",
  " no_locate",
  " sell_extract",
  " burn_proof",
  " no_uncurse"
};

/*
 * Return ascii name of extra flags vector.
 */
char *extra_bit_name(int extra_flags) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';
  if (extra_flags & ITEM_GLOW) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_GLOW], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_GLOW]) - 2; }
  if (extra_flags & ITEM_HUM) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_HUM], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_HUM]) - 2; }
  if (extra_flags & ITEM_DARK) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_DARK], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_DARK]) - 2; }
  if (extra_flags & ITEM_LOCK) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_LOCK], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_LOCK]) - 2; }
  if (extra_flags & ITEM_EVIL) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_EVIL], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_EVIL]) - 2; }
  if (extra_flags & ITEM_INVIS) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_INVIS], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_INVIS]) - 2; }
  if (extra_flags & ITEM_MAGIC) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_MAGIC], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_MAGIC]) - 2; }
  if (extra_flags & ITEM_NODROP) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_NODROP], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_NODROP]) - 2; }
  if (extra_flags & ITEM_BLESS) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_BLESS], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_BLESS]) - 2; }
  if (extra_flags & ITEM_ANTI_GOOD) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_ANTI_GOOD], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_ANTI_GOOD]) - 2; }
  if (extra_flags & ITEM_ANTI_EVIL) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_ANTI_EVIL], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_ANTI_EVIL]) - 2; }
  if (extra_flags & ITEM_ANTI_NEUTRAL) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_ANTI_NEUTRAL], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_ANTI_NEUTRAL]) - 2; }
  if (extra_flags & ITEM_NOREMOVE) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_NOREMOVE], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_NOREMOVE]) - 2; }
  if (extra_flags & ITEM_INVENTORY) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_INVENTORY], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_INVENTORY]) - 2; }
  if (extra_flags & ITEM_NOPURGE) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_NOPURGE], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_NOPURGE]) - 2; }
  if (extra_flags & ITEM_VIS_DEATH) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_VIS_DEATH], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_VIS_DEATH]) - 2; }
  if (extra_flags & ITEM_ROT_DEATH) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_ROT_DEATH], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_ROT_DEATH]) - 2; }
  if (extra_flags & ITEM_NOLOCATE) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_NOLOCATE], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_NOLOCATE]) - 2; }
  if (extra_flags & ITEM_SELL_EXTRACT) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_SELL_EXTRACT], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_SELL_EXTRACT]) - 2; }
  if (extra_flags & ITEM_BURN_PROOF) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_BURN_PROOF], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_BURN_PROOF]) - 2; }
  if (extra_flags & ITEM_NOUNCURSE) { strncat(buf, EXTRA_BIT_NAMES[EXTRA_BIT_NAME_NOUNCURSE], sizeof(buf) - offset - 1); offset += sizeof(EXTRA_BIT_NAMES[EXTRA_BIT_NAME_NOUNCURSE]) - 2; }
  return (buf[0] != '\0') ? buf + 1 : "none";
}

#define ACT_BIT_NAME_IS_NPC 0
#define ACT_BIT_NAME_SENTINEL 1
#define ACT_BIT_NAME_SCAVENGER 2
#define ACT_BIT_NAME_AGGRESSIVE 3
#define ACT_BIT_NAME_STAY_AREA 4
#define ACT_BIT_NAME_WIMPY 5
#define ACT_BIT_NAME_PET 6
#define ACT_BIT_NAME_TRAIN 7
#define ACT_BIT_NAME_PRACTICE 8
#define ACT_BIT_NAME_UNDEAD 9
#define ACT_BIT_NAME_CLERIC 10
#define ACT_BIT_NAME_MAGE 11
#define ACT_BIT_NAME_THIEF 12
#define ACT_BIT_NAME_WARRIOR 13
#define ACT_BIT_NAME_NOALIGN 14
#define ACT_BIT_NAME_NOPURGE 15
#define ACT_BIT_NAME_IS_HEALER 16
#define ACT_BIT_NAME_IS_CHANGER 17
#define ACT_BIT_NAME_GAIN 18
#define ACT_BIT_NAME_UPDATE_ALWAYS 19

#define PLR_BIT_NAME_PLAYER 0
#define PLR_BIT_NAME_AUTOASSIST 1
#define PLR_BIT_NAME_AUTOEXIT 2
#define PLR_BIT_NAME_AUTOLOOT 3
#define PLR_BIT_NAME_AUTOSAC 4
#define PLR_BIT_NAME_AUTOGOLD 5
#define PLR_BIT_NAME_AUTOSPLIT 6
#define PLR_BIT_NAME_HOLYLIGHT 7
#define PLR_BIT_NAME_CANLOOT 8
#define PLR_BIT_NAME_NOSUMMON 9
#define PLR_BIT_NAME_NOFOLLOW 10
#define PLR_BIT_NAME_FREEZE 11
#define PLR_BIT_NAME_THIEF 12
#define PLR_BIT_NAME_KILLER 13

const char *PLAYER_BIT_NAMES[] = {
" player",
" autoassist",
" autoexit",
" autoloot",
" autosac",
" autogold",
" autosplit",
" holy_light",
" loot_corpse",
" no_summon",
" no_follow",
" frozen",
" thief",
" killer"
};

const char *ACT_BIT_NAMES[] = {
" npc",
" sentinel",
" scavenger",
" aggressive",
" stay_area",
" wimpy",
" pet",
" train",
" practice",
" undead",
 " cleric",
 " mage",
 " thief",
 " warrior",
 " no_align",
 " no_purge",
 " healer",
 " changer",
 " skill_train",
 " update_always"
};

/* return ascii name of an act vector */
char *act_bit_name(int act_flags) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';

  if (IS_SET(act_flags, ACT_IS_NPC)) {
    strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_IS_NPC], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_IS_NPC]) - 2;
    if (act_flags & ACT_SENTINEL) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_SENTINEL], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_SENTINEL]) - 2; }
    if (act_flags & ACT_SCAVENGER) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_SCAVENGER], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_SCAVENGER]) - 2; }
    if (act_flags & ACT_AGGRESSIVE) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_AGGRESSIVE], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_AGGRESSIVE]) - 2; }
    if (act_flags & ACT_STAY_AREA) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_STAY_AREA], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_STAY_AREA]) - 2; }
    if (act_flags & ACT_WIMPY) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_WIMPY], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_WIMPY]) - 2; }
    if (act_flags & ACT_PET) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_PET], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_PET]) - 2; }
    if (act_flags & ACT_TRAIN) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_TRAIN], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_TRAIN]) - 2; }
    if (act_flags & ACT_PRACTICE) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_PRACTICE], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_PRACTICE]) - 2; }
    if (act_flags & ACT_UNDEAD) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_UNDEAD], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_UNDEAD]) - 2; }
    if (act_flags & ACT_CLERIC) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_CLERIC], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_CLERIC]) - 2; }
    if (act_flags & ACT_MAGE) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_MAGE], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_MAGE]) - 2; }
    if (act_flags & ACT_THIEF) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_THIEF], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_THIEF]) - 2; }
    if (act_flags & ACT_WARRIOR) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_WARRIOR], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_WARRIOR]) - 2; }
    if (act_flags & ACT_NOALIGN) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_NOALIGN], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_NOALIGN]) - 2; }
    if (act_flags & ACT_NOPURGE) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_NOPURGE], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_NOPURGE]) - 2; }
    if (act_flags & ACT_IS_HEALER) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_IS_HEALER], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_IS_HEALER]) - 2; }
    if (act_flags & ACT_IS_CHANGER) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_IS_CHANGER], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_IS_CHANGER]) - 2; }
    if (act_flags & ACT_GAIN) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_GAIN], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_GAIN]) - 2; }
    if (act_flags & ACT_UPDATE_ALWAYS) { strncat(buf, ACT_BIT_NAMES[ACT_BIT_NAME_UPDATE_ALWAYS], sizeof(buf) - offset - 1); offset += sizeof(ACT_BIT_NAMES[ACT_BIT_NAME_UPDATE_ALWAYS]) - 2; }
  } else {
    strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_PLAYER], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_PLAYER]) - 2;
    if (act_flags & PLR_AUTOASSIST) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOASSIST], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOASSIST]) - 2; }
    if (act_flags & PLR_AUTOEXIT) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOEXIT], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOEXIT]) - 2; }
    if (act_flags & PLR_AUTOLOOT) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOLOOT], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOLOOT]) - 2; }
    if (act_flags & PLR_AUTOSAC) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOSAC], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOSAC]) - 2; }
    if (act_flags & PLR_AUTOGOLD) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOGOLD], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOGOLD]) - 2; }
    if (act_flags & PLR_AUTOSPLIT) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOSPLIT], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_AUTOSPLIT]) - 2; }
    if (act_flags & PLR_HOLYLIGHT) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_HOLYLIGHT], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_HOLYLIGHT]) - 2; }
    if (act_flags & PLR_CANLOOT) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_CANLOOT], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_CANLOOT]) - 2; }
    if (act_flags & PLR_NOSUMMON) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_NOSUMMON], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_NOSUMMON]) - 2; }
    if (act_flags & PLR_NOFOLLOW) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_NOFOLLOW], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_NOFOLLOW]) - 2; }
    if (act_flags & PLR_FREEZE) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_FREEZE], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_FREEZE]) - 2; }
    if (act_flags & PLR_THIEF) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_THIEF], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_THIEF]) - 2; }
    if (act_flags & PLR_KILLER) { strncat(buf, PLAYER_BIT_NAMES[PLR_BIT_NAME_KILLER], sizeof(buf) - offset - 1); offset += sizeof(PLAYER_BIT_NAMES[PLR_BIT_NAME_KILLER]) - 2; }
  }
  return buf+1;
}

#define COMM_BIT_NAME_QUIET 0
#define COMM_BIT_NAME_DEAF 1
#define COMM_BIT_NAME_NOWIZ 2
#define COMM_BIT_NAME_NOAUCTION 3
#define COMM_BIT_NAME_NOGOSSIP 4
#define COMM_BIT_NAME_NOQUESTION 5
#define COMM_BIT_NAME_NOMUSIC 6
#define COMM_BIT_NAME_NOQUOTE 7
#define COMM_BIT_NAME_COMPACT 8
#define COMM_BIT_NAME_BRIEF 9
#define COMM_BIT_NAME_PROMPT 10
#define COMM_BIT_NAME_COMBINE 11
#define COMM_BIT_NAME_NOEMOTE 12
#define COMM_BIT_NAME_NOSHOUT 13
#define COMM_BIT_NAME_NOTELL 14
#define COMM_BIT_NAME_NOCHANNELS 15

const char *COMM_BIT_NAMES[] = {
" quiet",
" deaf",
" no_wiz",
" no_auction",
" no_gossip",
" no_question",
" no_music",
" no_quote",
" compact",
" brief",
" prompt",
" combine",
" no_emote",
" no_shout",
" no_tell",
" no_channels"
};

char *comm_bit_name(int comm_flags) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';

  if (comm_flags & COMM_QUIET) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_QUIET], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_QUIET]) - 2; }
  if (comm_flags & COMM_DEAF) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_DEAF], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_DEAF]) - 2; }
  if (comm_flags & COMM_NOWIZ) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_NOWIZ], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_NOWIZ]) - 2; }
  if (comm_flags & COMM_NOAUCTION) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_NOAUCTION], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_NOAUCTION]) - 2; }
  if (comm_flags & COMM_NOGOSSIP) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_NOGOSSIP], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_NOGOSSIP]) - 2; }
  if (comm_flags & COMM_NOQUESTION) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_NOQUESTION], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_NOQUESTION]) - 2; }
  if (comm_flags & COMM_NOMUSIC) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_NOMUSIC], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_NOMUSIC]) - 2; }
  if (comm_flags & COMM_NOQUOTE) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_NOQUOTE], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_NOQUOTE]) - 2; }
  if (comm_flags & COMM_COMPACT) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_COMPACT], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_COMPACT]) - 2; }
  if (comm_flags & COMM_BRIEF) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_BRIEF], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_BRIEF]) - 2; }
  if (comm_flags & COMM_PROMPT) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_PROMPT], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_PROMPT]) - 2; }
  if (comm_flags & COMM_COMBINE) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_COMBINE], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_COMBINE]) - 2; }
  if (comm_flags & COMM_NOEMOTE) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_NOEMOTE], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_NOEMOTE]) - 2; }
  if (comm_flags & COMM_NOSHOUT) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_NOSHOUT], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_NOSHOUT]) - 2; }
  if (comm_flags & COMM_NOTELL) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_NOTELL], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_NOTELL]) - 2; }
  if (comm_flags & COMM_NOCHANNELS) { strncat(buf, COMM_BIT_NAMES[COMM_BIT_NAME_NOCHANNELS], sizeof(buf) - offset - 1); offset += sizeof(COMM_BIT_NAMES[COMM_BIT_NAME_NOCHANNELS]) - 2; }

  return (buf[0] != '\0') ? buf + 1 : "none";
}
#define IMM_BIT_NAME_SUMMON 0
#define IMM_BIT_NAME_CHARM 1
#define IMM_BIT_NAME_MAGIC 2
#define IMM_BIT_NAME_WEAPON 3
#define IMM_BIT_NAME_BASH 4
#define IMM_BIT_NAME_PIERCE 5
#define IMM_BIT_NAME_SLASH 6
#define IMM_BIT_NAME_FIRE 7
#define IMM_BIT_NAME_COLD 8
#define IMM_BIT_NAME_LIGHTNING 9
#define IMM_BIT_NAME_ACID 10
#define IMM_BIT_NAME_POISON 11
#define IMM_BIT_NAME_NEGATIVE 12
#define IMM_BIT_NAME_HOLY 13
#define IMM_BIT_NAME_ENERGY 14
#define IMM_BIT_NAME_MENTAL 15
#define IMM_BIT_NAME_DISEASE 16
#define IMM_BIT_NAME_DROWNING 17
#define IMM_BIT_NAME_LIGHT 18

#define VULN_BIT_NAME_IRON 0
#define VULN_BIT_NAME_WOOD 1
#define VULN_BIT_NAME_SILVER 2

const char *IMM_BIT_NAMES[] = {
  " summon",
  " charm",
  " magic",
  " weapon",
  " blunt",
  " piercing",
  " slashing",
  " fire",
  " cold",
  " lightning",
  " acid",
  " poison",
  " negative",
  " holy",
  " energy",
  " mental",
  " disease",
  " drowning",
  " light"
};

const char *VULN_BIT_NAMES[] = {
  " iron",
  " wood",
  " silver"
};

char *imm_bit_name(int imm_flags) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';

  if (imm_flags & IMM_SUMMON) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_SUMMON], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_SUMMON]) - 2; }
  if (imm_flags & IMM_CHARM) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_CHARM], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_CHARM]) - 2; }
  if (imm_flags & IMM_MAGIC) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_MAGIC], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_MAGIC]) - 2; }
  if (imm_flags & IMM_WEAPON) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_WEAPON], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_WEAPON]) - 2; }
  if (imm_flags & IMM_BASH) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_BASH], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_BASH]) - 2; }
  if (imm_flags & IMM_PIERCE) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_PIERCE], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_PIERCE]) - 2; }
  if (imm_flags & IMM_SLASH) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_SLASH], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_SLASH]) - 2; }
  if (imm_flags & IMM_FIRE) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_FIRE], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_FIRE]) - 2; }
  if (imm_flags & IMM_COLD) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_COLD], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_COLD]) - 2; }
  if (imm_flags & IMM_LIGHTNING) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_LIGHTNING], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_LIGHTNING]) - 2; }
  if (imm_flags & IMM_ACID) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_ACID], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_ACID]) - 2; }
  if (imm_flags & IMM_POISON) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_POISON], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_POISON]) - 2; }
  if (imm_flags & IMM_NEGATIVE) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_NEGATIVE], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_NEGATIVE]) - 2; }
  if (imm_flags & IMM_HOLY) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_HOLY], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_HOLY]) - 2; }
  if (imm_flags & IMM_ENERGY) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_ENERGY], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_ENERGY]) - 2; }
  if (imm_flags & IMM_MENTAL) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_MENTAL], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_MENTAL]) - 2; }
  if (imm_flags & IMM_DISEASE) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_DISEASE], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_DISEASE]) - 2; }
  if (imm_flags & IMM_DROWNING) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_DROWNING], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_DROWNING]) - 2; }
  if (imm_flags & IMM_LIGHT) { strncat(buf, IMM_BIT_NAMES[IMM_BIT_NAME_LIGHT], sizeof(buf) - offset - 1); offset += sizeof(IMM_BIT_NAMES[IMM_BIT_NAME_LIGHT]) - 2; }
  if (imm_flags & VULN_IRON) { strncat(buf, VULN_BIT_NAMES[VULN_BIT_NAME_IRON], sizeof(buf) - offset - 1); offset += sizeof(VULN_BIT_NAMES[VULN_BIT_NAME_IRON]) - 2; }
  if (imm_flags & VULN_WOOD) { strncat(buf, VULN_BIT_NAMES[VULN_BIT_NAME_WOOD], sizeof(buf) - offset - 1); offset += sizeof(VULN_BIT_NAMES[VULN_BIT_NAME_WOOD]) - 2; }
  if (imm_flags & VULN_SILVER) { strncat(buf, VULN_BIT_NAMES[VULN_BIT_NAME_SILVER], sizeof(buf) - offset - 1); offset += sizeof(VULN_BIT_NAMES[VULN_BIT_NAME_SILVER]) - 2; }

  return (buf[0] != '\0') ? buf + 1 : "none";
}

#define WEAR_BIT_NAME_TAKE 0
#define WEAR_BIT_NAME_WEAR_FINGER 1
#define WEAR_BIT_NAME_WEAR_NECK 2
#define WEAR_BIT_NAME_WEAR_BODY 3
#define WEAR_BIT_NAME_WEAR_HEAD 4
#define WEAR_BIT_NAME_WEAR_LEGS 5
#define WEAR_BIT_NAME_WEAR_FEET 6
#define WEAR_BIT_NAME_WEAR_HANDS 7
#define WEAR_BIT_NAME_WEAR_ARMS 8
#define WEAR_BIT_NAME_WEAR_SHIELD 9
#define WEAR_BIT_NAME_WEAR_ABOUT 10
#define WEAR_BIT_NAME_WEAR_WAIST 11
#define WEAR_BIT_NAME_WEAR_WRIST 12
#define WEAR_BIT_NAME_WIELD 13
#define WEAR_BIT_NAME_HOLD 14
#define WEAR_BIT_NAME_NO_SAC 15
#define WEAR_BIT_NAME_WEAR_FLOAT 16

const char *WEAR_BIT_NAMES[] = {
  " take",
  " finger",
  " neck",
  " torso",
  " head",
  " legs",
  " feet",
  " hands",
  " arms",
  " shield",
  " body",
  " waist",
  " wrist",
  " wield",
  " hold",
  " nosac",
  " float"
};

char *wear_bit_name(int wear_flags) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';
  if (wear_flags & ITEM_TAKE) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_TAKE], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_TAKE]) - 2; }
  if (wear_flags & ITEM_WEAR_FINGER) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_FINGER], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_FINGER]) - 2; }
  if (wear_flags & ITEM_WEAR_NECK) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_NECK], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_NECK]) - 2; }
  if (wear_flags & ITEM_WEAR_BODY) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_BODY], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_BODY]) - 2; }
  if (wear_flags & ITEM_WEAR_HEAD) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_HEAD], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_HEAD]) - 2; }
  if (wear_flags & ITEM_WEAR_LEGS) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_LEGS], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_LEGS]) - 2; }
  if (wear_flags & ITEM_WEAR_FEET) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_FEET], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_FEET]) - 2; }
  if (wear_flags & ITEM_WEAR_HANDS) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_HANDS], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_HANDS]) - 2; }
  if (wear_flags & ITEM_WEAR_ARMS) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_ARMS], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_ARMS]) - 2; }
  if (wear_flags & ITEM_WEAR_SHIELD) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_SHIELD], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_SHIELD]) - 2; }
  if (wear_flags & ITEM_WEAR_ABOUT) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_ABOUT], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_ABOUT]) - 2; }
  if (wear_flags & ITEM_WEAR_WAIST) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_WAIST], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_WAIST]) - 2; }
  if (wear_flags & ITEM_WEAR_WRIST) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_WRIST], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_WRIST]) - 2; }
  if (wear_flags & ITEM_WIELD) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WIELD], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WIELD]) - 2; }
  if (wear_flags & ITEM_HOLD) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_HOLD], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_HOLD]) - 2; }
  if (wear_flags & ITEM_NO_SAC) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_NO_SAC], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_NO_SAC]) - 2; }
  if (wear_flags & ITEM_WEAR_FLOAT) { strncat(buf, WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_FLOAT], sizeof(buf) - offset - 1); offset += sizeof(WEAR_BIT_NAMES[WEAR_BIT_NAME_WEAR_FLOAT]) - 2; }

  return (buf[0] != '\0') ? buf + 1 : "none";
}

#define FORM_BIT_NAME_POISON 0
#define FORM_BIT_NAME_EDIBLE 1
#define FORM_BIT_NAME_MAGICAL 2
#define FORM_BIT_NAME_INSTANT_DECAY 3
#define FORM_BIT_NAME_OTHER 4
#define FORM_BIT_NAME_ANIMAL 5
#define FORM_BIT_NAME_SENTIENT 6
#define FORM_BIT_NAME_UNDEAD 7
#define FORM_BIT_NAME_CONSTRUCT 8
#define FORM_BIT_NAME_MIST 9
#define FORM_BIT_NAME_INTANGIBLE 10
#define FORM_BIT_NAME_BIPED 11
#define FORM_BIT_NAME_CENTAUR 12
#define FORM_BIT_NAME_INSECT 13
#define FORM_BIT_NAME_SPIDER 14
#define FORM_BIT_NAME_CRUSTACEAN 15
#define FORM_BIT_NAME_WORM 16
#define FORM_BIT_NAME_BLOB 17
#define FORM_BIT_NAME_MAMMAL 18
#define FORM_BIT_NAME_BIRD 19
#define FORM_BIT_NAME_REPTILE 20
#define FORM_BIT_NAME_SNAKE 21
#define FORM_BIT_NAME_DRAGON 22
#define FORM_BIT_NAME_AMPHIBIAN 23
#define FORM_BIT_NAME_FISH 24
#define FORM_BIT_NAME_COLD_BLOOD 25

const char *FORM_BIT_NAMES[] = {
  " poison",
  " edible",
  " magical",
  " instant_rot",
  " other",
  " animal",
  " sentient",
  " undead",
  " construct",
  " mist",
  " intangible",
  " biped",
  " centaur",
  " insect",
  " spider",
  " crustacean",
  " worm",
  " blob",
  " mammal",
  " bird",
  " reptile",
  " snake",
  " dragon",
  " amphibian",
  " fish",
  " cold_blooded"
};

char *form_bit_name(int form_flags) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';
  if (form_flags & FORM_POISON) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_POISON], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_POISON]) - 2; }
  else if (form_flags & FORM_EDIBLE) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_EDIBLE], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_EDIBLE]) - 2; }
  if (form_flags & FORM_MAGICAL) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_MAGICAL], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_MAGICAL]) - 2; }
  if (form_flags & FORM_INSTANT_DECAY) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_INSTANT_DECAY], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_INSTANT_DECAY]) - 2; }
  if (form_flags & FORM_OTHER) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_OTHER], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_OTHER]) - 2; }
  if (form_flags & FORM_ANIMAL) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_ANIMAL], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_ANIMAL]) - 2; }
  if (form_flags & FORM_SENTIENT) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_SENTIENT], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_SENTIENT]) - 2; }
  if (form_flags & FORM_UNDEAD) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_UNDEAD], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_UNDEAD]) - 2; }
  if (form_flags & FORM_CONSTRUCT) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_CONSTRUCT], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_CONSTRUCT]) - 2; }
  if (form_flags & FORM_MIST) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_MIST], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_MIST]) - 2; }
  if (form_flags & FORM_INTANGIBLE) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_INTANGIBLE], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_INTANGIBLE]) - 2; }
  if (form_flags & FORM_BIPED) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_BIPED], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_BIPED]) - 2; }
  if (form_flags & FORM_CENTAUR) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_CENTAUR], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_CENTAUR]) - 2; }
  if (form_flags & FORM_INSECT) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_INSECT], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_INSECT]) - 2; }
  if (form_flags & FORM_SPIDER) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_SPIDER], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_SPIDER]) - 2; }
  if (form_flags & FORM_CRUSTACEAN) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_CRUSTACEAN], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_CRUSTACEAN]) - 2; }
  if (form_flags & FORM_WORM) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_WORM], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_WORM]) - 2; }
  if (form_flags & FORM_BLOB) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_BLOB], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_BLOB]) - 2; }
  if (form_flags & FORM_MAMMAL) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_MAMMAL], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_MAMMAL]) - 2; }
  if (form_flags & FORM_BIRD) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_BIRD], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_BIRD]) - 2; }
  if (form_flags & FORM_REPTILE) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_REPTILE], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_REPTILE]) - 2; }
  if (form_flags & FORM_SNAKE) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_SNAKE], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_SNAKE]) - 2; }
  if (form_flags & FORM_DRAGON) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_DRAGON], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_DRAGON]) - 2; }
  if (form_flags & FORM_AMPHIBIAN) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_AMPHIBIAN], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_AMPHIBIAN]) - 2; }
  if (form_flags & FORM_FISH) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_FISH], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_FISH]) - 2; }
  if (form_flags & FORM_COLD_BLOOD) { strncat(buf, FORM_BIT_NAMES[FORM_BIT_NAME_COLD_BLOOD], sizeof(buf) - offset - 1); offset += sizeof(FORM_BIT_NAMES[FORM_BIT_NAME_COLD_BLOOD]) - 2; }

  return (buf[0] != '\0') ? buf + 1 : "none";
}

#define PART_BIT_NAME_HEAD 0
#define PART_BIT_NAME_ARMS 1
#define PART_BIT_NAME_LEGS 2
#define PART_BIT_NAME_HEART 3
#define PART_BIT_NAME_BRAINS 4
#define PART_BIT_NAME_GUTS 5
#define PART_BIT_NAME_HANDS 6
#define PART_BIT_NAME_FEET 7
#define PART_BIT_NAME_FINGERS 8
#define PART_BIT_NAME_EARS 9
#define PART_BIT_NAME_EYES 10
#define PART_BIT_NAME_LONG_TONGUE 11
#define PART_BIT_NAME_EYESTALKS 12
#define PART_BIT_NAME_TENTACLES 13
#define PART_BIT_NAME_FINS 14
#define PART_BIT_NAME_WINGS 15
#define PART_BIT_NAME_TAIL 16
#define PART_BIT_NAME_CLAWS 17
#define PART_BIT_NAME_FANGS 18
#define PART_BIT_NAME_HORNS 19
#define PART_BIT_NAME_SCALES 20

const char *PART_BIT_NAMES[] = {
  " head",
  " arms",
  " legs",
  " heart",
  " brains",
  " guts",
  " hands",
  " feet",
  " fingers",
  " ears",
  " eyes",
  " long_tongue",
  " eyestalks",
  " tentacles",
  " fins",
  " wings",
  " tail",
  " claws",
  " fangs",
  " horns",
  " scales"
};

char *part_bit_name(int part_flags) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';
  if (part_flags & PART_HEAD) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_HEAD], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_HEAD]) - 2; }
  if (part_flags & PART_ARMS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_ARMS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_ARMS]) - 2; }
  if (part_flags & PART_LEGS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_LEGS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_LEGS]) - 2; }
  if (part_flags & PART_HEART) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_HEART], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_HEART]) - 2; }
  if (part_flags & PART_BRAINS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_BRAINS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_BRAINS]) - 2; }
  if (part_flags & PART_GUTS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_GUTS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_GUTS]) - 2; }
  if (part_flags & PART_HANDS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_HANDS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_HANDS]) - 2; }
  if (part_flags & PART_FEET) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_FEET], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_FEET]) - 2; }
  if (part_flags & PART_FINGERS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_FINGERS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_FINGERS]) - 2; }
  if (part_flags & PART_EAR) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_EARS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_EARS]) - 2; }
  if (part_flags & PART_EYE) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_EYES], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_EYES]) - 2; }
  if (part_flags & PART_LONG_TONGUE) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_LONG_TONGUE], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_LONG_TONGUE]) - 2; }
  if (part_flags & PART_EYESTALKS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_EYESTALKS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_EYESTALKS]) - 2; }
  if (part_flags & PART_TENTACLES) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_TENTACLES], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_TENTACLES]) - 2; }
  if (part_flags & PART_FINS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_FINS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_FINS]) - 2; }
  if (part_flags & PART_WINGS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_WINGS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_WINGS]) - 2; }
  if (part_flags & PART_TAIL) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_TAIL], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_TAIL]) - 2; }
  if (part_flags & PART_CLAWS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_CLAWS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_CLAWS]) - 2; }
  if (part_flags & PART_FANGS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_FANGS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_FANGS]) - 2; }
  if (part_flags & PART_HORNS) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_HORNS], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_HORNS]) - 2; }
  if (part_flags & PART_SCALES) { strncat(buf, PART_BIT_NAMES[PART_BIT_NAME_SCALES], sizeof(buf) - offset - 1); offset += sizeof(PART_BIT_NAMES[PART_BIT_NAME_SCALES]) - 2; }

  return (buf[0] != '\0') ? buf + 1 : "none";
}

#define WEAPON_BIT_NAME_FLAMING 0
#define WEAPON_BIT_NAME_FROST 1
#define WEAPON_BIT_NAME_VAMPIRIC 2
#define WEAPON_BIT_NAME_SHARP 3
#define WEAPON_BIT_NAME_VORPAL 4
#define WEAPON_BIT_NAME_TWO_HANDS 5
#define WEAPON_BIT_NAME_SHOCKING 6
#define WEAPON_BIT_NAME_POISON 7

const char *WEAPON_BIT_NAMES[] = {
  " flaming",
  " frost",
  " vampiric",
  " sharp",
  " vorpal",
  " two-handed",
  " shocking",
  " poison"
};

char *weapon_bit_name(int weapon_flags) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';
  if (weapon_flags & WEAPON_FLAMING) { strncat(buf, WEAPON_BIT_NAMES[WEAPON_BIT_NAME_FLAMING], sizeof(buf) - offset - 1); offset += sizeof(WEAPON_BIT_NAMES[WEAPON_BIT_NAME_FLAMING]) - 2; }
  if (weapon_flags & WEAPON_FROST) { strncat(buf, WEAPON_BIT_NAMES[WEAPON_BIT_NAME_FROST], sizeof(buf) - offset - 1); offset += sizeof(WEAPON_BIT_NAMES[WEAPON_BIT_NAME_FROST]) - 2; }
  if (weapon_flags & WEAPON_VAMPIRIC) { strncat(buf, WEAPON_BIT_NAMES[WEAPON_BIT_NAME_VAMPIRIC], sizeof(buf) - offset - 1); offset += sizeof(WEAPON_BIT_NAMES[WEAPON_BIT_NAME_VAMPIRIC]) - 2; }
  if (weapon_flags & WEAPON_SHARP) { strncat(buf, WEAPON_BIT_NAMES[WEAPON_BIT_NAME_SHARP], sizeof(buf) - offset - 1); offset += sizeof(WEAPON_BIT_NAMES[WEAPON_BIT_NAME_SHARP]) - 2; }
  if (weapon_flags & WEAPON_VORPAL) { strncat(buf, WEAPON_BIT_NAMES[WEAPON_BIT_NAME_VORPAL], sizeof(buf) - offset - 1); offset += sizeof(WEAPON_BIT_NAMES[WEAPON_BIT_NAME_VORPAL]) - 2; }
  if (weapon_flags & WEAPON_TWO_HANDS) { strncat(buf, WEAPON_BIT_NAMES[WEAPON_BIT_NAME_TWO_HANDS], sizeof(buf) - offset - 1); offset += sizeof(WEAPON_BIT_NAMES[WEAPON_BIT_NAME_TWO_HANDS]) - 2; }
  if (weapon_flags & WEAPON_SHOCKING) { strncat(buf, WEAPON_BIT_NAMES[WEAPON_BIT_NAME_SHOCKING], sizeof(buf) - offset - 1); offset += sizeof(WEAPON_BIT_NAMES[WEAPON_BIT_NAME_SHOCKING]) - 2; }
  if (weapon_flags & WEAPON_POISON) { strncat(buf, WEAPON_BIT_NAMES[WEAPON_BIT_NAME_POISON], sizeof(buf) - offset - 1); offset += sizeof(WEAPON_BIT_NAMES[WEAPON_BIT_NAME_POISON]) - 2; }

  return (buf[0] != '\0') ? buf + 1 : "none";
}

#define CONTAINER_BIT_NAME_CLOSEABLE 0
#define CONTAINER_BIT_NAME_PICKPROOF 1
#define CONTAINER_BIT_NAME_CLOSED 2
#define CONTAINER_BIT_NAME_LOCKED 3

const char *CONTAINER_BIT_NAMES[] = {
  " closable",
  " pickproof",
  " closed",
  " locked"
};

char *cont_bit_name(int cont_flags) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';

  if (cont_flags & CONT_CLOSEABLE) { strncat(buf, CONTAINER_BIT_NAMES[CONTAINER_BIT_NAME_CLOSEABLE], sizeof(buf) - offset - 1); offset += sizeof(CONTAINER_BIT_NAMES[CONTAINER_BIT_NAME_CLOSEABLE]) - 2; }
  if (cont_flags & CONT_PICKPROOF) { strncat(buf, CONTAINER_BIT_NAMES[CONTAINER_BIT_NAME_PICKPROOF], sizeof(buf) - offset - 1); offset += sizeof(CONTAINER_BIT_NAMES[CONTAINER_BIT_NAME_PICKPROOF]) - 2; }
  if (cont_flags & CONT_CLOSED) { strncat(buf, CONTAINER_BIT_NAMES[CONTAINER_BIT_NAME_CLOSED], sizeof(buf) - offset - 1); offset += sizeof(CONTAINER_BIT_NAMES[CONTAINER_BIT_NAME_CLOSED]) - 2; }
  if (cont_flags & CONT_LOCKED) { strncat(buf, CONTAINER_BIT_NAMES[CONTAINER_BIT_NAME_LOCKED], sizeof(buf) - offset - 1); offset += sizeof(CONTAINER_BIT_NAMES[CONTAINER_BIT_NAME_LOCKED]) - 2; }

  return (buf[0] != '\0') ? buf + 1 : "none";
}

#define OFF_BIT_NAME_AREA_ATTACK 0
#define OFF_BIT_NAME_BACKSTAB 1
#define OFF_BIT_NAME_BASH 2
#define OFF_BIT_NAME_BERSERK 3
#define OFF_BIT_NAME_DISARM 4
#define OFF_BIT_NAME_DODGE 5
#define OFF_BIT_NAME_FADE 6
#define OFF_BIT_NAME_FAST 7
#define OFF_BIT_NAME_KICK 8
#define OFF_BIT_NAME_KICK_DIRT 9
#define OFF_BIT_NAME_PARRY 10
#define OFF_BIT_NAME_RESCUE 11
#define OFF_BIT_NAME_TAIL 12
#define OFF_BIT_NAME_TRIP 13
#define OFF_BIT_NAME_CRUSH 14

const char *OFFENSE_BIT_NAMES[] = {
  " area attack",
  " backstab",
  " bash",
  " berserk",
  " disarm",
  " dodge",
  " fade",
  " fast",
  " kick",
  " kick_dirt",
  " parry",
  " rescue",
  " tail",
  " trip",
  " crush"
};

#define ASSIST_BIT_NAME_ALL 0
#define ASSIST_BIT_NAME_ALIGN 1
#define ASSIST_BIT_NAME_RACE 2
#define ASSIST_BIT_NAME_PLAYERS 3
#define ASSIST_BIT_NAME_GUARD 4
#define ASSIST_BIT_NAME_VNUM 5

const char *ASSIST_BIT_NAMES[] = {
  " assist_all",
  " assist_align",
  " assist_race",
  " assist_players",
  " assist_guard",
  " assist_vnum"
};

char *off_bit_name(int off_flags) {
  static char buf[512];
  unsigned offset = 0;

  buf[0] = '\0';

  if (off_flags & OFF_AREA_ATTACK) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_AREA_ATTACK], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_AREA_ATTACK]) - 2; }
  if (off_flags & OFF_BACKSTAB) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_BACKSTAB], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_BACKSTAB]) - 2; }
  if (off_flags & OFF_BASH) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_BASH], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_BASH]) - 2; }
  if (off_flags & OFF_BERSERK) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_BERSERK], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_BERSERK]) - 2; }
  if (off_flags & OFF_DISARM) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_DISARM], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_DISARM]) - 2; }
  if (off_flags & OFF_DODGE) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_DODGE], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_DODGE]) - 2; }
  if (off_flags & OFF_FADE) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_FADE], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_FADE]) - 2; }
  if (off_flags & OFF_FAST) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_FAST], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_FAST]) - 2; }
  if (off_flags & OFF_KICK) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_KICK], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_KICK]) - 2; }
  if (off_flags & OFF_KICK_DIRT) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_KICK_DIRT], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_KICK_DIRT]) - 2; }
  if (off_flags & OFF_PARRY) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_PARRY], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_PARRY]) - 2; }
  if (off_flags & OFF_RESCUE) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_RESCUE], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_RESCUE]) - 2; }
  if (off_flags & OFF_TAIL) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_TAIL], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_TAIL]) - 2; }
  if (off_flags & OFF_TRIP) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_TRIP], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_TRIP]) - 2; }
  if (off_flags & OFF_CRUSH) { strncat(buf, OFFENSE_BIT_NAMES[OFF_BIT_NAME_CRUSH], sizeof(buf) - offset - 1); offset += sizeof(OFFENSE_BIT_NAMES[OFF_BIT_NAME_CRUSH]) - 2; }
  if (off_flags & ASSIST_ALL) { strncat(buf, ASSIST_BIT_NAMES[ASSIST_BIT_NAME_ALL], sizeof(buf) - offset - 1); offset += sizeof(ASSIST_BIT_NAMES[ASSIST_BIT_NAME_ALL]) - 2; }
  if (off_flags & ASSIST_ALIGN) { strncat(buf, ASSIST_BIT_NAMES[ASSIST_BIT_NAME_ALIGN], sizeof(buf) - offset - 1); offset += sizeof(ASSIST_BIT_NAMES[ASSIST_BIT_NAME_ALIGN]) - 2; }
  if (off_flags & ASSIST_RACE) { strncat(buf, ASSIST_BIT_NAMES[ASSIST_BIT_NAME_RACE], sizeof(buf) - offset - 1); offset += sizeof(ASSIST_BIT_NAMES[ASSIST_BIT_NAME_RACE]) - 2; }
  if (off_flags & ASSIST_PLAYERS) { strncat(buf, ASSIST_BIT_NAMES[ASSIST_BIT_NAME_PLAYERS], sizeof(buf) - offset - 1); offset += sizeof(ASSIST_BIT_NAMES[ASSIST_BIT_NAME_PLAYERS]) - 2; }
  if (off_flags & ASSIST_GUARD) { strncat(buf, ASSIST_BIT_NAMES[ASSIST_BIT_NAME_GUARD], sizeof(buf) - offset - 1); offset += sizeof(ASSIST_BIT_NAMES[ASSIST_BIT_NAME_GUARD]) - 2; }
  if (off_flags & ASSIST_VNUM) { strncat(buf, ASSIST_BIT_NAMES[ASSIST_BIT_NAME_VNUM], sizeof(buf) - offset - 1); offset += sizeof(ASSIST_BIT_NAMES[ASSIST_BIT_NAME_VNUM]) - 2; }

  return (buf[0] != '\0') ? buf + 1 : "none";
}
