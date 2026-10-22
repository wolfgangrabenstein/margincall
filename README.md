# MARGIN CALL

![MARGIN CALL – Bulls vs. Bears vs. You vs. AI](docs/banner.png)

**Language:** **English** · [Deutsch](README.de.md)

A retro first-person stock market shooter: bulls and bears fight each other and you. Real daily quotes for the Dow Jones, S&P 500, Nasdaq-100 and crude oil decide which side is stronger, and the AI ALGO-9000 manipulates the market.

**Bull run and bear market:** take out lots of bears and the market tips into a **bull run** (faster, more cash, 1.5× points, plus a bull stampede you have to dodge). Take out lots of bulls and a **bear market** follows with a **stock market crash**: the floor shakes, cash slides away, bears attack in packs – but bears give double **short gains**. The real market sets the base mood; a marker above the bulls/bears bar shows where the market is heading.

## Play in the browser

No installation needed: **[play MARGIN CALL in the browser](https://wolfgangrabenstein.github.io/margincall/?lang=en)**. Solo and online multiplayer work there too; LAN multiplayer needs the app.

## Downloads

Current version **3.28.1** – click a file and the download starts right away:

| Download | System |
|---|---|
| [**MarginCall-3.28.1-Windows.exe**](https://github.com/wolfgangrabenstein/margincall/releases/download/v3.28.1/MarginCall-3.28.1-Windows.exe) | Windows 10/11 |
| [**MarginCall-3.28.1-macOS.dmg**](https://github.com/wolfgangrabenstein/margincall/releases/download/v3.28.1/MarginCall-3.28.1-macOS.dmg) | macOS 10.15 or newer (Intel and Apple Silicon) |
| [**MarginCall-3.28.1-Linux.AppImage**](https://github.com/wolfgangrabenstein/margincall/releases/download/v3.28.1/MarginCall-3.28.1-Linux.AppImage) | Linux (x86_64) |
| [**MarginCall-3.28.1-Cardputer.bin**](https://github.com/wolfgangrabenstein/margincall/releases/download/v3.28.1/MarginCall-3.28.1-Cardputer.bin) | M5Stack Cardputer ADV, via M5Launcher from the SD card |

Newer version out? Always current: [latest release](https://github.com/wolfgangrabenstein/margincall/releases/latest) · older: [all releases](https://github.com/wolfgangrabenstein/margincall/releases) · changes: [CHANGELOG](CHANGELOG.md)

## Screenshots

| Title screen with live quotes | Level 1: bulls, bears and flying banknotes | Online play |
|---|---|---|
| ![Title screen](docs/screenshot-title.png) | ![Gameplay](docs/screenshot-level1.png) | ![Online play](docs/screenshot-online.png) |

Made by Wolfgang Rabenstein · Open source under the [MIT license](LICENSE) · [Code signing policy](CODE_SIGNING_POLICY.md)

MARGIN CALL runs as a small standalone app on Windows, macOS and Linux (Tauri), supports multiplayer for 2–4 players over LAN or online (co-op, team, deathmatch), runs in the browser and also exists as firmware for the M5Stack Cardputer ADV. All binaries are built from this repository by GitHub Actions and published as free downloads on the Releases page.

## First start

- **Windows:** SmartScreen reports an unknown app. Click "More info", then "Run anyway". If the firewall asks, allow access on private networks (needed for multiplayer).
- **macOS:** The app is not notarized by Apple.
  1. Open the `.dmg` file and drag MARGIN CALL into the Applications folder.
  2. Run `xattr -cr "/Applications/MARGIN CALL.app"` once in the Terminal.
  3. Then start the app as usual.
- **Linux:**
  1. Make the file executable: `chmod +x MarginCall-Linux.AppImage`.
  2. Start it with `./MarginCall-Linux.AppImage`.
  3. If the window stays empty, `libfuse2` may be missing (Ubuntu: `sudo apt install libfuse2`).

## Controls (PC)

| | Default |
|---|---|
| Move | W / S or ↑ / ↓ |
| Strafe | A / D |
| Turn | Mouse (click into the view) or ← / → |
| Fire | Space or left mouse button |
| Dodge | Double-tap a direction |
| Pause | Esc or P |
| Resume | Esc or P in the pause menu |
| Back to main menu | Q in the pause menu |
| Quit | Q in the main menu, then confirm with Y (N cancels) |
| Play solo: easy, normal or hardcore | Enter in the main menu, then E (easy: weaker enemies, unlimited lives), Enter (normal: 5 lives) or X (hardcore: one life) |
| Credits | C in the main menu |
| Language (English/German) | L in the main menu or Options → Language; Cardputer: L on the title screen |
| Fullscreen | F4 |
| Music | M |
| Options (key bindings) | O |
| Choose character (multiplayer) | C in the lobby |
| Highscores | H on the title screen |
| Chat (multiplayer) | T |
| Show stats | Tab (hold) |

All keys can be remapped in the options menu. On the Cardputer: E/S move, A/D turn, W/R strafe.

## Online multiplayer (2–4 players over the internet)

1. Choose **[N] Multiplayer → [O] Online play** (in the browser version: **[N] Multiplayer**) and agree to the privacy notice once.
2. **Public game:** press **[H]**. Your game shows up in the list of open games for everyone with the same game version.
3. **Private game:** press **[P]** and share the five-character code shown in the top right corner of the lobby with your friends.
4. **Join:** pick a game from the list or type the code and press **Enter**.

Online play is free and runs through a small game server on Cloudflare. The free daily quota is shared by all players worldwide; if it is used up, online play pauses until 00:00 UTC (LAN keeps working).

## Multiplayer (2–4 players on the same network)

1. One player chooses **[N] Multiplayer → [H] Host game**. The lobby shows the address, e.g. `192.168.1.20:8080`.
2. The others choose **[N] Multiplayer → [J] Join game** and click the game in the list. If it is not found (some routers block discovery), they enter the address from the host's lobby or open it in a browser.
3. Players mark themselves **ready** with **E** (or a click). In the lobby you can chat with **T** and change your nickname with **K**; the host renames the game with **G**.
4. The host picks the mode (co-op, Team Bull vs. Team Bear, or deathmatch) and starts once everyone is ready.

## Project structure

| Folder | Contents |
|---|---|
| `pc/game.html` | The game (rendering, logic, sounds, music, multiplayer) |
| `pc/server.py` | Development server (Python), same behavior as the app server |
| `desktop/` | Desktop app (Tauri 2): built-in Rust server plus game window |
| `src/`, `platformio.ini` | Cardputer firmware (PlatformIO, Arduino) |
| `tools/` | Generators for sound effects and the app icon |
| `docs/` | Banner and screenshots for the GitHub page |
| `worker/` | Online leaderboard (Cloudflare Worker + D1), setup in [worker/README.md](worker/README.md) (German) |

### Building it yourself

- **Desktop:** Requires Node 20+, Rust and, on Linux, the WebKitGTK packages. Build with `cd desktop && npm install && npx tauri build`.
- **Cardputer:** Build with `pio run`.
- **All platforms:** The GitHub workflow `.github/workflows/build.yml` does this automatically.

## Privacy

Live quotes are only loaded if you agree on first start. The game then requests public market data from Yahoo Finance, which sees your IP address like with any website you visit. You can change this anytime under *Options → Live quotes*. Only what you explicitly upload ends up on the online leaderboard: after a run, your self-chosen name and score are sent to the leaderboard server only when you press "Save & post online" (Enter). TAB saves locally only, ESC saves nothing; you can change the default permanently under *Options → Online leaderboard*. Online multiplayer only connects after you agree to a separate notice: your nickname, the game name and the running game are forwarded to the other players through the MARGIN CALL server on Cloudflare, which does not store game contents. Otherwise the game transmits nothing; in LAN multiplayer it only connects to the host you pick. Details: [Code signing policy → Privacy policy](CODE_SIGNING_POLICY.md#privacy-policy--datenschutz).

## Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by [SignPath Foundation](https://signpath.org/). Process, roles and privacy: [CODE_SIGNING_POLICY.md](CODE_SIGNING_POLICY.md).

## License & contributing

MARGIN CALL is released under the [MIT license](LICENSE). Contributions are welcome; please follow the [Code of Conduct](CODE_OF_CONDUCT.md).
