/*
 * Tatoosh - Game Data Model
 *
 * Synchronizes Quake game state (cl.stats[], cl.items) to RmlUI data model.
 * The data model is automatically updated each frame and can be used in
 * RML documents via data binding expressions.
 *
 * Usage in RML:
 *   <body data-model="game">
 *     <div>Health: {{ health }}</div>
 *     <div data-if="has_quad">QUAD DAMAGE!</div>
 *   </body>
 */

#ifndef TATOOSH_GAME_DATA_MODEL_H
#define TATOOSH_GAME_DATA_MODEL_H

#include <RmlUi/Core.h>

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
    Rml::String level_name;
    Rml::String map_name;

    // Time
    int time_minutes = 0;
    int time_seconds = 0;

    // Face animation state (0-4 health tier, plus special states)
    int face_index = 0;
    bool face_pain = false;
};

// Global game state that gets synced each frame
extern GameState g_game_state;

class GameDataModel {
public:
    // Initialize the data model with the RmlUI context
    // Returns true on success
    static bool Initialize(Rml::Context* context);

    // Shutdown and cleanup
    static void Shutdown();

    // Update the data model from Quake's game state
    // Call this each frame (typically from UI_Update)
    static void Update();

    // Force a dirty check on all variables (call after level load)
    static void MarkAllDirty();

    // Check if initialized
    static bool IsInitialized();

private:
    static Rml::DataModelHandle s_model_handle;
    static bool s_initialized;
};

} // namespace Tatoosh

// C API for integration with vkQuake
#ifdef __cplusplus
extern "C" {
#endif

// Initialize game data model (call after UI_Init)
int GameDataModel_Init(void);

// Shutdown game data model
void GameDataModel_Shutdown(void);

// Update game data from Quake state (call each frame)
void GameDataModel_Update(void);

// Sync from Quake's game state
// stats: pointer to cl.stats[] array (MAX_CL_STATS ints)
// items: cl.items bitfield
void GameDataModel_SyncFromQuake(const int* stats, int items,
                                  int intermission, int gametype,
                                  const char* level_name, const char* map_name,
                                  double game_time);

#ifdef __cplusplus
}
#endif

#endif // TATOOSH_GAME_DATA_MODEL_H
