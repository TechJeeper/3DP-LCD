// Phone side: settings page + AppMessage bridge to the watch.
var configPage = require('./config_page');

var KEYS = [
  'BackgroundColor', 'PanelColor', 'TextColor', 'AccentColor', 'BarColor',
  'HourMode', 'ShowWeekday', 'ShowXYZ', 'ShowRemaining', 'ShowProgress',
  'FanSpin', 'ShowFeedrate'
];

var STORAGE_KEY = '3dp-lcd.settings';
var defaults = {
  BackgroundColor: 0x000055,
  PanelColor: 0x0000AA,
  TextColor: 0xFFFFFF,
  AccentColor: 0x55AAFF,
  BarColor: 0xFFFFFF,
  HourMode: 0,
  ShowWeekday: true,
  ShowXYZ: true,
  ShowRemaining: true,
  ShowProgress: true,
  FanSpin: true,
  ShowFeedrate: true
};

function loadSettings() {
  var out = {};
  var saved = null;
  try {
    saved = JSON.parse(localStorage.getItem(STORAGE_KEY));
  } catch (e) {}
  KEYS.forEach(function (k) {
    out[k] = saved && saved[k] !== undefined ? saved[k] : defaults[k];
  });
  return out;
}

function saveSettings(s) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(s));
}

function toWatchPayload(s) {
  return {
    BackgroundColor: s.BackgroundColor,
    PanelColor: s.PanelColor,
    TextColor: s.TextColor,
    AccentColor: s.AccentColor,
    BarColor: s.BarColor,
    HourMode: String(s.HourMode),
    ShowWeekday: s.ShowWeekday ? 1 : 0,
    ShowXYZ: s.ShowXYZ ? 1 : 0,
    ShowRemaining: s.ShowRemaining ? 1 : 0,
    ShowProgress: s.ShowProgress ? 1 : 0,
    FanSpin: s.FanSpin ? 1 : 0,
    ShowFeedrate: s.ShowFeedrate ? 1 : 0
  };
}

function sendSettings(s) {
  Pebble.sendAppMessage(toWatchPayload(s), function () {}, function () {});
}

function isColorWatch() {
  try {
    var info = Pebble.getActiveWatchInfo && Pebble.getActiveWatchInfo();
    if (!info || !info.platform) return true;
    return info.platform !== 'aplite' && info.platform !== 'diorite' && info.platform !== 'flint';
  } catch (e) {
    return true;
  }
}

function configUrl(settings) {
  var state = {};
  KEYS.forEach(function (k) { state[k] = settings[k]; });
  state.color = isColorWatch();
  var json = JSON.stringify(state).replace(/</g, '\\u003c');
  var returnTo = Pebble.platform === 'pypkjs' ? '$$$RETURN_TO$$$' : 'pebblejs://close#';
  var html = configPage
    .replace('__STATE__', function () { return json; })
    .replace('__RETURN_TO__', function () { return returnTo; });
  return 'data:text/html;charset=utf-8,' + encodeURIComponent(html);
}

function parseResponse(response) {
  try {
    return JSON.parse(response);
  } catch (e) {
    try {
      return JSON.parse(decodeURIComponent(response));
    } catch (e2) {
      return null;
    }
  }
}

var settings = loadSettings();

Pebble.addEventListener('ready', function () {
  settings = loadSettings();
  sendSettings(settings);
});

Pebble.addEventListener('showConfiguration', function () {
  settings = loadSettings();
  Pebble.openURL(configUrl(settings));
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) return;
  var page = parseResponse(e.response);
  if (!page) return;
  KEYS.forEach(function (k) {
    if (page[k] !== undefined) settings[k] = page[k];
  });
  saveSettings(settings);
  sendSettings(settings);
});
