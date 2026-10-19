# MARGIN CALL – Changelog

**Language:** **English** · [Deutsch](CHANGELOG.de.md)

## Version 3.22.0 – Separate high score list for Easy (2026-10-19)

- **Easy has its own high score list** – local, in the app and online (all-time and today). In the high score view, **X** switches between normal, hardcore and easy. Since easy is no longer compared with normal, it gives full points again instead of 0.6×.
- **Crypto bubble:** as long as it hasn't noticed you, it is completely green, spikes included. When it attacks, it pulses with a fluorescent red, spikes and all. On the Cardputer too.

## Version 3.21.3 – Menacing crypto bubble (2026-10-19)

- **More menacing crypto bubble:** it looked like a bonus, so players ran towards it. The green bubble and gold bitcoin coin stay, but it now has red spikes all around and pulses with a fluorescent red glow when it attacks. On the Cardputer too.

## Version 3.21.2 – Lives in the HUD, full price charts (2026-10-19)

- **Lives visible:** In normal mode, hearts at the top right of the HUD show how many lives are left (hardcore: one heart). The stats overlay (Tab) also shows "Lives left".
- **Price charts on the walls:** Shortly after the market opens there were only a few price points, so the charts looked flat and blocky. They now always show the latest ~78 price points (about one trading day), including the previous day if needed. Applies to browser, app and Cardputer.
- **Solo choice:** the Normal button no longer has an arrow icon.

## Version 3.21.1 – Back button in the solo dialog (2026-10-19)

- **Bug fix:** Clicking **Back** in the solo choice still started the game, because the "Play solo" button underneath was clicked as well. Main menu dialogs now only accept clicks on their own buttons.

## Version 3.21.0 – Easy mode, 5 lives in normal mode (2026-10-19)

- **New Easy mode** (key E in the solo choice, on the Cardputer on the title screen):
  - Unlimited lives, weaker enemies (less health, less damage, slower fire, slower shots), but 0.6× points.
  - In level 2 you need 1 to 3 fewer kills for the vault keycard.
  - On phones auto aim defaults to **Full** here (on PC with a mouse your setting stays). If you set auto aim yourself in the options, your choice applies.
- **Normal now has 5 lives:** after the fifth bankruptcy the run is over. Remaining lives are shown in the status line and on the bankruptcy screen.
- **Level 1 (normal and hardcore):** the vault keycard needs 1 to 2 more kills.
- **No hits through walls, in every level:** melee (shark, oil slick, pirate), popping crypto bubbles and your own banknotes against large enemies now check line of sight as well. Tested with about 4000 random lines in each of the five levels.
- **Cardputer:** music starts at 30 % volume by default; Easy, 5 lives and the new keycard counts are available here too.

## Version 3.20.3 – No more shots through walls (2026-10-19)

- **Bug fix:** Enemies could shoot through walls when passing close to a corner (and melee enemies could hit around corners). Line of sight and projectile paths are now checked exactly cell by cell, including your own banknotes. Applies to PC, browser and Cardputer.
- **Music** now starts at **30 %** volume by default (existing settings are kept).
- **Phones:** The touch guide now shows the **pause** and **dash** buttons, so the hints point at something.

## Version 3.20.2 – Touch guide with start button (2026-10-19)

- **Phones:**
  - The touch controls guide on your first game now has a **Let's go** button; the solo game only starts once you tap it.
  - In the options, **Touch controls ▶** shows the guide again at any time.
  - The fullscreen button now stays at the **top right** during play as well, so it is no longer hit by accident.

## Version 3.20.1 – Tidier options, new phone icons (2026-10-19)

- **Options (PC):** Key bindings and mouse settings now live on their own **Controls ▶** sub-page (with its own "Restore default controls"). The main page is much clearer.
- **Restore defaults** now only resets controls, sound and display; names, consents and the local high score are kept.
- **Key hint line (PC):** shorter and consistent ("2× direction dodge", "L language"), no line break.
- **Solo choice:** the "All 5 levels" note is gone.
- **Phones:**
  - Modern **pause** and **fullscreen** icons (corner icons for entering/leaving fullscreen), so they are no longer mistaken for the options.
  - Leaving fullscreen during a solo game now **pauses** the game automatically.

## Version 3.20.0 – New main menu, credits, touch tutorial (2026-10-19)

- **Main menu:**
  - One big **Play solo** button; then you choose **Normal** or **Hardcore**, each with a short explanation (Hardcore: much tougher enemies, fewer supplies, 1.5× points and only one life). The separate Hardcore button is gone; X still starts Hardcore directly.
  - New **Credits** button (key C): the credits from the end of the game, then back to the menu.
  - **Fullscreen** is now a clear button at the top right, **language** a button at the top left; the **L** key switches the language in the main menu.
- **Phones:** On your first game a short overlay explains the touch controls (stick on the left, swipe and tap on the right, dodge, pause); it disappears after a few seconds or when you tap. The key hints below the game screen are hidden on phones.
- **Key hint line (PC):** "Q in pause" is gone; it now says "L language" instead.

## Version 3.19.8 – Auto aim in three levels (2026-10-19)

- **Auto aim: Off, Semi or Full.**
  - **Semi** only kicks in for nearby enemies (up to about 7 tiles) within a narrow field of view (about 12° off center). On phones and tablets Semi is now on by default.
  - **Full** works like the previous aim assist: wide field of view, distant enemies too.
  - On PCs, auto aim is in the options as well (meant for trackpads without a mouse), off by default there.
- **Title screen:** The controls line in the middle is gone; it is already shown below the game screen.

## Version 3.19.7 – Aim assist on phones, quitting in the browser (2026-10-19)

