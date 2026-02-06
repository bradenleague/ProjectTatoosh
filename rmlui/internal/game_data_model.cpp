/*
 * Tatoosh - Game Data Model Implementation
 *
 * Synchronizes Quake game state to RmlUI data binding system.
 */

#include "game_data_model.h"

// Quake stat indices (from quakedef.h)
#define STAT_HEALTH         0
#define STAT_WEAPON         2
#define STAT_AMMO           3
#define STAT_ARMOR          4
#define STAT_SHELLS         6
#define STAT_NAILS          7
#define STAT_ROCKETS        8
#define STAT_CELLS          9
#define STAT_ACTIVEWEAPON   10
#define STAT_TOTALSECRETS   11
#define STAT_TOTALMONSTERS  12
#define STAT_SECRETS        13
#define STAT_MONSTERS       14
#define STAT_ITEMS          15

// Item bitflags (from quakedef.h)
#define IT_SHOTGUN          1
#define IT_SUPER_SHOTGUN    2
#define IT_NAILGUN          4
#define IT_SUPER_NAILGUN    8
#define IT_GRENADE_LAUNCHER 16
#define IT_ROCKET_LAUNCHER  32
#define IT_LIGHTNING        64
#define IT_ARMOR1           8192
#define IT_ARMOR2           16384
#define IT_ARMOR3           32768
#define IT_KEY1             131072
#define IT_KEY2             262144
#define IT_INVISIBILITY     524288
#define IT_INVULNERABILITY  1048576
#define IT_SUIT             2097152
#define IT_QUAD             4194304
#define IT_SIGIL1           (1<<28)
#define IT_SIGIL2           (1<<29)
#define IT_SIGIL3           (1<<30)
#define IT_SIGIL4           (1<<31)

extern "C" {
    void Con_Printf(const char* fmt, ...);
}

namespace Tatoosh {

// Global game state
GameState g_game_state;

// Static members
Rml::DataModelHandle GameDataModel::s_model_handle;
bool GameDataModel::s_initialized = false;

bool GameDataModel::Initialize(Rml::Context* context)
{
    if (s_initialized) {
        Con_Printf("GameDataModel: Already initialized\n");
        return true;
    }

    if (!context) {
        Con_Printf("GameDataModel: ERROR - null context\n");
        return false;
    }

    Rml::DataModelConstructor constructor = context->CreateDataModel("game");
    if (!constructor) {
        Con_Printf("GameDataModel: ERROR - Failed to create data model\n");
        return false;
    }

    // Bind core stats
    constructor.Bind("health", &g_game_state.health);
    constructor.Bind("armor", &g_game_state.armor);
    constructor.Bind("ammo", &g_game_state.ammo);
    constructor.Bind("active_weapon", &g_game_state.active_weapon);

    // Bind ammo counts
    constructor.Bind("shells", &g_game_state.shells);
    constructor.Bind("nails", &g_game_state.nails);
    constructor.Bind("rockets", &g_game_state.rockets);
    constructor.Bind("cells", &g_game_state.cells);

    // Bind level statistics
    constructor.Bind("monsters", &g_game_state.monsters);
    constructor.Bind("total_monsters", &g_game_state.total_monsters);
    constructor.Bind("secrets", &g_game_state.secrets);
    constructor.Bind("total_secrets", &g_game_state.total_secrets);

    // Bind weapon ownership
    constructor.Bind("has_shotgun", &g_game_state.has_shotgun);
    constructor.Bind("has_super_shotgun", &g_game_state.has_super_shotgun);
    constructor.Bind("has_nailgun", &g_game_state.has_nailgun);
    constructor.Bind("has_super_nailgun", &g_game_state.has_super_nailgun);
    constructor.Bind("has_grenade_launcher", &g_game_state.has_grenade_launcher);
    constructor.Bind("has_rocket_launcher", &g_game_state.has_rocket_launcher);
    constructor.Bind("has_lightning_gun", &g_game_state.has_lightning_gun);

    // Bind keys
    constructor.Bind("has_key1", &g_game_state.has_key1);
    constructor.Bind("has_key2", &g_game_state.has_key2);

    // Bind powerups
    constructor.Bind("has_invisibility", &g_game_state.has_invisibility);
    constructor.Bind("has_invulnerability", &g_game_state.has_invulnerability);
    constructor.Bind("has_suit", &g_game_state.has_suit);
    constructor.Bind("has_quad", &g_game_state.has_quad);

    // Bind sigils
    constructor.Bind("has_sigil1", &g_game_state.has_sigil1);
    constructor.Bind("has_sigil2", &g_game_state.has_sigil2);
    constructor.Bind("has_sigil3", &g_game_state.has_sigil3);
    constructor.Bind("has_sigil4", &g_game_state.has_sigil4);

    // Bind armor type
    constructor.Bind("armor_type", &g_game_state.armor_type);

    // Bind game state flags
    constructor.Bind("intermission", &g_game_state.intermission);
    constructor.Bind("deathmatch", &g_game_state.deathmatch);
    constructor.Bind("coop", &g_game_state.coop);

    // Bind level info
    constructor.Bind("level_name", &g_game_state.level_name);
    constructor.Bind("map_name", &g_game_state.map_name);

    // Bind time
    constructor.Bind("time_minutes", &g_game_state.time_minutes);
    constructor.Bind("time_seconds", &g_game_state.time_seconds);

    // Bind face state
    constructor.Bind("face_index", &g_game_state.face_index);
    constructor.Bind("face_pain", &g_game_state.face_pain);

    s_model_handle = constructor.GetModelHandle();
    s_initialized = true;

    Con_Printf("GameDataModel: Initialized successfully\n");
    return true;
}

void GameDataModel::Shutdown()
{
    if (!s_initialized) return;

    // RmlUI handles cleanup when context is destroyed
    s_model_handle = Rml::DataModelHandle();
    s_initialized = false;

    Con_Printf("GameDataModel: Shutdown\n");
}

void GameDataModel::Update()
{
    if (!s_initialized || !s_model_handle) return;

    // Mark the model as dirty so RmlUI re-evaluates bindings
    s_model_handle.DirtyAllVariables();
}

void GameDataModel::MarkAllDirty()
{
    if (!s_initialized || !s_model_handle) return;
    s_model_handle.DirtyAllVariables();
}

bool GameDataModel::IsInitialized()
{
    return s_initialized;
}

} // namespace Tatoosh

