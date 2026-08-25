# 3D_contact_trigger89

`3D_contact_trigger89` is a vendorizable, no-heap C89 contact/proximity trigger core.
It does not know what a player, pickup, inventory, health box, weapon, door,
physics engine, UI prompt, Actor, ECS entity or Thing is.

Its job is deliberately smaller:

```text
sensor provider -> contact/proximity relation -> event -> action provider
                                                -> accepted?
                                                   -> consume policy
                                                      -> lifecycle provider
```

## Design goals

- strict C89 core
- fixed Q16.16 (`CT89_FX`), no `float`/`double`
- no `malloc`, `realloc`, `free` or hidden heap
- caller-owned context and fixed-capacity tables
- generational trigger handles
- contact sensor provider
- proximity sensor provider
- optional filter provider
- event provider for UI/gameplay observation
- action provider with ACCEPTED / REJECTED / DEFERRED / UNHANDLED result
- lifecycle provider instead of direct entity deletion
- ENTER / STAY / EXIT / ACTIVATE semantics
- consume policies: keep, disable trigger, destroy owner, destroy other, destroy both
- cooldown and maximum accepted activations

## Typical pickup-like behaviors

### Touch and disappear after a successful action

Configure `CT89_SENSOR_TOUCH`, action on `CT89_EVENT_ENTER`, and
`CT89_CONSUME_DESTROY_OWNER`. The action provider decides what "do something"
means. If it rejects the action, the object remains.

### Approach, show a prompt, press Use, then disappear

Configure `CT89_SENSOR_PROXIMITY`, notify ENTER/STAY/EXIT, put the action only on
`CT89_EVENT_ACTIVATE`, and use `CT89_CONSUME_DESTROY_OWNER`. ENTER can make the
host show "Store item?". The host calls `ct89_activate()` when the player confirms.
If the inventory provider returns REJECTED, the object is not consumed.

## Standalone build

```sh
make
make test
make audit
```

The Blank3D package adds adapters separately; the vendor core has no Blank3D,
ThingSystem89 or VPhysics dependency.
