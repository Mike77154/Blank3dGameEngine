# gplayerfireray89

Player-only visible-camera fire-ray policy. It is intentionally separate from
NPC/AI aim. The host supplies a raycast callback and layer mask; the module owns
only camera-ray -> muzzle-validation semantics and fixed-point spread.