// C API Implementation
extern "C" {

int GameDataModel_Init(void)
{
    // Initialization is deferred - called from UI_InitializeVulkan
    // after context is fully ready
    return 1;
}

void GameDataModel_Shutdown(void)
{
    Tatoosh::GameDataModel::Shutdown();
}

void GameDataModel_Update(void)
{
    Tatoosh::GameDataModel::Update();
}

void GameDataModel_SyncFromQuake(const int* stats, int items,
                                  int intermission, int gametype,
                                  int maxclients,
                                  const char* level_name, const char* map_name,
                                  double game_time)
{
    using namespace Tatoosh;

    if (!stats) return;

    // Sync core stats
    g_game_state.health = stats[STAT_HEALTH];
    g_game_state.armor = stats[STAT_ARMOR];
    g_game_state.ammo = stats[STAT_AMMO];
    g_game_state.active_weapon = stats[STAT_ACTIVEWEAPON];

    // Sync ammo counts
    g_game_state.shells = stats[STAT_SHELLS];
    g_game_state.nails = stats[STAT_NAILS];
    g_game_state.rockets = stats[STAT_ROCKETS];
    g_game_state.cells = stats[STAT_CELLS];

    // Sync level statistics
    g_game_state.monsters = stats[STAT_MONSTERS];
    g_game_state.total_monsters = stats[STAT_TOTALMONSTERS];
    g_game_state.secrets = stats[STAT_SECRETS];
    g_game_state.total_secrets = stats[STAT_TOTALSECRETS];

    // Decode item bitflags for weapons
    g_game_state.has_shotgun = (items & IT_SHOTGUN) != 0;
    g_game_state.has_super_shotgun = (items & IT_SUPER_SHOTGUN) != 0;
    g_game_state.has_nailgun = (items & IT_NAILGUN) != 0;
    g_game_state.has_super_nailgun = (items & IT_SUPER_NAILGUN) != 0;
    g_game_state.has_grenade_launcher = (items & IT_GRENADE_LAUNCHER) != 0;
    g_game_state.has_rocket_launcher = (items & IT_ROCKET_LAUNCHER) != 0;
    g_game_state.has_lightning_gun = (items & IT_LIGHTNING) != 0;

    // Decode keys
    g_game_state.has_key1 = (items & IT_KEY1) != 0;
    g_game_state.has_key2 = (items & IT_KEY2) != 0;

    // Decode powerups
    g_game_state.has_invisibility = (items & IT_INVISIBILITY) != 0;
    g_game_state.has_invulnerability = (items & IT_INVULNERABILITY) != 0;
    g_game_state.has_suit = (items & IT_SUIT) != 0;
    g_game_state.has_quad = (items & IT_QUAD) != 0;

    // Decode sigils
    g_game_state.has_sigil1 = (items & IT_SIGIL1) != 0;
    g_game_state.has_sigil2 = (items & IT_SIGIL2) != 0;
    g_game_state.has_sigil3 = (items & IT_SIGIL3) != 0;
    g_game_state.has_sigil4 = (items & IT_SIGIL4) != 0;

    // Determine armor type from items
    if (items & IT_ARMOR3) {
        g_game_state.armor_type = 3;  // Red
    } else if (items & IT_ARMOR2) {
        g_game_state.armor_type = 2;  // Yellow
    } else if (items & IT_ARMOR1) {
        g_game_state.armor_type = 1;  // Green
    } else {
        g_game_state.armor_type = 0;  // None
    }

    // Game state
    g_game_state.intermission = (intermission != 0);
    g_game_state.deathmatch = (gametype != 0);
    g_game_state.coop = (gametype == 0 && maxclients > 1);

    // Level info
    if (level_name) {
        g_game_state.level_name = level_name;
    }
    if (map_name) {
        g_game_state.map_name = map_name;
    }

    // Time calculation
    int total_seconds = static_cast<int>(game_time);
    g_game_state.time_minutes = total_seconds / 60;
    g_game_state.time_seconds = total_seconds % 60;

    // Calculate face index based on health
    int health = g_game_state.health;
    if (health >= 100) {
        g_game_state.face_index = 4;
    } else if (health >= 80) {
        g_game_state.face_index = 3;
    } else if (health >= 60) {
        g_game_state.face_index = 2;
    } else if (health >= 40) {
        g_game_state.face_index = 1;
    } else {
        g_game_state.face_index = 0;
    }
}

} // extern "C"
