# Codex Session Continuity

Different Codex accounts may sequentially work on the same working tree.
Conversation history is not assumed to transfer between sessions.

## Starting a Session

Before continuing substantial existing work:

1. Read `.codex/HANDOFF.md` if present.
2. Inspect:
   - `git status`
   - `git diff`
   - `git diff --staged`
3. Treat existing uncommitted changes as intentional until proven otherwise.
4. Continue completed/in-progress work rather than starting it again.

## One Active Writer

Only one Codex session should actively modify the working tree at a time.

Accounts are used for sequential failover, not concurrent editing.

Before modifying files:
- acquire .codex/ACTIVE_SESSION
- if another active session owns it, do not modify the repo
- release/update it when handing off

## Handoff

Maintain `.codex/HANDOFF.md` during substantial tasks.

Update it after meaningful milestones, important decisions, significant
test results, before risky/large operations, and when usage is becoming
constrained.

Do not depend on having a final model turn before a usage limit is reached.

Keep the handoff concise and include:

- Objective
- Current status
- Completed work
- Work in progress
- Next actions
- Files changed
- Tests and results
- Known problems
- Important decisions

## Git Safety

Do not reset, clean, restore, stash, or otherwise discard inherited work
solely because it came from another session.

Do not use destructive commands such as `git reset --hard`,
`git clean -fd`, or `git restore .` unless explicitly instructed.

If `.codex/HANDOFF.md` disagrees with the actual working tree, trust the
working tree and update the handoff.

## Interrupted Sessions

Assume a previous session may have stopped in the middle of an operation.

Check for partial edits, incomplete renames, missing imports,
half-completed migrations, lockfile changes, and failing tests before
continuing.

## Core Requirement

At any point another Codex session should be able to inspect:

- `AGENTS.md`
- `.codex/SESSION_CONTINUITY.md`
- `.codex/HANDOFF.md`
- `git status`
- `git diff`

and safely continue the task.