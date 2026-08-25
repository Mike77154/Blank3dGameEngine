# gloco89 v0.2 - engine-ready provider mode

## Autoridad

GLOCO es un controlador/politica de locomocion. No pretende ser identidad de entidad, scene graph, world manager ni autoridad final de transform en un host grande.

Un host puede mapear su identidad canonica a un slot interno de GLOCO y delegar solo las etapas que le convienen.

## Capacidades configurables

```c
#define GLOCO_MAX_ACTORS   64
#define GLOCO_MAX_PROFILES 64
```

Los defaults siguen definidos por el header y pueden sustituirse desde el build.

## StatsProvider

`GLOCO_StatsProvider` ofrece `get_stamina`, `set_stamina` y `consume_stamina`.

- `GLOCO_PROVIDER_HANDLED`: el host es autoridad de esa operacion.
- `GLOCO_PROVIDER_FALLBACK`: GLOCO usa su almacenamiento interno.

Esto permite que stamina viva en NumSys, RPG stats u otro backend sin quitar la capacidad standalone.

## Speed override

`GLOCO_Input.speed_override > 0` sustituye la velocidad objetivo elegida por el perfil para ese tick. Es apropiado para path following, AI y motion planners que producen una velocidad deseada sin querer editar el preset permanente.

## HARD_STOP

El evento `GLOCO_EVENT_HARD_STOP` es edge-triggered: se emite una vez al entrar en hard-stop, se rearma al soltar y puede volver a emitirse al presionar de nuevo.

## Fallbacks

Todos los providers son opcionales. Si no hay provider o devuelve `GLOCO_PROVIDER_FALLBACK`, se conserva la implementacion interna de GLOCO.
