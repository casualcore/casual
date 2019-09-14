import information from './information/informationservice';
import informationcontroller from './information/informationcontroller';

import update from './update/updateservice';

const getAllServers = async function () {

    try {
        return await information.getAllServers();
    } catch (error) {
        return '';
    }

};

const getServerByName = async function (serverAlias) {


    try {
        const servers = await getAllServers();
        let server = informationcontroller.getServerByAlias(servers, serverAlias);
        server.services = await information.getServerServices(server.instances[0]);
        return server;

    } catch (error) {
        return '';
    }

}

const scaleInstance = async function (alias, instances) {


    let res = await update.scaleServer(alias, instances);
    return res.data;


}

const getAllGroups = async function () {

    try {
        return await information.getAllGroups();
    } catch (error) {
        return '';
    }

}

const getGroupByName = async function (groupName) {
    try {
        const groups = await getAllGroups();
        let group = informationcontroller.getGroupByName(groups, groupName);
        group.dependencyNames = informationcontroller.getGroupDependencyNames(groups, group.dependencies);
        return group;
    } catch (error) {
        return '';
    }

}

const getAllServices = async function () {
    try {
        return await information.getAllServices();
    } catch (error) {
        return '';
    }


}

const getServiceByName = async function (serviceName) {


    try {
        const servers = await information.getAllServers();
        let services = await information.getAllServices();
        return informationcontroller.getServiceByName(servers, services, serviceName);
    } catch (error) {
        return '';
    }


}

const getServiceApi = async function (serviceName) {

    try {
        const api = await information.getServiceApi(serviceName);
        return api;
    } catch (error) {
        return {service: 'none'};
    }
}

const getAllExecutables = async function () {
    try {
        return await information.getAllExecutables();
    } catch (error) {
        return '';
    }


};

const getExecutableByAlias = async function (executableAlias) {

    try {
        const executables = await information.getAllExecutables();
        return informationcontroller.getExecutableByAlias(executables, executableAlias);
    } catch (error) {
        return '';
    }

}

const getAllGateways = async function () {

    try {

        return await information.getAllGateways();
    } catch (error) {
        return '';
    }

};

const getAllTransactions = async function () {
    try {
        return await information.getAllTransactions();
    } catch (error) {
        return '';
    }
};


export default {
    getAllServers,
    getServerByName,
    scaleInstance,
    getAllGroups,
    getGroupByName,
    getAllServices,
    getServiceByName,
    getServiceApi,
    getAllExecutables,
    getExecutableByAlias,
    getAllGateways,
    getAllTransactions
}





