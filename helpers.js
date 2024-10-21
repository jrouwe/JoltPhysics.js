Vec3.prototype['ToString'] = Vec3.prototype.ToString = function () { return `(${this.GetX()}, ${this.GetY()}, ${this.GetZ()})` };
Vec3.prototype['Clone'] = Vec3.prototype.Clone = function () { return new Vec3(this.GetX(), this.GetY(), this.GetZ()); };
RVec3.prototype['ToString'] = RVec3.prototype.ToString = function () { return `(${this.GetX()}, ${this.GetY()}, ${this.GetZ()})` };
RVec3.prototype['Clone'] = RVec3.prototype.Clone = function () { return new RVec3(this.GetX(), this.GetY(), this.GetZ()); };
Quat.prototype['ToString'] = Quat.prototype.ToString = function () { return `(${this.GetX()}, ${this.GetY()}, ${this.GetZ()}, ${this.GetW()})` };
Quat.prototype['Clone'] = Quat.prototype.Clone = function () { return new Quat(this.GetX(), this.GetY(), this.GetZ(), this.GetW()); };
AABox.prototype['ToString'] = AABox.prototype.ToString = function () { return `[${this.mMax.ToString()}, ${this.mMin.ToString()}]`; };
