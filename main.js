var binary = require('node-pre-gyp');
var path = require('path');
var binding_path = binary.find(path.resolve(path.join(__dirname,'./package.json')));
var binding = require(binding_path);


class CReadImage{
    constructor(imagePath, savepath){
        this.obj = new addons.CImageReader(imagePath, imagePath);
    }
}
window.CReadImage = CReadImage;