- **Aim assist (phones):** New option in the options menu, off by default. When it is on, the view turns smoothly toward the nearest visible enemy in your field of view while you fire, and shots that would just miss snap onto it. The crosshair turns red while a target is locked.
- **Quitting in the browser:** If the browser does not allow closing the tab (e.g. Firefox on phones), the game goes back to the previous page or shows "Thanks for playing". Before, nothing seemed to happen.
- **Online screen:** Tapping an empty spot no longer joins the first game in the list by accident.

## Version 3.19.6 – Phone: tap to fire, own options; keycard earlier in level 1 (2026-10-19)

- **Phone controls:**
  - **No fire button anymore:** A short tap anywhere fires one shot. Holding your finger still on the right turns into continuous fire, and you can keep swiping to turn while doing so. Swiping right away only turns, without shooting.
  - **Dodge** is now a golden double arrow at the far right.
  - **Faster turning** by default, plus a turn sensitivity slider in the options.
  - **Options for phones:** no key bindings, large rows and an easy-to-hit Back button.
- **Keycard earlier in the first level:** There it now drops after 3 to 5 enemies instead of 8 to 16, since the first level has only a few enemies.
- **Language via link:** Opening the browser game from the German GitHub page starts it in German.
- **GitHub page:** "Play in the browser" and the downloads are now at the very top, with new screenshots.

## Version 3.19.5 – Separate screen halves, big fire button (2026-10-18)

- **Phone controls:** The left and right halves of the screen are now strictly separate: the left side only controls the stick (a second finger on the left is ignored), the right side only turns. Moving and turning at the same time now works without interference.
- **No more unwanted continuous fire** when you hold your finger still while turning; a short tap on the right still fires one shot.
- **Fire button** is big again, bottom right above the status bar; dodge sits above it to the left.

## Version 3.19.4 – Fullscreen turns to landscape (2026-10-18)

- **Fullscreen button on phones:** It now switches to landscape automatically; pressing it again leaves fullscreen and the phone turns back to portrait. Before, the rotate command never reached the browser.

## Version 3.19.3 – Better phone controls, fullscreen button (2026-10-18)

- **Turning:** Swiping on the right side now turns more smoothly and scales with the screen width; fast swipes turn further. Turning only starts after a small movement, so the view doesn't jitter when tapping.
- **Firing:** Tap the right side = one shot, hold still = continuous fire. The fire button is smaller and now sits at the right edge, so it is not in the way when turning.
- **Fullscreen button:** On phones, a dedicated button switches to fullscreen and back (iPhone: launching the game via "Add to Home Screen" also hides the browser bar).

## Version 3.19.2 – Phone controls, keycard sound, online polish (2026-10-18)

- **Phones and tablets:**
  - **New controls:** A virtual stick on the left for moving and strafing, swipe on the right to turn, a big fire button (you can also swipe on it to turn while holding it), plus buttons for dodge and pause.
  - **Menus work:** The on-screen buttons can now be tapped. Before, the old control bar covered them, and a tap often ended up in the pause menu.
  - **Big picture:** In landscape the game fills the whole screen, in fullscreen where the browser allows it. In portrait a notice asks you to turn the phone while playing.
  - **On-screen keyboard** for nickname, game name, code and highscore name.
- **Keycard sound:** Picking up a keycard now plays its own sound: a card swipe, two beeps and a bright chime.
- **Online multiplayer:**
  - The game code is now shown large in its own box in the lobby, with a copy button ([Y]).
  - "Join private game" is its own button ([J]); only then does the code dialog appear.
- **Browser version:** "Quit" closes the tab if the browser allows it.
- **Version display:** The main menu now shows the game version, on the Cardputer too.

## Version 3.19.1 – Browser version fixed (2026-10-18)

- **Browser version:** The "Game server stopped" message no longer appears. The browser version wrongly looked for the app's built-in server, which does not exist in the browser.

## Version 3.19.0 – Online multiplayer and browser version (2026-10-18)

- **Online multiplayer over the internet:** Under *Multiplayer → Online play* you play with 2–4 people over the internet, in all three modes (co-op, team, deathmatch).
  - **Public games** appear in a list for everyone with the same game version.
  - **Private games** are not listed; friends join with the five-character code shown in the top right corner of the lobby.
  - Online, 10 updates per second are sent; other players and shots move smoothly in between.
  - The game server runs for free on Cloudflare and only forwards the messages; the host runs the game just like in LAN play.
- **Privacy:** Before your first online game, a notice explains what is transmitted, and the game only connects after you agree. You can withdraw your consent anytime under *Options → Online multiplayer*.
- **Browser version:** MARGIN CALL now also runs right in the browser at https://wolfgangrabenstein.github.io/margincall/, with solo and online multiplayer. Live quotes come through the MARGIN CALL server there, so Yahoo does not see your IP address.

## Version 3.18.2 – Deck chair in a new color (2026-10-18)

- **Closing scene:** The deck chair now has yellow-orange and white stripes instead of blue, so it stands out better against the blue suit.

## Version 3.18.1 – New closing text, cocktail fix (2026-10-18)

- **Credits:** The closing text now reads "The market is free again. But where to put all that money now …? Off to tax haven!"
- **Closing scene:** When drinking, the cocktail glass no longer ends up inside the head. It stays in front of the chin, and the straw tilts toward the mouth.

## Version 3.18.0 – Keycards, randomized levels, visible shots (2026-10-18)

- **Randomized levels:** Every time you start a game, the enemies in each level (except the tax haven) are placed at random, and the vault door sits at a different spot on the outer wall.
- **Keycards:** The vault only opens with the level's keycard, and each level has its own color (yellow, blue, red, green, purple).
  - In levels with a boss, the boss drops the keycard.
  - In the other levels, it drops after a random number of enemies have been taken out.
  - Without the keycard, the door tells you which color you need. Once you have it, the HUD shows it.
