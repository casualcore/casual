export class Transaction {
  private log: any = {};
  private pending: any = {};
  private resources = [];
  private transactions = [];

  constructor(log = {}, pen = {}, res = [], tran = []) {
    this.log = log;
    this.pending = pen;
    this.resources = res;
    this.transactions = tran;
  }
}
