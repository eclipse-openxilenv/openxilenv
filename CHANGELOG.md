# Changelog

All notable changes to OpenXilEnv are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project aims to adhere to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

> Detailed, per-release notes are generated automatically from the merged pull
> requests and are published with each
> [GitHub Release](https://github.com/eclipse-openxilenv/openxilenv/releases).
> This file provides the curated, human-readable high-level history.

## [Unreleased]

### Added
- Automated release pipeline: pushing a version tag (`v*`) builds self-contained
  Linux and Windows packages via CPack and publishes them together as a GitHub Release.
- CPack packaging support (`.tar.gz` on Linux, `.zip` on Windows), reproducible
  manually with `cpack`.

## [0.9.2] - 2026-01-22

Current baseline release of OpenXilEnv (the open X-in-the-loop simulation
environment), including the GUI (`XilEnvGui`), the console variant (`XilEnv`),
the RPC interface, the external-process interface, the Python API, and CAN
message decoding.

[Unreleased]: https://github.com/eclipse-openxilenv/openxilenv/compare/v0.9.2...HEAD
[0.9.2]: https://github.com/eclipse-openxilenv/openxilenv/releases/tag/v0.9.2
