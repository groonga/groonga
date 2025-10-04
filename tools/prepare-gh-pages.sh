#!/bin/bash

set -eux

if ! git fetch origin gh-pages; then
  git worktree add --orphan -b gh-pages site
else
  git worktree add site -B gh-pages origin/gh-pages
fi

cd site

mkdir -p _layouts
cat <<HTML > _layouts/skelton.html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Groonga</title>
  <meta name="author" content="Groonga Project">
</head>
<body>
  {{ content }}
</body>
</html>
HTML
git add _layouts/skelton.html

cat <<GITIGNORE > .gitignore
/_site/
GITIGNORE
git add .gitignore

user=${GITHUB_REPOSITORY%/*}
repository=${GITHUB_REPOSITORY#*/}
cat <<CONFIG > _config.yml
markdown: kramdown
uri: https://${user}.github.io/${repository}/

kramdown:
  input: GFM
  smart_quotes: ["apos", "apos", "quot", "quot"]
CONFIG
git add _config.yml

cat <<MARKDOWN > index.md
---
layout: skelton
---

* [Benchmark](dev/bench/)
MARKDOWN
git add index.md

git config user.name "github-actions[bot]"
git config user.email "github-actions[bot]@users.noreply.github.com"

if [ "$(git diff --cached)" != "" ]; then
  git commit -m "Prepare"
  git push origin "$(git branch --show-current)"
fi
