// Common stuff for application which uses NaCl plugins.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Nick Pershin. All rights reserved.

(function(win, doc, undef) {
    "use strict";

    // external dependency
    if(win.utils === undef) {
        throw new Error("Requred module (file 'utils.js') is absent!");
    }

    // JS module pattern
    win.common = (win.common || (function() {

        var settings = {
            isTest: false,// set to true when the Document is loaded IF "test=true" is in the query string
            isRelease: true,// set to true when loading a "Release" NaCl module, false when loading a "Debug" NaCl module
            kMaxLogMessageLength: 20,// max length of logMessageArray
            logMessageArray: [],// an array of messages to display in the DOM-element linked to settings.logElem
            statusText: 'NO-STATUSES',// saved text to display in the DOM-element linked to settings.statusElem
            defaultMessageTypes: {},
            listenerElem: null,
            statusElem: null,
            logElem: null
        },


        init = function() {

            // link & check all needed DOM-elements
            settings.logElem = doc.getElementById('logField');
            if(!settings.logElem) {
                throw new Error("Element with ID 'logField' is not found in HTML!");
            }

            settings.statusElem = doc.getElementById('statusField');
            if(!settings.statusElem) {
                throw new Error("Element with ID 'statusField' is not found in HTML!");
            }

            settings.listenerElem = doc.getElementById('listener');
            if(!settings.listenerElem) {
                throw new Error("Element with ID 'listener' is not found in HTML!");
            }


            var loadFunction,
                searchVars = {},
                dataset = doc.body.dataset;// the data-* attributes on the body

            if(!dataset) {
                throw new Error("No dataset is provided in <BODY>!");
            }

            if(!dataset.customLoad) {
                loadFunction = common.domContentLoaded;
            } else if(albe.domContentLoaded) {
                loadFunction = albe.domContentLoaded;
            }

            // populate defaultMessageTypes
            settings.defaultMessageTypes = {
                'alert': alert,
                'log': shprot.logMessage
            };


            if(win.location.search.length > 1) {
                var pairs = win.location.search.substr(1).split('&'),
                    keyValue;
                for(var key_ix = 0, l = pairs.length; key_ix < l; key_ix++) {
                    keyValue = pairs[key_ix].split('=');
                    searchVars[unescape(keyValue[0])] = (keyValue.length > 1) ? unescape(keyValue[1]) : '';
                }
            }


            if(loadFunction) {

                var toolchains = dataset.tools.split(' '),
                    configs = dataset.configs.split(' '),
                    attrs = {},
                    config;

                if(dataset.attrs) {
                    var attr_list = dataset.attrs.split(' '),
                        attr;
                    for(var key in attr_list) {
                        attr = attr_list[key].split('=');
                        attrs[attr[0]] = attr[1];
                    }
                }

                // If the config value is included in the search vars, use that.
                // Otherwise default to Release if it is valid, or the first value if Release is not valid.
                if(configs.indexOf(searchVars.config) !== -1) {
                    config = searchVars.config;
                } else if(configs.indexOf('Release') !== -1) {
                    config = 'Release';
                } else {
                    config = configs[0];
                }

                var tc = toolchains.indexOf(searchVars.tc) !== -1 ? searchVars.tc : toolchains[0],
                    pathFormat = dataset.path,
                    path = pathFormat.replace('{tc}', tc).replace('{config}', config);

                settings.isTest = searchVars.test === 'true';
                settings.isRelease = path.toLowerCase().indexOf('release') != -1;

                // We use a non-zero sized embed to give Chrome some space
                // to place the 'bad plug-in' graphic, if there is a problem.
                dataset.width = dataset.width || 200;
                dataset.height = dataset.height || 200;

                loadFunction(dataset.name, tc, path, dataset.width, dataset.height, attrs);
            }

        },


        shprot = {
            
            //////////////////////
            ///// "PRIVATE" //////
            //////////////////////


            isHostToolchain: function(tool) {
                return tool == 'win' || tool == 'linux' || tool == 'mac';
            },


            /**
             * Return true when |s| starts with the string |prefix|.
             *
             * @param {string} s The string to search.
             * @param {string} prefix The prefix to search for in |s|.
             */
            startsWith: function(s, prefix) {
                // indexOf would search the entire string, lastIndexOf(p, 0) only checks at the first index
                return s.lastIndexOf(prefix, 0) === 0;
            },


            /**
             * Return the mime type for NaCl plugin.
             *
             * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
             * @return {string} The mime-type for the kind of NaCl plugin matching the given toolchain.
             */
            mimeTypeForTool: function(tool) {

                // for NaCl modules use application/x-nacl
                var mimetype = 'application/x-nacl';

                if(shprot.isHostToolchain(tool)) {
                    // for non-NaCl PPAPI plugins use the x-ppapi-debug/release mime type
                    mimetype = (settings.isRelease) ? 'application/x-ppapi-release' : 'application/x-ppapi-debug';
                } else if(tool == 'pnacl') {
                    mimetype = 'application/x-pnacl';
                }
                return mimetype;
            },


            /**
             * Check if the browser supports NaCl plugins.
             *
             * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
             * @return {bool} True if the browser supports the type of NaCl plugin produced by the given toolchain.
             */
            browserSupportsNaCl: function(tool) {

                // Assume host toolchains always work with the given browser.
                // The below mime-type checking might not work with --register-pepper-plugins.
                if(shprot.isHostToolchain(tool)) {
                    return;
                }
                var mimetype = shprot.mimeTypeForTool(tool);
                return navigator.mimeTypes[mimetype] !== undef;
            },


            /**
             * Inject a script into the DOM, and call a callback when it is loaded.
             *
             * @param {string} url The url of the script to load.
             * @param {Function} onload The callback to call when the script is loaded.
             * @param {Function} onerror The callback to call if the script fails to load.
             */
            injectScript: function(url, onload, onerror) {

                var scriptEl = utils.node('script', {attr:{'type':'text/javascript', 'src':url}});
                scriptEl.onload = onload;
                if(onerror) {
                    scriptEl.addEventListener('error', onerror, false);
                }
                doc.head.appendChild(scriptEl);
            },


            /**
             * Run all tests for this application.
             *
             * @param {Object} moduleEl The module DOM element.
             */
            runTests: function(moduleEl) {

                console.log('runTests()');
                common.tester = new Tester();//TODO: WRITE the Tester object (!)

                // All NaCl SDK apps are OK if the app exits cleanly; (i.e. the NaCl module returns 0 or calls exit(0)).
                // Without this exception, the browser_tester thinks that the module has crashed.
                common.tester.exitCleanlyIsOK();

                common.tester.addAsyncTest('loaded', function(test) {
                    test.pass();
                });

                win.addTests && win.addTests();

                common.tester.waitFor(moduleEl);
                common.tester.run();
            },


            /**
             * Called when the NaCl module fails to load.
             * This event listener is registered in createNaClModule.
             */
            handleError: function(event) {
                // we can't use common.naclModule yet because the module has not been loaded
                var moduleEl = doc.getElementById('nacl_module');
                shprot.updateStatus('ERROR [' + moduleEl.lastError + ']');
            },


            /**
             * Called when the NaCl module sends a message to JavaScript (via PPB_Messaging.PostMessage())
             * This event listener is registered in createNaClModule.
             *
             * @param {Event} msg_event A message event. msg_event.data contains the data sent from the NaCl module.
             */
            handleMessage: function(msg_event) {

                var mdata = msg_event.data;
                if(typeof mdata === 'string') {
                    for(var type in settings.defaultMessageTypes) {
                        if(settings.defaultMessageTypes.hasOwnProperty(type)) {
                            if(shprot.startsWith(mdata, type + ':')) {
                                func = settings.defaultMessageTypes[type];
                                func(mdata.slice(type.length + 1));
                                return;
                            }
                        }
                    }
                }

                if(albe.handleMessage) {
                    albe.handleMessage(msg_event);
                } else {
                    shprot.logMessage('Unhandled message: ' + mdata);
                }
            },


            /**
             * Called when the Browser can not communicate with the Module
             * This event listener is registered in attachDefaultListeners.
             */
            handleCrash: function(event) {

                if(common.naclModule.exitStatus == -1) {
                    shprot.updateStatus('CRASHED');
                } else {
                    shprot.updateStatus('EXITED [' + common.naclModule.exitStatus + ']');
                }
                win.handleCrash && win.handleCrash(common.naclModule.lastError);
            },


            /**
             * Called when the NaCl module is loaded.
             * This event listener is registered in attachDefaultListeners.
             */
            moduleDidLoad: function() {
                common.naclModule = doc.getElementById('nacl_module');
                shprot.updateStatus('RUNNING');
                if(win.albe === undef) {
                    throw new Error("ALBE module (file 'albe.js') is undefined while moduleDidLoad() in common.js!");
                }
                albe.moduleDidLoad && albe.moduleDidLoad();
            },



            ////////////////////
            ///// "PUBLIC" /////
            ////////////////////


            /**
             * Called when the DOM content has loaded; i.e. the page's document is fully parsed.
             * At this point, we can safely query any elements in the document via document.querySelector,
             * document.getElementById, etc.
             *
             * @param {string} name The name of the example.
             * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
             * @param {string} path Directory name where .nmf file can be found.
             * @param {number} width The width to create the plugin.
             * @param {number} height The height to create the plugin.
             * @param {Object} attrs Optional dictionary of additional attributes.
             */
            domContentLoaded: function(name, tool, path, width, height, attrs) {
                // If the page loads before the Native Client module loads,
                // then set the status message indicating that the module is still loading.
                // Otherwise, do not change the status message.
                shprot.updateStatus('Page loaded.');
                if(!shprot.browserSupportsNaCl(tool)) {
                    shprot.updateStatus('Browser does not support NaCl (' + tool + '), or NaCl is disabled');
                } else if(common.naclModule == null) {
                    shprot.updateStatus('Creating embed: ' + tool);
                    shprot.attachDefaultListeners();
                    shprot.createNaClModule(name, tool, path, width, height, attrs);
                } else {
                    // It's possible that the Native Client module onload event fired before the page's onload event.
                    // In this case, the status message will reflect 'SUCCESS', but won't be displayed.
                    // This call will display the current message.
                    shprot.updateStatus('Waiting.');
                }
            },


            /**
             * Create the Native Client <embed> element as a child of the DOM element linked to settings.listenerElem.
             *
             * @param {string} name The name of the application.
             * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
             * @param {string} path Directory name where .nmf file can be found.
             * @param {number} width The width to create the plugin.
             * @param {number} height The height to create the plugin.
             * @param {Object} attrs Dictionary of attributes to set on the module.
             */
            createNaClModule: function(name, tool, path, width, height, attrs) {

                var moduleEl = utils.node('embed', {attr:{
                    'name': 'nacl_module',
                    'id': 'nacl_module',
                    'width': width,
                    'height': height,
                    'path': path,
                    'type': shprot.mimeTypeForTool(tool),
                    'src': path+'/'+name+'.nmf'
                }});

                // add any optional arguments
                if(attrs) {
                    for(var key in attrs) {
                        moduleEl.setAttribute(key, attrs[key]);
                    }
                }

                // The <EMBED> element is wrapped inside a <DIV>,
                // which has both 'load' and 'message' event listeners attached.
                // This wrapping method is used instead of attaching the event listeners directly to the <EMBED> element
                // to ensure that the listeners are active before the NaCl module 'load' event fires.
                settings.listenerElem.appendChild(moduleEl);

                // Request the offsetTop property to force a relayout. As of Apr 10, 2014
                // this is needed if the module is being loaded on a Chrome App's background page (see crbug.com/350445).
                moduleEl.offsetTop;

                // Host plugins don't send a moduleDidLoad message. We'll fake it here.
                if(shprot.isHostToolchain(tool)) {
                    win.setTimeout(function() {
                        moduleEl.readyState = 1;
                        moduleEl.dispatchEvent(new CustomEvent('loadstart'));
                        moduleEl.readyState = 4;
                        moduleEl.dispatchEvent(new CustomEvent('load'));
                        moduleEl.dispatchEvent(new CustomEvent('loadend'));
                    }, 100);
                }

                // this code is only used to test the SDK
                if(settings.isTest) {
                    var loadNaClTest = function() {
                        shprot.injectScript('nacltest.js', function() {
                            shprot.runTests(moduleEl);
                        });
                    };

                    // try to load test.js for the example. Whether or not it exists, load nacltest.js.
                    shprot.injectScript('test.js', loadNaClTest, loadNaClTest);
                }
            },


            /**
             * Hide the NaCl module's embed element.
             *
             * We don't want to hide by default; if we do, it is harder to determine that a plugin failed to load.
             * Instead, call this function inside the application's "moduleDidLoad" function.
             */
            hideModule: function() {
                // Setting common.naclModule.style.display = "none" doesn't work;
                // the module will no longer be able to receive postMessages.
                common.naclModule.style.height = '0';
            },


            /**
             * Remove the NaCl module from the page.
             */
            removeModule: function() {
                common.naclModule.parentNode.removeChild(common.naclModule);
                common.naclModule = null;
            },


            /**
             * Add the default "load" and "message" event listeners to the element linked to settings.listenerElem.
             *
             * The "load" event is sent when the module is successfully loaded.
             * The "message" event is sent when the naclModule posts a message using
             * PPB_Messaging.PostMessage() (in C) or pp::Instance().PostMessage() (in C++).
             */
            attachDefaultListeners: function() {

                var l = settings.listenerElem;
                l.addEventListener('load', shprot.moduleDidLoad, true);
                l.addEventListener('message', shprot.handleMessage, true);
                l.addEventListener('error', shprot.handleError, true);
                l.addEventListener('crash', shprot.handleCrash, true);
                albe.attachListeners && albe.attachListeners();
            },


            /**
             * Add a message to the DOM-element linked to settings.logElem.
             * This function is used by the default "log:" message handler.
             *
             * @param {string} message The message to log.
             */
            logMessage: function(message) {

                var msgar = settings.logMessageArray;
                msgar.push(message);
                if(msgar.length > settings.kMaxLogMessageLength) {
                    msgar.shift();            
                }

                settings.logElem.textContent = msgar.join('\n');
                console.log(message);
            },


            /**
             * Set the global status message.
             * If the element linked to settings.statusElem exists, set its HTML to the status message as well.
             *
             * @param {string} opt_message The message to set. If null or undefined, then
             *     set element 'statusElem' to the message from the last call to updateStatus.
             */
            updateStatus: function(opt_message) {

                if(opt_message) {
                    console.log(opt_message);
                    settings.statusText = opt_message;
                }
                settings.statusElem.innerHTML = settings.statusText;
            }

        };


        init.prototype = shprot;


        doc.addEventListener('DOMContentLoaded', function(e) {
            return new init();
        },false);


        // Public Interface
        return {
            naclModule: null,// reference to the NaCl module once it is loaded
            attachDefaultListeners: shprot.attachDefaultListeners,
            domContentLoaded: shprot.domContentLoaded,
            createNaClModule: shprot.createNaClModule,
            hideModule: shprot.hideModule,
            removeModule: shprot.removeModule,
            logMessage: shprot.logMessage,
            updateStatus: shprot.updateStatus
        };


    })());


})(window, document, undefined);
