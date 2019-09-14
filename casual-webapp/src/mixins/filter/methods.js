const getOptions = function (object, name) {


    const options = [];

    for (const obj of object) {

        if (!options.includes(obj[name])) {
            options.push(obj[name]);
        }

    }
    return options;
};

const filterObject = function (object, filters) {
    for (const filter in filters) {
        object = filterSingle(object, filter, filters[filter]);
    }
    return object;

};

const filterSingle = function (object, key, value) {
    return object.filter(obj => {

        if (obj.hasOwnProperty(key)) {
            return obj[key] === value;
        }

    })
}


export default {
    getOptions: getOptions,
    filterObject: filterObject
}