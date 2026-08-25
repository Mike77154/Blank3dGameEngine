# Master HUD recipes

Edit only `use=` to swap pieces through a catalog.

Example:

```ini
[recipe]
include=../components/sniper_mask.ini
include=../components/sniper_telemetry.ini
select=scope
select=zoom
select=sway

[scope]
catalog=../catalogs/scopes.ini
use=pso1
```

The selected catalog maps `pso1` to its formal preset INI. That preset may itself include other INIs.
