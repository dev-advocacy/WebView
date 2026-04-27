# Security Review Notes

This project separates security-relevant components into `Security/` to simplify auditing.

## Scope

The following files are considered security-sensitive:

- `Security/WebViewAuthentication.h`
- `Security/Cookie.h`
- `Security/ClientCertificate.h`
- `Security/CertificateDlg.h`
- `Security/CertificateDlg.cpp`

## Review Checklist

- Validate all authentication event handlers enforce expected origin and state checks.
- Review cookie lifecycle operations for correct domain scoping and cleanup behavior.
- Verify certificate selection and display logic does not expose sensitive material.
- Ensure errors from security APIs are logged without leaking secrets.

## Operational Guidance

- Keep security code changes scoped to `Security/` where possible.
- Require focused review for changes touching authentication, cookie, or certificate logic.
- Add regression tests or manual smoke checks for login and cookie flows when modifying these files.
