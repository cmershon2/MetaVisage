# Desktop Integration Q&A — Web Portal Responses

Responses to the desktop team's questions ahead of Sprint 6/7 implementation. Each answer cites the relevant file in the web portal repo so the desktop team can verify against source.

---

## Blocking

### 1. `LICENSE_JWT_SECRET` delivery + rotation

**Current state:** HS256 with a shared secret. `src/libs/jwt/license-jwt.ts:24` reads it from `process.env.LICENSE_JWT_SECRET`. `.env.local.example:18` lists it as a placeholder; no rotation guidance is documented.

**Contract:**
- **Per-environment secrets** (staging ≠ production). Both are configured in Vercel env vars; staging will be shared now, production at deploy time.
- **Out-of-band delivery:** 1Password shared vault. Encrypted file via Signal as a fallback. Not email or Slack DM.
- **Rotation cadence:** Plan for **annual rotation**, with ad-hoc rotation if either side suspects compromise.

**Important caveat — HS256 rotation is a coordinated cutover, not a graceful overlap.** The comment at `src/libs/jwt/license-jwt.ts:36-38` flags this: there is no `kid` / multi-key support yet, so rotation requires both sides to redeploy in lockstep. If we expect frequent rotation, migrating to RS256 (public key distributed to desktop, web keeps private) before launch is a small spike — flag if you want this.

### 2. Supabase access-token refresh strategy

**Honest answer:** the doc line "Use the Supabase client library" in `docs/DESKTOP_AUTH_FLOW.md` is stale — written before the desktop side dropped Qt's Supabase dependency. There is **no** `/api/desktop/refresh` endpoint today.

**Recommended contract: Option A (direct to Supabase), with Option B as a fast follow if preferred.**

Option A works today with no portal changes:

```
POST https://kkfybpsqqhnzvebzvqrc.supabase.co/auth/v1/token?grant_type=refresh_token
apikey: <NEXT_PUBLIC_SUPABASE_ANON_KEY>
Content-Type: application/json
{ "refresh_token": "<stored_refresh_token>" }
```

The anon key is `NEXT_PUBLIC_` (intentionally client-distributable). Project URL + anon key will be shared alongside the JWT secret.

If you'd rather not bake a Supabase URL into the desktop binary, the portal can add `POST /api/desktop/refresh` (mirrors Option B) — the portal proxies the same call. **Pending desktop team decision.**

Option C (cold re-auth on every access-token expiry) would be a regression from the current spec — not recommended.

### 3. Live staging endpoints

**Status:** Sprint 7 Story 4 (production deployment) is still open per `.agents/context/sprints/sprint-07.md`. The codebase alone doesn't tell us whether `staging.metavisage.io` is reachable — that's a Vercel/DNS question.

**Action item (web portal side):** verify `https://staging.metavisage.io/desktop-auth/init`, `/api/desktop/exchange`, `/api/desktop/token`, and `/api/license/validate` are live and reachable. Results will be posted in the integration thread.

If staging isn't deployed yet, the desktop team can develop against `localhost:3000` (a dev tunnel URL will be provided) until staging lands.

---

## Important

### 4. User email in the JWT — recommendation: **add it to the `/api/desktop/exchange` response**

The current JWT payload (`src/libs/jwt/license-jwt.ts:5-21`) has `sub` (UUID), `tier`, `license_key`, `feature_flags`, conversion fields — no email. The exchange row stores `access_token` / `refresh_token` / `license_jwt` / `state` (`src/app/api/desktop/exchange/route.ts:61-75`) — no email.

**Plan:** add `email` to the `/api/desktop/exchange` response body. Cheapest change — no JWT schema migration, no rotation impact.

**Trade-off:** if a user changes their email later, the cached value stales until next exchange. Acceptable for a "show signed-in email" UI label. If the desktop team needs it to stay live, the alternative is a `/api/desktop/whoami` endpoint — flag if that's preferred over an exchange-time field.

### 5. Test accounts on staging

Free / Indie / Pro staging accounts will be manually provisioned (no seed script exists in the repo). Credentials shared out-of-band. Same delivery window as the staging endpoint verification.

### 6. `license_key` in the exchange response

**It's already there — just inside the JWT, not as a separate field.** `license_key` is part of the signed payload (`src/app/api/desktop/token/route.ts:121-128`, `src/libs/jwt/license-jwt.ts:7`). The desktop app can decode `license_jwt` immediately after exchange and read `license_key` directly — no second `/api/license/validate` call needed for the License Status dialog.

If a top-level field on the exchange response would be cleaner (alongside the `email` field being added), it can be included there too. Either works.

---

## Nice-to-have

### 7. Portal-side state cookie timeout

- **Cookie max-age:** 600 seconds (10 minutes) — `src/app/(auth)/desktop-auth/init/route.ts:36`.
- **Cleanup behavior:** the cookie is deleted at the start of every callback invocation, success or `state_mismatch` — `src/app/(auth)/desktop-auth/callback/route.ts:36-39`.
- **Abandoned flows:** there is **no server-side abandonment cleanup**. If the user never reaches the callback, the cookie silently expires after 10 min via `maxAge`.

