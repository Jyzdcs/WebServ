# /project:pr-review

Review the current branch changes against the WebServ subject and evaluation grid before creating a PR.

## Instructions

Follow the skill defined in `.agents/skills/pr-review/SKILL.md` step by step:

1. **Run tests first** (step 0): `make` then `bash tests/run_all.sh` (or auto-discover in `tests/`)
   - Stop and report if build or tests fail
2. Read `docs/subject.md` and `docs/evaluation.md`
3. Run `git fetch origin main && git diff origin/main...HEAD` to get the full diff
4. Run `git diff --name-only origin/main...HEAD` for changed files
5. Run `git log origin/main...HEAD --oneline` for commits
6. Analyse each change against subject requirements and evaluation criteria
7. Check for all grade-0 critical violations (single poll, no errno after read/write, fork only for CGI, etc.)
8. Output the structured PR Review Report as defined in the skill

Do not create the PR — only produce the review report.
