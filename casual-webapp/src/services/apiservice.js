import axios from 'axios';
import Parser from "./utilities/parser";


const urls = {
    serviceURI: '/casual/.casual/service/state',
    gatewayURI: '/casual/.casual/gateway/state',
    transactionURI: '/casual/.casual/transaction/state',
    domainURI: '/casual/.casual/domain/state',
    scaleURI: '/casual/.casual/domain/scale/instances',
    baseURL: '/casual/',
    WEB_VERSION: 'CASUAL_WEB_VERSION'
};
const headers = {
    headers: {
        'content-type': 'application/json'
    }
};

const getDomain = async function () {
    try {
        let domain = await axios.post(urls.domainURI, {}, headers);
        return domain.data.result;
    } catch (error) {
        throw error;
    }

};

const getService = async function () {
    try {
        let service = await axios.post(urls.serviceURI, {}, headers);
        return service.data.result;
    } catch (error) {
        throw error;
    }

};


const getGateway = async function () {
    try {
        let gateway = await axios.post(urls.gatewayURI, {}, headers);
        return gateway.data.result;
    } catch (error) {
        throw error;
    }
};

const getTransaction = async function () {
    try {
        let transaction = await axios.post(urls.transactionURI, {}, headers);
        return transaction.data.result;
    } catch (error) {
        throw error;
    }
};


const getServiceApi = async function (serviceName) {

    const specHeaders = {
        headers: {
            'content-type': 'application/json',
            'casual-service-describe': true
        }
    };


    let responseBody = {
        "service": "none", input: [], output: []
    };
    try {

        const url = urls.baseURL + serviceName;
        const response = await axios.post(url, {}, specHeaders);
        if (response.data.hasOwnProperty("model")) {
            responseBody = Parser.jsonToYaml(response.data.model);
        }
        return responseBody;

    } catch (error) {
        return responseBody;
    }

};


const scaleServer = async function (alias, instances) {


    const data = {
        instances: [
            {
                alias: alias,
                instances: instances
            }
        ]
    };

    const response = await axios.post(urls.scaleURI, data, headers);

    return response;


}


export default {
    getDomain,
    getService,
    getGateway,
    getTransaction,
    getServiceApi,
    scaleServer
}
