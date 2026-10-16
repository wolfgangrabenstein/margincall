# Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

## What is signed

Only the Windows executable of MARGIN CALL (`MarginCall-<version>-Windows.exe`) is signed. Every signed file is built from the source code in this repository by the public GitHub Actions workflow [`.github/workflows/build.yml`](.github/workflows/build.yml). Nothing built outside this workflow is ever signed. Files from other projects are not signed under this project.

## Build and signing process

1. A release is started by pushing a version tag (`v*`) to this repository.
2. GitHub Actions builds the Windows executable from source (Tauri 2, Rust).
3. The unsigned executable is submitted to SignPath.io through the official GitHub Action.
4. An approver reviews the request and approves it manually in SignPath.
5. The signed executable is attached to the GitHub release.

## Team roles

| Role | Members |
|---|---|
| Committers and reviewers | [Wolfgang Rabenstein](https://github.com/wolfgangrabenstein) |
| Approvers | [Wolfgang Rabenstein](https://github.com/wolfgangrabenstein) |

All team members use multi-factor authentication for GitHub and SignPath. Contributions from people outside the team (pull requests) are reviewed by a reviewer before they are merged.

## Privacy policy / Datenschutz

**English:** This program will not transfer any information to other networked systems unless specifically requested by the user or the person installing or operating it. In detail:

- **Live market data:** On first start, MARGIN CALL asks whether it may download current market data (Dow Jones, S&P 500, Nasdaq-100, crude oil and a few individual stocks) from Yahoo Finance (`query1.finance.yahoo.com`). Only if the user agrees are these public price data requested. As with any web request, Yahoo receives the user's IP address. No personal data, game data or identifiers are sent. The setting can be changed at any time under *Optionen → Live-Kurse*. Yahoo's privacy policy: <https://legal.yahoo.com/us/en/yahoo/privacy/index.html>
- **Multiplayer:** The game only connects to a multiplayer host whose address the user enters. Other players in the same game see the player's in-game name, chosen character and position.
- **Local storage:** Key bindings, settings and the single-player highscore list (name entered by the player, score, date) are stored locally on the user's computer only and are never transmitted.

**Deutsch:** Dieses Programm überträgt keine Informationen an andere Systeme im Netzwerk, außer der Nutzer verlangt es ausdrücklich. Im Einzelnen:

- **Live-Kurse:** Beim ersten Start fragt MARGIN CALL, ob es aktuelle Kurse (Dow Jones, S&P 500, Nasdaq-100, Rohöl und einige Einzelaktien) bei Yahoo Finance (`query1.finance.yahoo.com`) abrufen darf. Nur bei Zustimmung werden diese öffentlichen Kursdaten angefragt. Wie bei jedem Webseitenaufruf erhält Yahoo dabei die IP-Adresse. Persönliche Daten, Spieldaten oder Kennungen werden nicht gesendet. Die Einstellung lässt sich jederzeit unter *Optionen → Live-Kurse* ändern. Datenschutzhinweise von Yahoo: <https://legal.yahoo.com/de/de/yahoo/privacy/index.html>
- **Multiplayer:** Das Spiel verbindet sich nur mit einem Host, dessen Adresse der Nutzer selbst eingibt. Mitspieler sehen im Spiel Name, gewählte Figur und Position.
- **Lokale Speicherung:** Tastenbelegung, Einstellungen und die Highscore-Liste des Einzelspielers (selbst eingegebener Name, Punkte, Datum) werden nur lokal auf dem eigenen Rechner gespeichert und nie übertragen.

The firmware for the M5Stack Cardputer ADV is not signed by SignPath. It only loads market data after the user has set up WLAN on the device.

## Uninstallation

MARGIN CALL is not installed. To remove it, delete the downloaded file. Settings are kept in the WebView storage of the app and can be reset under *Optionen → Standard wiederherstellen*. The highscore list is the file `MarginCall/highscores.json` in the user's application data folder (Windows: `%APPDATA%`, macOS: `~/Library/Application Support`, Linux: `~/.local/share`); delete it to remove all entries.
