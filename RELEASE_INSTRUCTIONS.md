# Release Build Workflow Instructions

This document explains how to trigger the release build workflow for the Digital Clock application.

## Overview

The release workflow (`release.yml`) builds a Windows executable and creates a GitHub release. It can be triggered in two ways:

## Method 1: Automatic Trigger (Tag Push)

The workflow automatically triggers when you push a tag starting with "v":

```bash
# Create a version tag
git tag v1.0.0

# Push the tag to trigger the workflow
git push origin v1.0.0
```

## Method 2: Manual Trigger (Workflow Dispatch)

You can manually trigger the workflow from the GitHub interface:

1. Go to the **Actions** tab in the GitHub repository
2. Select **"Build and Release Digital Clock"** from the workflow list
3. Click **"Run workflow"** button
4. Enter the desired version tag (e.g., `v1.0.0`) in the input field
5. Click **"Run workflow"** to start the build

## What the Workflow Does

1. **Build Job** (Windows):
   - Checks out the repository code
   - Installs MinGW (GCC for Windows)
   - Compiles `digital_clock_win32.c` into `digitalclock.exe`
   - Uploads the executable as a build artifact

2. **Release Job** (Ubuntu):
   - Downloads the build artifact
   - Creates a GitHub release with the executable attached

## Expected Output

After successful completion, you should see:
- A new release in the GitHub repository's "Releases" section
- The release will contain the `digitalclock.exe` file for download
- The release will be tagged with the version you specified

## Version Tags

Use semantic versioning for tags:
- `v1.0.0` - Major release
- `v1.1.0` - Minor release with new features
- `v1.0.1` - Patch release with bug fixes

## Troubleshooting

If the workflow fails:
1. Check the Actions tab for error messages
2. Ensure the `digital_clock_win32.c` file compiles correctly
3. Verify that the repository has the necessary permissions for creating releases