- **Visible shots:** The gun fires flying banknotes. Damage only happens when a note hits, and walls stop it.
- **Gun reworked:** The barrel now sits on a chrome mount in the briefcase, with brass rings and a feed from the banknote magazine.
- **Pirates guard the treasure:** Most pirates stay at the coin piles. If you attack one or get too close, it and its mates fight back, but they only chase you up to a certain distance and then zigzag back to the treasure. Only a few hunt you directly.
- **Pause menu:** Esc resumes the game, Q goes to the main menu.
- **Quitting:** Esc in the main menu now asks whether you really want to quit. Enter quits, Esc cancels.

## Version 3.17.3 – Ammo on the tax haven (2026-10-18)

- **Cash on the island:** The tax haven now also has a few cash bundles lying around as ammo, 3 at the start (Hardcore 2). Every 10 to 14 seconds (Hardcore 14 to 18) a new one appears, as long as fewer than 2 are lying around.

## Version 3.17.2 – Unique music for every level, volume controls (2026-10-16)

- **Every level has its own music**, using the same chiptune instruments as before so everything fits together:
  - **Dow Jones (banking hall):** festive march in C major
  - **S&P 500 (trading floor):** frantic, in E minor
  - **Nasdaq-100 (vault):** mysterious, in D minor
  - **Oil (refinery):** heavy and stomping, in G minor
  - **ALGO-9000 (data center):** electronic with arpeggios, in F-sharp minor
  - **Tax haven:** sunny calypso tune in F major with cowbell
  - The title screen and credits keep the familiar title song.
- **Bug fixed:** After a game, the music sometimes played much too fast because the more frantic low-portfolio tempo got stuck.
- **Volume controls:** In the options, music and sound effects can now be adjusted in ten steps instead of just switching music on or off. The M key still mutes the music and then restores the previous volume.
- **Leaderboards reset:** The online leaderboard starts over.

## Version 3.17.1 – Hit feedback, smarter pirates, new weapon (2026-10-16)

- **Closing scene reworked:**
  - The meme sunglasses now stay on the whole time and are no longer hidden by the head.
  - It is the same trader as in the HUD: brown hair, blue suit, white collar, red tie, barefoot in the deck chair.
  - The head looks more realistic and matches the size of the glasses: wide face, narrower chin, rounded back of the head, nose, ears, hair with bangs, and a neck.
  - The sly grin with one raised corner of the mouth is clearly visible.
  - The scene is lit more brightly, and the camera ends head-on at eye level.
- **More and smarter pirates in the tax haven:**
  - 8 instead of 5 at the start (Hardcore 10), up to 12 at once (Hardcore 14).
  - They are a bit faster than you, but need a moment after spawning before they charge.
  - Even when you are surrounded, a sabre hits you at most once every 0.5 seconds (Hardcore 0.35 s).
  - Three roles:
    - **Guards** lurk by a pile of coins and only attack when you get close.
    - **Interceptors** run to where you are about to be.
    - **Hunters** chase you directly.
- **New weapon:** a cash-case cannon, a briefcase with a chrome barrel, bundles of banknotes as the magazine and a small price display, held by a trader's hands in a suit.
- **New dividend icon:** a stack of coins with a big green arrow pointing up, easier to recognize than the old coin.

- **You now feel hits:**
  - **Red screen flash:** The whole screen briefly turns red; the more damage, the stronger.
  - **Shake:** The screen shakes briefly.
  - **Price crash:** The damage falls across the screen as "▼ −12%".
- **Trader face in the HUD:**
  - grins with a full portfolio, sweats with a medium one, panics when the portfolio is low
  - grimaces when hit and glances to the side now and then
  - has X eyes when you are broke
- **Cardputer:** red screen flash and damage number.
- **Bilingual GitHub page:** README in English (main page) and German (`README.de.md`), switchable at the top.

## Version 3.17.0 – "Deal with it" and a brutally hard tax haven (2026-10-16)

- **Meme sunglasses in the closing scene:** The player gets the well-known "Deal with it" pixel glasses, black blocks with white highlight pixels, on a larger, boxy head that matches the glasses exactly, with a lopsided grin. Just like in the meme, they float down onto his face from above, shortly before "EARLY RETIREMENT." appears.
- **Tax haven much harder:**
  - Only 45 instead of 75 seconds.
  - 42 instead of 30 gold coins.
  - The pirates can withstand 60% more and are 15% faster.
  - Collecting all the coins is now almost impossible.

## Version 3.16.1 – Online leaderboard with one keypress (2026-10-16)

- **Submitting online is now preselected.** After a run, **"[ENTER] SAVE & POST ONLINE"** posts your name and score to the online leaderboard. Still, nothing is sent automatically: every upload requires this explicitly labeled button.
- **Local only:** Press **TAB** to save locally only, or **ESC** to not save at all. You can change this permanently under *Options → Online leaderboard*. If you had already turned off the online leaderboard before, you stay on "local only".
- The privacy notice on first launch, the README and the privacy policy have been updated, including the legal basis, objection and deletion.

## Version 3.16.0 – Pirates, closing scene, bigger tax haven, visible timer (2026-10-16)

- **Pirates instead of bears in the tax haven:** three types that differ only in appearance:
  - **Captain:** red coat, beard, tricorn hat with a skull
  - **Sailor:** striped shirt, headscarf, eye patch
  - **Buccaneer:** blue vest, earring, hook
  They raise their sabres when attacking.
- **Closing scene:** The tax haven is followed by a short scene rendered in real time. The camera moves toward your yacht at sunset: you are lying in a deck chair with sunglasses, sipping cocktails. Skippable with Enter, on the Cardputer too.
- **Tax haven bigger and harder:**
  - The island is about a quarter larger.
  - 30 instead of 20 gold coins.
  - 5 instead of 3 pirates at the start, and they respawn faster: up to 8, in Hardcore 10.
  - Only 75 instead of 90 seconds, making it almost impossible to collect all the coins.
