import casualservice from '../services/casualservice';

export const mixin = {
        data() {
            return {
                yaml: [],
                refreshButtonText: 'Refresh',
                Casual: casualservice
            }
        },
        methods:
            {
                formatDate: function (nano) {


                    let milli = this.toMilli(nano);
                    let date = "-";
                    try {
                        let last = milli;
                        if (last !== "" && last > 0) {
                            let timeZoneOffset = (new Date()).getTimezoneOffset() * 60000;
                            let newDate = new Date(last - timeZoneOffset).toISOString().split(/[A-Z]/);
                            date = newDate[0] + " | " + newDate[1];
                        }
                    } catch (erro) {
                        date = "-";
                    }
                    return date;
                },
                toMilli: function (nano) {
                    let million = 1000000;
                    return nano / million;
                },
                parseYamlString: function (obj) {

                    const role = obj.role !== "" ? obj.role : "-";
                    const category = obj.category;
                    const level = obj.level;


                    this.yaml.push({
                        category: category,
                        role: role,
                        level: level
                    });

                    if (obj.children.length !== 0) {
                        for (let child of obj.children) {
                            this.yaml.push(this.parseYamlString(child));
                        }
                    }

                },

                toYaml: function (json) {

                    this.yaml = [];
                    if (json[0]) {
                        this.parseYamlString(json[0]);
                    }

                    return this.yaml;
                }
            }
    }
;

