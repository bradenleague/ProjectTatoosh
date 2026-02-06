/*
 * Tatoosh - Game State Domain Object
 *
 * Represents the current state of the game for UI display.
 * This is a pure data structure with no framework dependencies.
 */

#ifndef TATOOSH_DOMAIN_GAME_STATE_H
#define TATOOSH_DOMAIN_GAME_STATE_H

#include <string>

namespace Tatoosh {

// Game state structure that mirrors Quake's cl.stats and cl.items
struct GameState {
    // Core stats (from cl.stats[])
    int health = 100;
    int armor = 0;
    int ammo = 0;
    int active_weapon = 0;

    // Ammo counts
    int shells = 0;
    int nails = 0;
    int rockets = 0;
    int cells = 0;

    // Level statistics
    int monsters = 0;
    int total_monsters = 0;
    int secrets = 0;
    int total_secrets = 0;

    // Weapons owned (from cl.items bitflags)
    bool has_shotgun = false;
    bool has_super_shotgun = false;
    bool has_nailgun = false;
    bool has_super_nailgun = false;
    bool has_grenade_launcher = false;
    bool has_rocket_launcher = false;
    bool has_lightning_gun = false;

    // Keys
    bool has_key1 = false;  // Silver key
    bool has_key2 = false;  // Gold key

    // Powerups
    bool has_invisibility = false;
    bool has_invulnerability = false;
    bool has_suit = false;
    bool has_quad = false;

    // Sigils (runes)
    bool has_sigil1 = false;
    bool has_sigil2 = false;
    bool has_sigil3 = false;
    bool has_sigil4 = false;

    // Armor type (0=none, 1=green, 2=yellow, 3=red)
    int armor_type = 0;

    // Game state flags
    bool intermission = false;
    bool deathmatch = false;
    bool coop = false;

    // Level info
    std::string level_name;
    std::string map_name;

    // Time
    int time_minutes = 0;
    int time_seconds = 0;

    // Face animation state (0-4 health tier, plus special states)
    int face_index = 0;
    bool face_pain = false;
};

} // namespace Tatoosh

#endif // TATOOSH_DOMAIN_GAME_STATE_H
