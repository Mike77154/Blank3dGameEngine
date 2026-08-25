# Integración sugerida

## Con un weapon system agnóstico

```ini
[visual]
ammo_profile=sniper
shell_part=shell
projectile_part=projectile
outline=1
outline_width=2
fill=#B56A3A
outline_color=#171A1F
```

El parser de INI queda fuera de GProj2D89. El engine traduce `ammo_profile` al `ammo_id` correspondiente y llama `gp2d_build_part`.

## Con gbar o un HUD

Para dibujar cartuchos restantes:

1. genera una sola escena base del perfil;
2. cachea los paths en almacenamiento estático;
3. repite la escena con transformaciones distintas para cada segmento;
4. cuando una bala se consume, deja de emitir ese segmento.

## Con un renderer de triángulos

GProj2D89 entrega contornos. El backend puede:

- triangular con un fan para shapes convexos;
- usar un triangulador propio para shapes futuros cóncavos;
- emitir los contornos como líneas;
- rasterizar a una textura de atlas en una etapa offline.
