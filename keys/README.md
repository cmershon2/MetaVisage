# License JWT Public Keys

RS256 public keys for verifying license JWTs from the web portal. Drop one
PEM file per environment here:

- `development-public.pem` — local-dev key (web team will provide via 1Password)
- `staging-public.pem` — staging environment key
- `production-public.pem` — production environment key

The active build picks the file referenced by `METAVISAGE_LICENSE_JWT_PUBLIC_KEY_FILE`
in the matching `.env.<profile>` at the project root. CMake reads the file
at configure time and bakes the contents into `LicenseConfig.h` — no runtime
file load.

## Generating keys

The web team owns key generation; this is the openssl recipe documented in
the portal repo for reference:

```sh
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out private.pem
openssl rsa -in private.pem -pubout -out public.pem
```

The portal stores the **private** half (Vercel env var `LICENSE_JWT_PRIVATE_KEY`).
Only the **public** half lives here — never commit a private key.

## Gitignore

`keys/*.pem` is gitignored. This directory is preserved via `.gitkeep` so the
default `METAVISAGE_LICENSE_JWT_PUBLIC_KEY_FILE` path resolves cleanly even
on a fresh clone (CMake emits a warning when the referenced file is missing
but does not fail in development; production builds refuse to configure
without a real key).
