# Desktop Authentication Flow

## Overview

This document describes the browser-based OAuth-style authentication flow that allows the MetaVisage desktop application to authenticate users through the web portal and receive a session token and license JWT via a secure exchange code pattern using the `metavisage://` custom URL scheme.

## Prerequisites

- The desktop application must register the `metavisage://` custom URL scheme on Windows. This is typically done via a Windows Registry entry during installation.
- The web portal handles its side of the flow entirely; the desktop team is responsible for registering the scheme in the Windows application.

---

## Authentication Flow

### Sequence

```
Desktop App                    Browser / Web Portal                  Supabase Auth
    |                                  |                                  |
    |  1. Generate random `state`      |                                  |
    |  2. Open system browser -------->|                                  |
    |     /desktop-auth/init?state=... |                                  |
    |                                  |  3. /init sets state in httpOnly  |
    |                                  |     cookie, redirects to /page   |
    |                                  |  4. Show sign in / sign up UI    |
    |                                  |                                  |
    |                                  |  5. User authenticates --------->|
    |                                  |     (state threaded in redirect) |
    |                                  |                                  |
    |                                  |<-- 6. Auth callback with code ---|
    |                                  |     + state in URL               |
    |                                  |                                  |
    |                                  |  7. Exchange code for session    |
    |                                  |  8. Compare state cookie against |
    |                                  |     URL state parameter          |
    |                                  |  9. Redirect to /success?state=  |
    |                                  |     (NO tokens in URL)           |
    |                                  |                                  |
    |                                  |  10. Success page reads session  |
    |                                  |      from cookies, signs JWT,    |
    |                                  |      creates exchange code       |
    |                                  |                                  |
    |<-- 11. Client-side redirect to  |                                  |
    |    metavisage://auth?            |                                  |
    |      code={exchange_code}&       |                                  |
    |      state={state}               |                                  |
    |                                  |                                  |
    |  12. Validate state matches      |                                  |
    |  13. Exchange code for tokens -->|                                  |
    |      POST /api/desktop/exchange  |                                  |
    |<-- 14. Receive tokens -----------|                                  |
    |  15. Store tokens locally        |                                  |
    |  16. User is authenticated       |                                  |
```

### Step-by-Step

1. **Desktop app generates a random `state` parameter** — a cryptographically random string (minimum 32 characters). This is stored locally in the desktop app for later validation.

2. **Desktop app opens the system browser** to:
   ```
   https://metavisage.io/desktop-auth/init?state={random_state}
   ```
   > **Important:** The entry point is `/desktop-auth/init`, not `/desktop-auth`. The `/init` route is a lightweight handler that sets the CSRF cookie and then redirects to the sign-in page.

3. **Web portal stores the `state` parameter** in an httpOnly cookie (via the `/init` route handler) and redirects to `/desktop-auth?state={state}`.

4. **Web portal presents the sign in / sign up UI** optimized for the desktop handoff context, with clear messaging: "You are signing in to the MetaVisage desktop app."

5. **User authenticates** via Google OAuth or email magic link (the same auth methods as the regular portal). The `state` parameter is threaded through the Supabase `redirectTo` URL so it survives the OAuth/OTP round trip.

6. **Supabase Auth completes the OAuth/OTP flow** and redirects back to the portal's desktop auth callback with both the auth `code` and the `state` parameter in the URL.

7. **Portal exchanges the auth code for a Supabase session** — session cookies are set on the browser.

8. **Portal compares the `state` cookie value against the URL `state` parameter.** Both must exist and their values must match exactly. If they don't match, the flow is aborted and the error page is shown with `reason=state_mismatch`. The cookie is deleted after validation (single-use).

9. **Portal redirects to `/desktop-auth/success?state={state}`** — only the non-sensitive state parameter is in the URL. No tokens are passed via URL.

10. **The success page creates a short-lived exchange code.** It reads the authenticated session from Supabase cookies, fetches the user's active license, signs a fresh license JWT, and stores all three (access token, refresh token, license JWT) in a `desktop_auth_codes` database row with a 2-minute expiry. The exchange code is a 64-character hex string.

