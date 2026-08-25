# gweaponloadout89

Portable catalog + actor loadout parser. It owns no filesystem and no inventory
backend. File reads go through `GWP89_SERVICE_IO`; applying the resulting loadout
to an actor is left to the registered `GWP89_SERVICE_INVENTORY` provider.
