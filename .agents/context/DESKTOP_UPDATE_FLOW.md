# Desktop Update Flow (WinSparkle)

## Overview

MetaVisage desktop ships updates via [WinSparkle](https://github.com/vslavik/winsparkle). The desktop client polls a public **appcast** URL hosted by this web portal. When a new active release exists, WinSparkle handles the prompt, download, signature check (optional), and relaunch — no manual intervention is required from the user.

This document is the integration contract for the desktop team.

---

## Appcast Endpoint

| Property | Value |
|---|---|
| **URL** | `https://metavisage.io/api/appcast.xml` |
| **Method** | `GET` |
| **Auth** | None (public) |
| **Content-Type** | `application/rss+xml; charset=utf-8` |
| **Cache** | `no-store` — every poll hits the live database |
| **Status when no release published** | `404 No active release` |

WinSparkle should be configured to point at the production URL above. A staging equivalent (e.g. `https://staging.metavisage.io/api/appcast.xml`) can be used during development.

---

## Appcast XML Schema

The endpoint returns a WinSparkle-compatible RSS 2.0 document with the `sparkle` namespace.

```xml
<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0"
     xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle"
     xmlns:dc="http://purl.org/dc/elements/1.1/">
  <channel>
    <title>MetaVisage</title>
    <link>https://metavisage.io</link>
    <description>MetaVisage desktop application updates</description>
    <language>en</language>
    <item>
      <title>Version 1.2.3</title>
      <pubDate>Sat, 18 Apr 2026 10:00:00 GMT</pubDate>
      <sparkle:version>1.2.3</sparkle:version>
      <sparkle:shortVersionString>1.2.3</sparkle:shortVersionString>
      <description><![CDATA[- Bug fixes
- New feature X]]></description>
      <enclosure
        url="https://<supabase-project>.supabase.co/storage/v1/object/public/installers/1.2.3/installer.exe"
        sparkle:version="1.2.3"
        sparkle:shortVersionString="1.2.3"
        length="58432000"
        type="application/octet-stream" />
    </item>
  </channel>
</rss>
```

### Field reference

| Field | Source | Notes |
|---|---|---|
| `item/title` | Active release version | Always `Version <semver>` |
| `item/pubDate` | `releases.published_at` | RFC 822 / GMT |
| `item/sparkle:version` | `releases.version` | Strict semver: `MAJOR.MINOR.PATCH[-prerelease][+build]` |
| `item/sparkle:shortVersionString` | `releases.version` | Same as above — WinSparkle requires both |
| `item/description` | `releases.release_notes` | Markdown, wrapped in `<![CDATA[…]]>`. Falls back to `Version <semver>` if empty. |
| `enclosure[url]` | `releases.installer_url` | Public Supabase Storage URL |
| `enclosure[length]` | `releases.installer_size_bytes` | Bytes; `0` if unknown |
| `enclosure[type]` | constant | Always `application/octet-stream` |

Only **one** release is exposed at a time — the row with `is_active = true`. The single-active invariant is enforced by a partial unique index in Postgres.

---

## WinSparkle Configuration (Desktop Side)

```cpp
#include <winsparkle.h>

// Initialise once at startup, before win_sparkle_init().
win_sparkle_set_appcast_url("https://metavisage.io/api/appcast.xml");
win_sparkle_set_app_details(
    L"MetaVisage",            // company name
    L"MetaVisage",            // app name
    L"1.2.3"                  // current installed version
);

// Recommended: check on launch, then every 24 hours.
win_sparkle_set_update_check_interval(24 * 60 * 60); // seconds
win_sparkle_set_automatic_check_for_updates(1);

win_sparkle_init();
```

### Recommended polling interval

- **24 hours** in production. Balances responsiveness against bandwidth on Vercel and Supabase Storage.
- **5 minutes** during integration testing — useful when validating a fresh release flips the appcast.

### Code-signing (optional, recommended for production)

WinSparkle can verify the installer's authenticode signature before launching it. The portal does not currently embed a signature in the appcast — the desktop team should sign the installer with the same certificate used for the main app and rely on Windows' built-in signature verification.

If the desktop team wants the portal to embed a Sparkle EdDSA signature in the future, the `releases` table can be extended with a `signature` column and a corresponding `sparkle:edSignature` attribute added to the `<enclosure>`.

---

## Verifying an Update End-to-End

1. **Build a test installer** with a known low version, e.g. `0.0.1`, configured with the staging appcast URL.
2. **Install and launch** the test build on a Windows VM. WinSparkle silently polls the appcast on first run.
3. **In the admin panel**, navigate to `/admin/releases` and click **New Release**.
4. **Upload an installer** with a higher version (e.g. `0.0.2`), enter release notes, and click **Publish Release**.
5. **Trigger an update check** in the test build — either restart it (with a short polling interval) or call `win_sparkle_check_update_with_ui()` from a debug menu.
6. WinSparkle should display the update prompt with the version, release notes, and file size from the appcast. Clicking **Install Update** downloads from Supabase Storage and runs the installer.

If the prompt does not appear:

- Confirm the appcast URL returns `200` and not `404` (`curl -i https://metavisage.io/api/appcast.xml`).
- Confirm the version in the appcast is **strictly greater** than the installed version per semver comparison.
- Check WinSparkle's log output (`%TEMP%\WinSparkle.log` by default).

---

## Operational Notes

- Publishing a new release **automatically deactivates** the previous one. There is no rollback button — to revert, re-publish the previous release from `/admin/releases/<id>`.
- The appcast endpoint is `Cache-Control: no-store`. There is no CDN cache to bust after publishing.
- Installer files are hosted in the public `installers` Supabase Storage bucket. Anyone with the URL can download — treat installers as public artifacts.
- Deleting a release row from the admin panel does **not** delete the installer file from Storage. Clean up unused files manually if disk usage matters.