- **Timer clearly visible:** The remaining time is shown large in the top right of the screen, with the coin count below it. In the last 15 seconds it turns red and blinks, on the Cardputer too.
- **Hardcore a bit harder still:** Enemies have 1.8 times instead of 1.6 times the health, fire 45% instead of 35% more often, their projectiles are 30% instead of 25% faster, and you take 1.65 times instead of 1.5 times the damage.
- **Names up to 15 characters:** for high scores (local and online) and nicknames, previously 12.
- **Game name only when hosting:** The multiplayer menu no longer has a game name field. The host names their game in the lobby with **[G]**.

## Version 3.15.0 – English by default, German optional (2026-10-16)

- **The game is now in English by default.** This applies to menus, HUD, messages, enemy and level names, credits, leaderboards and multiplayer. Numbers appear in English format (e.g. 52,178 and +1.08 %).
- **German can still be selected:**
  - PC/desktop: under *Options → Language*
  - Cardputer: with **L** on the title screen
  - The choice is saved.
- Minor improvements.

## Version 3.14.0 (2026-10-16)

- Minor improvements.

## Version 3.13.0 – Credits and bonus level "Tax Haven" (2026-10-16)

- **Short credits after the final boss:** After defeating ALGO-9000 and the bull market bonus, credits of about 15 seconds roll. They can be skipped with Enter.
- **Bonus level "Tax Haven":** Then it's off to a tropical island in the open air, with all-new low-poly graphics instead of dark corridors.
  - **Graphics:** sunset, sandy beach, turquoise shallows, glittering sea, palm trees, a beach bar, a luxury yacht off the coast and spinning gold coins. A small custom 3D renderer draws flat-shaded triangles with sunlight and evening haze.
  - **Goal:** 90 seconds to collect 20 gold coins while "tax investigator bears" hunt you. Each coin is worth +$500, all 20 together an extra +$10,000, each 1.5 times in Hardcore.
  - **No risk to your run:** If you get caught or time runs out, only the bonus level ends, and you keep the gold you collected. The high score follows as usual.
  - **Start:** You get a full portfolio and +20 cash.
- **Cardputer:** Credits and tax haven also run on the Cardputer, with 10 tiles of view distance and leaner geometry.

## Version 3.12.0 – Bull market bonus for beating the game (2026-10-16)

- **If you defeat ALGO-9000 and beat the game, you get a bull market bonus on your high score:**
  - **+$10,000** flat
  - **+25%** of the P&L earned up to that point
  - **+$5,000** for a clean record, i.e. without a single death
  - In Hardcore, the bonus counts 1.5 times, like all points.
- Example: With $20,900 P&L and no deaths you get +$20,225, in Hardcore +$30,506.
- The victory screen shows the bonus, and the run is still listed with "WON" in the leaderboard. Previously, the last vault only gave the same small bonus as every other level.
- On the Cardputer too, which now counts the deaths in a run for this.

## Version 3.11.0 – Hardcore mode, fairer bosses (2026-10-16)

- **Hardcore mode:** Start it from the main menu with **X** (or the red button), solo play only.
  - **One death ends the run:** Instead of "New loan" you get **GAME OVER**, and then your score is entered.
  - **Tougher enemies:** 60% more health, they fire 35% more often, their projectiles fly 25% faster, and they deal 50% more damage. ALGO-9000 attacks more often.
  - **Scarce supplies:** Start with 20 instead of 30 cash, no refill between levels, 40% less loot. Cash gives +7 instead of +10, a dividend +15% instead of +20%, and scattered cash lies around less often.
  - **Reward:** All points count 1.5 times.
  - **Separate leaderboards:** Hardcore has its own local top 10 and its own online lists (all-time and today). In the high score menu, press **X** to switch between Normal and Hardcore.
- **Bosses can no longer be cheesed from behind pedestals:** If you stood half behind an edge, you could still hit the wide boss, but it didn't shoot back at all because it only checked its center line. Now bosses and all ranged enemies check their full width and shoot from the open side.
- **Defeated enemies disappear after 7 instead of 25 seconds.**
- **Esc goes to the main menu first:** From the pause and multiplayer menus, Esc takes you to the main menu; the game can only be quit completely from there.
- **Cardputer:** Hardcore (X on the title screen), the boss fix and the 7 seconds are available there too; there are no leaderboards on the Cardputer.

## Version 3.10.1 – Online leaderboard is live (2026-10-16)

- **The online leaderboard is now enabled.** After a solo run, choose "Also submit online" with **Tab**. In the high score menu (**H**) you'll then see the worldwide all-time and daily leaderboards with your rank under [2] and [3].
- Server: https://margincall-highscores.margincall.workers.dev (Cloudflare Worker + D1).

## Version 3.10.0 – Online leaderboard prepared, calmer lighting (2026-10-16)

- **The lights now only flicker during the ALGO attack:** Previously they flickered several times per second on every strong price movement and throughout the whole manipulation. Now they only flicker for the first 0.7 seconds when ALGO-9000 strikes (also when Pump & Dump flips). In multiplayer they flicker for everyone at the same time.
- **Online leaderboard (starts as soon as the server is running):** In addition to the local top 10, there is a worldwide leaderboard.
  - **Tabs in the high score menu:** [1] Local, [2] Online all-time (top 100), [3] Online today. Switch with 1–3 or ←/→, scroll with ↑/↓.
  - **Your own rank:** After submitting, the list jumps to your entry and shows e.g. "YOUR RANK: 23 OF 41", even outside the top 100.
  - **Optional:** After each run, press **Tab** to turn on "Also submit online". The setting is remembered and can also be changed under *Options → Online leaderboard*. Only name, score, level, victory, deaths and game version are sent.
  - **Server:** a Cloudflare Worker in the `worker/` folder. As long as it isn't set up, everything online stays hidden in the game. Instructions: [worker/README.md](worker/README.md).

