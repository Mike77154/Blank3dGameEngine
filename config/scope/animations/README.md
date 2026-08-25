# gscopeanim89 recipes

Animation sets are ordinary recursive recipes. A set may include any number of
animation INIs. Sections are named `animation.<clip>` and `channel.<channel>`.

The bundled example supports `fire` plus an always-on breathing drift. The base
reticle data is never mutated: animation is sampled into a pose and applied to a
caller-owned temporary shape buffer immediately before emission.
