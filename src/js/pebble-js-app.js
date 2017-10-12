// Watchface config version 
var version=42;

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://www.matb.se/pebble-config/config/index.html?v=' + version;
  //var url = 'file:///Users/mattias/gitroot/Fuzzy-Text-watch-Plus/config/index.html?v=' + version;

  if (!hasColor()) { // Black n white
    url = url + "&bw";
  }

  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  var dict = {};
  // Inverse BW colors
  dict['KEY_INVERSE'] = configData['inverse_colors'] ? 1 : 0;  // Send a boolean as an integer
  // Background color
  var argb = hexColorToARGB2222(configData['background_color']);
  if (argb > 0) dict['KEY_BACKGROUND'] = argb;
  // Regular text color
  argb = hexColorToARGB2222(configData['regular_color']);
  if (argb > 0) dict['KEY_REGULAR_TEXT'] = argb;
  // Bold text color
  argb = hexColorToARGB2222(configData['bold_color']);
  if (argb > 0) dict['KEY_BOLD_TEXT'] = argb;

  // Language
  var lang = configData['language'];
  if (lang > 0) dict['KEY_LANGUAGE'] = parseInt(lang);

  // Time offset
  var offset = configData['offset'];
  if (offset !== undefined) dict['KEY_OFFSET'] = parseInt(offset);

  var message_time = configData['message_time'];
  if (message_time !== undefined) dict['KEY_MESSAGE_TIME'] = parseInt(message_time);

  var gesture = configData['gesture'];
  if (gesture !== undefined) dict['KEY_GESTURE'] = parseInt(gesture);

  var bt_notification = configData['bt_notification'];
  if (bt_notification !== undefined) dict['KEY_BT_NOTIFICATION'] = parseInt(bt_notification);

  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function(e) {
    console.log('Send failed! ' + JSON.stringify(e) + ', ' + JSON.stringify(dict));
  });
});

function hexColorToARGB2222(color) {
  if (color.length == 8) { // Expect "0xRRGGBB"
    var r = parseInt(color.substring(2,3), 16) >> 2;
    var g = parseInt(color.substring(4,5), 16) >> 2;
    var b = parseInt(color.substring(6,7), 16) >> 2;

    var col = 3;  //alpha
    col = (col << 2) + r;
    col = (col << 2) + g;
    col = (col << 2) + b;
    return col;
  }

  return 0;
}

function hasColor() {
  var has_color = false;
 
  if(Pebble.getActiveWatchInfo) {
    var platform = Pebble.getActiveWatchInfo().platform;
    if (platform.indexOf("basalt") >= 0
      || platform.indexOf("chalk") >= 0
      || platform.indexOf("emery") >= 0) {
      has_color = true;
    }
  }
  
  return has_color;
}
