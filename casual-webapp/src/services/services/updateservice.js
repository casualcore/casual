const request = require('request');
const Parser = require('../utilities/extract');

let urls = {
    serviceURI: '/casual/.casual/service/state',
    gatewayURI: '/casual/.casual/gateway/state',
    transactionURI: '/casual/.casual/transaction/state',
    domainURI: '/casual/.casual/domain/state',
    scaleURI: '/casual/.casual/domain/scale/instances',
    WEB_VERSION: 'CASUAL_WEB_VERSION'
};


function scaleServer(alias, instances, callback) {


    let scaleOption = {
        uri: 'http://localhost:8080' + urls.scaleURI,
        method: 'POST',
        json: {
            instances: [
                {
                    alias: alias,
                    instances: instances
                }
            ]
        }
    };

    request(scaleOption, function (error, response, body) {

        if (error) {
            callback(error);
        } else {
            callback(body.result);
        }
    });
}

exports.scaleServer = scaleServer;