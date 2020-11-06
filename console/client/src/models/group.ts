export class Group {
  private name = "";
  constructor(name = "") {
    this.name = name;
  }

  public getName(): string {
    return this.name;
  }
}

export class GroupExtended extends Group {}
