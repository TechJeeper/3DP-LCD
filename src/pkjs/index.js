// Clay generates a real config page (matches the Pebble app style) and
// relays saved values to the watch over AppMessage.
var Clay = require('@rebble/clay');
var clayConfig = require('./config');

// auto: true → handles showConfiguration / webviewclosed / sendAppMessage
var clay = new Clay(clayConfig, null, { auto: true });
