# Raid Encounter Tactics

This subtree contains encounter-specific overrides layered above the existing Normal Rotation AI.

Priority contract:

```text
Encounter HARD override
  -> Encounter SOFT override / movement / target assignment
  -> existing Normal Rotation AI
```

Layout:

- `common/`: encounter routing, actor resolution and shared contracts only.
- `<instance>/`: one router per raid instance.
- `<instance>/<boss>/`: mechanics, pull coordination, formation and boss-local data.

Do not place ordinary class rotation logic here. Do not server-move or server-cast protected human actors.
