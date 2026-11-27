# NX Title Editor

A homebrew app (`.nro`) that runs a small web server on your Switch, so you can
change the icons, names, publishers and version numbers of your installed games
and apps from a phone or PC browser.

Launch it from the Homebrew Menu, then scan the QR code on screen or type the
address into a browser. Changes only appear on the console after a reboot,
which you can do from the app by pressing **`-`** twice, or from the web UI with
**`Reboot Switch`**. Press **`+`** at any time to exit.

By [ImmortalEye](https://github.com/ImmortalEye3115).

## Requirements

**An icon sysmodule must be running on the Switch**, such as
[sys-ticon](https://github.com/masagrator/sys-ticon) or
[sys-tweak](https://github.com/p-sam/switch-sys-tweak). This app only saves
your custom icons and details to the SD card; the sysmodule is what reads them
and shows them on the Switch.

## Screenshots

| Game list | Editor | Details Editor |
|:---:|:---:|:---:|
| <img src="https://github.com/user-attachments/assets/4147a7b4-84ba-42a8-8cc0-bbf30bce93cd" alt="Game list" width="250"> | <img src="https://github.com/user-attachments/assets/b976ef70-c4b8-44c5-b95e-9abec1b51d4a" alt="Editor" width="250"> | <img src="https://github.com/user-attachments/assets/d2802495-5892-4ff4-aa63-1180d047cfb8" alt="Details Editor" width="250"> |

| On the Switch |
|:---:|
| <img src="https://github.com/user-attachments/assets/f27a71da-4c86-4ad3-b41e-b95fd77f2116" alt="Switch screen" width="760"> |

## How to use

- Download the latest `.nro` from [Releases](../../releases), copy it to `/switch/` on your SD card and launch it from the Homebrew Menu.
- Your phone or PC must be on the **same network** as the Switch.
- To search [SteamGridDB](https://www.steamgriddb.com/) for art, get a free API
  key from your SteamGridDB account's preferences and paste it into
  **`Settings`** in the web UI. Uploading your own image or using a link works
  without a key.
- All settings and the API key are saved in `/config/nx-title-editor/`.
- Everything the app does is logged to `/config/nx-title-editor/log.txt`. If
  something goes wrong, the full error is in there (the previous run is kept as
  `log_old.txt`).

## How it works

The app hosts a web server that your browser connects to. When you pick art or
edit a game's details, the browser converts everything into the exact formats
the icon sysmodules expect, then sends the finished files to the Switch, which
saves them in the folder for that game.

Each game or app has its own folder named after its Title ID, e.g.
`/atmosphere/contents/0100E46006708000/`. The sysmodules look for three files
there:

### `icon.jpg`
A 256×256 JPEG used as the game's icon across most of the Switch's interface,
including the Home Screen.

### `icon174.jpg`
A 174×174 JPEG shown only on the **All Software** page.

### `config.ini`
A text file holding a custom name, publisher and version number:

```ini
[override_nacp]
name=Minecraft
author=Mojang Studios
display_version=26.2
```
- `name=` and `author=` must **both** have values, or the sysmodule ignores
  them. If you only change one of them in the web editor, the app quietly fills in
  the other with the game's original value.
- `display_version=` is optional. Without it, the game shows its real version
  number, and the custom name and publisher still apply.

Any of these files can be left out. If one is missing, the Switch simply uses the game's original icon or details for that part.

---

## Building

Most people can just download the `.nro` from
[Releases](../../releases). Build it yourself only if you want to change the code.

### 1. Install devkitPro (one time)

devkitPro is the toolchain that builds Switch homebrew. Follow the official
[Getting Started guide](https://devkitpro.org/wiki/Getting_Started) for your
system (Windows installer, macOS package, or Linux), then install the Switch
packages:

| System | Command |
|--------|---------|
| Windows | In the **devkitPro MSYS2** shell: `pacman -S switch-dev switch-curl` |
| macOS / Debian / Ubuntu | `sudo dkp-pacman -S switch-dev switch-curl` |
| Arch / Fedora / other pacman setups | `sudo pacman -S switch-dev switch-curl` |

Accept the defaults when it asks which packages to install. Then open a new
terminal and check that `echo $DEVKITPRO` prints `/opt/devkitpro`. If it
doesn't, log out and back in.

### 2. Build

Download this repo (**`Code`** → **`Download ZIP`**, or `git clone`), open a terminal
in its folder and run:

```bash
make
```

This creates `nx-title-editor.nro`. Run `make clean` first if you ever need a
fresh build.

**Without installing devkitPro:** if you have
[Docker](https://www.docker.com/) or Podman, the official image has everything
built in:

```bash
docker run --rm -v "$PWD":/project -w /project devkitpro/devkita64 make
```

(Swap `docker` for `podman` if you use Podman; on Linux with SELinux, add `:Z`
after `/project` in the `-v` part. In Windows PowerShell, use `${PWD}` instead
of `"$PWD"`.)

**`curl-config: command not found`?** The `switch-curl` package is missing.
Install it with the command for your system above, then build again.

### 3. Copy it to the Switch

Copy `nx-title-editor.nro` to `/switch/` on your SD card. Use any method you like: an SD card reader, FTP, or MTP.

**Quick testing with nxlink:** devkitPro includes `nxlink`, which sends and
launches an `.nro` over the network in one step. On the Switch, open the
Homebrew Menu and press **`Y`**, then on your PC run:

```bash
nxlink -a <switch-ip> nx-title-editor.nro
```

---

## Project layout

| Path | What it is |
|---|---|
| `source/main.cpp` | Startup, URL routes, network watch |
| `source/http.cpp/.hpp` | Minimal HTTP server polled from the main loop |
| `source/titles.cpp/.hpp` | Installed-title listing and icons via the `ns` service |
| `source/fetch.cpp/.hpp` | Outbound HTTPS via curl (SteamGridDB, links) |
| `source/storage.cpp/.hpp` | SD card files: settings and icon writes |
| `source/details.cpp/.hpp` | Custom name/publisher/version in sys-ticon's `config.ini` |
| `source/screen.cpp/.hpp` | The Switch screen: status, QR code, activity log and `log.txt` |
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
- Thanks to MasaGratoR for [sys-ticon](https://github.com/masagrator/sys-ticon) and Samuel P. for [sys-tweak](https://github.com/p-sam/switch-sys-tweak), which make this app possible
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
