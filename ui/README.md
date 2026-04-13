# UI Directory Conventions

This directory stores site-level UI code only.

## Purpose

- Keep presentation code separated from project content assets.
- Make staged visual changes easy to review and roll back.

## What belongs here

- `ui/css/` for global stylesheet files
- `ui/js/` for UI behavior scripts

## What does not belong here

- Images, videos, data files, plot exports, or embedded interactive deliverables

## Content-only assets location

- Keep content assets in `assets/`:
  - `assets/images/`
  - `assets/videos/`
  - `assets/plots/`
  - `assets/html-assets/`

## Integration points

- Head include references live in `_includes/head-custom.html`.
- Use site-relative paths for UI files:
  - `/ui/css/...`
  - `/ui/js/...`
