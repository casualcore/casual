import { Group } from "@/models";

export class Server {
  
  private _alias : string;
  public get alias() : string {
      return this._alias;
  }
  public set alias(v : string) {
      this._alias = v;
  }
  
  private _note = "";
    public get note() {
        return this._note;
    }
    public set note(value) {
        this._note = value;
    }
  private _memberof: string[];
    public get memberof(): string[] {
        return this._memberof;
    }
    public set memberof(value: string[]) {
        this._memberof = value;
    }

  constructor(alias = "", note = "", memberof: string[] = []) {
    this._alias = alias;
    this._note = note;
    this._memberof = memberof;
  }

}


interface InstanceCount {
    configured: number[],
    running: number[]
}
const emptyIC: InstanceCount = {configured: [], running: []}

export class ServerExtended extends Server {


    
    private _restarts : number;
    public get restarts() : number {
        return this._restarts;
    }
    public set restarts(v : number) {
        this._restarts = v;
    }

    
    
    private _restart : boolean;
    public get restart() : boolean {
        return this._restart;
    }
    public set restart(v : boolean) {
        this._restart = v;
    }
    
    
    private _instancesCount : InstanceCount;
    public get instancesCount() : InstanceCount {
        return this._instancesCount;
    }
    public set instancesCount(v : InstanceCount) {
        this._instancesCount = v;
    }
    



  constructor(restarts = 0, restart = false, ic = emptyIC) {
    super();

    this._restarts = restarts;
    this._restart = restart;
    this._instancesCount = ic;
  }
}
