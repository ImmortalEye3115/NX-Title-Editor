// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

const $ = (id) => document.getElementById(id);

// ---------------------------------------------------------------------------
// Icons: Tabler Icons (MIT licence, see licenses/)
// ---------------------------------------------------------------------------
const SVG_OPEN =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" ' +
  'stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">';
const paths = (...d) => SVG_OPEN + d.map((p) => `<path d="${p}"/>`).join("") + "</svg>";
const ICONS = {
  home: paths("M5 12l-2 0l9 -9l9 9l-2 0", "M5 12v7a2 2 0 0 0 2 2h10a2 2 0 0 0 2 -2v-7",
    "M9 21v-6a2 2 0 0 1 2 -2h2a2 2 0 0 1 2 2v6"),
  grid: paths("M4 5a1 1 0 0 1 1 -1h4a1 1 0 0 1 1 1v4a1 1 0 0 1 -1 1h-4a1 1 0 0 1 -1 -1l0 -4",
    "M14 5a1 1 0 0 1 1 -1h4a1 1 0 0 1 1 1v4a1 1 0 0 1 -1 1h-4a1 1 0 0 1 -1 -1l0 -4",
    "M4 15a1 1 0 0 1 1 -1h4a1 1 0 0 1 1 1v4a1 1 0 0 1 -1 1h-4a1 1 0 0 1 -1 -1l0 -4",
    "M14 15a1 1 0 0 1 1 -1h4a1 1 0 0 1 1 1v4a1 1 0 0 1 -1 1h-4a1 1 0 0 1 -1 -1l0 -4"),
  square: paths("M3 5a2 2 0 0 1 2 -2h14a2 2 0 0 1 2 2v14a2 2 0 0 1 -2 2h-14a2 2 0 0 1 -2 -2v-14"),
  vertical: paths("M5 5a2 2 0 0 1 2 -2h10a2 2 0 0 1 2 2v14a2 2 0 0 1 -2 2h-10a2 2 0 0 1 -2 -2l0 -14"),
  refresh: paths("M20 11a8.1 8.1 0 0 0 -15.5 -2m-.5 -4v4h4", "M4 13a8.1 8.1 0 0 0 15.5 2m.5 4v-4h-4"),
  up: paths("M12 5l0 14", "M18 11l-6 -6", "M6 11l6 -6"),
  x: paths("M18 6l-12 12", "M6 6l12 12"),
  left: paths("M5 12l14 0", "M5 12l6 6", "M5 12l6 -6"),
  right: paths("M5 12l14 0", "M13 18l6 -6", "M13 6l6 6"),
  eye: paths("M10 12a2 2 0 1 0 4 0a2 2 0 0 0 -4 0", "M21 12c-2.4 4 -5.4 6 -9 6c-3.6 0 -6.6 -2 -9 -6c2.4 -4 5.4 -6 9 -6c3.6 0 6.6 2 9 6"),
  settings: paths("M10.325 4.317c.426 -1.756 2.924 -1.756 3.35 0a1.724 1.724 0 0 0 2.573 1.066c1.543 -.94 3.31 .826 2.37 2.37a1.724 1.724 0 0 0 1.065 2.572c1.756 .426 1.756 2.924 0 3.35a1.724 1.724 0 0 0 -1.066 2.573c.94 1.543 -.826 3.31 -2.37 2.37a1.724 1.724 0 0 0 -2.572 1.065c-.426 1.756 -2.924 1.756 -3.35 0a1.724 1.724 0 0 0 -2.573 -1.066c-1.543 .94 -3.31 -.826 -2.37 -2.37a1.724 1.724 0 0 0 -1.065 -2.572c-1.756 -.426 -1.756 -2.924 0 -3.35a1.724 1.724 0 0 0 1.066 -2.573c-.94 -1.543 .826 -3.31 2.37 -2.37c1 .608 2.296 .07 2.572 -1.065", "M9 12a3 3 0 1 0 6 0a3 3 0 0 0 -6 0"),
  pencil: paths("M4 20h4l10.5 -10.5a2.828 2.828 0 1 0 -4 -4l-10.5 10.5v4", "M13.5 6.5l4 4"),
  search: paths("M3 10a7 7 0 1 0 14 0a7 7 0 1 0 -14 0", "M21 21l-6 -6"),
  trash: paths("M4 7l16 0", "M10 11l0 6", "M14 11l0 6", "M5 7l1 12a2 2 0 0 0 2 2h8a2 2 0 0 0 2 -2l1 -12",
    "M9 7v-3a1 1 0 0 1 1 -1h4a1 1 0 0 1 1 1v3"),
};

function icon(name, cls = "") {
  const span = document.createElement("span");
  span.className = "ico " + cls;
  span.innerHTML = ICONS[name];
  return span;
}
// Fill static placeholders: <span class="ico" data-icon="home">
document.querySelectorAll(".ico[data-icon]").forEach((el) => { el.innerHTML = ICONS[el.dataset.icon]; });

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
let titles = [];
let current = null;           // title being edited
let settings = { has_sgdb_key: false, home_shape: "1:1", all_shape: "1:1", all_default: "skip", port: 0, active_port: 0 };
let kindFilter = "all";  // "all" | "home" (only Home Screen custom) | "all174" (only All Software custom) | "stock" (neither)
let showing = "home";         // which icon the list cards show: "home" | "all"
let target = "home";          // which preview picked art goes to: "home" | "all"
let sgdbStyle = "square";
let sgdbGameId = null;

// Unsaved edits for the open game, per screen. null = show what's on the Switch.
//   { kind: "pick", label, bitmap, url?, preview, from? }  new art (from: "copy" | "auto")
//   { kind: "remove" }                                     remove the custom icon
// Nothing is written until Save.
let draft = { home: null, all: null, details: null };

let statusTimer = null;
function setStatus(el, text, kind = "") {
  el.textContent = text;
  el.classList.add("status");
  el.classList.remove("ok", "err", "fade");
  if (kind) el.classList.add(kind);
}

async function api(path, opts = {}) {
  const r = await fetch(path, opts);
  let data = null;
  try { data = await r.json(); } catch { /* non-JSON */ }
  if (!r.ok) throw new Error((data && data.error) || `HTTP ${r.status}`);
  return data;
}

// ---------------------------------------------------------------------------
// Shapes ("W:H" strings). Display only: files are always square.
// ---------------------------------------------------------------------------
function gcd(a, b) { return b ? gcd(b, a % b) : a; }

function reduce(w, h) {
  const g = gcd(w, h);
  return [w / g, h / g];
}

// Accepts "92:43", "920x430", "920×430", "920*430", "2/3", or a decimal like "0.67".
// Returns "W:H" (whole numbers, simplified) or null.
function parseShape(text) {
  const s = String(text).trim().toLowerCase();
  let w, h;
  let m = s.match(/^(\d+(?:\.\d+)?)\s*[:x×*/]\s*(\d+(?:\.\d+)?)$/);
  if (m) {
    w = parseFloat(m[1]);
    h = parseFloat(m[2]);
    const scale = Math.pow(10, Math.max(decimals(m[1]), decimals(m[2])));
    w = Math.round(w * scale);
    h = Math.round(h * scale);
  } else if ((m = s.match(/^\d*\.?\d+$/))) {
    [w, h] = approximate(parseFloat(s));
  } else {
    return null;
  }
  if (!(w > 0 && h > 0)) return null;
  [w, h] = reduce(w, h);
  if (w > 100000 || h > 100000) return null;
  return `${w}:${h}`;
}

