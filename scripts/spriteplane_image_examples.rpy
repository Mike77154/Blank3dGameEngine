# Blank3D generic image-plane examples. This file is documentation/example only;
# it is not included by startup.rpy.

# Sticker / wall mark. Nudge the plane slightly off the wall to avoid z-fighting.
spriteplane "art/stickers/example.tga" pos 2 1 -5 size 1.2 1.2 mode fixed axis xy life 0 blend alpha depth 1

# Static television/monitor face.
spriteplane "art/screens/example.png" pos -3 2 -8 size 4 2.25 mode fixed axis xy life 0 blend alpha depth 1

# Camera-facing world billboard.
spriteplane "art/fx/example.png" pos 0 2 -6 size 2 2 mode camera life 0 blend alpha depth 1

# Register a numeric image for HUD / BigHUD / crosshair-like authoring.
image_asset 2100 "art/hud/example.png"
