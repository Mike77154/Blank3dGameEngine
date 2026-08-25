# gfireframe89

Same-frame camera/weapon synchronization. Camera capture and recoil application
are host providers; the module owns only immutable-frame state and delayed
recoil bookkeeping. This prevents a projectile, HUD and recoil from observing
different camera frames.
