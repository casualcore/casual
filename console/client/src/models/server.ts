export class Server {
  private alias = "";
  private note = "";

  constructor(alias = "", note = "") {
    this.alias = alias;
    this.note = note;
  }

  public getAlias(): string {
    return this.alias;
  }
}

export class ServerExtended extends Server {
  constructor(alias = "") {
    super(alias);
  }
}
