# Pomo

![Pomo](screenshot.png)

A keyboard-driven Pomodoro timer for [Omarchy](https://omarchy.org). 25 minutes of focus,
then a 5 minute break, with a 15 minute break after every fourth session. It follows the
Omarchy theme accent and language live, sends a desktop notification when a timer ends,
and counts the focus sessions you finish each day. The window shows where you are in
the set of four and what comes next, and its colors follow the whole Omarchy theme,
light themes included.

Each new phase waits for you to press Space, so a break never starts while you're away.

## Shortcuts

| Key   | Action                  |
|-------|-------------------------|
| Space | Start or pause          |
| R     | Reset this timer        |
| S     | Skip to the next phase  |
| - / + | Make this timer 5 min shorter or longer (while it is not running) |
| ?     | Show or hide shortcuts  |
| Q     | Quit (asks while running) |

## Build and install

```sh
bin/build     # build/pomo
bin/test      # Qt Test, offscreen
bin/install   # makepkg -fsi, adds Pomo to the app launcher
```

Today's count lives in `~/.local/state/pomo/today`.

## Bar widget

`plugin/exu23.pomo` is an omarchy-shell bar widget: a progress ring and the time left,
shown only while Pomo is open. Clicking it focuses the Pomo window.

```sh
ln -s "$PWD/plugin/exu23.pomo" ~/.config/omarchy/plugins/exu23.pomo
omarchy plugin enable exu23.pomo
```

Pomo writes its state to `$XDG_RUNTIME_DIR/pomo/status.json` for the widget to read.