## Version 3.9.0 – ALGO-9000 manipulates the price, individual stocks in the ticker (2026-10-16)

- **ALGO-9000 shifts the in-game price:** Each attack adds a markup or markdown to the level's real price for 15–24 seconds. Everything tied to the price reacts immediately: the ratio of bulls to bears, their strength and, in team mode, the damage bonus. Shortly before the end, the manipulation slowly fades out.
  - **Flash Crash:** −5%
  - **Pump & Dump:** first +4%, after 12 seconds the price flips to −4%
  - **Fake News:** ±2–4%
  - **System manipulated:** ±4%
  - **Trading Bots:** −1.5 to −2.5%
  - **Bulls & bears stirred up:** ±3% in the direction of the real daily trend
- **Manipulation visible:** The HUD shows the real and manipulated prices side by side, e.g. "DOW +1.08% → ALGO −3.92%", blinking in magenta. Wall charts and the ticker tape get a magenta line with the distorted curve. The ticker shows the ALGO price for the level's index.
- **Lights flicker on price swings:** During strong real intraday movements and during manipulation, the lights flicker briefly.
- **Individual stocks in the ticker:** Each index is followed by its best-known stocks with price and daily change, green when rising, red when falling. For the Dow these are e.g. Goldman Sachs, Caterpillar and Boeing, for the Nasdaq-100 Nvidia, Microsoft and Tesla, for oil Exxon and Chevron. The individual stocks are display only; the game is still based solely on the benchmark indices.
- **Ammo and health a bit scarcer:**
  - Enemies drop something less often: 38% instead of 45%, in fights between animals 22% instead of 30%.
  - A dividend heals 20% instead of 25%.
  - In return, **occasional cash** lies around off the beaten path, roughly every one to two minutes, at most two bills at a time. If you're almost out of ammo, you'll find some faster.
- **Cardputer:** All new features are also available on the Cardputer.

## Version 3.8.1 – Tougher co-op (2026-10-16)

- **Enemies scale more strongly with the number of players:**
  - **+60% health** per additional player, previously +35%.
  - **3 additional enemies per teammate** at the start of a level, matching the level: bulls and bears according to market conditions, also oil slicks and oil barons in the oil level, trading bots in the AI level.
  - **More and faster reinforcements:** up to 4 more enemies at once per teammate, about 50% faster per teammate.
- **Example with 2 players:** 12 instead of 9 enemies at the start, a bear takes 10 instead of 6 hits.

## Version 3.8.0 – Nicknames, game names, round break, Tab stats (2026-10-16)

- **Nickname for multiplayer:** Can be changed in the multiplayer menu and in the lobby with **[K]** (or a click), up to 12 characters.
  - The nickname appears in the lobby, chat, name tags, messages and stats.
  - It is saved and carried over when joining.
- **Game name:** The host names their game with **[G]**, e.g. "Stock Market Party".
  - The name appears in the lobby below the title.
  - The game name and host appear in the game list under "Join".
- **Round break in Deathmatch and team mode:** After a round ends, there is a **5-second break** with the stats of all players and a countdown "Next level in 5 s". Then the next level starts automatically; the host can continue immediately with Enter.
- **In-game stats:** While you hold **Tab**, you see a table of all players with P&L, frags, deaths and kills.
  - In solo mode it shows your run with P&L, level, kills, deaths and market cleanup.
  - The key can be bound in the options menu.
- **Ready with E:** Teammates mark themselves ready in the lobby with **E**. Enter and Space no longer change the ready status.
- **Fixed:**
  - The portfolio never shows "0%" anymore: when you are knocked out, it shows **BROKE** until you rejoin.
  - Names and stats overlapped in the round summary.
  - The hint for continuing play overlapped the credits.

## Version 3.7.0 – Game browser, lobby chat, ready button, Deathmatch stats (2026-10-16)

- **Find games on the network:** **[N] Multiplayer → [J] Join game** now automatically shows all open games on the Wi-Fi, with host, mode and player count.
  - A click or Enter is enough to join; you no longer have to type in the IP address and port.
  - Manual address entry remains as a fallback.
  - A game is announced as long as the host is in the lobby.
- **Ready button:** Teammates confirm in the lobby with **"Ready"**. The host can only start once everyone is ready. The player list shows ✔ READY or ✖ WAITING.
- **Lobby chat:** **[T]** or a click in the chat field opens the input, Enter sends.
- **In-game chat:** **[T]** opens a chat window at the top. Sent messages appear for everyone as a message at the top of the ticker, e.g. "» BLUE: gg wp". While you are typing, your character stands still.
  - The chat key can be freely bound in the options menu.
- **Deathmatch stats:** The HUD shows your **frags** (teammates shot down), **deaths** and **kills** (stock market enemies defeated), plus a ranking of all players with frags and deaths. The round summary shows the same values.
- **Fullscreen:**
  - New line **"Fullscreen (F4)"** in the options menu.
  - The setting is saved, and the app then starts directly in fullscreen.
  - The main menu shows **[F4] FULLSCREEN** in the bottom left, which is also clickable.
- **Lobby layout reorganized:** players on the left, chat in the middle, your own character on the right.
- **Team colors in team mode:** Players are named after their team's color ("TRADER GREEN" for Team Bull, "TRADER RED" for Team Bear) and appear in that color.
  - This applies in the lobby (with "TEAM BULL · GREEN" / "TEAM BEAR · RED"), on the in-game name tags and in the summary.
  - Teammates are numbered, e.g. "GREEN 1" and "GREEN 2".

