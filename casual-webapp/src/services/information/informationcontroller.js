function getAllServers(body) {

    let responseBody = [];
    let servers = body.servers;
    let groups = body.groups;

    for (let i = 0; i < servers.length; i++) {
        let serverBody = servers[i];

        let memberships = servers[i].memberships;
        serverBody.membershipNames = getMembershipNameFromId(memberships, groups);
        responseBody.push(serverBody);

    }
    return responseBody;
}

function getServerByAlias(servers, alias) {

    for (let serve of servers) {
        if (serve.alias === alias) {
            return serve;
        }
    }

}


function getMembershipNameFromId(memberships, groups) {

    let membershipNames = [];
    for (let j = 0; j < memberships.length; j++) {
        for (let i = 0; i < groups.length; i++) {
            if (groups[i].id === memberships[j]) {
                membershipNames.push(groups[i].name);
            }
        }
    }
    return membershipNames;
}

function getAllExecutables(body) {
    let responseBody = [];
    let executables = body.executables;
    let groups = body.groups;

    for (let i = 0; i < executables.length; i++) {
        let executableBody = executables[i];

        let memberships = executables[i].memberships;
        executableBody.membershipNames = getMembershipNameFromId(memberships, groups);
        responseBody.push(executableBody);
    }
    return responseBody;
}

function getExecutableByAlias(executables, alias) {
    for (let exec of executables) {
        if (exec.alias === alias) {
            return exec;
        }
    }
    return "";
}


function getAllGroups(body) {

    let groups = body.groups;
    let servers = body.servers;
    let executables = body.executables;

    for (let i = 0; i < groups.length; i++) {
        let group = groups[i];

        group.members = getGroupMembers(group.id);
    }

    function getGroupMembers(id) {
        let groupMembers = {
            servers: [],
            executables: []
        };

        for (let i = 0; i < servers.length; i++) {
            let serverMemberships = servers[i].memberships;

            for (let membership of serverMemberships) {
                if (id === membership) {
                    groupMembers.servers.push(servers[i].alias);

                }
            }

        }

        for (let i = 0; i < executables.length; i++) {
            let serverMemberships = executables[i].memberships;

            for (let membership of serverMemberships) {
                if (id === membership) {
                    groupMembers.executables.push(executables[i].alias);

                }
            }

        }

        return groupMembers;

    }

    return groups;

}

function getGroupByName(body, name) {

    for (let group of body) {
        //console.log(group);
        if (group.name === name) {
            return group;
        }
    }

}

function getGroupById(groups, id) {

    for (let group of groups) {
        if (group.id === id) {
            return group.name;
        }
    }

}

function getGroupDependencyNames(groups, dependencies) {


    let dependencyNames = [];
    for (let dependency of dependencies) {
        dependencyNames.push(getGroupById(groups, dependency));
    }

    return dependencyNames;
}

function getServiceByName(servers, services, name) {

    let service = {};
    for (let s of services) {
        if (s.name === name) {
            service = s;
        }
    }

    service.parentServer = getParentServer(servers, service.instances);
    service.lastFormat = formatTime(service.last);
    return service;
}

function getServiceApiByName(services, name) {

    let api = {};
    for (let s of services) {
        if (s.service === name) {
            api = s;
        }
    }

    return api;
}

function getParentServer(servers, service) {
    let parent = {};
    for (let seq of service.sequential) {
        for (let server of servers) {
            if (server.instances.length > 0) {
                for (let instance of server.instances) {
                    if (seq.pid === instance.handle.pid) {
                        parent = server;
                        return parent;
                    }
                }
            }
        }


    }

    return parent;


}

function getAllServiceAverages(services) {
    for (let service of services) {
        service.metrics.average = getAverage(service.metrics);
        service.pending.average = getAverage(service.pending);
    }
    return services;

}

function getAverage(metric) {
    if (metric.count === 0) {
        return 0;
    } else {
        return metric.total / metric.count;
    }
}

function formatTime(nano) {
    if (nano > 0) {
        let million = 1000000;
        let milliseconds = nano / million;

        //  console.log(milliseconds);
        return milliseconds;
    } else {
        return "";
    }
}


export default {

    allServers : getAllServers,
    allGroups : getAllGroups,
    allExecutables : getAllExecutables,
    getGroupByName : getGroupByName,
    getGroupDependencyNames : getGroupDependencyNames,
    getServerByAlias : getServerByAlias,
    getServiceByName : getServiceByName,
    getExecutableByAlias : getExecutableByAlias,
    getAllServiceAverages : getAllServiceAverages,
    getServiceApiByName : getServiceApiByName

}

