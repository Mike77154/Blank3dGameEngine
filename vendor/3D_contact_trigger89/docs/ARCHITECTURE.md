# Architecture

`3D_contact_trigger89` treats a touchable object as composition rather than a
special pickup class.

```text
                    +----------------------+
collision backend ->| contact provider     |--+
                    +----------------------+  |
                                               v
                    +----------------------+  relation table
world/spatial ----->| proximity provider   |--+ (enter/stay/exit)
                    +----------------------+  |
                                               v
                                         +-----------+
                                         | event bus |
                                         +-----------+
                                               |
                         +---------------------+------------------+
                         |                                        |
                         v                                        v
                  prompt / feedback                       action provider
                                                                  |
                                                       accepted/rejected/deferred
                                                                  |
                                                       +----------+----------+
                                                       |                     |
                                                     keep              consume policy
                                                                           |
                                                                   lifecycle provider
```

The core never directly destroys an ECS entity or a Blank3D Thing. `DESTROY_*`
means "request destruction through the installed lifecycle provider".
