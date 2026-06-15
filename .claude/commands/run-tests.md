# /project:run-tests

Run the WebServ test suite and optionally add a new test if the current changes warrant it.

## Instructions

Follow the skill defined in `.agents/skills/run-tests/SKILL.md` step by step:

1. Run `git diff --name-only origin/main...HEAD` to detect which module is being worked on
2. Build the binary if needed: `make`
3. Run the relevant tests:
   - `bash tests/run_all.sh` (all modules)
   - `bash tests/run_all.sh config` / `server` / `http` (specific module)
4. Report pass/fail results clearly
5. Decide if the current changes introduce a major new feature that warrants a new test (use the criteria table in the skill)
6. If yes, add the test to the correct file in `tests/`
7. Report what was added or why nothing was added
