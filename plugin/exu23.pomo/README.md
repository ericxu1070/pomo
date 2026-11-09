# exu23.pomo

Omarchy shell bar widget that shows the Pomo timer while the app is open.

- Reads `$XDG_RUNTIME_DIR/pomo/status.json`, which Pomo writes on every state change and deletes on quit.
- Hidden when the file is missing or unparsable, or when the pid it names is no longer a running pomo (a killed app).
- Ring and `m:ss`: accent while a focus session runs, plain foreground on a break, dimmed when paused.
- Tooltip names the phase and session. Click focuses the Pomo window (`hyprctl dispatch focuswindow class:pomo`).

Install:

    ln -s "$PWD" ~/.config/omarchy/plugins/exu23.pomo
    omarchy bar add exu23.pomo --section centerBegin   # or add {"id": "exu23.pomo"} to shell.json
