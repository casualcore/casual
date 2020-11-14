import http from "@/../http.config";
import { Gateway, Group, Server, Service, Transaction } from "@/models";
import { ServerExtended, ServiceExtended } from '@/models';
import { Metric } from '@/models/service';
import moment from "moment";

class ApiServiceImpl {
    async getAllGroups(): Promise<Group[]> {
        const groups: Group[] = [];
        try {
            const response = await http().get("/api/v1/groups/get");

            for (const g of response.data) {
                groups.push(new Group(g.name, g.note, g.members));
            }
        } catch (err) {
            console.log(err);
        }
        return groups;
    }
    async getAllServers(): Promise<Server[]> {
        const servers: Server[] = [];
        try {
            const response = await http().get("/api/v1/servers/get");
            for (const s of response.data) {
                const groups: string[] = [];
                for (const g of s.memberof) {
                    groups.push(g);
                }
                servers.push(new Server(s.alias, s.note, groups));
            }
        } catch (err) {
            console.log(err);
        }
        return servers;
    }
    async getAllExecutables(): Promise<Server[]> {
        const executables: Server[] = [];
        try {
            const response = await http().get("/api/v1/executables/get");
            for (const e of response.data) {
                executables.push(new Server(e.alias, e.note, e.memberof));
            }
        } catch (err) {
            console.log(err);
        }
        return executables;
    }
    async getAllServices(): Promise<Service[]> {
        const services: Service[] = [];
        try {
            const response = await http().get("/api/v1/services/get");
            const serviceState = response.data.services;
            for (const s of serviceState) {
                services.push(
                    new Service(
                        s.name,
                        s.category,
                        s.metric.invoked.count
                    )

                );
            }
        } catch (err) {
            console.log(err);
        }
        return services;
    }

    async getGateways(): Promise<Gateway> {
        let gateway: Gateway = new Gateway();

        try {
            const response = await http().get("/api/v1/gateways/get");
            const gatewayState = response.data;
            gateway = new Gateway(gatewayState.connections, gatewayState.listeners);
        } catch (err) {
            console.log(err);
        }
        return gateway;
    }

    async getTransactions(): Promise<Transaction> {
        let transaction: Transaction = new Transaction();
        try {
            const response = await http().get("/api/v1/transactions/get");
            const transactionState = response.data;
            transaction = new Transaction(
                transactionState.log,
                transactionState.pending,
                transactionState.resources,
                transactionState.transactions
            );
        } catch (err) {
            console.log(err);
        }
        return transaction;
    }


    async getServerByAlias(alias: string): Promise<ServerExtended> {
        const serverExt = new ServerExtended();
        try {
            const response = await http().get(`/api/v1/servers/get/server?alias=${alias}`)
            const serverState = response.data;
            serverExt.alias = serverState.alias;
            serverExt.note = serverState.note;
            serverExt.instancesCount = serverState.instances_count;
            serverExt.memberof = serverState.memberof;
            serverExt.restarts = serverState.restarts;
            serverExt.restart = serverState.restart;

            return serverExt
        } catch (error) {
            console.log(error);
        }
        return serverExt;
    }

    async getExecutableByAlias(alias: string): Promise<ServerExtended> {
        const execExt = new ServerExtended();
        try {
            const response = await http().get(`/api/v1/executables/get/executable?alias=${alias}`)
            const execState = response.data;
            execExt.alias = execState.alias;

            return execState
        } catch (error) {
            console.log(error);
        }
        return execExt;
    }
    async getServiceByName(name: string): Promise<ServiceExtended> {
        const serviceExt = new ServiceExtended();
        try {
            const response = await http().get(`/api/v1/services/get/service?name=${name}`)
            const s = response.data;
            const met = s.metric;

            serviceExt.called = met.invoked.count;
            serviceExt.name = s.name;
            serviceExt.category = s.category;
            serviceExt.parent = s.parent;

            const invoked = new Metric(
                met.invoked.min, met.invoked.max, met.invoked.avg, met.invoked.total
            )

            const pending = new Metric(
                met.pending.min, met.pending.max, met.pending.avg, met.pending.total
            )
            serviceExt.invoked = invoked;
            serviceExt.pending = pending;

            let last = "-";
            if (met.last > 0) {
                last = moment(met.last).format("YYYY-MM-DD hh:mm:ss");
            }
            serviceExt.last = last;

            return serviceExt
        } catch (error) {
            console.log(error);
        }
        return serviceExt;
    }
}

export const ApiService = new ApiServiceImpl();
