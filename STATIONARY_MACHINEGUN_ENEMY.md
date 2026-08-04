# Stationary machine-gun enemy

The first enemy instance now loads `config/entities/gunner_enemy.ini`.
Its GFO owns `scripts/gunner_enemy.fpi`, whose only behavior is:

    :always:lookatplayer,fireplayer

`fireplayer` binds a dedicated NPC actor to the existing `gweapon89` manager,
equips weapon id 2 (`machine_gun.ini`), and submits normal fire input with the
enemy muzzle and player-directed basis vectors. Projectiles, casings, trails,
audio, cadence and damage therefore use the same weapon stack as the player.
NPC ammo uses the manager's internal bank; the player's GKInventory provider is
used only for actor id 1.