11. **The client component triggers a redirect** to:
    ```
    metavisage://auth?code={exchange_code}&state={state}
    ```
    If the automatic redirect fails, a fallback "Open MetaVisage" button is shown.

12. **Desktop app intercepts the custom URL scheme**, extracts the `code` and `state` parameters.

13. **Desktop app validates the `state` parameter** matches the one it generated in step 1. If they don't match, the authentication is rejected.

14. **Desktop app exchanges the code for tokens** by calling `POST /api/desktop/exchange` (see below).

15. **Desktop app stores the session tokens and license JWT** for use in API calls and offline license caching.

---

## Code Exchange

After intercepting the `metavisage://` deep link, the desktop app must exchange the code for the actual tokens.

### `POST /api/desktop/exchange`

**Request body:**
```json
{
  "code": "{exchange_code_from_deep_link}"
}
```

**Response (200):**
```json
{
  "access_token": "{supabase_access_token}",
  "refresh_token": "{supabase_refresh_token}",
  "license_jwt": "{signed_license_jwt}",
  "state": "{state}",
  "email": "user@example.com"
}
```

The `email` field is the email address of the signed-in Supabase user — provided so the desktop client can label the License Status dialog without an additional round trip. It is a snapshot at exchange time; if the user later changes their email it will not refresh until the next sign-in.

**Error Responses:**
- `400 Bad Request` — Exchange code is invalid, expired (>2 minutes), or already used. The desktop app should re-initiate the full auth flow.
- `429 Too Many Requests` — Rate limit exceeded (10 requests per minute per IP). Check the `Retry-After` header.

**Important notes:**
- Exchange codes are **single-use**. Once exchanged, the same code cannot be used again.
- Exchange codes **expire after 2 minutes**. If the user takes too long, the code will be invalid.
- No authentication header is needed for this endpoint — the code itself is the credential.

---

## Token Refresh

The desktop app holds two distinct tokens after the exchange:

1. A **Supabase access token** (short-lived, used as the `Authorization: Bearer …` credential on every API call).
2. A **license JWT** (7-day expiry, cached for offline grace).

Each refreshes via a different endpoint.

### Refreshing the Supabase access token

When the access token expires (the desktop app gets a `401` from any portal endpoint), refresh it directly against Supabase Auth using the `refresh_token` returned by `/api/desktop/exchange`:

```
POST https://{SUPABASE_PROJECT_REF}.supabase.co/auth/v1/token?grant_type=refresh_token
apikey: {NEXT_PUBLIC_SUPABASE_ANON_KEY}
Content-Type: application/json

{ "refresh_token": "{stored_refresh_token}" }
```

A successful response returns a new `access_token` and `refresh_token`. Replace the cached pair with the new values. If the refresh call itself returns `401`/`400`, the refresh token is invalid (revoked or expired) — fall back to the full `metavisage://` auth flow.

The Supabase project URL and the anon key are public values intended for client-side distribution and may be embedded in the desktop binary.

### Refreshing the license JWT

After the access token is valid, fetch a fresh license JWT via:

### `POST /api/desktop/token`

**Headers:**
```
Authorization: Bearer {supabase_access_token}
Content-Type: application/json
```

**Response (200):**
```json
{
  "token": "{fresh_license_jwt}",
  "tier": "indie",
  "license_key": "MVL-XXXX-XXXX-XXXX",
  "conversions_used": 5,
  "conversions_limit": 20,
  "conversions_reset_at": "2026-05-01T00:00:00Z"
}
```

**Error Responses:**
- `401 Unauthorized` — Invalid or expired session token. The desktop app should re-initiate the full auth flow.
- `403 Forbidden` — Subscription is inactive, cancelled, or past due.
- `404 Not Found` — No active license found.
- `429 Too Many Requests` — Rate limit exceeded (20 requests per minute). Check the `Retry-After` header.

---

