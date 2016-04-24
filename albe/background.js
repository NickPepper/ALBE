// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Agile Fusion. All rights reserved.

function makeURL(toolchain, config) {
    return './index.html?tc=' + toolchain + '&config=' + config;
}

function createWindow(url) {
    console.log('loading ' + url);
    chrome.app.window.create(url, {
        width: 800,//1024,
        height: 600,//800,
        frame: 'none'
    });
}

function onLaunched(launchData) {
    // Send and XHR to get the URL to load from a configuration file.
    // Normally you won't need to do this; just call:
    //
    // chrome.app.window.create('<your url>', {...});
    //
    // We want to be able to load different URLs (for different toolchain/config combinations) from the commandline,
    // so we need to read this information from the file "run_package_config".
    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'run_package_config', true);
    xhr.onload = function() {
        var toolchain_config = this.responseText.split(' ');
        createWindow(makeURL.apply(null, toolchain_config));
    };
    xhr.onerror = function() {
        // can't find the config file? just load the default
        createWindow('./index.html');
    };
    xhr.send();
}

chrome.app.runtime.onLaunched.addListener(onLaunched);
