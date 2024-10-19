const express = require('express');
const app = express();
const path = require('path');

app.get('/*', function (req, res) {
	res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
	res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
	let fn = req.originalUrl;
	if (fn == "/")
		fn = "/index.html";
	else if (!path.extname(fn))
		fn += ".html";
	fn = path.join(path.join(__dirname, "../Examples"), fn);
	res.sendFile(fn);
})

app.listen(3000)
console.log("To browse the examples go to: http://localhost:3000/");