The desktop's 5-min timeout means the portal cookie will outlive the desktop's wait by ~5 minutes if abandoned. That's harmless — the cookie is httpOnly, single-use, and tied to a state value the desktop has already discarded.

### 8. Dashboard vs account URL

**`/dashboard` does not exist in the web portal.**

Canonical authenticated routes:
- `/account` — main account page (`src/app/(account)/account/page.tsx`)
- `/manage-subscription` — Route Handler that creates a Stripe billing portal session and redirects (`src/app/(account)/manage-subscription/route.ts:29-34`)

The current Help menu link to `https://metavisage.io/account` is correct. The Sprint 6 Story 6.6 reference to `/dashboard` is wrong — **`/manage-subscription` is the canonical "Manage Subscription" path.** Wiring the desktop's "Manage Subscription" button to `/manage-subscription` launches directly into the Stripe portal in a single hop.

### 9. `Retry-After` format

**Integer seconds** (delta), not HTTP-Date. All three desktop-relevant endpoints emit it identically:

- `src/app/api/desktop/exchange/route.ts:36` — `String(Math.ceil((resetAt - Date.now()) / 1000))`
- `src/app/api/desktop/token/route.ts:54` — same
- `src/app/api/license/validate/route.ts:45` — same

A simple `parseInt` is sufficient on the desktop side.

---

## Round 2 — Decisions & Implementation Status

The desktop team confirmed four decisions in the follow-up. All web-side code changes have shipped on `sprint/v2/QA-Hardening-&-Launch-Prep`.

### Decision 1 — Migrate to RS256 (✅ shipped)

HS256 → RS256 to remove the lockstep-rotation problem. Public key embedded in the desktop binary, private key stays on the portal.

**Web-side changes:**
- `src/libs/jwt/license-jwt.ts` — switched signer to RS256 with PKCS8 private key + SPKI public key (PEM, normalises `\n` literals).
- `src/libs/jwt/license-jwt.test.ts` and `src/app/api/license/validate/route.test.ts` — generate ephemeral RSA keypairs at suite startup.
- `.env.local.example` — `LICENSE_JWT_SECRET` replaced by `LICENSE_JWT_PRIVATE_KEY` + `LICENSE_JWT_PUBLIC_KEY`, with the openssl one-liner inline.
- `docs/DESKTOP_APP_INTEGRATION.md` — JWT header now documents `alg: RS256`; desktop env-var table now lists `LICENSE_JWT_PUBLIC_KEY` (not the secret).

**Desktop-side action:** verify with the public key only — `jwt-cpp` handles RS256 natively. Casey will share the public key alongside the Supabase project URL + anon key in the 1Password drop.

### Decision 2 — Option A for access-token refresh (✅ no code change needed)

Desktop calls `https://{SUPABASE_PROJECT_URL}/auth/v1/token?grant_type=refresh_token` directly with the stored refresh token and the anon key as the `apikey` header. The portal does not proxy.

**Web-side changes:**
- `docs/DESKTOP_AUTH_FLOW.md` — Token Refresh section rewritten to spell out Option A's exact request shape and 401-fallback behaviour. The stale "Use the Supabase client library" line is gone.
- `docs/DESKTOP_APP_INTEGRATION.md` — 401 error-handling note now points at the Supabase endpoint explicitly.

### Decision 3 — Email via the exchange response (✅ shipped)

The exchange endpoint now returns the user's email so the License Status dialog has it without a second round trip. Snapshot at exchange time; refreshes on next sign-in.

**Web-side changes:**
- `src/app/api/desktop/exchange/route.ts` — selects `user_id` from the `desktop_auth_codes` row, calls `auth.admin.getUserById(user_id)`, returns `email` in the response body.
- `docs/DESKTOP_AUTH_FLOW.md` — exchange response example updated.

### Decision 4 — `license_key` stays inside the JWT only (✅ no code change needed)

The desktop app decodes `license_jwt` once after exchange and caches `license_key` from the payload. No top-level field added.

---

## Operational Open Items

| # | Item | Owner | Notes |
|---|------|-------|-------|
| 1 | Generate production + staging RSA keypairs | Web (Casey) | `openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out private.pem && openssl rsa -in private.pem -pubout -out public.pem` — set both halves in Vercel env vars |
| 2 | Verify staging endpoints reachable | Web | `/desktop-auth/init`, `/api/desktop/exchange`, `/api/desktop/token`, `/api/license/validate` |
| 3 | Provision Free/Indie/Pro test accounts on staging | Web | Credentials out-of-band |
| 4 | Deliver `LICENSE_JWT_PUBLIC_KEY`, `SUPABASE_PROJECT_URL`, `NEXT_PUBLIC_SUPABASE_ANON_KEY`, and test account credentials | Web (Casey) | 1Password shared vault — public key only, **not** the private key |
