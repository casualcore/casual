/* eslint-disable */

import request from 'request';
import informationcontroller from './informationcontroller';
import Parser from '../utilities/parser';

import api from '../apiservice';


const getAllGroups = async function () {

    try {
        let domain = await api.getDomain();
        return informationcontroller.allGroups(domain);
    } catch (error) {
        throw error;
    }
};

const getAllServers = async function () {

    try {
        let domain = await api.getDomain();
        return informationcontroller.allServers(domain);
    } catch (error) {
        throw error;
    }
}

const getAllExecutables = async function () {
    try {
        let domain = await api.getDomain();

        let ex = informationcontroller.allExecutables(domain);
        console.log(ex);
        return ex;
    } catch
        (error) {
        throw error;
    }

};
const getAllServices = async function () {

    try {
        let services = await api.getService();
        return informationcontroller.getAllServiceAverages(services.services);

    } catch
        (error) {
        throw error;
    }
};
const getServiceApi = async function (serviceName) {

    try {
        return await api.getServiceApi(serviceName);
    } catch (error) {
        throw error;
    }

}
const serviceApiAvailable = function (serviceName) {




    return new Promise(function (resolve, reject) {


        let hasApi = false;

        request(options, function (error, response, body) {
            try {
                if (body.hasOwnProperty("model")) {
                    hasApi = true;
                }
                resolve(hasApi);
            } catch (error) {
                reject(hasApi);
            }
        }, function () {
            reject(hasApi);
        });
    })
};

const getServerServices = function (instancepid) {

    return new Promise(async function (resolve, reject) {

        let servicelist = [];
        let services = await getAllServices();
        for (let service of services) {


            for (let instance of service.instances.sequential) {

                if (instance.pid === instancepid.handle.pid) {
                    servicelist.push(service.name);
                }
            }
        }
        resolve(servicelist);
    });
}

const getInstancesFromServer = function (alias) {
    return new Promise(async function (resolve, reject) {

        let servers = await getAllServers();
        //console.log(JSON.stringify(servers, null,1));

        // console.log(alias);
        let server = informationcontroller.getServerByAlias(servers, alias);


        //console.log(JSON.stringify(server, null,1));
        //   console.log(server.instances.length);
        resolve(server.instances.length);

    })
}


const getAllGateways = async function () {

    try {
        return await api.getGateway();
    } catch (error) {
        throw error;
    }
};
const getAllTransactions = async function () {

    try {
        return await api.getTransaction();
    } catch (error) {
        throw error;
    }
}

export default {
    getAllGroups,
    getAllServers,
    getServerServices,
    getAllServices,
    getServiceApi,
    serviceApiAvailable,
    getAllExecutables,
    getInstancesFromServer,
    getAllGateways,
    getAllTransactions
}

