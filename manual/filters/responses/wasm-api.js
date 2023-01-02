/*
	Mostly wraps a module which uses `wasm-api.h`, so that array/string results get returned properly.

	It also provides a `.range()` method:
		WasmApi(Module).range({
			key: 'key', label: 'Label'
			min: 0, max: 100, step: 1,
			initial: 75
		});
	This inserts an HTML table (if one is not referenced in the optional second argument) with a slider.
*/
function WasmApi(Module, table) {
	if (!(this instanceof WasmApi)) return new WasmApi(Module, table);
	let script = document.currentScript;
	function createTable() {
		if (typeof table == 'string') table = document.querySelector(table);
		if (!table) {
			table = document.createElement('table');
			script.parentNode.insertBefore(table, script);
		}
		this.table = table;
	}

	this.range = (obj) => {
		createTable();
		let row = document.createElement('tr');
		
		let label = document.createElement('th');
		label.textContent = obj.label || obj.key;
		row.appendChild(label);
		
		let values = document.createElement('td');
		let input = document.createElement('input');
		input.type = 'range';
		input.min = obj.min;
		input.max = obj.max;
		input.step = obj.step;
		input.value = obj.initial;
		input.setAttribute('value', obj.initial);
		input.name = obj.key;
		values.appendChild(input);

		let text = document.createElement('input');
		text.type = 'number';
		text.value = input.value;
		text.step = input.step;
		input.addEventListener('input', e => {
			text.value = input.value;
		});
		input.addEventListener('dblclick', e => {
			text.value = input.value = obj.initial;
		});
		text.addEventListener('change', e => {
			input.value = text.value;
			update();
		});
		values.appendChild(text);

		row.appendChild(values);
		table.appendChild(row);
		return this;
	};
	
	this.module = null;
	let firstUpdate = true;
	this.run = (controlsChanged) => {
		return Module().then(module => {
			this.module = module;

			function wrap(fn) {
				return function wrappedResult() {
					let args = [].slice.call(arguments, 0);
					let callback = arguments[arguments.length - 1];
					if (typeof callback == 'function') {
						// Callback
						module.heapResultCallback = (result) => {
							module.heapResultCallback = null;
							callback(result);
						};
						fn.apply(null, args.slice(0, args.length - 1));
						if (module.heapResultCallback) throw "Function did not trigger callback";
					} else {
						// No callback
						let result = fn.apply(this, arguments);
						if (typeof result == 'undefined') {
							result = module.heapResult;
							module.heapResult = null;
						}
						return result;
					}
				}
			}
			let api = {};
			for (let key in module) {
				if (/^_[^_]/.test(key)) {
					api[key.substring(1)] = wrap(module[key]);
				}
			}
			
			function update() {
				let obj = {};
				if (table) table.querySelectorAll('input, select').forEach(input => {
					let name = input.name || input.id;
					if (!name) return;
					let value = input.value;
					if (input.type == 'range') value = parseFloat(value);
					if (input.type == 'checkbox') value = input.checked;
					obj[name] = value;
				});
				controlsChanged(api, obj, firstUpdate);
				firstUpdate = false;
			}
			update();
			if (table) table.querySelectorAll('input, select').forEach(input => {
				let initialValue = input.getAttribute("value");
				input.oninput = input.onchange = update;
				input.ondblclick = () => {
					input.value = initialValue;
					update();
				};
			});
		});
	};
}
