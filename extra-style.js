// We generated this text file from the Git tags
fetch("../version-notes.txt").then(response => {
	if (!response.ok) return;
	return response.text().then(text => {
		let numElement = document.getElementById('projectnumber');
		let current = numElement.textContent;
		numElement.innerHTML = "";
		let select = document.createElement('select');
		// The format is like `vA.B.C: ...`
		text.replace(/^v([0-9]+.[0-9]+.[0-9]):+/gm, (all, version) => {
			let option = document.createElement('option');
			option.textContent = version;
			select.appendChild(option);
		});
		select.value = current;
		select.onchange = () => {
			// The modules page for the other version is a reasonable default
			let fallbackHref = location.href.replace(/\/html\/.*/, "/v" + select.value + "/modules.html");
			let parts = location.href.split(current);
			if (parts.length > 1) {
				fallbackHref = parts[0] + select.value + "/modules.html";
			}
			// However, check (with a HEAD request) whether there's an exact equivalent to the current page
			let newHref = location.href.replace(current, select.value);
			if (newHref == location.href) newHref = newHref.replace("/html/", "/v" + select.value + "/");
			fetch(newHref, {method: 'HEAD'}).then(response =>{
				location.href = response.ok ? newHref : fallbackHref;
			}).catch(() => {
				location.href = fallbackHref;
			});
		};
		numElement.appendChild(select);
	});
}).then(console.log).catch(console.log);
