# Code Style

## Universal

- Prefer clarity over cleverness — write for the next developer
- Functions and methods do one thing; extract when logic branches significantly
- Name variables and functions after what they represent, not how they work
- Delete dead code rather than commenting it out
- No magic numbers or strings — use named constants
- Keep files focused; if a file grows unwieldy, split by responsibility

## Error Handling

- Only handle errors that can realistically occur — no defensive coding for impossible states
- Validate at system boundaries (user input, external APIs, file I/O)
- Propagate errors with enough context to diagnose without leaking internals
- Never silently swallow errors

## Comments

- Write comments for *why*, not *what* — the code shows what
- Don't document the obvious; document surprising behavior or important constraints
- Keep comments updated with code — stale comments are worse than none

## Functions and Methods

- Prefer pure functions where practical
- Avoid side effects in functions that return values
- Default to short, focused functions (~20-40 lines); use judgment
- Boolean parameters are a code smell — prefer separate functions or enums

## Dependencies

- Prefer standard library over third-party where functionality is equivalent
- Pin dependency versions in production; use ranges in library manifests
- Review what you import — prefer well-maintained, minimal packages
