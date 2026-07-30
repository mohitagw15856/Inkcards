# Security policy

InkCards is an offline flashcard app for a personal e-reader. It has a small
attack surface: it reads deck and review-state files from an SD card and writes
review state back. It does not, by itself, use networking. Even so, we take
input-handling bugs seriously, because a malformed file should never crash the
device or corrupt your progress.

## Supported versions

InkCards is pre-1.0 and moves quickly. Security fixes are applied to the
`main` branch and released from there. Please test against the latest `main`
before reporting.

## Reporting a vulnerability

Please report suspected vulnerabilities privately rather than opening a public
issue:

- Use GitHub's **Report a vulnerability** button under the repository's
  **Security** tab (Private vulnerability reporting), or
- open a minimal private channel with the maintainers if that is not available.

Include:

- a description of the problem and its impact,
- steps to reproduce (a sample `.deck` or `.rev` file is ideal),
- the affected component (firmware engine, device app, or companion) and
  version or commit.

We aim to acknowledge a report within a week and to agree a disclosure timeline
with you. Please give us a reasonable chance to ship a fix before disclosing
publicly.

## Scope and good-to-know

- The parsers in `firmware/lib/inkcards_core` and `companion/inkcards` are the
  most security-relevant code. They are written to fail safe on malformed
  input (bounds-checked reads, tolerant of unknown fields). Bug reports with a
  reproducing file are especially welcome.
- The companion reads Anki `.apkg` files, which are ZIP archives containing an
  SQLite database. Only convert `.apkg` files you trust.
- InkCards stores no secrets and requires no credentials.

Thank you for helping keep InkCards safe.
