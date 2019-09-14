const Category = [
    'unknown',
    'container',
    'composite',
    'integer',
    'floatingpoint',
    'character',
    'boolean',
    'string',
    'binary',
    'fixed binary'
];
const jsonToYaml = function (json) {

    try {
        let service = json.service;
        let input = json.arguments.input;
        let output = json.arguments.output;

        let newInput = "";
        let newOutput = "";

        if (input[0]) {
            newInput = parseResult(input[0], 0);
        }
        if (output[0]) {
            newOutput = parseResult(output[0], 0);
        }

        return {
            service, input: [newInput], output: [newOutput]
        };
    } catch (error) {
        throw error;
    }
};

const parseResult = function (object, level) {


    if (!object) {
        return [];
    } else {


        let obj = {};
        obj.category = Category[object.category];
        obj.role = object.role;
        obj["level"] = level++;
        obj["children"] = [];
        if (object.attribues.length > 0) {
            for (let child of object.attribues) {
                obj["children"].push(parseResult(child, level));
            }
        }
        return obj;
    }
};

export default {
    jsonToYaml
}
