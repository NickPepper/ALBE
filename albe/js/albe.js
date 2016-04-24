// Copyright (c) 2015 Agile Fusion. All rights reserved.
// Author: Nick Pershin

(function(win, doc, undef) {
    "use strict";

    // external dependencies
    if(win.utils === undef) {
        throw new Error("Requred module (file 'utils.js') is absent!");
    }
    if(win.common === undef) {
        throw new Error("Requred module (file 'common.js') is absent!");
    }


    // JS module pattern
    win.albe = (win.albe || (function() {

        var DOM = {
            open_ext_Btn: null,
            save_2ext_Btn: null,
            apptitle: null,
            dlopenBtn: null,
            httpmountBtn: null
        },

        init = function() {

            // link & check all needed DOM-elements

            DOM.open_ext_Btn = doc.getElementById('openextfile');
            if(!DOM.open_ext_Btn) {
                throw new Error("Element with ID 'openextfile' is not found in HTML!");
            }

            DOM.save_2ext_Btn = doc.getElementById('savetoextdir');
            if(!DOM.save_2ext_Btn) {
                throw new Error("Element with ID 'savetoextdir' is not found in HTML!");
            }

            DOM.apptitle = doc.getElementById('apptitle');
            if(!DOM.apptitle) {
                throw new Error("Element with ID 'apptitle' is not found in HTML!");
            }
            // for now this is the only one example of i18n in our app
            DOM.apptitle.innerText = chrome.i18n.getMessage("app_title");

            DOM.dlopenBtn = doc.getElementById('dlopen');
            if(!DOM.dlopenBtn) {
                throw new Error("Element with ID 'dlopen' is not found in HTML!");
            }

            DOM.httpmountBtn = doc.getElementById('httpmount');
            if(!DOM.httpmountBtn) {
                throw new Error("Element with ID 'httpmount' is not found in HTML!");
            }
        },

        proto = {

            /////////////////////
            ///// "PRIVATE" /////
            /////////////////////


            // TODO: fill in ;)


            /////////////////// "PUBLIC" ////////////////////////////
            //
            // All these methods are called from common.js module (!)
            //
            /////////////////////////////////////////////////////////


            moduleDidLoad: function() {
                // the module is not hidden by default so we can easily see if the plugin failed to load
                common.hideModule();

                // make sure we are running as an App
                if (!chrome.fileSystem) {
                    common.updateStatus('ERROR: must be run as an App!');
                    return;
                }
            },


            domContentLoaded: function(name, tc, config, width, height) {

                console.log("domContentLoaded() from albe.js");//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                
                navigator.webkitPersistentStorage.requestQuota(1024 * 1024,
                    function(bytes) {
                        common.updateStatus('Allocated ' + bytes + ' bytes of persistant storage.');
                        common.attachDefaultListeners();
                        common.createNaClModule(name, tc, config, width, height);
                    },
                    function(e) {
                        console.log('ERROR: Failed to allocate space.');
                    }
                );

                /////////// entry point ///////////
                return new init();
            },


            attachListeners: function() {
                
                console.log("attachListeners() from albe.js");//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                DOM.open_ext_Btn.addEventListener('click', function(e) {
                    chrome.fileSystem.chooseEntry({type:'openFile'}, function(entry) {
                        if (!entry) {
                            // user has cancelled the file dialog
                            console.log('User has cancelled the dialog.');
                            return;
                        }

                        // send to the NaCl module - always use Objects for this
                        common.naclModule.postMessage({
                            cmd: "ld_ext",
                            filesystem: entry.filesystem,
                            fullPath: entry.fullPath
                        });
                    });
                }, false);


                DOM.save_2ext_Btn.addEventListener('click', function(e) {
                    chrome.fileSystem.chooseEntry({type:'openDirectory'}, function(entry) {
                        if (!entry) {
                            // user has cancelled the file dialog
                            console.log('User has cancelled the dialog.');
                            return;
                        }

                        // send to the NaCl module - always use Objects for this
                        common.naclModule.postMessage({
                            cmd: "sv_ext",
                            filesystem: entry.filesystem,
                            fullPath: entry.fullPath
                        });
                    });
                }, false);


                DOM.dlopenBtn.addEventListener('click', function(e) {
                    common.naclModule.postMessage({
                        cmd: "dlopen"
                    });
                }, false);


                DOM.httpmountBtn.addEventListener('click', function(e) {
                    common.naclModule.postMessage({
                        cmd: "httpmount"
                    });
                }, false);

            },


            handleMessage: function(message_event) {

                var msg = message_event.data,
                    parts = msg.split('|'),
                    command = parts[0],
                    args = parts.slice(1);

                switch(command) 
                {
                    case 'ERR':
                        common.logMessage('\nNaCl ERROR: ' + args[0] + '\n');
                    break;

                    case 'STAT':
                        common.logMessage(args[0]);
                    break;
/*
                    case 'DISP':
                    {
                        // find the file editor that is currently visible
                        var fileEditorEl = doc.querySelector('.funcdiv:not([hidden]) > textarea');
                        // rejoin args with pipe (|) -- there is only one argument, and it can contain the pipe character
                        fileEditorEl.value = args.join('|');
                    }
                    break;

                    case 'LIST':
                    {
                        // NOTE: files with | in their names will be incorrectly split.
                        // Fixing this is left as an exercise for the reader ;)

                        var itemEl;
                        if(args.length) {
                            // add new <li> element for each file
                            for(var i = 0, l = args.length; i < l; i++) {
                                itemEl = doc.createElement('li');
                                itemEl.textContent = args[i];
                                DOM.listDirOutputEl.appendChild(itemEl);
                            }
                        } else {
                            itemEl = doc.createElement('li');
                            itemEl.textContent = '<empty directory>';
                            DOM.listDirOutputEl.appendChild(itemEl);
                        }
                    }
                    break;
*/
                    default:
                        break;
                }

            }

        };
        
        init.prototype = proto;


        // Public Interface
        return {
            moduleDidLoad: proto.moduleDidLoad,
            domContentLoaded: proto.domContentLoaded,
            attachListeners: proto.attachListeners,
            handleMessage: proto.handleMessage
        };

    })());


})(window, document, undefined);
