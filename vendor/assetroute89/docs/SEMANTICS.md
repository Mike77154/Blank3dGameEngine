# AssetRoute89 semantics

Image auto-name mode follows the useful Ren'Py rule: basename only, extension removed, lowercased, subdirectory ignored for the logical name. Explicit registrations win over automatic ones.

Audio auto-aliases are lowercased basenames without extensions and must be identifier-shaped. Duplicate automatic aliases resolve deterministically to the alphabetically first path. Search roots are configurable; a typical engine uses `game/images` and `game/audio`.