## License Validation

For launch-time license validation, the desktop app can also use:

### `POST /api/license/validate`

**Headers:**
```
Authorization: Bearer {supabase_access_token}
Content-Type: application/json
```

This endpoint works identically to `/api/desktop/token` — it authenticates via a Supabase access token (not a license key) and returns a fresh signed license JWT. Rate-limited to 30 requests per minute per user.

---

## Custom URL Scheme

### Registration

The desktop application must register the `metavisage://` URL scheme on Windows. This is typically done by adding a registry entry during installation:

```
HKEY_CLASSES_ROOT
  metavisage
    (Default) = "URL:MetaVisage Protocol"
    URL Protocol = ""
    shell
      open
        command
          (Default) = "C:\Path\To\MetaVisage.exe" "%1"
```

### Redirect URI Format

```
metavisage://auth?code={exchange_code}&state={state}
```

| Parameter | Description |
|-----------|-------------|
| `code`    | Short-lived, single-use exchange code (64-char hex). Exchange via `POST /api/desktop/exchange` for actual tokens. Expires after 2 minutes. |
| `state`   | The same random string the desktop app sent in the initial request. Must be validated before exchanging the code. |

> **Note:** Raw session tokens and license JWTs are never exposed in the deep link URL. Only the exchange code is transmitted via the URL scheme.

---

## Error Handling

| Scenario | Web Portal Behavior | Desktop App Behavior |
|----------|---------------------|---------------------|
| User cancels authentication | Shows error page with retry option | No redirect received; user can retry |
| Invalid/missing state parameter | Shows error page; does not redirect | No redirect received |
| State cookie does not match URL param | Shows error page with `state_mismatch` reason | No redirect received |
| No active license found | Shows error page with link to pricing | No redirect received |
| Authentication succeeds but redirect fails | Shows success page with manual "Open MetaVisage" button | User clicks the fallback button |
| Exchange code expired or already used | N/A (server returns 400) | Re-initiate the full auth flow |

---

## Security Considerations

- **Exchange code pattern:** Raw session tokens and license JWTs are never exposed in the `metavisage://` deep link URL. The deep link contains only a short-lived (2 minute), single-use exchange code. The desktop app exchanges this code for the actual tokens via a server-side API call. This prevents token interception by other applications registered for the `metavisage://` scheme.
- **CSRF state comparison:** The `state` parameter prevents CSRF attacks. It is stored in an httpOnly cookie during `/init` and compared against the URL `state` parameter in the callback. Both must exist and match exactly — a cookie-existence-only check is not sufficient.
- **No tokens in URLs:** Tokens are never passed as URL query parameters between portal pages. The callback redirects to `/success?state=...` (non-sensitive), and the success page generates the exchange code server-side. The `metavisage://` deep link contains only the exchange code and state — no raw tokens.
- **Origin validation:** All desktop API endpoints reject cross-origin browser requests. Native desktop HTTP clients (which don't send `Origin` headers) are allowed.
- **Security headers:** The portal sets `X-Frame-Options: DENY`, `X-Content-Type-Options: nosniff`, `Strict-Transport-Security`, `Referrer-Policy`, and `Permissions-Policy` on all responses.
- **Bearer token authentication:** All API endpoints authenticate via Supabase access tokens, not license keys. Access tokens are short-lived and validated against Supabase Auth on every request.
- **Short-lived sessions:** Supabase access tokens are short-lived JWTs. The desktop app refreshes them via Supabase's `/auth/v1/token` endpoint (see [Token Refresh](#token-refresh)) and refreshes its license JWT via `/api/desktop/token`.
- **Offline grace period:** The license JWT has a 7-day expiry window for offline access.
- **Scheme registration:** The `metavisage://` scheme should only be registered by the official MetaVisage installer.
- **Rate limiting:** All endpoints are rate-limited — `/api/desktop/exchange` at 10 req/min per IP, `/api/desktop/token` at 20 req/min per user, `/api/license/validate` at 30 req/min per user.
