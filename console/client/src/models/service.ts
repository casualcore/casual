import { START_LOCATION } from 'vue-router';

export class Metric {

    private _min: string;
    public get min(): string {
        return this._min;
    }
    public set min(v: string) {
        this._min = v;
    }


    private _max: string;
    public get max(): string {
        return this._max;
    }
    public set max(v: string) {
        this._max = v;
    }

    private _avg: string;
    public get avg(): string {
        return this._avg;
    }
    public set avg(v: string) {
        this._avg = v;
    }


    private _total: string;
    public get total(): string {
        return this._total;
    }
    public set total(v: string) {
        this._total = v;
    }

    constructor(min = 0, max = 0, avg = 0, total = 0) {
        this._min = min.toFixed(3);
        this._max = max.toFixed(3);
        this._avg = avg.toFixed(3);
        this._total = total.toFixed(3)
    }

}

export class Service {



    private _name: string;
    public get name(): string {
        return this._name;
    }
    public set name(value: string) {
        this._name = value;
    }


    private _category: string;
    public get category(): string {
        return this._category;
    }
    public set category(v: string) {
        this._category = v;
    }

    
    private _parent : string;
    public get parent() : string {
        return this._parent;
    }
    public set parent(v : string) {
        this._parent = v;
    }
    
    private _called: number;
    public get called(): number {
        return this._called;
    }
    public set called(v: number) {
        this._called = v;
    }



    private _invoked: Metric;
    public get invoked(): Metric {
        return this._invoked;
    }
    public set invoked(v: Metric) {
        this._invoked = v;
    }


    private _pending: Metric;
    public get pending(): Metric {
        return this._pending;
    }
    public set pending(v: Metric) {
        this._pending = v;
    }
    
    private _last : string;
    public get last() : string {
        return this._last;
    }
    public set last(v : string) {
        this._last = v;
    }
    


    constructor(
        name = "",
        cat = "",
        called = 0,
        parent= "",
        inv = new Metric(),
        pen = new Metric(),
        last = ""
    ) {
        this._name = name;
        this._category = cat;
        this._parent = parent;
        this._called = called;
        this._invoked = inv;
        this._pending = pen;
        this._last = last;
    }

}

export class ServiceExtended extends Service {
    constructor(name = "") {
        super(name);
    }
}





