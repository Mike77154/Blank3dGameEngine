# actor_system89 0.2

Agnostic actor registry for C89/no-heap hosts.

An Actor is a gameplay participant identity associated with an **opaque owner Entity key**. The ActorSystem does not know player/enemy/ally kinds, factions, teams, inventory, equipment, weapons, AI, health models or concrete entity storage. Those systems associate their own state by `actor_id` or use providers.
