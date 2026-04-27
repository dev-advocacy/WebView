# Namespace Guide

This guide defines the intended namespace organization after project restructuring.

## Target Namespaces

- `WebView2::Core`
- `WebView2::Security`
- `WebView2::UI`
- `WebView2::Messaging`
- `WebView2::Utilities`
- `WebView2::Logging`

## Migration Strategy

Namespace migration should be incremental to avoid broad regression risk.

1. Introduce namespace blocks in leaf headers first.
2. Add focused `using` declarations only where needed.
3. Update call sites module-by-module.
4. Run full Debug and Release builds after each module conversion.

## Practical Rules

- Prefer fully-qualified symbols in headers.
- Avoid `using namespace` in headers.
- In `.cpp` files, local aliases are acceptable for readability.
- Keep public API names stable when possible during migration.

## Suggested Mapping

- Core classes (`CWebView2`, dialog host internals): `WebView2::Core`
- Authentication/cookie/cert helpers: `WebView2::Security`
- Main frame and dialog UIs: `WebView2::UI`
- Registered message helpers: `WebView2::Messaging`
- Utility/helper code: `WebView2::Utilities`
- Logger internals: `WebView2::Logging`

## Status

Folder restructuring is complete; namespace migration is staged and should be applied in follow-up refactors with dedicated compile/test cycles.
