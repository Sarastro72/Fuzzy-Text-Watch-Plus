var alignments = {
  center: 0,
  left:   1,
  right:  2
};

var langs = {
  en: 0,
  sv: 1
};

function onReady(event) {}

function showConfiguration(event) {
  var opts = localStorage.getItem("options") || encodeURIComponent("{}");
  Pebble.openURL("http://sitr.us/apps/fuzzy-text/configurable.html#options="+opts);
}

function webviewclosed(event) {
  console.log('configuration response: '+ event.response + ' ('+ typeof event.response +')');

  var options = JSON.parse(decodeURIComponent(event.response));
  if (typeof options.invert === 'undefined' &&
      typeof options.text_align === 'undefined' &&
      typeof options.lang === 'undefined') {
    return;
  }

  localStorage.setItem("options", event.response);

  var message = {
    "0": options.invert ? 1 : 0,
    "1": alignments[options.text_align],
    "2": langs[options.lang]
  };

  console.log('sending message: '+ JSON.stringify(message));
  Pebble.sendAppMessage(message, function(event) {
    // Message delivered successfully
  }, logError);
}

function logError(event) {
    console.log('Unable to deliver message with transactionId='+
                event.data.transactionId +' ; Error is'+ event.error.message);
}

Pebble.addEventListener("ready", onReady);
Pebble.addEventListener("showConfiguration", showConfiguration);
Pebble.addEventListener("webviewclosed", webviewclosed);
