# Design Companion Workflow

Use this rule when work touches a user-facing frontend, marketing page, product
UI, dashboard, onboarding flow, empty state, form, visual design system, or
responsive layout.

`codex-agent-boilerplate` owns project runtime, planning, review, and upgrade
workflows. `impeccable` is an optional companion plugin for frontend design
quality. It must be installed or enabled separately from the Benchmarke Codex
Plugins marketplace.

When `impeccable` is installed, prefer it for:

- `$impeccable teach` to create or refresh `PRODUCT.md` design strategy.
- `$impeccable document` to capture `DESIGN.md` from an existing frontend.
- `$impeccable shape` before implementing substantial UI changes.
- `$impeccable critique` for UX, hierarchy, cognitive load, and information architecture review.
- `$impeccable audit` for accessibility, performance, responsive behavior, and technical UI quality.
- `$impeccable polish` before shipping user-facing UI.

If Impeccable is not installed, continue with the boilerplate workflow and note
the missing design companion only when it materially affects confidence. Do not
auto-install Impeccable, copy it into the project, or block non-UI work because
it is unavailable.

For UI-heavy sprints and campaigns, record the design quality evidence expected
before implementation starts: design context, screenshots or visual audit,
accessibility checks, responsive checks, and loading, empty, and error states.
