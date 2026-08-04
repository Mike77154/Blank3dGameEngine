# GAttach89 -> attached object -> NationalMecanicanimal89

## Purpose

This revision separates three jobs that must not be merged:

```text
carrier character
    -> GAttach89 places a child object on a named body socket
        -> NationalMecanicanimal89 animates that child's internal parts
            -> the child exposes output sockets to independent systems
```

The animator no longer owns the character attachment and it no longer draws a
muzzle flash. `GAttach89` decides **which object belongs to which character and
where its root transform lives**. `NationalMecanicanimal89` decides **how the
parts below that root move**.

## Vendored library

The original library is preserved under:

```text
vendor/gattach89/
```

The Blank3D adapter is:

```text
src/blank3d_attachment.h
src/blank3d_attachment.c
```

It uses fixed arrays only and converts between:

```text
Soquete3D pose Q16.16
        <->
GAttach89 transform Q20.12
```

No allocator is used.

## Generic attachment API

Any character/entity ID may expose named sockets and receive any child object
ID. The mechanical animator is not involved in attachment selection.

```c
Blank3DAttachmentWorld attachments;
soq3d_pose hand_world;
GAtt89_Xform grip_offset;

blank3d_attachment_init(&attachments);

blank3d_attachment_define_socket(
    &attachments,
    character_id,
    "weapon_r",
    B3D_ATTACH89_SOCKET_WEAPON_R,
    0);

blank3d_attachment_publish_socket_pose(
    &attachments,
    character_id,
    B3D_ATTACH89_SOCKET_WEAPON_R,
    &hand_world);

grip_offset = gatt89_xform_identity();
blank3d_attachment_attach_object(
    &attachments,
    character_id,
    rifle_object_id,
    "equipped_rifle",
    "weapon_r",
    &grip_offset);

blank3d_attachment_update(&attachments);
```

The same object can later be moved to another socket without rebuilding its
animation rig:

```c
blank3d_attachment_move_object(
    &attachments,
    character_id,
    rifle_object_id,
    "equipped_rifle",
    "back_weapon",
    &back_offset);
```

Or detached:

```c
blank3d_attachment_detach_object(
    &attachments,
    character_id,
    rifle_object_id,
    "equipped_rifle");
```

## Current engine path

The TPS player demonstrates the complete path:

```text
Soquete3D player.weapon
    -> GAttach89 player / weapon_r
        -> child object 10001 / equipped_weapon
            -> NationalMecanicanimal89 root world provider
                -> body
                -> slide
                -> magazine
                -> barrel
                -> muzzle_socket (no mesh)
```

`Blank3DMechanicalWeapon` receives the resolved `GAtt89_Xform` of child object
`10001`; it does not query the player socket directly anymore.

The bridge type is caller-owned and can be instantiated once per animated
object. The demo runner currently instantiates one rig for the player's visible
TPS weapon; the attachment world itself accepts arbitrary actors and objects.

## Muzzle separation

The old experimental `muzzle_flash` geometry binding was removed. The rig now
contains an invisible `muzzle_socket` part with no render binding.

After the mechanical rig resolves its matrices, Blank3D republishes that
socket through Soquete3D as `player.muzzle`. Existing projectile, muzzle,
audio, smoke, gas, light, or particle providers remain independent consumers.

```text
GWeapon89 FIRE_ACCEPTED
    |-> NationalMecanicanimal89: recoil and moving parts
    |-> existing muzzle systems: flash/gas/light/audio

NationalMecanicanimal89 muzzle_socket
    -> Soquete3D player.muzzle
        -> projectile and muzzle providers read the resolved point
```

The animator may emit mechanical markers such as `slide_rear`; it does not
render or own muzzle effects.

## Update order

```text
1. character/world transforms
2. Soquete3D body and hand sockets
3. GAttach89 resolves equipped child-object roots
4. NationalMecanicanimal89 resolves child parts
5. mechanical muzzle_socket is republished to Soquete3D
6. weapon/projectile/muzzle providers consume that socket
7. renderer draws only mechanical geometry packets
```

## Tests

```bash
make test-gattach-vendor
make test-attachment-stack
make test-nationalmecanicanimal-vendor
make test-mechanical-weapon
make syntax-check
make audit
```

`test_attachment_stack` verifies that an arbitrary object follows a named
character socket, can be moved, hidden, and detached without touching its rig.
`test_mechanical_weapon` verifies four geometry packets and confirms that the
muzzle is a socket rather than a rendered effect.
