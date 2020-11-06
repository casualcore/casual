export class Gateway {
  private connections = [];
  private listensers = [];

  constructor(con = [], lis = []) {
    this.connections = con;
    this.listensers = lis;
  }
}
