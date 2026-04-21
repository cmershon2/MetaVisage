# Desktop App Integration Guide

## MetaVisage License Validation API

This document describes how the MetaVisage desktop application should integrate with the web portal's license validation and conversion tracking APIs.

---

## Base URL

| Environment | URL                             |
| ----------- | ------------------------------- |
| Production  | `https://metavisage.io`         |
| Staging     | `https://staging.metavisage.io` |
| Local Dev   | `http://localhost:3000`         |

---

## Authentication

All API requests are authenticated via a **Bearer token** using the user's **Supabase access token** obtained during the desktop auth handoff.

The access token is issued by the web portal at the end of the `metavisage://` deep-link auth flow (see `docs/DESKTOP_AUTH_FLOW.md`). The desktop app stores the access token (and refresh token) locally and refreshes it via the standard Supabase Auth refresh endpoint when it expires.

```
Authorization: Bearer <supabase_access_token>
```

The `license_key` returned by `/api/license/validate` is informational (shown in the user's account dashboard) — it is **not** used as a credential.

---

## Endpoints

### 1. Validate License — `POST /api/license/validate`

Called on app launch and periodically (recommended: every 24 hours) to obtain a fresh signed license JWT.

#### Request

```http
POST /api/license/validate
Authorization: Bearer <supabase_access_token>
Content-Type: application/json
```

No request body is required.

#### Success Response (200)

```json
{
  "token": "eyJhbGciOiJIUzI1NiIs...",
  "tier": "indie",
  "license_key": "a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4",
  "conversions_used": 5,
  "conversions_limit": 20,
  "conversions_reset_at": "2026-04-15T00:00:00.000Z"
}
```

| Field                  | Type   | Description                                           |
| ---------------------- | ------ | ----------------------------------------------------- |
| `token`                | string | Signed JWT — cache this locally for offline use       |
| `tier`                 | string | `free`, `indie`, or `pro`                             |
| `license_key`          | string | The user's license key                                |
| `conversions_used`     | number | Conversions used in the current billing period        |
| `conversions_limit`    | number | Maximum conversions allowed in the billing period     |
| `conversions_reset_at` | string | ISO 8601 timestamp when the conversion counter resets |

#### Error Responses

| Status | Error Code       | Meaning                                               |
| ------ | ---------------- | ----------------------------------------------------- |
| 401    | `unauthorized`   | Invalid, expired, or missing access token             |
| 403    | `inactive`       | Subscription cancelled or unpaid                      |
| 403    | `expired`        | Payment is past due — user must update payment method |
| 404    | `not_found`      | No active license or subscription found               |
| 429    | `rate_limited`   | Too many requests — respect the `Retry-After` header  |
| 500    | `internal_error` | Server error — retry with exponential backoff         |

Error response body format:

```json
{
  "error": "inactive",
  "message": "Your subscription has been cancelled. Please resubscribe to continue."
}
```

---

### 2. Report Conversion — `POST /api/license/conversion`

Called by the desktop app **after each successful conversion** to increment the user's monthly conversion counter.

#### Request

```http
POST /api/license/conversion
Authorization: Bearer <supabase_access_token>
Content-Type: application/json
```

No request body is required.

#### Success Response (200)

```json
{
  "conversions_used": 6,
  "conversions_limit": 20,
  "conversions_reset_at": "2026-04-15T00:00:00.000Z",
  "limit_reached": false
}
```

| Field                  | Type    | Description                                    |
| ---------------------- | ------- | ---------------------------------------------- |
| `conversions_used`     | number  | Updated count after this conversion            |
| `conversions_limit`    | number  | Maximum conversions for the billing period     |
| `conversions_reset_at` | string  | ISO 8601 timestamp of next counter reset       |
| `limit_reached`        | boolean | `true` if the user has hit their monthly limit |

#### Error Responses

| Status | Error Code       | Meaning                                              |
| ------ | ---------------- | ---------------------------------------------------- |
| 401    | `unauthorized`   | Invalid, expired, or missing access token            |
| 403    | `limit_reached`  | Monthly conversion limit has been reached            |
| 404    | `not_found`      | No active license found                              |
| 429    | `rate_limited`   | Too many requests — respect the `Retry-After` header |
| 500    | `internal_error` | Server error — retry with exponential backoff        |

When `limit_reached` is returned, the response includes the current usage:

```json
{
  "error": "limit_reached",
  "message": "You have reached your monthly conversion limit of 20.",
  "conversions_used": 20,
  "conversions_limit": 20,
  "conversions_reset_at": "2026-04-15T00:00:00.000Z"
}
```

---

## License JWT

The `token` returned by the validation endpoint is a signed JWT that the desktop app should cache locally for offline use.

### JWT Header

```json
{
  "alg": "HS256",
  "typ": "JWT"
}
```

### JWT Payload

```json
{
  "sub": "550e8400-e29b-41d4-a716-446655440000",
  "tier": "indie",
  "license_key": "a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4",
  "feature_flags": {
    "conversions_limit": 20,
    "custom_head_upload": true,
    "texture_transfer": "albedo",
    "morphing_parameters": "full",
    "masking": true,
    "touchup_tools": "basic",
    "batch_processing": false
  },
  "conversions_used": 5,
  "conversions_limit": 20,
  "conversions_reset_at": "2026-04-15T00:00:00.000Z",
  "iss": "metavisage-web",
  "aud": "metavisage-desktop",
  "iat": 1711929600,
  "exp": 1712534400
}
```

### Standard Claims

| Claim | Value                | Description                       |
| ----- | -------------------- | --------------------------------- |
| `iss` | `metavisage-web`     | Issuer — always the web portal    |
| `aud` | `metavisage-desktop` | Audience — always the desktop app |
| `iat` | Unix timestamp       | Issued at                         |
| `exp` | Unix timestamp       | Expires 7 days from issuance      |
| `sub` | UUID                 | The user's Supabase auth user ID  |

### Feature Flags

| Flag                  | Type    | Values                                                       |
| --------------------- | ------- | ------------------------------------------------------------ |
| `conversions_limit`   | number  | `3` (free), `20` (indie), `60` (pro)                         |
| `custom_head_upload`  | boolean | `false` (free), `true` (indie/pro)                           |
| `texture_transfer`    | string  | `"none"` (free), `"albedo"` (indie), `"albedo_normal"` (pro) |
| `morphing_parameters` | string  | `"presets"` (free), `"full"` (indie/pro)                     |
| `masking`             | boolean | `false` (free), `true` (indie/pro)                           |
| `touchup_tools`       | string  | `"basic_smooth"` (free), `"basic"` (indie), `"full"` (pro)   |
| `batch_processing`    | boolean | `false` (all tiers — reserved for Studio)                    |

---

## Offline / Cached License (Grace Period)

The desktop app must implement the following caching strategy:

1. **On successful validation**: Store the JWT locally, encrypted using a key derived from the machine ID and user account identifier.
2. **On app launch**: Attempt to validate online. If successful, update the cached JWT.
3. **If offline**: Fall back to the cached JWT. Grant full access if the JWT has not expired (within the 7-day window).
4. **If cached JWT is expired and offline**: Deny access and prompt the user to connect to the internet.
5. **On validation error** (`inactive`, `expired`): Clear the cached JWT and downgrade/block access accordingly.

### Cache Encryption

The cached JWT file should be encrypted using a key derived from:

- The machine's unique hardware identifier
- The user's license key

This prevents simple file copying between machines.

---

## Rate Limits

| Endpoint                       | Limit              |
| ------------------------------ | ------------------ |
| `POST /api/license/validate`   | 30 requests/minute |
| `POST /api/license/conversion` | 10 requests/minute |

When rate limited, the response includes:

- HTTP status `429`
- `Retry-After` header (seconds until the rate limit resets)

The desktop app should respect the `Retry-After` header and implement exponential backoff.

---

## Recommended Integration Flow

```
┌─────────────────────────────────────────────────┐
│                  APP LAUNCH                     │
└──────────────────────┬──────────────────────────┘
                       │
                       ▼
              ┌─────────────────┐
              │  Has cached JWT? │
              └────────┬────────┘
                 yes   │   no
          ┌────────────┴────────────┐
          ▼                         ▼
   ┌──────────────┐         ┌──────────────┐
   │ JWT expired?  │         │  Call /validate│
   └──────┬───────┘         └──────┬───────┘
    no    │   yes                   │
    │     ▼                         ▼
    │  ┌──────────────┐     ┌──────────────┐
    │  │ Call /validate│     │  Success?     │
    │  └──────┬───────┘     └──────┬───────┘
    │         │                yes │  no
    │         ▼                    │   │
    │  ┌──────────────┐           │   ▼
    │  │  Success?     │          │  Show error,
    │  └──────┬───────┘          │  prompt login
    │   yes   │   no              │
    │   │     ▼                   │
    │   │  ┌────────────────┐    │
    │   │  │ Use cached JWT  │    │
    │   │  │ (grace period)  │    │
    │   │  └────────┬───────┘    │
    │   │           │             │
    │   ▼           ▼             ▼
    └───────────────┬─────────────┘
                    │
                    ▼
           ┌────────────────┐
           │ Cache new JWT   │
           │ Start app with  │
           │ feature flags   │
           └────────────────┘
                    │
                    ▼
          ┌──────────────────┐
          │  On conversion   │
          │  Call /conversion │
          │  Update local UI │
          └──────────────────┘
```

---

## Tier Feature Matrix (Quick Reference)

| Feature               | Free                | Indie ($19/mo) | Pro ($49/mo)    |
| --------------------- | ------------------- | -------------- | --------------- |
| Conversions/mo        | 3                   | 20             | 60              |
| Custom MH head upload | No                  | Yes            | Yes             |
| Texture transfer      | None                | Albedo only    | Albedo + Normal |
| Morphing parameters   | Presets only        | Full control   | Full control    |
| Masking               | No                  | Yes            | Yes             |
| Touch-up tools        | Basic (smooth only) | Basic          | Full            |
| Batch processing      | No                  | No             | No              |

---

## Error Handling Recommendations

1. **Network errors**: Retry with exponential backoff (1s, 2s, 4s, max 30s). Fall back to cached JWT if available.
2. **401 Unauthorized**: The access token is invalid or expired. Refresh the token via the Supabase refresh endpoint, or re-trigger the `metavisage://` auth flow if the refresh token has also expired.
3. **403 Inactive/Expired**: Show a clear message directing the user to the web portal to manage their subscription.
4. **403 Limit Reached**: Show remaining time until `conversions_reset_at` and suggest upgrading.
5. **429 Rate Limited**: Wait for the duration specified in `Retry-After` before retrying.
6. **500 Server Error**: Retry with backoff. If persistent, fall back to cached JWT.

---

## Environment Variables (Desktop App)

The desktop app needs to be configured with:

| Variable                  | Description                                  |
| ------------------------- | -------------------------------------------- |
| `METAVISAGE_API_BASE_URL` | Base URL of the web portal API               |
| `LICENSE_JWT_SECRET`      | Shared secret for JWT signature verification |

The `LICENSE_JWT_SECRET` must match the value configured on the web portal. This allows the desktop app to independently verify JWT signatures without a network call.
