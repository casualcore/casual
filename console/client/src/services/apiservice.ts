import http from "@/../http.config";
import { Gateway, Group, Server, Service, Transaction } from "@/models";
class ApiServiceImpl {
  async getAllGroups(): Promise<Group[]> {
    const groups: Group[] = [];
    try {
      const response = await http().get("/api/v1/groups/get");

      for (const g of response.data) {
        groups.push(new Group(g.name));
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
        servers.push(new Server(s.alias, s.note));
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
        executables.push(new Server(e.alias, e.note));
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
        services.push(new Service(s.name));
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
}

export const ApiService = new ApiServiceImpl();
