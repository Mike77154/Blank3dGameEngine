# AssetRoute89

Provider-driven, no-heap C89 asset discovery and routing service inspired by Ren'Py's image and audio lookup behavior.

The core never touches the OS. Filesystem existence/enumeration are providers. POSIX and Win32 reference providers are optional.
