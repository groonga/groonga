const {owner: owner, repo: repo} = context.repo;
const tag = context.ref.replace("refs/tags/", "")
try {
  const {data: release} = await github.repos.getReleaseByTag({
    owner: owner,
    repo: repo,
    tag: tag
  });
  return release.upload_url;
} catch (e) {
  const {data: createdRelease} = await github.repos.createRelease({
    owner: owner,
    repo: repo,
    tag_name: tag
  });
  return createdRelease.upload_url;
}