## Version 3.6.2 – Deaths count, mouse active immediately, bear roar (2026-10-15)

- **Deaths in single player:**
  - Every death is counted and costs **25% of your P&L, at least $500**. In exchange, the previous halving on "New loan" is gone.
  - The bankruptcy screen shows which bankruptcy this is and how high the penalty was.
  - The HUD shows a ☠ with the number of deaths next to the P&L.
  - The high score list has a new **Deaths** column.
- **Mouse control active immediately:** At game start, at every new level, after "New loan" and after pausing, you no longer have to click into the screen first.
  - In multiplayer this applies to the host. Teammates see a hint "Click the screen for mouse control", because the browser only allows capturing the mouse there after a click of their own.
- **The bear roars:**
  - when it spots you on the hunt,
  - when it strikes,
  - and in between, like the bull when charging.

  This applies on the PC and on the Cardputer.

## Version 3.6.1 – Solo button in the start menu (2026-10-15)

- **Dedicated "▶ PLAY SOLO" button:** The large, gently pulsing button in the start menu starts single player with a click or Enter.
- **No more accidental starts:** Clicking anywhere on the title screen no longer starts a game.
- **Shorter status display:** When live prices are turned off, it now just says "SIMULATED". The long text stuck out over the edge of the HUD.

## Version 3.6.0 – High score list (2026-10-15)

- **High score list for single player:** It shows the best 10 runs with name, P&L, level reached or "WON", and date.
  - On the title screen, open it via **[H] HIGHSCORES**, by key or click.
- **Entering names:** If you make the top 10 after a victory, you enter your name (up to 12 characters). The most recently used name is prefilled. The new entry blinks in the list.
- **Give up after bankruptcy:** After a bankruptcy you now have a choice:
  - **[ENTER] New loan:** Keep playing with your P&L halved, as before.
  - **[H] Give up & high score:** The run ends, and you can enter your name.
- **Storage location:** The list is stored in a file in your user folder, so it is kept across updates.
  - Windows: `%APPDATA%\MarginCall\highscores.json`
  - macOS: `~/Library/Application Support/MarginCall/`
  - Linux: `~/.local/share/MarginCall/`
- **Write protection:** Teammates on the Wi-Fi cannot change your list. The app only accepts entries from your own computer.

## Version 3.5.3 – Menus with the mouse (2026-10-15)

- **Everything can be operated with mouse clicks:**
  - Multiplayer: **Host game**, **Join game**, **Connect**, **Back**
  - Lobby: game mode via the arrows **◀ ▶**, **character selection** by clicking the character or "Change", **Team Bull** / **Team Bear**, **Start game**, **Leave**
  - Pause menu (**Resume**, **Options**, **Quit**), in-game multiplayer menu and privacy notice
- **Highlighting:** Buttons light up when the mouse is over them, and the cursor turns into a hand.
- **Keyboard:** All keyboard shortcuts still work.

## Version 3.5.2 – Music in the options (2026-10-15)

- **New "Music" toggle** in the options menu **[O]**: It turns the music on and off.
- **Saved:** The setting is kept even after a restart, including when you toggle the music with the **M** key.

## Version 3.5.1 – Privacy and open source (2026-10-15)

- **Live prices only with consent:** On first launch, a notice explains that prices are fetched from Yahoo Finance. Press **Enter** to agree, or **N** to play with simulated prices.
- **New "Live prices" toggle** in the options menu, changeable at any time.
- **No more automatic fetching:** The app no longer fetches anything on its own at startup, only when the game requests prices.
- **Open source:** MARGIN CALL is now under the **MIT license**. Also new are a code signing policy with a privacy policy and a Code of Conduct.
- **Preparation for Windows signing** via the SignPath Foundation: The build script submits the EXE for signing as soon as the SignPath credentials are configured.

## Version 3.5.0 – Player characters, Esc menu and fullscreen (2026-10-15)

- **Two player characters in multiplayer:** In the lobby, press **[C]** to switch between male and female. Your character is shown large on the right side of the lobby.
  - **Male:** hoodie in your player color with a "$" print, gold chain, baseball cap, headset, beard, jeans and sneakers.
  - **Female:** long red ponytail, dress in your player color with a gold belt, gold hoop earrings, sunglasses in her hair and boots.
  - Both characters deliberately look completely different from the suited enemies.
  - In team mode they wear the team color, plus horns (Team Bull) or bear ears (Team Bear).
  - The choice is saved and carried over when joining a host.
- **Esc pauses the game.**
  - **Esc in the pause menu quits the game.** So that a single Esc press doesn't quit right away, the menu only responds after a short delay.
  - **P** pauses and resumes as before.
  - With mouse control, one Esc is enough: the first press releases the mouse and pauses at the same time.
  - In multiplayer, Esc and P open the multiplayer menu. Esc there quits, **[L]** leaves the game, **P** goes back.
  - On the title screen, **Esc** quits the game.
- **Q no longer quits the game.**
- **F4** toggles fullscreen mode on and off.
- **Version number in the file name:** The downloads are now named e.g. `MarginCall-3.5.0-Windows.exe`, `MarginCall-3.5.0-macOS.dmg`, `MarginCall-3.5.0-Linux.AppImage` and `MarginCall-3.5.0-Cardputer.bin`.

## Version 3.4.1 – WASD on PC (2026-10-15)

- **New default controls on PC** following the usual shooter layout:

  | Action | Keys |
  |---|---|
  | Forward / Backward | W / S (also ↑ / ↓) |
  | Strafe | A / D |
  | Turn | Mouse or ← / → |
  | Fire | Space or left mouse button |

