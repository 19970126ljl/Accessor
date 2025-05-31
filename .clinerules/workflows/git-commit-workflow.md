---
description: Automates the Git commit process, including .gitignore management and automatic commit message generation.
author: Cline
version: 1.0
tags: ["git", "workflow", "automation"]
globs: ["*"]
---
# Git Commit Workflow

**Objective:** To guide Cline through a standardized Git commit process, ensuring proper file exclusion and automatic commit message generation.

**Trigger:** When the user explicitly requests to perform a Git commit, e.g., "请帮我提交更改" (Please help me commit changes) or "执行 Git 提交" (Execute Git commit).

**Process:**

1.  **Check Git Status:**
    *   Execute `git status` to identify modified, added, or deleted files.
    *   Inform the user about the current status.

2.  **Manage .gitignore:**
    *   Check for the existence of `.gitignore`.
    *   If it exists, read its content.
    *   If it doesn't exist or is incomplete, suggest creating/updating it with common exclusions (e.g., `build/`, `dist/`, `node_modules/`, `*.log`, `*.tmp`, `*.swp`, `.vscode/`, `nohup.out`, `sequentialthinking-mcp-server/`).
    *   Ask the user for confirmation before modifying `.gitignore`.

3.  **Stage Changes:**
    *   Execute `git add .` to stage all changes, respecting `.gitignore` rules.
    *   Confirm staging with the user.

4.  **Generate Commit Message:**
    *   Execute `git diff --staged` to get the diff of staged changes.
    *   Analyze the diff to automatically generate a concise and descriptive commit message, following conventional commit guidelines if applicable.
    *   Present the proposed commit message to the user for review and approval.

5.  **Perform Commit:**
    *   Execute `git commit -m "Generated commit message"` after user approval.
    *   Confirm successful commit.

6.  **Push Changes (Optional):**
    *   Ask the user if they want to push the changes to the remote repository.
    *   If confirmed, execute `git push`.

7.  **Workflow Reflection (Optional):**
    *   Ask the user if they would like to reflect on the execution of this Git Commit Workflow and suggest potential improvements to its definition (`.clinerules/workflows/git-commit-workflow.md`).
    *   If confirmed, analyze the interaction during this workflow's execution and propose specific, actionable improvements to this workflow's definition.

**Constraint:**
*   This workflow assumes a standard Git repository setup.
*   User interaction is required for `.gitignore` modifications and commit message approval.
