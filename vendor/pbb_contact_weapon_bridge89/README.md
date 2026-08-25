# pbb_contact_weapon_bridge89

Vendorizable C89 bridge between `3D_contact_trigger89`, `pbb_item_system_c89`
and a host weapon/inventory implementation.

The bridge deliberately does not include Blank3D headers. Blank3D provides a
small weapon provider from `blank3d_systems.c`.

Flow:

```text
3D sensor provider
  -> CT89 ENTER / ACTIVATE
  -> pbb_contact_weapon_bridge89
  -> PBB rules
  -> optional action gate
       -> host grant_weapon / grant_ammo
       -> reject means item remains in world
  -> PBB effects / consume
  -> CT89 accepted consume policy
       -> disable trigger or host lifecycle destroy
```

The PBB action gate is optional and backwards compatible: with no gate installed
PBB v0.2 behavior is unchanged.
