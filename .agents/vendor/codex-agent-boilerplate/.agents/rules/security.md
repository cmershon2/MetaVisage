# Security

## Secrets and Credentials

- Never hardcode secrets, API keys, or credentials in source code
- Use environment variables; document required variables in `.env.example`
- Never commit `.env` files — ensure they are gitignored before first commit
- Rotate any secret accidentally committed immediately

## Input Validation

- Validate all user input at the boundary — before any processing
- Sanitize for the context of use (HTML, SQL, shell, etc.)
- Reject unexpected input; do not try to "fix" malformed data
- Validate on the server side regardless of client-side validation

## Common Vulnerabilities (OWASP Top 10)

- **Injection**: Use parameterized queries, never string-concatenate SQL; use safe shell APIs
- **XSS**: Escape output in the correct context; use framework-provided templating
- **IDOR**: Verify ownership/authorization before any resource access, not just authentication
- **Sensitive Data Exposure**: Encrypt data in transit (TLS) and at rest where required
- **Broken Auth**: Use established auth libraries; don't roll your own

## Dependencies

- Run dependency audits regularly (`npm audit`, `pip audit`, etc.)
- Update dependencies with known CVEs promptly
- Pin versions in production to prevent supply chain drift

## Error Messages

- Return generic error messages to clients — never expose stack traces or internal details
- Log detailed errors server-side only
- Do not log sensitive data (passwords, tokens, PII) even in debug mode

## Code Review Checklist for Security

- [ ] No secrets in code or config files
- [ ] All inputs validated and sanitized
- [ ] Authorization checked on every protected resource
- [ ] No SQL/shell/HTML injection vectors
- [ ] Dependencies audited
