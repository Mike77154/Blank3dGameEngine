# PRESET_DESIGN_NOTES

Los 96 presets nuevos no copian assets propietarios. Son combinaciones paramétricas
originales construidas a partir de familias de retícula ampliamente usadas:

- cruz estática/dinámica con gap, largo, grosor y punto;
- punto mínimo para precisión/hitscan;
- círculo o elipse para tracking, proyectiles, melee y área;
- retículas que crecen con dispersión/bloom;
- T y postes para compensación visual y scopes;
- figuras segmentadas, brackets y chevrons para lock-on y HUD sci-fi;
- variantes grandes y de alto contraste para accesibilidad.

Fuentes de investigación consultadas el 14 de julio de 2026:

- ProSettings, CS2 Crosshair Generator y database.
- Steam Community, opciones de retícula de Overwatch 2 y estilos de CS2/Quake.
- PC Gamer y Polygon, categorías de retícula de Marvel Rivals.
- Halopedia, resumen de formas comunes de retícula.
- documentación comunitaria de Quake/eZQuake sobre tipo, color, escala,
  transparencia y offset.

La implementación permanece agnóstica al juego: usa nombres descriptivos y
parámetros genéricos en lugar de clonar códigos o texturas concretas.
