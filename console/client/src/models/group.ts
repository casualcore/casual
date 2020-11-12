export class Group {

  
    
    private _name : string;
    public get name() : string {
        return this._name;
    }
    public set name(v : string) {
        this._name = v;
    }
    
    private _note : string;
    public get note() : string {
        return this._note;
    }
    public set note(v : string) {
        this._note = v;
    }
    
    
    private _members : string[];
    public get members() : string[] {
        return this._members;
    }
    public set members(v : string[]) {
        this._members = v;
    }
    



  constructor(name = "", note = "", members = []) {
    this._name = name;
    this._note = note;
    this._members = members;
  }

}

export class GroupExtended extends Group {}
