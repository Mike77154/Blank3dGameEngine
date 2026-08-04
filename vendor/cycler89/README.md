# cycler89

Tiny C89 list walker for moving forward or backward through any provider-backed
list while skipping inactive entries.

- no allocation;
- no knowledge of weapons, menus, entities or engines;
- provider-backed count/read/active callbacks;
- deterministic wrap-around;
- deterministic recovery when the current item disappeared;
- stateful cursor helpers and stateless next/previous helpers.

The host owns all storage. `cycler89_item` is a `long`, so an engine may expose
IDs, handles, enum values or stable array keys without leaking its structures.
