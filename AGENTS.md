# Agent instructions

## Rules

- Project base language is American English.
- No *snake_case* allowed within the project; *UPPER_SNAKE_CASE* or *camelCase* are strongly suggested.
- No overconfident-overdocumentation; keep comments short. A comment is not a
  design document and not a changelog: **two lines max**, and only for the
  *why* the code cannot say itself. No multi-paragraph essays, no "here is why
  this fix is correct", no walkthrough of the bug that led here, no restating
  what the diff just did. If an explanation needs a paragraph, the code is
  failing to say it — rename something, extract a method, or drop the comment.
  Long comments also rot: the longer they are, the faster they stop matching
  the code, and a wrong comment is worse than none.
- No gigantic and unnecessary documentation files in docs folder.
- Refer to `docs/clean-code.md` for writing clean code, which is common sense.
- No manual wrapping in code; Users have **automatic wrapping** option in their IDEs.
- Use only the following comment tags: *NOTE*, *FIXME*, *BUG* (if confident that **it is** a bug), *HACK*.
- No dumb mock tests allowed; Mocking means just reading lines or implementing behavior inside the test. Only test real behavior.

## Patch artifact - for every turn inside the chat

If you modify anything in this local clone, refresh the root-level
`agent-changes.patch` before completing the turn. It must represent all current
working-tree changes except itself, including newly created files:

```bash
# Mark each new non-patch file as intent-to-add, without staging its content.
git add -N -- <new-files>

# Write a portable patch for tracked edits and intent-to-add files.
git diff --binary -- . ':(exclude)agent-changes.patch' > agent-changes.patch
```

Do not put `.venv`, build products, or other generated artifacts into the patch.
Mention the patch path and any validation performed in the final response.