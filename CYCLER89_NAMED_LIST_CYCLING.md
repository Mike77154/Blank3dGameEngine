# Blank3D v3.13.0 — cycler89 y listas activas con nombre

El ciclo de armas dejó de depender de una tabla fija del runner. La secuencia
anterior estaba escrita directamente en C como IDs concretos, así que cualquier
arma nueva podía cargarse desde INI y aun así quedar fuera de N/M.

Ahora el flujo es:

```text
DDSL2 input
   |
   | list_cyclenext=active_weapon
   v
Blank3DListCycleRegistry
   |
   v
cycler89 provider list
   | count / read / is_active
   v
host activate callback
```

## Sintaxis DDSL2

```text
If key_press M then list_cyclenext=active_weapon
If key_press N then list_cycleprev=active_weapon
```

Alias admitidos por el host:

```text
list_cyclenext
list_cycle_next
list_next

list_cycleprev
list_cycle_prev
list_prev
```

El valor de la derecha es el nombre de una lista registrada. DDSL2 no conoce
armas ni inventarios; emite un nombre simbólico y el host resuelve la lista.

## cycler89

Ubicación:

```text
vendor/cycler89/
├── include/cycler89.h
├── src/cycler89.c
├── Makefile
└── README.md
```

La librería solo conoce:

- cantidad de elementos;
- lectura de un elemento estable por índice;
- consulta opcional de si ese elemento está activo;
- elemento actual;
- dirección `next` o `previous`.

No conoce armas, DDSL2, Blank3D, inventarios ni ventanas. El host conserva toda
la memoria y puede representar cada elemento mediante un `long` estable.

Comportamiento:

- salta elementos inactivos;
- hace wrap-around en ambos sentidos;
- si el elemento actual desapareció, `next` recupera el primer activo y `prev`
  el último activo;
- si solo queda un activo, permanece seleccionado;
- no crea listas compactadas temporales;
- no usa `malloc`, `realloc` ni `free`.

## Registro de listas del engine

`src/blank3d_list_cycle.*` mantiene un registro fijo de listas con nombre. Cada
binding aporta:

```text
Cycler89List provider
get_current(context)
activate(context, selected_item)
```

La primera lista registrada es:

```text
active_weapon
```

Su fuente es el orden real de perfiles dentro de `GWP89_Manager`, que a su vez
proviene de la sección `[weapons]` del manifiesto. Su predicate `is_active`
consulta la propiedad viva en GKInventory. Por ello:

- una nueva arma INI registrada entra automáticamente al ciclo;
- quitar o recoger un arma afecta la siguiente pulsación inmediatamente;
- el orden se controla desde el manifiesto, no desde una tabla C;
- los IDs pueden ser no consecutivos.

El mouse wheel usa el mismo registro de lista. Las teclas M/N ya no se consultan
directamente desde el loop Win32; viven en `scripts/player.ddsl2`.

## NPCs

El ciclo de inventarios NPC también fue migrado a `cycler89`. Cada NPC expone su
array privado de IDs como provider y conserva el equipamiento mediante su propio
manager e inventario. Esto elimina una segunda implementación manual del mismo
algoritmo de wrap-around.

## Próximas listas posibles

Sin cambiar DDSL2 ni cycler89, el engine puede registrar nombres como:

```text
active_inventory_item
active_skill
active_target
menu_option
camera_preset
conversation_choice
```

Cada lista decide qué significa “activo” y cómo aplicar el elemento elegido.
