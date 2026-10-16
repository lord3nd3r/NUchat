# Releasing

GitHub Release assets are built by [`.github/workflows/release.yml`](../.github/workflows/release.yml). The workflow does not create the tag or the release. It builds the four platform jobs that already run on pull requests, packs each artifact into the same archive name used on previous releases, and uploads those archives.

## Assets

| Archive | Workflow | Archive contents |
|---------|----------|------------------|
| `nuchat-linux.zip` | Linux | `nuchat` (mode `0755`) |
| `nuchat-freebsd.zip` | FreeBSD | `nuchat` (mode `0755`) |
| `nuchat-macos.zip` | macOS | `nuchat-macos.dmg` |
| `nuchat-windows-installer.zip` | Windows | `NUchat_<version>_Windows_x64.exe` |

`<version>` is `project(NUchat VERSION ...)` in `CMakeLists.txt`. The Windows CI job passes that version to CPack, so the installer filename inside the zip changes when the project version changes.

Linux ASan/UBSan stays a pull-request check. It does not produce a release asset.

Push and pull request CI is unchanged. Each platform workflow still runs on its own for pushes and pull requests to `main`. `workflow_call` is how the release workflow runs those same steps.

## Publish a release

1. Update the version in `CMakeLists.txt`, `CHANGELOG.md`, and the version badge in `README.md`. Merge that to `main`.
2. Publish a GitHub Release whose tag matches `v*` (for example `v1.2.3`) and whose target is the commit to ship. Publishing creates the tag.
3. The Release workflow starts for the `release` `published` event and for the `v*` tag push. Both runs share one concurrency group for that tag, so the later run cancels the earlier one. The run that finishes uploads with `gh release upload --clobber`, which replaces an asset of the same name.

A `v*` tag push whose release already exists is enough on its own. The upload waits about a minute for the release to appear, then fails if it is still missing.

## Run the workflow without publishing

After this workflow file is on the default branch, open **Actions → Release → Run workflow**.

- Leave **tag** empty to build and package only. The Package job uploads a `release-zips` artifact with the four archives and does not call `gh release upload`.
- Set **tag** to an existing release tag to build again and upload with `--clobber`.

Pick the branch or tag to build from the **Use workflow from** menu. The workflow definition must exist on the default branch before GitHub lists it.

## Permissions

| Job | `GITHUB_TOKEN` |
|-----|----------------|
| Linux, macOS, Windows | `contents: read`, `actions: write` (cache save, same as the pull-request workflows) |
| FreeBSD, Package | `contents: read` |
| Upload | `contents: write` |

`contents: write` is only on the upload job. That is what `gh release upload` needs. The package job is the one that downloads the build artifacts and creates the zip files.
