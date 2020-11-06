export class Service {
  private name = "";

  constructor(name = "") {
    this.name = name;
  }

  public getName(): string {
    return this.name;
  }
}

export class ServiceExtended extends Service {
  constructor(name = "") {
    super(name);
  }
}
