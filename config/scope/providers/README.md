# Provider recipes

- `internal.ini` forces the complete bundled path.
- `plug_and_play.ini` uses `auto:host` for every provider-capable domain. With no registered `host`, behavior is identical to the internal library.
- `external_required_example.ini` demonstrates strict engine-owned domains.

A master HUD recipe may simply include one of these files. Provider routing is therefore data-driven just like the HUD and reticle selection.
