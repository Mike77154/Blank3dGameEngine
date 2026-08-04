# Weapon catalog and player loadout split

Blank3D now separates three responsibilities:

1. `config/weapons/weapons.ini`
   - Global catalog shared by every entity.
   - Lists every weapon profile.
   - Defines maximum ownership and ammunition capacity.
2. `config/weapons/player_weapons.ini`
   - Player-only starting possession.
   - Defines initial reserve ammunition and initially equipped weapon.
3. `config/weapons/<weapon>.ini`
   - One behavior recipe per weapon: projectile, casing, sound, fire mode,
     spread, physics, trail, recoil, reload and other weapon modules.

NPCs and future entity families can receive their own loadout files without
copying or changing the global catalog.