function decimals(str) {
  const i = str.indexOf(".");
  return i < 0 ? 0 : str.length - i - 1;
}

// Closest fraction with a small denominator, so 0.67 -> 2:3.
function approximate(x) {
  let best = [Math.round(x), 1];
  for (let d = 1; d <= 100; d++) {
    const n = Math.round(x * d);
    if (n > 0 && Math.abs(n / d - x) < Math.abs(best[0] / best[1] - x) - 1e-9) best = [n, d];
    if (Math.abs(n / d - x) <= x * 0.005 && n > 0) return [n, d];
  }
  return best;
}

function shapeNums(shape) {
  const [w, h] = shape.split(":").map(Number);
  return { w, h };
}
const shapeFor = (which) => shapeNums(which === "home" ? settings.home_shape : settings.all_shape);

function applyShapeVars() {
  const root = document.documentElement.style;
  const h = shapeFor("home"), a = shapeFor("all");
  root.setProperty("--home-ar", `${h.w} / ${h.h}`);
  root.setProperty("--all-ar", `${a.w} / ${a.h}`);
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------
// Custom shapes have three boxes that fill each other in: a ratio (92:43), a
// size in pixels (920x430) and a decimal (2.14). A typed size is stored as-is
// ("920:430") so it shows again next time; otherwise the reduced ratio is stored.
function parseSize(text) {
  const m = String(text).trim().toLowerCase().match(/^(\d{1,6})\s*[x×*]\s*(\d{1,6})$/);
  if (!m || +m[1] < 1 || +m[2] < 1) return null;
  return [+m[1], +m[2]];
}
function parseDecimal(text) {
  const t = String(text).trim();
  return /^\d*\.?\d+$/.test(t) && parseFloat(t) > 0 ? parseShape(t) : null;
}
const reducedShape = (shape) => { const [w, h] = shape.split(":").map(Number); return reduce(w, h).join(":"); };
const sameShape = (a, b) => reducedShape(a) === reducedShape(b);
const shapeDecimal = (shape) => { const [w, h] = shape.split(":").map(Number); return String(+(w / h).toFixed(3)); };

function shapeBoxes(prefix) {
  return { sel: $(`${prefix}ShapeSel`), row: $(`${prefix}ShapeCustom`),
           r: $(`${prefix}ShapeRatio`), z: $(`${prefix}ShapeSize`), d: $(`${prefix}ShapeDec`) };
}

function fillShapeControls(prefix, shape) {
  const b = shapeBoxes(prefix);
  const preset = ["1:1", "22:31", "92:43", "2:3"].find((p) => sameShape(shape, p)) || null;
  b.sel.value = preset || "custom";
  const [w, h] = shape.split(":").map(Number);
  b.r.value = preset ? "" : reducedShape(shape);
  b.z.value = preset ? "" : `${w}x${h}`;
  b.d.value = preset ? "" : shapeDecimal(shape);
  syncShapeBoxes(prefix);
}

// Shows the custom line only for Custom, and marks unreadable boxes red.
function syncShapeBoxes(prefix) {
  const b = shapeBoxes(prefix);
  const custom = b.sel.value === "custom";
  b.row.classList.toggle("hidden", !custom);
  const bad = (box, parse) => box.classList.toggle("bad", custom && !!box.value.trim() && !parse(box.value));
  bad(b.r, parseShape); bad(b.z, parseSize); bad(b.d, parseDecimal);
}

// Returns the chosen "W:H", or null if the custom boxes can't be read.
function readShapeControl(prefix) {
  const b = shapeBoxes(prefix);
  if (b.sel.value !== "custom") return b.sel.value;
  const rT = b.r.value.trim(), zT = b.z.value.trim(), dT = b.d.value.trim();
  const ratio = parseShape(rT), size = parseSize(zT), dec = parseDecimal(dT);
  if ((rT && !ratio) || (zT && !size) || (dT && !dec)) return null;  // a red box blocks saving
  if (size && (!ratio || sameShape(`${size[0]}:${size[1]}`, ratio))) return `${size[0]}:${size[1]}`;
  return ratio || dec;
}

["home", "all"].forEach((prefix) => {
  const b = shapeBoxes(prefix);
  b.sel.addEventListener("change", () => {
    syncShapeBoxes(prefix);
    if (b.sel.value === "custom" && !b.r.value && !b.z.value && !b.d.value) b.r.focus();
  });
  b.r.addEventListener("input", () => {
    const ratio = parseShape(b.r.value);
    if (ratio) {
      const size = parseSize(b.z.value);
      if (!(size && sameShape(`${size[0]}:${size[1]}`, ratio))) b.z.value = ratio.replace(":", "x");
      b.d.value = shapeDecimal(ratio);
    }
    syncShapeBoxes(prefix);
  });
  b.z.addEventListener("input", () => {
    const size = parseSize(b.z.value);
    if (size) {
      b.r.value = reduce(size[0], size[1]).join(":");
      b.d.value = shapeDecimal(`${size[0]}:${size[1]}`);
    }
    syncShapeBoxes(prefix);
  });
  b.d.addEventListener("input", () => {
    const dec = parseDecimal(b.d.value);
    const ratio = parseShape(b.r.value);
    // Leave the other boxes alone if they already round to what was typed.
    if (dec && !(ratio && +shapeDecimal(ratio) === parseFloat(b.d.value))) {
      b.r.value = dec;
      b.z.value = dec.replace(":", "x");
    }
    syncShapeBoxes(prefix);
  });
});

// Key state: dots in the Settings box when a key is saved, and the SteamGridDB
// tab swaps its search for a short message when there isn't one.
// A saved key shows as dots in the box (the real key never leaves the Switch).
// Leave the dots to keep it, type/paste to replace it, or clear the box to remove it.
const KEY_DOTS = "••••••••••••••••";
function syncKeyState() {
  $("sgdbKey").value = settings.has_sgdb_key ? KEY_DOTS : "";
  syncClear("sgdbKey", "sgdbKeyClear");
  $("sgdbNoKey").classList.toggle("hidden", settings.has_sgdb_key);
  $("sgdbBody").classList.toggle("hidden", !settings.has_sgdb_key);
}

function fillSettingsForm() {
  syncKeyState();
  fillShapeControls("home", settings.home_shape);
  fillShapeControls("all", settings.all_shape);
  $("allDefault").value = settings.all_default;
  $("port").value = settings.port ? String(settings.port) : "";
  $("portHint").textContent =
    (settings.active_port ? `Currently using port ${settings.active_port}. ` : "") +
    "Leave empty for automatic. Changes apply next time you open the app.";
}

async function loadSettings() {
  try { settings = await api("/api/settings"); } catch { /* keep defaults */ }
  fillSettingsForm();
  applyShapeVars();
}

// ---- Browser tab icon ----
// The header badge, drawn as an SVG icon in the current theme's colours
// (mint tile in dark, blue in light). Browsers without SVG icons use icon.png.
function updateFavicon() {
  const logo = document.querySelector(".brand .sw-logo");
  const css = getComputedStyle(document.documentElement);
  const accent = css.getPropertyValue("--accent").trim(), bg = css.getPropertyValue("--bg").trim();
  if (!logo || !accent) return;
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">` +
    `<rect width="24" height="24" rx="6" fill="${accent}"/>` +
    `<g transform="translate(3.3 3.3) scale(0.737)" color="${bg}">${logo.innerHTML}</g></svg>`;
  $("favicon").href = "data:image/svg+xml," + encodeURIComponent(svg);
}
if (window.matchMedia) {
  const mq = window.matchMedia("(prefers-color-scheme: light)");
  (mq.addEventListener ? mq.addEventListener.bind(mq, "change") : mq.addListener.bind(mq))(updateFavicon);
}

// ---- App Theme (kept in this browser, not on the Switch) ----
function currentTheme() {
  try { return localStorage.getItem("nxte-theme") || "auto"; } catch { return "auto"; }
}
function applyTheme(theme) {
  if (theme === "light" || theme === "dark") document.documentElement.dataset.theme = theme;
  else delete document.documentElement.dataset.theme;
  document.querySelectorAll("#themeSeg .seg-btn").forEach((b) => b.classList.toggle("active", b.dataset.theme === theme));
  updateFavicon();
}
document.querySelectorAll("#themeSeg .seg-btn").forEach((btn) => {
  btn.onclick = () => {
    try { localStorage.setItem("nxte-theme", btn.dataset.theme); } catch { /* storage unavailable */ }
    applyTheme(btn.dataset.theme);
  };
});
applyTheme(currentTheme());

// ---- Settings dialog ----
function openSettings() {
  fillSettingsForm();
  setStatus($("settingsStatus"), "");
  $("settingsDlg").classList.remove("hidden");
  document.body.classList.add("lb-open");
}
function closeSettings() {
  $("settingsDlg").classList.add("hidden");
  if ($("lightbox").classList.contains("hidden")) document.body.classList.remove("lb-open");
}
$("settingsToggle").onclick = openSettings;
$("sgdbKey").addEventListener("input", () => syncClear("sgdbKey", "sgdbKeyClear"));
// Clicking into the dots selects them, so typing or pasting replaces them.
$("sgdbKey").addEventListener("focus", () => { if ($("sgdbKey").value === KEY_DOTS) $("sgdbKey").select(); });
$("sgdbKeyClear").onclick = (e) => {
  e.preventDefault();
  $("sgdbKey").value = "";
  syncClear("sgdbKey", "sgdbKeyClear");
  $("sgdbKey").focus();
};
$("noKeySettings").onclick = () => { openSettings(); $("sgdbKey").focus(); };
$("settingsClose").onclick = closeSettings;

// ---- About ----
function openAbout() {
  $("aboutVersion").textContent = settings.version || "";
  $("aboutDlg").classList.remove("hidden");
  document.body.classList.add("lb-open");
}
function closeAbout() {
  $("aboutDlg").classList.add("hidden");
  if ($("lightbox").classList.contains("hidden")) document.body.classList.remove("lb-open");
}
$("aboutToggle").onclick = openAbout;
$("aboutClose").onclick = closeAbout;
$("aboutDlg").onclick = (e) => { if (e.target === $("aboutDlg")) closeAbout(); };
document.addEventListener("keydown", (e) => {
  if (e.key === "Escape" && !$("aboutDlg").classList.contains("hidden")) closeAbout();
});
$("settingsDlg").onclick = (e) => { if (e.target === $("settingsDlg")) closeSettings(); };
document.addEventListener("keydown", (e) => {
  // Esc closes the reboot confirmation first if it's open on top
  if (e.key === "Escape" && !confirmResolve && !$("settingsDlg").classList.contains("hidden")) closeSettings();
});

$("saveSettings").onclick = async () => {
  const st = $("settingsStatus");
  const home = readShapeControl("home");
  const all = readShapeControl("all");
  if (!home || !all) {
    setStatus(st, "Fix the custom shape first (the red box).", "err");
    return;
  }
  const port = $("port").value.trim();
  if (port && !(/^\d+$/.test(port) && +port >= 1 && +port <= 65535)) {
    setStatus(st, "Port must be a number from 1 to 65535.", "err");
    return;
  }
  const form = new URLSearchParams({ home_shape: home, all_shape: all, all_default: $("allDefault").value, port });
  const key = $("sgdbKey").value.trim();
  if (key !== KEY_DOTS && (key || settings.has_sgdb_key)) form.set("sgdb_key", key);  // "" removes it
  try {
    const hadKey = settings.has_sgdb_key;
    settings = await api("/api/settings", { method: "POST", body: form });
    fillSettingsForm();
    applyShapeVars();
    renderList();
    if (current) {
      renderSlots();
      if (!hadKey && settings.has_sgdb_key) sgdbSearch(true);
    }
    // Tick on the button, then close
    const btn = $("saveSettings");
    btn.classList.add("saved");
    btn.textContent = "✓";
    setTimeout(() => {
      btn.classList.remove("saved");
      btn.textContent = "Save settings";
      closeSettings();
    }, 900);
  } catch (e) {
    setStatus(st, e.message, "err");
  }
};

// ---------------------------------------------------------------------------
// Art boxes: custom art stretched to its screen's shape, stock art square
// ---------------------------------------------------------------------------
function fillArt(box, t, which) {
  const custom = which === "home" ? t.custom_icon : t.custom_icon174;
  box.innerHTML = "";
  box.classList.toggle("custom", !!custom);
  const img = document.createElement("img");
  img.alt = "";
  img.loading = "lazy";
  img.src = custom
    ? `/api/custom-icon/${t.id}?${which === "all" ? "size=174&" : ""}v=${t._v || 0}`
    : `/api/icon/${t.id}`;
  box.appendChild(img);
}

const isCustom = (t) => t.custom_icon || t.custom_icon174;
// Custom details (sys-ticon config.ini). Empty strings mean "not overridden".
const hasDetails = (t) => !!(t.custom_name || t.custom_publisher || t.custom_version);
const shownName = (t) => t.custom_name || t.name;

function statusLine(t) {
  return `Home Screen: ${t.custom_icon ? "custom" : "stock"} · All Software: ${t.custom_icon174 ? "custom" : "stock"}`;
}

// ---------------------------------------------------------------------------
// Game list
// ---------------------------------------------------------------------------
function renderList() {
  const q = $("filter").value.trim().toLowerCase();
  const grid = $("games");
  grid.innerHTML = "";
  grid.classList.toggle("showing-all", showing === "all");
  const shown = titles.filter((t) => {
    if (kindFilter === "home" && !(t.custom_icon && !t.custom_icon174)) return false;
    if (kindFilter === "all174" && !(t.custom_icon174 && !t.custom_icon)) return false;
    if (kindFilter === "stock" && isCustom(t)) return false;
    if (kindFilter === "both" && !(t.custom_icon && t.custom_icon174)) return false;
    return !q || [t.name, shownName(t), t.publisher, t.custom_publisher, t.id]
      .some((v) => v && v.toLowerCase().includes(q));
  });
  shown.forEach((t) => {
    const card = document.createElement("button");
    card.className = "game";
    card.onclick = () => { location.hash = "#/" + t.id; };

    const art = document.createElement("div");
    art.className = "art";
    fillArt(art, t, showing);
    card.appendChild(art);

    const name = document.createElement("div");
    name.className = "name";
    name.textContent = shownName(t);
    name.title = (t.custom_publisher || t.publisher) ? `${shownName(t)} (${t.custom_publisher || t.publisher})` : shownName(t);
    card.appendChild(name);

    const tid = document.createElement("div");
    tid.className = "tid";
    tid.textContent = t.id;
    card.appendChild(tid);

    const ver = document.createElement("div");
    ver.className = "ver";
    ver.textContent = t.version ? `v${t.version}` : "";
    card.appendChild(ver);

    const foot = document.createElement("div");
    foot.className = "foot";
    foot.title = statusLine(t);
    const pill = document.createElement("span");
    pill.className = "pill" + (isCustom(t) ? " on" : "");
    pill.textContent = isCustom(t) ? "custom icon" : "stock icon";
    foot.append(pill, icon("home", t.custom_icon ? "on" : ""), icon("grid", t.custom_icon174 ? "on" : ""));
    card.appendChild(foot);

    grid.appendChild(card);
  });
  const label = { all: "games", home: "games with only a custom Home Screen icon",
    all174: "games with only a custom All Software icon", stock: "games with stock icons",
    both: "games with both icons custom" }[kindFilter];
  $("summary").textContent =
    `${shown.length} of ${titles.length} ${label}` + (q ? ` matching "${q}"` : "") + " · tap a game to change its icon";
}

async function loadTitles(refresh = false) {
  $("summary").textContent = refresh ? "Rescanning. Watch the Switch screen for progress..." : "Loading...";
  try {
    const data = await api(`/api/titles${refresh ? "?refresh=1" : ""}`);
    titles = data.titles || [];
    updateChanges(data.changes);
    renderList();
  } catch (e) {
    $("summary").textContent = `Couldn't reach the Switch: ${e.message}`;
  }
}

$("filter").addEventListener("input", () => { syncClear("filter", "filterClear"); renderList(); });

// ✕ inside a search box: shown only when there's text
function syncClear(inputId, btnId) {
  $(btnId).classList.toggle("hidden", !$(inputId).value);
}
$("filterClear").onclick = () => {
  $("filter").value = "";
  syncClear("filter", "filterClear");
  renderList();
  $("filter").focus();
};
$("refresh").onclick = () => loadTitles(true);

function segChoice(segId, attr, onPick) {
  document.querySelectorAll(`#${segId} .seg-btn`).forEach((btn) => {
    btn.onclick = () => {
      document.querySelectorAll(`#${segId} .seg-btn`).forEach((b) => b.classList.remove("active"));
      btn.classList.add("active");
      onPick(btn.dataset[attr]);
    };
  });
}
segChoice("showingSeg", "show", (v) => { showing = v; renderList(); });

// Filter: All, plus two toggles that match the card indicators exactly.
// From All, a toggle picks just itself; after that they toggle freely:
// one on = only that icon custom, both on = both custom, both off = stock.
const kindState = { all: true, home: false, apps: false };
function syncKindFilter() {
  for (const k of ["all", "home", "apps"]) {
    const btn = document.querySelector(`#kindFilter [data-toggle="${k}"]`);
    btn.classList.toggle("active", kindState[k]);
    btn.setAttribute("aria-pressed", kindState[k]);
  }
  kindFilter = kindState.all ? "all"
    : kindState.home && kindState.apps ? "both"
    : kindState.home ? "home" : kindState.apps ? "all174" : "stock";
  renderList();
}
document.querySelectorAll("#kindFilter .seg-btn").forEach((btn) => {
  btn.onclick = () => {
    const k = btn.dataset.toggle;
    if (k === "all") Object.assign(kindState, { all: true, home: false, apps: false });
    else if (kindState.all) Object.assign(kindState, { all: false, home: k === "home", apps: k === "apps" });
    else kindState[k] = !kindState[k];
    syncKindFilter();
  };
});

// ---- Pinned toolbar: same layout pinned or not; just a divider once pinned ----
const toolbar = $("toolbar");
new IntersectionObserver(([entry]) => {
  toolbar.classList.toggle("pinned", !entry.isIntersecting && !$("listView").classList.contains("hidden"));
}).observe($("toolbarSentinel"));

// ---------------------------------------------------------------------------
// Views (hash routing so the phone's back button works)
// ---------------------------------------------------------------------------
function showRoute() {
  const m = location.hash.match(/^#\/([0-9A-Fa-f]{16})$/);
  const t = m && titles.find((x) => x.id.toUpperCase() === m[1].toUpperCase());
  closeLightbox();
  if (current && isDirty() && (!t || t.id !== current.id)) {
    history.pushState(null, "", "#/" + current.id);  // undo the navigation, then ask
    askDiscard();
    return;
  }
  if (t) {
    openEditor(t);
  } else {
    current = null;
    draft = { home: null, all: null };
    $("editorView").classList.add("hidden");
    $("listView").classList.remove("hidden");
    updateBar();
  }
}
window.addEventListener("hashchange", showRoute);

// ---------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------
function cleanName(name) {
  return name.replace(/[™®©]/g, "").replace(/\s+/g, " ").trim();
}

const hasCustom = (t, which) => (which === "home" ? t.custom_icon : t.custom_icon174);
const isDirty = () => !!(draft.home || draft.all || detailsDirty());

// ---- Details (name / publisher / version) ----
// Boxes hold only what differs from the game's own values; empty = keep original.
const DETAIL_KEYS = [["name", "Name", "name"], ["publisher", "Publisher", "publisher"], ["version", "Version", "version"]];
function savedBoxes(t) {
  const pick = (custom, orig) => (custom && custom !== orig ? custom : "");
  return { name: pick(t.custom_name, t.name), publisher: pick(t.custom_publisher, t.publisher), version: pick(t.custom_version, t.version) };
}
const sameBoxes = (a, b) => a.name === b.name && a.publisher === b.publisher && a.version === b.version;
const emptyBoxes = (b) => !b.name && !b.publisher && !b.version;
function detailsDirty() {
  return !!(current && draft.details && !sameBoxes(draft.details, savedBoxes(current)));
}
const effBoxes = () => draft.details || savedBoxes(current);
function effDetails() {
  const b = effBoxes(), t = current;
  return { name: b.name || t.name, publisher: b.publisher || t.publisher, version: b.version || t.version };
}

function renderHeader() {
  const t = current, d = effDetails();
  $("edName").textContent = d.name;
  $("edTid").textContent = [t.id, d.version && "v" + d.version, d.publisher].filter(Boolean).join(" · ");
  const tag = $("detailsTag");
  const kind = detailsDirty() ? (emptyBoxes(effBoxes()) ? "remove" : "new") : hasDetails(t) ? "custom" : "";
  tag.className = "name-tag " + (kind || "hidden");
  tag.textContent = kind;
  $("resetBtn").disabled = !(isCustom(t) || hasDetails(t) || isDirty());
}

function openEditor(t) {
  const switching = !current || current.id !== t.id;
  current = t;
  $("listView").classList.add("hidden");
  $("editorView").classList.remove("hidden");
  if (switching) {
    $("uploadName").value = "";
    draft = { home: null, all: null, details: null };
    $("slotError").textContent = "";
    $("sgdbQuery").value = cleanName(t.name);
    syncClear("sgdbQuery", "sgdbClear");
    suggestions = [];
    suggestFor = "";
    closeSuggestions();
    $("sgdbMatchLink").classList.add("hidden");
    sgdbMessage("");
    $("iconGrid").innerHTML = "";
    sgdbGameId = null;
    setTarget("home", false);
    if (settings.has_sgdb_key) sgdbSearch();
  }
  renderSlots();
  updateBar();
  window.scrollTo(0, 0);
}

// Draw one slot: new pick, marked for removal (shows stock), or what's on the Switch.
// Tag elements are moved into the art box on each render, so keep references.
const slotTags = { home: $("tagHome"), all: $("tagAll") };
const slotEyes = { home: $("eyeHome"), all: $("eyeAll") };

function renderSlot(which) {
  const t = current;
  const d = draft[which];
  const box = which === "home" ? $("artHome") : $("artAll");
  const tag = slotTags[which];
  if (d && d.kind === "pick") {
    box.innerHTML = "";
    box.classList.add("custom");
    const img = document.createElement("img");
    img.alt = "";
    img.src = d.preview;
    box.appendChild(img);
  } else {
    fillArt(box, d && d.kind === "remove" ? { ...t, custom_icon: false, custom_icon174: false } : t, which);
  }
  // Outline tags = what's on the Switch now; solid tags = unsaved changes.
  const kind = d ? (d.kind === "pick" ? "new" : "remove") : hasCustom(t, which) ? "custom" : "stock";
  tag.className = "tag " + kind;
  tag.textContent = kind;
  box.appendChild(tag);              // pinned to the art's top-right corner
  box.appendChild(slotEyes[which]);  // and the eye to its bottom-right

  const shows = showsCustom(which);
  const trash = which === "home" ? $("trashHome") : $("trashAll");
  trash.disabled = !d && !hasCustom(t, which);
  trash.classList.toggle("active", !!(d && d.kind === "remove"));
  trash.title = d && d.kind === "remove" ? "Keep this icon" : d ? "Discard the new pick" : "Remove this icon";
  trash.querySelector(".btn-lbl").textContent = d && d.kind === "remove" ? "Undo" : "Remove";
  (which === "home" ? $("copyHome") : $("copyAll")).disabled = !shows;
}

// Does this slot currently show custom art (a pick, or the Switch's custom icon)?
function showsCustom(which) {
  const d = draft[which];
  if (d) return d.kind === "pick";
  return !!hasCustom(current, which);
}

function renderSlots() {
  renderSlot("home");
  renderSlot("all");
  for (const [id, which] of [["artHome", "home"], ["artAll", "all"]]) {
    const { w, h } = shapeFor(which);
    $(id).classList.toggle("wide", w > h);
  }
  renderHeader();
  const copied = (draft.all && draft.all.from) || (draft.home && draft.home.from);
  $("mismatchNote").classList.toggle("hidden", !(copied && !sameShape(settings.home_shape, settings.all_shape)));
  markPicked();
  updateBar();  // every staged change (pick, remove, copy, details) lands here
}

function trash(which) {
  const d = draft[which];
  if (d) draft[which] = null;                                  // discard pick / keep icon
  else if (hasCustom(current, which)) draft[which] = { kind: "remove" };
  if (which === "home" && draft.all && draft.all.from === "auto") draft.all = null;
  renderSlots();
}
$("trashHome").onclick = () => trash("home");
$("trashAll").onclick = () => trash("all");

// Copy what one slot shows into the other.
async function copySlot(from, to) {
  const d = draft[from];
  $("slotError").textContent = "";
  try {
    let bitmap, label;
    if (d && d.kind === "pick") {
      ({ bitmap, label } = d);
    } else {
      const r = await fetch(`/api/custom-icon/${current.id}${from === "all" ? "?size=174" : ""}`);
      if (!r.ok) throw new Error("Couldn't read the current icon to copy it.");
      bitmap = await createImageBitmap(await r.blob());
      label = "current icon";
    }
    setSlot(to, { label, bitmap, url: d && d.url, from: "copy" });
  } catch (e) {
    $("slotError").textContent = e.message;
  }
}
$("copyHome").onclick = () => copySlot("home", "all");
$("copyAll").onclick = () => copySlot("all", "home");

// ---- Shared confirmation dialog ----
// confirmDialog({title, text, warn?, yes, no}) -> Promise<boolean>
let confirmResolve = null;
function confirmDialog({ title, text, items = [], note = "", warn = "", yes, no = "Cancel" }) {
  $("confirmTitle").textContent = title;
  $("confirmText").textContent = text;
  $("confirmList").replaceChildren(...items.map((x) => Object.assign(document.createElement("li"), { textContent: x })));
  $("confirmList").classList.toggle("hidden", !items.length);
  $("confirmNote").textContent = note;
  $("confirmNote").classList.toggle("hidden", !note);
  $("confirmWarn").textContent = warn;
  $("confirmWarn").classList.toggle("hidden", !warn);
  $("confirmYes").textContent = yes;
  $("confirmNo").textContent = no;
  $("confirmDlg").classList.remove("hidden");
  $("confirmNo").focus();
  return new Promise((resolve) => { confirmResolve = resolve; });
}
function closeConfirm(answer) {
  $("confirmDlg").classList.add("hidden");
  if (confirmResolve) confirmResolve(answer);
  confirmResolve = null;
}
$("confirmNo").onclick = () => closeConfirm(false);
$("confirmYes").onclick = () => closeConfirm(true);
$("confirmDlg").onclick = (e) => { if (e.target === $("confirmDlg")) closeConfirm(false); };
document.addEventListener("keydown", (e) => {
  if (e.key === "Escape" && confirmResolve) closeConfirm(false);
});

// ---- Edit Details popup ----
const byteLen = (v) => new TextEncoder().encode(v).length;
const detBox = (key) => $("det" + key[0].toUpperCase() + key.slice(1));
function syncDetailsField(key) {
  const box = detBox(key), count = $(box.id + "Count");
  const n = byteLen(box.value.trim()), limit = +box.dataset.limit;
  count.textContent = `${n}/${limit}`;
  count.classList.toggle("hidden", !box.value);
  count.classList.toggle("over", n > limit);
  box.classList.toggle("bad", n > limit);
  syncClear(box.id, box.id + "Clear");
  const any = DETAIL_KEYS.some(([k]) => detBox(k).value.trim());
  $("detailsResetWrap").classList.toggle("hidden", !(hasDetails(current) || any));
}
function openDetails() {
  const b = effBoxes(), t = current;
  for (const [key] of DETAIL_KEYS) {
    const box = detBox(key);
    box.value = b[key];
    box.placeholder = t[key] || "";
    syncDetailsField(key);
  }
  $("detailsDlg").classList.remove("hidden");
  document.body.classList.add("lb-open");
}
function closeDetails() {
  $("detailsDlg").classList.add("hidden");
  if ($("lightbox").classList.contains("hidden")) document.body.classList.remove("lb-open");
}
function stageDetails(boxes) {
  draft.details = boxes;
  closeDetails();
  renderSlots();
  updateBar();
}
for (const [key] of DETAIL_KEYS) {
  const box = detBox(key);
  box.addEventListener("input", () => syncDetailsField(key));
  box.addEventListener("keydown", (e) => { if (e.key === "Enter") $("detailsDone").click(); });
  $(box.id + "Clear").onclick = () => { box.value = ""; syncDetailsField(key); box.focus(); };
}
$("editDetails").onclick = openDetails;
$("detailsClose").onclick = closeDetails;
$("detailsDlg").onclick = (e) => { if (e.target === $("detailsDlg")) closeDetails(); };
document.addEventListener("keydown", (e) => {
  if (e.key === "Escape" && !confirmResolve && !$("detailsDlg").classList.contains("hidden")) closeDetails();
});
$("detailsDone").onclick = () => {
  const boxes = {};
  for (const [key] of DETAIL_KEYS) {
    const box = detBox(key);
    if (byteLen(box.value.trim()) > +box.dataset.limit) { box.focus(); return; }  // red box blocks Done
    boxes[key] = box.value.trim();
  }
  stageDetails(boxes);
};
$("detailsReset").onclick = () => stageDetails({ name: "", publisher: "", version: "" });

// ---- Reset to Defaults (trash in the header) ----
// After a confirmation, stages removal of everything custom, like any other
// edit: nothing changes on the Switch until Save.
$("resetBtn").onclick = async () => {
  const t = current, name = shownName(t);
  const items = [];
  if (t.custom_icon) items.push("Home Screen icon");
  if (t.custom_icon174) items.push("All Software icon");
  if (hasDetails(t)) items.push("Name, publisher and version");
  const ok = await confirmDialog(items.length ? {
    title: "Reset to Defaults?",
    text: `This will remove all customisations for ${name}, including:`,
    items,
    note: isDirty() ? "Your unsaved changes will also be discarded." : "",
    warn: "Press Save to apply. A reboot will be required to see these changes.",
    yes: "Reset",
  } : {
    title: "Reset to Defaults?",
    text: `Your unsaved changes to ${name} will be discarded.`,
    yes: "Reset",
  });
  if (!ok) return;
  draft = {
    home: t.custom_icon ? { kind: "remove" } : null,
    all: t.custom_icon174 ? { kind: "remove" } : null,
    details: hasDetails(t) ? { name: "", publisher: "", version: "" } : null,
  };
  $("slotError").textContent = "";
  renderSlots();
};

// Back to the list. With unsaved changes, ask first.
function leaveEditor() {
  draft = { home: null, all: null, details: null };
  location.hash = "";
}
async function askDiscard() {
  if (confirmResolve) return;
  const ok = await confirmDialog({
    title: "Discard changes?",
    text: `Your unsaved changes to ${shownName(current)} will be lost.`,
    yes: "Discard",
    no: "Keep editing",
  });
  if (ok) leaveEditor();
}
$("backBtn").onclick = () => (isDirty() ? askDiscard() : leaveEditor());

// ---- Which slot picked art goes to; tap a slot to choose ----
function setTarget(which, reload = true) {
  target = which;
  $("slotHome").classList.toggle("targeted", which === "home");
  $("slotAll").classList.toggle("targeted", which === "all");
  const name = which === "home" ? "Home Screen" : "All Software";
  $("addingName").textContent = name;
  $("addingIcon").innerHTML = ICONS[which === "home" ? "home" : "grid"];
  setStyle(styleForShape(shapeFor(which)), reload);
}
$("slotHome").onclick = (e) => { if (!e.target.closest("button")) setTarget("home"); };
$("slotAll").onclick = (e) => { if (!e.target.closest("button")) setTarget("all"); };

// ---- Source tabs ----
document.querySelectorAll("#sourceTabs .tab-btn").forEach((btn) => {
  btn.onclick = () => {
    document.querySelectorAll("#sourceTabs .tab-btn").forEach((b) => b.classList.remove("active"));
    document.querySelectorAll(".tab-content").forEach((c) => c.classList.remove("active"));
    btn.classList.add("active");
    $(`tab-${btn.dataset.tab}`).classList.add("active");
  };
});

// ---- SteamGridDB: search box with suggestions ----
const qBox = $("sgdbQuery");
const suggestList = $("sgdbSuggest");
let suggestions = [];       // [{id, name}] for the text currently in the box
let suggestFor = "";        // the text those suggestions belong to
let activeSuggestion = -1;  // highlighted row for arrow keys
let suggestTimer = null;
let sgdbSearchRequest = 0;

function sgdbMessage(text, isError = false) {
  const el = $("sgdbMsg");
  el.textContent = text;
  el.className = (isError ? "status err" : "muted small") + (text ? "" : " hidden");
}

async function fetchSuggestions(q) {
  const req = ++sgdbSearchRequest;  // ignore replies to older searches
  const data = await api(`/api/sgdb/search?q=${encodeURIComponent(q)}`);
  if (req !== sgdbSearchRequest) return null;
  suggestions = (data.data || []).slice(0, 10);
  suggestFor = q;
  activeSuggestion = -1;
  return suggestions;
}

function renderSuggestions() {
  suggestList.innerHTML = "";
  if (!suggestFor) return closeSuggestions();
  if (!suggestions.length) {
    const li = document.createElement("li");
    li.className = "suggest-empty";
    li.textContent = "No SteamGridDB matches. Try a shorter name.";
    suggestList.appendChild(li);
  }
  suggestions.forEach((g, i) => {
    const li = document.createElement("li");
    li.role = "option";
    li.textContent = g.name;
    li.classList.toggle("active", i === activeSuggestion);
    li.classList.toggle("current", String(g.id) === String(sgdbGameId));
    li.onmousedown = (e) => e.preventDefault();  // keep focus so the click lands
    li.onclick = () => chooseSuggestion(g);
    suggestList.appendChild(li);
  });
  suggestList.classList.remove("hidden");
  qBox.setAttribute("aria-expanded", "true");
}

function closeSuggestions() {
  suggestList.classList.add("hidden");
  qBox.setAttribute("aria-expanded", "false");
  activeSuggestion = -1;
}

$("sgdbClear").onmousedown = (e) => e.preventDefault();  // keep focus in the box
$("sgdbClear").onclick = () => {
  qBox.value = "";
  syncClear("sgdbQuery", "sgdbClear");
  clearTimeout(suggestTimer);
  suggestions = [];
  suggestFor = "";
  closeSuggestions();
  qBox.focus();
};

function chooseSuggestion(g) {
  qBox.value = g.name;
  syncClear("sgdbQuery", "sgdbClear");
  closeSuggestions();
  selectSgdbGame(g.id);
}

qBox.addEventListener("input", () => {
  syncClear("sgdbQuery", "sgdbClear");
  clearTimeout(suggestTimer);
  const q = qBox.value.trim();
  if (q.length < 2) {
    suggestions = [];
    suggestFor = "";
    return closeSuggestions();
  }
  suggestTimer = setTimeout(async () => {
    try {
      if (await fetchSuggestions(q)) { sgdbMessage(""); if (document.activeElement === qBox) renderSuggestions(); }
    } catch (e) { sgdbMessage(e.message, true); }
  }, 300);
});

// Focusing or clicking the box (even when it already has focus) shows the list.
qBox.addEventListener("focus", () => { if (suggestFor) renderSuggestions(); });
qBox.addEventListener("click", () => { if (suggestFor && suggestList.classList.contains("hidden")) renderSuggestions(); });
qBox.addEventListener("blur", () => setTimeout(closeSuggestions, 120));

qBox.addEventListener("keydown", async (e) => {
  const open = !suggestList.classList.contains("hidden");
  if (e.key === "ArrowDown" || e.key === "ArrowUp") {
    if (!suggestions.length) return;
    e.preventDefault();
    const n = suggestions.length;
    activeSuggestion = e.key === "ArrowDown" ? (activeSuggestion + 1) % n : (activeSuggestion - 1 + n) % n;
    renderSuggestions();
  } else if (e.key === "Enter") {
    e.preventDefault();
    const q = qBox.value.trim();
    if (!q) return;
    clearTimeout(suggestTimer);
    try {
      if (suggestFor !== q && !(await fetchSuggestions(q))) return;
      const g = suggestions[activeSuggestion >= 0 ? activeSuggestion : 0];
      if (g) chooseSuggestion(g);
      else renderSuggestions();
    } catch (err) { sgdbMessage(err.message, true); }
  } else if (e.key === "Escape" && open) {
    closeSuggestions();
  }
});

// Opening the editor: search the game's name and load the top match, list closed.
async function sgdbSearch() {
  const q = qBox.value.trim();
  if (!q) return;
  sgdbMessage("Searching...");
  try {
    const list = await fetchSuggestions(q);
    if (!list) return;
    if (!list.length) {
      sgdbMessage("No SteamGridDB matches. Try a shorter name.");
      $("iconGrid").innerHTML = "";
      sgdbGameId = null;
      $("sgdbMatchLink").classList.add("hidden");
      return;
    }
    sgdbMessage("");
    qBox.value = list[0].name;
    syncClear("sgdbQuery", "sgdbClear");
    suggestFor = qBox.value.trim();
    selectSgdbGame(list[0].id);
  } catch (e) {
    sgdbMessage(e.message, true);
  }
}

function selectSgdbGame(id) {
  sgdbGameId = id;
  $("sgdbMatchLink").href = `https://www.steamgriddb.com/game/${id}`;
  $("sgdbMatchLink").classList.remove("hidden");
  sgdbLoadImages();
}

// SteamGridDB's grid shapes, as [width, height]
const SGDB_STYLES = { square: [1, 1], galaxy: [22, 31], horizontal: [92, 43], vertical: [2, 3] };

// The SteamGridDB shape closest to a screen's shape (exact match when there is one).
function styleForShape({ w, h }) {
  let best = "square", bestDiff = Infinity;
  for (const [style, [sw, sh]] of Object.entries(SGDB_STYLES)) {
    const diff = Math.abs(Math.log(w / h) - Math.log(sw / sh));
    if (diff < bestDiff) { best = style; bestDiff = diff; }
  }
  return best;
}

function setStyle(style, reload = true) {
  const changed = style !== sgdbStyle;
  sgdbStyle = style;
  $("styleSelect").value = style;
  if (changed && reload && sgdbGameId) sgdbLoadImages();
}
$("styleSelect").onchange = () => setStyle($("styleSelect").value);

let sgdbRequest = 0;
async function sgdbLoadImages() {
  const req = ++sgdbRequest;  // ignore replies to older requests
  const grid = $("iconGrid");
  const [sw, sh] = SGDB_STYLES[sgdbStyle];
  grid.style.setProperty("--thumb-ar", `${sw} / ${sh}`);
  grid.classList.toggle("wide-thumbs", sw > sh);
  grid.innerHTML = `<span class="muted">Loading...</span>`;
  try {
    const data = await api(`/api/sgdb/grids/${sgdbGameId}?style=${sgdbStyle}`);
    if (req !== sgdbRequest) return;
    const items = (data.data || []).slice(0, 36);
    grid.innerHTML = "";
    if (!items.length) {
      grid.innerHTML = `<span class="muted">No images in this shape. Try the other one.</span>`;
      return;
    }
    items.forEach((item) => {
      const cell = document.createElement("div");
      cell.className = "thumb";
      cell.dataset.url = item.url;
      const img = document.createElement("img");
      img.loading = "lazy";
      img.alt = "";
      img.src = item.thumb || item.url;  // thumbnails load straight from SteamGridDB
      cell.onclick = () => assignFromUrl(target, item.url, "SteamGridDB image");
      const eye = document.createElement("button");
      eye.className = "eye-btn";
      eye.setAttribute("aria-label", "View full size");
      eye.appendChild(icon("eye"));
      eye.onclick = (e) => { e.stopPropagation(); viewOriginal(item.url); };
      cell.append(img, eye);
      grid.appendChild(cell);
    });
    markPicked();
  } catch (e) {
    if (req === sgdbRequest) showError(grid, e.message);
  }
}

// Outline thumbnails that are currently in a preview.
function markPicked() {
  const used = new Set([draft.home, draft.all].filter((d) => d && d.url).map((d) => d.url));
  document.querySelectorAll("#iconGrid .thumb").forEach((c) => c.classList.toggle("picked", used.has(c.dataset.url)));
}

// ---- Large view (view only) ----
// ratio = [w, h] to stretch the image to (slot icons), or null to keep its own shape.
function showLightbox(src, ratio, caption) {
  const lb = $("lbImg");
  lb.style.aspectRatio = ratio ? `${ratio[0]} / ${ratio[1]}` : "";
  lb.style.objectFit = ratio ? "fill" : "contain";
  lb.style.width = ratio ? `min(92vw, 900px, calc(72vh * ${ratio[0] / ratio[1]}))` : "";
  lb.style.maxWidth = ratio ? "" : "min(92vw, 900px)";
  lb.style.height = "auto";
  lb.src = src;
  $("lbCaption").textContent = caption;
  $("lightbox").classList.remove("hidden");
  document.body.classList.add("lb-open");
}

// A slot's icon, stretched to its screen's shape like on the Switch.
function openLightbox(which) {
  const box = which === "home" ? $("artHome") : $("artAll");
  const img = box.querySelector("img");
  if (!img) return;
  const { w, h } = shapeFor(which);
  showLightbox(img.src, [w, h], `${which === "home" ? "Home Screen" : "All Software"} · ${slotTags[which].textContent}`);
}

// A SteamGridDB image in its original shape, with its size once it loads.
function viewOriginal(url) {
  showLightbox(url, null, "Original size");
  $("lbImg").onload = () => {
    const i = $("lbImg");
    $("lbCaption").textContent = `Original · ${i.naturalWidth} × ${i.naturalHeight}`;
    i.onload = null;
  };
}
function closeLightbox() {
  $("lightbox").classList.add("hidden");
  document.body.classList.remove("lb-open");
}
$("lbClose").onclick = closeLightbox;
$("lightbox").onclick = (e) => { if (e.target !== $("lbImg")) closeLightbox(); };
document.addEventListener("keydown", (e) => { if (e.key === "Escape") closeLightbox(); });
for (const which of ["home", "all"])
  slotEyes[which].onclick = (e) => { e.stopPropagation(); openLightbox(which); };

// ---- Upload ----
// The box only shows the chosen file's name; the box and the button both open the picker.
$("uploadPick").onclick = $("uploadName").onclick = () => $("fileInput").click();
$("fileInput").onchange = async (e) => {
  const input = e.target;
  const file = input.files[0];
  input.value = "";
  if (!file) return;
  $("uploadName").value = file.name;
  const which = target;
  try {
    const bitmap = await createImageBitmap(file);
    setSlot(which, { label: file.name, bitmap });
  } catch {
    slotStatus(which, `Couldn't read "${file.name}" as an image.`, "err");
  }
};

// ---- Link ----
$("linkUse").onclick = () => {
  const url = $("linkUrl").value.trim();
  if (!/^https?:\/\//i.test(url)) {
    setStatus($("linkStatus"), "Paste a link starting with http:// or https://", "err");
    return;
  }
  setStatus($("linkStatus"), "");
  assignFromUrl(target, url, "linked image");
};

// Remote images go through the Switch so the browser may read their pixels.
async function assignFromUrl(which, url, label) {
  slotStatus(which, "Downloading via the Switch...");
  try {
    const r = await fetch(`/api/fetch?url=${encodeURIComponent(url)}`);
    if (!r.ok) {
      let msg = `HTTP ${r.status}`;
      try { msg = (await r.json()).error || msg; } catch { /* ignore */ }
      throw new Error(msg);
    }
    const blob = await r.blob();
    let bitmap;
    try { bitmap = await createImageBitmap(blob); }
    catch { throw new Error("That link isn't an image this browser can read."); }
    setSlot(which, { label, bitmap, url });
  } catch (e) {
    slotStatus(which, e.message, "err");
  }
}

// ---------------------------------------------------------------------------
// Slots and resizing
// ---------------------------------------------------------------------------
// Messages about picking art ("Downloading...", errors) show under the slots.
function slotStatus(which, text, kind = "") {
  $("slotError").textContent = text;
  $("slotError").className = kind === "err" ? "status err" : "muted small";
}

function setSlot(which, value) {
  const pick = { kind: "pick", ...value, preview: renderSquare(value.bitmap, 256).toDataURL("image/jpeg", 0.9) };
  draft[which] = pick;
  // Setting: copy a Home Screen pick to All Software, unless it has its own pick.
  if (which === "home" && settings.all_default === "same" && (!draft.all || draft.all.from === "auto"))
    draft.all = { ...pick, from: "auto" };
  $("slotError").textContent = "";
  renderSlots();
}

// Stretch to an exact square (no cropping), stepping down in halves so big
// sources stay sharp. Transparent areas become black (JPEG has no transparency).
function renderSquare(bitmap, size) {
  let src = bitmap;
  let w = bitmap.width;
  let h = bitmap.height;
  while (w / 2 >= size * 1.5 || h / 2 >= size * 1.5) {
    w = Math.max(size, Math.round(w / 2));
    h = Math.max(size, Math.round(h / 2));
    const step = document.createElement("canvas");
    step.width = w;
    step.height = h;
    const sctx = step.getContext("2d");
    sctx.imageSmoothingQuality = "high";
    sctx.drawImage(src, 0, 0, w, h);
    src = step;
  }
  const out = document.createElement("canvas");
  out.width = out.height = size;
  const ctx = out.getContext("2d");
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, size, size);
  ctx.imageSmoothingQuality = "high";
  ctx.drawImage(src, 0, 0, size, size);
  return out;
}

// sys-ticon ignores icons over these sizes (icon.jpg limit is for firmware 19+).
const JPEG_LIMIT = { 256: 102400, 174: 65536 };
function encodeJpeg(canvas, quality) {
  return new Promise((resolve, reject) => {
    canvas.toBlob((b) => (b ? resolve(b) : reject(new Error("Couldn't encode JPEG"))), "image/jpeg", quality);
  });
}
// Lower the quality step by step until the file fits.
async function toJpeg(slot, size) {
  const canvas = renderSquare(slot.bitmap, size);
  for (let q = 0.92; q >= 0.4; q -= 0.08) {
    const blob = await encodeJpeg(canvas, q);
    if (blob.size <= JPEG_LIMIT[size]) return blob;
  }
  throw new Error("This image is too detailed to fit the Switch's icon size limit. Try another.");
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------
function fadeStatus(el) {
  clearTimeout(statusTimer);
  statusTimer = setTimeout(() => {
    el.classList.add("fade");
    statusTimer = setTimeout(() => setStatus(el, ""), 600);
  }, 3000);
}

// Save: writes every unsaved change, then the button shows a tick for a moment.
let savedTimer = null;
$("sendBtn").onclick = async () => {
  const btn = $("sendBtn");
  const t = current;
  btn.disabled = true;
  $("slotError").textContent = "";
  try {
    for (const which of ["home", "all"]) {
      const d = draft[which];
      if (!d) continue;
      const size = which === "home" ? 256 : 174;
      const r = d.kind === "pick"
        ? await api(`/api/custom-icon/${t.id}?size=${size}`, { method: "POST", body: await toJpeg(d, size) })
        : await api(`/api/custom-icon/${t.id}?size=${size}`, { method: "DELETE" });
      if (which === "home") t.custom_icon = d.kind === "pick";
      else t.custom_icon174 = d.kind === "pick";
      draft[which] = null;
      updateChanges(r.changes);
    }
    if (detailsDirty()) {
      const b = draft.details;
      let r;
      if (emptyBoxes(b)) {
        r = await api(`/api/details/${t.id}`, { method: "DELETE" });
        t.custom_name = t.custom_publisher = t.custom_version = "";
      } else {
        // sys-ticon needs name and publisher together: fill the other with the original.
        const pair = b.name || b.publisher;
        const body = new URLSearchParams({
          name: pair ? b.name || t.name : "",
          publisher: pair ? b.publisher || t.publisher : "",
          version: b.version,
        });
        r = await api(`/api/details/${t.id}`, { method: "POST", body });
        t.custom_name = body.get("name");
        t.custom_publisher = body.get("publisher");
        t.custom_version = body.get("version");
      }
      updateChanges(r.changes);
    }
    draft.details = null;
    t._v = Date.now();
    clearTimeout(savedTimer);
    btn.classList.add("saved");
    btn.textContent = "✓";
    showSaved(true);
    savedTimer = setTimeout(() => {
      btn.classList.remove("saved");
      btn.textContent = "Save";
      btn.disabled = !isDirty();
      showSaved(false);
    }, 2000);
  } catch (e) {
    slotStatus(null, e.message, "err");
  } finally {
    renderSlots();
    renderList();
  }
};

// ---------------------------------------------------------------------------
// Changes bar, scroll-to-top, reboot
// ---------------------------------------------------------------------------
let changeCount = 0;
function updateChanges(n) {
  if (typeof n === "number") changeCount = n;
  updateBar();
}

// The bar shows Save in the editor, and the change count + Reboot once something
// has been saved. On the game list it only appears after changes.
// "Saved" replaces the middle text while the Save button shows its tick.
let savedShowing = false;
function showSaved(on) {
  savedShowing = on;
  updateBar();
}
function updateBar() {
  // Always visible. Save is faded until there's something to save; the middle
  // says "Ready" until changes are waiting for a reboot.
  const inEditor = !!current;
  document.body.classList.add("has-changes");
  if (!$("sendBtn").classList.contains("saved")) $("sendBtn").disabled = !(inEditor && isDirty());
  $("changesReady").classList.toggle("hidden", changeCount > 0 || savedShowing);
  $("changesText").classList.toggle("hidden", changeCount === 0 || savedShowing);
  $("changesSaved").classList.toggle("hidden", !savedShowing);
  $("changesCount").textContent = `${changeCount} Change${changeCount === 1 ? "" : "s"}`;
}

window.addEventListener("scroll", () => {
  $("toTop").classList.toggle("hidden", window.scrollY < window.innerHeight * 0.9);
}, { passive: true });
$("toTop").onclick = () => window.scrollTo({ top: 0, behavior: "smooth" });

// Reboot: ask first (and warn about unsaved edits in the editor).
// Progress and errors show in the button itself, so nothing around it moves.
document.querySelectorAll(".reboot-btn").forEach((btn) => {
  btn.onclick = async () => {
    const ok = await confirmDialog({
      title: "Reboot the Switch?",
      text: (changeCount > 0
        ? "Your saved changes will be visible on reboot."
        : "The Switch will restart.") + " This page will stop working until you open NX Title Editor again.",
      warn: current && isDirty() ? `Your unsaved changes to ${shownName(current)} haven't been saved and will be lost.` : "",
      yes: "Reboot",
    });
    if (!ok) return;
    btn.disabled = true;
    btn.textContent = "Rebooting...";
    try {
      await api("/api/reboot", { method: "POST" });
    } catch (e) {
      btn.textContent = "Reboot failed";
      btn.title = e.message;
      setTimeout(() => { btn.textContent = "Reboot Switch"; btn.title = ""; btn.disabled = false; }, 3000);
    }
  };
});

// ---------------------------------------------------------------------------
(async () => {
  await loadSettings();
  await loadTitles();
  showRoute();
})();
