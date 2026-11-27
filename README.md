# NX Title Editor (on-console version)

A homebrew app (`.nro`) that runs a small web server on your Switch while it's
open. Launch it from the Homebrew Menu, scan the QR code on screen with your
phone (or type the address on a PC), and change game icons, names,
publishers and version numbers from the browser.
Close it with **+** and nothing is left running.

By ImmortalEye.

**Needs an icon sysmodule** on the Switch, such as
[sys-ticon](https://github.com/masagrator/sys-ticon) or
[sys-tweak](https://github.com/p-sam/switch-sys-tweak): it's what shows the
custom icons and details. This app writes the files it reads.

**What it does (v1.0.0):**
- Lists every installed game (retail, eShop and custom NSPs). Cards show the
  current art, a `custom icon` / `stock icon` pill, and a house and grid icon
  that light up when the **Home Screen** or **All Software** icon is custom.
- The pinned header has the **Home Screen / All Software** switch for which
  icon the cards show, a search box with a **Rescan** button, and filters:
  **All**, plus house and grid toggles that match the card indicators. From
  All, tapping one shows games where only that icon is custom; with both on
  you see games where both are custom; with both off, stock games.
- The cog opens **Settings** and **?** opens **About**, which explains how
  the app and icon sysmodules work together.
- Tap a game to open its editor. It shows the **Home Screen** and **All
  Software** icons side by side, each with a tag (custom, stock, new,
  remove), an eye button to view it large, **Remove** and **Copy** to the
  other side. Changes are staged until you press **Save**, and leaving with
  unsaved changes asks first.
- The pencil next to the name opens **Edit Details**: **Name**, **Publisher**
  and **Version**. Boxes start empty with the game's own values shown in
  grey; anything left empty keeps the original. Byte limits are checked as
  you type. sys-ticon only uses a name and publisher together, so if you
  change one the app writes the original for the other. A **custom** tag
  shows next to the pencil once a game has saved details.
- The trash button (top right) **resets the game to defaults**: after a
  confirmation it stages removal of the custom icons and details, applied
  when you press **Save** like any other change.
- The **Icon Picker** below has three tabs: **SteamGridDB** (search with
  suggestions, then pick a **Dimensions** option: 1:1 Square, 22:31 Galaxy
  2.0, 92:43 Steam Horizontal or 2:3 Steam Vertical), **Upload** from your
  phone/PC, or paste a **Link**.
- Icons are shown stretched to each screen's shape (set in Settings to match
  your theme). Files on the SD card are always the square 256x256 `icon.jpg`
  and 174x174 `icon174.jpg` that sys-ticon expects, kept under its size
  limits (100 KB and 64 KB). Details go in that folder's `config.ini`, in an
  `[override_nacp]` section at the top; anything else in the file is left
  alone.
- The page follows the Switch's own system look: dark (charcoal, mint) or
  light (soft grey, blue), with the pulsing selection frame. **App Theme** in
  Settings picks Auto (follows your device, dark if it can't tell), Light or
  Dark; it's saved in each browser, not on the Switch. The browser tab icon is
  the same badge as the page header and follows the theme too.
- **Save** writes straight to `atmosphere/contents/<TitleID>/`. A bar at the
  bottom counts your changes, then shows **Reboot Required** with a **Reboot
  Switch** button (it asks to confirm first). The Home Screen needs the
  reboot; All Software shows changes straight away.

Your browser does all the resizing, so the Switch only receives the finished
small JPEGs. SteamGridDB requests and links go through the Switch, because
browsers block those sites from being read directly.

**On the Switch screen:** a QR code with the page address under it (and a reminder that your
phone or PC must be on the same network), the
network status, the game count (a progress bar while scanning), and an
activity table (Time, Action, Title, Item) of what was written or removed
and any errors. Item says Home, Apps (All Software), Both, or Info for
name/publisher/version changes. The status in the top-right
corner shows **Ready** in white, or the current job in mint, next to a
spinner. The spinner only moves while the app is running normally, so a
frozen spinner next to a job means it's busy with that job; a frozen
**Ready** means something went wrong.

**Buttons on the Switch:** **A** full rescan, **-** reboot (opens a
confirmation screen: press **-** again to reboot or **B** to cancel), **+**
exit.

**Game list cache:** the first launch reads every game. After that, the list
is saved on the SD card and each launch only reads newly installed games
(and drops removed ones), so startup is near instant. Press **A** on the
Switch, or **Rescan** on the web page, to re-read everything, e.g. to pick
up new version numbers after game updates.

---

## 1. Install devkitPro on Fedora (one time)

devkitPro is the toolchain that builds Switch homebrew. Paste these one at a
time into a terminal.

**Install pacman** (devkitPro uses it as its package manager) **and set up its keyring:**
```bash
sudo dnf install -y pacman git make
sudo pacman-key --init
sudo pacman-key --recv BC26F752D25B92CE272E0F44F7FD5492264BB9D0 --keyserver keyserver.ubuntu.com
sudo pacman-key --lsign BC26F752D25B92CE272E0F44F7FD5492264BB9D0
sudo pacman -U https://pkg.devkitpro.org/devkitpro-keyring.pkg.tar.zst
```
If that last command complains about an "execv" error, run
`sudo pacman-key --populate devkitpro` and carry on.

**Add devkitPro's package sources:**
```bash
sudo tee -a /etc/pacman.conf > /dev/null <<'EOF'

[dkp-libs]
Server = https://pkg.devkitpro.org/packages

[dkp-linux]
Server = https://pkg.devkitpro.org/packages/linux/$arch/
EOF
```

**Install the Switch toolchain:**
```bash
sudo pacman -Syu
sudo pacman -S switch-dev switch-curl
```
Press Enter to accept the defaults when it asks which packages to install.

**Log out and back in** (or reboot) so the `DEVKITPRO` setting takes effect.
To check, run `echo $DEVKITPRO` in a new terminal. It should print
`/opt/devkitpro`. If it prints nothing, add it yourself:
```bash
echo 'export DEVKITPRO=/opt/devkitpro' >> ~/.bashrc && source ~/.bashrc
```

## 2. Build the app

Unzip this project somewhere, open a terminal in the `nx-title-editor`
folder and run:
```bash
make
```
When it finishes you'll have **`nx-title-editor.nro`** in the folder.
(`make clean` removes build files if you ever need a fresh build.)

**No pacman?** You can build with devkitPro's container instead (Podman comes
with Fedora). It already includes everything, curl too:
```bash
podman run --rm -v "$PWD":/project:Z -w /project docker.io/devkitpro/devkita64 make
```

**If `make` says `curl-config: command not found`:** the app needs devkitPro's
curl library. Run `sudo pacman -S switch-curl` and build again.

## 3. Put it on the Switch

Copy `nx-title-editor.nro` to the SD card at:
```
/switch/nx-title-editor/nx-title-editor.nro
```
You can use your existing FTP setup for this. In Fedora's Files app, click
**Other Locations**, type `ftp://<switch-ip>:5000` in the address bar at the
bottom, then drag the file in.

**Faster while we're testing:** devkitPro includes `nxlink`, which sends and
launches an `.nro` in one step. On the Switch, open the Homebrew Menu and
press **Y** (it waits for a network connection), then on your PC run:
```bash
$DEVKITPRO/tools/bin/nxlink -a <switch-ip> nx-title-editor.nro
```

## 4. Run it

1. Open the Homebrew Menu. Launching it by **holding R while opening a game**
   gives homebrew full memory. Album mode works too for this stage.
2. Start **NX Title Editor**. It reads your games, then shows a QR code and an
   address such as `http://192.168.1.42:8420`.
3. Scan the QR code with your phone, or open the address in a browser on your
   PC. It must be on the same network as the Switch.
4. Open **Settings** in the page:
   - paste your SteamGridDB API key (free; **Get a key** links to the page).
     Only the SteamGridDB tab needs it; Upload and Link work without one. A
     saved key shows as dots; clear the box and save to remove it.
   - set **Icon Dimensions** for **Home Screen** and **All Software** to
     match your theme (a preset, or Custom as a ratio, a size in pixels or a
     decimal)
   - choose whether picking a Home Screen icon also copies it to All Software
   - optionally set a fixed **port** (otherwise it's automatic, see below)
5. Tap a game, pick art and/or edit its details with the pencil, press
   **Save**, then **Reboot Switch** in the bar at the bottom or in Settings
   (or press **-** twice on the Switch).
6. Press **+** on the Switch to quit without rebooting.

**Port:** the app uses 8420. If something else on your Switch already uses
it, it automatically tries 8421 to 8429 and the screen notes which port it
picked. The QR code always has the right address. Set a fixed port in
Settings if you want an address that never changes; it applies the next time
you open the app.

**Files it keeps** in `sdmc:/config/nx-title-editor/`: `sgdb_key.txt`,
`settings.txt`, `titles_cache.txt`, and `log.txt` / `log_old.txt`. Deleting the
folder resets everything.

**Log:** everything in the on-screen Activity table is also written in full to
`log.txt`, one line per change, e.g.
`[13:33:05] [Wrote] [Home Screen icon] DARK SOULS™: REMASTERED (010040600C5CE000)`.
Errors only say "Check log" on screen; the full message is in the file. Each
time the app starts, the last log becomes `log_old.txt` (replacing the one
before), so only two runs are kept.

The app keeps the console from dimming or sleeping while it's open, and asks
the Switch to keep the network up.

**If the network drops:** the screen shows **Waiting for network...** and the
app checks again every second. When the connection comes back it reopens the
web server on the same port and redraws the QR code and address (in case the
Switch got a new IP), so you don't need to restart the app. Just reload the
page in your browser.

## If something goes wrong

The Switch screen logs every request and every file it writes. A photo of
it plus the error shown in the page is usually enough to track a problem
down.

---

## Project layout

| Path | What it is |
|---|---|
| `source/main.cpp` | Startup, console screen, URL routes |
| `source/http.cpp/.hpp` | Minimal HTTP server polled from the main loop |
| `source/titles.cpp/.hpp` | Installed-title listing and icons via the `ns` service |
| `source/fetch.cpp/.hpp` | Outbound HTTPS via curl (SteamGridDB, links) |
| `source/storage.cpp/.hpp` | SD card files: settings and icon writes |
| `source/details.cpp/.hpp` | Custom name/publisher/version in sys-ticon's `config.ini` |
| `source/screen.cpp/.hpp` | The Switch screen: status, spinner, QR code, log |
| `source/qrcodegen.c/.h` | QR code encoder by Project Nayuki (MIT) |
| `licenses/` | Licences for the QR encoder and Tabler Icons (MIT) |
| `romfs/web/` | The web page, bundled into the `.nro` |
| `icon.jpg` | The app's icon in the Homebrew Menu |
| `Makefile` | Standard devkitPro application Makefile. `APP_VERSION` here is the one place the version is set (Homebrew Menu, Sphaira, the Switch screen and the About popup all use it) |

### API

| Route | Does |
|---|---|
| `GET /api/titles` | Installed games as JSON (with `custom_name`, `custom_publisher`, `custom_version` from `config.ini`), plus `changes` (games changed since the app started). `?refresh=1` does a full rescan |
| `GET /api/icon/<TitleID>` | The game's built-in icon (JPEG) |
| `GET /api/custom-icon/<TitleID>` | The custom `icon.jpg` on the SD card (`?size=174` for `icon174.jpg`) |
| `POST /api/custom-icon/<TitleID>?size=256\|174` | Body is a JPEG; writes `icon.jpg` / `icon174.jpg`. Returns `changes` |
| `DELETE /api/custom-icon/<TitleID>?size=256\|174\|all` | Removes custom icon file(s) |
| `POST /api/details/<TitleID>` | Form-encoded `name`, `publisher`, `version`. Name and publisher must be set together; version is optional. Writes the `[override_nacp]` section of `config.ini` |
| `DELETE /api/details/<TitleID>` | Removes that section (and the file, if nothing else is in it) |
| `GET/POST /api/settings` | Read / save settings (POST body is form-encoded: `sgdb_key`, `home_shape`, `all_shape`, `all_default`, `port`). An empty `sgdb_key` deletes the saved key |
| `GET /api/sgdb/search?q=` | SteamGridDB game search |
| `GET /api/sgdb/grids/<id>?style=square\|galaxy\|horizontal\|vertical` | SteamGridDB grids in that style's dimensions |
| `GET /api/fetch?url=` | Downloads an http(s) image for the browser |
| `POST /api/reboot` | Reboots the console |

## Credits

- Made by [ImmortalEye](https://github.com/ImmortalEye3115)
- Custom icons and details on the console: icon sysmodules such as [sys-ticon](https://github.com/masagrator/sys-ticon) by masagrator and [sys-tweak](https://github.com/p-sam/switch-sys-tweak) by p-sam (one is required on the Switch; this app only writes the files they read)
- QR codes: [QR Code generator library](https://www.nayuki.io/page/qr-code-generator-library) by Project Nayuki (MIT)
- Interface icons: [Tabler Icons](https://tabler.io/icons) (MIT)
- Art search: [SteamGridDB](https://www.steamgriddb.com/)
- Built with help from [Claude](https://www.anthropic.com/claude) by Anthropic

## Licence

NX Title Editor is free software by ImmortalEye, released under the
[GNU General Public License v3.0](LICENSE) or later. You can use, change and
share it; if you release a changed version, it must stay open source under the
same licence and keep the original credit.

The bundled QR code encoder (`source/qrcodegen.c/.h`) and Tabler Icons are
MIT-licensed; their licences are in `licenses/`.
