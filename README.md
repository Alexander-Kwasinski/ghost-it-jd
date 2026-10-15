# Ghost Hunter Gadget

A handheld Bop-It-style game for ECE 1895 (Design Project #2). The device plays a sound cue, and the player must perform the matching action (SCAN, TRAP IT, or PURGE IT) before time runs out. Built on an ESP32, programmed with the Arduino IDE.

---

## Setup

1. Install [Git](https://git-scm.com/downloads) and the [Arduino IDE](https://www.arduino.cc/en/software).
2. In the Arduino IDE, open **Tools > Board > Boards Manager**, search for **esp32 by Espressif Systems**, and install it.
3. Install these libraries from **Sketch > Include Library > Manage Libraries**:
   - Adafruit SSD1306
   - Adafruit GFX
   - Adafruit MPU6050
   - audio library (still needs to be selected)
4. Clone the repo (see the Git section below) and open `firmware/ghost_hunter/ghost_hunter.ino`.

> Everyone should use the same library versions. Write the versions here once the team has picked them:
> - Adafruit SSD1306: _x.x.x_
> - Adafruit GFX: _x.x.x_
> - Adafruit MPU6050: _x.x.x_
> - audio: _x.x.x_

## Uploading to the ESP32

1. Plug the ESP32 in with a data-capable USB cable.
2. Select **Tools > Board > ESP32 Dev Module** and choose the correct port.
3. Click **Upload**. If it fails to connect, hold the **BOOT** button on the board while it says "Connecting...".
4. Open the Serial Monitor at **115200 baud** to see debug messages.
5. Copy the MP3 files from `audio/sd_card/` onto the root of the FAT32-formatted microSD card before testing sound.

---

## Git Basics

### What git is

| Term | Meaning |
|---|---|
| **Repository (repo)** | The project folder plus its full history |
| **Clone** | Download a copy of the repo from GitHub |
| **Commit** | A saved snapshot of your changes, with a short message |
| **Branch** | Your own line of work, so you can't break anyone else's |
| **Push** | Upload your commits to GitHub |
| **Pull** | Download other people's commits from GitHub |
| **Pull Request (PR)** | A request to merge your branch into `main` |
| **`main`** | The branch that always holds the working version |

If you'd rather use buttons than commands, **GitHub Desktop** or the **Source Control tab in VS Code** do all of the steps below.

### One-time setup

```bash
git config --global user.name "Your Name"
git config --global user.email "you@pitt.edu"
git clone <REPO_URL>
cd ghost-hunter
```

When prompted to sign in, use the browser window that opens. GitHub no longer accepts account passwords for git.

### Daily workflow

**1. Get the latest code**
```bash
git switch main
git pull
```

**2. Make a branch for your task**
```bash
git switch -c <branch name>
```

**3. Do your work**, then check what changed:
```bash
git status
```

**4. Commit your changes**
```bash
git add .
git commit -m "message describing changes"
```
Add files by name instead `git add <filename>` if you only want to select specific files.

**5. Push your branch**
```bash
git push -u origin <branch name>
```

**6. Open a Pull Request on GitHub** (click "Compare & pull request"), ask a teammate to look at it, then merge it.

**7. Update your computer after the merge**
```bash
git switch main
git pull
```

Then start the next task from step 2.

### Common problems

| Problem | Fix |
|---|---|
| Want to throw away edits to a file | `git restore <file>` (permanently deletes your uncommitted changes) |
| Staged the wrong file | `git restore --staged <file>` |
| Typo in last commit message (not pushed) | `git commit --amend -m "New message"` |
| `git push` rejected | Someone pushed first. Run `git pull`, fix any conflicts, push again |
| Not sure what's going on | `git status` and `git log --oneline -10` |

Before running anything with `--force`, `reset --hard`, or `clean`, ask a teammate. Those can permanently destroy work.

### Cheat sheet

```bash
git status                  # what's changed?
git pull                    # get teammates' work
git switch -c feature/name  # new branch
git add <files>             # stage files
git commit -m "message"     # save a snapshot
git push                    # upload your commits
git log --oneline           # recent history
```

---

## Team Rules

1. Never commit directly to `main`. Use a branch and a pull request.
2. Pull before you start working and before you push.
3. Commit small and often, with clear messages.
4. Altium and CAD files are binary and can't be merged, so only one person edits a given file at a time. Say so in the group chat first.