- **Dodging** by double-tap works with the new keys: tap W, S, A or D twice quickly.
- **Saved bindings:** The key bindings are reset once to the new defaults. Custom bindings can still be set in the options menu **[O]**.
- **Touch buttons** on mobile: top A ▲ D, bottom turn ⟲ ▼ ⟳.
- The **Cardputer** stays with **E/S to move, A/D to turn, W/R to strafe**.

## Version 3.4 – Standalone app for Windows, macOS and Linux (2026-10-15)

- **No more browser:** MARGIN CALL is now a standalone app with its own window. There is one file per system:

  | System | File |
  |---|---|
  | Windows | `MarginCall-Windows.exe` |
  | macOS | `MarginCall-macOS.dmg` (Intel and Apple Silicon) |
  | Linux | `MarginCall-Linux.AppImage` |

  Live prices, music, sounds and multiplayer are built in; no additional software is needed.
- **Quitting:** The game can now be quit, via **[Q] Quit** on the title screen, in the pause menu and in the multiplayer menu. Closing the window also quits the app.
- **Multiplayer menu:** **[N] Multiplayer** now offers a choice between **[H] Host game** and **[J] Join game**.
  - When joining, you enter the host's address, which is shown in their lobby.
  - Your key bindings are carried over.
  - If the host's game ends, **[H]** takes you back to your own game.
  - Teammates in the browser still work.
- **In multiplayer**, the pause key opens a small menu with **[L] Leave game** and **[Q] Quit**. The game keeps running meanwhile.
- **Credits:** "Made by Wolfgang Rabenstein" appears on the title screen and on the end screens, on the PC and on the Cardputer.

## Version 3.3 – Dodging (2026-10-15)

- **Dodge by double-tap:** If you tap a movement direction twice in quick succession (within 0.25 s), your character leaps a short distance in that direction.
- This works **forward, backward, left and right**.
  - On PC, the keys from the options menu count, including the arrow keys.
  - On the Cardputer, it's E, S, W and R.
- A leap carries you about 1.75 tiles in a quarter of a second; after that you can't dodge again for 0.6 s.
- If you are stuck in oil, the leap is only half as far.
- When dodging, you hear a whoosh sound and see motion streaks at the edge of the screen.
- Dodging also works in multiplayer.

## Version 3.2 – Music pauses too (2026-10-15)

- **Pause:** When you pause the game, the music pauses too, on the PC and on the Cardputer. When you resume, it continues from the same spot.
- **PC, tab in the background:** If you switch to another browser tab or minimize the window, music and sounds go silent until you come back.
- **PC, game server closed:** If `MarginCall.exe` is closed while the game is still open in the browser, the game stops after a few seconds and shows "GAME SERVER STOPPED". Previously, the music kept playing in the browser in this case. If you restart the EXE, the game continues.

## Version 3.1 – PC version: options and multiplayer (2026-10-15)

This update only affects `MarginCall.exe`. The Cardputer version stays at 3.0.

### New: Options menu with keyboard and mouse bindings

- Open the options menu with **[O]** on the title screen, while paused or in the lobby.
- Every action has **two freely assignable bindings**. Each can be a key or a mouse button (left, middle, right, mouse 4/5).
- Bindable actions:
  - Forward and backward
  - Turn left and right
  - Strafe left and right
  - Fire
  - Pause
  - Music
- If a key is already bound, it is automatically removed from the old action. This prevents duplicate bindings.
- Mouse:
  - Mouse control on/off
  - Sensitivity in 10 steps
  - Invert
- **Del** clears a binding. **Restore defaults** resets everything.
- The settings are saved in the browser and still apply the next time you start.

### New: Multiplayer over Wi-Fi (2–4 players)

- On the title screen, **[N]** opens the lobby.
  - The first player in the lobby is the host. They choose the mode with ←/→ and start with Enter.
  - Teammates on the same Wi-Fi open the address from the console window (`http://<IP>:8080`) and also press **[N]**.
- **Co-op:**
  - You play all 5 levels together. With each additional teammate, enemies get tougher and more reinforcements arrive.
  - Anyone who dies rejoins at the level start after 6 seconds, with 60% portfolio value and 10% less P&L.
- **Team Bull vs. Team Bear:**
  - Choose Team Bull with **[1]** or Team Bear with **[2]**.
  - The animals of your faction don't attack you and can't be hit by you either. They fight the other team.
  - If the level's market is up today, Team Bull deals 25% more damage. If it is down, the same applies to Team Bear.
  - A round lasts 4 minutes or until a team has 3,000 points.
- **Deathmatch:**
  - Everyone against everyone, and the stock market enemies still join in. A kill earns 500 P&L.
  - A round lasts 4 minutes or until someone has 3,000 P&L.
- There are no bosses in team and Deathmatch. After each round, play continues in the next level.
- Teammates appear as traders in colored suits (green, blue, orange, pink) with a name tag and health bar. In team mode, Team Bull wears golden horns and Team Bear wears bear ears.
- There is no pause in multiplayer.
- If the host leaves the game, everyone ends up back on the title screen.

## Version 3 – "Bulls vs. Bears" (2026-10-15)

Version 3 connects the game to the real stock market. Bulls and bears now fight not only against you, but also against each other. The real market of the day decides how strong each side is.

### At a glance: Version 2 and Version 3

| | Version 2 | Version 3 |
|---|---|---|
| Levels | 3 | **5** (new: oil refinery, AI data center) |
| Enemy types | 4 | **9** |
| Market data | none, all made up | **real daily prices** of the Dow Jones, S&P 500, Nasdaq-100 and crude oil (WTI) |
| Bulls and bears | bears only | **both**, and they fight each other |
| Enemy reinforcements | fixed number of enemies per level | **continuous reinforcements** depending on market conditions |
| AI opponent | – | **ALGO-9000** disrupts the game and is the final boss |
| Wi-Fi | – | **Wi-Fi menu** right in the game |
| Spectating | – | **screen stream** in the browser for the whole network |
| PC version | – | **MarginCall.exe** for Windows with live prices |
| Sound effects | 11 | **16** |

