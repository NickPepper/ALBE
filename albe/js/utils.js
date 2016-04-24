/**
 * @file Utils
 */

(function(window, document, undefined) {
	'use strict';

	/**
	 * @author Alexander Kolobov
	 */
	window.Pish = (function(pish) {
		var prop, inst;

		/**
		 * Public api
		 * @param  {Object} api    Api properties or methods
		 * @return {Object}        Api object
		 */
		pish.prototype.api = function(api) {
			function Api(){};
			Api.prototype = pish.prototype;
			Api.prototype.constructor = Api;
			Api = new Api();
			for(prop in api) {Api[prop] = api[prop].bind(this);}
			return Api;
		};
		
		/**
		 * Create a class or an object with inheritance from class or object
		 * @param  {Object} Pish Constructor
		 * @param  {Object} prot Prototype properties and methods
		 * @param  {Object} face Interfcae properties or methods
		 * @return {Object}      Object or constructor
		 */
		pish.prototype.pish = function(Pish, prot, face) {
			inst = false;
			(Pish && Pish.constructor && Pish.call && Pish.apply) || (prot = 0, inst = face = Pish, Pish = function(){});
			function Helper(){};
			Helper.prototype = this instanceof Function ? this.prototype : this;
			Pish.prototype = new Helper();
			Pish.prototype.constructor = inst ? pish : Pish;
			Pish.prototype.parent = this;
			if(prot) for(prop in prot) Pish.prototype[prop] = prot[prop];
			if(inst) Pish = new Pish();
			if(face) for(prop in face) Pish[prop] = face[prop];
			return Pish;
		};

		return pish;
	})(function Pish(){
		return this.pish.apply(this, Array.prototype.slice.call(arguments));
	});

	/**
	 * @author Alexander Kolobov (except 'trim' which is written by Nick Pershin)
	 */
	window.utils = {

		debug: (function () {
			var inited = false,
				rows = {},
				elem = document.createElement('div');
			elem.style.cssText = 'font:11px/13px Arial;text-align:left;position:fixed;left:0;top:0;'+
				'padding:6px;z-index:99999;background:#000;opacity:0.6;color:#fff;';
			function init() {
				document.body ? document.body.appendChild(elem) : document.addEventListener("DOMContentLoaded", function(event) {
		    		document.body.appendChild(elem);
		  		});
			}
			function dump(rows, level) {
				var row = null,
					frag = document.createDocumentFragment();
				for (var prop in rows) {
					row = document.createElement('div');
					row.style.paddingLeft = level*10+'px';
					if ((!!rows[prop]) && (rows[prop].constructor === Object)) {
						row.textContent = prop.toLowerCase()+': ';
						row.appendChild(dump(rows[prop], level+1));
					} else {
						row.textContent = prop.toLowerCase()+': '+rows[prop].toString();
					} 
					frag.appendChild(row);
				}
				return frag;
			}
			return function (props) {
				if (!inited) init();
				for (var prop in props) {
					if (props[prop] == null) {
						try{delete rows[prop];}catch(e){};
					} else {
						rows[prop] = props[prop];
					}
				}
				elem.innerHTML = '';
				elem.appendChild(dump(rows, 0));
			}
		})(),
		
		isios: ~['iPod','iPhone','iPad','iPod Simulator','iPhone Simulator','iPad Simulator'].indexOf(navigator.platform),

		/**
		 * Convert number to string representation
		 * @param  {HTMLElement/Number} elem  [description]
		 * @param  {Array} arr   ['{N} файл','{N} файла','{N} файлов']
		 */
		numeric : function(elem, arr, force){
			function num(num) {
				num = parseInt(num, 10);
				if(!num){
					return arr[2]
				}else{
					if(num%1){
						return arr[1]
					}else{
						if(/(11|12|13|14|5|6|7|8|9|0)$/.test(num)){
							return arr[2]
						}else{
							if(/1$/.test(num)){
								return arr[0]
							}else{
								if(/(2|3|4)$/.test(num)){
									return arr[1]
								}
							}
						}
					}
				}
			}
			if(typeof elem == 'number') {
				return num(elem).replace(/\{N\}/,elem||0);
			} else if(typeof elem == 'string') {
				Array.prototype.slice.call(utils.find(document.boody, elem)).forEach(function(elem, content) {
					if(force || !elem.numeric_processed) {
						elem.numeric_processed = true;
						content = elem.innerHTML;
						elem.innerHTML = num(elem.innerHTML).replace(/\{N\}/,elem.innerHTML||0);
					}
				});
			}
		},

		/**
		 * Multiline strings realization for javascript
		 * ex: utils.ml(function(){\/*
		 * 		{var1}
		 * 		string
		 * 		here
		 * *\/}, {
		 * 		var1: 'multiline'
		 * });
		 * @param  {Function}
		 * @param {Object} Optional, variables to be replaced
		 * @return {String}
		 */
		ml: function(m, v) {
			return ( m = (m + '').match(/^[^*]*\*((\s|.)*)\*\/\s*\;?\s*\}$/)) ? (
					v ? m[1].replace(/{(\w+)}/g, function(a, s) {
						return v[s] !== void(0) ? v[s] : s;
					}) : m[1]
				) : '';
		},

		/**
		 * gets CSS 3 specific prefix
		 * @author Nick Pershin, WWW
		 * @param  {Object}
		 * @return {String}
		 *
		 * @usage_example utils.getVendorsPrefix(doc.body.style)
		 */
		getVendorsPrefix: function(ob){
			var v='';
			['moz','webkit','khtml','o','ms'].forEach(function(i){
				if((i+'Transform') in ob){
					v='-'+i+'-';
				}
			});
			return v;
		},

		/**
		 * Wrapper for xmlHTTPRequest
		 * @param  {Object} params {url:'://' [,data:'prop=val&...']
		 *							[,method:'GET/POST']
		 *							[,response_type:'TEXT/XML']
		 *							[, headers: [{n:'Header-Name', v:'Header-Value'}, ...] ]
		 *                          [,timeout:30000]
		 *							[,success:function({status,response}){ utils context }]
		 *                          [,error:function({status,response}){ utils context }]}
		 * @return {xmlHTTPRequest}
		 *
	 	 * @author Alexander Kolobov
	 	 * @editor Nick Pershin (response_type added, headers added & other minor improvements)
		 */
		ajax_default_timeout: 30000,
		ajax : function(params) {
			var factories = [
				function(){return new ActiveXObject("Msxml2.XMLHTTP");},
				function(){return new ActiveXObject("Msxml3.XMLHTTP");},
				function(){return new ActiveXObject("Microsoft.XMLHTTP");},
				function(){return new XMLHttpRequest();}
				],
				count = factories.length,
				XMLHttp = false;
			while(count--) {
				try{
					XMLHttp = factories[count]();
				} catch(e) {
					continue;
				}
				break;
			}
			if(!XMLHttp){return;}
			params.method = params.method ? params.method.toUpperCase() : 'GET';
			params.response_type = params.response_type ? params.response_type.toUpperCase() : 'TEXT';
			if(params.method == 'GET') {
				params.url = params.url+(params.data ? '?'+params.data : '');
				params.data = null;
			} else {
				if(!params.data)params.data = null;
			}
			params.timeout = params.timeout || this.ajax_default_timeout;
			XMLHttp.open(params.method, params.url, true);
			XMLHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
			if(params.headers) {
				for(var i = 0, l = params.headers.length; i < l; i++) {
					XMLHttp.setRequestHeader(params.headers[i].n, params.headers[i].v);
				}
			}
			params.timeout = setTimeout(function(){ XMLHttp.abort(); params.error && params.error({url:params.url, status:408, message:'Request timeout'});}, params.timeout);
			XMLHttp.onreadystatechange = function() {
				if(XMLHttp.readyState != 4){return;}
				if(XMLHttp.status == 200) {
					params.timeout && clearTimeout(params.timeout);
					params.success && params.success({status:200,response:(params.response_type == 'TEXT') ? XMLHttp.responseText : XMLHttp.responseXML});
				} else {
					params.timeout && clearTimeout(params.timeout);
					params.error && params.error({url:params.url, status:XMLHttp.status});
				}
			};
			XMLHttp.send(params.data);
			return XMLHttp;
		},

		/**
		 * Convert json string to object
		 * @param  {String} json
		 * @return {Object}
		 */
		parseJson: function(json) {
			try{
				json = new Function('return '+json+';')();
			}catch(e){}
			return typeof json === "object" ? json : null;
		},

		/**
		 * XML to JSON parser
		 * Support attributes, numbers, dates, booleans, arrays
		 * @param  {string} data XML file
		 * @return {object}      Converted XML
		 */
		xmlParse: function(data) {
			try{
				return (function loop( obj, node, parent, key ) {
					var nodes = node.childNodes,
						temp = [],
						counter = 0;
					for( ; counter<nodes.length ; counter++ ){
						if( nodes[counter].nodeType === 1 ){
							temp.push(nodes[counter]);
						}
					}
					nodes = temp;
					counter = 0;
					if (nodes.length) {
						for ( ; counter<nodes.length ; counter++ ) {
							temp = nodes[counter].tagName;
							if (obj.hasOwnProperty(temp)) {
								if (obj[temp] instanceof Array === true) {
									obj[temp].push({});
								} else {
									obj[temp] = [obj[temp],{}];
								}
								loop(obj[temp][obj[temp].length-1],nodes[counter],obj[temp],obj[temp].length-1);
							} else {
								obj[temp] = {};
								loop(obj[temp],nodes[counter],obj,temp);
							}
						}
					} else if (parent) { 
						temp = node.textContent;
						if ( temp.toLowerCase() === 'true' ) {
							temp = true;
						} else if ( temp.toLowerCase() === 'false' ) {
							temp = false;
						} else if( (temp * 1) + '' === temp ) {
							temp = temp * 1;
						} else if( temp == '' ) {
							temp = null;
						} else {
							var date = Date.parse(temp);
							if(!isNaN(date)){
								temp = new Date();
								temp.setTime(date);
							}
						}
						parent[key] = temp;
					}
					if( node.attributes && node.attributes.length ){
						counter = 0;
						if ( typeof parent[key] !== 'object' || parent[key] === null ) {
							temp = {};
							temp['#value'] = parent[key];
							parent[key] = temp;
						}
						for( ; counter<node.attributes.length ; counter++ ) {
							parent[key]['-'+node.attributes[counter].nodeName] = node.attributes[counter].nodeValue;
						}
					}
					return obj;
				})({}, new DOMParser().parseFromString(data,"text/xml"));
			}catch(e){
				return null;
			}
		},

		/**
		 * Construct a child of queue object to execute sync and async functions in one stack
		 * 
		 * For example:
		 * 
		 * var queue = new utils.queue();
		 * queue.repeat(2).sync(function() { // this queue will be repeated two times
		 * 		return '1'; // the result of sync functions you should return as usual
		 * }).async(function() {
		 * 		queue.result(); // to get the result of previous function call .result()
		 * 		setTimeout(function(){
		 * 			queue.result(); // result will be safe until you return new result
		 * 			queue.execute(); // you CAN'T execute queue while it is executing
		 * 			queue.reset().execute(); // but you can reset and then execute it from the beginning
		 *   		queue.proceed('2'); // the result of async functions you should pass to .proceed() as argument
		 *      }, 500);
		 * }).sleep(
		 * 		1000 // sleep function will pause all queue for requested amount of milliseconds
		 * 	).callback(function() { // callback fucntion will be called only once
		 * 		queue.result(); // this will return result of last executed function
		 * }).execute();
		 *
		 * // After execution you can execute queue once more
		 * queue.execute();
		 * // or clean it and add another functions
		 * queue.clean().sync( ...
		 * 
		 */
		queue: (function(_){
			return(_=function(){
				this.clean();
			}).prototype={
				/**
				 * Add async function in queue
				 * Somewhere in this function you must run .proceed() method to go further
				 * @param  {Function} callback Function to be executed
				 * @return {Object} 		          
				 */
				async: function(callback) {
					if(typeof callback === 'undefined') {callback = function(){};}
					if(typeof callback === 'function') {
						callback.waitForAsyncProceed = callback.waitForAsyncResult = true;
						this._queue.push(callback);
					}
					return this;
				},
				/**
				 * Add function in queue
				 * Queue will wait for return from this function before going to the next step
				 * @param  {Function} callback Function to be executed
				 * @return {Object}            
				 */
				sync: function(callback) {
					if(typeof callback === 'function') {this._queue.push(callback);}
					return this;
				},
				/**
				 * Set the number of repetitions of queue
				 * @param  {Number} times
				 * @return {Object}       
				 */
				repeat: function(repeat) {
					if(repeat == repeat|0) {this._repeat = Math.max(repeat-1,0);}
					this._counter = this._repeat;
					return this;
				},
				/**
				 * Set the callback, which will be called when queue finish execution
				 * @param  {Function} callback 
				 * @return {Object}
				 */
				callback: function(callback) {
					if(typeof callback === 'function') {this._callback = callback;}
					return this;
				},
				/**
				 * Start execution of queue
				 * @return {Object}
				 */
				execute: function() {
					if(!this._execution) {
						this._execution = true;
						this.proceed();
					}
					return this;
				},
				/**
				 * Stop execution and reset queue to initial state
				 * @return {Object}
				 */
				reset: function () {
					this._cursor = 0;
					this._execution = false;
					this._counter = this._repeat;
					this._result = this.undefined;
					return this;
				},
				/**
				 * Reset to initial state and clean stack and callback
				 * @return {Object}
				 */
				clean: function() {
					this._queue = [];
					this._repeat = 0;
					this._counter = 0;
					this.reset();
					return this;
				},
				/**
				 * Add a "sleep" function to stack
				 * @param  {Number} time Time to sleep in milliseconds
				 * @return {Object}
				 */
				sleep: function(time) {
					this.async(function(time){setTimeout(this.proceed.bind(this), time)}.bind(this, time));
					this._queue[this._queue.length-1].waitForAsyncResult = false;
					return this;
				},
				/**
				 * Stop queue, call global callback and reset queue
				 * @return {Object}
				 */
				stop: function() {
					this._cursor = this._queue.length;
					this._counter = 0;
					return this;
				},
				/**
				 * Set or get current cursor
				 * @param  {integer} step
				 * @return {Object}
				 */
				setCursor: function(cursor) {
					this._cursor = Math.min(this._queue.length, Math.max(0,cursor));
					return this;
				},
				getCursor: function() {
					return this._cursor;
				},
				/**
				 * Function which will switch queue to the next tick
				 * @return {Object}
				 */
				proceed: function() {
					if(this._execution) {
						if (this._cursor>0 && this._queue[this._cursor-1].waitForAsyncResult) {
							this._result = arguments[0];
						}
						if(this._cursor === this._queue.length) {
							if(this._counter) {
								this._counter--;
								this._cursor = 0;
							} else {
								this._callback && this._callback(this);
								return this.reset();
							}
						}
						if(this._queue[this._cursor].waitForAsyncProceed) {
							this._queue[this._cursor++](this);
						} else {
							this._result = this._queue[this._cursor++](this);
							this.proceed();
						}
					}
					return this;
				},
				/**
				 * Return result of last executed function
				 * @return {Any}
				 */
				result: function() {
					return this._result;
				}
		},_})(),

		/**
		 * @param target can be any DOM Element or other EventTarget
		 * @param type Event type (i.e. 'click')
		 * @param doc Placeholder for document
		 * @param event Placeholder for creating an Event
		 */
		trigger: function (target, type, doc, event) {
			doc = document;
			if (doc.createEvent) {
			event = new Event(type);
			target.dispatchEvent(event);
			} else {
			event = doc.createEventObject();
			target.fireEvent('on' + type, event);
			}
		},

		/**
		 * Create copy of first object and merge props and methods from second
		 * object into it, avoid duplicates
		 * @param  {object} obj       
		 * @param  {object} extension
		 * @return {object}          
		 */
		merge: function (obj, extension) {
			var prop = null,
				result = {};
			for(prop in obj) {result[prop] = obj[prop];}
			for(prop in extension) {if(typeof result[prop] === 'undefined') {result[prop] = extension[prop];}}
			return result;
		},

		/**
		 * Extend first object with props and methods from second, avoid duplicates
		 * @param  {object} obj       
		 * @param  {object} extension
		 * @return {object}          
		 */
		extend: function(obj, extension) {
			var prop = null;
			for(prop in extension) {if(typeof obj[prop] === 'undefined') {obj[prop] = extension[prop];}}
			return obj;
		},

		/**
		 * Parse query params string into object
		 * @param  {string} str optional, query string
		 * @return {object}
		 */
		parseParams: function(str) {
			var pair,
				result = {},
				counter,
				length;
			str = str || location.search;
			str = str.substr(1).split('&');
			counter = 0;
			length = str.length;
			for( ; counter < length ; counter++ ) {
				pair = str[counter].split('=');
				result[decodeURIComponent(pair[0])] = decodeURIComponent(pair[1]);
			}
			return result;
		},

		/**
		 * Check long chains
		 */
		getChain: function(context) {
			var chain = Array.prototype.slice.call(arguments,1).reverse(),
				counter = chain.length;
			while (counter--) {
				if (typeof context[chain[counter]] === 'undefined') return null;
				context = context[chain[counter]];
			}
			return context;
		},

		/**
		 * Hash key/value manager
		 */
		hash: {
			_handlers: [],
			_objCache: null,
			_strCache: null,
			_inited: false,
			_dropCache: function () {
				this._objCache = this.get();
				this._strCache = location.hash;
			},
			_fire: function () {
				var current = this.get(),
					prop = null,
					changed = false,
					changes = {};
				for (prop in current) {if(typeof this._objCache[prop] === 'undefined' || this._objCache[prop] != current[prop]) {changes[prop] = current[prop];changed=true;}}
				for (prop in this._objCache) {if(typeof current[prop] === 'undefined') {changes[prop] = null;changed=true;}}
				if (!changed) return;
				this._dropCache();
				for (prop in this._handlers) { this._handlers[prop] && this._handlers[prop](changes, current); }
			},
			addListener: function (handler) {
				this.init();
				handler && this._handlers.push(handler);
				return this;
			},
			removeListener: function (handler) {
				var prop = null;
				for (prop in this._handlers) { if(this._handlers[prop] == handler) { delete this._handlers[prop]; } }
				return this;
			},
			set: function (key, val) {
				var prop = null, hash = [], subhash = this.get();
				if (typeof val === 'undefined') {
					for (prop in key) {
						if (key[prop] == null) {
							delete subhash[prop];
						} else {
							subhash[prop] = key[prop];
						}
					}
				} else {
					if (val == null) {
						delete subhash[key];
					} else {
						subhash[key] = val;
					}
				}
				for (prop in subhash) {
					hash.push(prop+'='+encodeURIComponent(subhash[prop])); 
				}
				location.hash = hash.join('&');
				return this;
			},
			get: function (hash) {
				hash = hash ? hash+'' : location.hash;
				hash = hash[0] == '#' ? hash.substr(1) : hash;
				hash = hash.split('&');
				var result = {},
					index = -1,
					counter = 0,
					total = hash.length;
				for (;counter<total;counter++) {
					index = hash[counter].indexOf('=');
					if (index<0) continue;
					result[hash[counter].substr(0,index)] = decodeURIComponent(hash[counter].substr(index+1));
				}
				return result;
			},
			init: function(){
				if(this._inited)return;
				this._dropCache();
				if ('onhashchange' in window) {
					utils.events(window, 'hashchange', utils.bind(this, this._fire));
				} else {
					setInterval(function(){
						if (this._strCache != location.hash){
							this._fire();
						} 
					}, 250);
				}
				this._inited = true;
			}
		},

		/**
		 * DOM Ready event
		 */
		ready: (function() {
			var queue = [],
				loaded = document.readyState === "complete",
				detach = function () {
					if ( document.addEventListener ) {
						document.removeEventListener( "DOMContentLoaded", handler, false );
						window.removeEventListener( "load", handler, false );
					} else {
						document.detachEvent( "onreadystatechange", handler );
						window.detachEvent( "onload", handler );
					}
				},
				handler = function( event ) {
					if ( document.addEventListener || event.type === "load" || document.readyState === "complete" ) {
						loaded = true;
						detach();
						run();
					}
				},
				run = function (handler) {
					while (handler = queue.shift()) handler();
				},
				add = function (handler) {
					handler && typeof handler === 'function' && queue.push(handler) && loaded && run();
				};
			if (!loaded) {
				if ( document.addEventListener ) {
					document.addEventListener( "DOMContentLoaded", handler, false );
					window.addEventListener( "load", handler, false );
				} else {
					document.attachEvent( "onreadystatechange", handler );
					window.attachEvent( "onload", handler );
					var top = false;
					try {
						top = window.frameElement == null && document.documentElement;
					} catch(e) {}
					if ( top && top.doScroll ) {
						(function doScrollCheck() {
							if ( !loaded ) {
								try {
									top.doScroll("left");
								} catch(e) {
									return setTimeout( doScrollCheck, 50 );
								}
								loaded = true;
								detach();
								run();
							}
						})();
					}
				}
			} else {
				run();
			}
			return add;
		})(),

		/**
		 * Clone object reqursively (date, objects, arrays)
		 * @param  {Object} o
		 * @return {Object}
		 */
		clone: function(o) {
			if(!o || "object" !== typeof o)  {
				return o;
			}
			if (o instanceof Date) return new Date(o.getTime());
			var c = "function" === typeof o.pop ? [] : {};
			var p, v;
			for(p in o) {
				if(o.hasOwnProperty(p)) {
					v = o[p];
					if(v && "object" === typeof v) {
						c[p] = this.clone(v);
					}
					else c[p] = v;
				}
			}
			return c;
		},

		/**
		 * Add event listener wrapper
		 * @param  {Object} obj
		 * @param  {String} events  One or more event names
		 * @param  {Function} handler
		 * @return {Function}         
		 */
		events: function(obj, events, handler, remove) {
			var events = events.split(' '),
				prop = null;
			for (prop in events){
				if(remove) {
					obj.addEventListener ? obj.removeEventListener(events[prop], handler) : obj.detachEvent('on'+events[prop], handler);
				} else {
					obj.addEventListener ? obj.addEventListener(events[prop], handler, false) : obj.attachEvent('on'+events[prop], handler);
				}
			}
			return handler;
		},

		/**
		 * Distance between two points
		 */
		dist : function(x0, y0, x1, y1) {
			x0 = Math.sqrt( Math.pow(x1-x0, 2) + Math.pow(y1-y0, 2) );
			return x0;
		},

		/**
		 * Angle between two points
		 */
		angle: function(x0, y0, x1, y1) {
			var x = x0-x1,
				y = y1-y0,
				r = Math.atan2(y,x),
				angle = Math.round(r*180/Math.PI);
			return (angle < 0 ? 360 + angle : angle);
		},

		/**
		 * Function for translation numeric values
		 * @param  {object}   obj      Host object to be animated, may be HTMLElement or any another object
		 * @param  {object}   params   List of numeric params, for example: { param1: {from: 1, to: 0} }
		 * @param  {number}   time     Time for animation
		 * @param  {function}   handler  Handler for all animations, will get values in second arg
		 * @param  {function} callback
		 */
		animate: function mem(obj, params, time, handler, callback) {
			time = time*1;
			if(!mem.animations) {mem.animations = [];}
			if(obj.animationId) {
				var animationId = obj.animationId;
			} else {
				var animationId = mem.animations.length;
				obj.animationId = animationId;
				mem.animations[animationId] = {};
			}
			var instant = true;
			for(var key in params) {
				if(params[key].from !== params[key].to) {instant = false;}
				if(mem.animations[animationId] && mem.animations[animationId][key]) {
					clearInterval(mem.animations[animationId][key]);
				}
			}
			if(instant) {return callback&&callback(obj, true);}
			var progress,
				temp = null,
				start = new Date().getTime(),
				interval = setInterval(function(){
					progress = (new Date().getTime()-start)/time;
					if(progress>=1){
						temp = {};
						for(key in params) {
							clearInterval(mem.animations[animationId][key]);
							mem.animations[animationId][key] = null;
							temp[key] = params[key].to;
						}
						handler(obj, temp);
						callback&&callback(obj);
					} else {
						temp = {};
						for(key in params) {
							temp[key] = params[key].from + (params[key].to-params[key].from )*( progress<0.5 ? Math.pow(2*progress,3)+0*1 : 2-Math.pow(2*(1-progress),3) )/2;
						}
						handler(obj, temp);
					}
				}, 10);
			for(key in params) {
				mem.animations[animationId][key]=interval;
			}
		},

		/**
		 * Fast and nice trim
		 * @author Nick Pershin, AgileFusion
		 * @return {string}
		 */
		trim: (function() {
			var i = 0,
				ws = {},
				chars = ' \n\r\t\v\f\u00a0\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200a\u200b\u2028\u2029\u3000',
				length = chars.length;
			for(; i < length; i++)ws[chars.charAt(i)] = true;
			return function(str) {
				var s = -1,
					e = str.length;
				while(ws[str.charAt(--e)]);
				while(s++ !== e && ws[str.charAt(s)]);
				return str.substring(s, e+1);
			};
		})(),

		/**
		 * Allows you to add, remove and check classes quickly
		 */
		classes : (function() {
			function _get(str) {
				return utils.trim(str).split(/\s+/);
			}
			function _remove(arr1, arr2) {
				var count1 = arr1.length,
					count2,
					total = arr2.length;
				while(count1--){
					count2 = arr2.length;
					while(count2--){
						if(arr1[count1] == arr2[count2]){
							total--;
							arr1[count1]='';break;
						}
					}
				}
				return total==0;
			}
			function _set(node, arr) {
				node.className = utils.trim(arr.join(' ').replace(/\s+/g,' '));
			}
			return {

				/**
				 * Find node by class
				 * @param  {HTMLElement} node Container
				 * @param  {string} str  classes devided by space
				 * @return {array}      collection of elems
				 */
				find: function(node, str) {
					if(!node) {
						node = document.body;
					}
					if(str === void(0)) {
						str = node;
						node = document.body;

					}
					if (typeof document.getElementsByClassName == 'function') return node.getElementsByClassName(str);
					var elems = node.getElementsByTagName('*'),
						collection = [],
						total = elems.length,
						counter = 0;
					for (;counter<total;counter++) if (this.has(elems[counter], str)) collection.push(elems[counter]);
					return collection;
				},

				/**
				 * Add classes to node
				 * @param {HTMLElement} node
				 * @param {string} str  classes devided by space
				 * @return {HTMLElement}
				 */
				add : function(node, str) {
					var arr1 = _get(node.className||''),
						arr2 = _get(str);
					_remove(arr1,arr2);
					_set(node,arr1.concat(arr2));
					return node;
				},

				/**
				 * Remove classes from node
				 * @param {HTMLElement} node
				 * @param {string} str  classes devided by space
				 * @return {HTMLElement}
				 */
				remove : function(node, str) {
					var arr1 = _get(node.className||''),
						arr2 = _get(str);
					_remove(arr1,arr2)
					_set(node,arr1);
					return node;
				},

				/**
				 * Check if node has each of requested classes
				 * @param {HTMLElement} node
				 * @param {string} str  classes devided by space
				 * @return {boolean}
				 */
				has : function(node, str) {
					var arr1 = _get(node.className||''),
						arr2 = _get(str);
					return _remove(arr1,arr2);
				},

				/**
				 * Remove first group of classes and add second group
				 * @param  {HTMLElement} node [description]
				 * @param  {string} str1 classes devided by space
				 * @param  {string} str2 classes devided by space
				 * @return {HTMLElement}
				 */
				replace : function(node, str1, str2) {
					var arr1 = _get(node.className||''),
						arr2 = _get(str1),
						arr3 = _get(str2);
					_remove(arr1,arr2);
					_remove(arr1,arr3);
					_set(node,arr1.concat(arr3));
					return node;
				}
			};
		})(),

		/**
		 * Create HTMLElement and set some params
		 * @param  {string} node name
		 * @param  {object} params, ex: {attr: {prop:val}, css: {prop:val}, html: '', text: ''}
		 * @return {HTMLElement}
		 */
		node: function(node, params) {
			if(node === 'fragment') {
				return document.createDocumentFragment();
			} else {
				node = document.createElement(node);
				if(params) {
					var prop;
					if(params.text) {node.appendChild(document.createTextNode(params.text+''));}
					if(params.html) {node.innerHTML = params.html+'';}
					if(params.css) {for(prop in params.css) {node.style[prop] = params.css[prop];}}
					if(params.attr) {for(prop in params.attr) {node.setAttribute(prop,params.attr[prop]);}}
				}
				return node;
			}
		},

		/**
		 * Parse mouse/touch/MSPointer events to arrays of coordinates
		 * @param  {event} event 
		 * @return {obj}   {x:[],y:[],type:'pointer/mouse/touch'}
		 */
		eventsParse: function host(event) {
			host.__mspointer__ = host.__mspointer__ || {};		
			var type = event.type.indexOf('touch')===0 ? 'touch' : (
					event.type.indexOf('MSPointer')===0 ? 'pointer' : 'mouse'
				),
				x = [],
				y = [];
			switch(type){
				case 'pointer':
					host.__mspointer__[event.pointerId] = {x:event.pageX, y:event.pageY, id:event.pointerId};
					var counter = null, arr = [];
					for (counter in host.__mspointer__) {arr.push(host.__mspointer__[counter]);}
					arr.sort(function (a, b) {return a.id-b.id;});
					counter = 0;
					for (;counter<arr.length;counter++) {x.push(arr[counter].x);y.push(arr[counter].y);}
					if(event.type === 'MSPointerUp' || event.type === 'MSPointerCancel') delete host.__mspointer__[event.pointerId];

				break;
				case 'touch':
					if (event.touches.length) {
						var counter = event.touches.length;
						while(counter--){
							x.unshift(event.touches[counter].pageX);
							y.unshift(event.touches[counter].pageY);
						}
					} else {
						var counter = event.changedTouches.length;
						while(counter--){
							x.unshift(event.changedTouches[counter].pageX);
							y.unshift(event.changedTouches[counter].pageY);
						}
					}
				break;
				case 'mouse':
					x.push(event.pageX);
					y.push(event.pageY);
				break;
			}
			return {x:x,y:y,type:type};
		},

		/**
		 * Capitalize first letter of the string
		 * @param  {string} str 
		 * @return {string}
		 */
		capitalize: function(str) {
			str = str+'';
			return str[0].toUpperCase()+str.substring(1);
		},

		/**
		 * Manages custom events creation and observation;
		 * Now supports event propagation, use event.stopPropagation() if necessary 
		 * @author Alexander Kolobov, AgileFusion 
		 */
		customEvents: (function() {
			var queue = {},
				dummy = {};
			function addListener(elem, event, handler) {
				// create new event array if needed
				if(typeof queue[event] === 'undefined') {queue[event] = [];}
				// take dummy as elem if no elem specified
				if(!elem) {elem = dummy;}
				// try to add new handler if elem is already listen for this event
				var elems = queue[event].length;
				while(elems--) {
					if(queue[event][elems].elem === elem) {
						queue[event][elems].handlers.push(handler);
						dispatchEvent(null, 'addlistener', {event: event, elem: elem});
						return true;
					}
				}
				// set elem to listen this event
				queue[event].push({elem: elem, handlers: [handler]});
				dispatchEvent(null, 'addlistener', {event: event, elem: elem});
				return true;
			}
			function removeListener(elem, event, handler) {
				// if no event array created - nothing to remove
				if(typeof queue[event] === 'undefined') {return true;}
				// take dummy as elem if no elem specified
				if(!elem) {elem = dummy;}
				var handlers = null,
					elems = queue[event].length;
				while(elems--) {
					if(queue[event][elems].elem === elem) {
						if(typeof handler === 'undefined') {
							queue[event][elems].handlers = [];
							dispatchEvent(null, 'removelistener', {event: event, elem: elem});
							return true;
						} else {
							handlers = queue[event][elems].handlers.length;
							while(handlers--) {if(queue[event][elems].handlers[handlers] === handler) {
								queue[event][elems].handlers.splice(handlers, 1);
								dispatchEvent(null, 'removelistener', {event: event, elem: elem});
							}}
						}
					}
				}
				return true;
			}
			function customEvent(type, target, details) {
				this.stoped = false;
				this.type = type;
				this.details = details;
				this.target = target;
			}
			customEvent.prototype = {
				stopPropagation: function() {this.stoped = true;},
				propagate: function() {this.target = this.target.parentNode || null;}
			};
			function dispatchEvent(elem, event, data) {
				if(typeof queue[event] === 'undefined') {return false;}
				var evt = new customEvent(event, elem || dummy, data || {}), elems, handlers, htotal, etotal;
				while (evt.target && !evt.stoped) {
					elems = 0;
					etotal = queue[event].length;
					for(;elems<etotal;elems++) {
						if(!evt.target || evt.target === queue[event][elems].elem) {
							handlers = 0;
							htotal = queue[event][elems].handlers.length;
							for(;handlers<htotal;handlers++) {
								queue[event][elems].handlers[handlers] && queue[event][elems].handlers[handlers](evt);
							}
						}
					}
					evt.propagate();
				}
				return true;
			}
			return {

				/**
				 * Get elems which are listeners for specified event
				 * @param  {String} event Event name
				 * @return {Array}       
				 */
				getElems: function getElems(event) {
					if(typeof queue[event] === 'undefined') {return [];}
					var collection = [],
						elems = queue[event].length;
					while(elems--) {collection.push(queue[event][elems].elem);}
					return collection;
				},

				/**
				 * Add event listener to specified elem
				 * @param {Object} elem    Elem to listen on
				 * @param {String} event   Event name
				 * @param {Function} handler
				 */
				addListener: addListener,

				/**
				 * Add event listener globally
				 * @param {String} event   Event name
				 * @param {Function} handler
				 */
				addGlobListener: function(event, handler) {return addListener(null, event, handler)},

				/**
				 * Remove event listener for specified event, element and handler
				 * @param  {Object} elem
				 * @param  {String} event
				 * @param  {Function} handler
				 * @return {Boolean}
				 */
				removeListener: removeListener,

				/**
				 * Remove event listener for specified event and handler
				 * @param  {String} event
				 * @param  {Function} handler
				 * @return {Boolean}
				 */
				removeGlobListener: function(event, handler) {return removeListener(null, event, handler)},

				/**
				 * Fire event of requested type for specified element width additional data
				 * @param  {Object} elem
				 * @param  {String} event 
				 * @param  {Object} data 
				 * @return {Boolean}
				 */
				dispatchEvent: dispatchEvent,

				/**
				 * Fire event of requested type width additional data
				 * @param  {String} event 
				 * @param  {Object} data 
				 * @return {Boolean}
				 */
				dispatchGlobEvent: function(event, data) {return dispatchEvent(null, event, data);}
			};
		})()

	};

	/**
	 * Swipe custom event, will observe swipe event on any element bounded to "swipe" event.
	 * use this to stop/start observation (started by default)
	 * customEventSwipe.stop();
	 * customEventSwipe.start();
	 * @author Alexander Kolobov, AgileFusion 
	 */
	window.customEventSwipe = (function() {
		var test = true,
			target = null,
			parsed = null,
			minVelocity = 500,
			maxVelocity = 10000, 
			units = 1000,
			touches = [],
			handler = function (event) {
				if (!test) {
					touches = [];
					return;
				}
				parsed = utils.eventsParse(event);
				if (parsed.x.length > 1) return; // more than one finger
				switch (event.type) {
					case 'touchstart':
						touches = [];
						target = event.target;
					break;
					case 'touchmove':
						touches.push({
							x: parsed.x[0],
							y: parsed.y[0],
							pastTime: new Date().getTime()
						});
					break;
					case 'touchend':case 'touchleave':
						if (touches.length>10) touches = touches.splice(touches.length-10, 10);
						var accumX = 0,
							accumY = 0,
							vel,
							dur,
							xVelocity,
							yVelocity,
							N = Math.min(10, touches.length),
							i = 1;
						if (N > 3) N--;
						for (;i<N;i++) {
							dur = touches[i].pastTime - touches[0].pastTime;
							if (dur == 0) continue;
							vel = ((touches[i].x - touches[0].x) / dur) * units;
							accumX = accumX == 0 ? vel : (accumX + vel) * .5;
							vel = ((touches[i].y - touches[0].y) / dur) * units;
							accumY = accumY == 0 ? vel : (accumY + vel) * .5;
						}
						xVelocity = accumX < 0 ? Math.max(accumX, -maxVelocity) : Math.min(accumX, maxVelocity);
						yVelocity = accumY < 0 ? Math.max(accumY, -maxVelocity) : Math.min(accumY, maxVelocity);
						if (target && (Math.abs(xVelocity)>minVelocity || Math.abs(yVelocity)>minVelocity)) { // velocity treshold
							utils.customEvents.dispatchEvent(target, 'swipe', {x: xVelocity, y: yVelocity});
						}
					break;
				}
			};
		window.addEventListener('touchmove', handler, false);
		window.addEventListener('touchstart', handler, false);
		window.addEventListener('touchend', handler, false);
		window.addEventListener('touchleave', handler, false);
		return {
			status: function () {return test;},
			start: function () {test = true;},
			stop: function () {test = false;},
		}
	})();


	/**
	 * @author Alexander Kolobov, AgileFusion
	 * @editor Nick Pershin, WWW (bitwise optimizations and 'onopen/onclose' override possibility)
	 */
	window.tip = (function(_){
		'use strict';
		var options = {
				prefix: 'tip',
				content: '',
				timer: 50,
				type: 'general',
				width: 'auto'
			},
			repositionAll = function mem() {
				mem.timeout && clearTimeout(mem.timeout);
				mem.timeout = setTimeout(function() {
					tips.forEach(function(tip) {
						tip.reposition();
					});
				}, options.timer);
			},
			handlerAll = function(event) {
				tips.forEach(function(tip, mask) {
					if (tip.opened && event.target && event.target.tip != tip) {
						mask = tip.container.compareDocumentPosition(event.target);
						if (mask<16) {
							tip.close();
						} else if(mask>=16 && tip.elem.getAttribute(options.prefix+'-close') == 'true'){
							//setTimeout(tip.close(true).bind(tip),1);
						}
					}
				});
			},
			tips = [],
			publ = function tip(elem){
				if(!this || this===window) { // parse

					Array.prototype.slice.call(utils.classes.find(document.body, options.prefix)).forEach(function(elem) {
						if(!elem.tipprocessed) new tip(elem);
					});

					tips.forEach(function(tip, index) {
						if(document.body.compareDocumentPosition(tip.target)&Node.DOCUMENT_POSITION_DISCONNECTED) {
							tips.splice(index,1);
							tip.container.parentNode.removeChild(tip.container);
						}
					});

				} else { // create

					this.elem = typeof elem == 'string' ?  document.getElementById(elem) : elem;
					if(this.elem.tipprocessed) return;
					this.elem.tipprocessed = true;
					this.target = null;
					this.opened = false;
					this.opacity = 0;
					this.onopen = null;
					this.onclose = null;
					this.type = options.type;
					this.x = 0;
					this.y = 0;
					this.ax = 0;
					this.ay = 0;
					this.container = utils.node('div');
					this.content = utils.node('div');
					this.wrapper = utils.node('div');
					this.corner1 = utils.node('div');
					this.corner2 = utils.node('div');
					this.container.appendChild(this.wrapper);
					this.container.appendChild(this.content);
					this.wrapper.appendChild(this.corner1);
					this.wrapper.appendChild(this.corner2);
					utils.classes.add(this.container, 'tip-container');
					utils.classes.add(this.wrapper, 'tip-corners');
					utils.classes.add(this.content, 'tip-content');
					utils.classes.add(this.corner1, 'tip-corner-background');
					utils.classes.add(this.corner2, 'tip-corner-foreground');
					this.close();
					document.body.appendChild(this.container);

					if(this.elem) {
						this.elem.addEventListener('click', this.handler.bind(this));
						this.setContent(this.elem.getAttribute(options.prefix+'-content') || options.content);
						this.setWidth(this.elem.getAttribute(options.prefix+'-width') || options.width);
						this.setType(this.elem.getAttribute(options.prefix+'-type') || options.type);
						utils.classes.add(this.elem, this.type);
						this.setTarget(this.elem.getAttribute(options.prefix+'-target') ? document.getElementById(this.elem.getAttribute(options.prefix+'-target')) : this.elem);
					} else {
						this.setContent(options.content);
						this.setWidth(options.width);
					}

					tips.push(this);

				}
			}

		window.addEventListener('touchstart', handlerAll);
		window.addEventListener('click', handlerAll);
		window.addEventListener('resize', repositionAll);
		window.addEventListener('scroll', repositionAll);

		publ.close = function(){
			tips.forEach(function(tip){ tip.close(true); });
		}

		publ.prototype={

			handler: function(event) {
				event.preventDefault();
				event.stopPropagation();
				handlerAll(event);
				this[this.opened ? 'close' : 'open']();
			},

			setContent: function(content) {
				var opened = this.opened;
				opened && this.close();
				switch(true) {
					case ((content+'').split('')[0] == '#'): this.content.appendChild(document.getElementById(content.substr(1)));break;
					case !!content.appendChild: this.content.appendChild(content);break;
					default: this.content.innerHTML = content+'';break;
				}
				opened && this.open();
			},

			setType: function(type) {
				var opened = this.opened;
				opened && this.close();
				utils.classes.remove(this.container, 'tip-'+this.type);
				utils.classes.remove(this.elem, this.type);
				this.type = type || options.type;
				utils.classes.add(this.elem, this.type);
				utils.classes.add(this.container, 'tip-'+this.type);
				opened && this.open();
			},

			setTarget: function(target) {
				var opened = this.opened;
				opened && this.close();
				this.target = target||this.elem||null;
				this.target.tip = this;
				opened && this.open();
			},

			setWidth: function(width) {
				var opened = this.opened;
				opened && this.close();
				this.container.style.width = width || options.width;
				//this.container.style.whiteSpace = (width || options.width) == 'auto' ? 'nowrap' : 'normal';
				opened && this.open();
			},
			
			open: function() {
				!this.opened && utils.customEvents.dispatchEvent(this, 'open');
				this.opened = true;
				if(this.target) {
					this.target && utils.classes.add(this.target, 'selected');
					this.reposition();	
					utils.classes.remove(this.container, options.prefix+'-hidden');
					utils.animate(this.container, {opacity: {from: this.opacity, to: 1}}, 200, function(obj, props){
						obj.style.opacity = props.opacity;
						this.opacity = props.opacity;
						this.onopen && this.onopen();
					}.bind(this));
				}
			},

			close: function(instant) {
				this.opened && utils.customEvents.dispatchEvent(this, 'close');
				this.opened = false;
				this.target && utils.classes.remove(this.target, 'selected');
				utils.animate(this.container, {opacity: {from: this.opacity, to: 0}}, instant?0:200, function(obj, props){
					obj.style.opacity = props.opacity;
					this.opacity = props.opacity;
				}.bind(this), function() {
					utils.classes.add(this.container, options.prefix+'-hidden');
					this.onclose && this.onclose();
				}.bind(this));
			},

			reposition: function() {
				if(this.target && this.opened) {

					var r = this.target.getBoundingClientRect();
					
					if(Math.max(r.left,r.right,r.width,r.height) == 0) {
						utils.classes.add(this.container, options.prefix+'-notarget');
						return;
					} else {
						utils.classes.remove(this.container, options.prefix+'-notarget');
					}
					
					var	t = document.documentElement || document.body.parentNode,
						x = typeof window.pageXOffset == 'undefined' ? (t && typeof t.ScrollLeft == 'number' ? t : document.body).ScrollLeft : window.pageXOffset,
						y = typeof window.pageYOffset == 'undefined' ? (t && typeof t.ScrollTop == 'number' ? t : document.body).ScrollTop : window.pageYOffset,
						w = document.documentElement.clientWidth || document.body.clientWidth,
						h = document.documentElement.clientHeight || document.body.clientHeight,
						width = this.container.offsetWidth,
						height = this.container.offsetHeight,
						ww = this.wrapper.offsetWidth,
						wh = this.wrapper.offsetHeight;

					if(width <= w && height < r.top && r.top >= h-r.bottom) { // top
						this.y = r.top - height;
						this.ay = height-wh;
						this.x = Math.max(0, r.left + ((r.right - r.left) >> 1) - (width >> 1));
						if(this.x + width > w) this.x = Math.max(0, w-width);
						this.ax = Math.min(width-(ww >> 1)*3, Math.max(ww >> 1, ((r.right-r.left) >> 1) + (r.left - this.x) - (ww >> 1)));
					} else if(width <= w && height < h-r.bottom && r.top <= h-r.bottom) { // bottom
						this.y = r.bottom;
						this.ay = 0;
						this.x = Math.max(0, r.left + ((r.right - r.left) >> 1) - (width >> 1));
						if(this.x + width > w) this.x = Math.max(0, w-width);
						this.ax = Math.min(width-(ww >> 1)*3, Math.max(ww >> 1, ((r.right-r.left) >> 1) + (r.left - this.x) - (ww >> 1)));
					} else if(height <= h && width < r.left && r.left >= w-r.right) { // left
						this.x = r.left - width;
						this.ax = width-ww;
						this.y = Math.max(0, r.top + ((r.bottom - r.top) >> 1) - (height >> 1));
						if(this.y + height > h) this.y = Math.max(0, h-height);
						this.ay = Math.min(height-(wh >> 1)*3, Math.max(wh >> 1, ((r.bottom-r.top) >> 1) + (r.top - this.y) - (wh >> 1)));
					} else if(height <= h && width < w-r.right && r.left <= w-r.right) { // right
						this.x = r.right;
						this.ax = 0;
						this.y = Math.max(0, r.top + ((r.bottom - r.top) >> 1) - (height >> 1));
						if(this.y + height > h) this.y = Math.max(0, h-height);
						this.ay = Math.min(height-(wh >> 1)*3, Math.max(wh >> 1, ((r.bottom-r.top) >> 1) + (r.top - this.y) - (wh >> 1)));
					} else { // bottom
						this.y = r.bottom;
						this.ay = 0;
						this.x = Math.max(0, r.left + ((r.right - r.left) >> 1) - (width >> 1));
						if(this.x + width > w) this.x = Math.max(0, w-width);
						this.ax = Math.min(width-(ww >> 1)*3, Math.max(ww >> 1, ((r.right-r.left) >> 1) + (r.left - this.x) - (ww >> 1)));
					}

					this.container.style.left = x+this.x+'px';
					this.container.style.top = y+this.y+'px';

					this.wrapper.style.left = this.ax+'px';
					this.wrapper.style.top = this.ay+'px';
				}
			}
		};

		return publ;
	})();

	utils.ready(tip);

})(window, document, void(0));
