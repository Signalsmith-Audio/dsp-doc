fetch("../version-notes.txt").then(r => r.text()).then(text => {
console.log(text);
	let numElement = document.getElementById('projectnumber');
	let current = numElement.textContent;
	numElement.innerHTML = "";
	let select = document.createElement('select');
	text.replace(/^v([0-9]+.[0-9]+.[0-9]):+/gm, (all, version) => {
		let option = document.createElement('option');
		option.textContent = version;
		select.appendChild(option);
	});
	select.value = current;
	select.onchange = () => {
		let parts = location.href.split(current);
		if (parts.length > 1) {
			location.href = parts[0] + select.value + "/modules.html";
		} else {
			location.href = location.href.replace(/\/html\/.*/, "/v" + select.value + "/modules.html");
		}
	};
	numElement.appendChild(select);
}).then(console.log).catch(console.log);