### New: Real stock prices

- If the Cardputer is on Wi-Fi, the game loads the current prices of the **Dow Jones, S&P 500, Nasdaq-100 and crude oil (WTI)** at startup.
- Without Wi-Fi, the prices are simulated. In the game they are then marked with `SIM` or `*`.
- Each level is tied to a market:
  - Level 1: Dow Jones
  - Level 2: S&P 500
  - Level 3: Nasdaq-100
  - Level 4: Oil
  - Level 5: average of the three indices
- **Balance of power:** If the index is up today, more and stronger bulls show up. If it is down, the same applies to the bears.
- **Intraday curve:** The real daily chart runs along in the background. Its short-term direction determines whether a bull or a bear spawns next.
- **Oil price:**
  - If oil rises, all enemies fire faster ("energy crisis").
  - If oil falls, you move faster.
- **Displays:**
  - A ticker tape at the top of the screen shows all four prices.
  - The scrolling-text walls show the real daily curve.
  - The chart boards show real candlesticks.
- Before each new level, the prices are updated if the last fetch is older than 2 minutes.

### New: Bulls vs. bears

- **The bull** is a new enemy with golden horns. It charges at its target with its head lowered and rams it.
- Bulls and bears attack you, **but hate each other even more**. When they meet, they go after each other first.
- You can play them off against each other. Enemies defeated by others count as "market cleanup" at the end of the level.
- Short sellers fight for the bears. Crypto bubbles fight for the bulls.
- A new bar in the HUD shows the current balance of power: green is the bulls, red the bears.

### New: ALGO-9000, the manipulating AI

From level 1 on, ALGO-9000 regularly interferes with the market. In the last level this happens much more often. Each intervention is accompanied by a red AI eye and static noise. These are the interventions:

- **Fake News:** The ticker tape shows false prices, and additional enemies appear.
- **Flash Crash:** All bulls and bears attack only you, and faster.
- **Trading Bots:** Flying drones shoot lasers at you.
- **Pump & Dump:** A swarm of crypto bubbles appears.
- **System manipulation:** The screen flickers, and for a few seconds turning and strafing are swapped.
- **Stirring up:** Bulls and bears get stronger.

In **level 5 (data center)**, ALGO-9000 is the **final boss**:
- It fires laser fans and keeps summoning bots.
- At half health it triggers a Flash Crash.
- The vault to the exit only opens once ALGO-9000 has been shut down.

### New: Oil level and oil enemies

- **Level 4 "Refinery"** takes place among pipelines and oil barrels.
- **Oil baron:** wears a top hat, monocle and cigar and throws oil barrels. A hit slows you down briefly.
- **Oil slick:** a black, glossy monster. If it catches you, you are stuck for 3 seconds ("oil spill").

### New: Wi-Fi and screen stream

- On the title screen, **[W]** opens the Wi-Fi menu:
  1. Select a network with `;` and `.`.
  2. Type the password. **Tab** shows or hides it.
  3. Connect with Enter.
- The credentials are saved on the Cardputer. From the next start on, it connects automatically.
- **[F]** in the Wi-Fi menu deletes the saved credentials.
- **Stream:** Anyone on the same network can watch in the browser, at `http://margincall.local` or the IP address shown on the title screen and in the pause menu.
  - Viewers see the game live with the current prices below it.
  - When someone is watching, a red dot lights up in the HUD.
  - The Wi-Fi menu, and therefore your password, is **never** transmitted.

### New: PC version (`MarginCall.exe`)

- A double-click starts the game in the browser, with real prices updated every minute.
- The game logic is the same as on the Cardputer, just at a higher resolution.
- Controls:
  - Keyboard as on the Cardputer
  - Plus the arrow keys
  - Mouse control: click into the screen, then the mouse turns the view and left-click fires
- Other devices on the same Wi-Fi can join in via the displayed address (`http://<IP>:8080`), even on a phone with touch controls.
- Windows shows a warning on first launch because the file is not signed. In that case, click "More info" and "Run anyway".

### New sounds

| Sound | When |
|---|---|
| Bull roar | A bull charges or hits |
| Bear growl | A bear strikes |
| Data screech | ALGO-9000 intervenes |
| Oil gurgle | Oil baron throws, oil slick hits |
| Laser | Trading bots and ALGO-9000 fire |

All sounds from Version 2 are still included, plus the music, which speeds up when your portfolio value is low.

### Changed

- **Damage:** Your bull blaster now deals 2 damage per hit, because the enemies have become tougher.
- **End of game:** At the end of a level and on Game Over, your own kills and the market cleanup are listed separately.
- **Title screen:** It shows all four prices with their daily change and the LIVE or SIM indicator, plus the Wi-Fi and stream status.
- **HUD:** New are the bulls-vs-bears power bar and the daily change of the level's market.
- **Victory screen:** It is now called "ALGO-9000 defeated – the market is free".

### Unchanged

- Controls:
  - E/S forward/backward
  - A/D turn
  - W/R strafe
  - Space/Enter fire
  - P pause
  - M music
- Installation via the M5Launcher: copy `MarginCall.bin` to the SD card and install it via **SD → Install**.

### Known limitations

- **Price source:** The prices come from Yahoo Finance. If the source goes down or its format changes, the game automatically continues with simulated prices.
- **Stream:** Depending on Wi-Fi quality, it shows only a few frames per second and has less color depth than the display.
- **Memory:** The Cardputer version uses almost all of the available memory. If something goes wrong while fetching prices, the loading screen shows "ERROR – SIMULATED", and the game continues with simulated prices.
