# Releasing a version

What happens for a new version (Claude does these steps when asked for "Ditto 1.1"):

1. **The work** happens on `next`. When it is ready: the number in `firmware/src/felucca.c` (`FELUCCA_VERSION`) and in
   `web/site/project.json`, a new entry at the top of `CHANGELOG.md` (`## 1.1 — date`, then what is new).
2. **Build and test**: `./build.sh`, `sh tests/run_tests.sh`, then the package: `./build.sh --release 1.1`
   (`build/felucca-1.1.fwsc`, identity `FM-1_911`).
3. **The website** (branch `gh-pages`): `python3 web/site/make_pages.py build/felucca-1.1.fwsc build/emu/ditto.html OUT`
   writes it: "What's new" and the list of versions come from `CHANGELOG.md`. Commit OUT on `gh-pages`; check the
   installer with `node web/site/test_install.mjs`, the SOUNDS page with `node web/site/test_sounds_page.mjs OUT`.
4. **`main`** takes `next` (a merge), and the tag: `git tag ditto-v1.1 && git push origin main ditto-v1.1` (with the name in front: Felucca's tags v1.0 .. v1.5.1 and Golana's golana-v* came with its history). The tag (or, by hand, Actions › Release › Run workflow with the
   version: then the workflow makes the tag) starts the **Release** workflow (`.github/workflows/release.yml`): the GitHub Release with the notes from `CHANGELOG.md` and
   the package the website serves attached.

The numbers: **1.0.1** only fixes; **1.1** adds something and saved programs still open; **2.0** may change how
things are saved. The package identity has room for X.Y: 1.0.1 keeps 1.0's `FM-1_910`.
