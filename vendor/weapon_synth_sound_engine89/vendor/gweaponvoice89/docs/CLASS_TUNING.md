# Class tuning

Use `gwv89_set_class` to replace any default class policy.

Important fields:

- `max_logical`: maximum concurrent events in the class.
- `physical_reserve`: minimum physical opportunities reserved for audible events in the class.
- `priority`: coarse importance; distance penalty and event bias modify it.
- `default_instance_limit`: concurrency per nonzero `instance_key`.
- `steal_protect_ms`: prevents a fresh transient from being immediately replaced.
- `minimum_physical_ms`: reduces physical/virtual flutter after promotion.
- `virtual_timeout_ms`: kills virtual leaks.
- `virtual_behavior`: continue, advance, pause, restart or kill.

Suggested instance keys:

- weapon entity ID for reports and mechanisms;
- material/impact cell ID for impacts;
- emitter or room ID for ambience;
- batch ID for debris and casings.
