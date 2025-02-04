---
name: Release
about: Issue for release
title: Groonga x.x.x
labels: ''
assignees: ''

---

We plan to release x.x.x in a few days.

guide: http://groonga.org/ja/docs/contribution/development/release.html (Japanese)

- [ ] Update version for a new release: `reke release:version:update NEW_RELEASE_DATE=YYYY-MM-DD`
- [ ] Add a release note
  - [ ] Update `doc/text/news/XX.md`
  - [ ] Translate news
- [ ] Prepare announcement text
  - [ ] Announce
    - [ ] X (Japanese/English)
    - [ ] Facebook (Japanese/English)
- [ ] Update documentation
  - [ ] Update .rst
  - [ ] Translate (Update .po)
- [ ] Check CI https://github.com/groonga/groonga/actions
- [ ] Check Launchpad Nightly https://launchpad.net/~groonga/+archive/ubuntu/nightly/+packages
- [ ] Tagging: `rake release:tag`
- [ ] Upload packages
  - [ ] Ubuntu (launchpad)
- [ ] Tagging on groonga/groonga.org: `rake release:version:update`
- [ ] Announce
  - [ ] X (Japanese/English)
  - [ ] Facebook (Japanese/English)
- [ ] Update version for new development: `rake dev:version`
- [ ] Update MSYS2 package: `packages/post-msys2.sh your/MINGW-packages/fork`
- [ ] Update Docker image on groonga/docker: `./update.sh`
