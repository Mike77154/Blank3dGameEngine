# RPYL vendorizado: parser/runtime real; usa VM cuando el bytecode es válido.
# La percepción ya no se configura en el spawn. Cada arquetipo carga
# [perception] desde config/entities/<archetype>.ini. Los verbos inline
# antiguos siguen aceptados únicamente como compatibilidad/override.
label start:
    scene blank3d
    # window size moved to config/GeneralConfig.cfg (GenWinConfigC89)
    camera thirdperson dist 8 height 4 fov 70
    show_grid 1
    player pos 0 0 0
    player floor 0
    player flyingentity 1
    player yaw 0
    movement move 7 strafe 5.5 turn 140 vertical 4
    # Vehicle test yard. Every box is authored from an external INI.
    # gvehpos89 decides occupancy, Mount89 owns the rider/seat relationship,
    # and the Vehicle System chooses movement/physics providers per recipe.
    vehicle_ini "config/vehicles/test_car.ini" pos 5 2.5 7
    vehicle_ini "config/vehicles/test_motorcycle.ini" pos -5 2.3 8
    vehicle_ini "config/vehicles/test_tank.ini" pos 14 1.0 9
    # Test pickups: both are ordinary GFO/Thing objects, not Actors.
    # Change only these coordinates to reposition them in the scene.
    weapon_pickup "config/pickups/uzi_weapon.ini" pos -6.5 0 24
    ammo_pickup "config/pickups/uzi_ammo.ini" pos -9.5 0 24
    # Generic item-handler proof: this is NOT a weapon command.
    # At 100 HP it stays in the world; after damage it heals 25 and is consumed.
    pickup "config/pickups/health_box.ini" pos -4.5 0 24
    armed_ally 7 pos -4 0 8 hp 60 loadout "config/npc_loadouts/armed_ally.ini" faction allies team survivors role armed_ally tags "human,organic,armed,ally,targetable"
    gunner_enemy 0 pos 0 0 18 hp 30 loadout "config/npc_loadouts/gunner_enemy.ini"
    zombie 1 pos -8 0 24 hp 30
    zombie 2 pos 8 0 24 hp 30
    hopper_enemy 3 pos 0 0 34 hp 30 floor 0
    dive_enemy 4 pos 0 8 12 hp 30 floor 0 flyingentity 1
    air_lunger_enemy 5 pos -14 0 34 hp 34 floor 0
    ground_lancer_enemy 6 pos 14 0 34 hp 42 floor 0
