
const ObjectStreamIn = {
	parseObjectDefinition: function (lines) {
		return lines.map(line => {
			const parts = line.trim().split(/\s+/);
			const item = {};
			item.name = parts.shift();
			if (parts[0] == 'array') {
				item.isArray = true;
				parts.shift();
			}
			if (parts[0] == 'instance') {
				item.isInstance = true;
				parts.shift();
			}
			item.type = parts[0];
			return item;
		})
	},
	parseType: function (line, type) {
		const toNum = (s) => s.trim().split(/\s+/).map(parseFloat);
		switch (type) {
			case 'double3':
			case 'float3':
			case 'vec3':
			case 'dvec3':
			case 'quat':
			case 'vec4':
			case 'mat44':
			case 'dmat44':
				return { value: toNum(line), _type: type };
			case 'uint8':
			case 'uint16':
			case 'uint32':
			case 'uint64':
			case 'int':
			case 'int64':
				return parseInt(line, 10);
			case 'float':
			case 'double':
				return parseFloat(line);
			case 'bool':
				return line === 'true';
		}
		return line;
	},
	parseObjectDeclaration: function (lines, schema, definitions, instances) {
		const padStartLength = (s) => (/\s*/).exec(s)[0].length;
		const resp = {};
		resp._class = schema;
		if (!definitions[schema]) {
			const line = lines.shift().trim();
			return ObjectStreamIn.parseType(line, schema)
		}
		const startPad = padStartLength(lines[0]);
		definitions[schema].forEach(element => {
			const line = lines[0].trim();
			if (element.isArray) {
				const count = parseInt(line, 10);
				lines.shift();
				const arr = [];
				for (let i = 0; i < count; i++) {
					arr.push(ObjectStreamIn.parseObjectDeclaration(lines, element.type, definitions, instances));
				}
				resp[element.name] = arr;
			} else if (element.isInstance) {
				if (padStartLength(lines[0]) > startPad) {
					const object = ObjectStreamIn.parseObjectDeclaration(lines, element.type, definitions, instances);
					resp[element.name] = object;
				} else {
					resp[element.name] = { instance: line }
					lines.shift();
				}
			}
			else {
				resp[element.name] = ObjectStreamIn.parseType(line, element.type);
				lines.shift();
			}
		});
		return resp;
	},
	flattenObject(object, definitions, instances) {
		if (definitions[object._class]) {
			definitions[object._class].forEach(element => {
				let field = object[element.name];
				if (element.isArray) {
					field.forEach(obj => {
						ObjectStreamIn.flattenObject(obj, definitions, instances);
					})
				} else {
					if (field.instance) {
						field = object[element.name] = instances[field.instance];
					}
					if (field && field._class) {
						ObjectStreamIn.flattenObject(field, definitions, instances);
					}
				}
			});
		}

	},
	sReadObject: function (text) {
		const chunks = text.split('\n');
		const definitions = {};
		const objects = {};

		let firstObject = undefined;

		while (chunks.length > 0) {
			const line = chunks.shift().trim();
			if (line.length > 0) {
				const [command, name, value] = line.split(/\s+/);
				switch (command) {
					case 'TOS':
						definitions.TOS_VERSION = name;
						break;
					case 'declare':
						definitions[name] = ObjectStreamIn.parseObjectDefinition(chunks.splice(0, parseInt(value, 10)));
						break;
					case 'object':
						objects[value] = ObjectStreamIn.parseObjectDeclaration(chunks, name, definitions, objects);
						objects[value]._instance = value;
						if (!firstObject) {
							firstObject = objects[value];
						}
						break;
				}
			}
		}

		if (firstObject) {
			ObjectStreamIn.flattenObject(firstObject, definitions, objects)
		}

		return { result: firstObject, objects, definitions };
	}
}