# gweaponfoley89 1.6

Sintetizador C89 fixed-point de ruidos mecánicos de armas basado únicamente en
tres corrientes de ruido blanco.

La versión 1.6 conserva la partícula metálica brillante de la 1.5 y añade:

- amortiguación y cuerpo distintos por microcontacto;
- tres variantes deterministas por preset;
- velocidad lenta, normal y rápida;
- pequeñas variaciones de timing, ganancia, cutoff, release y cuerpo;
- salida separada `dry` / `room`;
- control del envío final al cuarto.

No genera disparos, explosiones, recargas, casquillos ni bombeo de shotgun.

```c
gwf89_trigger_ex(&ctx, GWF89_SNIPER_BOLT_DRY,
                 seed, 1U, GWF89_SPEED_NORMAL);
```

Para obtener stems:

```c
mixed = gwf89_process_sample_stems(&ctx, &dry, &room);
```

Todo el estado pertenece al caller. No hay heap, `float` ni `double`.
